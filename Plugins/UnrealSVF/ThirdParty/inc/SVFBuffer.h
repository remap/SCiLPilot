//---------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Abstract:
//      Defines our buffer class (buffers are either textures, vertex buffers,
//      index buffers, or audio buffers). 
//
//---------------------------------------------------------------------------
#pragma once
#include "SVFRefCount.h"

namespace SVF
{
    //**********************************************************************
    // Define base class for our buffers we hand back to client
    //**********************************************************************
    class CSVFBufferBase :
        public ISVFBuffer,
        public CSVFRefCount,
        public CSVFAttributesBase
    {
    public:
        CSVFBufferBase();
        virtual ~CSVFBufferBase();

        //**********************************************************************
        // IUnknown
        //**********************************************************************
        virtual ULONG STDMETHODCALLTYPE AddRef() override;
        virtual ULONG STDMETHODCALLTYPE Release() override;
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override;
    };

    //**********************************************************************
    // Define class for handling of D3D textures where texture continues to
    // live inside the ID3D11Texture2D returned by MF
    //**********************************************************************
    class CSVFBufferD3D : public CSVFBufferBase
    {
    public:
        CSVFBufferD3D();
        virtual ~CSVFBufferD3D();
        HRESULT Initialize(_In_ ID3D11Texture2D *pTexture, _In_ IMFDXGIDeviceManager *pDevMgr, _In_ IMFSample *pSample);

        //**********************************************************************
        // IUnknown
        //**********************************************************************
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override;

        //**********************************************************************
        // ISVFBuffer
        //**********************************************************************
        virtual HRESULT GetSize(_Out_ DWORD *pSize) override;
        virtual HRESULT LockBuffer(_In_ SVFLockedMemory *pLockedMemory) override;
        virtual HRESULT UnlockBuffer() override;

    private:
        CComPtr<ID3D11Texture2D> m_spTexture;
        CComPtr<IMFSample> m_spSample;
        CComPtr<IMFDXGIDeviceManager> m_spDevMgr;
        D3D11_MAPPED_SUBRESOURCE m_ms;
    };

    //---------------------------------------------------------------------------
    //
    //  CSVFBuffer defines a single buffer. This could be a single frame's texture,
    //  audio sample, vertex or index buffer.
    //
    //---------------------------------------------------------------------------
    class CSVFBuffer : public CSVFBufferBase
    {
    public:
        CSVFBuffer();
        virtual ~CSVFBuffer();

        HRESULT Initialize(DWORD size, DWORD sride, DWORD chromaOffset);

        //**********************************************************************
        // ISVFBuffer
        //**********************************************************************
        virtual HRESULT GetSize(_Out_ DWORD *pSize) override;
        virtual HRESULT LockBuffer(_In_ SVFLockedMemory *pLockedMemory) override;
        virtual HRESULT UnlockBuffer() override;

    private:
        DWORD m_size;
        DWORD m_chromaOffset;
        DWORD m_stride;
        std::unique_ptr<BYTE[]> m_pData;
    };
}
