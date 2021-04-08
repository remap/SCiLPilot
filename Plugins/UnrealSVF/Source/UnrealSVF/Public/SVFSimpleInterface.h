// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SVFTypes.h"
#include "EngineMinimal.h"
#include "SVFSimpleInterface.generated.h"


// Forward declaration
class ISVFClockInterface;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USVFSimpleInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 *
 */
class UNREALSVF_API ISVFSimpleInterface
{
    GENERATED_BODY()

        // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    static bool CreateInstance(UObject* InOwner, const FString& FilePath, const FSVFOpenInfo& OpenInfo, UObject** OutWrapper, UObject* CustomClockObject);

    /** Returns the current clock being used to drive playback */
    virtual bool GetClock(ISVFClockInterface*& OutClock) PURE_VIRTUAL(ISVFReaderInterface::GetClock, return false; );

    /** Performs a seek on the HoloVideo file to the specified time */
    virtual bool StartSource(int64 timeInStreamUnits) PURE_VIRTUAL(ISVFReaderInterface::StartSource, return false; );

    /** BeginPlayback should be called once the client is ready to start reading frames */
    virtual bool BeginPlayback() PURE_VIRTUAL(ISVFReaderInterface::BeginPlayback, return false; );

    /** Causes SVF's clock to stop and for all worker threads to stop processing */
    virtual void Sleep() PURE_VIRTUAL(ISVFReaderInterface::Sleep, );

    /**  */
    virtual void Wakeup() PURE_VIRTUAL(ISVFReaderInterface::Wakeup, );

    /** Closes the current file */
    virtual void Close() PURE_VIRTUAL(ISVFReaderInterface::Close, );
    virtual void Close_BackgroundThread() PURE_VIRTUAL(ISVFReaderInterface::Close_BackgroundThread, );

    virtual FSVFFileInfo GetFileInfo() PURE_VIRTUAL(ISVFReaderInterface::GetFileInfo, return FSVFFileInfo(););

#ifdef SUPPORT_HRTF
    /** Sets position of the listener for HRTF (3D audio positioning) */
    virtual void SetAudio3DPosition(const FVector& InLocation) PURE_VIRTUAL(ISVFReaderInterface::SetAudio3DPosition, );
#endif // SUPPORT_HRTF

    /**
    * Get the next available frame. This method only get a frame if it is time (according to SVF's clock
    * (@see ISVFClockInterface)) to render that frame. If there is no frame ready, this method set bNewFrame to false.
    * If the next frame is late (current time is past its presentation time) the frame will be dropped.
    **/
    virtual bool GetNextFrameViaClock(bool& bNewFrame, bool *pEndOfStream) PURE_VIRTUAL(ISVFReaderInterface::GetNextFrameViaClock, return false; );

    virtual bool GetNextFrame(bool *pEndOfStream) PURE_VIRTUAL(ISVFReaderInterface::GetNextFrame, return false; );

    virtual void GetFrameInfo(FSVFFrameInfo& OutFrameInfo) PURE_VIRTUAL(ISVFReaderInterface::GetFrameInfo, );

    virtual bool VertexHasNormals() PURE_VIRTUAL(ISVFReaderInterface::VertexHasNormals, return true; );

    virtual bool GetFrameData(TSharedPtr<FFrameData>& OutFrameDataPtr) PURE_VIRTUAL(ISVFReaderInterface::GetFrameData, return false; );

    virtual bool CanSeek() PURE_VIRTUAL(ISVFReaderInterface::CanSeek, return false; );

    virtual bool GetInternalStateFlags(uint32& OutFlags) PURE_VIRTUAL(ISVFReaderInterface::GetInternalStateFlags, return false; );

#if PLATFORM_WINDOWS
    virtual const FSVFConfiguration* GetSVFConfig() PURE_VIRTUAL(ISVFReaderInterface::GetSVFConfig, return nullptr; );
#endif

    virtual bool GetSVFStatus(FSVFStatus& OutStatus) PURE_VIRTUAL(ISVFReaderInterface::GetSVFStatus, return false; );

    virtual uint32 SVFBufferedFramesCount() PURE_VIRTUAL(ISVFReaderInterface::SVFBufferedFramesCount, return 0; );

    virtual void Start() PURE_VIRTUAL(ISVFReaderInterface::Start, );

    virtual void Stop() PURE_VIRTUAL(ISVFReaderInterface::Stop, );

    virtual void Rewind() PURE_VIRTUAL(ISVFReaderInterface::Rewind, );

    virtual void SetPlayFlow(bool bIsForwardPlay) PURE_VIRTUAL(ISVFReaderInterface::SetPlayFlow, );

    virtual void SetAudioVolume(float volume) PURE_VIRTUAL(ISVFReaderInterface::SetAudioVolume, );

    virtual float GetAudioVolume() PURE_VIRTUAL(ISVFReaderInterface::GetAudioVolumeLevel, return -1.0f; );

    virtual bool SeekToFrame(uint32 frameId) PURE_VIRTUAL(ISVFReaderInterface::SeekToFrame, return false; );

    virtual bool SeekToTime(const FTimespan& InTime) PURE_VIRTUAL(ISVFReaderInterface::SeekToTime, return false; );

    virtual bool SeekToPercent(float SeekToPercent) PURE_VIRTUAL(ISVFReaderInterface::SeekToPercent, return false; );

    virtual void SetReaderClockScale(float InScale) PURE_VIRTUAL(ISVFReaderInterface::SetReaderClockScale, );

    virtual bool GetSeekRange(FInt32Range& OutFrameRange) PURE_VIRTUAL(ISVFReaderInterface::GetSeekRange, return false; );

    virtual bool ForceFlush() PURE_VIRTUAL(ISVFReaderInterface::ForceFlush, return false; );

    virtual void SuspendReadingThread() PURE_VIRTUAL(ISVFReaderInterface::SuspendReadingThread, );

    virtual void ResumeReadingThread() PURE_VIRTUAL(ISVFReaderInterface::ResumeReadingThread, );


};
