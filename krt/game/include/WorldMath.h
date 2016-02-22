#pragma once

// Important math about camera frustum and world sectoring.

#define MATH_INLINE __forceinline

namespace krt
{
namespace math
{

struct Plane
{
    rw::V3d pos;
    rw::V3d a;
    rw::V3d b;

    inline bool intersectWith( const Plane& right ) const;
};

struct Sphere
{
    rw::V3d point;
    float radius;

    inline bool intersectLine( const rw::V3d& pos, const rw::V3d& dir, double& first, double& second ) const;
};

// Don't mind the German names. :)
struct Quader
{
    inline Quader(
        rw::V3d brl, rw::V3d brr, rw::V3d bfl, rw::V3d bfr,
        rw::V3d trl, rw::V3d trr, rw::V3d tfl, rw::V3d tfr
    );

    inline bool isPointInside( const rw::V3d& point ) const;
    inline bool intersectWith( const Quader& right ) const;
    inline bool intersectWith( const Sphere& right ) const;

    // Please do not write into those fields directly!
    const rw::V3d brl, brr, bfl, bfr; // bottom plane.
    const rw::V3d trl, trr, tfl, tfr; // top plane.
};

// A function to test our math.
// Should succeed, eh.
void Tests( void );

}
};