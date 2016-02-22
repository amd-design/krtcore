#include "StdInc.h"
#include "World.h"

#include "QuadTree.h"

namespace krt
{

World::World( void )
{
    // A world can store a lot of entities.
    LIST_CLEAR( this->entityList.root );

    QuadTree <3, 6000, int> tree;

    tree.root.VisitByPoint( 200, 200,
        []( int& ok )
    {
        ok = 1337;
    });
}

World::~World( void )
{
    // Unlink all entities from the world.
    {
        LIST_FOREACH_BEGIN( Entity, this->entityList.root, worldNode )
            
            item->onWorld = NULL;

        LIST_FOREACH_END

        LIST_CLEAR( this->entityList.root );
    }
}

// TODO: add more cool world stuff.

void World::PutEntitiesOnGrid( void )
{

}

}