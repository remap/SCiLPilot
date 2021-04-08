//---------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------
#pragma once
#ifdef _MSC_VER
#include <guiddef.h>
#endif

/*! \file
*/

#ifdef COMPILINGSVF
// If we are including this header inside SVF then don't define declspec(dllimport)
#define SVFDECLIMPORT 
#else
#define SVFDECLIMPORT _declspec(dllimport)
#endif

// The default version of DEFINE_GUID doesn't include declspec(dllexport).
// Ideally I would not redefine this but instead just export these variables
// through the SVF.def file. However, that doesn't appear to work (clients
// end up with wrong definitions...there appears to be something wrong
// with the linker or my definition of SVF.dev).
#ifdef INITGUID
#pragma message("Compiling GUID")
#define SVF_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID _declspec(dllexport) name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define SVF_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    EXTERN_C const GUID SVFDECLIMPORT name
#endif // INITGUID

//! SVFMediaType_VertexStream (UINT32) Used to identify a stream that contains vertex data
SVF_DEFINE_GUID(SVFMediaType_VertexStream, 0x30bd92ba, 0x2af3, 0x4a3c, 0xad, 0x73, 0xc1, 0xd4, 0x67, 0x6c, 0xf3, 0xcb);

//! SVFMediaType_IndexStream (UINT32) Used to identify a stream that contains index data
SVF_DEFINE_GUID(SVFMediaType_IndexStream, 0xfd2aae31, 0x5b7, 0x4730, 0xb7, 0xdb, 0x6a, 0x4c, 0x2d, 0x5d, 0x26, 0xf8);

//**********************************************************************
// Define GUIDs for various attributes stored on the SVFBuffer
//**********************************************************************

//! Not yet supported
SVF_DEFINE_GUID(SVFAttrib_Version, 0xbf07a1db, 0x37b3, 0x4f86, 0xb7, 0xa0, 0x8e, 0x4c, 0x79, 0xf8, 0xf0, 0xc4);

//! SVFAttrib_Width (UINT32) returns width of texture
SVF_DEFINE_GUID(SVFAttrib_Width, 0xd59f1c5c, 0xd812, 0x41b0, 0xa5, 0xf5, 0xeb, 0x6f, 0x99, 0x15, 0x41, 0x6d);

//! SVFAttrib_Height (UINT32) returns height of texture
SVF_DEFINE_GUID(SVFAttrib_Height, 0x3e0d1911, 0x81a0, 0x46b2, 0xb4, 0xd3, 0xc0, 0xc0, 0x18, 0xce, 0x0, 0xcf);

//! SVFAttrib_Inverted (UINT32) returns 1 if image is flipped vertically, otherwise returns 0
SVF_DEFINE_GUID(SVFAttrib_Inverted, 0x954561fe, 0x568a, 0x441c, 0xaa, 0xd0, 0x54, 0x6f, 0x7f, 0xc4, 0x5c, 0xb);

//! SVFAttrib_Stride (UINT32) returns number of bytes in each scanline of texture data
SVF_DEFINE_GUID(SVFAttrib_Stride, 0x3b26c9b2, 0xa8aa, 0x4d02, 0x9f, 0x9b, 0x39, 0xec, 0xb8, 0x4c, 0x44, 0x56);

//! SVFAttrib_NumVertices (UINT32) returns number of vertices in vertex buffer
SVF_DEFINE_GUID(SVFAttrib_NumVertices, 0x370e73, 0xada9, 0x4f24, 0x85, 0xb1, 0xb2, 0x2d, 0x2a, 0xdc, 0x93, 0xf6);

//! SVFAttrib_NumFaces (UINT32) returns number of triangle faces in vertex buffer
SVF_DEFINE_GUID(SVFAttrib_NumFaces, 0x44444132, 0x4772, 0x414a, 0xb4, 0x65, 0x32, 0x85, 0x4, 0x88, 0xd0, 0xf1);

//! SVFAttrib_NumIndices (UINT32) returns number of indices in index buffer
SVF_DEFINE_GUID(SVFAttrib_NumIndices, 0xc79e8586, 0x94d9, 0x4439, 0xb2, 0x8f, 0xba, 0xc2, 0xbb, 0x7f, 0xf3, 0x98);

//! \brief SVFAttrib_BBoxMinX (double) returns min X coordinate of axis aligned bounding box.
//!
//! If this attribute is requested on the \ref ISVFFrame the value is from the bounding box of the current
//! frame's mesh. If requested on the ISVFReader it is the bounding box of the entire video clip (union of
//! all meshes)
SVF_DEFINE_GUID(SVFAttrib_BBoxMinX, 0xeeb82bc6, 0x22fe, 0x426f, 0x85, 0xfc, 0x7, 0x16, 0x4e, 0xdb, 0x31, 0xf4);

//! \brief SVFAttrib_BBoxMinY (double) returns min Y coordinate of axis aligned bounding box.
//!
//! If this attribute is requested on the \ref ISVFFrame the value is from the bounding box of the current
//! frame's mesh. If requested on the ISVFReader it is the bounding box of the entire video clip (union of
//! all meshes)
SVF_DEFINE_GUID(SVFAttrib_BBoxMinY, 0x5928706c, 0xcc40, 0x427c, 0x9b, 0x25, 0x93, 0xbd, 0x5a, 0x95, 0x8e, 0x31);

//! \brief SVFAttrib_BBoxMinZ (double) returns min Z coordinate of axis aligned bounding box.
//!
//! If this attribute is requested on the \ref ISVFFrame the value is from the bounding box of the current
//! frame's mesh. If requested on the ISVFReader it is the bounding box of the entire video clip (union of
//! all meshes)
SVF_DEFINE_GUID(SVFAttrib_BBoxMinZ, 0x28111338, 0xa3fb, 0x4c29, 0x8c, 0x1b, 0xa9, 0xd, 0x30, 0x8b, 0xbe, 0x4e);

//! \brief SVFAttrib_BBoxMaxX (double) returns max X coordinate of axis aligned bounding box.
//!
//! If this attribute is requested on the \ref ISVFFrame the value is from the bounding box of the current
//! frame's mesh. If requested on the ISVFReader it is the bounding box of the entire video clip (union of
//! all meshes)
SVF_DEFINE_GUID(SVFAttrib_BBoxMaxX, 0x1b717e5a, 0x75cf, 0x4358, 0x8f, 0xc7, 0x29, 0xaa, 0x21, 0xa, 0x46, 0x54);

//! \brief SVFAttrib_BBoxMaxY (double) returns max Y coordinate of axis aligned bounding box.
//!
//! If this attribute is requested on the \ref ISVFFrame the value is from the bounding box of the current
//! frame's mesh. If requested on the ISVFReader it is the bounding box of the entire video clip (union of
//! all meshes)
SVF_DEFINE_GUID(SVFAttrib_BBoxMaxY, 0xc8483604, 0x704, 0x4efc, 0xa5, 0xa3, 0x3a, 0xc7, 0x19, 0x70, 0x72, 0x24);

//! \brief SVFAttrib_BBoxMaxZ (double) returns max Z coordinate of axis aligned bounding box.
//!
//! If this attribute is requested on the \ref ISVFFrame the value is from the bounding box of the current
//! frame's mesh. If requested on the ISVFReader it is the bounding box of the entire video clip (union of
//! all meshes)
SVF_DEFINE_GUID(SVFAttrib_BBoxMaxZ, 0x694a656, 0xdabd, 0x49b1, 0x89, 0x6c, 0x8f, 0xe0, 0x88, 0xc, 0xc2, 0x76);
//!

//! SVFAttrib_UseTriList (UINT32) returns 1 if vertex buffer contains triangle lists. Otherwise, it returns 0 and vertex buffer contains triangle strips.
SVF_DEFINE_GUID(SVFAttrib_UseTriList, 0xc18a431c, 0xd41c, 0x47bc, 0x8a, 0xc6, 0xbd, 0x93, 0xfc, 0xfd, 0xb5, 0x6);

//! SVFAttrib_VertHasNormals (UINT32) returns 1 if each vertex contains a normal vector
SVF_DEFINE_GUID(SVFAttrib_VertHasNormals, 0x109655a2, 0x2822, 0x46d7, 0xb0, 0xfa, 0x8, 0x29, 0x7, 0x66, 0x56, 0x58);

//! SVFAttrib_UseUnsignedUVs (UINT32) returns 1 if each vertex uses unsigned UVs (in range 0..1). Otherwise UVs are signed in range -1..+1
SVF_DEFINE_GUID(SVFAttrib_UseUnsignedUVs, 0xff644db5, 0xebaa, 0x4c2d, 0x89, 0x6a, 0x8d, 0x12, 0x14, 0xf7, 0x34, 0xd7); 

//! Obsolete
SVF_DEFINE_GUID(SVFAttrib_BBoxFull, 0xd2f6b5cc, 0xcb27, 0x4483, 0x8e, 0xc, 0x3c, 0xd8, 0xe2, 0x42, 0xf7, 0x49);

// [UINT32] Frame Id. TODO: when we pass processing metadata to mp4, show actual *capture* frame id
// {129ACAC9-DA92-406D-A560-DDF2B7FD3625}
SVF_DEFINE_GUID(SVFAttrib_FrameId, 0x129acac9, 0xda92, 0x406d, 0xa5, 0x60, 0xdd, 0xf2, 0xb7, 0xfd, 0x36, 0x25);

// [UINT64] Frame timestamp, in 100ns, coming from Media Foundation
// {4FC3B0A7-6644-42A5-9BEB-77BC5B020024}
SVF_DEFINE_GUID(SVFAttrib_FrameTimestamp, 0x4fc3b0a7, 0x6644, 0x42a5, 0x9b, 0xeb, 0x77, 0xbc, 0x5b, 0x2, 0x0, 0x24);

// [UINT32] vertex format flags, as defined in CStreamingMeshHeader::VersionType
// {F00B04E4-0272-4BE2-8382-D938591528F8}
SVF_DEFINE_GUID(SVFAttrib_VertexFormatFlags, 0xf00b04e4, 0x272, 0x4be2, 0x83, 0x82, 0xd9, 0x38, 0x59, 0x15, 0x28, 0xf8);

// [UINT32] (0|1) Is a new key frame
// {1E41250E-E111-4020-89F2-9947FADAEC97}
SVF_DEFINE_GUID(SVFAttrib_IsKeyFrame, 0x1e41250e, 0xe111, 0x4020, 0x89, 0xf2, 0x99, 0x47, 0xfa, 0xda, 0xec, 0x97);

//**********************************************************************
// Define GUIDs for SVFReader attributes
//**********************************************************************
// [UINT32] total frame count in presentation
// {36B53E4E-0A14-42F8-8143-DCA29E480AA8}
//! SVFAttrib_FrameCount (UINT32) returns the total number of frames in the video clip
SVF_DEFINE_GUID(SVFAttrib_FrameCount, 0x36b53e4e, 0xa14, 0x42f8, 0x81, 0x43, 0xdc, 0xa2, 0x9e, 0x48, 0xa, 0xa8);

// [UINT32] max vertex count per frame throughout presentation
// {A5E5F089-BA7D-48B3-8145-4C9C2E412D42}
//! SVFAttrib_MaxVertexCount (UINT32) returns the maximum vertex count that any mesh in the clip contains
SVF_DEFINE_GUID(SVFAttrib_MaxVertexCount, 0xa5e5f089, 0xba7d, 0x48b3, 0x81, 0x45, 0x4c, 0x9c, 0x2e, 0x41, 0x2d, 0x42);

// [UINT32] max index count per frame throughout presentation
// {C0E6118D-A24F-491C-96F7-CC5A8FA70D24}
//! SVFAttrib_MaxIndexCount (UINT32) returns the maximum index count that any mesh in the clip contains
SVF_DEFINE_GUID(SVFAttrib_MaxIndexCount, 0xc0e6118d, 0xa24f, 0x491c, 0x96, 0xf7, 0xcc, 0x5a, 0x8f, 0xa7, 0xd, 0x24);

// [string] full path to final destination of downloaded file, points to local cache
// if you get "ERROR_NOT_FOUND" when asking for this attribute, file has not been finalized (fully downloaded) yet, 
// or it will be streaming from dash server
// {AB0EA9AA-CEC9-4A36-B90F-2958154E4712}
//! SVFAttrib_LocalFullPath (string) Full path to download file in local cache
SVF_DEFINE_GUID(SVFAttrib_LocalFullPath, 0xab0ea9aa, 0xcec9, 0x4a36, 0xb9, 0xf, 0x29, 0x58, 0x15, 0x4e, 0x47, 0x12);


// [UINT32] (0|1) has Audio
// {1AED3F91-FB45-44F6-98BE-59FC3AFB8D2B}
SVF_DEFINE_GUID(SVFAttrib_HasAudio, 0x1aed3f91, 0xfb45, 0x44f6, 0x98, 0xbe, 0x59, 0xfc, 0x3a, 0xfb, 0x8d, 0x2b);

// [UINT64] Duration, in 100ns Media Foundation units
// {DE7AB62A-915F-4CD9-8193-9B93137D29D9}
SVF_DEFINE_GUID(SVFAttrib_Duration100ns, 0xde7ab62a, 0x915f, 0x4cd9, 0x81, 0x93, 0x9b, 0x93, 0x13, 0x7d, 0x29, 0xd9);

// [double] Bitrate, Mbps 
// {B70518C5-D70A-482D-8FB3-E783C72E0642}
SVF_DEFINE_GUID(SVFAttrib_BitrateMbps, 0xb70518c5, 0xd70a, 0x482d, 0x8f, 0xb3, 0xe7, 0x83, 0xc7, 0x2e, 0x6, 0x42);

// TODO: document
// {22F1F808-02AD-4D65-B094-BEC42F90BEDD}
//! Obsolete
SVF_DEFINE_GUID(SVFAttrib_H264Wrapper, 0x22f1f808, 0x2ad, 0x4d65, 0xb0, 0x94, 0xbe, 0xc4, 0x2f, 0x90, 0xbe, 0xdd);

// [UINT32] (0|1) has valid vertex normals
// {DE7A5734-8816-4C0A-A16E-2B61666EBB02}
SVF_DEFINE_GUID(SVFAttrib_HasVertexNormals, 0xde7a5734, 0x8816, 0x4c0a, 0xa1, 0x6e, 0x2b, 0x61, 0x66, 0x6e, 0xbb, 0x2);
