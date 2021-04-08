// Copyright (C) Microsoft Corporation. All rights reserved.

#include "SVFMeshComponents.h"
#include "SVFComponent.h"
#include "UnrealSVF.h"
#include "Misc/Paths.h"
#include "SVFSimpleInterface.h"
#include "DynamicMeshBuilder.h"
#include "Engine/Engine.h"
#include "LocalVertexFactory.h"

#include "Runtime/Launch/Resources/Version.h"
#include "Runtime/Engine/Classes/PhysicsEngine/BodySetup.h"

DEFINE_LOG_CATEGORY_STATIC(LogSVFMeshComponents, Log, All);

#define LogSVF(pmt, ...) UE_LOG(LogSVFMeshComponents, Log, TEXT(pmt), ##__VA_ARGS__)
#define WarnSVF(pmt, ...) UE_LOG(LogSVFMeshComponents, Warning, TEXT(pmt), ##__VA_ARGS__)
#define FatalSVF(pmt, ...) UE_LOG(LogSVFMeshComponents, Fatal, TEXT(pmt), ##__VA_ARGS__)

#if PLATFORM_ANDROID
#include "SVFReaderAndroid.h"
#endif // PLATFORM_ANDROID

FSVFMeshVertexBuffer::FSVFMeshVertexBuffer(uint32 InNumTexCoords, uint32 InLightmapCoordinateIndex, bool InUse16bitTexCoord, uint32 InNumVertices) :
    NumTexCoords(InNumTexCoords), LightmapCoordinateIndex(InLightmapCoordinateIndex), Use16bitTexCoord(InUse16bitTexCoord)
{
    check(NumTexCoords > 0 && NumTexCoords <= MAX_STATIC_TEXCOORDS);
    check(LightmapCoordinateIndex < NumTexCoords);
    Vertices.SetNumUninitialized(InNumVertices);
}

void FSVFMeshVertexBuffer::Reset(uint32 InNumVertices)
{
    Vertices.SetNumUninitialized(InNumVertices);
    ReleaseResource();
    InitResource();
}

void FSVFMeshVertexBuffer::InitRHI()
{
    uint32 TextureStride = sizeof(FVector2D);
    EPixelFormat TextureFormat = PF_G32R32F;

    if (Use16bitTexCoord)
    {
        TextureStride = sizeof(FVector2DHalf);
        TextureFormat = PF_G16R16F;
    }

    FRHIResourceCreateInfo PositionCreateInfo;
    PositionBuffer.VertexBufferRHI = RHICreateVertexBuffer(sizeof(FVector) * Vertices.Num(),
        BUF_Static | BUF_ShaderResource, PositionCreateInfo);
    FRHIResourceCreateInfo TangentCreateInfo;
    TangentBuffer.VertexBufferRHI = RHICreateVertexBuffer(sizeof(FPackedNormal) * 2 * Vertices.Num(),
        BUF_Static | BUF_ShaderResource, TangentCreateInfo);
    FRHIResourceCreateInfo TexCoordCreateInfo;
    TexCoordBuffer.VertexBufferRHI = RHICreateVertexBuffer(TextureStride * 4 * Vertices.Num(),
        BUF_Static | BUF_ShaderResource, TexCoordCreateInfo);
#if PLATFORM_WINDOWS
    FRHIResourceCreateInfo ColorCreateInfo;
    ColorBuffer.VertexBufferRHI = RHICreateVertexBuffer(sizeof(FColor) * Vertices.Num(),
        BUF_Static | BUF_ShaderResource, ColorCreateInfo);
#endif

    if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
    {
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION <= 19
        TangentBufferSRV = RHICreateShaderResourceView(TangentBuffer.VertexBufferRHI, 4, PF_R8G8B8A8);
#elif ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
        TangentBufferSRV = RHICreateShaderResourceView(TangentBuffer.VertexBufferRHI, 4, PF_R8G8B8A8_SNORM);
#endif
        TexCoordBufferSRV = RHICreateShaderResourceView(TexCoordBuffer.VertexBufferRHI, TextureStride, TextureFormat);
        PositionBufferSRV = RHICreateShaderResourceView(PositionBuffer.VertexBufferRHI, sizeof(float), PF_R32_FLOAT);
#if PLATFORM_WINDOWS
        ColorBufferSRV = RHICreateShaderResourceView(ColorBuffer.VertexBufferRHI, 4, PF_R8G8B8A8);
#endif
    }

#if PLATFORM_WINDOWS
    void* TexCoordBufferData = RHILockVertexBuffer(TexCoordBuffer.VertexBufferRHI,
        0, NumTexCoords * TextureStride * Vertices.Num(), RLM_WriteOnly);
    FVector2D* TexCoordBufferData32 = !Use16bitTexCoord ? static_cast<FVector2D*>(TexCoordBufferData) : nullptr;
    FVector2DHalf* TexCoordBufferData16 = Use16bitTexCoord ? static_cast<FVector2DHalf*>(TexCoordBufferData) : nullptr;

    // Copy the vertex data into the vertex buffers.
    FVector* PositionBufferData = static_cast<FVector*>(
        RHILockVertexBuffer(PositionBuffer.VertexBufferRHI,
            0, sizeof(FVector) * Vertices.Num(), RLM_WriteOnly));
    FPackedNormal* TangentBufferData = static_cast<FPackedNormal*>(
        RHILockVertexBuffer(TangentBuffer.VertexBufferRHI,
            0, 2 * sizeof(FPackedNormal) * Vertices.Num(), RLM_WriteOnly));
    FColor* ColorBufferData = static_cast<FColor*>(
        RHILockVertexBuffer(ColorBuffer.VertexBufferRHI,
            0, sizeof(FColor) * Vertices.Num(), RLM_WriteOnly));

    for (int32 i = 0; i < Vertices.Num(); i++)
    {
        PositionBufferData[i] = Vertices[i].Position;
        TangentBufferData[2 * i + 0] = Vertices[i].TangentX;
        TangentBufferData[2 * i + 1] = Vertices[i].TangentZ;
        ColorBufferData[i] = Vertices[i].Color;

        for (uint32 j = 0; j < NumTexCoords; j++)
        {
            if (Use16bitTexCoord)
            {
                TexCoordBufferData16[NumTexCoords * i + j] = FVector2DHalf(Vertices[i].TextureCoordinate[j]);
            }
            else
            {
                TexCoordBufferData32[NumTexCoords * i + j] = Vertices[i].TextureCoordinate[j];
            }
        }
    }

    RHIUnlockVertexBuffer(PositionBuffer.VertexBufferRHI);
    RHIUnlockVertexBuffer(TangentBuffer.VertexBufferRHI);
    RHIUnlockVertexBuffer(TexCoordBuffer.VertexBufferRHI);
    RHIUnlockVertexBuffer(ColorBuffer.VertexBufferRHI);
#endif
}

void FSVFMeshVertexBuffer::ReleaseRHI()
{
    PositionBufferSRV.SafeRelease();
    TexCoordBufferSRV.SafeRelease();
    TangentBufferSRV.SafeRelease();
#if PLATFORM_WINDOWS
    ColorBufferSRV.SafeRelease();
#endif
}

void FSVFMeshVertexBuffer::InitResource()
{
    FRenderResource::InitResource();
    PositionBuffer.InitResource();
    TangentBuffer.InitResource();
    TexCoordBuffer.InitResource();
#if PLATFORM_WINDOWS
    ColorBuffer.InitResource();
#endif
}

void FSVFMeshVertexBuffer::ReleaseResource()
{
    FRenderResource::ReleaseResource();
    PositionBuffer.ReleaseResource();
    TangentBuffer.ReleaseResource();
    TexCoordBuffer.ReleaseResource();
#if PLATFORM_WINDOWS
    ColorBuffer.ReleaseResource();
#endif
}

void FSVFMeshVertexFactory::InitResource()
{
    FLocalVertexFactory* VertexFactory = this;
    const FSVFMeshVertexBuffer* SVFVertexBuffer = VertexBuffer;
    ENQUEUE_RENDER_COMMAND(InitDynamicMeshVertexFactory)(
        [VertexFactory, SVFVertexBuffer](FRHICommandListImmediate& RHICmdList)
        {
            FDataType Data;
            Data.PositionComponent = FVertexStreamComponent(
                &SVFVertexBuffer->PositionBuffer,
                0,
                sizeof(FVector),
                VET_Float3
            );

            Data.NumTexCoords = SVFVertexBuffer->GetNumTexCoords();
            {
                Data.LightMapCoordinateIndex = SVFVertexBuffer->GetLightmapCoordinateIndex();
                Data.TangentsSRV = SVFVertexBuffer->TangentBufferSRV;
                Data.TextureCoordinatesSRV = SVFVertexBuffer->TexCoordBufferSRV;
#if PLATFORM_WINDOWS
                Data.ColorComponentsSRV = SVFVertexBuffer->ColorBufferSRV;
#endif
                Data.PositionComponentSRV = SVFVertexBuffer->PositionBufferSRV;
            }
            {
                EVertexElementType UVDoubleWideVertexElementType = VET_None;
                EVertexElementType UVVertexElementType = VET_None;
                uint32 UVSizeInBytes = 0;
                if (SVFVertexBuffer->GetUse16bitTexCoords())
                {
                    UVSizeInBytes = sizeof(FVector2DHalf);
                    UVDoubleWideVertexElementType = VET_Half4;
                    UVVertexElementType = VET_Half2;
                }
                else
                {
                    UVSizeInBytes = sizeof(FVector2D);
                    UVDoubleWideVertexElementType = VET_Float4;
                    UVVertexElementType = VET_Float2;
                }

                int32 UVIndex;
                uint32 UvStride = UVSizeInBytes * SVFVertexBuffer->GetNumTexCoords();
                for (UVIndex = 0; UVIndex < (int32)SVFVertexBuffer->GetNumTexCoords() - 1; UVIndex += 2)
                {
                    Data.TextureCoordinates.Add
                    (
                        FVertexStreamComponent(
                            &SVFVertexBuffer->TexCoordBuffer,
                            UVSizeInBytes * UVIndex,
                            UvStride,
                            UVDoubleWideVertexElementType,
                            EVertexStreamUsage::ManualFetch
                        )
                    );
                }

                // possible last UV channel if we have an odd number
                if (UVIndex < (int32)SVFVertexBuffer->GetNumTexCoords())
                {
                    Data.TextureCoordinates.Add(FVertexStreamComponent(
                        &SVFVertexBuffer->TexCoordBuffer,
                        UVSizeInBytes * UVIndex,
                        UvStride,
                        UVVertexElementType,
                        EVertexStreamUsage::ManualFetch
                    ));
                }

                Data.TangentBasisComponents[0] = FVertexStreamComponent(&SVFVertexBuffer->TangentBuffer,
                    0, 2 * sizeof(FPackedNormal), VET_PackedNormal, EVertexStreamUsage::ManualFetch);
                Data.TangentBasisComponents[1] = FVertexStreamComponent(&SVFVertexBuffer->TangentBuffer,
                    sizeof(FPackedNormal), 2 * sizeof(FPackedNormal), VET_PackedNormal, EVertexStreamUsage::ManualFetch);
#if PLATFORM_WINDOWS
                Data.ColorComponent = FVertexStreamComponent(&SVFVertexBuffer->ColorBuffer,
                    0, sizeof(FColor), VET_Color, EVertexStreamUsage::ManualFetch);
#endif
            }
            VertexFactory->SetData(Data);
        });

    FLocalVertexFactory::InitResource();
}

FSVFMeshSceneProxy::FSVFMeshSceneProxy(USVFComponent* Component)
    : FPrimitiveSceneProxy(Component)
    , VertexBuffer(1, 0, true, Component->GetMaxVertexCount())
    , IndexBuffer(Component->GetMaxIndexCount())
    , VertexFactory(GetScene().GetFeatureLevel(), &VertexBuffer)
    , MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
    , BodySetup(Component->GetBodySetup())
{
    TSharedPtr<FFrameData> FrameData = Component->GetLastFrameData();
    if (FrameData.IsValid())
    {
        FSVFFrameInfo FrameInfo;
        if (FrameData->GetFrameInfo(FrameInfo))
        {
            if (FrameInfo.frameId > 0)
            {
                // Copy verts
                FrameData->GetFrameVertices(VertexBuffer.Vertices);
                // Copy index buffer
                FrameData->GetFrameIndices(IndexBuffer.Indices);
                // Enqueue initialization of render resource
                BeginInitResource(&VertexBuffer);
                BeginInitResource(&IndexBuffer);
                BeginInitResource(&VertexFactory);
            }
        }
    }

    // Grab material
    Material = Component->GetMaterial(0);
    if (Material == NULL)
    {
        Material = UMaterial::GetDefaultMaterial(MD_Surface);
    }
}

FSVFMeshSceneProxy::FSVFMeshSceneProxy(USVFComponent* Component, int InitialVertexCount, int InitialIndexCount)
    : FPrimitiveSceneProxy(Component)
    , VertexBuffer(1, 0, false, InitialVertexCount)
    , IndexBuffer(InitialIndexCount)
    , VertexFactory(GetScene().GetFeatureLevel(), &VertexBuffer)
    , MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
    , BodySetup(Component->GetBodySetup())
{
    BeginInitResource(&VertexBuffer);
    BeginInitResource(&IndexBuffer);
    BeginInitResource(&VertexFactory);

    // Grab material
    Material = Component->GetMaterial(0);
    if (Material == NULL)
    {
        Material = UMaterial::GetDefaultMaterial(MD_Surface);
    }
}

FSVFMeshSceneProxy::~FSVFMeshSceneProxy()
{
    VertexBuffer.ReleaseResource();
    IndexBuffer.ReleaseResource();
    VertexFactory.ReleaseResource();
}

void FSVFMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views,
    const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
    SCOPE_CYCLE_COUNTER(STAT_SVF_GetMeshElements);

    // Set up wireframe material (if needed)
    const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

    FColoredMaterialRenderProxy* WireframeMaterialInstance = NULL;
    if (bWireframe)
    {
        WireframeMaterialInstance = new FColoredMaterialRenderProxy(
#if ENGINE_MINOR_VERSION < 22
            GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
#else
            GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : NULL,
#endif
            FLinearColor(0, 0.5f, 1.f)
        );

        Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
    }

    if (IndexBuffer.Indices.Num() > 0 && IndexBuffer.IndexBufferRHI)
    {
#if ENGINE_MINOR_VERSION < 22
        FMaterialRenderProxy* MaterialProxy = bWireframe ? WireframeMaterialInstance : Material->GetRenderProxy(IsSelected());
#else
        FMaterialRenderProxy* MaterialProxy = bWireframe ? WireframeMaterialInstance : Material->GetRenderProxy();
#endif

        // For each view..
        for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
        {
            if (VisibilityMap & (1 << ViewIndex))
            {
                const FSceneView* View = Views[ViewIndex];
                // Draw the mesh.
                FMeshBatch& Mesh = Collector.AllocateMesh();
                FMeshBatchElement& BatchElement = Mesh.Elements[0];
                BatchElement.IndexBuffer = &IndexBuffer;
                Mesh.bWireframe = bWireframe;
                Mesh.VertexFactory = &VertexFactory;
                Mesh.MaterialRenderProxy = MaterialProxy;
#if ENGINE_MINOR_VERSION < 22
                BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(
                    GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, true);
#endif
                BatchElement.FirstIndex = 0;
                BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
                BatchElement.MinVertexIndex = 0;
                BatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;
                //Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
                Mesh.ReverseCulling = true;
                Mesh.Type = PT_TriangleList;
                Mesh.DepthPriorityGroup = SDPG_World;
                Mesh.bCanApplyViewModeOverrides = false;
                Collector.AddMesh(ViewIndex, Mesh);
            }
        }
    }

    // Draw bounds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
    {
        if (VisibilityMap & (1 << ViewIndex))
        {
            // Draw simple collision as wireframe if 'show collision', and collision is enabled,
            // and we are not using the complex as the simple
            if (ViewFamily.EngineShowFlags.Collision &&
                IsCollisionEnabled() &&
                BodySetup->CollisionTraceFlag != ECollisionTraceFlag::CTF_UseComplexAsSimple)
            {
                FTransform GeomTransform(GetLocalToWorld());
                BodySetup->AggGeom.GetAggGeom(GeomTransform, GetSelectionColor(FColor(157, 149, 223, 255),
                    IsSelected(), IsHovered()).ToFColor(true), NULL, false, false, true, ViewIndex, Collector);
            }

            // Render bounds
            RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
        }
    }
#endif
}

FPrimitiveViewRelevance FSVFMeshSceneProxy::GetViewRelevance(const FSceneView* View) const
{
    FPrimitiveViewRelevance Result;
    Result.bDrawRelevance = IsShown(View);
    Result.bShadowRelevance = IsShadowCast(View);
    Result.bDynamicRelevance = true;
    Result.bRenderInMainPass = ShouldRenderInMainPass();
    Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
    Result.bRenderCustomDepth = ShouldRenderCustomDepth();
    MaterialRelevance.SetPrimitiveViewRelevance(Result);
    return Result;
}


SIZE_T FSVFMeshSceneProxy::GetTypeHash() const
{
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
}

/**
 * Called on render thread to assign new dynamic data
 * Here we assume that this is not a new keyframe, and only update position/normals
 */
void FSVFMeshSceneProxy::Update_RenderThread(TSharedPtr<FFrameData> FrameData, int VertexCount, int IndexCount)
{
    SCOPE_CYCLE_COUNTER(STAT_SVF_UpdateMeshElements);

    check(IsInRenderingThread());
    if (FrameData.IsValid())
    {
#if PLATFORM_ANDROID
        FrameData->UnrealRenderEvent_RenderThread();
#endif
        FSVFFrameInfo FrameInfo;
        if (FrameData->GetFrameInfo(FrameInfo) && FrameInfo.frameId > 0)
        {
            if ((VertexCount > 0 && VertexCount > VertexBuffer.Vertices.Num()) ||
                FrameInfo.vertexCount > VertexBuffer.Vertices.Num())
            {
                WarnSVF("Resizing vertexBuffer from %d to max of (%d, %d)",
                    VertexBuffer.Vertices.Num(), VertexCount, FrameInfo.vertexCount);
                VertexBuffer.Reset(FMath::Max(VertexCount, FrameInfo.vertexCount));
            }
            if ((IndexCount > 0 && IndexCount > IndexBuffer.Indices.Num()) ||
                FrameInfo.indexCount > IndexBuffer.Indices.Num())
            {
                WarnSVF("Resizing indexBuffer from %d to max of (%d, %d)",
                    IndexBuffer.Indices.Num(), IndexCount, FrameInfo.indexCount);
                IndexBuffer.Reset(FMath::Max(IndexCount, FrameInfo.indexCount));
            }

            uint32 NumVertices = FrameInfo.vertexCount;
            uint32 NumIndicies = FrameInfo.indexCount;

            if (VertexBuffer.PositionBuffer.VertexBufferRHI &&
                VertexBuffer.TangentBuffer.VertexBufferRHI &&
                IndexBuffer.IndexBufferRHI)
            {
                FrameData->GetFrameIndices(IndexBuffer.Indices);
                FrameData->GetFrameVertices(VertexBuffer.Vertices);

                // Update Index buffer
                void* IndexBufferData = RHILockIndexBuffer(IndexBuffer.IndexBufferRHI,
                    0, NumIndicies * sizeof(int32), RLM_WriteOnly);
                FMemory::Memcpy(IndexBufferData, IndexBuffer.Indices.GetData(),
                    NumIndicies * sizeof(int32));
                RHIUnlockIndexBuffer(IndexBuffer.IndexBufferRHI);

                // Update Vertex, Tangent, TexCoord buffer
                FVector* PositionBufferData = static_cast<FVector*>(
                    RHILockVertexBuffer(VertexBuffer.PositionBuffer.VertexBufferRHI,
                        0, sizeof(FVector) * NumVertices, RLM_WriteOnly));
                FPackedNormal* TangentBufferData = static_cast<FPackedNormal*>(
                    RHILockVertexBuffer(VertexBuffer.TangentBuffer.VertexBufferRHI,
                        0, 2 * sizeof(FPackedNormal) * NumVertices, RLM_WriteOnly));

                uint32 NumTexCoords = VertexBuffer.GetNumTexCoords();
                bool Use16bitTexCoord = VertexBuffer.GetUse16bitTexCoords();

                void* TexCoordBufferData = RHILockVertexBuffer(VertexBuffer.TexCoordBuffer.VertexBufferRHI,
                    0, NumTexCoords * sizeof(FVector2D) * FrameInfo.vertexCount, RLM_WriteOnly);
                FVector2D* TexCoordBufferData32 = !Use16bitTexCoord ?
                    static_cast<FVector2D*>(TexCoordBufferData) : nullptr;
                FVector2DHalf* TexCoordBufferData16 = Use16bitTexCoord ?
                    static_cast<FVector2DHalf*>(TexCoordBufferData) : nullptr;

                for (uint32 i = 0; i < NumVertices; i++)
                {
                    PositionBufferData[i] = VertexBuffer.Vertices[i].Position;
                    TangentBufferData[2 * i + 0] = VertexBuffer.Vertices[i].TangentX;
                    TangentBufferData[2 * i + 1] = VertexBuffer.Vertices[i].TangentZ;

                    for (uint32 j = 0; j < NumTexCoords; j++)
                    {
                        if (Use16bitTexCoord)
                        {
                            TexCoordBufferData16[NumTexCoords * i + j] =
                                FVector2DHalf(VertexBuffer.Vertices[i].TextureCoordinate[j]);
                        }
                        else
                        {
                            TexCoordBufferData32[NumTexCoords * i + j] =
                                VertexBuffer.Vertices[i].TextureCoordinate[j];
                        }
                    }
                }

                RHIUnlockVertexBuffer(VertexBuffer.PositionBuffer.VertexBufferRHI);
                RHIUnlockVertexBuffer(VertexBuffer.TangentBuffer.VertexBufferRHI);
                RHIUnlockVertexBuffer(VertexBuffer.TexCoordBuffer.VertexBufferRHI);
            }
        }
    }
}

void FSVFMeshVertexBuffer::GetPositionalVertices(TArray<FVector>& OutVertices, int32 VerticesCount) const
{
    for (int32 VertexIndex = 0; VertexIndex < VerticesCount; VertexIndex++)
    {
        OutVertices.Push(Vertices[VertexIndex].Position);
    }
}

#undef LogSVF
#undef WarnSVF
#undef FatalSVF