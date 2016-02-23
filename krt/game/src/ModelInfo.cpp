#include "StdInc.h"
#include "ModelInfo.h"

#include "vfs\Manager.h"
#include "Game.h"

#include "NativePerfLocks.h"

namespace krt
{

ModelManager::ModelManager( streaming::StreamMan& streaming, TextureManager& texManager ) : streaming( streaming ), texManager( texManager ), curModelId (0)
{
    bool didRegister = streaming.RegisterResourceType( MODEL_ID_BASE, MAX_MODELS, this );

    assert( didRegister == true );

    this->models.resize( MAX_MODELS );

	this->loadAllModelsCommand = std::make_unique<ConsoleCommand>("load_all_models", [=] ()
	{
		this->LoadAllModels();
	});
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
    this->basifierLookup.clear();
    this->lodifierLookup.clear();

    streaming.UnregisterResourceType( MODEL_ID_BASE );
}

streaming::ident_t ModelManager::RegisterAtomicModel(
    std::string name, std::string texDictName, float lodDistance, int flags,
    std::string absFilePath
)
{
	// Get an identifier
	streaming::ident_t id = (curModelId.fetch_add(1)) + MODEL_ID_BASE;

    // Check whether we already have a model that goes by that name.
    {
        auto findIter = this->modelByName.find( name );

        if ( findIter != this->modelByName.end() )
        {
            // The name is already taken, so bail.
            return findIter->second->GetID();
        }
    }

    // Get the device this model is bound to.
    std::string devPath = ( absFilePath );

    vfs::DevicePtr resDevice = vfs::GetDevice( devPath );

    if ( resDevice == nullptr )
    {
        // No device means we do not care.
        return -1;
    }

    // Check whether this resource even exists.
    if ( resDevice->GetLength( devPath ) == -1 )
    {
        // Does not exist, I think.
        return -1;
    }

    ModelResource *modelEntry = new ModelResource( resDevice, std::move( devPath ) );

    modelEntry->manager = this;

    modelEntry->id = id;
    modelEntry->texDictID = -1;
    modelEntry->lodDistance = lodDistance;
    modelEntry->flags = flags;
    modelEntry->modelPtr = NULL;
    modelEntry->modelType = eModelType::ATOMIC;
    modelEntry->lod_model = NULL;

    modelEntry->lockModelLoading = SRWLOCK_INIT;

    bool couldLink = streaming.LinkResource( id, name, &modelEntry->vfsResLoc );

    if ( !couldLink )
    {
        // The resource does not really exist I guess.
        delete modelEntry;

        return -1;
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

    // Check whether we are a LOD model.
    bool isLODModel = false;

    if ( name.size() >= 4 )
    {
        // We are only interresting for LOD matching if our name is longer than three characters.
        std::string lod_id = name.substr( 3 );

        if ( strncmp( name.c_str(), "LOD", 3 ) == 0 )
        {
            isLODModel = true;
        }

        // If we are a LOD model, we have to check whether there already is a model that would want this as LOD instance.
        // Otherwise we have to check whether there already is a LOD model that would map to us.
        if ( isLODModel )
        {
            // DO NOTE that we lookup for the last model that registered itself as base here.
            // There can be multiple that map to the same base id in the IDE files, and we have to be
            // careful about that if we ever want to support unregistering of models.
            auto findBaseModel = this->basifierLookup.find( lod_id );

            if ( findBaseModel != this->basifierLookup.end() )
            {
                // We found a base model to map to!
                ModelResource *baseModelEntry = findBaseModel->second;

                baseModelEntry->lod_model = modelEntry;
            }
        }
        else
        {
            // Check whether a LOD model already exists that should map to us.
            auto findLODModel = this->lodifierLookup.find( lod_id );

            if ( findLODModel != this->lodifierLookup.end() )
            {
                // Good catch. There is a LOD model already that should be mapped to us.
                ModelResource *lodModelEntry = findLODModel->second;

                modelEntry->lod_model = lodModelEntry;
            }
        }

        // Register us for lookup.
        if ( isLODModel )
        {
            // Register us in the LOD lookup map.
            this->lodifierLookup.insert( std::make_pair( std::move( lod_id ), modelEntry ) );
        }
        else
        {
            // We are not a LOD model, so lets register us as base model.
            this->basifierLookup.insert( std::make_pair( std::move( lod_id ), modelEntry ) );
        }
    }

    // Store us. :)
    this->modelByName.insert( std::make_pair( name, modelEntry ) );

    this->models[ id - MODEL_ID_BASE ] = modelEntry;

    // Success!
	return id;
}

// TODO: maybe allow unregistering of models.

ModelManager::ModelResource* ModelManager::GetModelByID( streaming::ident_t id )
{
    if ( id < 0 || id >= MAX_MODELS )
        return NULL;

    return this->models[ id ];
}

ModelManager::ModelResource* ModelManager::GetModelByName( const std::string& name )
{
	auto findIter = this->modelByName.find(name);

	if (findIter != this->modelByName.end())
	{
		return findIter->second;
	}

	return NULL;
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