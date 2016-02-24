#include "StdInc.h"
#include "Camera.h"

// Camera is responsible for setting up the rendering context.
#include <windows.h>
#include <d3d9.h>

// Camera class to abstract away world rendering.

namespace krt
{

Camera::Camera( void )
{
    // Default members.
    this->aspect_ratio = 1.0f;
    this->fov = 65.0f;
    this->isRendering = false;
    this->rendering_context = NULL;

    // We are not initialized at first.
    this->rwCamera = NULL;
    this->camFrame = NULL;
}

Camera::~Camera( void )
{
    this->Shutdown();
}

void Camera::Initialize( void )
{
    if ( this->rwCamera )
        return; // already initialized.

    // Initialize us.
    this->rwCamera = rw::Camera::create();

    assert( this->rwCamera != NULL );

    this->rwCamera->farPlane = 1000.f;
    this->rwCamera->nearPlane = 0.5f;

    rw::Frame *camFrame = rw::Frame::create();

    assert( camFrame != NULL );

    rw::Matrix cam_mat;
    cam_mat.setIdentity();
    cam_mat.pos = rw::V3d( -15, 100, -1890 );
    cam_mat.posw = 1;
    
    camFrame->matrix = cam_mat;
    camFrame->updateObjects();

    this->camFrame = camFrame;

    this->rwCamera->setFrame( camFrame );
}

void Camera::Shutdown( void )
{
    // Finish any rendering if it was still going on.
    if ( this->isRendering )
    {
        // We really do not want to leak rendering, so notify the user.
        assert( this->isRendering == false );

        this->EndUpdate();
    }

    if ( rw::Camera *myCam = this->rwCamera )
    {
        myCam->destroy();

        this->rwCamera = NULL;
    }

    if ( rw::Frame *myFrame = this->camFrame )
    {
        myFrame->destroy();

        this->camFrame = NULL;
    }
}

void Camera::BeginUpdate( void *gpuContext )
{
    // Must be initialized first.
    assert( this->rwCamera != NULL );

    assert( this->isRendering == false );

    if ( this->isRendering )
        return;

	// perform rendering
	IDirect3DDevice9* device = reinterpret_cast<IDirect3DDevice9*>(gpuContext);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(255, 0, 0x99, 0xff), 1.0f, 0);

	device->BeginScene();

    // Prepare things with some basic render states.
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
	device->SetRenderState(D3DRS_ALPHATESTENABLE, 1);
	device->SetRenderState(D3DRS_ALPHAREF, 128);
	device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
	device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

    // Set projection stuff.
    {
	    rw::Matrix viewmat;
	    rw::Matrix::invert(&viewmat, camFrame->getLTM());
	    viewmat.right.x = -viewmat.right.x;
	    viewmat.rightw = 0.0;
	    viewmat.up.x = -viewmat.up.x;
	    viewmat.upw = 0.0;
	    viewmat.at.x = -viewmat.at.x;
	    viewmat.atw = 0.0;
	    viewmat.pos.x = -viewmat.pos.x;
	    viewmat.posw = 1.0;
	    device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&viewmat);
	    device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)rwCamera->projMat);
    }

    this->rendering_context = gpuContext;

    this->isRendering = true;
}

void Camera::EndUpdate( void )
{
    // Must be initialized first.
    assert( this->rwCamera != NULL );

    assert( this->isRendering == true );

    if ( !this->isRendering )
        return;

    // Finish rendering and present.
	IDirect3DDevice9* device = reinterpret_cast<IDirect3DDevice9*>( this->rendering_context );

    device->EndScene();

	device->Present(nullptr, nullptr, nullptr, nullptr);

    // Clean up rendering meta-data.
    this->rendering_context = NULL;

    this->isRendering = false;
}

void Camera::SetFOV( float fov_radians )
{
    // Must be initialized first.
    assert( this->rwCamera != NULL );

    this->fov = fov_radians;

    // Update RW camera.
    this->rwCamera->setFOV( fov_radians, this->aspect_ratio );
    this->rwCamera->updateProjectionMatrix();
}

float Camera::GetFOV( void ) const
{
    return this->fov;
}

void Camera::SetAspectRatio( float aspect_ratio )
{
    // Must be initialized first.
    assert( this->rwCamera != NULL );

    this->aspect_ratio = aspect_ratio;

    // Update RW camera.
    this->rwCamera->setFOV( this->fov, aspect_ratio );
    this->rwCamera->updateProjectionMatrix();
}

float Camera::GetAspectRatio( void ) const
{
    return this->aspect_ratio;
}

void Camera::SetFarClip( float farclip )
{
    // Must be initialized first.
    assert( this->rwCamera != NULL );

    this->rwCamera->farPlane = farclip;

    this->rwCamera->updateProjectionMatrix();
}

float Camera::GetFarClip( void ) const
{
    return this->rwCamera->farPlane;
}

void Camera::SetViewMatrix( const rw::Matrix& viewMat )
{
    // Must be initialized first.
    assert( this->rwCamera != NULL );

    // We do not want to change camera position during rendering.
    assert( this->isRendering == false );

    this->camFrame->matrix = viewMat;
    this->camFrame->updateObjects();
}

const rw::Matrix& Camera::GetViewMatrix( void ) const
{
    // Must be initialized first.
    assert( this->rwCamera != NULL );

    return *this->camFrame->getLTM();
}

inline math::Frustum getCameraFrustum( rw::Camera *camera )
{
	rw::Matrix viewMat;
	rw::Matrix cameraWorldMat = *camera->getFrame()->getLTM();
	rw::Matrix::invert(&viewMat, &cameraWorldMat);

	// why??? ask aap.
	rw::Matrix cameraProjMat;
	memcpy(&cameraProjMat, camera->projMat, sizeof(rw::Matrix));

	rw::Matrix viewProj;
	rw::Matrix::mult(&viewProj, &cameraProjMat, &viewMat);

	return math::Frustum(viewProj);
}

inline math::Quader getCameraFrustumQuader( rw::Camera *camera )
{
    math::Frustum opt_frust = getCameraFrustum( camera );

    // Construct a view frustum.
    return math::Quader(
        opt_frust.corners[2], opt_frust.corners[3], opt_frust.corners[6], opt_frust.corners[7], // bottom plane
        opt_frust.corners[1], opt_frust.corners[0], opt_frust.corners[5], opt_frust.corners[4]  // top plane
    );
}

math::Frustum Camera::GetSimpleFrustum( void ) const
{
    // This frustum promises to be fast while sacrificing unimportant correctness guarrantees.

    // Must be initialized first.
    assert( this->rwCamera != NULL );

    // TODO: cache the frustum.

    return getCameraFrustum( this->rwCamera );
}

math::Quader Camera::GetComplexFrustum( void ) const
{
    // This frustum promises to deliver correct results.

    // Must be initialized first.
    assert( this->rwCamera != NULL );

    // TODO: cache the frustum.

    return getCameraFrustumQuader( this->rwCamera );
}

};