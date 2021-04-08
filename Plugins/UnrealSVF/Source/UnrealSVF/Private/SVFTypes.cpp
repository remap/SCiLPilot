// Fill out your copyright notice in the Description page of Project Settings.

#include "SVFTypes.h"

#include "UnrealSVF.h"
#include "PreSVFAPI.h"
#if PLATFORM_WINDOWS
#include "SVF.h"
#include "PostSVFAPI.h"
#endif

FHrtfAudioDecaySettings::FHrtfAudioDecaySettings()
    : MinGain(0.f)
    , MaxGain(0.f)
    , GainDistance(0.f)
    , CutoffDistance(0.f)
{
}

FSVFOpenInfo::FSVFOpenInfo()
    : AudioDisabled(false)
    , RenderViaClock(true)
    , OutputNormals(true)
    , StartDownloadOnOpen(true)
    , AutoLooping(false)
    , forceSoftwareClock(true)
    , playbackRate(1.f)
{
}

FSVFOpenInfo::FSVFOpenInfo(EUSVFOpenPreset PresetMode)
    : AudioDisabled(true)
    , RenderViaClock(true)
    , OutputNormals(true)
    , StartDownloadOnOpen(false)
    , AutoLooping(false)
    , forceSoftwareClock(true)
    , playbackRate(1.f)
{
    switch (PresetMode) {
    case EUSVFOpenPreset::FirstMovie:
        StartDownloadOnOpen = true;
        break;
    case EUSVFOpenPreset::SmallMemoryBuffer_DropIfNeed:
        break;
    case EUSVFOpenPreset::BigMemoryBuffer_DropIfNeed:
        break;
    default:
    case EUSVFOpenPreset::DefaultPreset_DropIfNeed:
        break;
    case EUSVFOpenPreset::DefaultPreset_NoDropping:
        //StartDownloadOnOpen = true;
        break;
    case EUSVFOpenPreset::HardCore_NoHW:
        break;
    case EUSVFOpenPreset::NoCache_FirstMovie:
        //StartDownloadOnOpen = true;
        // Experimental - holograms randomly being too bright bug fix
        break;
    case EUSVFOpenPreset::NoCache_Default:
        // TODO: Was not originally suppposed to be here. 
        // Alternative way way to do this is to switch presets to No_CacheFirstMovie for all instances.
        //StartDownloadOnOpen = true;
        // Experimental - holograms randomly being too bright bug fix
        break;
    case EUSVFOpenPreset::NoCache_Variant2:
        break;
    }
}

#if PLATFORM_WINDOWS
FSVFConfiguration::FSVFConfiguration()
    : useHardwareDecode(true)
    , useHardwareTextures(false)
    , outputNV12(true)
    , bufferCompressedData(true)
    , useKeyedMutex(true)
    , disableAudio(false)
    , playAudio(true)
    , returnAudio(false)
    , prerollSize(c_PrerollSize)
    , minBufferSize(c_MinBufferSize)
    , maxBufferSize(c_MaxBufferSize)
    , maxBufferUpperBound(c_MaxBufferUpperBound)
    , bufferHysteresis(c_BufferHysteresis)
    , minCompressedBufferSize(c_MinCompressedBufferSize)
    , maxCompressedBufferSize(c_MaxCompressedBufferSize)
    , allowableDelay(150)
    , allowDroppedFrames(true)
    , maxDropFramesPerCall(10)
    , presentationLatency(0)
    , looping(EUSVFLoop::LoopViaRestart)
    , startDownloadWhenOpen(false)
    , clockScale(1.0f)
    , outputVertexFormat(ESVFOutputVertexFormat::Position | ESVFOutputVertexFormat::UV)
{
}
#else
FSVFConfiguration::FSVFConfiguration() {}
#endif


FString USVFTypes::SVFReaderStateToString(EUSVFReaderState State) {
    switch (State) {
    case EUSVFReaderState::Initialized:
        return TEXT("Initialized");
        break;
    case EUSVFReaderState::OpenPending:
        return TEXT("OpenPending");
        break;
    case EUSVFReaderState::Opened:
        return TEXT("Opened");
        break;
    case EUSVFReaderState::Prerolling:
        return TEXT("Prerolling");
        break;
    case EUSVFReaderState::Ready:
        return TEXT("Ready");
        break;
    case EUSVFReaderState::Buffering:
        return TEXT("Buffering");
        break;
    case EUSVFReaderState::Closing:
        return TEXT("Closing");
        break;
    case EUSVFReaderState::Closed:
        return TEXT("Closed");
        break;
    case EUSVFReaderState::EndOfStream:
        return TEXT("EndOfStream");
        break;
    case EUSVFReaderState::ShuttingDown:
        return TEXT("ShuttingDown");
        break;
    case EUSVFReaderState::Unknown:
    default:
        return TEXT("Unknown");
        break;
    }
}
