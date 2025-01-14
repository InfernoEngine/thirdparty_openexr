//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Contributors to the OpenEXR Project.
//

#ifndef INCLUDED_IMF_ENVMAP_H
#define INCLUDED_IMF_ENVMAP_H

//-----------------------------------------------------------------------------
//
//	Environment maps
//
//	Environment maps define a mapping from 3D directions to 2D
//	pixel space locations.  Environment maps are typically used
//	in 3D rendering, for effects such as quickly approximating
//	how shiny surfaces reflect their environment.
//
//	Environment maps can be stored in scanline-based or in tiled
//	OpenEXR files.  The fact that an image is an environment map
//	is indicated by the presence of an EnvmapAttribute whose name
//	is "envmap". (Convenience functions to access this attribute
//	are defined in header file ImfStandardAttributes.h.)
//	The attribute's value defines the mapping from 3D directions
//	to 2D pixel space locations.
//
//	This header file defines the set of possible EnvmapAttribute
//	values.
//
//	For each possible EnvmapAttribute value, this header file also
//	defines a set of convenience functions to convert between 3D
//	directions and 2D pixel locations.
//
//	Most of the convenience functions defined below require a
//	dataWindow parameter.  For scanline-based images, and for
//	tiled images with level mode ONE_LEVEL, the dataWindow
//	parameter should be set to the image's data window, as
//	defined in the image header.  For tiled images with level
//	mode MIPMAP_LEVELS or RIPMAP_LEVELS, the data window of the
//	image level that is being accessed should be used instead.
//	(See the dataWindowForLevel() methods in ImfTiledInputFile.h
//	and ImfTiledOutputFile.h.)
//
//-----------------------------------------------------------------------------

#include "ImfExport.h"
#include "ImfNamespace.h"

#include <Imath/ImathBox.h>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER

//--------------------------------
// Supported environment map types
//--------------------------------

enum IMF_EXPORT_ENUM Envmap : int
{
    ENVMAP_LATLONG = 0, // Latitude-longitude environment map
    ENVMAP_CUBE    = 1, // Cube map

    NUM_ENVMAPTYPES // Number of different environment map types
};

//-------------------------------------------------------------------------
// Latitude-Longitude Map:
//
// The environment is projected onto the image using polar coordinates
// (latitude and longitude).  A pixel's x coordinate corresponds to
// its longitude, and the y coordinate corresponds to its latitude.
// Pixel (dataWindow.min.x, dataWindow.min.y) has latitude +pi/2 and
// longitude +pi; pixel (dataWindow.max.x, dataWindow.max.y) has
// latitude -pi/2 and longitude -pi.
//
// In 3D space, latitudes -pi/2 and +pi/2 correspond to the negative and
// positive y direction.  Latitude 0, longitude 0 points into positive
// z direction; and latitude 0, longitude pi/2 points into positive x
// direction.
//
// The size of the data window should be 2*N by N pixels (width by height),
// where N can be any integer greater than 0.
//-------------------------------------------------------------------------

namespace LatLongMap
{
//----------------------------------------------------
// Convert a 3D direction to a 2D vector whose x and y
// components represent the corresponding latitude
// and longitude.
//----------------------------------------------------

IMF_EXPORT
IMATH_NAMESPACE::V2f latLong (const IMATH_NAMESPACE::V3f& direction);

//--------------------------------------------------------
// Convert the position of a pixel to a 2D vector whose
// x and y components represent the corresponding latitude
// and longitude.
//--------------------------------------------------------

IMF_EXPORT
IMATH_NAMESPACE::V2f latLong (
    const IMATH_NAMESPACE::Box2i& dataWindow,
    const IMATH_NAMESPACE::V2f&   pixelPosition);

//-------------------------------------------------------------
// Convert a 2D vector, whose x and y components represent
// longitude and latitude, into a corresponding pixel position.
//-------------------------------------------------------------

IMF_EXPORT
IMATH_NAMESPACE::V2f pixelPosition (
    const IMATH_NAMESPACE::Box2i& dataWindow,
    const IMATH_NAMESPACE::V2f&   latLong);

//-----------------------------------------------------
// Convert a 3D direction vector into a corresponding
// pixel position.  pixelPosition(dw,dir) is equivalent
// to pixelPosition(dw,latLong(dw,dir)).
//-----------------------------------------------------

IMF_EXPORT
IMATH_NAMESPACE::V2f pixelPosition (
    const IMATH_NAMESPACE::Box2i& dataWindow,
    const IMATH_NAMESPACE::V3f&   direction);

//--------------------------------------------------------
// Convert the position of a pixel in a latitude-longitude
// map into a corresponding 3D direction.
//--------------------------------------------------------

IMF_EXPORT
IMATH_NAMESPACE::V3f direction (
    const IMATH_NAMESPACE::Box2i& dataWindow,
    const IMATH_NAMESPACE::V2f&   pixelPosition);
} // namespace LatLongMap

//--------------------------------------------------------------
// Cube Map:
//
// The environment is projected onto the six faces of an
// axis-aligned cube.  The cube's faces are then arranged
// in a 2D image as shown below.
//
//          2-----------3
//         /           /|
//        /           / |       Y
//       /           /  |       |
//      6-----------7   |       |
//      |           |   |       |
//      |           |   |       |
//      |   0       |   1       *------- X
//      |           |  /       /
//      |           | /       /
//      |           |/       /
//      4-----------5       Z
//
//   dataWindow.min
//        /
//       /
//      +-----------+
//      |3    Y    7|
//      |     |     |
//      |     |     |
//      |  ---+---Z |  +X face
//      |     |     |
//      |     |     |
//      |1         5|
//      +-----------+
//      |6    Y    2|
//      |     |     |
//      |     |     |
//      | Z---+---  |  -X face
//      |     |     |
//      |     |     |
//      |4         0|
//      +-----------+
//      |6    Z    7|
//      |     |     |
//      |     |     |
//      |  ---+---X |  +Y face
//      |     |     |
//      |     |     |
//      |2         3|
//      +-----------+
//      |0         1|
//      |     |     |
//      |     |     |
//      |  ---+---X |  -Y face
//      |     |     |
//      |     |     |
//      |4    Z    5|
//      +-----------+
//      |7    Y    6|
//      |     |     |
//      |     |     |
//      | X---+---  |  +Z face
//      |     |     |
//      |     |     |
//      |5         4|
//      +-----------+
//      |2    Y    3|
//      |     |     |
//      |     |     |
//      |  ---+---X |  -Z face
//      |     |     |
//      |     |     |
//      |0         1|
//      +-----------+
//                 /
//                /
//          dataWindow.max
//
// The size of the data window should be N by 6*N pixels
// (width by height), where N can be any integer greater
// than 0.
//
//--------------------------------------------------------------

//------------------------------------
// Names for the six faces of the cube
//------------------------------------

enum IMF_EXPORT_ENUM CubeMapFace
{
    CUBEFACE_POS_X, // +X face
    CUBEFACE_NEG_X, // -X face
    CUBEFACE_POS_Y, // +Y face
    CUBEFACE_NEG_Y, // -Y face
    CUBEFACE_POS_Z, // +Z face
    CUBEFACE_NEG_Z  // -Z face
};

namespace CubeMap
{
//---------------------------------------------
// Width and height of a cube's face, in pixels
//---------------------------------------------

IMF_EXPORT
int sizeOfFace (const IMATH_NAMESPACE::Box2i& dataWindow);

//------------------------------------------
// Compute the region in the environment map
// that is covered by the specified face.
//------------------------------------------

IMF_EXPORT
IMATH_NAMESPACE::Box2i
dataWindowForFace (CubeMapFace face, const IMATH_NAMESPACE::Box2i& dataWindow);

//----------------------------------------------------
// Convert the coordinates of a pixel within a face
// [in the range from (0,0) to (s-1,s-1), where
// s == sizeOfFace(dataWindow)] to pixel coordinates
// in the environment map.
//----------------------------------------------------

IMF_EXPORT
IMATH_NAMESPACE::V2f pixelPosition (
    CubeMapFace                   face,
    const IMATH_NAMESPACE::Box2i& dataWindow,
    IMATH_NAMESPACE::V2f          positionInFace);

//--------------------------------------------------------------
// Convert a 3D direction into a cube face, and a pixel position
// within that face.
//
// If you have a 3D direction, dir, the following code fragment
// finds the position, pos, of the corresponding pixel in an
// environment map with data window dw:
//
// CubeMapFace f;
// V2f pif, pos;
//
// faceAndPixelPosition (dir, dw, f, pif);
// pos = pixelPosition (f, dw, pif);
//
//--------------------------------------------------------------

IMF_EXPORT
void faceAndPixelPosition (
    const IMATH_NAMESPACE::V3f&   direction,
    const IMATH_NAMESPACE::Box2i& dataWindow,
    CubeMapFace&                  face,
    IMATH_NAMESPACE::V2f&         positionInFace);

// --------------------------------------------------------
// Given a cube face and a pixel position within that face,
// compute the corresponding 3D direction.
// --------------------------------------------------------

IMF_EXPORT
IMATH_NAMESPACE::V3f direction (
    CubeMapFace                   face,
    const IMATH_NAMESPACE::Box2i& dataWindow,
    const IMATH_NAMESPACE::V2f&   positionInFace);
} // namespace CubeMap

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

#endif
