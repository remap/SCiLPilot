// Copyright (C) Microsoft Corporation. All rights reserved.

#include "SVFReaderPassThrough.h"

#if PLATFORM_WINDOWS

#include "UnrealSVF.h"
#include "DynamicMeshBuilder.h"
#if SVF_USED3D11
// Include D3D11 RHI
#include "D3D11State.h"
#include "D3D11Resources.h"
#endif
#include "Stats/Stats.h"

DEFINE_LOG_CATEGORY_STATIC(LogSVFReaderLive, Log, All);

#define LogSVF(pmt, ...) UE_LOG(LogSVFReaderLive, Log, TEXT(pmt), ##__VA_ARGS__)
#define WarnSVF(pmt, ...) UE_LOG(LogSVFReaderLive, Warning, TEXT(pmt), ##__VA_ARGS__)
#define FatalSVF(pmt, ...) UE_LOG(LogSVFReaderLive, Fatal, TEXT(pmt), ##__VA_ARGS__)

/*
DECLARE_CYCLE_STAT(TEXT("Get Next Frame"), STAT_SVF_GetNextFrame, STATGROUP_UnrealSVF);
DECLARE_CYCLE_STAT(TEXT("Copy Vertices buffer"), STAT_SVF_CopyVerticeBuffer, STATGROUP_UnrealSVF);
DECLARE_CYCLE_STAT(TEXT("Copy Indices buffer"), STAT_SVF_CopyIndicesBuffer, STATGROUP_UnrealSVF);
DECLARE_CYCLE_STAT(TEXT("Copy Texture buffer"), STAT_SVF_CopyTextureBuffer, STATGROUP_UnrealSVF);*/


bool USVFReaderPassThrough::CreateInstance(UObject* InOwner, const FString& FilePath,
        const FSVFOpenInfo& OpenInfo, UObject** OutWrapper, UObject* CustomClockObject)
{
    if (FilePath.IsEmpty())
    {
        WarnSVF("Error in CSVFReaderPassThrough::CreateInstance: FilePath is empty");
        return false;
    }
    if (!OutWrapper)
    {
        WarnSVF("Error in CSVFReaderPassThrough::CreateInstance: OutWrapper is null");
        return false;
    }

    USVFReaderPassThrough* pWrapperObj = NewObject<USVFReaderPassThrough>(InOwner);
    if (!pWrapperObj)
    {
        FatalSVF("Error in CSVFReaderPassThrough::CreateInstance: memory allocation error when creating CSVFReaderPassThrough object");
        return false;
    }

    HRESULT hr = S_OK;
    hr = pWrapperObj->CreateReader(FilePath, OpenInfo, CustomClockObject);
    if (FAILED(hr))
    {
        //delete pWrapperObj;
        pWrapperObj->MarkPendingKill();
        return false;
    }
    *OutWrapper = pWrapperObj;

    return true;
}

HRESULT USVFReaderPassThrough::CreateReader(const FString& FilePath, const FSVFOpenInfo& OpenInfo, UObject* CustomClockObject)
{
    HRESULT hr = S_OK;
    ComPtr<ISVFBufferAllocator> spBufferAllocator;
    SVFReaderConfiguration readerConfig;
    readerConfig.performCacheCleanup = false;

    hr = SVFCreateBufferAllocator(&spBufferAllocator);
    if (FAILED(hr))
    {
        FatalSVF("Error in CSVFReaderPassThrough::CreateReader: failed in SVFCreateBufferAllocator, hr = 0x%08X", hr);
        return hr;
    }

    ComPtr<ISVFClock> spCustomClock;
    if (OpenInfo.forceSoftwareClock)
    {
        if (!CustomClockObject)
        {
            return E_POINTER;
        }
        // using raw pointer because of a potentially dangerous mix of COM ptr and std::unique_ptr
        ComPtr<USVFClockUseUnrealInterface> spPluginClock = new USVFClockUseUnrealInterface();
        if (!spPluginClock)
        {
            return E_OUTOFMEMORY;
        }
        hr = spPluginClock->Initialize(CustomClockObject);
        if (SUCCEEDED(hr))
        {
            spCustomClock = spPluginClock;
            m_ClockObject = CustomClockObject;
        }
        spPluginClock = nullptr;
    }

    ComPtr<ISVFReader> spReader;
    hr = SVFCreateReaderWithConfig(spBufferAllocator.Get(), spCustomClock.Get(), &readerConfig, &spReader);
    if (FAILED(hr))
    {
        FatalSVF("Error in CSVFReaderPassThrough::CreateReader: failed in SVFCreateReaderWithConfig, hr = 0x%08X", hr);
        return hr;
    }

    {
        ComPtr<USVFReaderCallbackUseUnrealInterface> spNotifierWrapper = new USVFReaderCallbackUseUnrealInterface();
        if (!spNotifierWrapper)
        {
            return E_OUTOFMEMORY;
        }
        hr = spNotifierWrapper->Initialize(this);
        if (SUCCEEDED(hr))
        {
            spReader->SetNotifyState(spNotifierWrapper.Get());
            spReader->SetNoifyInternalState(spNotifierWrapper.Get());
        }
        spNotifierWrapper = nullptr;
    }

    SVFConfiguration svfConfig;
    svfConfig.disableAudio = OpenInfo.AudioDisabled;
    svfConfig.returnAudio = false;
    svfConfig.playAudio = !(OpenInfo.AudioDisabled);
    svfConfig.outputNV12 = false; // we will use RGBA format
    svfConfig.useHardwareDecode = true;
    svfConfig.useKeyedMutex = OpenInfo.UseKeyedMutex;
    svfConfig.useHardwareTextures = true;
    svfConfig.outputVertexFormat = ESVFOutputVertexFormat::Position | ESVFOutputVertexFormat::UV | ESVFOutputVertexFormat::Uncompressed;
    svfConfig.looping = OpenInfo.AutoLooping ? ESVFLoop::LoopViaRestart : ESVFLoop::NoLooping;
    bLoop = OpenInfo.AutoLooping;
    if (svfConfig.useHardwareDecode && FString(TEXT("D3D11")).Equals(GDynamicRHI->GetName()))
    {
        svfConfig.spDevice = nullptr;
    }
    if (svfConfig.useHardwareTextures == false)
    {
        // for software mode, re-adjust internal SVF buffers to prevent E_OUTOFMEMORY
        svfConfig.minBufferSize = 15;
        svfConfig.maxBufferSize = 50;
        svfConfig.bufferHysteresis = 17;
        svfConfig.maxBufferUpperBound = 100;
        svfConfig.maxDropFramesPerCall = 3;
        svfConfig.allowableDelay = 500;
    }
    svfConfig.looping = ESVFLoop::NoLooping; // plugin takes care of looping
    if (OpenInfo.OutputNormals)
    {
        svfConfig.outputVertexFormat |= ESVFOutputVertexFormat::Normal;
    }
    bUseNormal = OpenInfo.OutputNormals;
    svfConfig.clockScale = OpenInfo.playbackRate;

#ifdef SUPPORT_HRTF
    // Initialize HRTF audio settings
    spReader->SetHrtfAudioDecaySettings(OpenInfo.hrtf.MinGain, OpenInfo.hrtf.MaxGain, OpenInfo.hrtf.GainDistance, OpenInfo.hrtf.CutoffDistance);
#endif // SUPPORT_HRTF
#if UE_EDITOR
    try
    {
        hr = spReader->Open(*FilePath, &svfConfig);
        if (FAILED(hr))
        {
            FatalSVF("Error in CSVFReaderPassThrough::CreateReader: failed in ISVFReader::Open, hr = 0x%08X", hr);
            return hr;
        }
    }
    catch (...)
    {
        FatalSVF("Unhandled exception in CSVFReaderPassThrough::CreateReader");
        //PLUGIN_LOG_EXCEPTION(ex);
        return E_UNEXPECTED;
    }
#else
    hr = spReader->Open(*FilePath, &svfConfig);
    if (FAILED(hr))
    {
        FatalSVF("Error in CSVFReaderPassThrough::CreateReader: failed in ISVFReader::Open, hr = 0x%08X", hr);
        return hr;
    }
#endif

    if (!OpenInfo.AudioDeviceId.IsEmpty())
    {
        ComPtr<ISVFAudioDevice> pAudioDevice;
        hr = spReader->QueryInterface(__uuidof(ISVFAudioDevice), (void**)(&pAudioDevice));
        if (SUCCEEDED(hr))
        {
            pAudioDevice->SetAudioDevice(*OpenInfo.AudioDeviceId);
        }
        else
        {
            FatalSVF("Error in CHCapObject::SetAudioDevice: cannot get ISVFAudioDevice from ISVFReader");
        }
    }

    if (!SVFHelpers::ObtainFileInfoFromSVF(FilePath, spReader, FileInfo))
    {
        // Suppressed crash
        //FatalSVF("Error in CSVFReaderPassThrough::CreateReader: failed read FileInfo!");
        return E_UNEXPECTED;
    }

    SVFHelpers::CopyConfig(svfConfig, m_svfConfig);
    m_spReader = nullptr;
    m_spReader = spReader;
    return S_OK;
}

/*USVFReaderPassThrough::USVFReaderPassThrough(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , m_svfBufferedFramesCount(0L)
{*/
USVFReaderPassThrough::USVFReaderPassThrough()
    : m_svfBufferedFramesCount(0L)
{
    ZeroMemory(&m_svfConfig, sizeof(m_svfConfig));
    ZeroMemory(&m_svfStatus, sizeof(m_svfStatus));
    m_svfStatus.isLiveSVFSource = true;
    m_svfStatus.lastKnownState = static_cast<int>(ESVFReaderState::Unknown);

#if SVF_USED3D11
    m_hostageFrames = std::make_shared<std::queue<HostageFrameD3D11>>();
#endif
}

USVFReaderPassThrough::~USVFReaderPassThrough()
{
    m_spReader = nullptr;
}

bool USVFReaderPassThrough::GetClock(ISVFClockInterface*& OutClock)
{
    checkSlow(m_spReader);
    if (!m_spReader)
    {
        return false;
    }

    if (!m_ClockObject.IsValid())
    {
        return false;
    }
    // Temporarily commented out due to error C2065: 'StatPtr_STAT_SVF_GetNextFrame': undeclared identifier
    //OutClock = Cast<ISVFClockInterface>(m_ClockObject.Get());

    return OutClock != nullptr;
}

bool USVFReaderPassThrough::BeginPlayback()
{
    checkSlow(m_spReader);
    if (!m_spReader)
    {
        return false;
    }

    return SUCCEEDED(m_spReader->BeginPlayback());
}

bool USVFReaderPassThrough::StartSource(int64 timeInStreamUnits)
{
    checkSlow(m_spReader);
    if (!m_spReader)
    {
        return false;
    }

    {
        SVFAutoLock lock(m_statusCS);
        m_svfBufferedFramesCount = 0L;
    }

    return SUCCEEDED(m_spReader->StartSource(timeInStreamUnits));
}

bool USVFReaderPassThrough::GetNextFrame(bool *pEndOfStream)
{
    checkSlow(m_spReader);
    if (!m_spReader)
    {
        return false;
    }

    HRESULT hr = S_OK;
    ISVFFrame* ppFrame = nullptr;
    {
        // error C2065: 'StatPtr_STAT_SVF_GetNextFrame': undeclared identifier
        //SCOPE_CYCLE_COUNTER(STAT_SVF_GetNextFrame);
        hr = m_spReader->GetNextFrame(&ppFrame, pEndOfStream);
        m_FrameInfo.isRepeatedFrame = true;
    }

    {
        SVFAutoLock lock(m_statusCS);
        if (SUCCEEDED(hr) && ppFrame == nullptr)
        {
            m_svfStatus.unsuccessfulReadFrameCount++;
        }
        if (SUCCEEDED(hr) && (ppFrame))
        {
            ComPtr<ISVFFrame> spFrame = ppFrame;
            ComPtr<ISVFBuffer> spTB;
            ComPtr<ISVFBuffer> spVB;
            ComPtr<ISVFBuffer> spIB;
            ComPtr<ISVFBuffer> spAB;
            // Check all buffers
            hr = SVFFrameHelper::ExtractBuffers(spFrame, spTB, spVB, spIB, spAB);
            if (SUCCEEDED(hr) && spVB)
            {
                ZeroMemory(&m_FrameInfo, sizeof(m_FrameInfo));
                hr = SVFFrameHelper::ExtractFrameInfo(spVB, false, m_FrameInfo);
            }
            else
            {
                hr = E_UNEXPECTED;
            }
            if (SUCCEEDED(hr))
            {
                if (m_Frame)
                {
                    m_Frame->Release();
                    m_Frame = nullptr;
                }
                m_Frame = spFrame;
                m_svfStatus.unsuccessfulReadFrameCount = 0;
                if (m_svfBufferedFramesCount > 0)
                {
                    m_svfBufferedFramesCount--;
                }
            }
        }
    }

    return SUCCEEDED(hr) && ppFrame != nullptr;
}

bool USVFReaderPassThrough::GetNextFrameViaClock(bool& bNewFrame, bool *pEndOfStream)
{
    checkSlow(m_spReader);
    if (!m_spReader || !pEndOfStream)
    {
        return false;
    }

    ISVFFrame* ppFrame = nullptr;
    HRESULT hr = m_spReader->GetNextFrameViaClock(&ppFrame, pEndOfStream);

    {
        SVFAutoLock lock(m_statusCS);
        if (SUCCEEDED(hr) && ppFrame == nullptr)
        {
            m_svfStatus.unsuccessfulReadFrameCount++;
            m_FrameInfo.isRepeatedFrame = true;
        }
        if (SUCCEEDED(hr) && (ppFrame))
        {
            int droppedCount = m_spReader->GetNumberDroppedFrames();
            ComPtr<ISVFFrame> spFrame = ppFrame;
            ComPtr<ISVFBuffer> spTB;
            ComPtr<ISVFBuffer> spVB;
            ComPtr<ISVFBuffer> spIB;
            ComPtr<ISVFBuffer> spAB;
            hr = SVFFrameHelper::ExtractBuffers(spFrame, spTB, spVB, spIB, spAB);
            if (SUCCEEDED(hr) && spVB)
            {
                ZeroMemory(&m_FrameInfo, sizeof(m_FrameInfo));
                hr = SVFFrameHelper::ExtractFrameInfo(spVB, *pEndOfStream, m_FrameInfo);
            }
            else
            {
                hr = E_UNEXPECTED;
            }
            if (SUCCEEDED(hr))
            {
                if (m_Frame)
                {
                    m_Frame->Release();
                    m_Frame = nullptr;
                }
                m_Frame = spFrame;
                m_svfStatus.droppedFrameCount = droppedCount;
                m_svfStatus.lastReadFrame = m_FrameInfo.frameId;
                m_svfStatus.unsuccessfulReadFrameCount = 0;
                if (m_svfBufferedFramesCount > 0)
                {
                    m_svfBufferedFramesCount--;
                }

            }
        }
        if ((*pEndOfStream) && bLoop)
        {
            StartSource(0);
        }
    }

    bNewFrame = SUCCEEDED(hr) && hr != S_FALSE && ppFrame;

    return SUCCEEDED(hr);
}

void USVFReaderPassThrough::GetFrameInfo(FSVFFrameInfo& OutFrameInfo)
{
    SVFHelpers::CopyFrameInfo(m_FrameInfo, OutFrameInfo);
}

bool USVFReaderPassThrough::VertexHasNormals()
{
    return bUseNormal;
}

bool USVFReaderPassThrough::GetFrameData(TSharedPtr<FFrameData>& OutFrameDataPtr)
{
    checkSlow(m_spReader);
    if (!m_spReader || !m_Frame)
    {
        return false;
    }

    OutFrameDataPtr = MakeShareable(new FFrameDataFromSVFBuffer(m_Frame, m_hostageFrames, m_FrameInfo, bUseNormal));
    return OutFrameDataPtr->bIsValid;
}

void USVFReaderPassThrough::Sleep()
{
    checkSlow(m_spReader);
    if (!m_spReader)
    {
        return;
    }

    m_spReader->Sleep();
}

void USVFReaderPassThrough::Wakeup()
{
    checkSlow(m_spReader);
    if (!m_spReader)
    {
        return;
    }

    m_spReader->Wakeup();
}

void USVFReaderPassThrough::Close()
{
    if (m_Frame)
    {
        m_Frame->Release();
        m_Frame = nullptr;
    }
    if (m_spReader)
    {
        m_spReader->SetNotifyState(nullptr);
        m_spReader->SetNoifyInternalState(nullptr);
        m_spReader->Close();
        m_spReader = nullptr;
    }
}

void USVFReaderPassThrough::Close_BackgroundThread()
{
    if (m_Frame)
    {
        m_Frame->Release();
        m_Frame = nullptr;
    }
    if (m_spReader)
    {
        m_spReader->SetNotifyState(nullptr);
        m_spReader->SetNoifyInternalState(nullptr);
        AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this]()
        {
            if (m_spReader)
            {
                m_spReader->Close();
                m_spReader = nullptr;
            }
        });
    }
}

#ifdef SUPPORT_HRTF
void USVFReaderPassThrough::SetAudio3DPosition(const FVector& InLocation)
{
    checkSlow(m_spReader);
    if (!m_spReader)
    {
        return;
    }

    m_spReader->SetAudio3DPosition(InLocation.X, InLocation.Y, InLocation.Z);
}
#endif

bool USVFReaderPassThrough::GetSVFStatus(FSVFStatus& OutStatus)
{
    checkSlow(m_spReader);
    if (!m_spReader)
    {
        return false;
    }

    SVFAutoLock lock(m_statusCS);
    FMemory::Memcpy(OutStatus, m_svfStatus);

    // if we passed the error, we delegated error handling to the caller, clear error code here
    if (FAILED(m_svfStatus.errorHresult))
    {
        m_svfStatus.errorHresult = S_OK;
    }

    return true;
}

uint32 USVFReaderPassThrough::SVFBufferedFramesCount()
{
    SVFAutoLock lock(m_statusCS);
    return m_svfBufferedFramesCount;
}

// --------------------------------------------------------------------------
// ISVFReaderStateCallback
bool USVFReaderPassThrough::OnStateChange(EUSVFReaderState, EUSVFReaderState newState)
{
    SVFAutoLock lock(m_statusCS);
    m_svfStatus.lastKnownState = static_cast<int>(newState);
    return S_OK;
}

// --------------------------------------------------------------------------
// ISVFReaderInternalStateCallback
void USVFReaderPassThrough::OnError(const FString& ErrorString, int32 hrError)
{
    SVFAutoLock lock(m_statusCS);
    UE_LOG(LogTemp, Error, TEXT("SVFReader Error: %s"), *ErrorString);
    m_svfStatus.errorHresult = FAILED(hrError) ? hrError : E_UNEXPECTED;
}

void USVFReaderPassThrough::OnEndOfStream(uint32 remainingVideoSampleCount)
{
    SVFAutoLock lock(m_statusCS);
    m_svfBufferedFramesCount = remainingVideoSampleCount;
}

bool USVFReaderPassThrough::GetInternalStateFlags(uint32& OutFlags)
{
    OutFlags = static_cast<uint32>(EInternalStateFlags::Live);
    return true;
}

void USVFReaderPassThrough::Start()
{
    checkSlow(m_spReader);
    if (!m_spReader)
    {
        return;
    }

    m_spReader->StartClock();
}

void USVFReaderPassThrough::Stop()
{
    checkSlow(m_spReader);
    if (!m_spReader)
    {
        return;
    }

    m_spReader->StopClock();
}

void USVFReaderPassThrough::Rewind()
{
    if (m_spReader)
    {
        m_spReader->StartSource(0);
        m_FrameInfo.isEOS = false;
    }
}

bool USVFReaderPassThrough::CanSeek()
{
    return false;
}

bool USVFReaderPassThrough::SeekToFrame(uint32 frameId)
{
    checkSlow(m_spReader);
    if (!m_spReader || FileInfo.FrameCount < static_cast<int32>(frameId))
    {
        return false;
    }

    FTimespan SeekToTime = FileInfo.Duration * (static_cast<double>(frameId) / static_cast<double>(FileInfo.FrameCount));
    return SUCCEEDED(m_spReader->StartSource(static_cast<uint64>(SeekToTime.GetTicks())));
}

bool USVFReaderPassThrough::SeekToTime(const FTimespan& InTime)
{
    checkSlow(m_spReader);
    if (!m_spReader)
    {
        return false;
    }

    return SUCCEEDED(m_spReader->StartSource(static_cast<uint64>(InTime.GetTicks())));
}

bool USVFReaderPassThrough::SeekToPercent(float SeekToPercent)
{
    checkSlow(m_spReader);
    if (!m_spReader || SeekToPercent < 0.f || SeekToPercent > 1.f)
    {
        return false;
    }

    FTimespan SeekToTime = FileInfo.Duration * SeekToPercent;
    return SUCCEEDED(m_spReader->StartSource(static_cast<uint64>(SeekToTime.GetTicks())));
}

void USVFReaderPassThrough::SetReaderClockScale(float ClockScale)
{
    if (!m_spReader)
    {
        return;
    }
    m_spReader->SetClockScale(ClockScale);
}

void USVFReaderPassThrough::SetAudioVolume(float volume)
{
    checkSlow(m_spReader);
    if (m_spReader)
    {
        m_spReader->SetAudioVolumeLevel(volume);
    }
}

float USVFReaderPassThrough::GetAudioVolume()
{
    checkSlow(m_spReader);
    if (m_spReader)
    {
        return m_spReader->GetAudioVolumeLevel();
    }
    else
    {
        return -1.0f;
    }
}

#else
USVFReaderPassThrough::USVFReaderPassThrough() {}
USVFReaderPassThrough::~USVFReaderPassThrough() {}
#endif

#undef LogSVF
#undef WarnSVF
#undef FatalSVF
