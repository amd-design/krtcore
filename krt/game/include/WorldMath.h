#pragma once

// Important math about camera frustum and world sectoring.

#define MATH_INLINE __forceinline

//#define USE_MATH_VERIFICATION

namespace krt
{
namespace math
{

struct Plane
{
    rw::V3d pos;
    rw::V3d a;
    rw::V3d b;

    bool intersectWith( const Plane& right ) const;
};

struct TrianglePlane
{
    rw::V3d pos;
    rw::V3d a;
    rw::V3d b;

    bool intersectWith( const TrianglePlane& right ) const;
};

struct Sphere
{
    rw::V3d point;
    float radius;

    bool intersectLine( const rw::V3d& pos, const rw::V3d& dir, double& first, double& second ) const;
};

// Don't mind the German names. :)
struct Quader
{
    inline Quader(
        rw::V3d brl, rw::V3d brr, rw::V3d bfl, rw::V3d bfr,
        rw::V3d trl, rw::V3d trr, rw::V3d tfl, rw::V3d tfr
    );

    bool isPointInside( const rw::V3d& point ) const;
    bool intersectWith( const Quader& right ) const;
    bool intersectWith( const Sphere& right ) const;

    // Please do not write into those fields directly!
    rw::V3d brl, brr, bfl, bfr; // bottom plane.
    rw::V3d trl, trr, tfl, tfr; // top plane.

private:
    rw::Matrix inv_localSpace_bottom, inv_localSpace_top;

    rw::Matrix leftPlane_inv_planeSpace_bottom, leftPlane_inv_planeSpace_top;
    rw::Matrix rightPlane_inv_planeSpace_bottom, rightPlane_inv_planeSpace_top;
    rw::Matrix topPlane_inv_planeSpace_bottom, topPlane_inv_planeSpace_top;
    rw::Matrix bottomPlane_inv_planeSpace_bottom, bottomPlane_inv_planeSpace_top;
    rw::Matrix frontPlane_inv_planeSpace_bottom, frontPlane_inv_planeSpace_top;
    rw::Matrix rearPlane_inv_planeSpace_bottom, rearPlane_inv_planeSpace_top;
};

// this is actually a plane. get your air miles today!
struct SimplePlane
{
	inline SimplePlane()
		: normal(0.0f, 0.0f, 0.0f), dist(0.0f)
	{

	}

	inline SimplePlane(rw::V3d normal, float dist)
		: normal(normal), dist(dist)
	{

	}

	inline SimplePlane(float a, float b, float c, float d)
		: SimplePlane(rw::V3d(a, b, c), d)
	{

	}

	inline void normalize()
	{
		float factor = 1.0f / rw::length(normal);

		normal.x *= factor;
		normal.y *= factor;
		normal.z *= factor;
		dist *= factor;
	}

	rw::V3d normal;
	float dist;
};

struct Frustum
{
	Frustum(const rw::Matrix& matrix);

	bool isPointInside(const rw::V3d& point) const;

	// assumes it's axis-aligned, silly Martin with his weird 'ingenuity'
	bool intersectWith(const Quader& right) const;
	bool intersectWith(const Sphere& right) const;

public:
	SimplePlane planes[6];
	rw::V3d corners[8];

private:
	void createCorners();
};

// A function to test our math.
// Should succeed, eh.
void Tests( void );

}
};