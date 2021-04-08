// Copyright (C) Microsoft Corporation. All rights reserved.

#if PLATFORM_WINDOWS

#include "SVFPrivateTypes.h"
#include "UnrealSVF.h"
#include "SVFClockInterface.h"
#include "SVFCallbackInterface.h"
#include "USFVErrorToStr.h"

#include "EngineMinimal.h"
#include "HAL/PlatformFilemanager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/IPlatformFileModule.h"
#include "HAL/PlatformFile.h"
#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 26
#include "Rendering/Texture2DResource.h"
#endif

#define INITGUID
#include "svfguids.h"

#if SVF_USED3D11
// Include D3D11 RHI
#include "D3D11State.h"
#include "D3D11Resources.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogSVFPrivate, Log, All);

#define LogSVF(pmt, ...) UE_LOG(LogSVFPrivate, Log, TEXT(pmt), ##__VA_ARGS__)
#define WarnSVF(pmt, ...) UE_LOG(LogSVFPrivate, Warning, TEXT(pmt), ##__VA_ARGS__)
#define FatalSVF(pmt, ...) UE_LOG(LogSVFPrivate, Fatal, TEXT(pmt), ##__VA_ARGS__)

DECLARE_CYCLE_STAT(TEXT("Get Next Frame"), STAT_SVF_GetNextFrame, STATGROUP_UnrealSVF);
DECLARE_CYCLE_STAT(TEXT("Copy Vertices buffer"), STAT_SVF_CopyVerticeBuffer, STATGROUP_UnrealSVF);
DECLARE_CYCLE_STAT(TEXT("Copy Indices buffer"), STAT_SVF_CopyIndicesBuffer, STATGROUP_UnrealSVF);
DECLARE_CYCLE_STAT(TEXT("Copy Texture buffer"), STAT_SVF_CopyTextureBuffer, STATGROUP_UnrealSVF);

#define TRUE 1
#define FALSE 0

// --------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE USVFReaderCallbackUseUnrealInterface::AddRef() {
    return FPlatformAtomics::InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE USVFReaderCallbackUseUnrealInterface::Release() {
    ULONG ref = FPlatformAtomics::InterlockedDecrement(&m_cRef);
    if (ref == 0) {
        delete this;
    }
    return ref;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT STDMETHODCALLTYPE USVFReaderCallbackUseUnrealInterface::QueryInterface(REFIID riid, void **ppvObject) {
    _ASSERT(ppvObject);
    if (ppvObject == nullptr) {
        return E_POINTER;
    }
    if (riid == __uuidof(IUnknown)) {
        *ppvObject = reinterpret_cast<void**>(static_cast<IUnknown*>(static_cast<ISVFReaderStateCallback*>(this)));
    }
    else if (riid == __uuidof(ISVFReaderStateCallback)) {
        *ppvObject = reinterpret_cast<void**>(static_cast<ISVFReaderStateCallback*>(this));
    }
    else if (riid == __uuidof(ISVFReaderInternalStateCallback)) {
        *ppvObject = reinterpret_cast<void**>(static_cast<ISVFReaderInternalStateCallback*>(this));
    }
    else if (riid == __uuidof(ISVFReaderStatisticsCallback)) {
        *ppvObject = reinterpret_cast<void**>(static_cast<ISVFReaderStatisticsCallback*>(this));
    }
    else {
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

HRESULT USVFReaderCallbackUseUnrealInterface::Initialize(UObject* InUnrealObject) {
    UnrealCallback = InUnrealObject;
    checkSlow(UnrealCallback.IsValid());
    if (!UnrealCallback.IsValid()) {
        return E_POINTER;
    }
    InterfacePtr = Cast<ISVFCallbackInterface>(InUnrealObject);
    checkfSlow(InterfacePtr, TEXT("Object (class %s) not implement ISVFCallbackInterface!"), *InUnrealObject->GetClass()->GetName());
    if (!InterfacePtr) {
        return E_NOINTERFACE;
    }

    return S_OK;
}

HRESULT USVFReaderCallbackUseUnrealInterface::OnStateChange(ESVFReaderState oldState, ESVFReaderState newState) {
    if (!UnrealCallback.IsValid()) {
        return E_POINTER;
    }
    check(InterfacePtr);
    if (!InterfacePtr->OnStateChange((EUSVFReaderState)oldState, (EUSVFReaderState)newState)) {
        return E_UNEXPECTED;
    }
    return S_OK;
}

void USVFReaderCallbackUseUnrealInterface::OnError(HRESULT hrError) {
    if (!UnrealCallback.IsValid()) {
        return;
    }
    check(InterfacePtr);
    InterfacePtr->OnError(UnrealSFV_Private::GetHrStr(hrError), hrError);
}

void USVFReaderCallbackUseUnrealInterface::OnEndOfStream(UINT32 remainingVideoSampleCount) {
    if (!UnrealCallback.IsValid()) {
        return;
    }
    check(InterfacePtr);
    InterfacePtr->OnEndOfStream(remainingVideoSampleCount);
}

void USVFReaderCallbackUseUnrealInterface::UpdateSVFStatistics(UINT32 unsuccessfulReadFrameCount, UINT32 lastReadFrame, UINT32 droppedFrameCount) {
    if (!UnrealCallback.IsValid()) {
        return;
    }
    check(InterfacePtr);
    InterfacePtr->UpdateSVFStatistics(unsuccessfulReadFrameCount, lastReadFrame, droppedFrameCount);
}


// ### Clock
// --------------------------------------------------------------------------
USVFClockUseUnrealInterface::USVFClockUseUnrealInterface()
    : m_RefCnt(1L) // @TODO: Check, realy need start from 1?
    , m_DestructionFlag(0L)
    , InterfacePtr(nullptr)
{

}

// --------------------------------------------------------------------------
USVFClockUseUnrealInterface::~USVFClockUseUnrealInterface() {
    InterfacePtr = nullptr;
    UnrealClock.Reset();
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseUnrealInterface::Initialize(UObject* InUnrealObject) {
    UnrealClock = InUnrealObject;
    checkSlow(UnrealClock.IsValid());
    if (!UnrealClock.IsValid()) {
        return E_POINTER;
    }
    InterfacePtr = Cast<ISVFClockInterface>(InUnrealObject);
    checkfSlow(InterfacePtr, TEXT("Object (class %s) not implement ISVFClockInterface!"), *InUnrealObject->GetClass()->GetName());
    if (!InterfacePtr) {
        return E_NOINTERFACE;
    }

    if (!InterfacePtr->SVFClock_Initialize()) {
        return E_UNEXPECTED;
    }

    return S_OK;
}

// --------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE USVFClockUseUnrealInterface::AddRef() {
    checkSlow(m_DestructionFlag == FALSE);
    return FPlatformAtomics::InterlockedIncrement(&m_RefCnt);
}

// --------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE USVFClockUseUnrealInterface::Release() {
    ULONG ref = FPlatformAtomics::InterlockedDecrement(&m_RefCnt);
    if (ref == 0) {
        if (FALSE == FPlatformAtomics::InterlockedCompareExchange(&m_DestructionFlag, TRUE, FALSE)) {
            delete this;
        }
    }
    return ref;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT STDMETHODCALLTYPE USVFClockUseUnrealInterface::QueryInterface(REFIID riid, void **ppvObject) {
    if (ppvObject == nullptr) {
        return E_POINTER;
    }
    *ppvObject = nullptr;
    if (riid == __uuidof(ISVFClock)) {
        *ppvObject = static_cast<ISVFClock*>(this);
    }
    else if (riid == __uuidof(IUnknown)) {
        *ppvObject = static_cast<IUnknown*>(this);
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT USVFClockUseUnrealInterface::GetPresentationTime(SVFTIME *pTime) {
    checkSlow(pTime);
    if (pTime == nullptr) {
        return E_POINTER;
    }
    if (!UnrealClock.IsValid()) {
        return E_POINTER;
    }
    check(InterfacePtr);
    if (!InterfacePtr->SVFClock_GetPresentationTime(pTime)) {
        return E_UNEXPECTED;
    }
    return S_OK;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT USVFClockUseUnrealInterface::GetTime(SVFTIME *pTime) {
    checkSlow(pTime);
    if (pTime == nullptr) {
        return E_POINTER;
    }
    if (!UnrealClock.IsValid()) {
        return E_POINTER;
    }
    check(InterfacePtr);
    if (!InterfacePtr->SVFClock_GetTime(pTime)) {
        return E_UNEXPECTED;
    }
    return S_OK;
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseUnrealInterface::Start() {
    if (!UnrealClock.IsValid()) {
        return E_POINTER;
    }
    check(InterfacePtr);
    if (!InterfacePtr->SVFClock_Start()) {
        return E_UNEXPECTED;
    }
    return S_OK;
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseUnrealInterface::Stop() {
    if (!UnrealClock.IsValid()) {
        return E_POINTER;
    }
    check(InterfacePtr);
    if (!InterfacePtr->SVFClock_Stop()) {
        return E_UNEXPECTED;
    }
    return S_OK;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT USVFClockUseUnrealInterface::GetState(ESVFClockState *pState) {
    checkSlow(pState);
    if (pState == nullptr) {
        return E_POINTER;
    }
    if (!UnrealClock.IsValid()) {
        return E_POINTER;
    }
    check(InterfacePtr);
    EUSVFClockState UState;
    if (!InterfacePtr->SVFClock_GetState(&UState)) {
        return E_UNEXPECTED;
    }
    *pState = (ESVFClockState)UState;
    return S_OK;
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseUnrealInterface::Shutdown() {
    if (!UnrealClock.IsValid()) {
        return E_POINTER;
    }
    check(InterfacePtr);
    if (!InterfacePtr->SVFClock_Start()) {
        return E_UNEXPECTED;
    }
    return S_OK;
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseUnrealInterface::SetPresentationOffset(SVFTIME t) {
    if (!UnrealClock.IsValid()) {
        return E_POINTER;
    }
    check(InterfacePtr);
    if (!InterfacePtr->SVFClock_SetPresentationOffset(t)) {
        return E_UNEXPECTED;
    }
    return S_OK;
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseUnrealInterface::SnapPresentationOffset() {
    if (!UnrealClock.IsValid()) {
        return E_POINTER;
    }
    check(InterfacePtr);
    if (!InterfacePtr->SVFClock_SnapPresentationOffset()) {
        return E_UNEXPECTED;
    }
    return S_OK;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT USVFClockUseUnrealInterface::GetPresentationOffset(SVFTIME *pTime) {
    checkSlow(pTime);
    if (pTime == nullptr) {
        return E_POINTER;
    }
    if (!UnrealClock.IsValid()) {
        return E_POINTER;
    }
    check(InterfacePtr);
    if (!InterfacePtr->SVFClock_GetPresentationOffset(pTime)) {
        return E_UNEXPECTED;
    }
    return S_OK;
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseUnrealInterface::SetScale(float scale) {
    if (!UnrealClock.IsValid()) {
        return E_POINTER;
    }
    check(InterfacePtr);
    if (!InterfacePtr->SVFClock_SetScale(scale)) {
        return E_UNEXPECTED;
    }
    return S_OK;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT USVFClockUseUnrealInterface::GetScale(float *pScale) {
    checkSlow(pScale);
    if (pScale == nullptr) {
        return E_POINTER;
    }
    if (!UnrealClock.IsValid()) {
        return E_POINTER;
    }
    check(InterfacePtr);
    if (!InterfacePtr->SVFClock_GetScale(pScale)) {
        return E_UNEXPECTED;
    }
    return S_OK;
}


// --------------------------------------------------------------------------
USVFClockUseSystemInterface::USVFClockUseSystemInterface() :
    m_RefCnt(1L),
    m_DestructionFlag(0L),
    m_PresentationOffset(0),
    m_ClockOffset(0),
    m_Scale(1.0f),
    m_ClockState(ESVFClockState::Stopped) {
}

// --------------------------------------------------------------------------
USVFClockUseSystemInterface::~USVFClockUseSystemInterface() {
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseSystemInterface::Initialize() {
    HRESULT hr = S_OK;
    m_spPresentationClock = nullptr;
    hr = MFCreatePresentationClock(&m_spPresentationClock);
    if (FAILED(hr)) {
        return hr;
    }
    ComPtr<IMFPresentationTimeSource> spPresTimeSource;
    hr = MFCreateSystemTimeSource(&spPresTimeSource);
    if (FAILED(hr)) {
        return hr;
    }
    hr = m_spPresentationClock->SetTimeSource(spPresTimeSource.Get());
    if (FAILED(hr)) {
        return hr;
    }
    return S_OK;
}


// --------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE USVFClockUseSystemInterface::AddRef() {
    _ASSERT(m_DestructionFlag == FALSE);
    return FPlatformAtomics::InterlockedIncrement(&m_RefCnt);
}

// --------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE USVFClockUseSystemInterface::Release() {
    ULONG ref = FPlatformAtomics::InterlockedDecrement(&m_RefCnt);
    if (ref == 0) {
        if (FALSE == FPlatformAtomics::InterlockedCompareExchange(&m_DestructionFlag, TRUE, FALSE)) {
            delete this;
        }
    }
    return ref;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT STDMETHODCALLTYPE USVFClockUseSystemInterface::QueryInterface(REFIID riid, void **ppvObject) {
    if (ppvObject == nullptr) {
        return E_POINTER;
    }
    *ppvObject = nullptr;
    if (riid == __uuidof(ISVFClock)) {
        *ppvObject = static_cast<ISVFClock*>(this);
    }
    else if (riid == __uuidof(IUnknown)) {
        *ppvObject = static_cast<IUnknown*>(this);
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT USVFClockUseSystemInterface::GetPresentationTime(SVFTIME *pTime) {
    _ASSERT(pTime);
    if (pTime == nullptr) {
        return E_POINTER;
    }
    SVFTIME t;
    HRESULT hr = GetTime(&t);
    if (SUCCEEDED(hr)) {
        *pTime = t - m_PresentationOffset;
    }
    return hr;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT USVFClockUseSystemInterface::GetTime(SVFTIME *pTime) {
    _ASSERT(pTime);
    if (pTime == nullptr) {
        return E_POINTER;
    }
    SVFTIME t = 0;
    HRESULT hr = S_OK;

    if (m_ClockState == ESVFClockState::Running) {
        hr = m_spPresentationClock->GetTime(&t);
    }
    else {
        t = m_ClockOffset;
    }

    if (SUCCEEDED(hr)) {
        *pTime = static_cast<SVFTIME>(t * m_Scale);
    }
    return hr;
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseSystemInterface::Start() {
    HRESULT hr = S_OK;
    hr = m_spPresentationClock->Start(m_ClockOffset);
    if (SUCCEEDED(hr)) {
        m_ClockState = ESVFClockState::Running;
    }
    return hr;
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseSystemInterface::Stop() {
    HRESULT hr = S_OK;
    _ASSERT(m_spPresentationClock);
    hr = m_spPresentationClock->GetTime(&m_ClockOffset);
    if (SUCCEEDED(hr)) {
        hr = m_spPresentationClock->Stop();
    }
    if (SUCCEEDED(hr)) {
        m_ClockState = ESVFClockState::Stopped;
    }
    return hr;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT USVFClockUseSystemInterface::GetState(ESVFClockState *pState) {
    HRESULT hr = S_OK;
    _ASSERT(pState);
    if (pState == nullptr) {
        return E_POINTER;
    }
    *pState = m_ClockState;
    return hr;
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseSystemInterface::Shutdown() {
    HRESULT hr = S_OK;
    m_ClockOffset = 0L;
    return hr;
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseSystemInterface::SetPresentationOffset(SVFTIME t) {
    m_PresentationOffset = t;
    return S_OK;
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseSystemInterface::SnapPresentationOffset() {
    return GetTime(&m_PresentationOffset);
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT USVFClockUseSystemInterface::GetPresentationOffset(SVFTIME *pTime) {
    HRESULT hr = S_OK;
    _ASSERT(pTime);
    if (pTime == nullptr) {
        return E_POINTER;
    }
    *pTime = m_PresentationOffset;
    return hr;
}

// --------------------------------------------------------------------------
HRESULT USVFClockUseSystemInterface::SetScale(float scale) {
    m_Scale = scale;
    (void)SnapPresentationOffset();
    return S_OK;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT USVFClockUseSystemInterface::GetScale(float *pScale) {
    HRESULT hr = S_OK;
    _ASSERT(pScale);
    if (pScale == nullptr) {
        return E_POINTER;
    }
    *pScale = m_Scale;
    return hr;
}


FSVFRingBuffer::FSVFRingBuffer()
    : Data(nullptr),
    Capacity(0),
    Offset(0)
{

}

FSVFRingBuffer::FSVFRingBuffer(uint64 AllocationSize)
{
    Capacity = AllocationSize;
    Data = (uint8*)FMemory::Malloc(Capacity);
    Offset = 0;
}

FSVFRingBuffer::~FSVFRingBuffer()
{
    if (Data)
    {
        FMemory::Free(Data);
        Data = nullptr;
        Capacity = 0;
    }
}

uint8* FSVFRingBuffer::Allocate(uint64 Size)
{
    Offset += Size;
    if (Offset >= Capacity)
        Offset = 0;

    return (Data + Offset);
}

// --------------------------------------------------------------------------
//  SVFFrameHelper namespace
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
HRESULT SVFFrameHelper::ExtractBuffers(ComPtr<ISVFFrame>& spFrame, ComPtr<ISVFBuffer>& spTextureBuffer, ComPtr<ISVFBuffer>& spVertexBuffer, ComPtr<ISVFBuffer>& spIndexBuffer, ComPtr<ISVFBuffer>& spAudioBuffer) {
    if (!spFrame) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: spFrame is null");
        return E_POINTER;
    }
    HRESULT hr = S_OK;

    ULONG bufferCount = 0;
    hr = spFrame->GetBufferCount(&bufferCount);
    if (FAILED(hr) || bufferCount < 3) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: failed to get buffer count (must be >=3), hr = 0x%08X, bufferCount = %ld", hr, bufferCount);
        if (S_OK == hr) {
            hr = E_UNEXPECTED;
        }
        return hr;
    }
    for (ULONG i = 0; i < bufferCount; i++) {
        ComPtr<ISVFBuffer> spBuffer;
        ESVFStreamType bufferType = ESVFStreamType::SVFUnknown;
        hr = spFrame->GetBuffer(i, &spBuffer, &bufferType);
        if (!spBuffer) {
            continue;
        }
        switch (bufferType) {
        case SVFRGBTexture:
            spTextureBuffer = spBuffer;
            break;
        case SVFVertexBuffer:
            spVertexBuffer = spBuffer;
            break;
        case SVFIndexBuffer:
            spIndexBuffer = spBuffer;
            break;
        case SVFAudio:
            spAudioBuffer = spBuffer;
            break;
        }
    } // for i

    if (!spTextureBuffer) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: cannot find texture buffer");
        return E_UNEXPECTED;
    }
    if (!spVertexBuffer) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: cannot find vertex buffer");
        return E_UNEXPECTED;
    }
    if (!spIndexBuffer) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: cannot find index buffer");
        return E_UNEXPECTED;
    }

    return S_OK;
}

HRESULT SVFFrameHelper::ExtractVerticesBuffer(ComPtr<ISVFFrame>& spFrame, ComPtr<ISVFBuffer>& spVertexBuffer) {
    if (!spFrame) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: spFrame is null");
        return E_POINTER;
    }
    HRESULT hr = S_OK;

    ULONG bufferCount = 0;
    hr = spFrame->GetBufferCount(&bufferCount);
    if (FAILED(hr) || bufferCount < 3) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: failed to get buffer count (must be >=3), hr = 0x%08X, bufferCount = %ld", hr, bufferCount);
        if (S_OK == hr) {
            hr = E_UNEXPECTED;
        }
        return hr;
    }
    for (ULONG i = 0; i < bufferCount; i++) {
        ComPtr<ISVFBuffer> spBuffer;
        ESVFStreamType bufferType;
        hr = spFrame->GetBuffer(i, &spBuffer, &bufferType);
        if (!spBuffer) {
            continue;
        }
        if (bufferType == SVFVertexBuffer) {
            spVertexBuffer = spBuffer;
            break;
        }
        spBuffer = nullptr;
    } // for i

    if (!spVertexBuffer) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: cannot find vertex buffer");
        return E_UNEXPECTED;
    }

    return S_OK;
}

HRESULT SVFFrameHelper::ExtractIndicesBuffer(ComPtr<ISVFFrame>& spFrame, ComPtr<ISVFBuffer>& spIndexBuffer) {
    if (!spFrame) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: spFrame is null");
        return E_POINTER;
    }
    HRESULT hr = S_OK;

    ULONG bufferCount = 0;
    hr = spFrame->GetBufferCount(&bufferCount);
    if (FAILED(hr) || bufferCount < 3) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: failed to get buffer count (must be >=3), hr = 0x%08X, bufferCount = %ld", hr, bufferCount);
        if (S_OK == hr) {
            hr = E_UNEXPECTED;
        }
        return hr;
    }
    for (ULONG i = 0; i < bufferCount; i++) {
        ComPtr<ISVFBuffer> spBuffer;
        ESVFStreamType bufferType;
        hr = spFrame->GetBuffer(i, &spBuffer, &bufferType);
        if (!spBuffer) {
            continue;
        }
        if (bufferType == SVFIndexBuffer) {
            spIndexBuffer = spBuffer;
            break;
        }
        spBuffer = nullptr;
    } // for i

    if (!spIndexBuffer) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: cannot find index buffer");
        return E_UNEXPECTED;
    }

    return S_OK;
}

HRESULT SVFFrameHelper::ExtractTextureBuffer(ComPtr<ISVFFrame>& spFrame, ComPtr<ISVFBuffer>& spTextureBuffer) {
    if (!spFrame) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: spFrame is null");
        return E_POINTER;
    }
    HRESULT hr = S_OK;

    ULONG bufferCount = 0;
    hr = spFrame->GetBufferCount(&bufferCount);
    if (FAILED(hr) || bufferCount < 3) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: failed to get buffer count (must be >=3), hr = 0x%08X, bufferCount = %ld", hr, bufferCount);
        if (S_OK == hr) {
            hr = E_UNEXPECTED;
        }
        return hr;
    }
    for (ULONG i = 0; i < bufferCount; i++) {
        ComPtr<ISVFBuffer> spBuffer;
        ESVFStreamType bufferType;
        hr = spFrame->GetBuffer(i, &spBuffer, &bufferType);
        if (!spBuffer) {
            continue;
        }
        if (bufferType == SVFRGBTexture) {
            spTextureBuffer = spBuffer;
            break;
        }
        spBuffer = nullptr;
    } // for i

    if (!spTextureBuffer) {
        FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: cannot find texture buffer");
        return E_UNEXPECTED;
    }

    return S_OK;
}

HRESULT SVFFrameHelper::ExtractAudioBuffer(ComPtr<ISVFFrame>& spFrame, ComPtr<ISVFBuffer>& spAudioBuffer)
{
    if (!spFrame) {
        FatalSVF("Error in CSVFFrameHelper::ExtractAudioBuffer: spFrame is null");
        return E_POINTER;
    }
    HRESULT hr = S_OK;

    ULONG bufferCount = 0;
    hr = spFrame->GetBufferCount(&bufferCount);
    if (FAILED(hr) || bufferCount < 3) {
        FatalSVF("Error in CSVFFrameHelper::ExtractAudioBuffer: failed to get buffer count (must be >=3), hr = 0x%08X, bufferCount = %ld", hr, bufferCount);
        if (S_OK == hr) {
            hr = E_UNEXPECTED;
        }
        return hr;
    }
    for (ULONG i = 0; i < bufferCount; i++) {
        ComPtr<ISVFBuffer> spBuffer;
        ESVFStreamType bufferType;
        hr = spFrame->GetBuffer(i, &spBuffer, &bufferType);
        if (!spBuffer) {
            continue;
        }
        if (bufferType == SVFAudio) {
            spAudioBuffer = spBuffer;
            break;
        }
        spBuffer = nullptr;
    } // for i

    //if (!spAudioBuffer) {
    //    FatalSVF("Error in CSVFFrameHelper::ExtractBuffers: cannot find texture buffer");
    //    return E_UNEXPECTED;
    //}

    return S_OK;
}

HRESULT SVFFrameHelper::CopyVerticesBuffer(ComPtr<ISVFBuffer>& spVertexBuffer, FDynamicMeshVertex* OutVertices, bool bUseNormal, uint32 VertexCount) {
    if (!spVertexBuffer || !OutVertices) {
        return E_POINTER;
    }
    if (VertexCount < 3) {
        return E_INVALIDARG;
    }

    DWORD ActualSize = 0L;
    HRESULT hr = S_OK;
    spVertexBuffer->GetSize(&ActualSize);
    uint32 NeedSize = VertexCount * (bUseNormal ? sizeof(CSVFVertex_Norm_Full) : sizeof(CSVFVertex_Full));
    if (ActualSize != NeedSize) {
        UE_LOG(LogTemp, Error, TEXT("ActualSize (%ld) not equal to NeedSize (%ld)"), ActualSize, NeedSize);
        return E_UNEXPECTED;
    }

    SVFLockedMemory lockedMem;
    ZeroMemory(&lockedMem, sizeof(lockedMem));
    hr = spVertexBuffer->LockBuffer(&lockedMem);
    if (FAILED(hr)) {
        UE_LOG(LogTemp, Error, TEXT("Error in CopyBufferToArray: failed to lock SVF vertex buffer, hr = 0x%08X"), hr);
        return hr;
    }
    if (ActualSize < lockedMem.Size) {
        UE_LOG(LogTemp, Error, TEXT("Error in CopyBufferToArray: vertex buffer size is %ld while current frame vertex buffer size is %ld"), ActualSize, lockedMem.Size);
        spVertexBuffer->UnlockBuffer();
        return E_OUTOFMEMORY;
    }

    if (bUseNormal) {
        auto fData = (CSVFVertex_Norm_Full const*)lockedMem.pData;
        for (uint32 i = 0; i < VertexCount; ++i) {
            FDynamicMeshVertex& Vert = OutVertices[i];
            Vert.Position = FVector(fData[i].z, fData[i].x, fData[i].y);
            Vert.TextureCoordinate[0] = FVector2D(fData[i].u, fData[i].v);
            Vert.TangentX = FVector(1, 0, 0);
            Vert.TangentZ = FVector(-fData[i].nz, -fData[i].nx, -fData[i].ny);
            Vert.TangentZ.Vector.W = 127;
            Vert.Color = FColor(255, 255, 255);
        }
    }
    else {
        auto fData = (CSVFVertex_Full const*)lockedMem.pData;
        for (uint32 i = 0; i < VertexCount; ++i) {
            FDynamicMeshVertex& Vert = OutVertices[i];
            Vert.Position = FVector(fData[i].z, fData[i].x, fData[i].y);
            Vert.TextureCoordinate[0] = FVector2D(fData[i].u, fData[i].v);
            Vert.TangentX = FVector(1, 0, 0);
            Vert.TangentZ = FVector(0, 0, 1);
            Vert.TangentZ.Vector.W = 127;
            Vert.Color = FColor(255, 255, 255);
        }
    }

    spVertexBuffer->UnlockBuffer();

    return S_OK;
}

HRESULT SVFFrameHelper::CopyIndicesBuffer(ComPtr<ISVFBuffer>& spIndexBuffer, int32* OutIndices, uint32 IndicesCount) {
    if (!spIndexBuffer || !OutIndices) {
        return E_POINTER;
    }
    if (IndicesCount < 3) {
        return E_INVALIDARG;
    }

    DWORD ActualSize = 0L;
    HRESULT hr = S_OK;
    spIndexBuffer->GetSize(&ActualSize);
    uint32 NeedSize = IndicesCount * sizeof(int32);
    if (ActualSize != NeedSize) {
        UE_LOG(LogTemp, Error, TEXT("ActualSize (%ld) not equal to NeedSize (%ld)"), ActualSize, NeedSize);
        return false;
    }

    SVFLockedMemory lockedMem;
    ZeroMemory(&lockedMem, sizeof(lockedMem));
    hr = spIndexBuffer->LockBuffer(&lockedMem);
    if (FAILED(hr)) {
        UE_LOG(LogTemp, Error, TEXT("Error in CopyBufferToArray: failed to lock SVF vertex buffer, hr = 0x%08X"), hr);
        return hr;
    }
    if (ActualSize < lockedMem.Size) {
        UE_LOG(LogTemp, Error, TEXT("Error in CopyBufferToArray: vertex buffer size is %ld while current frame vertex buffer size is %ld"), ActualSize, lockedMem.Size);
        spIndexBuffer->UnlockBuffer();
        return E_OUTOFMEMORY;
    }

    FMemory::Memcpy(OutIndices, lockedMem.pData, lockedMem.Size);

    spIndexBuffer->UnlockBuffer();

    return S_OK;
}

HRESULT SVFFrameHelper::CopyTextureBuffer(ComPtr<ISVFBuffer>& spTextureBuffer, TArray<uint8>& OutData) {
    if (!spTextureBuffer) {
        return E_POINTER;
    }

    HRESULT hr = S_OK;

#if SVF_USED3D11
    ComPtr<ID3D11Texture2D> spSVFTexture2D;
    hr = spTextureBuffer.CopyTo(spSVFTexture2D.GetAddressOf());
    if (S_OK == hr) {
        // [HW texture]
        ComPtr<ID3D11Device> spDevice;
        spSVFTexture2D->GetDevice(&spDevice);

        ComPtr<ID3D11DeviceContext> spCtx;
        spDevice->GetImmediateContext(&spCtx);

        D3D11_TEXTURE2D_DESC descSVFTexture2D;
        spSVFTexture2D->GetDesc(&descSVFTexture2D);

        D3D11_TEXTURE2D_DESC m_descTexture;
        ZeroMemory(&m_descTexture, sizeof(m_descTexture));

        D3D11_TEXTURE2D_DESC descTextureSVF;
        spSVFTexture2D->GetDesc(&descTextureSVF);

        m_descTexture.Width = descTextureSVF.Width;
        m_descTexture.Height = descTextureSVF.Height;
        m_descTexture.MipLevels = descTextureSVF.MipLevels;
        m_descTexture.ArraySize = descTextureSVF.ArraySize;
        m_descTexture.Format = descTextureSVF.Format;
        m_descTexture.SampleDesc = descTextureSVF.SampleDesc;
        m_descTexture.Usage = D3D11_USAGE_STAGING;
        m_descTexture.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        //m_descTexture.MiscFlags = descTextureSVF.MiscFlags;
        // The flag here was preventing us from creating the texture
        // with a CREATETEXTURE2D_INVALIDMISCFLAGS

        ComPtr<ID3D11Texture2D> spReadTexture;
        hr = spDevice->CreateTexture2D(&m_descTexture, nullptr, &spReadTexture);
        if (FAILED(hr)) {
            FatalSVF("Error in CSVFPluginBufferTexture::CopyHardwareTexture: failed to create read texture, hr = 0x%08X", hr);
            return hr;
        }

        spCtx->CopyResource(spReadTexture.Get(), spSVFTexture2D.Get());
        D3D11_MAPPED_SUBRESOURCE mapResource;
        hr = spCtx->Map(spReadTexture.Get(), 0, D3D11_MAP_READ, 0L, &mapResource);
        //hr = spCtx->Map(spSVFTexture2D, 0, D3D11_MAP_READ, 0L, &mapResource);
        if (FAILED(hr)) {
            FatalSVF("Error in CSVFPluginBufferTexture::CopyHardwareTexture: failed to map read texture, hr = 0x%08X", hr);
            return hr;
        }

        int32 MemSize = descSVFTexture2D.Height * mapResource.RowPitch;
        if (OutData.Num() > 0) {
            OutData.Empty(MemSize);
        }
        OutData.AddUninitialized(MemSize);
        FMemory::Memcpy(OutData.GetData(), mapResource.pData, MemSize);
        spCtx->Unmap(spReadTexture.Get(), 0);
        //spCtx->Unmap(spSVFTexture2D, 0);
    }
    else
#endif
    {
        // [SW texture]
        DWORD ActualSize = 0L;
        spTextureBuffer->GetSize(&ActualSize);
        SVFLockedMemory lockedMem;
        ZeroMemory(&lockedMem, sizeof(lockedMem));
        hr = spTextureBuffer->LockBuffer(&lockedMem);
        if (FAILED(hr)) {
            WarnSVF("Error in CopyBufferToArray: failed to lock SVF vertex buffer, hr = 0x%08X", hr);
            return hr;
        }
        if (ActualSize < lockedMem.Size) {
            WarnSVF("Error in CopyBufferToArray: vertex buffer size is %ld while current frame vertex buffer size is %ld", ActualSize, lockedMem.Size);
            spTextureBuffer->UnlockBuffer();
            return E_OUTOFMEMORY;
        }

        if (OutData.Num() > 0) {
            OutData.Empty(lockedMem.Size);
        }
        OutData.AddUninitialized(lockedMem.Size);
        FMemory::Memcpy(OutData.GetData(), lockedMem.pData, lockedMem.Size);
        spTextureBuffer->UnlockBuffer();
    }

    return S_OK;
}

HRESULT SVFFrameHelper::CopyTextureBuffer(ComPtr<ISVFBuffer>& spTextureBuffer, uint8** OutData, int32& OutDataSize, bool& bOutCalleeShouldFreeMemory) {
    if (!spTextureBuffer || !OutData) {
        return E_POINTER;
    }

    HRESULT hr = S_OK;

#if SVF_USED3D11
    ComPtr<ID3D11Texture2D> spSVFTexture2D;
    hr = spTextureBuffer.CopyTo(spSVFTexture2D.GetAddressOf());
    if (S_OK == hr) {
        // [HW texture]
        ComPtr<ID3D11Device> spDevice;
        spSVFTexture2D->GetDevice(&spDevice);

        ComPtr<ID3D11DeviceContext> spCtx;
        spDevice->GetImmediateContext(&spCtx);

        D3D11_TEXTURE2D_DESC m_descTexture;
        ZeroMemory(&m_descTexture, sizeof(m_descTexture));

        D3D11_TEXTURE2D_DESC descTextureSVF;
        spSVFTexture2D->GetDesc(&descTextureSVF);

        m_descTexture.Width = descTextureSVF.Width;
        m_descTexture.Height = descTextureSVF.Height;
        m_descTexture.MipLevels = descTextureSVF.MipLevels;
        m_descTexture.ArraySize = descTextureSVF.ArraySize;
        m_descTexture.Format = descTextureSVF.Format;
        m_descTexture.SampleDesc = descTextureSVF.SampleDesc;
        m_descTexture.Usage = D3D11_USAGE_STAGING;
        m_descTexture.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        m_descTexture.MiscFlags = descTextureSVF.MiscFlags;

        ComPtr<ID3D11Texture2D> spReadTexture;
        hr = spDevice->CreateTexture2D(&m_descTexture, nullptr, &spReadTexture);
        if (FAILED(hr)) {
            FatalSVF("Error in CSVFPluginBufferTexture::CopyHardwareTexture: failed to create read texture, hr = 0x%08X", hr);
            return hr;
        }

        spCtx->CopyResource(spReadTexture.Get(), spSVFTexture2D.Get());
        D3D11_MAPPED_SUBRESOURCE mapResource;
        hr = spCtx->Map(spReadTexture.Get(), 0, D3D11_MAP_READ, 0L, &mapResource);
        //hr = spCtx->Map(spSVFTexture2D, 0, D3D11_MAP_READ, 0L, &mapResource);
        if (FAILED(hr)) {
            FatalSVF("Error in CSVFPluginBufferTexture::CopyHardwareTexture: failed to map read texture, hr = 0x%08X", hr);
            return hr;
        }

        OutDataSize = descTextureSVF.Height * mapResource.RowPitch;
        *OutData = (uint8*)FMemory::Malloc(OutDataSize);
        bOutCalleeShouldFreeMemory = true;
        FMemory::Memcpy(*OutData, mapResource.pData, OutDataSize);
        spCtx->Unmap(spReadTexture.Get(), 0);
    }
    else
#endif
    {
        // [SW texture]
        DWORD ActualSize = 0L;
        spTextureBuffer->GetSize(&ActualSize);
        SVFLockedMemory lockedMem;
        ZeroMemory(&lockedMem, sizeof(lockedMem));
        hr = spTextureBuffer->LockBuffer(&lockedMem);
        if (FAILED(hr)) {
            WarnSVF("Error in CopyBufferToArray: failed to lock SVF vertex buffer, hr = 0x%08X", hr);
            return hr;
        }
        if (ActualSize < lockedMem.Size) {
            WarnSVF("Error in CopyBufferToArray: vertex buffer size is %ld while current frame vertex buffer size is %ld", ActualSize, lockedMem.Size);
            spTextureBuffer->UnlockBuffer();
            return E_OUTOFMEMORY;
        }

        if (*OutData == nullptr)
        {
            *OutData = (uint8*)FMemory::Malloc(lockedMem.Size);
            bOutCalleeShouldFreeMemory = true;
        }
        OutDataSize = lockedMem.Size;
        //*OutData = MoveTemp(lockedMem.pData);
        FMemory::Memcpy(*OutData, lockedMem.pData, lockedMem.Size);
        spTextureBuffer->UnlockBuffer();
    }

    return S_OK;
}

// --------------------------------------------------------------------------
HRESULT SVFFrameHelper::ExtractFrameInfo(ComPtr<ISVFBuffer>& spVertexBuffer, bool isEOS, SVFFrameInfo& info) {
    HRESULT hr = S_OK;
    _ASSERT(spVertexBuffer);

    if (!spVertexBuffer) {
        FatalSVF("Error in CSVFFrameHelper::ExtractFrameInfo: spVertexBuffer is null");
        return E_POINTER;
    }

    ComPtr<ISVFAttributes> spAttributes;
    hr = spVertexBuffer.CopyTo(spAttributes.GetAddressOf());
    if (FAILED(hr)) {
        FatalSVF("Error in CSVFFrameHelper::ExtractFrameInfo: Unable to QI ISVFAttributes from vertex buffer, hr = 0x%08X", hr);
        return hr;
    }

    ZeroMemory(&info, sizeof(info));

    // we ignore errors if the following attributes are not set. They are only informative.
    hr = spAttributes->GetUINT32(SVFAttrib_FrameId, &(info.frameId));
    if (FAILED(hr)) {
        FatalSVF("Error in CSVFFrameHelper::ExtractFrameInfo: Failed to read SVFAttrib_FrameId, hr = 0x%08X", hr);
        return hr;
    }
    hr = spAttributes->GetUINT64(SVFAttrib_FrameTimestamp, &(info.frameTimestamp));
    if (FAILED(hr)) {
        FatalSVF("Error in CSVFFrameHelper::ExtractFrameInfo: Failed to read SVFAttrib_FrameTimestamp, hr = 0x%08X", hr);
        return hr;
    }
    if (info.frameId == 0) {
        // for legacy content, deduce frameId from MF timestamp
        info.frameId = (UINT32)(((info.frameTimestamp + 1) * 30L) / 10000000L) + 1;
    }
    UINT32 isKeyFrame = 1;
    (void)spAttributes->GetUINT32(SVFAttrib_IsKeyFrame, &(isKeyFrame));
    info.isKeyFrame = isKeyFrame == 1;
    (void)spAttributes->GetDouble(SVFAttrib_BBoxMinX, &(info.minY));
    (void)spAttributes->GetDouble(SVFAttrib_BBoxMinY, &(info.minZ));
    (void)spAttributes->GetDouble(SVFAttrib_BBoxMinZ, &(info.minX));
    (void)spAttributes->GetDouble(SVFAttrib_BBoxMaxX, &(info.maxY));
    (void)spAttributes->GetDouble(SVFAttrib_BBoxMaxY, &(info.maxZ));
    (void)spAttributes->GetDouble(SVFAttrib_BBoxMaxZ, &(info.maxX));

    hr = spAttributes->GetUINT32(SVFAttrib_NumVertices, &(info.vertexCount));
    if (FAILED(hr)) {
        FatalSVF("Error in CRenderFrameBuffer::FillFrameInfo: Failed to read SVFAttrib_NumVertices, hr = 0x%08X", hr);
        return hr;
    }
    hr = spAttributes->GetUINT32(SVFAttrib_NumIndices, &(info.indexCount));
    if (FAILED(hr)) {
        FatalSVF("Error in CRenderFrameBuffer::FillFrameInfo: Failed to read SVFAttrib_NumIndices, hr = 0x%08X", hr);
        return hr;
    }
    info.isEOS = isEOS;
    return S_OK;
}

HRESULT SVFFrameHelper::ExtractTextureInfo(ComPtr<ISVFBuffer>& spBuffer, SVFFrameInfo& frameInfo) {
    if (!spBuffer) {
        return E_POINTER;
    }
    ComPtr<ISVFAttributes> spAttributes;
    HRESULT hr = spBuffer->QueryInterface(__uuidof(ISVFAttributes), (void**)&spAttributes);
    if (FAILED(hr)) {
        FatalSVF("Error in ExtractTextureInfo: cannot get ISVFAttributes from ISVFBuffer");
        return hr;
    }
    UINT32 width = 0;
    UINT32 height = 0;
    hr = spAttributes->GetUINT32(SVFAttrib_Width, &width);
    if (FAILED(hr)) {
        FatalSVF("Error in ExtractTextureInfo: cannot get SVFAttrib_Width from ISVFAttributes");
        return hr;
    }
    hr = spAttributes->GetUINT32(SVFAttrib_Height, &height);
    if (FAILED(hr)) {
        FatalSVF("Error in ExtractTextureInfo: cannot get SVFAttrib_Height from ISVFAttributes");
        return hr;
    }
    frameInfo.textureHeight = height;
    frameInfo.textureWidth = width;

    UINT32 inverted = 0;
    hr = spAttributes->GetUINT32(SVFAttrib_Inverted, &inverted);
    if (FAILED(hr)) {
        FatalSVF("Error in ExtractTextureInfo: cannot get SVFAttrib_Inverted from ISVFAttributes");
        return hr;
    }

    return S_OK;
}

// --------------------------------------------------------------------------
_Use_decl_annotations_
void SVFFrameHelper::UpdateSVFStatus(ISVFReader* pReader, ISVFFrame* pFrame, FSVFStatus& status) {
    if (!pFrame || !pReader) {
        return;
    }
    int droppedCount = pReader->GetNumberDroppedFrames();
    UINT32 lastReadFrame = 0L;

    ComPtr<ISVFBuffer> spTB;
    ComPtr<ISVFBuffer> spVB;
    ComPtr<ISVFBuffer> spIB;
    ComPtr<ISVFBuffer> spAB;
    ComPtr<ISVFFrame> spFrame = pFrame;
    HRESULT hr = SVFFrameHelper::ExtractBuffers(spFrame, spTB, spVB, spIB, spAB);
    if (SUCCEEDED(hr) && spVB) {
        SVFFrameInfo frameInfo;
        ZeroMemory(&frameInfo, sizeof(frameInfo));
        hr = SVFFrameHelper::ExtractFrameInfo(spVB, false, frameInfo);
        if (SUCCEEDED(hr)) {
            lastReadFrame = frameInfo.frameId;
        }
    }
    {
        status.droppedFrameCount = (UINT32)droppedCount;
        if (0L != lastReadFrame) {
            status.lastReadFrame = lastReadFrame;
        }
    }
}


void SVFHelpers::CopyConfig(const SVFConfiguration& From, FSVFConfiguration& To) {
    To.useHardwareDecode = From.useHardwareDecode;
    To.useHardwareTextures = From.useHardwareTextures;
    To.outputNV12 = From.outputNV12;
    To.bufferCompressedData = From.bufferCompressedData;
    To.useKeyedMutex = From.useKeyedMutex;
    To.disableAudio = From.disableAudio;
    To.playAudio = From.playAudio;
    To.returnAudio = From.returnAudio;
    To.prerollSize = From.prerollSize;
    To.minBufferSize = From.minBufferSize;
    To.maxBufferSize = From.maxBufferSize;
    To.maxBufferUpperBound = From.maxBufferUpperBound;
    To.bufferHysteresis = From.bufferHysteresis;
    To.minCompressedBufferSize = From.minCompressedBufferSize;
    To.maxCompressedBufferSize = From.maxCompressedBufferSize;
    To.allowableDelay = From.allowableDelay;
    To.allowDroppedFrames = From.allowDroppedFrames;
    To.maxDropFramesPerCall = From.maxDropFramesPerCall;
    To.presentationLatency = From.presentationLatency;
    To.looping = (EUSVFLoop)From.looping;
    To.startDownloadWhenOpen = From.startDownloadWhenOpen;
    To.clockScale = From.clockScale;
    To.outputVertexFormat = From.outputVertexFormat;
}

void SVFHelpers::CopyFrameInfo(const SVFFrameInfo& From, FSVFFrameInfo& To) {
    To.frameId = From.frameId;
    To.frameTimestamp = FTimespan(static_cast<int64>(From.frameTimestamp));
    To.isKeyFrame = From.isKeyFrame;
    To.Bounds = FBox(FVector(From.minX, From.minY, From.minZ), FVector(From.maxX, From.maxY, From.maxZ));
    To.vertexCount = static_cast<int32>(From.vertexCount);
    To.indexCount = static_cast<int32>(From.indexCount);
    To.textureWidth = From.textureWidth;
    To.textureHeight = From.textureHeight;
    To.isEOS = From.isEOS;
    To.isRepeatedFrame = From.isRepeatedFrame;
}

bool SVFHelpers::ObtainFileInfoFromSVF(const FString& InFilePath, ComPtr<ISVFReader>& spReader, FSVFFileInfo& OutFileInfo) {
    HRESULT hr = S_OK;
    if (!spReader) {
        WarnSVF("Error in CHCapObject::ObtainFileInfoFromSVF: spReader is null");
        return false;
    }

    {
        float minX = 0.0;
        float minY = 0.0;
        float minZ = 0.0;
        float maxX = 0.0;
        float maxY = 0.0;
        float maxZ = 0.0;
        // note that the following call will wait for completion of SVF reader's Open thread
        hr = spReader->GetBBox(
            &minX, &minY, &minZ,
            &maxX, &maxY, &maxZ);
        if (S_OK == hr) {
            OutFileInfo.MaxBounds = FBox(FVector(minX, minY, minZ), FVector(maxX, maxY, maxZ));
        }
        else {
            OutFileInfo.MaxBounds.IsValid = false;
        }
    }

    ComPtr<ISVFAttributes> spReaderAttrib;
    hr = spReader.CopyTo(spReaderAttrib.GetAddressOf());
    if (FAILED(hr))
    {
        FatalSVF("Error in CHCapObject::ObtainFileInfoFromSVF: failed to obtain ISVFAttributes from reader");
        return false;
    }
    UINT32 val32 = 0L;
    UINT64 val64 = 0L;

    hr = spReaderAttrib->GetUINT32(SVFAttrib_HasAudio, &val32);
    if (S_OK == hr)
    {
        OutFileInfo.hasAudio = (val32 == 1L);
    }

    hr = spReaderAttrib->GetUINT32(SVFAttrib_FrameCount, &val32);
    if (S_OK == hr)
    {
        OutFileInfo.FrameCount = val32;
    }
    hr = spReaderAttrib->GetUINT64(SVFAttrib_Duration100ns, &val64);
    if (S_OK == hr)
    {
        OutFileInfo.Duration = FTimespan(val64);
    }
    hr = spReaderAttrib->GetUINT32(SVFAttrib_MaxVertexCount, &val32);
    if (S_OK == hr)
    {
        OutFileInfo.MaxVertexCount = val32;
    }
    hr = spReaderAttrib->GetUINT32(SVFAttrib_MaxIndexCount, &val32);
    if (S_OK == hr)
    {
        OutFileInfo.MaxIndexCount = val32;
    }
    hr = spReaderAttrib->GetUINT32(SVFAttrib_Width, &val32);
    if (S_OK == hr)
    {
        OutFileInfo.FileWidth = val32;
    }
    hr = spReaderAttrib->GetUINT32(SVFAttrib_Height, &val32);
    if (S_OK == hr)
    {
        OutFileInfo.FileHeight = val32;
    }
    if (OutFileInfo.FrameCount <= 0 && OutFileInfo.Duration.IsZero())
    {
        return false;
    }
    if (OutFileInfo.FrameCount <= 0)
    {
        OutFileInfo.FrameCount = OutFileInfo.Duration.GetTotalMilliseconds() / (1000.f / 30.f);
    }
    else if (OutFileInfo.Duration.IsZero())
    {
        OutFileInfo.Duration = ETimespan::TicksPerMillisecond * (OutFileInfo.FrameCount * (1000.f / 30.f));
    }

    double valDbl = 0.0;
    hr = spReaderAttrib->GetDouble(SVFAttrib_BitrateMbps, &valDbl);
    if (S_OK == hr)
    {
        OutFileInfo.BitrateMbps = (float)valDbl;
    }
    if (OutFileInfo.BitrateMbps == 0.0f && OutFileInfo.FrameCount > 0)
    {
        double FileSize = static_cast<double>(FPlatformFileManager::Get().GetPlatformFile().FileSize(*InFilePath));
        if (FileSize > 0)
        {
            FileSize /= (1024.0 * 1024.0);
            OutFileInfo.BitrateMbps = (float)(8.0f * 30.0 * FileSize / OutFileInfo.FrameCount);
        }
    }

    // even if we failed to collect all the attributes, do not fail the call
    return true;
}


// --------------------------------------------------------------------------
FFrameDataFromSVFBuffer::FFrameDataFromSVFBuffer(
    ComPtr<ISVFFrame>& spFrame, 
    std::shared_ptr<std::queue<HostageFrameD3D11>> const & spHostageFrames,
    bool InUseNormals/* = true*/, 
    bool InIsEOS/* = false*/)
    : bUseNormals(InUseNormals)
    , m_hostageFrames(spHostageFrames)
{
    bIsValid = false;
    if (spFrame) {
        ComPtr<ISVFBuffer> spTB;
        ComPtr<ISVFBuffer> spVB;
        ComPtr<ISVFBuffer> spIB;
        ComPtr<ISVFBuffer> spAB;
        ZeroMemory(&m_FrameInfo, sizeof(m_FrameInfo));

        // Check all buffers
        HRESULT hr = SVFFrameHelper::ExtractBuffers(spFrame, spTB, spVB, spIB, spAB);

        // Read frame info
        if (SUCCEEDED(hr) && spVB) {
            hr = SVFFrameHelper::ExtractFrameInfo(spVB, InIsEOS, m_FrameInfo);
        }
        else {
            hr = E_UNEXPECTED;
        }

        // Get texture info
        if (SUCCEEDED(hr)) {
            if (m_FrameInfo.textureHeight == 0 || m_FrameInfo.textureWidth == 0) {
                // Need read texture atributes
                hr = SVFFrameHelper::ExtractTextureInfo(spTB, m_FrameInfo);
            }
        }

        if (SUCCEEDED(hr)) {
            m_Frame = spFrame;
            bIsValid = true;
        }
    }
}

FFrameDataFromSVFBuffer::FFrameDataFromSVFBuffer(
    ComPtr<ISVFFrame>& spFrame, 
    std::shared_ptr<std::queue<HostageFrameD3D11>> const & spHostageFrames,
    const SVFFrameInfo& InFrameInfo, 
    bool InUseNormals/* = true*/)
    : m_Frame(spFrame)
    , m_FrameInfo(InFrameInfo)
    , bUseNormals(InUseNormals)
    , m_hostageFrames(spHostageFrames)
{
    bIsValid = false;
    HRESULT hr = S_OK;
    if (m_FrameInfo.textureHeight == 0 || m_FrameInfo.textureWidth == 0) {
        // Need read texture atributes
        ComPtr<ISVFBuffer> spTB;
        hr = SVFFrameHelper::ExtractTextureBuffer(m_Frame, spTB);
        if (SUCCEEDED(hr)) {
            hr = SVFFrameHelper::ExtractTextureInfo(spTB, m_FrameInfo);
        }
    }

    ComPtr<ISVFBuffer> spAB;
    hr = SVFFrameHelper::ExtractAudioBuffer(m_Frame, spAB);

    if (SUCCEEDED(hr)) {
        bIsValid = true;
    }
}

bool FFrameDataFromSVFBuffer::GetFrameInfo(FSVFFrameInfo& OutFrameInfo) {
    checkSlow(m_Frame);
    if (!m_Frame || !bIsValid) {
        return false;
    }

    SVFHelpers::CopyFrameInfo(m_FrameInfo, OutFrameInfo);

    return true;
}

bool FFrameDataFromSVFBuffer::GetFrameVerticesWithNormal(TArray<FSVFVertexNorm>& OutVertices) {
    checkSlow(m_Frame);
    if (!m_Frame || m_FrameInfo.vertexCount < 3 || !bUseNormals) {
        return false;
    }

    SCOPE_CYCLE_COUNTER(STAT_SVF_CopyVerticeBuffer);

    ComPtr<ISVFBuffer> spVB;
    HRESULT hr = S_OK;
    {
        hr = SVFFrameHelper::ExtractVerticesBuffer(m_Frame, spVB);
        if (FAILED(hr)) {
            return false;
        }
    }

    DWORD ActualSize = 0L;
    spVB->GetSize(&ActualSize);
    uint32 NeedSize = m_FrameInfo.vertexCount * sizeof(FSVFVertexNorm);
    if (ActualSize != NeedSize) {
        UE_LOG(LogTemp, Error, TEXT("ActualSize (%ld) not equal to NeedSize (%ld)"), ActualSize, NeedSize);
        return false;
    }

    if (OutVertices.Num() > 0) {
        OutVertices.Empty(m_FrameInfo.vertexCount);
    }
    OutVertices.AddUninitialized(m_FrameInfo.vertexCount);
    SVFLockedMemory lockedMem;
    ZeroMemory(&lockedMem, sizeof(lockedMem));
    hr = spVB->LockBuffer(&lockedMem);
    if (FAILED(hr)) {
        UE_LOG(LogTemp, Error, TEXT("Error in CopyBufferToArray: failed to lock SVF vertex buffer, hr = 0x%08X"), hr);
        return false;
    }
    if (ActualSize < lockedMem.Size) {
        UE_LOG(LogTemp, Error, TEXT("Error in CopyBufferToArray: vertex buffer size is %ld while current frame vertex buffer size is %ld"), ActualSize, lockedMem.Size);
        spVB->UnlockBuffer();
        return false;
    }

    auto fData = (CSVFVertex_Norm_Full const*)lockedMem.pData;
    for (uint32 i = 0; i < m_FrameInfo.vertexCount; ++i) {
        OutVertices[i].Position = FVector4(fData[i].z, fData[i].x, fData[i].y, 1.0f);
        OutVertices[i].Normal = FVector4(fData[i].nz, fData[i].nx, fData[i].ny, 0.0f);
        OutVertices[i].UV = FVector2D(fData[i].u, fData[i].v);
    }

    spVB->UnlockBuffer();
    OutVertices.Shrink();

    return true;
}

bool FFrameDataFromSVFBuffer::GetFrameVerticesWithoutNormal(TArray<FSVFVertex>& OutVertices) {
    checkSlow(m_Frame);
    if (!m_Frame || m_FrameInfo.vertexCount < 3 || bUseNormals) {
        return false;
    }

    SCOPE_CYCLE_COUNTER(STAT_SVF_CopyVerticeBuffer);

    ComPtr<ISVFBuffer> spVB;
    HRESULT hr = S_OK;
    {
        hr = SVFFrameHelper::ExtractVerticesBuffer(m_Frame, spVB);
        if (FAILED(hr)) {
            return false;
        }
    }

    DWORD ActualSize = 0L;
    spVB->GetSize(&ActualSize);
    uint32 NeedSize = m_FrameInfo.vertexCount * sizeof(FSVFVertex);
    if (ActualSize != NeedSize) {
        UE_LOG(LogTemp, Error, TEXT("ActualSize (%ld) not equal to NeedSize (%ld)"), ActualSize, NeedSize);
        return false;
    }

    if (OutVertices.Num() > 0) {
        OutVertices.Empty(m_FrameInfo.vertexCount);
    }
    OutVertices.AddUninitialized(m_FrameInfo.vertexCount);
    SVFLockedMemory lockedMem;
    ZeroMemory(&lockedMem, sizeof(lockedMem));
    hr = spVB->LockBuffer(&lockedMem);
    if (FAILED(hr)) {
        UE_LOG(LogTemp, Error, TEXT("Error in CopyBufferToArray: failed to lock SVF vertex buffer, hr = 0x%08X"), hr);
        return false;
    }
    if (ActualSize < lockedMem.Size) {
        UE_LOG(LogTemp, Error, TEXT("Error in CopyBufferToArray: vertex buffer size is %ld while current frame vertex buffer size is %ld"), ActualSize, lockedMem.Size);
        spVB->UnlockBuffer();
        return false;
    }

    auto fData = (CSVFVertex_Full const*)lockedMem.pData;
    for (uint32 i = 0; i < m_FrameInfo.vertexCount; ++i) {
        OutVertices[i].Position = FVector4(fData[i].z, fData[i].x, fData[i].y, 1.0f);
        OutVertices[i].UV = FVector2D(fData[i].u, fData[i].v);
    }

    spVB->UnlockBuffer();
    OutVertices.Shrink();

    return true;
}

bool FFrameDataFromSVFBuffer::GetFrameVertices(TArray<FDynamicMeshVertex>& OutVertices) {
    checkSlow(m_Frame);
    if (!m_Frame || m_FrameInfo.vertexCount < 3) {
        return false;
    }

    SCOPE_CYCLE_COUNTER(STAT_SVF_CopyVerticeBuffer);

    ComPtr<ISVFBuffer> spVB;
    HRESULT hr = S_OK;
    {
        hr = SVFFrameHelper::ExtractVerticesBuffer(m_Frame, spVB);
        if (FAILED(hr)) {
            return false;
        }
    }
    // if (OutVertices.Num() > 0)
    // {
    //     OutVertices.Empty(m_FrameInfo.vertexCount);
    // }
    // OutVertices.AddUninitialized(m_FrameInfo.vertexCount);
    return SUCCEEDED(SVFFrameHelper::CopyVerticesBuffer(spVB, OutVertices.GetData(), bUseNormals, m_FrameInfo.vertexCount));
}

bool FFrameDataFromSVFBuffer::GetFrameVertices(FDynamicMeshVertex* OutVertices) {
    checkSlow(m_Frame);
    if (!OutVertices || !m_Frame || m_FrameInfo.vertexCount < 3) {
        return false;
    }

    SCOPE_CYCLE_COUNTER(STAT_SVF_CopyVerticeBuffer);

    ComPtr<ISVFBuffer> spVB;
    HRESULT hr = S_OK;
    {
        hr = SVFFrameHelper::ExtractVerticesBuffer(m_Frame, spVB);
        if (FAILED(hr)) {
            return false;
        }
    }

    return SUCCEEDED(SVFFrameHelper::CopyVerticesBuffer(spVB, OutVertices, bUseNormals, m_FrameInfo.vertexCount));
}

bool FFrameDataFromSVFBuffer::GetFrameIndices(TArray<int32>& OutIndices)
{
    checkSlow(m_Frame);
    if (!m_Frame || m_FrameInfo.indexCount < 3)
    {
        return false;
    }

    SCOPE_CYCLE_COUNTER(STAT_SVF_CopyIndicesBuffer);

    ComPtr<ISVFBuffer> spIB;
    HRESULT hr = S_OK;
    {
        hr = SVFFrameHelper::ExtractIndicesBuffer(m_Frame, spIB);
        if (FAILED(hr))
        {
            return false;
        }
    }
    DWORD ActualSize = 0L;
    spIB->GetSize(&ActualSize);
    uint32 NeedSize = m_FrameInfo.indexCount * sizeof(int32);
    if (ActualSize != NeedSize)
    {
        WarnSVF("ActualSize (%ld) not equal to NeedSize (%ld)", ActualSize, NeedSize);
        return false;
    }
    if (OutIndices.Num() > 0)
    {
        OutIndices.Empty(m_FrameInfo.indexCount);
    }
    SVFLockedMemory lockedMem;
    ZeroMemory(&lockedMem, sizeof(lockedMem));
    hr = spIB->LockBuffer(&lockedMem);
    if (FAILED(hr))
    {
        WarnSVF("Error in CopyBufferToArray: failed to lock SVF vertex buffer, hr = 0x%08X", hr);
        return false;
    }
    if (ActualSize < lockedMem.Size)
    {
        WarnSVF("Error in CopyBufferToArray: vertex buffer size is %ld while current frame vertex buffer size is %ld", ActualSize, lockedMem.Size);
        spIB->UnlockBuffer();
        return false;
    }

    OutIndices.Append((int32*)lockedMem.pData, lockedMem.Size / sizeof(int32));
    spIB->UnlockBuffer();

    return true;
}

bool FFrameDataFromSVFBuffer::GetFrameIndices(int32* OutIndices) {
    checkSlow(m_Frame);
    if (!OutIndices || !m_Frame || m_FrameInfo.indexCount < 3) {
        return false;
    }

    SCOPE_CYCLE_COUNTER(STAT_SVF_CopyIndicesBuffer);

    ComPtr<ISVFBuffer> spIB;
    HRESULT hr = S_OK;
    {
        hr = SVFFrameHelper::ExtractIndicesBuffer(m_Frame, spIB);
        if (FAILED(hr)) {
            return false;
        }
    }

    return SUCCEEDED(SVFFrameHelper::CopyIndicesBuffer(spIB, OutIndices, m_FrameInfo.indexCount));
}

bool FFrameDataFromSVFBuffer::GetTextureInfo(EPixelFormat& OutPixelFormat, int32& OutWidth, int32& OutHeight) {
    checkSlow(m_Frame);
    if (!m_Frame) {
        return false;
    }

    ComPtr<ISVFBuffer> spTB;
    HRESULT hr = S_OK;
    {
        hr = SVFFrameHelper::ExtractTextureBuffer(m_Frame, spTB);
        if (FAILED(hr)) {
            return false;
        }
    }

    if (m_FrameInfo.textureHeight == 0 || m_FrameInfo.textureWidth == 0) {
        return false;
    }

    OutWidth = m_FrameInfo.textureWidth;
    OutHeight = m_FrameInfo.textureHeight;
    OutPixelFormat = EPixelFormat::PF_B8G8R8A8;

    return true;
}

bool FFrameDataFromSVFBuffer::GetTextureBuffer(uint8** OutData, int32& OutDataSize) {
    checkSlow(m_Frame);
    if (!OutData || !m_Frame) {
        return false;
    }

    SCOPE_CYCLE_COUNTER(STAT_SVF_CopyTextureBuffer);

    ComPtr<ISVFBuffer> spTB;
    HRESULT hr = S_OK;
    {
        hr = SVFFrameHelper::ExtractTextureBuffer(m_Frame, spTB);
        if (FAILED(hr)) {
            return false;
        }
    }

    if (m_FrameInfo.textureHeight < 4 || m_FrameInfo.textureWidth < 4) {
        return false;
    }

    bool bOutDummy; // Temp hack as this code path is not used
    return SUCCEEDED(SVFFrameHelper::CopyTextureBuffer(spTB, OutData, OutDataSize, bOutDummy));
}

bool FFrameDataFromSVFBuffer::CopyTextureBuffer(UTexture2D* WorkTexture, bool useHardwareTextureCopy) {
    checkSlow(m_Frame);
    checkSlow(WorkTexture);
    if (!WorkTexture || !m_Frame) {
        return false;
    }

    SCOPE_CYCLE_COUNTER(STAT_SVF_CopyTextureBuffer);

    if (m_FrameInfo.textureHeight < 4 || m_FrameInfo.textureWidth < 4) {
        return false;
    }

    if (WorkTexture->GetSizeX() != m_FrameInfo.textureWidth || WorkTexture->GetSizeY() != m_FrameInfo.textureHeight || WorkTexture->GetPixelFormat() != EPixelFormat::PF_B8G8R8A8) {
        // Need re-init texture
        return false;
    }

    ComPtr<ISVFBuffer> spTB;
    HRESULT hr = S_OK;
    {
        hr = SVFFrameHelper::ExtractTextureBuffer(m_Frame, spTB);
        if (FAILED(hr)) {
            return false;
        }
    }

#if SVF_USED3D11
    bool isD3D12 = FHardwareInfo::GetHardwareInfo(NAME_RHI).Equals("D3D12");

    ComPtr<ID3D11Texture2D> spSVFTexture2D;
    if (useHardwareTextureCopy && !isD3D12 && SUCCEEDED(spTB.CopyTo(spSVFTexture2D.GetAddressOf()))) {
        // [HW texture]
        if (!m_spDevice)
        {
            m_spDevice = reinterpret_cast<ID3D11Device*>(GDynamicRHI->RHIGetNativeDevice());
        }
        HANDLE sharedHandle = INVALID_HANDLE_VALUE;
        ComPtr<IDXGIResource> spSVFDXGIResource;
        hr = spSVFTexture2D->QueryInterface(__uuidof(IDXGIResource), (void**)&spSVFDXGIResource);
        if (FAILED(hr)) {
            FatalSVF("Error in USVFReaderPassThrough::CopyHWTexture: cannot QI IDXGIResource from SVF's ID3D11Texture2D");
            return false;
        }
        hr = spSVFDXGIResource->GetSharedHandle(&sharedHandle);
        if (FAILED(hr)) {
            FatalSVF("Error in USVFReaderPassThrough::CopyHWTexture: cannot get shared handle from SVF's ID3D11Texture2D, hr = 0x%08X", hr);
            return false;
        }
        ComPtr<ID3D11Texture2D> spSharedTexture;
        hr = m_spDevice->OpenSharedResource(sharedHandle, __uuidof(ID3D11Texture2D), (void**)&spSharedTexture);
        if (FAILED(hr)) {
            FatalSVF("Error in USVFReaderPassThrough::CopyHWTexture: failed in OpenSharedResource, hr = 0x%08X", hr);
            return false;
        }
        D3D11_TEXTURE2D_DESC descSharedTexture;
        spSharedTexture->GetDesc(&descSharedTexture);

        m_FrameInfo.textureWidth = (int)descSharedTexture.Width;
        m_FrameInfo.textureHeight = (int)descSharedTexture.Height;

        // Check texture format
        if (descSharedTexture.Format != DXGI_FORMAT_B8G8R8A8_TYPELESS && descSharedTexture.Format != DXGI_FORMAT_B8G8R8A8_UNORM && descSharedTexture.Format != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) {
            return false;
        }

        WorkTexture->SRGB = 1;// descSharedTexture.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        if (!WorkTexture->Resource) {
            WorkTexture->UpdateResource();
        }

        struct FCopySVFTextureData {
            FTexture2DResource* Texture2DResource;
            ComPtr<ID3D11Texture2D> spSharedTexture;
            ComPtr<ID3D11Device> spDevice;
            ComPtr<ISVFFrame> spFrame;
            std::shared_ptr<std::queue<HostageFrameD3D11>> spHostageFrames;
        };

        FCopySVFTextureData* UpdateData = new FCopySVFTextureData;
        UpdateData->spSharedTexture = spSharedTexture;
        UpdateData->Texture2DResource = (FTexture2DResource*)WorkTexture->Resource;
        UpdateData->spDevice = m_spDevice;
        UpdateData->spFrame = m_Frame;
        UpdateData->spHostageFrames = m_hostageFrames;

        ENQUEUE_RENDER_COMMAND(CopySVFTextureData) (
            [=](FRHICommandListImmediate& RHICmdList)
            {
                ComPtr<ID3D11DeviceContext> spCtx;
                UpdateData->spDevice->GetImmediateContext(&spCtx);

                auto hostageFrames = UpdateData->spHostageFrames;

                if (!hostageFrames->empty())
                {
                    BOOL finished = FALSE;
                    if (SUCCEEDED(spCtx->GetData(hostageFrames->front().query.Get(), &finished, sizeof(finished), 0)) && finished)
                    {
                        hostageFrames->pop();
                    }
                }
                
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 26
                FD3D11Texture2D* DestTexture2D = static_cast<FD3D11Texture2D*>(UpdateData->Texture2DResource->GetTexture2DRHI());
#else
                FD3D11Texture2D* DestTexture2D = static_cast<FD3D11Texture2D*>(UpdateData->Texture2DResource->GetTexture2DRHI().GetReference());
#endif
                spCtx->CopyResource(DestTexture2D->GetResource(), UpdateData->spSharedTexture.Get());

                D3D11_QUERY_DESC queryDesc = {};
                queryDesc.Query = D3D11_QUERY_EVENT;
                ComPtr<ID3D11Query> query;
                UpdateData->spDevice->CreateQuery(&queryDesc, &query);
                spCtx->End(query.Get());

                hostageFrames->push({ UpdateData->spFrame, spSharedTexture, query });

                delete UpdateData;
            }
        );

        return true;
    }
    else
#endif
        if (WorkTexture->Resource) {
            struct FUpdateTextureRegionData {
                FTexture2DResource* Texture2DResource = nullptr;
                int32 MipIndex = 0;
                uint8* SrcData = nullptr;
                int32 DataSize = 0;
                FUpdateTextureRegion2D Region;
                uint32 SrcPitch;
                uint32 SrcBpp;
                bool bDeleteSrc = false;

                ~FUpdateTextureRegionData() {
                    if (bDeleteSrc)
                    {
                        FMemory::Free(SrcData);
                        bDeleteSrc = false;
                    }
                }
            };

            FUpdateTextureRegionData* RegionData = new FUpdateTextureRegionData;
            static uint32 DataSize = 0;
            const uint32 MaxTextureSize = WorkTexture->Resource->GetSizeX() * WorkTexture->Resource->GetSizeY() * 4; // 4 bytes as it is 32-bit
            static FSVFRingBuffer* RingBuffer = nullptr;
            if (DataSize == 0)
            {
                int32 Size = 0;
                FString SectionBlock = GIsEditor ? TEXT("SVFSettings_Editor") : TEXT("SVFSettings");
                GConfig->GetInt(*SectionBlock, TEXT("TextureRingBufferSizeMB"), Size, GEngineIni);
                DataSize = (uint32)Size * 1024 * 1024;
                RingBuffer = new FSVFRingBuffer(DataSize);
            }

            //static uint8* SrcData = (uint8*)FMemory::Malloc(DataSize*2);
            //static uint32 index = 0;
            //if (!RegionData->SrcData)
            {
                //RegionData->DataSize = DataSize;
                //RegionData->SrcData = (SrcData + DataSize * (index++ % 2));// (uint8*)FMemory::Malloc(RegionData->DataSize);
                RingBuffer->ConditionalReset(GFrameNumber);
                if (RingBuffer->CanAllocate(MaxTextureSize))
                {
                    RegionData->SrcData = RingBuffer->Allocate(MaxTextureSize);
                }
                else
                {
                    RegionData->SrcData = nullptr;
                }
            }
            if (SUCCEEDED(SVFFrameHelper::CopyTextureBuffer(spTB, &RegionData->SrcData, RegionData->DataSize, RegionData->bDeleteSrc))) {
                RegionData->Texture2DResource = (FTexture2DResource*)WorkTexture->Resource;

                RegionData->Region = FUpdateTextureRegion2D(0, 0, 0, 0, WorkTexture->GetSizeX(), WorkTexture->GetSizeY());
                RegionData->SrcBpp = RegionData->DataSize / (WorkTexture->GetSizeX() * WorkTexture->GetSizeY());
                RegionData->SrcPitch = RegionData->SrcBpp * WorkTexture->GetSizeX();

                ENQUEUE_RENDER_COMMAND(UpdateTextureData)(
                [=](FRHICommandListImmediate& RHICmdList)
                    {
                        int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
                        if (RegionData->Texture2DResource && RegionData->Texture2DResource->GetTexture2DRHI() && RegionData->MipIndex >= CurrentFirstMip) {
                            RHIUpdateTexture2D(
                                RegionData->Texture2DResource->GetTexture2DRHI(),
                                RegionData->MipIndex - CurrentFirstMip,
                                RegionData->Region,
                                RegionData->SrcPitch,
                                RegionData->SrcData
                            );
                        }
                        delete RegionData;
                    }
                );

                return true;
            }
            else {
                delete RegionData;
            }
        }

    return false;
}




#undef TRUE
#undef FALSE

#undef LogSVF
#undef WarnSVF
#undef FatalSVF

#endif
