#include "StdInc.h"
#include "Entity.h"

#include "Game.h"

namespace krt
{

Entity::Entity( void )
{
    this->modelID = -1;
    this->matrix.setIdentity();
    this->rwObject = NULL;
}

Entity::~Entity( void )
{

}

void Entity::SetModelIndex( streaming::ident_t modelID )
{
    // Make sure we have no RW object when switching models
    // This is required because models are not entirely thread-safe if not following certain rules.
    assert( this->rwObject == NULL );

    this->modelID = modelID;
}

bool Entity::CreateRWObject( void )
{
    streaming::ident_t modelID = this->modelID;

    if ( modelID == -1 )
        return false;

    // If we already have an atomic, we refuse to update.
    if ( this->rwObject )
        return false;

    // Fetch a model from the model manager.
    Game *game = theGame;

    ModelManager::ModelResource *modelEntry = game->GetModelManager().GetModelByID( modelID );

    if ( !modelEntry )
        return false;

    rw::Object *rwobj = modelEntry->CloneModel();

    if ( !rwobj )
        return false;

    this->rwObject = rwobj;

    return true;
}

void Entity::DeleteRWObject( void )
{
    rw::Object *rwobj = this->rwObject;

    if ( !rwobj )
        return;

    Game *game = theGame;

    ModelManager::ModelResource *modelEntry = game->GetModelManager().GetModelByID( this->modelID );

    if ( !modelEntry )
        return;

    modelEntry->ReleaseModel( rwobj );

    this->rwObject = NULL;
}

void Entity::SetMatrix( const rw::Matrix& mat )
{
    this->matrix = mat;
}

const rw::Matrix& Entity::GetMatrix( void ) const
{
    return this->matrix;
}

}