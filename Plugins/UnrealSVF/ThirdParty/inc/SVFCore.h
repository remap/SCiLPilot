#pragma once

#ifdef _MSC_VER
#include <propsys.h>
#endif

//**********************************************************************
//! \brief CSVFVertex defines the vertex structure containing position and UVs
//!
//! This structure defines what a vertex looks like in our
//! vertex buffer (see \ref ISVFBuffer). This is the default format for a vertex.
//! The client may request other vertex formats by setting the outputVertexFormat
//! field in the \ref SVFConfiguration passed to SVFReader::Initialize.
//! This format is requested by setting:<br>
//!    config.outputFVertexFormat = ESVFOutputVertexFormat::Position | ESVFOutputVertexFormat::UV | ESVFOutputVertexFormat::Normal<br>
//!
//! X/Y/Z are stored as signed normalized shorts. These values need to be
//! rescaled (usually done in the renderer's shader) using the bbox provided by
//! \ref SVFAttrib_BBoxMinX,
//! \ref SVFAttrib_BBoxMinY,
//! \ref SVFAttrib_BBoxMinZ,
//! \ref SVFAttrib_BBoxMaxX,
//! \ref SVFAttrib_BBoxMaxY,
//! \ref SVFAttrib_BBoxMaxZ
//!attributes on the buffer.Rescaling should use the following equation<br>
//!&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(((float)X + 32768.0f) / 65535.0f) * (BBoxMaxX - BBoxMinX) + BBoxMinX;<br>
//!&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(((float)Y + 32768.0f) / 65535.0f) * (BBoxMaxY - BBoxMinY) + BBoxMinY;<br>
//!&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(((float)Z + 32768.0f) / 65535.0f) * (BBoxMaxY - BBoxMinY) + BBoxMinY;<br>
//! U/V are stored as signed normalized shorts. These values need to be
//!be rescaled (again, usually done in the renderer's shader) using the following:<br>
//!&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;((float)u + 32768.0f) / 65535.0f<br>
//!&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;((float)v + 32768.0f) / 65535.0f
//**********************************************************************
struct CSVFVertex
{
    short x; //!< Defines the X component for our vertex's position
    short y; //!< Defines the Y component for our vertex's position
    short z; //!< Defines the Z component for our vertex's position
    short w; //!< Currently unused
    short u; //!< Defines the U component for our texture UV coordinate
    short v; //!< Defines the V component for our texture UV coordinate
};

//**********************************************************************
//! \brief CSVFVertex_Norm defines the vertex structure for verts containing
//! position, normal and UV coordinates.
//!
//! This structure defines our vertex data when the client requests vertices
//! containing position, normals, and UV coordinates. Similar to \ref CSVFVertex
//! this data is typically scaled in the shader. See \ref CSVFVertex for details
//! on this scaling.<br>
//! The normal data is stored as unsigned shorts and should be rescaled using the following:<br>
//!&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;2.0f * ((float)nx) / 65535.0f - 1.0f<br>
//!&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;2.0f * ((float)ny) / 65535.0f - 1.0f<br>
//!&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;2.0f * ((float)nz) / 65535.0f - 1.0f<br>
//**********************************************************************
struct CSVFVertex_Norm
{
    short x; //!< Defines the X component for our vertex's position
    short y; //!< Defines the Y component for our vertex's position
    short z; //!< Defines the Z component for our vertex's position
    short w; //!< Currently unused
    unsigned short nx; //!< Defines the X component for the normal vector
    unsigned short ny; //!< Defines the Y component for the normal vector
    unsigned short nz; //!< Defines the Z component for the normal vector
    unsigned short nw; //!< Currently unused
    short u; //!< Defines the U component for our texture UV coordinate
    short v; //!< Defines the V component for our texture UV coordinate
};

//**********************************************************************
//! \brief CSVFVertex_Full defines the vertex structure for verts containing
//! position and UV coordinates using full floating point precision.
//!
//! This structure defines our vertex data when the client requests vertices
//! containing position and UV coordinates using full floating point precision. Similar to \ref CSVFVertex
//! this data is typically scaled in the shader. See \ref CSVFVertex for details
//! on this scaling.<br>
//! Clients request floating point using the ESVFOutputVertexFormat::Uncompressed<br>
//**********************************************************************
struct CSVFVertex_Full
{
    float x, y, z;
    float u, v;
};

//**********************************************************************
//! \brief SVFLockedMemory is used by LockBuffer to return a pointer to
//! the memory in an SVFBuffer
//**********************************************************************
struct SVFLockedMemory
{
    BYTE *pData;         //!< Pointer to the memory block
    ULONG StrideBytes;   //!< Size of our stride in bytes (for 2D data). When not applicable, StrideBytes = Size
    ULONG Size;          //!< Size of pData, in bytes.    
    ULONG ChromaOffset;  //!< For planar surfaces such as NV12, IYUV and YV12, this is an offset to access chroma

    SVFLockedMemory() :
        pData(nullptr),
        StrideBytes(0),
        Size(0),
        ChromaOffset(0)
    {
    }
};

//**********************************************************************
//! \brief ISVFAttributesCore attributes accessors needed by SVFCore
//!
//! See \ref SVFGuids.h for a complete list of attributes that are exposed
//**********************************************************************
interface DECLSPEC_UUID("E73F1728-AE2D-45BF-9FC7-FCDE723FB667") ISVFAttributesCore : public IUnknown
{
    virtual HRESULT OnGetAttribute() = 0;  //!< Ensures that our attributes have been read (must be called before trying to obtain an attribute). NOTE: Generally clients never should call this method.

    virtual HRESULT GetUINT32(_In_ REFGUID guidKey, _Out_ UINT32 *punValue) = 0; //!< Retrieves the specified UINT32 attribute
    virtual HRESULT GetUINT64(_In_ REFGUID guidKey, _Out_ UINT64 *punValue) = 0; //!< Retrieves the specified UINT64 attribute
    virtual HRESULT GetDouble(_In_ REFGUID guidKey, _Out_ double *pfValue) = 0; //!< Retrieves the specified double attribute

    virtual HRESULT SetUINT32(_In_ REFGUID guidKey, _In_ UINT32 unValue) = 0; //!< Sets the specified UINT32 attribute
    virtual HRESULT SetUINT64(_In_ REFGUID guidKey, _In_ UINT64 unValue) = 0; //!< Sets the specified UINT64  attribute
    virtual HRESULT SetDouble(_In_ REFGUID guidKey, _In_ double fValue) = 0; //!< Sets the specified double attribute
};

//**********************************************************************
//! \brief ISVFBuffer defines the buffer that vertex, index, and texture data is returned in.
//!
//! ISVFBuffer represents a single buffer from our spatial video. The buffer
//! may contain either texture, vertex or index information.
//**********************************************************************
interface DECLSPEC_UUID("D83BA762-145B-489D-BA08-E76578398FDB") ISVFBuffer : public IUnknown
{
    virtual HRESULT LockBuffer(_In_ SVFLockedMemory *pLockedMemory) = 0; //!< Locks buffer so client can access memory
    virtual HRESULT UnlockBuffer() = 0; //!< Unlock buffer
    virtual HRESULT GetSize(_Out_ DWORD *pSize) = 0; //!< Returns the total size of the data in the buffer
};

//**********************************************************************
//! \brief ISVFBufferAllocator is responsible for allocating buffers returned by SVF
//!
//! ISVFBufferAllocator is responsible for allocating buffers used to return
//! data back to the client. The client may either implement a custom allocator
//! implementing this interface or use on returned by \ref SVFCreateBufferAllocator.
//! Either way the client will pass this allocator to \ref SVFCreateReader.
//**********************************************************************
interface DECLSPEC_UUID("C79E8896-69C8-4D9D-ADB7-772BCE08471E") ISVFBufferAllocator : public IUnknown
{
    virtual HRESULT AllocateBuffer(DWORD bytes, DWORD stride, DWORD chromaOffset, _Outptr_ ISVFBuffer **ppBuffer) = 0;
    virtual HRESULT ReleaseBuffer(_In_ ISVFBuffer *pBuffer) = 0;
};

//**********************************************************************
//! \brief CSVFVertex_Norm_Full defines the vertex structure for verts containing
//! position, normals and UV coordinates using full floating point precision.
//!
//! This structure defines our vertex data when the client requests vertices
//! containing position, normals and UV coordinates using full floating point precision. Similar to \ref CSVFVertex_Norm
//! this data is typically scaled in the shader. See \ref CSVFVertex_Norm for details
//! on this scaling.<br>
//! Clients request floating point using the ESVFOutputVertexFormat::Uncompressed<br>
//**********************************************************************
struct CSVFVertex_Norm_Full
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

typedef DWORD CSVFVertexIndex; //!< Define format for our index data (32b/index)

//**********************************************************************
//! \brief ESVFOutputVertexFormat defines what vertex components are written into the vertex buffer
//!
//! By default SVF only writes vertex position and UV data into
//! the vertex buffer. These flags are used to request other types
//! of output. These flags are set on \ref SVFConfiguration "SVFConfiguration's" outputVertexFormat field.
//**********************************************************************
namespace ESVFOutputVertexFormat
{
    const UINT32 Position = 0x1; //!< Output vertex has a position member
    const UINT32 Normal = 0x2; //!< Output vertex has a normal member
    const UINT32 UV = 0x4; //!< Output vertex has a UV member
    const UINT32 Uncompressed = 0x8; //!< Output vertex members using full float format
};
