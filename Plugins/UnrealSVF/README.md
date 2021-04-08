# Introduction

Unreal Engine plugin for Video Hologram (HCap) playback.  This contains built binaries of the native plugin libraries
for playing back the HCap file format, called SVF, as well as UE4 C++ plugin code that directly interface to the native plugin.

This version is tested with UE4 version 4.24 through 4.26.

Currently only Android (OpenGL only; ARMv7 & ARM64) and Windows platforms are supported.

# UE4 Plugin Requirements

- **Your UE4 Project should be a C++ project.** Your project needs to be a C++ project in case you need to rebuild the plugin,
which can be the case when switching from one version of the engine to another, or when using a different version of the engine
than the plugin was created with. **Make sure that you do this before adding the plugin to your project.**

- **You should have Visual Studio installed and set up.** Follow the [instructions for setting up Visual Studio](https://docs.unrealengine.com/en-US/Programming/Development/VisualStudioSetup/index.html) in the Unreal Engine documentation.

# Getting Started

## Starting a New Project

If you are starting a new project in the UE4 Editor, under Project Settings, switch from a Blueprint project to a C++
project. 

## Using an Existing Project

If your project was started as a Blueprint project instead of a C++ project, add a C++ file to the project by selecting
File -> New C++ Class... You can choose None for the parent class and leave everything else at the defaults.

## Adding the Plugin

You can add the plugin to a UE4 project by exiting the UE4 editor, then copying the UnrealSVF folder into
`[ProjectName]/Plugins` directory of your project.  You may need to create the Plugins directory if it doesn't already
exist.

When reloading the project in the UE4 editor after adding the plugin, you may be prompted that the UnrealSVF module
is "missing or built with a different engine version." If so, click Yes to rebuild it.

The rebuild should be successful if it was needed.  If it fails for some reason, then open the project's .sln file in 
Visual Studio and build it there, but this should be rare. If you still have problems, check to make sure that you're linking 
against the following libraries (most of them are already part of Windows SDK): 
DXGI.lib;D3D11.lib;WinHttp.lib;mfreadwrite.lib;xaudio2.lib;propsys.lib;crypt32.lib;

# Using the Plugin

The plugin adds a new game object type, **SVFActor**. SVFActors can be created by clicking and dragging from the
Modes panel (type SVFActor in the search box) or the Content Browser (under UnrealSVF C++ Classes/Public). 

Copy the hologram file (.mp4) somewhere under Contents/Movies; subdirectories are okay.

Enter the .mp4 file path in the **Relative File Path From Content** field.

Tick the **Auto Play** box or in the Blueprint editor's Event Graph, 
right click and type 'SVF' to see available functions for video playback.

SVF component presets can be selected by clicking on an actor in the scene
and selecting SVF component in the Details panel of the Level Editor.

The SVFComponent's material can be changed by clicking and dragging any UE4 material as normal.
This will instantly create a dynamic material instance of that material because the texture
needs to be updated independently by the plugin. To change the dynamic material instance at runtime,
avoid assigning a material directly and use ChangeDynamicMaterial.

Multiple holograms playing simultaneously are supported - just create a separate SVFActor object for each. Since each hologram
requires some processing overhead to decode and play, be sure to test for acceptable performance on the desired minimum platform.

# How to Package a Build

Since the mp4s are not treated as native UE4 assets, to include them in a package build, you must include them manually
in your project setting under Project->Packaging->Additional Non-Asset Directories to Copy.

**Note:** Android support is for OpenGL only, there is no Vulkan support. Currently, all files in the Movies folder
are copied to an external directory for the plugin to access and the file name is used as an identifier. If a video file
with same name already exists in the destination directory, it will not be overwritten.

**For ARM64:** UE4's Build.cs doesn't allow distinguish between ARMv7 and ARM64 for Android. To build for ARM64,
in Project Settings under Platforms - Android, tick `Support arm64 [aka arm64-v8a]` and untick
`Support armv7 [aka armeabi-v7a]`. Then, under UnrealSVF/Source/UnrealSVF/UnrealSVF.Build.cs, uncomment the line
`# define ANDROID_ARM64`. Similarly, to build for ARMv7 the opposite is true; it's not possible to build for both
architectures in a single APK at this time.

# SVF Component Settings

### General Settings

The following settings are useful:

- **Audio Disabled** - disable playback of any audio tracks embedded in the HoloVideo material.
- **Auto Looping** - restart playback at the beginning of the material after reaching the end.
- **Default Vertex/Index Count** - The RHI resource bufferes will be sized to these values if a file does not have
max vertex/index count in its file info. It is important to set these to be sufficiently large values to avoid 
resizing the buffers at runtime if your SVF content lacks these metadata. This is not necessary for new content.

### Exposing Vertex Data and Using in Niagara Particles

Accessing the vertex data of a HoloVideo can be done by calling GetVertices on the SVFComponent within the SVFActor. GetVertices returns an array of FVectors of the current frame being rendered in the playback.

In order to use this within the Niagara Particle System, add a User Exposed variable to the Niagara System.
Calling Niagara Set Vector Array on the Niagara Component of the Niagara System allows you to pass in the list of Vertices from GetVertices.
This variable will have to be updated every time you call GetVertices in order to update with the newest vertex data.

# Changelog

### 1.4.17 - 2021-03-18
Exposes Mobility and updates render bounds on tick to fix shadowmap clipping for more tightly bound meshes.

### 1.4.16 - 2021-03-08
Added functionality to get current frame vertex positional data.
Fix UE4 texture sync issues on Windows.

### 1.4.15 - 2021-02-10
Fixes a editor crash when you compile a class that extends the SVFActor.

### 1.4.14 - 2021-01-06
Fixes issue where holograms would not appear on Android devices.

### 1.4.13 - 2020-12-18
Adds support for UE 4.26

### 1.4.12.3 - 2020-12-14
Minor updates to exported headers.

### 1.4.12.2 - 2020-12-11
Provides option to disable H/W texture copying.

### 1.4.12.1 - 2020-12-07
Adds partial support for DirectX 12 by using software textures. This may limit performance on DirectX 12 so DirectX 11 is still recommended.

### 1.4.11 - 2020-12-05
Fixes D3D device crash issues on Windows.

### 1.4.10.3 - 2020-11-19
Minor bug fixes

### 1.4.10 - 2020-10-20
Fixes some D3D device crashes related to non-PIE(Play in editor) mode.

### 1.4.9 - 2020-10-12
Using DirectX 12 now shows warnings and disables functionality without crashing.

### 1.4.8 - 2020-09-24
Fixed UV mismatch and crashing issues in complex scenes on Windows.
Fixed bugs on JNI function calls.

### 1.4.7 - 2020-05-15
Support added for 4.24 and 4.25.
Fixed blueprint errors in example project.
Fixed frameinfo data being uninitialized in some cases.

### 1.4.6.0 - 2020-03-27
Fixed packaged asset copying in Android to catch up to latest changes in UE4

### 1.4.5.1 - 2019-11-01
Fixed packaging error

### 1.4.5 - 2019-10-30
Fixed shadows not appearing in 4.22 and 4.23

### 1.4.4 - 2019-10-07
4.23 support added

### 1.4.3 - 2019-06-12
Fixed clock scale (playback rate) not working for captures with audio on Windows
Fixed file copying on non-Oculus Android devices

### 1.4.2 - 2019-05-22
Add support for ARM64 on Android
Add support for 4.22
Fixed false alarm warning about UnrealSVF directories being missing

### 1.4.1 - 2019-05-21
MP4 files in Movies directory is now copied properly with the right subdirectory structure in Android
Fixed capture textures appearing black or green under certain project settings for Android

### 1.4.0 - 2019-04-02
Added BP function to change mobility of SVFActors allowing them to be moved
Added ability to destroy SVF components without deleting SVFActors
Improved stability
Decreased hanging in editor mode caused by unnecessary preview generation
SVFActors now initialize at proper UE4 scale
Fixed Rewind() in Windows
Fixed Pause/Play sometimes scrambling the texture
Autoplay behavior modified. Now autoplay just means the capture will be opened and played on BeginPlay, NOT
autoplay when it is opened. For playing automatically when opened, BP function for open takes in a parameter.

### 1.4.0b - 2019-01-11
Performance issues on Android fixed and promoted to beta
Bounding box now updates every frame
Fixed SVFComponents not pausing in PIE mode

### 1.4.0a - 2018-12-20
Alpha support for Android added
Performance issues limits playable captures to 1024x1024 texture
Texture handle can intermittently get mangled causing textures to stop being updated and file must be reopened to recover
Rewind added as blueprint function

### 1.3.7.5 - 2018-03-27
Various perf and memory leak improvements
Exposed UE4 ForceGarbageCollection. This can be used to force garbage collection on SVFActors in a scenario
many SVFActor is being instantiated and destroyed in a short period of time.

### 1.3.7.4 - 2018-12-14
Add AsyncOpen and AsyncClose BP functions
Note prematurely calling Play after AsyncOpen can still cause frame skips.
Fixed CalcBounds returning 0s in the Editor
Fixed signed normals in engine 4.20 and newer
DynamicMaterialInstance created by assigning a base material in the the editor is now named according to the base material

### 1.3.7.3 - 2018-12-13
Hot fix memory leak on destroying SVFActors in the middle of play session
Add BP Function ChangeDynamicMaterial to allow proper switching of dynamic material instance at runtime
Fix reverse culling which was showing backsides of faces when using translucency on SVFActors

### 1.3.7.2 - 2018-12-11
Update README

### 1.3.7.1 - 2018-11-21
Hotfix package build errors for 4.19

### 1.3.7 - 2018-10-03
Audio device enumeration
This allows user to query for audio devices and specify output audio device in SVFComponent's OpenInfo

### 1.3.6 - 2018-10-03
Add support for 4.20

### 1.3.5 - 2018-07-23
Proper key frame detection
This fixes a bug that scrambles indices if two keyframes have same number of vertices in succession

### 1.3.4 - 2018-07-05
Dependency to ATL.lib removed

### 1.3.3 - 2018-07-01
Project module moved to private dependency

## Additional Info

### Material assignment changes
Previously materials were assigned into a UPROPERTY of the component instead of directly to the component.
This has been simplified to be more conventional UE4-like, but there is a known bug where assigning the
same parent material to the component again will destroy the texture in the editor preview mode.

## Known issues
- Package build for Windows 32-bit fails
