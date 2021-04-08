#ifndef SVFExportFcns_UNITY3D_H
#define SVFExportFcns_UNITY3D_H

struct IUnityInterfaces;

extern "C"
{
	bool SetUnityBuffers(int id, void* textureHandle, int w, int h, void* vertexBufferHandle, int vertexCount, void* indexBufferHandle, int indexCount);
	void OnUnityRenderModeEvent(int instanceId);
	void UnityPluginLoad(IUnityInterfaces* unityInterfaces);
	void UnityPluginUnload();
	void UnitySetGraphicsDevice(void*, int deviceType, int eventType);
}

#endif // ! SVFExportFcns_UNITY3D_H