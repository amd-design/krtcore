#include "StdInc.h"
#include "World.h"

#include "QuadTree.h"

#include "Game.h"

#include "Console.CommandHelpers.h"

namespace krt
{

static ConsoleCommand cmdWorldStreamTest( "wsector_test",
    [&]( void )
{
    Game *theGame = krt::theGame;

    World *theWorld = theGame->GetWorld();

    theWorld->PutEntitiesOnGrid();
});

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
    return rw::V3d( rwvec.x, rwvec.z, rwvec.y );
}

inline math::Quader getCameraFrustum( rw::Camera *camera, float fov_angle )
{
    // Calculate it ourselves.
    float farPlaneDist = camera->farPlane;
    float nearPlaneDist = camera->nearPlane;

    double fov = DEG2RAD(fov_angle);
    double ar = 16.0f / 9.0f;

    double Hnear = 2 * tan( fov / 2 ) * nearPlaneDist;
    double Wnear = Hnear * ar;

    double Hfar = 2 * tan( fov / 2 ) * farPlaneDist;
    double Wfar = Hfar * ar;

    rw::Matrix cameraWorldMat = *camera->getFrame()->getLTM();

    float half_Wnear = (float)( Wnear / 2 );
    float half_Hnear = (float)( Hnear / 2 );
    
    float half_Wfar = (float)( Wfar / 2 );
    float half_Hfar = (float)( Hfar / 2 );

    rw::V3d brl = cameraWorldMat.transPoint( rw::V3d( -half_Wnear, -half_Hnear, nearPlaneDist ) );
    rw::V3d brr = cameraWorldMat.transPoint( rw::V3d(  half_Wnear, -half_Hnear, nearPlaneDist ) );
    rw::V3d trl = cameraWorldMat.transPoint( rw::V3d( -half_Wnear,  half_Hnear, nearPlaneDist ) );
    rw::V3d trr = cameraWorldMat.transPoint( rw::V3d(  half_Wnear,  half_Hnear, nearPlaneDist ) );
    rw::V3d bfl = cameraWorldMat.transPoint( rw::V3d( -half_Wfar, -half_Hfar, farPlaneDist ) );
    rw::V3d bfr = cameraWorldMat.transPoint( rw::V3d(  half_Wfar, -half_Hfar, farPlaneDist ) );
    rw::V3d tfl = cameraWorldMat.transPoint( rw::V3d( -half_Wfar,  half_Hfar, farPlaneDist ) );
    rw::V3d tfr = cameraWorldMat.transPoint( rw::V3d(  half_Wfar,  half_Hfar, farPlaneDist ) );

    // Construct a view frustum.
    return math::Quader( corvec( brl ), corvec( brr ), corvec( bfl ), corvec( bfr ), corvec( trl ), corvec( trr ), corvec( tfl ), corvec( tfr ) );
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

        this->staticEntityGrid.PutEntity( item );

    LIST_FOREACH_END
}

static ConsoleCommand putGridCmd( "pgrid",
    []( void )
{
    theGame->GetWorld()->DepopulateEntities();

    theGame->GetWorld()->PutEntitiesOnGrid();

    printf( "populated static entity grid\n" );
});

#include <windows.h>
#include <d3d9.h>

void World::RenderWorld( void *gfxDevice )
{
    // Thank you Bas for figuring out that rendering gunk!

	// perform rendering
	IDirect3DDevice9* device = reinterpret_cast<IDirect3DDevice9*>(gfxDevice);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(255, 0, 0x99, 0xff), 1.0f, 0);

	device->BeginScene();

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
	device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

    // Create a test frustum and see how it intersects with things.
    rw::Camera *testCamera = rw::Camera::create();

    assert( testCamera != NULL );

    testCamera->farPlane = 600.0f;
    testCamera->nearPlane = 1.0f;
    //testCamera->projection = rw::Camera::PERSPECTIVE;

    float fov_angle = 65.0f;

    testCamera->setFOV( fov_angle, 16.0f / 9.0f );

    rw::Frame *camFrame = rw::Frame::create();

    assert( camFrame != NULL );

    rw::Matrix cam_mat;
    cam_mat.setIdentity();
    cam_mat.pos = rw::V3d( -15, 100, -1890 );
    cam_mat.posw = 1;
    
    camFrame->matrix = cam_mat;
    camFrame->updateObjects();

    testCamera->setFrame( camFrame );

    testCamera->updateProjectionMatrix();

    {
		rw::V3d cameraPosition(500.0f, 50.0f, 100.0f);
		rw::V3d objectPosition(0.0f, 0.0f, 100.0f);

		rw::Frame* frame = testCamera->getFrame();

		if (frame)
		{
			rw::V3d forward = rw::normalize(rw::sub(objectPosition, cameraPosition));
			rw::V3d left = rw::normalize(rw::cross(rw::V3d(0.0f, 0.0f, 1.0f), forward));
			rw::V3d up = rw::cross(forward, left);

			frame->matrix.right = left;
			frame->matrix.up = up;
			frame->matrix.at = forward;
			frame->matrix.pos = cameraPosition;
			frame->updateObjects();
		}
    }

	rw::Matrix viewmat;
	rw::Matrix::invert(&viewmat, testCamera->getFrame()->getLTM());
	//viewmat.right.x = -viewmat.right.x;
	viewmat.rightw = 0.0;
	//viewmat.up.x = -viewmat.up.x;
	viewmat.upw = 0.0;
	//viewmat.at.x = -viewmat.at.x;
	viewmat.atw = 0.0;
	//viewmat.pos.x = -viewmat.pos.x;
	viewmat.posw = 1.0;
	device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&viewmat);
	device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)testCamera->projMat);

    // Get its frustum.
    math::Quader frustum = getCameraFrustum( testCamera, fov_angle );

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

    testCamera->destroy();

    testCamera = NULL;

    camFrame->destroy();

    camFrame = NULL;

    device->EndScene();

	device->Present(nullptr, nullptr, nullptr, nullptr);

    // OK.
}

}