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
}

ModelManager::~ModelManager( void )
{
    // We assume there is no more streaming activity.

    for ( ModelResource *model : this->models )
    {
        streaming.UnlinkResource( model->id );

        delete model;
    }

    this->models.clear();
    this->modelByName.clear();
    this->modelByID.clear();

    streaming.UnregisterResourceType( MODEL_ID_BASE );
}

void ModelManager::RegisterResource(
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
    vfs::DevicePtr resDevice;
    std::string devPath;
    {
        resDevice = theGame->FindDevice( relFilePath, devPath );
    }

    if ( resDevice == nullptr )
    {
        // No device means we do not care.
        return;
    }

    ModelResource *modelEntry = new ModelResource( resDevice, std::move( devPath ) );

    modelEntry->id = id;
    modelEntry->texDictID = -1;
    modelEntry->lodDistance = lodDistance;
    modelEntry->flags = flags;

    streaming.LinkResource( id, name, &modelEntry->vfsResLoc );

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
    this->modelByID.insert( std::make_pair( id, modelEntry ) );

    this->models.push_back( modelEntry );
}

ModelManager::ModelResource* ModelManager::GetModelByID( streaming::ident_t id )
{
    auto findIter = this->modelByID.find( id );

    if ( findIter == this->modelByID.end() )
        return NULL;

    return findIter->second;
}

void ModelManager::LoadResource( streaming::ident_t localID, const void *dataBuf, size_t memSize )
{
    ModelResource *modelEntry = this->models[ localID ];

    // TODO: hook up with aap's stuff.
}

void ModelManager::UnloadResource( streaming::ident_t localID )
{
    ModelResource *modelEntry = this->models[ localID ];

    // TODO: hook up with aap's stuff.
}

size_t ModelManager::GetObjectMemorySize( streaming::ident_t localID ) const
{
    // TODO.
    return 0;
}

}