#pragma once

#include "Streaming.h"

// Entity class for having actual objects in the world of things.

namespace krt
{

struct Entity
{
    Entity( void );
    ~Entity();

    void SetModelIndex( streaming::ident_t modelID );

    bool CreateRWObject( void );
    void DeleteRWObject( void );

    void SetMatrix( const rw::Matrix& mat );
    const rw::Matrix& GetMatrix( void ) const;

    //todo.

private:
    streaming::ident_t modelID;

    rw::Matrix matrix;
    rw::Object *rwObject;
};

}