//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

//-----------------------------------------------------------------------------
//
//	Environment maps
//
//-----------------------------------------------------------------------------

#include "ImfEnvmap.h"
#include <Imath/ImathFun.h>
#include "ImfNamespace.h"

#include <algorithm>
#include <math.h>

using namespace std;
using namespace IMATH_NAMESPACE;

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER

namespace LatLongMap
{

V2f
latLong (const V3f& dir)
{
    float r = sqrt (dir.z * dir.z + dir.x * dir.x);

    float latitude = (r < abs (dir.y)) ? acos (r / dir.length ()) * sign (dir.y)
                                       : asin (dir.y / dir.length ());

    float longitude = (dir.z == 0 && dir.x == 0) ? 0 : atan2 (dir.x, dir.z);

    return V2f (latitude, longitude);
}

V2f
latLong (const Box2i& dataWindow, const V2f& pixelPosition)
{
    float latitude, longitude;

    if (dataWindow.max.y > dataWindow.min.y)
    {
        latitude = -1 * float (M_PI) *
                   ((pixelPosition.y - dataWindow.min.y) /
                        (dataWindow.max.y - dataWindow.min.y) -
                    0.5f);
    }
    else
    {
        latitude = 0;
    }

    if (dataWindow.max.x > dataWindow.min.x)
    {
        longitude = -2 * float (M_PI) *
                    ((pixelPosition.x - dataWindow.min.x) /
                         (dataWindow.max.x - dataWindow.min.x) -
                     0.5f);
    }
    else
    {
        longitude = 0;
    }

    return V2f (latitude, longitude);
}

V2f
pixelPosition (const Box2i& dataWindow, const V2f& latLong)
{
    float x = latLong.y / (-2 * float (M_PI)) + 0.5f;
    float y = latLong.x / (-1 * float (M_PI)) + 0.5f;

    return V2f (
        x * (dataWindow.max.x - dataWindow.min.x) + dataWindow.min.x,
        y * (dataWindow.max.y - dataWindow.min.y) + dataWindow.min.y);
}

V2f
pixelPosition (const Box2i& dataWindow, const V3f& direction)
{
    return pixelPosition (dataWindow, latLong (direction));
}

V3f
direction (const Box2i& dataWindow, const V2f& pixelPosition)
{
    V2f ll = latLong (dataWindow, pixelPosition);

    return V3f (sin (ll.y) * cos (ll.x), sin (ll.x), cos (ll.y) * cos (ll.x));
}

} // namespace LatLongMap

namespace CubeMap
{

int
sizeOfFace (const Box2i& dataWindow)
{
    return min (
        (dataWindow.max.x - dataWindow.min.x + 1),
        (dataWindow.max.y - dataWindow.min.y + 1) / 6);
}

Box2i
dataWindowForFace (CubeMapFace face, const Box2i& dataWindow)
{
    int   sof = sizeOfFace (dataWindow);
    Box2i dwf;

    dwf.min.x = 0;
    dwf.min.y = int (face) * sof;

    dwf.max.x = dwf.min.x + sof - 1;
    dwf.max.y = dwf.min.y + sof - 1;

    return dwf;
}

V2f
pixelPosition (CubeMapFace face, const Box2i& dataWindow, V2f positionInFace)
{
    Box2i dwf = dataWindowForFace (face, dataWindow);
    V2f   pos (0, 0);

    switch (face)
    {
        case CUBEFACE_POS_X:

            pos.x = dwf.min.x + positionInFace.y;
            pos.y = dwf.max.y - positionInFace.x;
            break;

        case CUBEFACE_NEG_X:

            pos.x = dwf.max.x - positionInFace.y;
            pos.y = dwf.max.y - positionInFace.x;
            break;

        case CUBEFACE_POS_Y:

            pos.x = dwf.min.x + positionInFace.x;
            pos.y = dwf.max.y - positionInFace.y;
            break;

        case CUBEFACE_NEG_Y:

            pos.x = dwf.min.x + positionInFace.x;
            pos.y = dwf.min.y + positionInFace.y;
            break;

        case CUBEFACE_POS_Z:

            pos.x = dwf.max.x - positionInFace.x;
            pos.y = dwf.max.y - positionInFace.y;
            break;

        case CUBEFACE_NEG_Z:

            pos.x = dwf.min.x + positionInFace.x;
            pos.y = dwf.max.y - positionInFace.y;
            break;
    }

    return pos;
}

void
faceAndPixelPosition (
    const V3f& direction, const Box2i& dataWindow, CubeMapFace& face, V2f& pif)
{
    int   sof  = sizeOfFace (dataWindow);
    float absx = abs (direction.x);
    float absy = abs (direction.y);
    float absz = abs (direction.z);

    if (absx >= absy && absx >= absz)
    {
        if (absx == 0)
        {
            //
            // Special case - direction is (0, 0, 0)
            //

            face = CUBEFACE_POS_X;
            pif  = V2f (0, 0);
            return;
        }

        pif.x = (direction.y / absx + 1) / 2 * (sof - 1);
        pif.y = (direction.z / absx + 1) / 2 * (sof - 1);

        if (direction.x > 0)
            face = CUBEFACE_POS_X;
        else
            face = CUBEFACE_NEG_X;
    }
    else if (absy >= absz)
    {
        pif.x = (direction.x / absy + 1) / 2 * (sof - 1);
        pif.y = (direction.z / absy + 1) / 2 * (sof - 1);

        if (direction.y > 0)
            face = CUBEFACE_POS_Y;
        else
            face = CUBEFACE_NEG_Y;
    }
    else
    {
        pif.x = (direction.x / absz + 1) / 2 * (sof - 1);
        pif.y = (direction.y / absz + 1) / 2 * (sof - 1);

        if (direction.z > 0)
            face = CUBEFACE_POS_Z;
        else
            face = CUBEFACE_NEG_Z;
    }
}

V3f
direction (CubeMapFace face, const Box2i& dataWindow, const V2f& positionInFace)
{
    int sof = sizeOfFace (dataWindow);

    V2f pos;

    if (sof > 1)
    {
        pos = V2f (
            positionInFace.x / (sof - 1) * 2 - 1,
            positionInFace.y / (sof - 1) * 2 - 1);
    }
    else
    {
        pos = V2f (0, 0);
    }

    V3f dir (1, 0, 0);

    switch (face)
    {
        case CUBEFACE_POS_X:

            dir.x = 1;
            dir.y = pos.x;
            dir.z = pos.y;
            break;

        case CUBEFACE_NEG_X:

            dir.x = -1;
            dir.y = pos.x;
            dir.z = pos.y;
            break;

        case CUBEFACE_POS_Y:

            dir.x = pos.x;
            dir.y = 1;
            dir.z = pos.y;
            break;

        case CUBEFACE_NEG_Y:

            dir.x = pos.x;
            dir.y = -1;
            dir.z = pos.y;
            break;

        case CUBEFACE_POS_Z:

            dir.x = pos.x;
            dir.y = pos.y;
            dir.z = 1;
            break;

        case CUBEFACE_NEG_Z:

            dir.x = pos.x;
            dir.y = pos.y;
            dir.z = -1;
            break;
    }

    return dir;
}

} // namespace CubeMap

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT
