// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DynamicMeshBuilder.h"
#include "SVFTypes.generated.h"

// Forward declaration
class UTexture2D;

/**
 * ESVFClockState returns the state of our current clock
**/
UENUM()
enum class EUSVFClockState : uint8 {
    Running UMETA(DisplayName = "Specifies that the clock is running"),
    Stopped UMETA(DisplayName = "Specifies that the clock has stopped")
};

/**
 * ESVFLoop defines the various ways we can loop playback
 * when we reach the end of the stream.
**/
UENUM()
enum class EUSVFLoop : uint8 {
    LoopViaRewind UMETA(DisplayName = "Currently unsupported"),
    LoopViaRestart UMETA(DisplayName = "Forces playback to restart from beginning"),
    NoLooping UMETA(DisplayName = "Forces playback to stop at end of stream")
};

/**
 * ESVFReaderState defines the various states the ISVFReader may be in
**/
UENUM()
enum class EUSVFReaderState {
    Unknown UMETA(DisplayName = "Reader is in unknown state"),
    Initialized UMETA(DisplayName = "Reader was properly initialized"),
    OpenPending UMETA(DisplayName = "Open is pending"),
    Opened UMETA(DisplayName = "Open finished"),
    Prerolling UMETA(DisplayName = "File opened and pre-roll started"),
    Ready UMETA(DisplayName = "Frames ready to be delivered"),
    Buffering UMETA(DisplayName = "Reader is buffering frames (i.e. frames not available for delivery)"),
    Closing UMETA(DisplayName = "File being closed"),
    Closed UMETA(DisplayName = "Filed closed"),
    EndOfStream UMETA(DisplayName = "Reached end of current file"),
    ShuttingDown UMETA(DisplayName = "SVFReader is in process of shutting down"),
};

UENUM()
enum class EInternalStateFlags : uint8 {
    None = 0x00,
    Cached = 0x01, // cached version of playback mode
    Live = 0x02, // rendering directly off SVF
    DiscFlushing = 0x4, // currently flushing data to temp files
    ReaderThread = 0x8, // in cached mode, SVF reading thread is still active
};

//**********************************************************************
// Diagnostic status of SVF reader
//**********************************************************************
USTRUCT(Blueprintable, BlueprintType)
struct UNREALSVF_API FSVFStatus {
    GENERATED_USTRUCT_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        bool isLiveSVFSource;

    uint32 lastReadFrame;

    uint32 unsuccessfulReadFrameCount;

    uint32 droppedFrameCount;

    int32 errorHresult;

    // cast of ESVFReaderState
    int lastKnownState;

};

USTRUCT(Blueprintable, BlueprintType)
struct UNREALSVF_API FHrtfAudioDecaySettings {
    GENERATED_USTRUCT_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        float MinGain;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        float MaxGain;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        float GainDistance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        float CutoffDistance;

    FHrtfAudioDecaySettings();

};

/**
 * SVFReaderConfiguration defines general configuration information use to initialize an ISVFreader object
 *
 * This objects defines general configuration information used to setup an ISVFReader object.
 * This structure is passed to SVFCreateReaderWithConfig when the client creates the reader.
**/
USTRUCT(Blueprintable, BlueprintType)
struct FSVFReaderConfiguration {
    GENERATED_USTRUCT_BODY();

    // If true, run low - priority cache cleanup thread upon initializing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        bool performCacheCleanup;

    // Custom cache location. If null or empty, SVF reader will use default cache
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        FString cacheLocation;

    FSVFReaderConfiguration()
        : performCacheCleanup(true)
        , cacheLocation(TEXT(""))
    {
    }
};

UENUM(BlueprintType)
enum class EUSVFOpenPreset : uint8 {
    FirstMovie,
    SmallMemoryBuffer_DropIfNeed,
    BigMemoryBuffer_DropIfNeed,
    DefaultPreset_DropIfNeed,
    DefaultPreset_NoDropping,
    NoCache_FirstMovie,
    NoCache_Default,
    NoCache_Variant2,
    HardCore_NoHW
};

USTRUCT(Blueprintable, BlueprintType)
struct UNREALSVF_API FAudioDeviceInfo
{
    GENERATED_USTRUCT_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
    FString Id;
};

USTRUCT(Blueprintable, BlueprintType)
struct UNREALSVF_API FSVFOpenInfo {
    GENERATED_USTRUCT_BODY();

    // Do not play audio even if it is present
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        uint32 AudioDisabled : 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        uint32 UseKeyedMutex : 1;

    // Call SVF GetFrameViaClock rather than GetFrame
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        uint32 RenderViaClock : 1;

    // If true, add normals in returned vertex buffer
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        uint32 OutputNormals : 1;

    // if true, SVFReader starts download on Open(), independently on MF calls. If SVF does not get destroyed ahead of time, 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        uint32 StartDownloadOnOpen : 1;

    // if true, plugin will issue EOS, but will take care of rewinding to beginning
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        uint32 AutoLooping : 1;

    // if true, we enforce custom software clock to enable clock scaling. If false: filers with audio will have default scaling, files with no audio: no scaling
    // @TODO: remove it and force to TRUE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        uint32 forceSoftwareClock : 1;

    // HCap content gets pre-cached.
    // Used to speed up playback (1.0f = normal playback)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        float playbackRate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        FHrtfAudioDecaySettings hrtf;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        FString AudioDeviceId;

    FSVFOpenInfo();

    FSVFOpenInfo(EUSVFOpenPreset PresetMode);

};

/**
 * SVFConfiguration defines how to setup a playback session
 *
 * SVFConfiguration is passed to @see ISVFReader "ISVFReader's" Open() call
 * to configure various playback values and parameters.
**/
USTRUCT(Blueprintable, BlueprintType)
struct FSVFConfiguration {
    GENERATED_USTRUCT_BODY();

    // Should we use hardware decoding?
    bool useHardwareDecode;
    // Should output be kept in a hardware texture (ID3D11Texture2D)?
    bool useHardwareTextures;
    // Should output be in NV12 format (otherwise it is in RGB)?
    bool outputNV12;
    // Should we buffer the incoming compressed data?
    bool bufferCompressedData;
    // Should HW textures used keyed mutex for synchronization?
    bool useKeyedMutex;
    // Should we disable audio (audio won't even be decoded)?
    bool disableAudio;
    // Should SVF handle playback of audio?
    bool playAudio;
    // Should audio be decoded and returned via GetNextAudioPacket()?
    bool returnAudio;
    // Number of frames to preroll
    uint32 prerollSize;
    // Minimum number of frames that must be buffered (otherwise we go to buffering state)
    uint32 minBufferSize;
    // Maximum number of frames to buffer
    uint32 maxBufferSize;
    // Largest our buffer can ever grow to
    uint32 maxBufferUpperBound;
    // Minimum number of buffered frames before restarting playback
    uint32 bufferHysteresis;
    // The minimum number of compressed bytes that must be buffered before exiting buffering state.
    uint32 minCompressedBufferSize;
    // The maximum number of compressed bytes to buffer
    uint32 maxCompressedBufferSize;
    // Number of milliseconds late a frame can be before being considered for dropping
    int64 allowableDelay;
    // Should we allow dropped frames (otherwise every frame is returned even if playback is slowed)
    bool allowDroppedFrames;
    // Maximum number of dropped frames allowed per call to GetNextFrameViaClock()
    uint32 maxDropFramesPerCall;
    // Estimate of latency between time frame is returned via GetNextFrame() and time it is drawn
    int64 presentationLatency;
    // Type of looping
    EUSVFLoop looping;
    // do not wait for MF calls to download file to disk cache, instead, start a separate download thread inside Open().
    bool startDownloadWhenOpen;
    // Scale up the audio clock (1.0f = normal rate)
    float clockScale;
    // Set of flags (see namespace ESVFOutputVertexFormat) describing which components make up the output vertex
    uint32 outputVertexFormat;

    FSVFConfiguration();

};

USTRUCT(Blueprintable, BlueprintType)
struct UNREALSVF_API FSVFFrameInfo {
    GENERATED_USTRUCT_BODY();

    // Current frame time
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        FTimespan frameTimestamp = FTimespan(0L);

    // in 100ns MF time units
    //uint64 frameTimestamp = 0;

    // Current frame bounding box
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        FBox Bounds = FBox(ForceInitToZero);

    // Current frame ID (starts from 0)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        int32 frameId = 0;

    // Vertex count in this frame
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        int32 vertexCount = 0;

    // Index count in this frame
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        int32 indexCount = 0;

    // Texture sizeX (width)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        int32 textureWidth = 0;

    // Texture sizeY (height)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        int32 textureHeight = 0;

    // Is end frame
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        bool isEOS = false;

    // Is repeated frame
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        bool isRepeatedFrame = false;

    // Is repeated frame
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        bool isKeyFrame = false;

    FSVFFrameInfo() {}

};

FORCEINLINE FArchive& operator<<(FArchive &Ar, FSVFFrameInfo& SVFFrameInfo) {
    Ar << SVFFrameInfo.frameId;
    Ar << SVFFrameInfo.frameTimestamp;
    Ar << SVFFrameInfo.Bounds;
    Ar << SVFFrameInfo.vertexCount;
    Ar << SVFFrameInfo.indexCount;
    Ar << SVFFrameInfo.textureWidth;
    Ar << SVFFrameInfo.textureHeight;
    Ar << SVFFrameInfo.isEOS;
    Ar << SVFFrameInfo.isRepeatedFrame;
    Ar << SVFFrameInfo.isKeyFrame;
    return Ar;
};

/**
 * Define our vertex without normals for our output buffer
**/
USTRUCT(Blueprintable, BlueprintType)
struct UNREALSVF_API FSVFVertex {
    GENERATED_USTRUCT_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        FVector Position;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        FVector2D UV;

    FSVFVertex()
        : Position(ForceInitToZero)
        , UV(ForceInitToZero)
    {}

    FSVFVertex(const FVector& InPos, const FVector2D& InUV)
        : Position(InPos)
        , UV(InUV)
    {}

};

/**
* Define our vertex with normals for our output buffer
**/
USTRUCT(Blueprintable, BlueprintType)
struct UNREALSVF_API FSVFVertexNorm {
    GENERATED_USTRUCT_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        FVector Position;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        FVector Normal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        FVector2D UV;

    FSVFVertexNorm()
        : Position(ForceInitToZero)
        , Normal(ForceInitToZero)
        , UV(ForceInitToZero)
    {}

    FSVFVertexNorm(const FSVFVertex& Vertex)
        : Position(Vertex.Position)
        , Normal(ForceInitToZero)
        , UV(Vertex.UV)
    {}

    FSVFVertexNorm(const FVector& InPos, const FVector& InNormal, const FVector2D& InUV)
        : Position(InPos)
        , Normal(InNormal)
        , UV(InUV)
    {}

    operator FSVFVertex() const {
        return FSVFVertex(Position, UV);
    }

};

/**
* SVF file attributes (metadata)
**/
USTRUCT(Blueprintable, BlueprintType)
struct UNREALSVF_API FSVFFileInfo {
    GENERATED_USTRUCT_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        bool hasAudio = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        FTimespan Duration = FTimespan(0L);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        int32 FrameCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        int32 MaxVertexCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        int32 MaxIndexCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        int32 FileWidth = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        int32 FileHeight = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        float BitrateMbps = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        FBox MaxBounds = FBox(ForceInitToZero);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SVF")
        bool hasNormals = false;

    FSVFFileInfo() {};

};


struct UNREALSVF_API FFrameData {

    bool bIsValid;

    FFrameData() : bIsValid(false) {};
    virtual ~FFrameData() {};

#if PLATFORM_ANDROID
    virtual void UnrealRenderEvent_RenderThread();
#endif

    virtual bool GetFrameInfo(FSVFFrameInfo& OutFrameInfo) PURE_VIRTUAL(FFrameData::GetFrameInfo, return false; );

    virtual bool GetFrameVerticesWithNormal(TArray<FSVFVertexNorm>& OutVertices) PURE_VIRTUAL(FFrameData::GetFrameVerticesWithNormal, return false; );
    virtual bool GetFrameVerticesWithoutNormal(TArray<FSVFVertex>& OutVertices) PURE_VIRTUAL(FFrameData::GetFrameVerticesWithoutNormal, return false; );
    virtual bool GetFrameVertices(TArray<FDynamicMeshVertex>& OutVertices) PURE_VIRTUAL(FFrameData::GetFrameVertices, return false; );
    virtual bool GetFrameVertices(FDynamicMeshVertex* OutVertices) PURE_VIRTUAL(FFrameData::GetFrameVertices, return false; );

    virtual bool GetFrameIndices(TArray<int32>& OutIndices) PURE_VIRTUAL(FFrameData::GetFrameIndices, return false; );
    virtual bool GetFrameIndices(int32* OutIndices) PURE_VIRTUAL(FFrameData::GetFrameIndices, return false; );

    virtual bool GetTextureInfo(EPixelFormat& OutPixelFormat, int32& OutWidth, int32& OutHeight) PURE_VIRTUAL(FFrameData::GetTextureInfo, return false; );
    virtual bool GetTextureBuffer(uint8** OutData, int32& OutDataSize) PURE_VIRTUAL(FFrameData::GetTextureBuffer, return false; );
    virtual bool CopyTextureBuffer(UTexture2D* WorkTexture, bool UseHardwareTextureCopy = true) PURE_VIRTUAL(ISVFReaderInterface::CopyTextureBuffer, return false; );
};


/**
 *
 */
UCLASS()
class UNREALSVF_API USVFTypes : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    static FString SVFReaderStateToString(EUSVFReaderState State);


};
