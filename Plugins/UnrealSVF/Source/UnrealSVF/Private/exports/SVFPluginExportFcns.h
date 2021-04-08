#ifndef SVFPLUGINEXPORTFCN_H
#define SVFPLUGINEXPORTFCN_H

#include "SVFPluginExport.h"
#include "SVFMesh.h"

struct HCapSettingsInterop;


extern "C"
{
    bool PlayHCapObject(int instanceID);
    bool RewindHCapObject(int instanceID);
    bool CloseHCapObject(int instanceID);
    bool OpenHCapObject(int instanceID, const char* path, SVFOpenInfo* pOpenInfo);
    bool OpenHCapObjectUsingFD(int instanceID, int fd, long offset, long length, SVFOpenInfo* pOpenInfo);
    int  CreateHCapObject(HCapSettingsInterop* pHcapSettings);
    bool DestroyHCapObject(int instanceID);
    bool PauseHCapObject(int instanceID);
    bool SetAudioVolume(int instanceID, float volume);    
    bool GetHCapObjectFileInfo(int instanceID, SVFFileInfo* pFileInfo);
    bool SetComputeNormals(int id, bool computeNormals);
    bool GetHCapObjectFrameInfo(int instanceID, SVFFrameInfo* pFrameInfo);	
    void SetClockScale(int instanceID, float scale);
	HCapObjectState GetHCapState(int instanceID);
	void GetMeshAndTexture(int instanceID, const MeshRequestInputData& requestData, SVFMesh& outMeshData);

	// This API can be called using HCapTracerVerbosity as second argument
    void EnableTracing(bool enable, int level);
}

#endif // ! SVFPLUGINEXPORTFCN_H
