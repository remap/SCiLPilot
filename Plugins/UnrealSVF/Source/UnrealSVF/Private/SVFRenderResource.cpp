// Copyright (C) Microsoft Corporation. All rights reserved.

#include "SVFRenderResource.h"

FSVFVertexBuffer::FSVFVertexBuffer(int32 InStride)
    : Stride(InStride)
    , NumVertices(0)
    , ShaderResourceView(nullptr)
{}

void FSVFVertexBuffer::Reset(int32 InNumVertices)
{
    NumVertices = InNumVertices;
    ReleaseResource();
    InitResource();
}

void FSVFVertexBuffer::InitRHI()
{
    if (Stride > 0 && NumVertices > 0)
    {
        FRHIResourceCreateInfo CreateInfo;
        VertexBufferRHI = RHICreateVertexBuffer(GetBufferSize(), BUF_Dynamic | BUF_ShaderResource, CreateInfo);

        if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
        {
            CreateSRV();
        }
    }
}

void FSVFVertexBuffer::SetNum(int32 InNumVertices)
{
    if (InNumVertices != NumVertices)
    {
        NumVertices = InNumVertices;
        ReleaseResource();
        InitResource();
    }
}

void FSVFVertexBuffer::SetData(const TArray<uint8>& Data)
{
    check(VertexBufferRHI.IsValid());
    void* Buffer = RHILockVertexBuffer(VertexBufferRHI, 0, Data.Num(), RLM_WriteOnly);
    FMemory::Memcpy(Buffer, Data.GetData(), Data.Num());
    RHIUnlockVertexBuffer(VertexBufferRHI);
}



FSVFIndexBuffer::FSVFIndexBuffer()
    : NumIndices(0)
    , Stride(-1)
{}

void FSVFIndexBuffer::Reset(int32 InStride, int32 InNumIndices)
{
    Stride = InStride;
    NumIndices = InNumIndices;
    ReleaseResource();
    InitResource();
}

void FSVFIndexBuffer::InitRHI()
{
    if (Stride > 0 && NumIndices > 0)
    {
        FRHIResourceCreateInfo CreateInfo;
        IndexBufferRHI = RHICreateIndexBuffer(Stride, GetBufferSize(), BUF_Dynamic, CreateInfo);
    }
}

void FSVFIndexBuffer::SetNum(int32 InNumIndices)
{
    if (InNumIndices != NumIndices)
    {
        NumIndices = InNumIndices;
        ReleaseResource();
        InitResource();
    }
}

void FSVFIndexBuffer::SetData(const TArray<uint8>& Data)
{
    check(Data.Num() == GetBufferSize());
    check(IndexBufferRHI.IsValid());
    void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, Data.Num(), RLM_WriteOnly);
    FMemory::Memcpy(Buffer, Data.GetData(), Data.Num());
    RHIUnlockIndexBuffer(IndexBufferRHI);
}
