#ifndef SVFAndriodExportFcn_H
#define SVFAndriodExportFcn_H

#if PLATFORM_ANDROID

#include <jni.h>
    extern "C"
    {
        void Java_com_microsoft_mixedreality_mrcs_svf_JavaPlugin_onCreated(JNIEnv* env, jclass clazz, jobject javaPlugin, jobject assetManager);
    }
#endif


#endif // ! SVFAndriodExportFcn_H