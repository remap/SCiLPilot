#ifndef SVFExportFcns_UNREAL_H
#define SVFExportFcns_UNREAL_H

    extern "C"
    {
        void OnUnrealRenderEvent(int instanceId,
									void* vb, void* ib, int UnrealVertexBufferCount, int UnrealIndexBufferCount,
									int UnrealTexture, int UnrealTextureWidth, int UnrealTextureHeight);
    }


#endif // ! SVFExportFcns_H