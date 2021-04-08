// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SVFTypes.h"
#include "EngineMinimal.h"
#include "SVFReaderInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USVFReaderInterface : public UInterface
{
    GENERATED_BODY()
};


// Forward declaration
class ISVFClockInterface;

/**
 *
 */
class UNREALSVF_API ISVFReaderInterface
{
    GENERATED_BODY()

        // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

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

#ifdef SUPPORT_HRTF
    /** Sets position of the listener for HRTF (3D audio positioning) */
    virtual void SetAudio3DPosition(const FVector& InLocation) PURE_VIRTUAL(ISVFReaderInterface::SetAudio3DPosition, );
#endif // SUPPORT_HRTF

    /** Returns the bounding box of the entire video clip. */
    //virtual bool GetBBox(FBox& OutBox) PURE_VIRTUAL(ISVFReaderInterface::GetBBox, return false; );

    /**
     * Opens the specified URL (pFilename).
     * @note If the client is opening data from a DASH server this path should point to the URL to the .mpd file
    **/
    virtual bool Open(const FString& InFilePath, const FSVFOpenInfo& InOpenInfo) PURE_VIRTUAL(ISVFReaderInterface::Open, return false; );

    /** Closes playback (client should no longer call GetNextFrame after this call) */
    virtual bool EndPlayback() PURE_VIRTUAL(ISVFReaderInterface::EndPlayback, return false; );

    /** Stops the playback clock and seeks to time 0 */
    virtual bool StopSource() PURE_VIRTUAL(ISVFReaderInterface::StopSource, return false; );

    /** Get the next available frame from the HoloVideo. This method get the frame regardless of the frame's presentation time */
    virtual bool GetNextFrame(bool *pEndOfStream) PURE_VIRTUAL(ISVFReaderInterface::GetNextFrame, return false; );

    virtual void GetFrameInfo(FSVFFrameInfo& OutFrameInfo) PURE_VIRTUAL(ISVFReaderInterface::GetFrameInfo, );

    virtual bool VertexHasNormals() PURE_VIRTUAL(ISVFReaderInterface::VertexHasNormals, return true; );

    virtual bool GetFrameVerticesWithNormal(TArray<FSVFVertexNorm>& OutVertices) PURE_VIRTUAL(ISVFReaderInterface::GetFrameVerticesWithNormal, return false; );

    virtual bool GetFrameVerticesWithoutNormal(TArray<FSVFVertex>& OutVertices) PURE_VIRTUAL(ISVFReaderInterface::GetFrameVerticesWithoutNormal, return false; );

    virtual bool GetFrameIndices(TArray<uint32>& OutIndices) PURE_VIRTUAL(ISVFReaderInterface::GetFrameIndices, return false; );

    virtual bool GetTextureInfo(EPixelFormat& OutPixelFormat, int32& OutWidth, int32& OutHeight) PURE_VIRTUAL(ISVFReaderInterface::GetTextureInfo, return false; );

    virtual bool CopyTextureBuffer(UTexture2D* WorkTexture) PURE_VIRTUAL(ISVFReaderInterface::CopyTextureBuffer, return false; );

    /**
     * Get the next available frame. This method only get a frame if it is time (according to SVF's clock
     * (@see ISVFClockInterface)) to render that frame. If there is no frame ready, this method set bNewFrame to false.
     * If the next frame is late (current time is past its presentation time) the frame will be dropped.
    **/
    virtual bool GetNextFrameViaClock(bool& bNewFrame, bool *pEndOfStream) PURE_VIRTUAL(ISVFReaderInterface::GetNextFrameViaClock, return false; );

    /** Ïóå the next audio packet. Audio data is in PCM format.  */
    virtual bool GetNextAudioPacket(bool *pEndOfStream) PURE_VIRTUAL(ISVFReaderInterface::GetNextAudioPacket, return false; );

    /** Returns the number of streams in the HoloVideo (typically == 4 (index stream, vertex stream, texture stream, and audio stream) */
    virtual bool GetStreamCount(int32& OutCount) PURE_VIRTUAL(ISVFReaderInterface::GetStreamCount, return false; );

    /** Return the stream associated with an index */
    //virtual HRESULT GetStream(ULONG streamIndex, _Outptr_ ISVFStream **ppStream) = 0;

    /**
     * The client may call this method to indicate that it is currently rendering a frame previously acquired
     * whose time stamp was tstamp. This way SVF can determine if it is keeping up with the renderer and if
     * not it may start to degrade quality in attempt to keep up.
    **/
    virtual bool Presenting(int64 tstamp) PURE_VIRTUAL(ISVFReaderInterface::Presenting, return false; );

    /** Sets the looping behavior for when playback reaches the end of the stream (@see ESVFLoop for details) */
    virtual void SetLooping(EUSVFLoop InLoop) PURE_VIRTUAL(ISVFReaderInterface::SetLooping, );

    /** Returns the current loop behavior */
    virtual EUSVFLoop GetLooping() PURE_VIRTUAL(ISVFReaderInterface::GetLooping, return EUSVFLoop::NoLooping; );

    /** Sets the current playback's audio volume level */
    virtual void SetAudioVolumeLevel(float InVolume) PURE_VIRTUAL(ISVFReaderInterface::SetAudioVolumeLevel, );

    /** Gets the current playback's audio volume level */
    virtual float GetAudioVolumeLevel() PURE_VIRTUAL(ISVFReaderInterface::GetAudioVolumeLevel, return 0.f; );

    /**  */
    virtual int32 GetNumberDroppedFrames() PURE_VIRTUAL(ISVFReaderInterface::GetNumberDroppedFrames, return 0; );

    /** Starts the clock (causing frames to be returned... @see GetNextFrameViaClock) */
    virtual void StartClock() PURE_VIRTUAL(ISVFReaderInterface::StartClock, );

    /** Stops the clock */
    virtual void StopClock() PURE_VIRTUAL(ISVFReaderInterface::StopClock, );

    /** Gets the total duration of the HoloVideo */
    virtual bool GetDuration(int64& OutDuration) PURE_VIRTUAL(ISVFReaderInterface::GetDuration, return false; );

    /** Sets the clocks scaling factor. This can be used to speed up or slowdown playback (within a small range) */
    virtual void SetClockScale(float ClockScale) PURE_VIRTUAL(ISVFReaderInterface::SetClockScale, );

    /** Returns the current clock scale */
    virtual float GetClockScale() PURE_VIRTUAL(ISVFReaderInterface::GetClockScale, return 1.f; );

    /** Shuts down the SVF object along with all its worker threads */
    virtual bool Shutdown() PURE_VIRTUAL(ISVFReaderInterface::Shutdown, return false; );

    /**
    * Registers a client callback for being notified of internal fatal errors causing abrupt closing
    * Registers a client callback for being notified of state changes in SVF (@see ESVFReaderState)
    * Reciever must implement USVFCallbackInterface!
    **/
    virtual void SetNoifyReciever(UObject* Reciever) PURE_VIRTUAL(ISVFReaderInterface::SetNoifyReciever, );


    virtual bool SeekToFrame(uint32 frameId) PURE_VIRTUAL(ISVFReaderInterface::SeekToFrame, return false; );
    virtual bool GetSeekRange(FInt32Range& OutFrameRange) PURE_VIRTUAL(ISVFReaderInterface::GetSeekRange, return false; );
    virtual bool ForceFlush() PURE_VIRTUAL(ISVFReaderInterface::ForceFlush, return false; );
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
    virtual void SuspendReadingThread() PURE_VIRTUAL(ISVFReaderInterface::SuspendReadingThread, );
    virtual void ResumeReadingThread() PURE_VIRTUAL(ISVFReaderInterface::ResumeReadingThread, );

};
