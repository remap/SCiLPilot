// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "SVFSimpleInterface.h"
#include "SVFCallbackInterface.h"


#include "exports/SVFPluginExportFcns.h"
#include "exports/android/SVFExportFcns.h"
#include "exports/unreal/SVFExportFcns.h"

#include "SVFReaderAndroid.generated.h"

#if PLATFORM_ANDROID

typedef int UNITYHANDLE;

struct MeshVertex
{
    FVector pos;
    FVector normal;
    FVector2D uv;
};

struct UNREALSVF_API FFrameDataAndroid : public FFrameData
{
    FFrameDataAndroid(UNITYHANDLE InHandle, int InTextureHandle, int InTextureWidth,
        int InTextureHeight, const SVFFrameInfo& FrameInfo, bool InUseNormals = true);
    UNITYHANDLE UnityHandle;
    SVFFrameInfo m_FrameInfo;
    TArray<MeshVertex>* m_VertexBuffer;
    TArray<uint32_t>* m_IndexBuffer;
    int m_TextureHandle;
    int m_TextureWidth;
    int m_TextureHeight;
    bool bUseNormals;

    virtual void UnrealRenderEvent_RenderThread() override;
    virtual bool GetFrameInfo(FSVFFrameInfo& OutFrameInfo) override;

    virtual bool GetFrameVerticesWithNormal(TArray<FSVFVertexNorm>& OutVertices) override;
    virtual bool GetFrameVerticesWithoutNormal(TArray<FSVFVertex>& OutVertices) override;
    virtual bool GetFrameVertices(TArray<FDynamicMeshVertex>& OutVertices) override;
    virtual bool GetFrameVertices(FDynamicMeshVertex* OutVertices) override;

    virtual bool GetFrameIndices(TArray<int32>& OutIndices) override;
    virtual bool GetFrameIndices(int32* OutIndices) override;

    virtual bool GetTextureInfo(EPixelFormat& OutPixelFormat, int32& OutWidth, int32& OutHeight) override;
    virtual bool GetTextureBuffer(uint8** OutData, int32& OutDataSize) override;
    virtual bool CopyTextureBuffer(UTexture2D* WorkTexture, bool UseHardwareTextureCopy = true) override;
};
#endif


UCLASS()
class UNREALSVF_API USVFReaderAndroid : public UObject, public ISVFSimpleInterface, public ISVFCallbackInterface
{
    GENERATED_BODY()

public:

    USVFReaderAndroid();
    ~USVFReaderAndroid();

    static bool CreateInstance(UObject* InOwner, const FString& FilePath, const FSVFOpenInfo& OpenInfo, UObject** OutWrapper, UObject* CustomClockObject);

#if PLATFORM_ANDROID
    static bool bIsInitialized;

    virtual void Start() override;
    virtual void Stop() override;
    virtual void Rewind() override;
    virtual void Close() override;
    virtual bool BeginPlayback() override;
    virtual void GetFrameInfo(FSVFFrameInfo& OutFrameInfo) override;
    virtual bool GetFrameData(TSharedPtr<FFrameData>& OutFrameDataPtr) override;
    virtual bool CanSeek() override;
    virtual bool SeekToPercent(float SeekToPercent) override;
    virtual void SetReaderClockScale(float ClockScale) override;
    virtual void SetPlayFlow(bool bIsForwardPlay) override {};

    virtual FSVFFileInfo GetFileInfo() override
    {
        return m_FileInfo;
    }

    void ResizeBuffers(int VertexBufferSize, int IndexBufferSize);
    void EnqueueSetUnrealTexture(UTexture2D* Texture);
    bool Open(const FString& FilePath, const FSVFOpenInfo& OpenInfo);

    static void ConvertSVFFileInfo(const SVFFileInfo& InFileInfo, FSVFFileInfo& OutFileInfo);
    static void ConvertSVFFrameInfo(const SVFFrameInfo& InFrameInfo, FSVFFrameInfo& OutFrameInfo);
    static void ConvertFSVFOpenInfo(const FSVFOpenInfo& InOpenInfo, SVFOpenInfo& OutOpenInfo);

    FORCEINLINE int GetUnityHandle()
    {
        return UnityHandle;
    }

    FORCEINLINE void SetUnityHandle(int Handle)
    {
        UnityHandle = Handle;
    }

    FORCEINLINE bool IsUnityHandleValid()
    {
        return UnityHandle != -1;
    }
#endif

protected:
    FSVFFileInfo m_FileInfo;

private:
#if PLATFORM_ANDROID
    UNITYHANDLE UnityHandle = -1;
    SVFFrameInfo m_FrameInfo;
    SVFOpenInfo m_OpenInfo;
    TArray<MeshVertex> m_VertexBuffer;
    TArray<uint32_t> m_IndexBuffer;

    int m_TextureHandle;
    int m_TextureWidth;
    int m_TextureHeight;
#endif
};
