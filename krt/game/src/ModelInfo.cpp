#include "StdInc.h"
#include "ModelInfo.h"

#include "vfs\Manager.h"
#include "Game.h"

#include "NativePerfLocks.h"

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

    modelEntry->lockModelLoading = SRWLOCK_INIT;

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
    rw::Object *modelItem = this->modelPtr;

    if ( modelItem == NULL )
        return NULL;

    NativeSRW_Exclusive ctxCloneModel( this->lockModelLoading );

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

void ModelManager::ModelResource::NativeReleaseModel( rw::Object *rwobj )
{
    rw::uint8 modelType = rwobj->type;

    if ( modelType == rw::Atomic::ID )
    {
        rw::Atomic *atomic = (rw::Atomic*)rwobj;

        atomic->destroy();
    }
    else if ( modelType == rw::Clump::ID )
    {
        rw::Clump *clump = (rw::Clump*)rwobj;

        clump->destroy();
    }
    else
    {
        assert( 0 );
    }
}

void ModelManager::ModelResource::ReleaseModel( rw::Object *rwobj )
{
    // Release the resource under our lock.
    NativeSRW_Exclusive ctxReleaseModel( this->lockModelLoading );

    NativeReleaseModel( rwobj );
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
    ModelResource *modelEntry = this->models[ localID ];

    assert( modelEntry != NULL );

    NativeSRW_Exclusive ctxLoadModel( modelEntry->lockModelLoading );

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

        // Set the current TXD.
        {
            streaming::ident_t txdID = modelEntry->texDictID;

            if ( txdID != -1 )
            {
                // TODO: this does not work because this thing is a global variable.
                // It breaks multi-threaded loading, which is a real shame.
                // Solution: wait till tomorrow so that aap has built in some texture lookup function!
                texManager.SetCurrentTXD( txdID );
            }
        }

        try
        {
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
        catch( ... )
        {
            texManager.UnsetCurrentTXD();

            throw;
        }

        texManager.UnsetCurrentTXD();
    }

    // Store us. :)
    modelEntry->modelPtr = modelPtr;
}

void ModelManager::UnloadResource( streaming::ident_t localID )
{
    ModelResource *modelEntry = this->models[ localID ];

    assert( modelEntry != NULL );

    NativeSRW_Exclusive ctxUnloadModel( modelEntry->lockModelLoading );

    // Delete GPU data.
    {
        assert( modelEntry->modelPtr != NULL );

        rw::Object *rwobj = modelEntry->modelPtr;

        ModelResource::NativeReleaseModel( rwobj );
    }

    modelEntry->modelPtr = NULL;
}

size_t ModelManager::GetObjectMemorySize( streaming::ident_t localID ) const
{
    // TODO.
    return 0;
}

}