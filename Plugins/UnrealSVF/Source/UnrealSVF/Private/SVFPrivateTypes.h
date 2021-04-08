// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DynamicMeshBuilder.h"
#include "HardwareInfo.h"

#include "PreSVFAPI.h"
#if PLATFORM_WINDOWS
#include "Windows.h"
#include "SVF.h"
#include <wrl/client.h>
using namespace Microsoft::WRL;
THIRD_PARTY_INCLUDES_START
#include <mfidl.h>
THIRD_PARTY_INCLUDES_END
#endif
#include "PostSVFAPI.h"

#include "SVFTypes.h"
#include "SVFClockInterface.h"
#include "SVFCallbackInterface.h"

#include "exports/SVFPluginExport.h"

//#include "SVFPrivateTypes.generated.h"

// Forward declaration
class ISVFClockInterface;
class ISVFCallbackInterface;

#include <memory>
#include <queue>

struct HostageFrameD3D11
{
    // hold onto decoder output frame until copy to UE4 engine texture is complete
    ComPtr<ISVFFrame> frame;
    // hold onto shared texture because releasing it while it's still referenced in gfx pipeline will (supposedly) cause a stall
    ComPtr<ID3D11Texture2D> sharedTexture;
    // query to check when copy to engine texture is complete
    ComPtr<ID3D11Query> query; 
};

// ISVFReaderStatisticsCallback is used to communicate internal SVF reader status from frame cache to reader wrapper
interface DECLSPEC_UUID("95724409-D6C3-4200-A063-D816934EDCC5")ISVFReaderStatisticsCallback : public IUnknown
{
    STDMETHOD_(void, UpdateSVFStatistics) (UINT32 unsuccessfulReadFrameCount, UINT32 lastReadFrame, UINT32 droppedFrameCount) = 0;
};

// Wrapper for callback notify
class USVFReaderCallbackUseUnrealInterface : public ISVFReaderStateCallback, public ISVFReaderInternalStateCallback, public ISVFReaderStatisticsCallback {
public:

    // IUnknown
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    STDMETHODIMP QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override;

    HRESULT Initialize(UObject* InUnrealObject);

    // ISVFReaderStateCallback
    virtual HRESULT OnStateChange(ESVFReaderState oldState, ESVFReaderState newState) override;

    // ISVFReaderInternalStateCallback
    virtual void OnError(HRESULT hrError) override;
    virtual void OnEndOfStream(UINT32 remainingVideoSampleCount) override;

    // ISVFReaderStatisticsCallback
    STDMETHOD_(void, UpdateSVFStatistics) (UINT32 unsuccessfulReadFrameCount, UINT32 lastReadFrame, UINT32 droppedFrameCount) override;

    ISVFCallbackInterface* GetInterfacePtr() {
        return UnrealCallback.IsValid() ? InterfacePtr : nullptr;
    }

protected:

    PTRINT m_cRef;

    TWeakObjectPtr<UObject> UnrealCallback;
    ISVFCallbackInterface* InterfacePtr = nullptr;

};


// SCFClock wrapper for use UObjects that implement ISVFClockInterface
class USVFClockUseUnrealInterface : public ISVFClock {
public:
    USVFClockUseUnrealInterface();
    virtual ~USVFClockUseUnrealInterface();

    HRESULT Initialize(UObject* InUnrealObject);

    // IUnknown
    virtual ULONG STDMETHODCALLTYPE AddRef() override;
    virtual ULONG STDMETHODCALLTYPE Release() override;
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override;

    // ISVFClock
    virtual HRESULT GetPresentationTime(_Out_ SVFTIME *pTime) override;
    virtual HRESULT GetTime(_Out_ SVFTIME *pTime) override;
    virtual HRESULT Start() override;
    virtual HRESULT Stop() override;
    virtual HRESULT GetState(_Out_ ESVFClockState *pState) override;
    virtual HRESULT Shutdown() override;
    virtual HRESULT SetPresentationOffset(SVFTIME t) override;
    virtual HRESULT SnapPresentationOffset() override;
    virtual HRESULT GetPresentationOffset(_Out_ SVFTIME *pTime) override;
    virtual HRESULT SetScale(float scale) override;
    virtual HRESULT GetScale(_Out_ float *pScale) override;

    ISVFClockInterface* GetInterfacePtr() {
        return UnrealClock.IsValid() ? InterfacePtr : nullptr;
    }

protected:
    PTRINT m_RefCnt;
    PTRINT m_DestructionFlag;

    TWeakObjectPtr<UObject> UnrealClock;
    ISVFClockInterface* InterfacePtr = nullptr;

};

// SCFClock
class USVFClockUseSystemInterface : public ISVFClock {
public:
    USVFClockUseSystemInterface();
    virtual ~USVFClockUseSystemInterface();

    HRESULT Initialize();

    // IUnknown
    virtual ULONG STDMETHODCALLTYPE AddRef() override;
    virtual ULONG STDMETHODCALLTYPE Release() override;
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override;

    // ISVFClock
    virtual HRESULT GetPresentationTime(_Out_ SVFTIME *pTime) override;
    virtual HRESULT GetTime(_Out_ SVFTIME *pTime) override;
    virtual HRESULT Start() override;
    virtual HRESULT Stop() override;
    virtual HRESULT GetState(_Out_ ESVFClockState *pState) override;
    virtual HRESULT Shutdown() override;
    virtual HRESULT SetPresentationOffset(SVFTIME t) override;
    virtual HRESULT SnapPresentationOffset() override;
    virtual HRESULT GetPresentationOffset(_Out_ SVFTIME *pTime) override;
    virtual HRESULT SetScale(float scale) override;
    virtual HRESULT GetScale(_Out_ float *pScale) override;


protected:
    PTRINT m_RefCnt;
    PTRINT m_DestructionFlag;

    ComPtr<IMFPresentationClock> m_spPresentationClock;
    SVFTIME m_PresentationOffset;
    SVFTIME m_ClockOffset;
    float m_Scale;
    ESVFClockState m_ClockState;
};


class FSVFRingBuffer
{
private:
    uint8* Data;
    uint64 Capacity;
    uint64 Offset;
    uint32 CachedFrameNumber;

public:
    FSVFRingBuffer();
    FSVFRingBuffer(uint64 AllocationSize);
    ~FSVFRingBuffer();

    uint8* Allocate(uint64 Size);

    FORCEINLINE void ConditionalReset(uint32 FrameNumber)
    {
        if (CachedFrameNumber != FrameNumber)
        {
            CachedFrameNumber = FrameNumber;
            Offset = 0;
        }
    }

    FORCEINLINE bool CanAllocate(uint64 Size) const
    {
        return Offset + Size < Capacity;
    }
};


struct FFrameDataFromSVFBuffer : public FFrameData {

    FFrameDataFromSVFBuffer(
        ComPtr<ISVFFrame>& spFrame,
        std::shared_ptr<std::queue<HostageFrameD3D11>> const & spHostageFrames,
        bool InUseNormals = true, 
        bool InIsEOS = false);

    FFrameDataFromSVFBuffer(
        ComPtr<ISVFFrame>& spFrame,
        std::shared_ptr<std::queue<HostageFrameD3D11>> const & spHostageFrames,
        const SVFFrameInfo& InFrameInfo,
        bool InUseNormals = true);

#if SVF_USED3D11
    ComPtr<ID3D11Device> m_spDevice;
#endif

    // Hold Frame for rendering
    ComPtr<ISVFFrame> m_Frame;
    // Hold Frame info
    SVFFrameInfo m_FrameInfo;
    bool bUseNormals;

    std::shared_ptr<std::queue<HostageFrameD3D11>> m_hostageFrames;

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


// global functions for SVF frame
namespace SVFFrameHelper {
    HRESULT ExtractBuffers(ComPtr<ISVFFrame>& spFrame, ComPtr<ISVFBuffer>& spTextureBuffer, ComPtr<ISVFBuffer>& spVertexBuffer, ComPtr<ISVFBuffer>& spIndexBuffer, ComPtr<ISVFBuffer>& spAudioBuffer);
    HRESULT ExtractVerticesBuffer(ComPtr<ISVFFrame>& spFrame, ComPtr<ISVFBuffer>& spVertexBuffer);
    HRESULT ExtractIndicesBuffer(ComPtr<ISVFFrame>& spFrame, ComPtr<ISVFBuffer>& spIndexBuffer);
    HRESULT ExtractTextureBuffer(ComPtr<ISVFFrame>& spFrame, ComPtr<ISVFBuffer>& spTextureBuffer);
    HRESULT ExtractFrameInfo(ComPtr<ISVFBuffer>& spVertexBuffer, bool isEOS, SVFFrameInfo& frameInfo);
    HRESULT ExtractTextureInfo(ComPtr<ISVFBuffer>& spBuffer, SVFFrameInfo& frameInfo);
    HRESULT ExtractAudioBuffer(ComPtr<ISVFFrame>& spFrame, ComPtr<ISVFBuffer>& spAudioBuffer);

    void UpdateSVFStatus(_In_ ISVFReader* pReader, _In_ ISVFFrame* pFrame, FSVFStatus& status);

    HRESULT CopyVerticesBuffer(ComPtr<ISVFBuffer>& spVertexBuffer, struct FDynamicMeshVertex* OutVertices, bool bUseNormal, uint32 VertexCount);
    HRESULT CopyIndicesBuffer(ComPtr<ISVFBuffer>& spIndexBuffer, int32* OutIndices, uint32 IndicesCount);
    HRESULT CopyTextureBuffer(ComPtr<ISVFBuffer>& spTextureBuffer, TArray<uint8>& OutData);
    HRESULT CopyTextureBuffer(ComPtr<ISVFBuffer>& spTextureBuffer, uint8** OutData, int32& OutDataSize, bool& bOutCalleeShouldFreeMemory);
};

namespace SVFHelpers {
    void CopyConfig(const SVFConfiguration& From, FSVFConfiguration& To);
    void CopyFrameInfo(const SVFFrameInfo& From, FSVFFrameInfo& To);

    bool ObtainFileInfoFromSVF(const FString& InFilePath, ComPtr<ISVFReader>& spReader, FSVFFileInfo& OutFileInfo);

};


