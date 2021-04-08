// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SVFSimpleInterface.h"
#include "SVFCallbackInterface.h"

#if PLATFORM_WINDOWS
#include "PreSVFAPI.h"
#include "SVF.h"
#include "SVFCritSec.h"
#include "PostSVFAPI.h"

#include "SVFTypes.h"
#include "SVFPrivateTypes.h"
#endif
#include "SVFReaderPassThrough.generated.h"

UCLASS()
class UNREALSVF_API USVFReaderPassThrough : public UObject, public ISVFSimpleInterface, public ISVFCallbackInterface {
    GENERATED_BODY()

public:

    USVFReaderPassThrough();
    ~USVFReaderPassThrough();
#if PLATFORM_WINDOWS

    static bool CreateInstance(UObject* InOwner, const FString& FilePath, const FSVFOpenInfo& OpenInfo, UObject** OutWrapper, UObject* CustomClockObject);

    // ~ ISVFReader interface
    virtual bool GetClock(ISVFClockInterface*& OutClock) override;

    virtual bool StartSource(int64 timeInStreamUnits) override;

    virtual bool BeginPlayback() override;

    virtual bool GetNextFrame(bool *pEndOfStream) override;

    virtual bool GetNextFrameViaClock(bool& bNewFrame, bool *pEndOfStream) override;

    virtual void GetFrameInfo(FSVFFrameInfo& OutFrameInfo) override;

    virtual bool VertexHasNormals() override;

    virtual bool GetFrameData(TSharedPtr<FFrameData>& OutFrameDataPtr) override;

    virtual void Sleep() override;

    virtual void Wakeup() override;

    virtual void Close() override;
    virtual void Close_BackgroundThread() override;

    virtual FSVFFileInfo GetFileInfo() override {
        return FileInfo;
    }

#ifdef SUPPORT_HRTF
    virtual void SetAudio3DPosition(const FVector& InLocation) override;
#endif // SUPPORT_HRTF

    virtual bool SeekToFrame(uint32 frameId) override;
    virtual bool SeekToTime(const FTimespan& InTime) override;
    virtual bool SeekToPercent(float SeekToPercent) override;
    virtual void SetReaderClockScale(float ClockScale) override;
    virtual bool GetSeekRange(FInt32Range& OutFrameRange) override { return false; }
    virtual bool ForceFlush() override { return true; }
    virtual bool CanSeek() override;
    virtual bool GetInternalStateFlags(uint32& OutFlags) override;
    virtual const FSVFConfiguration* GetSVFConfig() override { return &m_svfConfig; };
    virtual bool GetSVFStatus(FSVFStatus& OutStatus) override;
    virtual uint32 SVFBufferedFramesCount() override;
    virtual void SetAudioVolume(float volume) override;
    virtual float GetAudioVolume() override;

    virtual void Start() override;
    virtual void Stop() override;
    virtual void Rewind() override;
    virtual void SetPlayFlow(bool bIsForwardPlay) override {};
    virtual void SuspendReadingThread() override {};
    virtual void ResumeReadingThread() override {};

    // ~ ISVFReaderStateCallback interface
    virtual bool OnStateChange(EUSVFReaderState oldState, EUSVFReaderState newState) override;

    // ~ ISVFReaderInternalStateCallback interface
    virtual void OnError(const FString& ErrorString, int32 hrError) override;
    virtual void OnEndOfStream(uint32 remainingVideoSampleCount) override;

    virtual void UpdateSVFStatistics(uint32 unsuccessfulReadFrameCount, uint32 lastReadFrame, uint32 droppedFrameCount) override {};

protected:

    HRESULT CreateReader(const FString& FilePath, const FSVFOpenInfo& OpenInfo, UObject* CustomClockObject);

    ComPtr<ISVFReader> m_spReader;
    TWeakObjectPtr<UObject> m_ClockObject;
    FSVFStatus m_svfStatus;
    SVFCriticalSection m_statusCS;
    UINT32 m_svfBufferedFramesCount; // initialized when SVF hits reading thread EOS. 

    // Read result
    ComPtr<ISVFFrame> m_Frame; // Hold Frame for rendering
    SVFFrameInfo m_FrameInfo;
    bool bUseNormal = true;
    bool bLoop = false;
    FSVFConfiguration m_svfConfig;
    FSVFFileInfo FileInfo;

    std::shared_ptr<std::queue<HostageFrameD3D11>> m_hostageFrames;

#endif
};