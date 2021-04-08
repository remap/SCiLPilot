#ifndef SVFPLUGINEXPORT_H
#define SVFPLUGINEXPORT_H


#include <cstdint>

extern "C"
{
    struct HrtfAudioDecaySettings
    {
        float                           MinGain;
        float                           MaxGain;
        float                           GainDistance;
        float                           CutoffDistance;
    };

    struct SVFOpenInfo
    {
        bool                            AudioDisabled; // do not play audio even if it is present
        bool                            UseKeyedMutex;
        bool                            RenderViaClock;// Call SVF GetFrameViaClock rather than GetFrame
        bool                            OutputNormals; // If true, add normals in returned vertex buffer
        bool                            StartDownloadOnOpen; // if true, SVFReader starts download on Open(), independently on MF calls. If SVF does not get destroyed ahead of time,
        bool                            AutoLooping; // if true, plugin will issue EOS, but will take care of rewinding to beginning
        bool                            forceSoftwareClock; // if true, we enforce custom software clock to enable clock scaling. If false: filers with audio will have default scaling, files with no audio: no scaling
        // HCap content gets pre-cached.
        float                           playbackRate; // Used to speed up playback (1.0f = normal playback)
        HrtfAudioDecaySettings          hrtf;
        wchar_t                         AudioDeviceId[1024]; // audio device to use for playback
    };

    struct SVFFileInfo
    {
        bool hasAudio;
        uint64_t duration100ns;  // as obtained from Media Foundation
        uint32_t frameCount;
        uint32_t maxVertexCount; // max frame count, across all frames
        uint32_t maxIndexCount; // max index count, across all frames
        float bitrateMbps;
        float fileSize;
        // max bound box
        double minX, minY, minZ;
        double maxX, maxY, maxZ;
        // max texture dimensions
        uint32_t fileWidth, fileHeight;
        bool hasNormals;
    };

    struct SVFFrameInfo
    {
        uint64_t frameTimestamp; // in 100ns MF time units

        // per-frame bound box
        double minX, minY, minZ;
        double maxX, maxY, maxZ;

        uint32_t frameId; // starts from 0
        uint32_t vertexCount; // per-frame vertex count
        uint32_t indexCount; // per-frame index count

        int textureWidth;
        int textureHeight;

        bool isEOS;
        bool isRepeatedFrame;
        bool isKeyFrame;
    };
	
	enum HCapObjectState {
        Empty = 0,
        Initialized = 1,
        Opened = 2,
        Playing = 3,
        Paused = 4,
        Closed = 5,
        Broken = 6
    };
	
	enum HCapTracerVerbosity {
		Chatty = 0,
        Normal = 1,
        Warnings = 2,
        Errors = 3,
        Never = 4,
        Max = Never
	};
}


#endif // ! SVFPLUGINEXPORT_H