#include "StdInc.h"
#include "World.h"

#include "QuadTree.h"

#include "Game.h"

#include "Console.CommandHelpers.h"

#include "Camera.h"
#include "CameraControls.h"

#include "KeyBinding.h"

#include "fonts/FontRenderer.h"

namespace krt
{

void RenderGfxConsole();

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
		//if (item->GetModelInfo() && item->GetModelInfo()->GetLODDistance() < 300.0f)
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

        static bool hasInitializedWorldCam = false;

        if ( !hasInitializedWorldCam )
        {
            worldCam.SetFarClip( 2150 );

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

            hasInitializedWorldCam = true;
        }

#if 0
        // Do a little test, meow.
        uint64_t cur_time = theGame->GetGameTime();

        unsigned int cur_divisor = ( ( cur_time / 500 ) % 2 );

        static EditorCameraControls editorControls;

        editorControls.SetYawVelocity( 40 );

        if ( cur_divisor == 0 )
        {
            editorControls.SetPitchVelocity( 40 );
        }
        else if ( cur_divisor == 1 )
        {
            editorControls.SetPitchVelocity( -40 );
        }

        editorControls.OnFrame( &worldCam );
#endif

#if 1
		static EditorCameraControls editorControls;

		static Button forwardButton("forward");
		static Button backButton("back");
		static Button moveLeftButton("moveleft");
		static Button moveRightButton("moveright");
		static Button lookLeftButton("lookleft");
		static Button lookRightButton("lookright");
		static Button lookUpButton("lookup");
		static Button lookDownButton("lookdown");

		static ConVar<bool> m_filter("m_filter", ConVar_Archive, false);
		static ConVar<float> m_sensitivity("sensitivity", ConVar_Archive, 5.0f);
		static ConVar<float> m_accel("m_accel", ConVar_Archive, 0.0f);
		static ConVar<float> m_yaw("m_yaw", ConVar_Archive, 0.022f);
		static ConVar<float> m_pitch("m_pitch", ConVar_Archive, 0.022f);

		static EventListener<MouseEvent> eventListener([] (const MouseEvent* event)
		{
			// calculate angle movement
			float mx = static_cast<float>(event->GetDeltaX());
			float my = static_cast<float>(event->GetDeltaY());

			// historical values for filtering
			static int mouseDx[2];
			static int mouseDy[2];
			static int mouseIndex;

			mouseDx[mouseIndex] = event->GetDeltaX();
			mouseDy[mouseIndex] = event->GetDeltaY();

			mouseIndex ^= 1;

			// filter input
			if (m_filter.GetValue())
			{
				mx = (mouseDx[0] + mouseDx[1]) * 0.5f;
				my = (mouseDy[0] + mouseDy[1]) * 0.5f;
			}

			float rate = rw::length(rw::V2d(mx, my)) / theGame->GetLastFrameTime();
			float factor = m_sensitivity.GetValue() + (rate * m_accel.GetValue());

			mx *= factor;
			my *= factor;

			// apply angles to camera
			krt::Camera& camera = theGame->GetWorldCamera();
			
			editorControls.AddViewAngles(&camera, mx * m_yaw.GetValue(), my * -(m_pitch.GetValue()));
		});

		static std::once_flag bindingFlag;

		std::call_once(bindingFlag, [] ()
		{
			console::ExecuteSingleCommand(ProgramArguments{ "bind", "W", "+forward" });
			console::ExecuteSingleCommand(ProgramArguments{ "bind", "S", "+back" });
			console::ExecuteSingleCommand(ProgramArguments{ "bind", "A", "+moveleft" });
			console::ExecuteSingleCommand(ProgramArguments{ "bind", "D", "+moveright" });
			console::ExecuteSingleCommand(ProgramArguments{ "bind", "Up", "+lookup" });
			console::ExecuteSingleCommand(ProgramArguments{ "bind", "Down", "+lookdown" });
			console::ExecuteSingleCommand(ProgramArguments{ "bind", "Left", "+lookleft" });
			console::ExecuteSingleCommand(ProgramArguments{ "bind", "Right", "+lookright" });
		});

		auto bindButton = [&] (const Button& addButton, const Button& subButton, auto& setFunction, auto& stopFunction, float speed)
		{
			if (addButton.IsDown())
			{
				setFunction(speed * addButton.GetPressedFraction());
			}
			else if (subButton.IsDown())
			{
				setFunction(-speed * subButton.GetPressedFraction());
			}
			else
			{
				stopFunction();
			}
		};

#define BUTTON_MACRO(b, sb, n, s) \
	bindButton(b, sb, std::bind(&EditorCameraControls::Set##n, &editorControls, std::placeholders::_1), std::bind(&EditorCameraControls::Stop##n, &editorControls), s)

		BUTTON_MACRO(forwardButton, backButton, FrontVelocity, 30.0f);

		BUTTON_MACRO(moveLeftButton, moveRightButton, RightVelocity, 30.0f);

		BUTTON_MACRO(lookRightButton, lookLeftButton, YawVelocity, 18.0f);

		BUTTON_MACRO(lookUpButton, lookDownButton, PitchVelocity, 10.0f);

#undef BUTTON_MACRO

		editorControls.OnFrame(&worldCam);
#endif
    }

    // Begin the rendering.
    worldCam.BeginUpdate( gfxDevice );

    // Get its frustum.
    // For now it does not matter which; both promise that things that should be visible are indeed visible.
    // The difference is that the complex frustum culls away more objects than the simple frustum.
    math::Frustum frustum = worldCam.GetSimpleFrustum();
    //math::Quader frustum = worldCam.GetComplexFrustum();

    streaming::StreamMan& streaming = theGame->GetStreaming();

	rw::V3d cameraPos = worldCam.GetRWFrame()->getLTM()->pos;

	std::vector<Entity*> renderList;
	renderList.reserve(5000);

	LIST_FOREACH_BEGIN(Entity, this->entityList.root, worldNode)

		item->ResetChildrenDrawn();

	LIST_FOREACH_END

    // Visit the things that are visible.
    this->staticEntityGrid.VisitSectorsByFrustum( frustum,
        [&]( StaticEntitySector& sector )
    {
        // Render all entities on this sector.
        for ( Entity *entity : sector.entitiesOnSector )
        {
            rw::Sphere worldSphere;

			rw::V3d entityPos = entity->GetMatrix().pos;
			float entityDistance = rw::length(rw::sub(entityPos, cameraPos));

			auto modelInfo = entity->GetModelInfo();

			if (entityDistance > modelInfo->GetLODDistance())
			{
				continue;
			}
			
			if (entityDistance < modelInfo->GetMinimumDistance())
			{
				continue;
			}

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
						if (entity->GetLODEntity())
						{
							entity->GetLODEntity()->IncrementChildrenDrawn();
						}

						renderList.push_back(entity);
                    }
                }
            }
        }
    });

	for (auto& entity : renderList)
	{
		if (entity->ShouldBeDrawn())
		{
			rw::Object* rwobj = entity->GetRWObject();

			if (rwobj->type == rw::Atomic::ID)
			{
				// We only render atomics for now.
				rw::Atomic *atomic = (rw::Atomic*)rwobj;

				atomic->render();
			}
		}
	}

	// Render 2D things.
	RenderGfxConsole();

	TheFonts->DrawPerFrame();

    // Present world scene.
    worldCam.EndUpdate();

    // OK.
}

}