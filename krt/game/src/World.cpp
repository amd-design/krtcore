#include "StdInc.h"
#include "World.h"

#include "QuadTree.h"

#include "Game.h"

#include "Console.CommandHelpers.h"

#include "Camera.h"

namespace krt
{

World::World( void )
{
    // A world can store a lot of entities.
    LIST_CLEAR( this->entityList.root );
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

#define DEG2RAD( x ) (x*3.14159265/180)

inline rw::V3d corvec( rw::V3d rwvec )
{
    return rwvec;
}

void World::DepopulateEntities( void )
{
    // Remove all entity world links for the entities we know.
    LIST_FOREACH_BEGIN( Entity, this->entityList.root, worldNode )
    
        item->RemoveEntityFromWorldSectors();

    LIST_FOREACH_END
}

void World::PutEntitiesOnGrid( void )
{
    // Try putting all the world entities on the grid.
    LIST_FOREACH_BEGIN( Entity, this->entityList.root, worldNode )

		// ignore LODs
		if (item->GetModelInfo() && item->GetModelInfo()->GetLODDistance() < 300.0f)
		{
			this->staticEntityGrid.PutEntity(item);
		}

    LIST_FOREACH_END
}

static ConsoleCommand putGridCmd( "pgrid",
    []( void )
{
    theGame->GetWorld()->DepopulateEntities();

    theGame->GetWorld()->PutEntitiesOnGrid();

    printf( "populated static entity grid\n" );
});

static ConsoleCommand clearGridCmd( "cgrid",
    []( void )
{
    theGame->GetWorld()->DepopulateEntities();

    printf( "removed entities from static grid\n" );
});

void World::RenderWorld( void *gfxDevice )
{
    // Thank you Bas for figuring out that rendering gunk!

    // Create a test frustum and see how it intersects with things.

    // Set some cool camera direction.
    Camera& worldCam = theGame->GetWorldCamera();
    {
        // Crash in release mode at those coors in gta3
		//rw::V3d cameraPosition(500.0f, 50.0f, 100.0f);
		//rw::V3d objectPosition(500.0f, 0.0f, 100.0f);

		rw::V3d cameraPosition(500.0f, 50.0f, 100.0f);
		rw::V3d objectPosition(0.0f, 50.0f, 100.0f);

#if 0
        // Bas' settings.
		rw::V3d cameraPosition(500.0f, 50.0f, 100.0f);
		rw::V3d objectPosition(0.0f, 0.0f, 0.0f);
#endif

		rw::V3d forward = rw::normalize(rw::sub(objectPosition, cameraPosition));
		rw::V3d left = rw::normalize(rw::cross(rw::V3d(0.0f, 0.0f, 1.0f), forward));
		rw::V3d up = rw::cross(forward, left);

        rw::Matrix viewMat;
        viewMat.setIdentity();

		viewMat.right = left;
		viewMat.up = up;
		viewMat.at = forward;
		viewMat.pos = cameraPosition;
		
        worldCam.SetViewMatrix( viewMat );
    }

    // Begin the rendering.
    worldCam.BeginUpdate( gfxDevice );

    // Get its frustum.
    // For now it does not matter which; both promise that things that should be visible are indeed visible.
    // The difference is that the complex frustum culls away more objects than the simple frustum.
    //math::Frustum frustum = worldCam.GetSimpleFrustum();
    math::Quader frustum = worldCam.GetComplexFrustum();

    streaming::StreamMan& streaming = theGame->GetStreaming();

    // Visit the things that are visible.
    this->staticEntityGrid.VisitSectorsByFrustum( frustum,
        [&]( StaticEntitySector& sector )
    {
        // Render all entities on this sector.
        for ( Entity *entity : sector.entitiesOnSector )
        {
            rw::Sphere worldSphere;

            bool hasEntityBounds = entity->GetWorldBoundingSphere( worldSphere );

            if ( hasEntityBounds )
            {
                math::Sphere math_sphere;
                math_sphere.point = worldSphere.center;
                math_sphere.radius = worldSphere.radius;

                if ( frustum.intersectWith( math_sphere ) )
                {
                    // Entity is visible.
                    // Request a model for this entity.
                    {
                        ModelManager::ModelResource *entityModel = entity->GetModelInfo();

                        if ( entityModel )
                        {
                            streaming::ident_t streaming_id = entityModel->GetID();

                            if ( streaming.GetResourceStatus( streaming_id ) != streaming::StreamMan::eResourceStatus::LOADED )
                            {
                                streaming.Request( streaming_id );
                            }
                        }
                    }

                    entity->CreateRWObject();

                    // If the entity has a valid rw object, we can render it.
                    if ( rw::Object *rwobj = entity->GetRWObject() )
                    {
                        if ( rwobj->type == rw::Atomic::ID )
                        {
                            // We only render atomics for now.
                            rw::Atomic *atomic = (rw::Atomic*)rwobj;

                            atomic->render();
                        }
                    }
                }
            }
        }
    });

    // Present world scene.
    worldCam.EndUpdate();

    // OK.
}

}