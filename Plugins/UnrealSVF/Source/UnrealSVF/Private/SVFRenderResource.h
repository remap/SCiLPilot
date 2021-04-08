// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include "Containers/StaticArray.h"
#include "LocalVertexFactory.h"
#include "Engine.h"
#include "Components/MeshComponent.h"

class FSVFVertexBuffer : public FVertexBuffer
{
protected:
    const int32 Stride;
    int32 NumVertices;
    FShaderResourceViewRHIRef ShaderResourceView;

public:
    FSVFVertexBuffer(int32 InVertexSize);
    ~FSVFVertexBuffer() {}

    virtual void InitRHI() override;
    void Reset(int32 InNumVertices);

    FORCEINLINE int32 Num() { return NumVertices; }
    FORCEINLINE int32 GetBufferSize() const { return NumVertices * Stride; }

    void SetNum(int32 InNumVertices);
    void SetData(const TArray<uint8>& Data);

    virtual void Bind(FLocalVertexFactory::FDataType& DataType) = 0;

protected:
    virtual void CreateSRV() = 0;
};

class FSVFPositionBuffer : public FSVFVertexBuffer
{
public:
    FSVFPositionBuffer()
        : FSVFVertexBuffer(sizeof(FVector)) {}

    virtual void Bind(FLocalVertexFactory::FDataType& DataType) override
    {
        DataType.PositionComponent = FVertexStreamComponent(this, 0, 12, VET_Float3);
        DataType.PositionComponentSRV = ShaderResourceView;
    }

protected:
    virtual void CreateSRV() override
    {
        ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, 4, PF_R32_FLOAT);
    }
};

class FSVFNormalBuffer : public FSVFVertexBuffer
{
    const bool bUseHighPrecision;

public:
    FSVFNormalBuffer(bool bInUseHighPrecision = false)
        : FSVFVertexBuffer(2 * (bInUseHighPrecision ? sizeof(FPackedRGBA16N) : sizeof(FPackedNormal)))
        , bUseHighPrecision(bInUseHighPrecision)
    {}

    virtual void Bind(FLocalVertexFactory::FDataType& DataType) override
    {
        uint32 TangentXOffset = 0;
        uint32 TangentZOffset = sizeof(FPackedNormal);
        EVertexElementType TangentElementType = VET_PackedNormal;

        if (bUseHighPrecision)
        {
            TangentElementType = VET_UShort4N;
            TangentZOffset = sizeof(FPackedRGBA16N);
        }

        DataType.TangentBasisComponents[0] = FVertexStreamComponent(this,
            TangentXOffset, Stride, TangentElementType, EVertexStreamUsage::ManualFetch);
        DataType.TangentBasisComponents[1] = FVertexStreamComponent(this,
            TangentZOffset, Stride, TangentElementType, EVertexStreamUsage::ManualFetch);
        DataType.TangentsSRV = ShaderResourceView;
    }

protected:
    virtual void CreateSRV() override
    {
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
        ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI,
            bUseHighPrecision ? 8 : 4, bUseHighPrecision ? PF_R16G16B16A16_SNORM : PF_R8G8B8A8_SNORM);
#else // For 4.19
        ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI,
            bUseHighPrecision ? 8 : 4, bUseHighPrecision ? PF_A16B16G16R16 : PF_R8G8B8A8);
#endif
    }
};

class FSVFTexCoordBuffer : public FSVFVertexBuffer
{
    bool bUseHighPrecision;
    int32 NumUVs;

public:
    FSVFTexCoordBuffer(bool bInUseHighPrecision = false)
        : FSVFVertexBuffer(bInUseHighPrecision ? sizeof(FVector2D) : sizeof(FVector2DHalf))
        , bUseHighPrecision(bInUseHighPrecision)
    {}

    virtual void Bind(FLocalVertexFactory::FDataType& DataType) override
    {
        DataType.TextureCoordinates.Empty();
        DataType.NumTexCoords = NumUVs;

        EVertexElementType UVDoubleWideVertexElementType = VET_Half4;
        EVertexElementType UVVertexElementType = VET_Half2;

        if (bUseHighPrecision)
        {
            UVDoubleWideVertexElementType = VET_Float4;
            UVVertexElementType = VET_Float2;
        }

        DataType.TextureCoordinates.Add(FVertexStreamComponent(this,
            0, Stride, UVDoubleWideVertexElementType, EVertexStreamUsage::ManualFetch));
        DataType.TextureCoordinatesSRV = ShaderResourceView;
    }

protected:
    virtual void CreateSRV() override
    {
        ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI,
            Stride, bUseHighPrecision ? PF_G32R32F : PF_G16R16F);
    }
};


class FSVFIndexBuffer : public FIndexBuffer
{
private:
    int32 NumIndices;
    int32 Stride;

public:
    FSVFIndexBuffer();
    ~FSVFIndexBuffer() {}

    virtual void InitRHI() override;
    void Reset(int32 InStride, int32 InNumIndices);

    FORCEINLINE int32 Num() { return NumIndices; }
    FORCEINLINE int32 GetBufferSize() const { return NumIndices * Stride; }

    void SetNum(int32 InNumIndices);
    void SetData(const TArray<uint8>& Data);
};

class FSVFSceneProxy;
class FSVFVertexFactory : public FLocalVertexFactory
{
public:

    FSVFVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, FSVFSceneProxy* InSectionParent);

    void Init(FLocalVertexFactory::FDataType VertexStructure);

private:
    FSVFSceneProxy* SectionParent;
};

class FSVFSceneProxy : public FPrimitiveSceneProxy
{
public:

    FSVFSceneProxy();

    virtual ~FSVFSceneProxy()
    {}

    SIZE_T GetTypeHash() const
    {
        static size_t UniquePointer;
        return reinterpret_cast<size_t>(&UniquePointer);
    }

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views,
        const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
    {
    }

private:
    UMaterialInterface* Material;
    FMaterialRelevance MaterialRelevance;
};