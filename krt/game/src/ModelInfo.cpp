#include "StdInc.h"
#include "ModelInfo.h"

namespace krt
{

ModelManager::ModelManager( streaming::StreamMan& streaming ) : streaming( streaming )
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
    this->modelMap.clear();

    streaming.UnregisterResourceType( MODEL_ID_BASE );
}

void ModelManager::RegisterResource( std::string name, vfs::DevicePtr device, std::string pathToRes )
{
    ModelResource *modelEntry = new ModelResource( device, std::move( pathToRes ) );

    streaming::ident_t curID = MODEL_ID_BASE + ( this->models.size() );

    modelEntry->id = curID;

    streaming.LinkResource( curID, name, &modelEntry->vfsResLoc );

    // Store us. :)
    this->modelMap.insert( std::make_pair( name, modelEntry ) );

    this->models.push_back( modelEntry );
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