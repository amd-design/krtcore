#include "StdInc.h"
#include "ModelInfo.h"

#include "vfs\Manager.h"

#include "Game.h"

namespace krt
{

ModelManager::ModelManager( streaming::StreamMan& streaming, TextureManager& texManager ) : streaming( streaming ), texManager( texManager )
{
    bool didRegister = streaming.RegisterResourceType( MODEL_ID_BASE, MAX_MODELS, this );

    assert( didRegister == true );

    this->models.resize( MAX_MODELS );
}

ModelManager::~ModelManager( void )
{
    // We assume there is no more streaming activity.

    for ( ModelResource *model : this->models )
    {
        if ( model != NULL )
        {
            streaming.UnlinkResource( model->id );

            delete model;
        }
    }

    this->models.clear();
    this->modelByName.clear();

    streaming.UnregisterResourceType( MODEL_ID_BASE );
}

void ModelManager::RegisterAtomicModel(
    streaming::ident_t id,
    std::string name, std::string texDictName, float lodDistance, int flags,
    std::string relFilePath
)
{
    // Check whether that id is taken already.
    {
        ModelResource *alreadyTaken = this->GetModelByID( id );

        if ( alreadyTaken )
        {
            // Cannot take this id because occupied, meow.
            return;
        }
    }

    // Get the device this model is bound to.
    std::string pathPrefix = theGame->GetDevicePathPrefix();

    std::string devPath = ( pathPrefix + relFilePath );

    vfs::DevicePtr resDevice = vfs::GetDevice( devPath );

    if ( resDevice == nullptr )
    {
        // No device means we do not care.
        return;
    }

    ModelResource *modelEntry = new ModelResource( resDevice, std::move( devPath ) );

    modelEntry->manager = this;

    modelEntry->id = id;
    modelEntry->texDictID = -1;
    modelEntry->lodDistance = lodDistance;
    modelEntry->flags = flags;
    modelEntry->modelPtr = NULL;
    modelEntry->modelType = eModelType::ATOMIC;

    bool couldLink = streaming.LinkResource( id, name, &modelEntry->vfsResLoc );

    if ( !couldLink )
    {
        // The resource does not really exist I guess.
        delete modelEntry;

        return;
    }

    // Find the texture dictionary that should link with this model.
    streaming::ident_t texDictID = -1;
    {
        texDictID = texManager.FindTexDict( texDictName );

        if ( texDictID != -1 )
        {
            // Try to link it.
            // If it does not work, then discard this id.
            bool couldLink = streaming.AddResourceDependency( id, texDictID );

            if ( !couldLink )
            {
                texDictID = -1;
            }
        }
    }

    if ( texDictID != -1 )
    {
        modelEntry->texDictID = texDictID;
    }

    // Store us. :)
    this->modelByName.insert( std::make_pair( name, modelEntry ) );

    this->models[ id - MODEL_ID_BASE ] = modelEntry;

    // Success!
}

ModelManager::ModelResource* ModelManager::GetModelByID( streaming::ident_t id )
{
    if ( id < 0 || id >= MAX_MODELS )
        return NULL;

    shared_lock_acquire <std::shared_timed_mutex> ctxGetModel( this->lockModelContext );

    return this->models[ id ];
}

void ModelManager::LoadAllModels( void )
{
    // Request all models and wait for them to load.
    for ( ModelResource *model : this->models )
    {
        if ( model != NULL )
        {
            streaming.Request( model->id );
        }
    }

    // Wait for things to finish ;)
    streaming.LoadingBarrier();
}

rw::Object* ModelManager::ModelResource::CloneModel( void )
{
    exclusive_lock_acquire <std::shared_timed_mutex> ctxCloneModel( this->manager->lockModelContext );

    rw::Object *modelItem = this->modelPtr;

    if ( modelItem == NULL )
        return NULL;

    rw::uint8 modelType = modelItem->type;

    if ( modelType == rw::Atomic::ID )
    {
        rw::Atomic *atomic = (rw::Atomic*)modelItem;

        return (rw::Object*)atomic->clone();
    }
    else if ( modelType == rw::Clump::ID )
    {
        rw::Clump *clump = (rw::Clump*)modelItem;

        return (rw::Object*)clump->clone();
    }

    return NULL;
}

static rw::Atomic* GetFirstClumpAtomic( rw::Clump *clump )
{
    rw::Atomic *firstAtom = NULL;

    rw::clumpForAllAtomics( clump,
        [&]( rw::Atomic *atom )
    {
        if ( !firstAtom )
        {
            firstAtom = atom;
        }
    });

    return firstAtom;
}

void ModelManager::LoadResource( streaming::ident_t localID, const void *dataBuf, size_t memSize )
{
    exclusive_lock_acquire <std::shared_timed_mutex> ctxLoadModel( this->lockModelContext );

    ModelResource *modelEntry = this->models[ localID ];

    assert( modelEntry != NULL );

    // Load the model resource.
    rw::Object *modelPtr = NULL;
    {
        rw::StreamMemory memoryStream;
        memoryStream.open( (rw::uint8*)dataBuf, (rw::uint32)memSize );

        bool foundModel = rw::findChunk( &memoryStream, rw::ID_CLUMP, NULL, NULL );

        if ( !foundModel )
        {
            throw std::exception( "not a model resource" );
        }

        rw::Clump *newClump = rw::Clump::streamRead( &memoryStream );

        if ( !newClump )
        {
            throw std::exception( "failed to parse model file" );
        }

        // Process it so that it is in the model format that we want.
        eModelType modelType = modelEntry->modelType;

        if ( modelType == eModelType::ATOMIC )
        {
            // We just store an atomic.
            rw::Atomic *firstAtom = GetFirstClumpAtomic( newClump );

            if ( firstAtom )
            {
                rw::Atomic *clonedObject = firstAtom->clone();

                if ( clonedObject )
                {
                    modelPtr = (rw::Object*)clonedObject;
                }
            }

            newClump->destroy();
        }
        else if ( modelType == eModelType::VEHICLE ||
                  modelType == eModelType::PED )
        {
            // Just store it as clump.
            modelPtr = (rw::Object*)newClump;
        }
        else
        {
            assert( 0 );
        }

        if ( modelPtr == NULL )
        {
            throw std::exception( "invalid model file" );
        }
    }

    // Store us. :)
    modelEntry->modelPtr = modelPtr;
}

void ModelManager::UnloadResource( streaming::ident_t localID )
{
    exclusive_lock_acquire <std::shared_timed_mutex> ctxUnloadModel( this->lockModelContext );

    ModelResource *modelEntry = this->models[ localID ];

    assert( modelEntry != NULL );

    // Delete GPU data.
    {
        assert( modelEntry->modelPtr != NULL );

        rw::uint8 modelType = modelEntry->modelPtr->type;

        if ( modelType == rw::Atomic::ID )
        {
            rw::Atomic *atomic = (rw::Atomic*)modelEntry->modelPtr;

            atomic->destroy();
        }
        else if ( modelType == rw::Clump::ID )
        {
            rw::Clump *clump = (rw::Clump*)modelEntry->modelPtr;

            clump->destroy();
        }
        else
        {
            assert( 0 );
        }
    }

    modelEntry->modelPtr = NULL;
}

size_t ModelManager::GetObjectMemorySize( streaming::ident_t localID ) const
{
    // TODO.
    return 0;
}

}