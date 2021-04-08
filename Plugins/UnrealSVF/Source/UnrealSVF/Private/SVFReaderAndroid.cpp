// Copyright (C) Microsoft Corporation. All rights reserved.

#include "SVFReaderAndroid.h"

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 26
#include "Rendering/Texture2DResource.h"
#endif

#if PLATFORM_ANDROID
#include "Misc/FileHelper.h"

DEFINE_LOG_CATEGORY_STATIC(LogSVFReaderAndroid, Log, All);

#define LogSVF(pmt, ...) UE_LOG(LogSVFReaderAndroid, Log, TEXT(pmt), ##__VA_ARGS__)
#define WarnSVF(pmt, ...) UE_LOG(LogSVFReaderAndroid, Warning, TEXT(pmt), ##__VA_ARGS__)
#define FatalSVF(pmt, ...) UE_LOG(LogSVFReaderAndroid, Fatal, TEXT(pmt), ##__VA_ARGS__)

extern FString GExternalFilePath;

FFrameDataAndroid::FFrameDataAndroid(UNITYHANDLE InHandle, int InTextureHandle,
    int InTextureWidth, int InTextureHeight, const SVFFrameInfo& InFrameInfo, bool InUseNormals)
    : UnityHandle(InHandle)
    , m_TextureHandle(InTextureHandle)
    , m_TextureWidth(InTextureWidth)
    , m_TextureHeight(InTextureHeight)
    , m_FrameInfo(InFrameInfo)
    , bUseNormals(InUseNormals)
{
    bIsValid = false;

    m_FrameInfo.frameId = 0;
    UnityHandle = InHandle;
    bUseNormals = InUseNormals;

    bIsValid = true;
}

bool FFrameDataAndroid::GetFrameInfo(FSVFFrameInfo& OutFrameInfo)
{
    m_FrameInfo.frameId = 0;
    GetHCapObjectFrameInfo(UnityHandle, &m_FrameInfo);
    USVFReaderAndroid::ConvertSVFFrameInfo(m_FrameInfo, OutFrameInfo);
    return true;
}

void FFrameDataAndroid::UnrealRenderEvent_RenderThread()
{
    check(IsInRenderingThread);
    if (m_TextureHandle > 0)
    {
        OnUnrealRenderEvent(UnityHandle, m_VertexBuffer->GetData(), m_IndexBuffer->GetData(),
            m_VertexBuffer->Num(), m_IndexBuffer->Num(), m_TextureHandle, m_TextureWidth, m_TextureHeight);
    }
}

bool FFrameDataAndroid::GetFrameVerticesWithNormal(TArray<FSVFVertexNorm>& OutVertices)
{
    WarnSVF("GetFrameVerticesWithNormal not implemented");
    return false;
}

bool FFrameDataAndroid::GetFrameVerticesWithoutNormal(TArray<FSVFVertex>& OutVertices)
{
    WarnSVF("GetFrameVerticesWithoutNormal not implemented");
    return false;
}

bool FFrameDataAndroid::GetFrameVertices(TArray<FDynamicMeshVertex>& OutVertices)
{
    if (OutVertices.Num() != m_VertexBuffer->Num())
    {
        OutVertices.SetNumUninitialized(m_VertexBuffer->Num());
    }
    MeshVertex* pVertexBuffer = (MeshVertex*) m_VertexBuffer->GetData();
    if (bUseNormals)
    {
        for (uint32 i = 0; i < m_FrameInfo.vertexCount; ++i)
        {
            FDynamicMeshVertex& Vert = OutVertices[i];
            Vert.Position = FVector(pVertexBuffer[i].pos.Z, pVertexBuffer[i].pos.X, pVertexBuffer[i].pos.Y);
            Vert.TextureCoordinate[0] = FVector2D(pVertexBuffer[i].uv.X, pVertexBuffer[i].uv.Y);
            Vert.TangentX = FVector(1, 0, 0);
            Vert.TangentZ = FVector(pVertexBuffer[i].normal.Z, pVertexBuffer[i].normal.X, pVertexBuffer[i].normal.Y);
            Vert.TangentZ.Vector.W = 127;
            Vert.Color = FColor(255, 255, 255);
        }
    }
    else
    {
        for (uint32 i = 0; i < m_FrameInfo.vertexCount; ++i)
        {
            FDynamicMeshVertex& Vert = OutVertices[i];
            Vert.Position = FVector(pVertexBuffer[i].pos.Z, pVertexBuffer[i].pos.X, pVertexBuffer[i].pos.Y);
            Vert.TextureCoordinate[0] = FVector2D(pVertexBuffer[i].uv.X, pVertexBuffer[i].uv.Y);
            Vert.TangentX = FVector(1, 0, 0);
            Vert.TangentZ = FVector(0, 0, 1);
            Vert.TangentZ.Vector.W = 127;
            Vert.Color = FColor(255, 255, 255);
        }
    }
    return true;
}

bool FFrameDataAndroid::GetFrameVertices(FDynamicMeshVertex* OutVertices)
{
    WarnSVF("GetFrameVertices not implemented");
    return false;
}

bool FFrameDataAndroid::GetFrameIndices(TArray<int32>& OutIndices)
{
    if (OutIndices.Num() != m_IndexBuffer->Num())
    {
        OutIndices.SetNumUninitialized(m_IndexBuffer->Num());
    }
    FMemory::Memcpy(OutIndices.GetData(), m_IndexBuffer->GetData(), sizeof(uint32_t) * m_IndexBuffer->Num());
    return true;
}

bool FFrameDataAndroid::GetFrameIndices(int32* OutIndices)
{
    return false;
}

bool FFrameDataAndroid::GetTextureInfo(EPixelFormat& OutPixelFormat, int32& OutWidth, int32& OutHeight)
{
    GetHCapObjectFrameInfo(UnityHandle, &m_FrameInfo);
    OutPixelFormat = EPixelFormat::PF_R8G8B8A8;
    OutWidth = m_FrameInfo.textureWidth;
    OutHeight = m_FrameInfo.textureHeight;
    return true;
}

bool FFrameDataAndroid::GetTextureBuffer(uint8** OutData, int32& OutDataSize)
{
    WarnSVF("GetTextureBuffer not implemented");
    return false;
}

// For UE4, we provide the native resource handle directly to SVF to copy texture
bool FFrameDataAndroid::CopyTextureBuffer(UTexture2D* WorkTexture, bool UseHardwareTextureCopy)
{
    return true;
}

bool USVFReaderAndroid::bIsInitialized = false;
#endif

USVFReaderAndroid::USVFReaderAndroid()
{
#if PLATFORM_ANDROID

#endif
}

USVFReaderAndroid::~USVFReaderAndroid()
{
#if PLATFORM_ANDROID
    //FSVFTexturePool::ReturnTexture(m_VideoTexture);
    if (IsUnityHandleValid())
    {
        DestroyHCapObject(GetUnityHandle());
    }
#endif
}

bool USVFReaderAndroid::CreateInstance(UObject* InOwner, const FString& FilePath,
    const FSVFOpenInfo& OpenInfo, UObject** OutWrapper, UObject* CustomClockObject)
{
#if PLATFORM_ANDROID
    if (!bIsInitialized)
    {
        bIsInitialized = true;
    }

    USVFReaderAndroid* pWrapperObj = NewObject<USVFReaderAndroid>(InOwner);
    UNITYHANDLE Handle = CreateHCapObject(nullptr);
    pWrapperObj->SetUnityHandle(Handle);
    if (!pWrapperObj->IsUnityHandleValid())
    {
        WarnSVF("Failed to create HCapObject");
        return false;
    }

    ConvertFSVFOpenInfo(OpenInfo, pWrapperObj->m_OpenInfo);
    SetComputeNormals(Handle, false);

    if (!OpenHCapObject(Handle, TCHAR_TO_UTF8(*(GExternalFilePath / FilePath)), &pWrapperObj->m_OpenInfo))
    {
        WarnSVF("Failed to open HCap file");
        return false;
    }

    SVFFileInfo CrossPlatformFileInfo = {};
    GetHCapObjectFileInfo(Handle, &CrossPlatformFileInfo);
    ConvertSVFFileInfo(CrossPlatformFileInfo, pWrapperObj->m_FileInfo);

    uint32_t maxVertices = CrossPlatformFileInfo.maxVertexCount;
    uint32_t maxIndices = CrossPlatformFileInfo.maxIndexCount;
    pWrapperObj->m_VertexBuffer.SetNumUninitialized(maxVertices, true);
    pWrapperObj->m_IndexBuffer.SetNumUninitialized(maxIndices, true);

    *OutWrapper = pWrapperObj;
    return true;
#else
    return false;
#endif
}

#if PLATFORM_ANDROID
bool USVFReaderAndroid::Open(const FString& FilePath, const FSVFOpenInfo& OpenInfo)
{
    ConvertFSVFOpenInfo(OpenInfo, m_OpenInfo);
    SetComputeNormals(GetUnityHandle(), false);

    if (!OpenHCapObject(GetUnityHandle(), TCHAR_TO_UTF8(*(GExternalFilePath / FilePath)), &m_OpenInfo))
    {
        WarnSVF("Failed to open HCap file");
        return false;
    }

    SVFFileInfo CrossPlatformFileInfo = {};
    GetHCapObjectFileInfo(GetUnityHandle(), &CrossPlatformFileInfo);
    ConvertSVFFileInfo(CrossPlatformFileInfo, m_FileInfo);

    uint32_t maxVertices = CrossPlatformFileInfo.maxVertexCount;
    uint32_t maxIndices = CrossPlatformFileInfo.maxIndexCount;

    m_VertexBuffer.SetNumUninitialized(maxVertices, true);
    m_IndexBuffer.SetNumUninitialized(maxIndices, true);

    return true;
}

void USVFReaderAndroid::Start()
{
    if (IsUnityHandleValid())
    {
        PlayHCapObject(GetUnityHandle());
    }
}

void USVFReaderAndroid::Stop()
{
    if (IsUnityHandleValid())
    {
        PauseHCapObject(GetUnityHandle());
    }
}

void USVFReaderAndroid::Rewind()
{
    if (IsUnityHandleValid())
    {
        RewindHCapObject(GetUnityHandle());
    }
}

void USVFReaderAndroid::Close()
{
    if (IsUnityHandleValid())
    {
        CloseHCapObject(GetUnityHandle());
    }
}

bool USVFReaderAndroid::BeginPlayback()
{
    WarnSVF("BeginPlayback is a no-op for Android");
    return true;
}

void USVFReaderAndroid::GetFrameInfo(FSVFFrameInfo& OutFrameInfo)
{
    m_FrameInfo.frameId = 0;
    GetHCapObjectFrameInfo(GetUnityHandle(), &m_FrameInfo);
    ConvertSVFFrameInfo(m_FrameInfo, OutFrameInfo);
}

bool USVFReaderAndroid::GetFrameData(TSharedPtr<FFrameData>& OutFrameDataPtr)
{
    if (!IsUnityHandleValid())
    {
        return false;
    }

    auto framedata = new FFrameDataAndroid(GetUnityHandle(), m_TextureHandle,
        m_TextureWidth, m_TextureHeight, m_FrameInfo, m_FileInfo.hasNormals);
    framedata->m_VertexBuffer = &m_VertexBuffer;
    framedata->m_IndexBuffer = &m_IndexBuffer;
    OutFrameDataPtr = MakeShareable(framedata);

    return OutFrameDataPtr->bIsValid;
}

bool USVFReaderAndroid::CanSeek()
{
    // TODO:
    return false;
}

bool USVFReaderAndroid::SeekToPercent(float SeekToPercent)
{
    if (SeekToPercent == 0.0f)
    {
        RewindHCapObject(GetUnityHandle());
        return true;
    }
    return false;
}

void USVFReaderAndroid::SetReaderClockScale(float ClockScale)
{
    SetClockScale(UnityHandle, ClockScale);
}

void USVFReaderAndroid::ResizeBuffers(int VertexBufferSize, int IndexBufferSize)
{
    m_VertexBuffer.SetNumUninitialized(VertexBufferSize);
    m_IndexBuffer.SetNumUninitialized(IndexBufferSize);
}

void USVFReaderAndroid::EnqueueSetUnrealTexture(UTexture2D* Texture)
{
    // Invalidate the texture until render thread creates the resource
    m_TextureHandle = -1;
    ENQUEUE_RENDER_COMMAND(FSVFSetUnrealTexture)(
		[=](FRHICommandListImmediate& RHICmdList)
        {
            FTexture2DResource* resource = (FTexture2DResource*) Texture->Resource;
            this->m_TextureHandle = *reinterpret_cast<int32*>(
                resource->GetTexture2DRHI()->GetNativeResource());
            this->m_TextureWidth = Texture->GetSizeX();
            this->m_TextureHeight = Texture->GetSizeY();
        }
    );
}

void USVFReaderAndroid::ConvertSVFFileInfo(const SVFFileInfo& InFileInfo, FSVFFileInfo& OutFileInfo)
{
    OutFileInfo.hasAudio = InFileInfo.hasAudio;
    OutFileInfo.Duration = InFileInfo.duration100ns;
    OutFileInfo.FrameCount = InFileInfo.frameCount;
    OutFileInfo.MaxVertexCount = InFileInfo.maxVertexCount;
    OutFileInfo.MaxIndexCount = InFileInfo.maxIndexCount;
    OutFileInfo.FileWidth = InFileInfo.fileWidth;
    OutFileInfo.FileHeight = InFileInfo.fileHeight;
    OutFileInfo.BitrateMbps = InFileInfo.bitrateMbps;
    OutFileInfo.MaxBounds = FBox(FVector(InFileInfo.minZ, InFileInfo.minX, InFileInfo.minY),
        FVector(InFileInfo.maxZ, InFileInfo.maxX, InFileInfo.maxY));
    OutFileInfo.Duration = InFileInfo.duration100ns;
    OutFileInfo.hasNormals = InFileInfo.hasNormals;
    // TODO: following aren't part of FSVFFileInfo:
    // float fileSize;
    // uint32_t fileWidth, fileHeight;
}

void USVFReaderAndroid::ConvertSVFFrameInfo(const SVFFrameInfo& InFrameInfo, FSVFFrameInfo& OutFrameInfo)
{
    OutFrameInfo.frameTimestamp = FTimespan(InFrameInfo.frameTimestamp);
    OutFrameInfo.Bounds = FBox(FVector(InFrameInfo.minZ, InFrameInfo.minX, InFrameInfo.minY),
        FVector(InFrameInfo.maxZ, InFrameInfo.maxX, InFrameInfo.maxY));
    OutFrameInfo.frameId = InFrameInfo.frameId;
    OutFrameInfo.vertexCount = InFrameInfo.vertexCount;
    OutFrameInfo.indexCount = InFrameInfo.indexCount;
    OutFrameInfo.textureWidth = InFrameInfo.textureWidth;
    OutFrameInfo.textureHeight = InFrameInfo.textureHeight;
    OutFrameInfo.isEOS = InFrameInfo.isEOS;
    OutFrameInfo.isRepeatedFrame = InFrameInfo.isRepeatedFrame;
    OutFrameInfo.isKeyFrame = InFrameInfo.isKeyFrame;
}

void USVFReaderAndroid::ConvertFSVFOpenInfo(const FSVFOpenInfo& InOpenInfo, SVFOpenInfo& OutOpenInfo)
{
    OutOpenInfo.AudioDisabled = InOpenInfo.AudioDisabled == 1;
    // UseKeyedMutex irrelevant for non-Windows
    OutOpenInfo.RenderViaClock = InOpenInfo.RenderViaClock == 1;
    OutOpenInfo.OutputNormals = InOpenInfo.OutputNormals == 1;
    // StartDownloadOnOpen irrelevant for non-dash stream readers
    OutOpenInfo.AutoLooping = InOpenInfo.AutoLooping == 1;
    OutOpenInfo.forceSoftwareClock = InOpenInfo.forceSoftwareClock == 1;
    OutOpenInfo.playbackRate = InOpenInfo.playbackRate;
    OutOpenInfo.hrtf.MinGain = InOpenInfo.hrtf.MinGain;
    OutOpenInfo.hrtf.MaxGain = InOpenInfo.hrtf.MaxGain;
    OutOpenInfo.hrtf.GainDistance = InOpenInfo.hrtf.GainDistance;
    OutOpenInfo.hrtf.CutoffDistance = InOpenInfo.hrtf.CutoffDistance;
}

#endif

#undef LogSVF
#undef WarnSVF
#undef FatalSVF
