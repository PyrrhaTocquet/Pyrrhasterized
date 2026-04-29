/*
author: Pyrrha Tocquet
date: 26/04/26
desc: Contains helper functions and structs for working with frustums
*/
module frustum;

namespace Frustum
{
    public struct Plane
    {
        float3 N; // normal
        float d;  // distance to origin
    };

    public struct Frustum
    {
        public Plane planes[4]; // lleft, right, top, bottom
    }

    public Plane PlaneFromPoints(float3 p0, float3 p1, float3 p2)
    {
        Plane retPlane;

        // two vectors from the plane
        float3 v0 = p1 - p0;
        float3 v2 = p2 - p0;

        // ortho to plane -> normal
        retPlane.N = normalize(cross(v0, v2));

        retPlane.d = dot(retPlane.N, p0);

        return retPlane;
    }
}

