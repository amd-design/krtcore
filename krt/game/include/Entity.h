#pragma once

#include "Streaming.h"
#include "ModelInfo.h"

// Entity class for having actual objects in the world of things.

namespace krt
{
class Game;

struct Entity
{
	friend class Game;
    friend struct World;
    friend class FileLoader;
	friend struct inst_section_manager; // leaky abstraction?

    Entity( Game *ourGame );
    ~Entity();

    void SetModelIndex( streaming::ident_t modelID );

    bool CreateRWObject( void );
    void DeleteRWObject( void );

    void SetModelling( const rw::Matrix& mat );
    const rw::Matrix& GetModelling( void ) const;
    const rw::Matrix& GetMatrix( void ) const;

    bool GetWorldBoundingSphere( rw::Sphere& sphere ) const;

    void LinkToWorld( World *theWorld );
    World* GetWorld( void );

    void SetLODEntity( Entity *lodInst );
    Entity* GetLODEntity( void );

    bool IsLowerLODOf( Entity *inst ) const;
    bool IsHigherLODOf( Entity *inst ) const;

    ModelManager::ModelResource* GetModelInfo( void ) const;

    inline Game* GetGame( void ) const          { return this->ourGame; }

private:
    Game *ourGame;

    NestedListEntry <Entity> gameNode;

    streaming::ident_t modelID;

    int interiorId;

    // Some entity flags given by IPL.
    bool isUnderwater;
    bool isTunnelObject;
    bool isTunnelTransition;
    bool isUnimportantToStreamer;

    rw::Matrix matrix;
    bool hasLocalMatrix;

    rw::Object *rwObject;

    Entity *higherQualityEntity;
    Entity *lowerQualityEntity;

    // Node of the world entity list.
    NestedListEntry <Entity> worldNode;

    World *onWorld;
};

}