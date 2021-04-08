#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "SVFClockInterface.h"
#include "Interfaces/Interface_CollisionDataProvider.h"

#if PLATFORM_WINDOWS
#include "SVFReaderPassThrough.h"
THIRD_PARTY_INCLUDES_START
#include <setupapi.h>
#if ENGINE_MINOR_VERSION > 19
#include <initguid.h> // must be included before mmdeviceapi.h for the pkey defines to be properly instantiated.
#include <mmdeviceapi.h>
#endif
THIRD_PARTY_INCLUDES_END

#elif PLATFORM_ANDROID
#include "SVFReaderAndroid.h"
#endif

class USVFComponent;

class FSVFMeshIndexBuffer : public FIndexBuffer
{
public:
    TArray<int32> Indices;

    FSVFMeshIndexBuffer(uint32 InNumIndices)
    {
        Indices.SetNumUninitialized(InNumIndices);
    }

    void Reset(uint32 InNumIndices)
    {
        Indices.SetNumUninitialized(InNumIndices);
        ReleaseResource();
        InitResource();
    }

#if PLATFORM_WINDOWS
    virtual void InitRHI() override
    {
        FRHIResourceCreateInfo CreateInfo;
        IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32), Indices.Num() * sizeof(int32), BUF_Static, CreateInfo);

        // Write the indices to the index buffer.
        void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, Indices.Num() * sizeof(int32), RLM_WriteOnly);
        FMemory::Memcpy(Buffer, Indices.GetData(), Indices.Num() * sizeof(int32));
        RHIUnlockIndexBuffer(IndexBufferRHI);
    }
#elif PLATFORM_ANDROID
    virtual void InitRHI() override
    {
        FRHIResourceCreateInfo CreateInfo;
        IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32), Indices.Num() * sizeof(int32), BUF_Dynamic, CreateInfo);
    }
#endif
    virtual void ReleaseRHI() override
    {
        IndexBufferRHI = NULL;
        FIndexBuffer::ReleaseRHI();
    }
};

class FSVFMeshVertexBuffer : public FVertexBuffer
{
public:
    TArray<FDynamicMeshVertex> Vertices;

    FVertexBuffer PositionBuffer;
    FVertexBuffer TangentBuffer;
    FVertexBuffer TexCoordBuffer;

    FShaderResourceViewRHIRef PositionBufferSRV;
    FShaderResourceViewRHIRef TangentBufferSRV;
    FShaderResourceViewRHIRef TexCoordBufferSRV;

#if PLATFORM_WINDOWS 
    FVertexBuffer ColorBuffer;
    FShaderResourceViewRHIRef ColorBufferSRV;
#endif

    FSVFMeshVertexBuffer(uint32 InNumTexCoords, uint32 InLightmapCoordinateIndex, bool InUse16bitTexCoord, uint32 InNumVertices);

    void Reset(uint32 InNumVertices);
    virtual void InitRHI() override;
    virtual void ReleaseRHI() override;
    void InitResource() override;
    void ReleaseResource() override;
    void GetPositionalVertices(TArray<FVector>& OutVertices, int32 VerticesCount) const;

    const uint32 GetNumTexCoords() const
    {
        return NumTexCoords;
    }

    const uint32 GetLightmapCoordinateIndex() const
    {
        return LightmapCoordinateIndex;
    }

    const bool GetUse16bitTexCoords() const
    {
        return Use16bitTexCoord;
    }
private:
    const uint32 NumTexCoords;
    const uint32 LightmapCoordinateIndex;
    const bool Use16bitTexCoord = true;
};

class FSVFMeshVertexFactory : public FLocalVertexFactory
{
public:

    FSVFMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, FSVFMeshVertexBuffer* InVertexBuffer) :
        FLocalVertexFactory(InFeatureLevel, "FSVFMeshVertexFactory")
        , VertexBuffer(InVertexBuffer)
    {};

    void InitResource() override;
private:
    FSVFMeshVertexBuffer* VertexBuffer;
};

class FSVFMeshSceneProxy : public FPrimitiveSceneProxy
{
public:

    FSVFMeshSceneProxy(USVFComponent* Component);
    FSVFMeshSceneProxy(USVFComponent* Component, int InitialVertexCount, int InitialIndexCount);
    virtual ~FSVFMeshSceneProxy();
    SIZE_T GetTypeHash() const;
    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views,
        const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
    //void Update_RenderThread(TSharedPtr<FFrameData> FrameData);
    void Update_RenderThread(TSharedPtr<FFrameData> FrameData, int VertexCount = 0, int IndexCount = 0);
    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const;

    virtual bool CanBeOccluded() const override
    {
        return !MaterialRelevance.bDisableDepthTest;
    }

    virtual uint32 GetMemoryFootprint(void) const
    {
        return(sizeof(*this) + GetAllocatedSize());
    }

    uint32 GetAllocatedSize(void) const
    {
        return(FPrimitiveSceneProxy::GetAllocatedSize());
    }

    void GetPositionalVertices(TArray<FVector>& OutVertices, int32 VerticesCount) const
    {
        VertexBuffer.GetPositionalVertices(OutVertices, VerticesCount);
    }

private:

    UMaterialInterface* Material;
    FSVFMeshVertexBuffer VertexBuffer;
    FSVFMeshIndexBuffer IndexBuffer;
    FSVFMeshVertexFactory VertexFactory;
    FMaterialRelevance MaterialRelevance;
    UBodySetup* BodySetup;
};