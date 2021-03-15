# Grasshopper

## Description

_TBD_


## 🐿 Initial Setup 

> ⚠️  **Use Engine Version 4.26**

This repo uses a number of [third-party plugins](https://github.com/remap/SCiLPilot/tree/main/Plugins) (which in their turn rely on other plugins and third-party libraries). Here are steps for initial setup:

1. Clone repo recursively
```
git clone https://github.com/remap/SCiLPilot.git --recursive
```
2. Download & install "NDI SDK for Unreal Engine" from [here](https://drive.google.com/drive/u/0/folders/1fxe91QI2GUpWTaii_0h3h9GYe4tf1kch)
 * 👀 may also install "NDI 4 Tools"
 * 🚫 don't download "NDIIOPlugin" -- it is already setup during recursive cloning of the repo
3. Setup VideoCore plugin:
  * download precompiled dependencies from [here](https://bintray.com/peetonn/grasshopper/videocore_bin/0.0.1-alpha/view/files#files/)
  * extract Zip archive into `Plugins/VideoCore/Source/ThirdParty` folder
    * after extraction the folder must contain only **three subfolders** with dependencies (`libmediasoupclient`, `libsdptransform` and `webrtc`)
4. Generate Visual Studio solution by right-clicking on **Grasshopper.uproject** file and selecting _"Generate Visual Studio project files"_
5. Compile Visual Studio solution by opening it and pressing **Ctrl + Shift + B**
6. Open Grasshopper.uproject file. 

## Maps and Structure

Since this repo will be used for building three distinct demo experiences, there are three main subfolders under the "Content" folder:
* **TheaterTour**
* **TheCityAndTheCity**
* **Shell**
* **XR Learning**

❗️ **All content, specific to either experience must be placed under corresponding folder**

❗️ There are persistent levels added as sample levels for each demo. New maps should be added as sublevels. Maps, common for all three can be placed under "Content/Maps"

## 🤨 How to use Grasshopper

The first-class resident of Grasshopper is a video plane. Video planes are implemented by the [VideoCore](https://github.com/remap/VideoCore) plugin. There can be four types of video planes: **Live**, **Recorded**, **NDI** and **RTC**. Planes can be moved / dragged using LMB. All the currently available functionality can be found in the keybindings spreadsheet [here](https://docs.google.com/spreadsheets/d/15tR_Fw6NjqN5HlNaSnjtIf0f3Qb1BTSDiAIzIfQG5Sg/edit#gid=0).

### RTC Planes support

Read about WebRTC media server [here](https://github.com/remap/VideoCore#dependencies).

_More documentation TBD_
