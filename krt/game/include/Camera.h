#pragma once

#include "WorldMath.h"

namespace krt
{

class Camera
{
public:
	Camera(void);
	~Camera(void);

	// Call this when RW has initialized.
	void Initialize(void);

	// Call this to clean up things.
	void Shutdown(void);

	void BeginUpdate(void* gpuContext);
	void EndUpdate(void);

	void SetFOV(float fov_radians);
	float GetFOV(void) const;

	void SetAspectRatio(float ar);
	float GetAspectRatio(void) const;

	void SetFarClip(float farclip);
	float GetFarClip(void) const;

	void SetViewMatrix(const rw::Matrix& matView);
	const rw::Matrix& GetViewMatrix(void) const;

	math::Frustum GetSimpleFrustum(void) const;
	math::Quader GetComplexFrustum(void) const;

	rw::Frame* GetRWFrame(void) { return this->camFrame; }
	rw::Camera* GetRWCamera(void) { return this->rwCamera; }

private:
	bool isRendering;
	void* rendering_context;

	rw::Frame* camFrame;
	rw::Camera* rwCamera;

	// Camera settings.
	float fov;
	float aspect_ratio;
};
};