#include <StdInc.h>
#include <Game.h>
#include <Streaming.h>

#include <windows.h>
#include <d3d9.h>

#include <Console.Variables.h>

namespace krt
{
void RenderingTest(void* gfxDevice)
{
	static std::unique_ptr<ConVar<std::string>> defaultRenderModel = std::make_unique<ConVar<std::string>>("default_render_model", ConVar_Archive, "ind_maindrag2");

	static bool modelChanged = false;
	static std::string modelName = defaultRenderModel->GetValue();

	static ConsoleCommand renderModel("render_model", [] (const std::string& newModelName)
	{
		if (!theGame->GetModelManager().GetModelByName(newModelName))
		{
			console::PrintError("No such model: %s\n", newModelName.c_str());
			return;
		}

		modelName = newModelName;
		modelChanged = true;
	});

	// handle original model data
	auto model = theGame->GetModelManager().GetModelByName(modelName);

    if ( !model )
        return;

	auto id = model->GetID();

	static streaming::ident_t lastID;

	bool loaded = false;
	static rw::Atomic* modelPtr = nullptr;

	if (theGame->GetStreaming().GetResourceStatus(id) == streaming::StreamMan::eResourceStatus::LOADED)
	{
		loaded = true;

		if (modelChanged)
		{
			theGame->GetStreaming().Unload(lastID);

			modelPtr->destroy();
			modelPtr = nullptr;

			modelChanged = false;
		}

		if (!modelPtr)
		{
			modelPtr = (rw::Atomic*)model->CloneModel();
			modelPtr->setFrame(rw::Frame::create());

			lastID = id;
		}
	}
	else if (theGame->GetStreaming().GetResourceStatus(id) == streaming::StreamMan::eResourceStatus::UNLOADED)
	{
		theGame->GetStreaming().Request(id);
	}

	// perform rendering
	IDirect3DDevice9* device = reinterpret_cast<IDirect3DDevice9*>(gfxDevice);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(255, 0, 0x99, 0xff), 1.0f, 0);

	device->BeginScene();

	if (modelPtr)
	{
		static float angle = 0.0f;
		
		angle += 10.0f * theGame->GetDelta();

		if (angle >= 360.0f)
		{
			angle -= 360.0f;
		}

		modelPtr->getFrame()->matrix = rw::Matrix::makeRotation(rw::Quat::rotation(angle * (3.14159265f / 180), rw::V3d(0.0f, 0.0f, 1.0f)));
		modelPtr->getFrame()->updateObjects();

		static rw::Camera* camera = rw::Camera::create();

		if (!camera->getFrame())
		{
			camera->setFrame(rw::Frame::create());
		}

		camera->nearPlane = 0.5f;
		camera->farPlane = 300.f;
		camera->setFOV(65.0f, 16.0f / 9.0f);
		camera->updateProjectionMatrix();

		rw::V3d cameraPosition(-50.0f, -50.0f, 30.0f);
		rw::V3d objectPosition(0.0f, 0.0f, 0.0f);

		rw::Frame* frame = camera->getFrame();

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

		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
		device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

		rw::Matrix viewmat;
		rw::Matrix::invert(&viewmat, camera->getFrame()->getLTM());
		viewmat.right.x = -viewmat.right.x;
		viewmat.rightw = 0.0;
		viewmat.up.x = -viewmat.up.x;
		viewmat.upw = 0.0;
		viewmat.at.x = -viewmat.at.x;
		viewmat.atw = 0.0;
		viewmat.pos.x = -viewmat.pos.x;
		viewmat.posw = 1.0;
		device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&viewmat);
		device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)camera->projMat);

		modelPtr->render();
	}

	device->EndScene();

	device->Present(nullptr, nullptr, nullptr, nullptr);
}
}