//---------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//---------------------------------------------------------------------------

/*! \file
* \mainpage SVF (Spatial Video Foundation)
*
* \section intro_sec Introduction
*
* SVF.DLL is the primary library for decoding HoloCapture videos (HoloVideo).
* The API is exposed through \ref ISVFReader (see \ref SVFCreateReader).
*
* \section usage Usage
*
* The typical usage of SVF follows the following pattern:
* \code
*        CreateReader
*        Open URL to HoloVideo
*        while (!end of stream)
*             Get HoloVideo frame
*             Render Mesh with Texture
*        Close HoloVideo
*        Shutdown Reader
* \endcode
*
* Note that the API is synchronous so this loop would typically be run on its own thread.
*
* Below is a simple example of using SVF:
* \include "Sample.cpp"
*/
#pragma once

#define SVF_RELEASE_VER_MAJOR 1
#define SVF_RELEASE_VER_MINOR 4
#define SVF_RELEASE_VER_PATCH 8
#define SVF_RELEASE_VER_BUILD 0

#ifndef SVF_VERSION_ONLY

#include <Windows.h>
#include <basetyps.h>
#include <d3d11.h>
#include <propsys.h>

#ifndef TFS_TREE
#define SVFEXPORT WINAPI
#else
#define SVFEXPORT
#endif

#define MAX_TIMEOUT 15000       // Never wait for longer than this (we don't want deadlocks if something bad happens)

//**********************************************************************
// Define helpers for converting from SVFTIME to milliseconds
//**********************************************************************
typedef LONGLONG SVFTIME;
const SVFTIME ONE_SECOND = 10000000; // One second.
const SVFTIME   ONE_MSEC = 1000;       // One millisecond

                                       //! Converts 100-nanosecond units to milliseconds.
inline SVFTIME SVFTimeToMsec(_In_ const SVFTIME &time)
{
    return static_cast<SVFTIME>(time / (ONE_SECOND / ONE_MSEC));
}

//! Converts milliseconds to 100-nanosecond units.
inline SVFTIME MsecToSVFTime(_In_ const SVFTIME &time)
{
    return static_cast<SVFTIME>(time * (ONE_SECOND / ONE_MSEC));
}

#include "SVFCore.h"

typedef DWORD CSVFVertexIndex; //!< Define format for our index data (32b/index)

                               //**********************************************************************
                               //! \brief ESVFStreamType defines our various streams
                               //!
                               //! SVF exposes buffers of data. Each buffer of data is associated with
                               //! a type of stream. This enumeration defines those types.
                               //!
                               //**********************************************************************
enum ESVFStreamType
{
    SVFUnknown = 0,         //!< Unknown stream type (shouldn't happen)
    SVFDecodedTexture = 1,  //!< Stream contains texture atlas data in NV12 format
    SVFVertexBuffer = 2,    //!< Stream contains our vertex data
    SVFIndexBuffer = 3,     //!< Stream contains our index data
    SVFAudio = 4,           //!< Stream contains our audio data
    SVFRGBTexture = 5,      //!< Stream contains texture atlas data in RGB format
};

//**********************************************************************
//! \brief ESVFLoop defines the various ways we can loop playback
//! when we reach the end of the stream.
//**********************************************************************
enum class ESVFLoop
{
    LoopViaRewind,  //!< Currently unsupported
    LoopViaRestart, //!< Forces playback to restart from beginning
    NoLooping,      //!< Forces playback to stop at end of stream
};

//**********************************************************************
//! \brief ISVFAttributes defines how to retrieve SVF attributes
//!
//! SVF exposes attributes at 3 levels:<br>
//! <ul>
//!    <li>Video Clip Level<ul><li>
//! Attributes related to the entire video can be read by QIing
//! for ISVFAttributes from the \ref ISVFReader</li></ul></li>
//!    <li>Frame Level
//! <ul><li>Attributes related to the entire video can be read by QIing
//! for ISVFAttributes from the \ref ISVFFrame</li></ul></li>
//!    <li>Buffer Level
//! <ul><li>Attributes related to the entire video can be read by QIing
//! for ISVFAttributes from the \ref ISVFBuffer</li></ul></li>
//! </ul>
//!
//! See \ref SVFGuids.h for a complete list of attributes that are exposed
//**********************************************************************
interface DECLSPEC_UUID("3D9929C3-F9F1-460A-BFB7-143B1C03906F") ISVFAttributes : public IUnknown
{
    virtual HRESULT OnGetAttribute() = 0;  //!< Ensures that our attributes have been read (must be called before trying to obtain an attribute). NOTE: Generally clients never should call this method.

    virtual HRESULT GetUINT32(_In_ REFGUID guidKey, _Out_ UINT32 *punValue) = 0; //!< Retrieves the specified UINT32 attribute
    virtual HRESULT GetUINT64(_In_ REFGUID guidKey, _Out_ UINT64 *punValue) = 0; //!< Retrieves the specified UINT64 attribute
    virtual HRESULT GetDouble(_In_ REFGUID guidKey, _Out_ double *pfValue) = 0; //!< Retrieves the specified double attribute
    virtual HRESULT GetString(_In_ REFGUID guidKey, _Out_ LPWSTR pbstrVal, UINT32 cchBufSize, UINT32 *pcchLength) = 0; //!< Retrieves the specified string attribute
    virtual HRESULT GetStringLength(_In_ REFGUID guidKey, _Out_ UINT32  *pcchLength) = 0; //!< Retrieves the length of the specified string attribute
    virtual HRESULT SetUINT32(_In_ REFGUID guidKey, _In_ UINT32 unValue) = 0; //!< Sets the specified UINT32 attribute
    virtual HRESULT SetUINT64(_In_ REFGUID guidKey, _In_ UINT64 unValue) = 0; //!< Sets the specified UINT64  attribute
    virtual HRESULT SetDouble(_In_ REFGUID guidKey, _In_ double fValue) = 0; //!< Sets the specified double attribute
    virtual HRESULT SetString(_In_ REFGUID guidKey, _In_z_ const wchar_t* pbstrVal) = 0; //!< Sets the specified string attribute

    virtual HRESULT Serialize(_In_z_ const wchar_t* pPath) = 0;  //!< Saves all attributes to file
    virtual HRESULT FillFromFile(_In_z_ const wchar_t* pPath) = 0; //!< Adds/overwrites attributes from file
    virtual HRESULT FillFromPropertyStore(_In_ IPropertyStore *pPropStore) = 0; //!< Adds/overwrites attributes from IPropertyStore
    virtual HRESULT CopyAll(_In_ ISVFAttributes* pOtherAttr) = 0; //!< Copy all attributes 
};

//**********************************************************************
//! \brief ISVFFrame defines a single HoloVideo frame
//!
//! ISVFFrame represents a single frame (texture and mesh) from a HoloVideo
//**********************************************************************
interface DECLSPEC_UUID("0D0D58A7-5D78-4B43-B577-AD45CC3FE38C") ISVFFrame : public IUnknown
{
    virtual HRESULT GetTimestamp(_Out_ LONGLONG *pllTimestamp) = 0; //!< Returns the timestamp associated with this frame
    virtual HRESULT GetBufferCount(_Out_ ULONG *pulCount) = 0; //!< Returns the total number of buffers in this frame (typically 3)
    virtual HRESULT GetBuffer(ULONG index, _Outptr_opt_result_maybenull_ ISVFBuffer **ppBuffer,
        _Out_ ESVFStreamType *pAssociatedStreamType) = 0; //!< Returns the specified buffer and its type
    virtual int GetFrameID() = 0; //!< Returns the current frame index
};

//**********************************************************************
//! \brief ISVFEvent defines a single event from the SVF event queue
//!
//! ISVFEvent represents a single event when using the SVF event queue. See \ref ISVFEventQueue for details.
//! SVFEvents are obsolete. 
//**********************************************************************
interface DECLSPEC_UUID("F18449E9-1BBF-498A-93CD-AD5740DAB832") ISVFEvent : public IUnknown
{
    virtual HRESULT GetEventID(_Out_ DWORD *pID) = 0; //!< Returns a unique ID associated with the event
    virtual HRESULT GetEventData(_Outptr_ void **ppData) = 0; //!< Returns the data associated with the event
    virtual HRESULT SetEventID(_In_ DWORD ID) = 0; //!< Sets the ID associated with the event
    virtual HRESULT SetEventData(_In_ void *pData) = 0; //!< Sets the data associated with the event
    virtual HRESULT WaitForFinish() = 0; //!< Waits until an event has finished executing
    virtual HRESULT Finish() = 0; //!< Sets the event to its finished state
};

//**********************************************************************
//! \brief SVFCreateEvent creates a new event object
//!
//! SVFEvents are obsolete. 
//**********************************************************************
extern "C" HRESULT SVFEXPORT SVFCreateEvent(_Outptr_ ISVFEvent **ppEvent);

//**********************************************************************
// ISVFEventQueue is used for posting events across threads
//**********************************************************************
interface DECLSPEC_UUID("5D462C0E-D90A-40C4-9881-FBD60A570907") ISVFEventQueue : public IUnknown
{
    virtual HRESULT PostEvent(_In_ ISVFEvent *pEvent) = 0;
    virtual HRESULT PostEventAndWait(_In_ ISVFEvent *pEvent) = 0;
    virtual HRESULT GetNextEvent(_Outptr_ ISVFEvent **ppEvent) = 0;
    virtual HRESULT FlushEvents() = 0;
};

//**********************************************************************
//! \brief SVFCreateEventQueue creates an event queue
//!
//! SVFEvents are obsolete. 
//**********************************************************************
extern "C" HRESULT SVFEXPORT SVFCreateEventQueue(_Outptr_ ISVFEventQueue **ppEventQueue);

//**********************************************************************
//! \brief ESVFClockState returns the state of our current clock
//**********************************************************************
enum ESVFClockState
{
    Running, //!< Specifies that the clock is running
    Stopped //!< Specifies that the clock has stopped
};

//**********************************************************************
//! \brief ISVFClock is the clock that drives presentation of our video frames
//!
//! The SVF playback is driven by a clock (see \ref SVFReader::GetNextFrameViaClock)
//! Typically, the clock used by SVF is internal and driven off the audio
//! playback. However, it is possible for clients to specify and control
//! their own clock. This interface implements that clock.
//**********************************************************************
interface DECLSPEC_UUID("336F6F29-F9F4-442A-BCD1-82DAA6A2644E") ISVFClock : public IUnknown
{
    virtual HRESULT GetPresentationTime(_Out_ SVFTIME *pTime) = 0; //!< Returns the current presentation time (Time-PresentationOffset)
    virtual HRESULT GetTime(_Out_ SVFTIME *pTime) = 0; //!< Returns the clocks underlying time
    virtual HRESULT Start() = 0; //!< Starts the clock
    virtual HRESULT Stop() = 0; //!< Stops the clock
    virtual HRESULT GetState(_Out_ ESVFClockState *pState) = 0; //!< Returns the clocks current state
    virtual HRESULT Shutdown() = 0; //!< Shutdown the clock
    virtual HRESULT SetPresentationOffset(SVFTIME t) = 0; //!< Sets the presentation offset. This value is added to the clocks time
    virtual HRESULT SnapPresentationOffset() = 0; //!< Sets the presentation offset to the clocks current time. This basically resets the clocks relative time back to 0.
    virtual HRESULT GetPresentationOffset(_Out_ SVFTIME *pTime) = 0; //!< Returns the current presentation offset
    virtual HRESULT SetScale(float scale) = 0; //!< Sets the clock scaling factor (making the clock run faster or slower)
    virtual HRESULT GetScale(_Out_ float *pScale) = 0; //!< Returns the clock's current scaling factor
};

//**********************************************************************
//! \brief ISVFStream represents a single stream in our spatial video.
//!
//! Each HoloVideo is logically comprised of different types of streams.
//! There is generally a stream of data that is the mesh data, a stream
//! for the audio data, and a stream for the texture data. 
//**********************************************************************
interface DECLSPEC_UUID("6CB4BA0A-FE21-4152-A006-B0B9B291BE2D")  ISVFStream : public IUnknown
{
    virtual HRESULT GetLODCount(_Out_ ULONG *pLODCount) = 0; //!< This feature is not yet supported.
    virtual HRESULT SetStreamState(bool enabled, ULONG lod) = 0; //!< This feature is not yet supported.
    virtual HRESULT GetStreamType(_Out_ ESVFStreamType *pStreamType) = 0; //!< Returns the type of stream
};

//**********************************************************************
//! \brief SVFCreateBufferAllocator creates a new buffer allocator object
//!
//! SVFCreateBufferAllocator creates a new buffer allocator object. This object
//! is then passed to \ref SVFCreateReader.
//**********************************************************************
extern "C" HRESULT SVFEXPORT SVFCreateBufferAllocator(_Outptr_ ISVFBufferAllocator **ppTextureAllocator);

const int c_SVFDisableDropFrames = -1;

//**********************************************************************
//! \brief ESVFReaderState defines the various states the \ref ISVFReader may be in
//**********************************************************************
enum class ESVFReaderState
{
    Unknown,        //!< Reader is in unknown state
    Initialized,    //!< Reader was properly initialized
    OpenPending,    //!< Open is pending
    Opened,         //!< Open finished
    Prerolling,     //!< File opened and pre-roll started
    Ready,          //!< Frames ready to be delivered
    Buffering,      //!< Reader is buffering frames (i.e. frames not available for delivery)
    Closing,        //!< File being closed
    Closed,         //!< Filed closed
    EndOfStream,    //!< Reached end of current file
    ShuttingDown,   //!< SVFReader is in process of shutting down
};

//**********************************************************************
//! \brief ISVFReaderStateCallback is the callback for notifying the current
//! state of the reader.
//!
//! If a client wants to receive notification of when the SVFReader transitions
//! its state (see \ref ESVFReaderState), the client should register a callback
//! implementing this interface via \ref ISVFReader::SetNotifyState.
//**********************************************************************
struct DECLSPEC_UUID("81BC2EC9-59CC-410B-BDBE-2B3016BA6949") ISVFReaderStateCallback : public IUnknown
{
    virtual HRESULT OnStateChange(ESVFReaderState oldState, ESVFReaderState newState) = 0; //!< Method called when reader changes state
};

//**********************************************************************
//! \brief ISVFReaderInternalStateCallback is the callback for notifying internal reader states 
//! of the reader. Client is supposed to close current reader and  cease any further communicatio
//!
//! If a client wants to receive notification of when the SVFReader encounters error, or EOS in the reading thread,
//! the client should register a callback
//! implementing this interface via \ref ISVFReader::SetNotifyInternalState.
//**********************************************************************
struct DECLSPEC_UUID("BAA39C96-5F8F-4550-82BF-ACEB095D13C2") ISVFReaderInternalStateCallback : public IUnknown
{
    virtual void OnError(HRESULT hrError) = 0; //!< Method called when reader encounters internal error
    virtual void OnEndOfStream(UINT32 remainingVideoSampleCount) = 0;
};

//**********************************************************************
//! \brief ISVFDashServer allows for retrieval of files from the DASH server
//!
//! This interface is obsolete
//**********************************************************************
interface DECLSPEC_UUID("2620C528-82CB-450A-8C0F-50A8DF283960") ISVFDashServer : public IUnknown
{
    virtual HRESULT GetFileCount(_Out_ UINT32 *pCount) = 0; //!< Returns number of available files from dash server
    virtual HRESULT GetFileURL(UINT32 index, _Outptr_ BSTR *pURL) = 0; //!< Returns the URL to the Nth file
    virtual HRESULT GetShortName(UINT32 index, _Outptr_ BSTR *pShortName) = 0; //!< Returns the short name for the Nth file
};

//**********************************************************************
//! This interface is obsolete
//**********************************************************************
extern "C" HRESULT SVFEXPORT SVFCreateDashServer(_In_z_ const wchar_t *pWebServer, _Out_ ISVFDashServer **ppDash);

//**********************************************************************
// Define our default buffer sizes. These are only used as starting values.
// These can grow based on playback results.
//**********************************************************************
const DWORD c_PrerollSize = 10; //!< Number of frames to preroll
const DWORD c_MinBufferSize = 45; //!< Minimum number of samples that must be in queue. Less than this and we will buffer.
const DWORD c_BufferHysteresis = c_MinBufferSize + (c_MinBufferSize + 9) / 10; //!< Number of samples past min size when we should restart playback (we don't want to bounce between buffering/playing state)
const DWORD c_MaxBufferSize = 90; //!< Never buffer more than 3 seconds
const DWORD c_MaxBufferUpperBound = 600; //!< Don't ever let our max buffer grow beyond this

                                         //**********************************************************************
                                         // Define compressed buffer size constants
                                         //**********************************************************************
const DWORD c_MinCompressedBufferSize = 6 * 1024 * 1024; //!< 6 megabytes must be downloaded into the compressed buffer before exiting buffering
const DWORD c_MaxCompressedBufferSize = 10 * 1024 * 1024; //!< Default size of compressed data buffer


                                                          //**********************************************************************
                                                          //! \brief SVFReaderConfiguration defines general configuration information use to initialize an \ref ISVFreader object
                                                          //!
                                                          //! This objects defines general configuration information used to setup an \ref ISVFReader object.
                                                          //! This structure is passed to \ref SVFCreateReaderWithConfig when the client creates the reader.
                                                          //**********************************************************************
struct SVFReaderConfiguration
{
    bool performCacheCleanup;        //!< If true, run low-priority cache cleanup thread upon initializing
    wchar_t cacheLocation[MAX_PATH]; //!< Custom cache location. If null or empty, SVF reader will use default cache

    SVFReaderConfiguration() :
        performCacheCleanup(true),
        cacheLocation(L"")
    {
    }
};

//**********************************************************************
//! \brief SVFConfiguration defines how to setup a playback session
//!
//! SVFConfiguration is passed to \ref ISVFReader "ISVFReader's" Open() call
//! to configure various playback values and parameters.
//**********************************************************************
struct SVFConfiguration
{
    bool useHardwareDecode;         //!< Should we use hardware decoding?
    bool useHardwareTextures;       //!< Should output be kept in a hardware texture (ID3D11Texture2D)?
    bool outputNV12;                //!< Should output be in NV12 format (otherwise it is in RGB)?
    bool bufferCompressedData;      //!< Should we buffer the incoming compressed data?
    bool useKeyedMutex;             //!< Should HW textures used keyed mutex for synchronization?
    bool disableAudio;              //!< Should we disable audio (audio won't even be decoded)?
    bool playAudio;                 //!< Should SVF handle playback of audio?
    bool returnAudio;               //!< Should audio be decoded and returned via GetNextAudioPacket()?
    ID3D11Device * spDevice; //!< Device to use (if nullptr then SVF will create device)
    IPropertyStore * spPropertyStore; //!< Property store (used to retrieve bbox...may be nullptr)
    DWORD prerollSize;              //!< Number of frames to preroll
    DWORD minBufferSize;            //!< Minimum number of frames that must be buffered (otherwise we go to buffering state)
    DWORD maxBufferSize;            //!< Maximum number of frames to buffer
    DWORD maxBufferUpperBound;      //!< Largest our buffer can ever grow to
    DWORD bufferHysteresis;         //!< Minimum number of buffered frames before restarting playback
    DWORD minCompressedBufferSize;  //!< The minimum number of compressed bytes that must be buffered before exiting buffering state.
    DWORD maxCompressedBufferSize;  //!< The maximum number of compressed bytes to buffer
    SVFTIME allowableDelay;         //!< Number of milliseconds late a frame can be before being considered for dropping
    bool allowDroppedFrames;        //!< Should we allow dropped frames (otherwise every frame is returned even if playback is slowed)
    DWORD maxDropFramesPerCall;     //!< Maximum number of dropped frames allowed per call to GetNextFrameViaClock()
    SVFTIME presentationLatency;    //!< Estimate of latency between time frame is returned via GetNextFrame() and time it is drawn
    ESVFLoop looping;               //!< Type of looping
    bool startDownloadWhenOpen;     //!< do not wait for MF calls to download file to disk cache, instead, start a separate download thread inside Open().
    float clockScale;               //!< Scale up the audio clock (1.0f = normal rate)
    UINT32 outputVertexFormat;      //!< Set of flags (see namespace ESVFOutputVertexFormat) describing which components make up the output vertex

    SVFConfiguration() :
        useHardwareDecode(true),
        useHardwareTextures(false),
        outputNV12(true),
        bufferCompressedData(true),
        useKeyedMutex(true),
        disableAudio(false),
        playAudio(true),
        returnAudio(false),
        spDevice(nullptr),
        spPropertyStore(nullptr),
        prerollSize(c_PrerollSize),
        minBufferSize(c_MinBufferSize),
        maxBufferSize(c_MaxBufferSize),
        maxBufferUpperBound(c_MaxBufferUpperBound),
        bufferHysteresis(c_BufferHysteresis),
        minCompressedBufferSize(c_MinCompressedBufferSize),
        maxCompressedBufferSize(c_MaxCompressedBufferSize),
        allowableDelay(150),
        allowDroppedFrames(true),
        maxDropFramesPerCall(10),
        presentationLatency(0),
        looping(ESVFLoop::LoopViaRestart),
        startDownloadWhenOpen(false),
        clockScale(1.0f),
        outputVertexFormat(ESVFOutputVertexFormat::Position | ESVFOutputVertexFormat::UV)
    {
    }
};

//**********************************************************************
//! \brief ISVFReader is the main interface that client talk to SVF through.
//!
//! This it the main interface that clients talk to SVF when using it in
//! synchronous mode.
//!\code
//! The general pattern for using this interface is as follows:
//!      Open | OpenStream
//!      .... You may now acquire metadata about the stream via ISVFAttributes ....
//!      [ .... Start of optional block ....
//!        BeginPlayback 
//!        GetNextFrame
//!        EndPlayback
//!      ]
//!      Close
//!\endcode
//**********************************************************************
interface DECLSPEC_UUID("0CAF78C6-16AF-4900-ADBF-BDFD257ED64F") ISVFReader : public IUnknown
{
    //! Returns the current clock being used to drive playback
    virtual HRESULT GetClock(_Outptr_ ISVFClock **ppClock) = 0;

    //! Returns the bounding box of the entire video clip. This method is obsolete.
    //! Clients should use \ref ISVFAttributes exposed by \ref ISVFReader via QI and read the GUIDs \ref SVFAttrib_BBoxMinX, etc
    virtual HRESULT GetBBox(_Out_ float *minX, _Out_ float *minY, _Out_ float *minZ, _Out_ float *maxX, _Out_ float *maxY, _Out_ float *maxZ) = 0;

    //! Opens the specified URL (pFilename). If the client is opening data from a DASH server this path should
    //! point to the URL to the .mpd file
    virtual HRESULT Open(_In_z_ const wchar_t *pFilename, _In_ SVFConfiguration *pConfig) = 0;

    //! Similar to Open except the HoloVideo is read from the provided stream
    virtual HRESULT OpenStream(_In_ IStream *pStream, _In_ SVFConfiguration *pConfig) = 0;

    //! BeginPlayback should be called once the client is ready to start reading frames
    virtual HRESULT BeginPlayback() = 0;

    //! Closes playback (client should no longer call GetNextFrame after this call)
    virtual HRESULT EndPlayback() = 0;

    //! Closes the current file
    virtual HRESULT Close() = 0;

    //! Performs a seek on the HoloVideo file to the specified time
    virtual HRESULT StartSource(_In_ SVFTIME timeInStreamUnits) = 0;

    //! Stops the playback clock and seeks to time 0
    virtual HRESULT StopSource() = 0;

    //! Returns the next available frame from the HoloVideo. This method returns the frame regardless
    //! of the frame's presentation time
    virtual HRESULT GetNextFrame(_Outptr_result_maybenull_ ISVFFrame **ppFrame, _Out_ bool *pEndOfStream) = 0;

    //! Returns the next available frame. This method only returns a frame if it is time (according to SVF's clock
    //! (see \ref ISVFClock)) to render that frame. If there is no frame ready, this method will return S_FALSE.
    //! If the next frame is late (current time is past its presentation time) the frame will be dropped.
    virtual HRESULT GetNextFrameViaClock(_Outptr_result_maybenull_ ISVFFrame **ppFrame, _Out_ bool *pEndOfStream) = 0;

    //! Returns the next audio packet. Audio data is in PCM format.
    virtual HRESULT GetNextAudioPacket(_Outptr_result_maybenull_ ISVFFrame **ppAudioFrame, _Out_ bool *pEndOfStream) = 0;

    //! Returns the number of streams in the HoloVideo (typically == 4 (index stream, vertex stream, texture stream, and audio stream)
    virtual HRESULT GetStreamCount(_Out_ ULONG *pNumberStreams) = 0;

    //! Return the stream associated with an index
    virtual HRESULT GetStream(ULONG streamIndex, _Outptr_ ISVFStream **ppStream) = 0;

    //! The client may call this method to indicate that it is currently rendering a frame previously acquired
    //! whose time stamp was tstamp. This way SVF can determine if it is keeping up with the renderer and if
    //! not it may start to degrade quality in attempt to keep up.
    virtual HRESULT Presenting(SVFTIME tstamp) = 0;

    //! Registers a client callback for being notified of state changes in SVF (see \ref ESVFReaderState)
    virtual void SetNotifyState(_In_ ISVFReaderStateCallback *pCallback) = 0;

    //! Sets the looping behavior for when playback reaches the end of the stream (see \ref ESVFLoop for details)
    virtual void SetLooping(ESVFLoop loop) = 0;

    //! Returns the current loop behavior
    virtual ESVFLoop GetLooping() = 0;

    //! Sets the current playback's audio volume level
    virtual void SetAudioVolumeLevel(float volume) = 0;

    //! Gets the current playback's audio volume level
    virtual float GetAudioVolumeLevel() = 0;

    //! Obsolete.
    // NEED TO REMOVE THIS. This should not be on the public interface
    virtual void SetStreamID(const wchar_t *pFilename) = 0;

    //! Causes SVF's clock to stop and for all worker threads to stop processing
    virtual void Sleep() = 0;

    //! Causes SVF's clock and worker threads to resume
    virtual void Wakeup() = 0;

    //! Returns the total number of dropped frames
    virtual int GetNumberDroppedFrames() = 0;

    // Obsolete
    virtual HRESULT GetThumbnailFrame(ISVFFrame **ppFrame) = 0;

    //! Sets parameters for HRTF (3D audio positioning)
    virtual void SetHrtfAudioDecaySettings(float minGain, float maxGain, float gainDistance, float cutoffDistance) = 0;

    //! Sets position of the listener for HRTF (3D audio positioning)
    virtual void SetAudio3DPosition(float x, float y, float z) = 0;

    //! Starts the clock (causing frames to be returned...see \ref ISVFReader::GetNextFrameViaClock)
    virtual void StartClock() = 0;

    //! Stops the clock
    virtual void StopClock() = 0;

    //! Gets the total duration of the HoloVideo
    virtual HRESULT GetDuration(_Out_ SVFTIME* pDuration) = 0;

    //! Sets the clocks scaling factor. This can be used to speed up or slowdown playback (within a small range)
    virtual void SetClockScale(float scale) = 0;

    //! Returns the current clock scale
    virtual float GetClockScale() = 0;

    //! Startup should be called immediately after creating a new SVF reader object
    virtual HRESULT Startup(ISVFBufferAllocator *pBufferAllocator, ISVFClock *pClock, SVFReaderConfiguration *pReaderConfig) = 0;

    //! Shuts down the SVF object along with all its worker threads
    virtual HRESULT Shutdown() = 0;

    //! Registers a client callback for being notified of internal fatal errors causing abrupt closing
    virtual void SetNoifyInternalState(_In_ ISVFReaderInternalStateCallback *pCallback) = 0;
};

//**********************************************************************
//! \brief ISVFBufferedStream allows clients to change the size of the download buffer
//! (see architecture document for more details). This method is exposed from
//! \ref ISVFReader (via QI).
//**********************************************************************
interface DECLSPEC_UUID("40B47CF8-AC84-4439-9932-6F45AEAA89C5") ISVFBufferedStream : public IUnknown
{
    virtual HRESULT SetBufferSize(int seconds) = 0;
};

//**********************************************************************
//! \brief ISVFAdapter allows specification of the graphics adapter to be used
//! for hardware decoding.  This method is exposed from \ref ISVFReader (via QI).
//**********************************************************************
interface DECLSPEC_UUID("F5B64436-6CC8-4C91-B733-69AEB3329636") ISVFAdapter : public IUnknown
{
    //! Specify graphics adapter to use; takes effect on subsequent ISVFReader::Open calls.
    virtual void SetAdapter(LUID luid) = 0;
};

//**********************************************************************
//! \brief ISVFAudioDevice allows specification of the audio device to be used
//! for rendering audio with XAudio.  This method is exposed from \ref ISVFReader (via QI).
//**********************************************************************
interface DECLSPEC_UUID("41608BD6-5935-43B4-96CB-1E31F98FA1CD") ISVFAudioDevice : public IUnknown
{
    //! Specify audio device to use; takes effect on subsequent ISVFReader::Open calls.
    virtual void SetAudioDevice(_In_ wchar_t const * pDeviceName) = 0;
};

//**********************************************************************
//! SVFCreateReader is the main function that clients should call to
//! create an ISVFReader object for using the SVF API in synchronous mode.
//**********************************************************************
extern "C" HRESULT SVFEXPORT SVFCreateReader(_In_ ISVFBufferAllocator *pBufferAllocator, _In_ ISVFClock *pClock, _Outptr_ ISVFReader **ppReader);

//**********************************************************************
//! SVFCreateReaderWithConfig is the main function that clients should call to
//! create an ISVFReader object for using the SVF API in synchronous mode.
//! This method is identical to SVFCreateReader but also allows for configuring
//! various reader attributes (such as cache location) via \ref SVFReaderConfiguration.
//**********************************************************************
extern "C" HRESULT SVFEXPORT SVFCreateReaderWithConfig(_In_ ISVFBufferAllocator *pBufferAllocator, _In_ ISVFClock *pClock, _In_ SVFReaderConfiguration *pReaderConfig, _Outptr_ ISVFReader **ppReader);

#endif
