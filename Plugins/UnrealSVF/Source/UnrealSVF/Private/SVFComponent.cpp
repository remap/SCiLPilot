// Copyright (C) Microsoft Corporation. All rights reserved.

#include "SVFComponent.h"
#include "SVFMeshComponents.h"
#include "UnrealSVF.h"
#include "Misc/Paths.h"
#include "SVFSimpleInterface.h"
#include "DynamicMeshBuilder.h"
#include "Engine/Engine.h"
#include "LocalVertexFactory.h"

#include "Runtime/Launch/Resources/Version.h"
#include "Runtime/Engine/Classes/PhysicsEngine/BodySetup.h"

DEFINE_LOG_CATEGORY_STATIC(LogSVFComponent, Log, All);

#define LogSVF(pmt, ...) UE_LOG(LogSVFComponent, Log, TEXT(pmt), ##__VA_ARGS__)
#define WarnSVF(pmt, ...) UE_LOG(LogSVFComponent, Warning, TEXT(pmt), ##__VA_ARGS__)
#define FatalSVF(pmt, ...) UE_LOG(LogSVFComponent, Fatal, TEXT(pmt), ##__VA_ARGS__)

DECLARE_CYCLE_STAT(TEXT("Get Mesh Elements"), STAT_SVF_GetMeshElements, STATGROUP_UnrealSVF);
DECLARE_CYCLE_STAT(TEXT("Create Scene Proxy"), STAT_SVF_CreateSceneProxy, STATGROUP_UnrealSVF);
DECLARE_CYCLE_STAT(TEXT("Update Collision"), STAT_SVF_UpdateCollision, STATGROUP_UnrealSVF);
DECLARE_CYCLE_STAT(TEXT("Update Mesh Elements"), STAT_SVF_UpdateMeshElements, STATGROUP_UnrealSVF);

#if PLATFORM_WINDOWS
#include "SVFReaderPassThrough.h"
THIRD_PARTY_INCLUDES_START
#include <setupapi.h>
#if ENGINE_MINOR_VERSION > 19
#include <initguid.h> // must be included before mmdeviceapi.h for the pkey defines to be properly instantiated.
#include <mmdeviceapi.h>
#endif
THIRD_PARTY_INCLUDES_END

bool USVFComponent::OpenFilePath()
{
    bool bResult = true;
    FString tmpFilePath = FPaths::ProjectContentDir() / RelativeFilePathFromContent.FilePath;

    if (!ValidateFilePath(tmpFilePath))
    {
        RelativeFilePathFromContent.FilePath.Empty();
        WarnSVF("File path %s is bad!", *tmpFilePath);
        return false;
    }

    // Use preset settings
    if (bOverride_PresetMode)
    {
        OpenInfo = FSVFOpenInfo(PresetMode);
    }

    // Can we do 
    if (FAILED(ISVFSimpleInterface::CreateInstance(this, RelativeFilePathFromContent.FilePath,
        OpenInfo, &SVFReaderObject, this)))
    {
        WarnSVF("SVFReader instance not created!");
        if (SVFReaderObject)
        {
            SVFReaderObject->MarkPendingKill();
        }
        SVFReaderObject = nullptr;
        SVFReader = nullptr;
        OpenedFilePath.Empty();
        return false;
    }

    SVFReader = Cast<ISVFSimpleInterface>(SVFReaderObject);
    OpenedFilePath = RelativeFilePathFromContent.FilePath;
    FileInfo = SVFReader->GetFileInfo();

#if WITH_EDITOR
    FEditorDelegates::PausePIE.AddUObject(this, &USVFComponent::HandlePausePIE);
    FEditorDelegates::ResumePIE.AddUObject(this, &USVFComponent::HandleResumePIE);
#endif

    return true;
}

void USVFComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    if (ClockState == EUSVFClockState::Running)
    {
        ClockCurrentTime += DeltaTime * m_ClockScale;
    }
    bool bDoUpdate = GFrameNumber % UpdateFrameInterval == 0;
    if (!bDoUpdate)
    {
        return;
    }

    if (SVFReader && !DisableAll)
    {
        SVFReader->GetSVFStatus(m_status);
        LastState = (EUSVFReaderState)m_status.lastKnownState;
        if (!DisableGetNextFrame && bIsPlaying)
        {
            bool bIsNewFrame = false;
            bool isEndOfStream = false;
            SVFReader->GetNextFrameViaClock(bIsNewFrame, &isEndOfStream);

            SVFReader->GetFrameData(LastFrameData);

            if (bIsNewFrame && !DisableUpdateMesh)
            {
                GenerateMesh();
            }
        }
    }

    UpdateBounds();
    MarkRenderTransformDirty();

    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USVFComponent::GenerateMesh()
{
    if (!LastFrameData.IsValid() || !SceneProxy)
    {
        return;
    }

    FSVFFrameInfo FrameInfo;
    LastFrameData->GetFrameInfo(FrameInfo);
    if (FrameInfo.frameId > 0)
    {
        FSVFMeshSceneProxy* LocalSceneProxy = (FSVFMeshSceneProxy*)SceneProxy;
        TSharedPtr<FFrameData> LocalLastFrameData = LastFrameData;
        // Enqueue command to send to render thread
        ENQUEUE_RENDER_COMMAND(FSVFMeshUpdate)(
            [=](FRHICommandListImmediate& RHICmdList)
            {
                if (LocalSceneProxy)
                {
                    LocalSceneProxy->Update_RenderThread(LocalLastFrameData);
                }
                else
                {
                    LogSVF("Lost SVFSceneProxy. Skipping Update_RenderThread for this frame.");
                }
                
            }
        );

        bUpdateTexture = FrameInfo.isKeyFrame;
        LastVerticesNum = FrameInfo.vertexCount;
        LastIndicesNum = FrameInfo.indexCount;
    }

    if ((bUpdateTexture && bUpdateTextureLessOften) || !bUpdateTextureLessOften)
    {
        UpdateMaterial();
    }
}

#elif PLATFORM_ANDROID
#include "SVFReaderAndroid.h"

bool USVFComponent::OpenFilePath()
{
    bool bResult = true;
    FString tmpFilePath = FPaths::ProjectContentDir() / RelativeFilePathFromContent.FilePath;

    if (!ValidateFilePath(tmpFilePath))
    {
        RelativeFilePathFromContent.FilePath.Empty();
        WarnSVF("File path %s is bad!", *tmpFilePath);
        return false;
    }

    // Use preset settings
    if (bOverride_PresetMode)
    {
        // TODO
        WarnSVF("bOverride_PresetMode not implemented");
    }

    if (SVFReader == nullptr)
    {
        if (!ISVFSimpleInterface::CreateInstance(this, RelativeFilePathFromContent.FilePath,
            OpenInfo, &SVFReaderObject, this))
        {
            WarnSVF("SVFReader instance not created!");
            if (SVFReaderObject)
            {
                SVFReaderObject->MarkPendingKill();
            }
            SVFReaderObject = nullptr;
            SVFReader = nullptr;
            OpenedFilePath.Empty();
            return false;
        }
        SVFReader = Cast<ISVFSimpleInterface>(SVFReaderObject);
    }
    else
    {
        Cast<USVFReaderAndroid>(SVFReader)->Open(RelativeFilePathFromContent.FilePath, OpenInfo);
    }

    OpenedFilePath = RelativeFilePathFromContent.FilePath;
    FileInfo = SVFReader->GetFileInfo();

    if (FileInfo.FileWidth > 0 && FileInfo.FileHeight > 0)
    {
        if (!DynTexture || FileInfo.FileWidth != DynTexture->GetSizeX() ||
            FileInfo.FileHeight != DynTexture->GetSizeY())
        {
            EPixelFormat TextureFormat = EPixelFormat::PF_R8G8B8A8;
            DynTexture = UTexture2D::CreateTransient(FileInfo.FileWidth, FileInfo.FileHeight, TextureFormat);
            DynTexture->UpdateResource();
        }
    }
    Cast<USVFReaderAndroid>(SVFReader)->EnqueueSetUnrealTexture(DynTexture);
    if (FileInfo.MaxVertexCount == 0 || FileInfo.MaxIndexCount == 0)
    {
        Cast<USVFReaderAndroid>(SVFReader)->ResizeBuffers(DefaultVertexCount, DefaultIndexCount);
    }

    if (!ResumeHandle.IsValid())
    {
        ResumeHandle = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.
            AddUObject(this, &USVFComponent::HandleApplicationHasEnteredForeground);
    }

    if (!PauseHandle.IsValid())
    {
        PauseHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.
            AddUObject(this, &USVFComponent::HandleApplicationWillEnterBackground);
    }

    return true;
}

void USVFComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    if (bIsPlaying && SVFReader && SVFReader->GetFrameData(LastFrameData))
    {
        GenerateMesh();
    }

    UpdateBounds();
    MarkRenderTransformDirty();

    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USVFComponent::GenerateMesh()
{
    if (!LastFrameData.IsValid() || !SceneProxy)
    {
        return;
    }

    FSVFFrameInfo FrameInfo;
    LastFrameData->GetFrameInfo(FrameInfo);
    if (FrameInfo.vertexCount >= 0)
    {
        // Call command for update Data in mesh
        if (SceneProxy)
        {
            int MaxVertexCount = FileInfo.MaxVertexCount == 0 ? DefaultVertexCount : FileInfo.MaxVertexCount;
            int MaxIndexCount = FileInfo.MaxIndexCount == 0 ? DefaultIndexCount : FileInfo.MaxIndexCount;

            FSVFMeshSceneProxy* LocalSceneProxy = (FSVFMeshSceneProxy*)SceneProxy;
            TSharedPtr<FFrameData> LocalLastFrameData = LastFrameData;
            // Enqueue command to send to render thread
            ENQUEUE_RENDER_COMMAND(FSVFMeshUpdate)(
				[=](FRHICommandListImmediate& RHICmdList)
                {
                    LocalSceneProxy->Update_RenderThread(LocalLastFrameData, MaxVertexCount, MaxIndexCount);
                }
            );
        }
        bUpdateTexture = FrameInfo.isKeyFrame;
    }

    if ((bUpdateTexture && bUpdateTextureLessOften) || !bUpdateTextureLessOften)
    {
        UpdateMaterial();
    }
}

#endif // PLATFORM_ANDROID

#if WITH_EDITOR
#include "Editor.h"

void USVFComponent::HandlePausePIE(bool bIsSimulating)
{
    HandleApplicationWillEnterBackground();
}

void USVFComponent::HandleResumePIE(bool bIsSimulating)
{
    HandleApplicationHasEnteredForeground();
}

void USVFComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
    FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ?
        PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

    if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(USVFComponent, RelativeFilePathFromContent))
    {
        FPaths::MakePathRelativeTo(RelativeFilePathFromContent.FilePath, *FPaths::ProjectContentDir());
        RelativeFilePath_EditorPreview = RelativeFilePathFromContent.FilePath;
    }
    if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(USVFComponent, OverrideMaterials))
    {
        BaseMaterial = GetMaterial(0);
        if (BaseMaterial == nullptr)
        {
            BaseMaterial = DefaultUnlitMaterial;
        }
        // OnRegister will be called anytime a property is changed, but we have to call
        // GeneratePreview when material has changed, since file path has not changed
        GeneratePreview();
    }
    Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool USVFComponent::GeneratePreview()
{
    bool bResult = true;
    FString tmpFilePath = FPaths::ProjectContentDir() / RelativeFilePath_EditorPreview;
    if (!ValidateFilePath(tmpFilePath))
    {
        WarnSVF("File path %s is bad!", *tmpFilePath);
        return false;
    }

    if (!GetWorld() || IsRunningCommandlet())
    {
        return true;
    }

    CloseCurrent(false);
    // For editor - need try open file for validating and generate preview first frame
    if (FAILED(ISVFSimpleInterface::CreateInstance(this, RelativeFilePath_EditorPreview,
        OpenInfo, &SVFReaderObject, this)))
    {
        WarnSVF("SVFReader instance not created!");
        if (SVFReaderObject)
        {
            SVFReaderObject->MarkPendingKill();
        }
        SVFReaderObject = nullptr;
        SVFReader = nullptr;
        OpenedFilePath.Empty();
        return false;
    }

    SVFReader = Cast<ISVFSimpleInterface>(SVFReaderObject);
    FileInfo = SVFReader->GetFileInfo();
    if (FileInfo.FrameCount < 1)
    {
        bResult = false;
    }

    if (bResult)
    {
        SVFReader->Start();
        bResult = false;
        if (!SVFReader->BeginPlayback())
        {
            WarnSVF("Can`t begin play file %s!", *tmpFilePath);
        }
        else
        {
            for (int32 GuardValue = 0; GuardValue < 300; ++GuardValue)
            {
                bool bIsEndFrame = false;
                if (SVFReader->GetNextFrame(&bIsEndFrame))
                {
                    SVFReader->GetFrameData(LastFrameData);
                    GenerateMesh();
                    UpdateMaterialEditor();
                    bResult = true;
                    OpenedFilePath = RelativeFilePath_EditorPreview;
                    break;
                }
                else
                {
                    Sleep(10);
                }
            }
        }
    }
    if (SVFReader)
    {
        SVFReader->Close();
    }
    return bResult;
}

void USVFComponent::UpdateMaterialEditor()
{
    if (LastFrameData.IsValid() && !DisableUpdateTexture)
    {
        if (!DynTexturePreview || FileInfo.FileWidth != DynTexturePreview->GetSizeX() || FileInfo.FileHeight != DynTexturePreview->GetSizeY())
        {
            EPixelFormat TextureFormat;
            int32 Height = 0;
            int32 Width = 0;
            if (LastFrameData->GetTextureInfo(TextureFormat, Width, Height))
            {
                DynTexturePreview = UTexture2D::CreateTransient(Width, Height, TextureFormat);
                DynTexturePreview->UpdateResource();
#if PLATFORM_ANDROID
                Cast<USVFReaderAndroid>(SVFReader)->EnqueueSetUnrealTexture(DynTexturePreview);
#endif
            }
        }
        LastFrameData->CopyTextureBuffer(DynTexturePreview);
    }

    // Dynamic material not assigned or the material has changed and new instance must be created
    if (DynInstance && DynInstance->Parent != BaseMaterial)
    {
        DynInstance = nullptr;
    }
    if (!DynInstance)
    {
        FString InstanceName = BaseMaterial->GetFName().ToString() + "_Inst";
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
        DynInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this, FName(*InstanceName));
#else
        DynInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
#endif
        SetMaterial(0, DynInstance);
    }
    DynInstance->SetTextureParameterValue(TEXT("Texture"), DynTexturePreview);
}

#endif // WITH_EDITOR

FPrimitiveSceneProxy* USVFComponent::CreateSceneProxy()
{
    SCOPE_CYCLE_COUNTER(STAT_SVF_CreateSceneProxy);
    return new FSVFMeshSceneProxy(this);
}

void USVFComponent::CloseCurrent(bool InAsync)
{
    if (!IsWorldPlaying())
    {
        return;
    }

    if (SVFReader)
    {
        if (InAsync)
        {
            SVFReader->Close_BackgroundThread();
        }
        else
        {
            SVFReader->Close();
        }
        if (SVFReaderObject)
        {
            SVFReaderObject->MarkPendingKill();
        }
        SVFReaderObject = nullptr;
        SVFReader = nullptr;
        OpenedFilePath.Empty();
    }

    if (PauseHandle.IsValid())
    {
        FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(PauseHandle);
        PauseHandle.Reset();
    }
    if (ResumeHandle.IsValid())
    {
        FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ResumeHandle);
        ResumeHandle.Reset();
    }

#if WITH_EDITOR
    FEditorDelegates::PausePIE.RemoveAll(this);
    FEditorDelegates::ResumePIE.RemoveAll(this);
#endif
}

// next

int32 USVFComponent::GetNumMaterials() const
{
    return 1;
}

FBoxSphereBounds USVFComponent::CalcBounds(const FTransform & LocalToWorld) const
{
    if (SVFReader && LastFrameData.IsValid())
    {
        FSVFFrameInfo FrameInfo;
        LastFrameData->GetFrameInfo(FrameInfo);
        if (FrameInfo.frameId > 0)
        {
            return FBoxSphereBounds(FrameInfo.Bounds).TransformBy(LocalToWorld);
        }
    }
    return FBoxSphereBounds(FVector::ZeroVector, FVector::ZeroVector, 1).TransformBy(LocalToWorld);
}

bool USVFComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
    return true;
}

bool USVFComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
    return false;
}

void USVFComponent::UpdateBodySetup()
{
    if (MeshBodySetup == NULL)
    {
        // The body setup in a template needs to be public since the property is Tnstanced and thus
        // is the archetype of the instance meaning there is a direct reference
        MeshBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
        MeshBodySetup->BodySetupGuid = FGuid::NewGuid();

        MeshBodySetup->bGenerateMirroredCollision = false;
        MeshBodySetup->bDoubleSidedGeometry = true;
        MeshBodySetup->CollisionTraceFlag = CTF_UseDefault;
    }
}

void USVFComponent::UpdateCollision()
{
    SCOPE_CYCLE_COUNTER(STAT_SVF_UpdateCollision);

    bool bCreatePhysState = false;// Should we create physics state at the end of this function?
                                    // If its created, shut it down now
    if (bPhysicsStateCreated)
    {
        DestroyPhysicsState();
        bCreatePhysState = true;
    }

    // Ensure we have a BodySetup
    UpdateBodySetup();

    // Fill in simple collision convex elements
    if (SVFReader)
    {
        FSVFFrameInfo FrameInfo;
        SVFReader->GetFrameInfo(FrameInfo);
        if (FrameInfo.vertexCount >= 3)
        {
            MeshBodySetup->AggGeom.BoxElems.Empty(1);
            FVector Extent = FrameInfo.Bounds.GetExtent();
            FKBoxElem BoxElem = FKBoxElem(Extent.X, Extent.Y, Extent.Z);
            MeshBodySetup->AggGeom.BoxElems.Add(BoxElem);
        }
    }

    // Set trace flag
    MeshBodySetup->CollisionTraceFlag = CTF_UseDefault;

    // New GUID as collision has changed
    MeshBodySetup->BodySetupGuid = FGuid::NewGuid();
    // Also we want cooked data for this
    MeshBodySetup->bHasCookedCollisionData = true;

#if WITH_PHYSX && WITH_EDITOR
    // Clear current mesh data
    MeshBodySetup->InvalidatePhysicsData();
    // Create new mesh data
    MeshBodySetup->CreatePhysicsMeshes();
#endif

    // Create new instance state if desired
    if (bCreatePhysState)
    {
        CreatePhysicsState();
    }
}

UBodySetup* USVFComponent::GetBodySetup()
{
    UpdateBodySetup();
    return MeshBodySetup;
}

USVFComponent::USVFComponent()
    : bAutoPlay(true)
    , DisableUpdateMesh(false)
    , DisableUpdateTexture(false)
    , DisableGetNextFrame(false)
    , DisableAll(false)
{
    PrimaryComponentTick.bCanEverTick = true;
    UpdateFrameInterval = 1;
    static ConstructorHelpers::FObjectFinder<UMaterialInterface>
            BaseUnlitMaterial(TEXT("/UnrealSVF/Materials/UnlitTexture"));
    DefaultUnlitMaterial = BaseUnlitMaterial.Object;
    BaseMaterial = DefaultUnlitMaterial;
    bWantsInitializeComponent = true;
}

void USVFComponent::HandleApplicationHasEnteredForeground()
{
    if (wasPlayingBeforeEnteringBackground)
    {
        SVF_Play();
    }
}

void USVFComponent::HandleApplicationWillEnterBackground()
 {
    wasPlayingBeforeEnteringBackground = bIsPlaying;
    if (bIsPlaying)
    {
        SVF_Pause();
    }
}

bool USVFComponent::ValidateFilePath(const FString& InFilePath)
{
    if (InFilePath.IsEmpty())
    {
        return false;
    }
    if (!FPaths::FileExists(InFilePath))
    {
        return false;
    }
    if (!(FPaths::GetExtension(InFilePath).Equals(TEXT("mp4"), ESearchCase::IgnoreCase)))
    {
        return false;
    }
    return true;
}

bool USVFComponent::IsWorldPlaying() const
{
#if WITH_EDITOR
    if (!GetWorld() || IsRunningCommandlet())
    {
        return false;
    }
#endif
    return HasBegunPlay();
}

bool USVFComponent::IsFileOpened() const
{
    return !OpenedFilePath.IsEmpty() && SVFReader != nullptr;
}

void USVFComponent::OnRegister()
{
    Super::OnRegister();

#if WITH_EDITOR
    // We have to GeneratePreview on register because to my knowledge for editor start,
    // but there may be a better more targeted event binding since this is called
    // whenever a property is edited
    if (GetWorld()->WorldType == EWorldType::Editor ||
        GetWorld()->WorldType == EWorldType::EditorPreview)
    {
        if (RelativeFilePathFromContent.FilePath != OpenedFilePath)
        {
            RelativeFilePath_EditorPreview = RelativeFilePathFromContent.FilePath;
            GeneratePreview();
        }
    }
#endif
}

void USVFComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoPlay)
    {
        SVF_Open(RelativeFilePathFromContent.FilePath);
        SVF_Play();
    }
}

void USVFComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    SVF_Close();
    Super::EndPlay(EndPlayReason);
}

void USVFComponent::DestroyComponent(bool bPromoteChildren)
{
    SVF_Close();
    Super::DestroyComponent(bPromoteChildren);
}

void USVFComponent::UpdateMaterial()
{
    if (LastFrameData.IsValid() && !DisableUpdateTexture)
    {
        if (!DynTexture || FileInfo.FileWidth != DynTexture->GetSizeX() || FileInfo.FileHeight != DynTexture->GetSizeY())
        {
            EPixelFormat TextureFormat;
            int32 Height = 0;
            int32 Width = 0;
            if (LastFrameData->GetTextureInfo(TextureFormat, Width, Height))
            {
                DynTexture = UTexture2D::CreateTransient(Width, Height, TextureFormat);
                DynTexture->UpdateResource();
#if PLATFORM_ANDROID
                Cast<USVFReaderAndroid>(SVFReader)->EnqueueSetUnrealTexture(DynTexture);
#endif
            }
        }
        if (!LastFrameData->CopyTextureBuffer(DynTexture, bUseHardwareTextureCopy))
        {
            WarnSVF("Unexpected texture copy failure.");
        }
    }

    // Dynamic material not assigned or the material has changed and new instance must be created
    if (DynInstance && DynInstance->Parent != BaseMaterial)
    {
        DynInstance = nullptr;
    }
    if (!DynInstance)
    {
        FString InstanceName = BaseMaterial->GetFName().ToString() + "_Inst";
        //DynInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this, FName(*InstanceName));
        DynInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        SetMaterial(0, DynInstance);
    }
    DynInstance->SetTextureParameterValue(TEXT("Texture"), DynTexture);
}

bool USVFComponent::SVF_IsPlaying()
{
    return SVFReader && bIsPlaying;
}

void USVFComponent::SVF_Play()
{
#if PLATFORM_ANDROID
    SVFReader->Start();
    bIsPlaying = true;
    return;
#endif
    if (!SVFReader || OpenedFilePath.IsEmpty())
    {
        return;
    }

    if (!bIsBeginPlayback)
    {
        SVFReader->Start();
        if (!SVFReader->BeginPlayback())
        {
            WarnSVF("Can`t BeginPlay!");
            CloseCurrent();
        }
        else
        {
            bIsPlaying = true;
            bIsBeginPlayback = true;
        }
    }
    else if (LastState < EUSVFReaderState::Closing && ClockState != EUSVFClockState::Running)
    {
        SVFReader->Start();
        bIsPlaying = true;
    }
    else if (!bIsPlaying && ClockState == EUSVFClockState::Running && LastState <= EUSVFReaderState::Ready)
    {
        SVFReader->Start();
        if (!SVFReader->BeginPlayback())
        {
            WarnSVF("Can`t BeginPlay!");
            CloseCurrent();
        }
        else
        {
            bIsPlaying = true;
        }
    }
}

void USVFComponent::SVF_Pause()
{
    if (SVF_IsPlaying())
    {
        SVFReader->Stop();
        bIsPlaying = false;
        PauseCounter = PauseCounterResetValue;
    }
}

void USVFComponent::SVF_Stop()
{
    if (SVF_IsPlaying())
    {
        SVFReader->Stop();
        SVF_SeekToPercent(0);
        bIsPlaying = false;
        PauseCounter = PauseCounterResetValue;
    }
}

float USVFComponent::SVF_GetCurrentPositionPercent()
{
    if (!SVFReader || FileInfo.FrameCount < 1)
    {
        return 0.f;
    }

    FSVFFrameInfo FrameInfo;
    SVFReader->GetFrameInfo(FrameInfo);
    return static_cast<float>(FrameInfo.frameId) / static_cast<float>(FileInfo.FrameCount);
}

FTimespan USVFComponent::SVF_GetCurrentPosition()
{
    if (!SVFReader || FileInfo.FrameCount < 1)
    {
        return 0.f;
    }

    FSVFFrameInfo FrameInfo;
    SVFReader->GetFrameInfo(FrameInfo);
    return FileInfo.Duration * (static_cast<float>(FrameInfo.frameId) / static_cast<float>(FileInfo.FrameCount));
}

void USVFComponent::SVF_SeekToPercent(float SeekToPercent)
{
#if PLATFORM_WINDOWS
    if (SVFReader && (LastState == EUSVFReaderState::Ready || LastState == EUSVFReaderState::Buffering))
#endif
    {
        SVFReader->SeekToPercent(FMath::Clamp(SeekToPercent, 0.f, 1.f));
    }
}

void USVFComponent::SVF_SeekToTime(FTimespan ToTime)
{
    if (SVFReader && (LastState == EUSVFReaderState::Ready || LastState == EUSVFReaderState::Buffering))
    {
        SVFReader->SeekToTime(ToTime);
    }
}

void USVFComponent::SVF_SeekToFrame(int32 frameId)
{
    if (SVFReader && (LastState == EUSVFReaderState::Ready || LastState == EUSVFReaderState::Buffering))
    {
        SVFReader->SeekToFrame((uint32)frameId);
    }
}

void USVFComponent::SVF_Rewind()
{
    if (SVFReader)
    {
        SVFReader->Rewind();
    }
}

FTimespan USVFComponent::SVF_GetDuration() const
{
    return FileInfo.Duration;
}

void USVFComponent::SVF_AsyncClose()
{
    SVF_Pause();
#if PLATFORM_WINDOWS
    CloseCurrent(true);
#else
    WarnSVF("SVF_AsyncOpen not implemented on non-Windows platforms");
    CloseCurrent(false);
#endif
}

void USVFComponent::SVF_Close()
{
    if (!OpenedFilePath.IsEmpty())
    {
        CloseCurrent();
        bIsPlaying = false;
    }
}

void USVFComponent::SVF_AsyncOpen(bool InPlayWhenReady)
{
    SVF_Pause();
    AsyncTask(ENamedThreads::AnyBackgroundHiPriTask, [this, InPlayWhenReady]()
    {
        SVF_Open(RelativeFilePathFromContent.FilePath, InPlayWhenReady);
    });
}

bool USVFComponent::SVF_Open(const FString& FileName, bool InStartPlayingImmediately)
{
    if (SVFReader)
    {
        CloseCurrent();
        bIsPlaying = false;
    }
    OpenInfo.playbackRate = m_ClockScale;
    bool bResult = OpenFilePath();
    if (bResult && InStartPlayingImmediately)
    {
        SVF_Play();
    }
    return bResult;
}

void USVFComponent::SVF_SetPlayRate(float InPlayRate)
{
    if (!SVFReader)
    {
        return;
    }

    if (!SVF_CanSeek())
    {
        InPlayRate = FMath::Abs(InPlayRate);
    }
    if (FMath::Abs(InPlayRate) < 0.1f)
    {
        SVF_Pause();
    }
    else
    {
        SVFClock_SetScale(FMath::Clamp(FMath::RoundToInt(FMath::Abs(InPlayRate) / 0.1f) * 0.1f, -10.f, 10.f));
        SVFReader->SetPlayFlow(InPlayRate >= 0.f);
    }
}

float USVFComponent::SVF_GetPlayRate()
{
    return m_ClockScale;
}

bool USVFComponent::SVF_CanSeek()
{
    return SVFReader && SVFReader->CanSeek();
}

bool USVFComponent::SVFClock_GetPresentationTime(int64* OutTime)
{
    if (!SVFClock_GetTime(OutTime))
    {
        return false;
    }
    *OutTime = *OutTime - PresentationOffset;
    return true;
}

bool USVFComponent::SVFClock_GetTime(int64* OutTime)
{
    int64 Result = static_cast<double>(ClockCurrentTime) * 10000000L;
    *OutTime = Result;
    return true;
}

bool USVFComponent::SVFClock_Start()
{
    ClockState = EUSVFClockState::Running;
    return true;
}

bool USVFComponent::SVFClock_Stop()
{
    ClockState = EUSVFClockState::Stopped;
    return true;
}

bool USVFComponent::SVFClock_GetState(EUSVFClockState* OutState)
{
    *OutState = ClockState;
    return true;
}

bool USVFComponent::SVFClock_Shutdown()
{
    ClockState = EUSVFClockState::Stopped;
    ClockCurrentTime = 0.f;
    return true;
}

bool USVFComponent::SVFClock_SetPresentationOffset(int64 InT)
{
    PresentationOffset = InT;
    return true;
}

bool USVFComponent::SVFClock_SnapPresentationOffset()
{
    return SVFClock_GetTime(&PresentationOffset);
}

bool USVFComponent::SVFClock_GetPresentationOffset(int64* OutTime)
{
    *OutTime = PresentationOffset;
    return true;
}

bool USVFComponent::SVFClock_SetScale(float ClockScale)
{
    m_ClockScale = ClockScale;
    if (SVFReader)
    {
        SVFReader->SetReaderClockScale(ClockScale);
    }
    return true;
}

bool USVFComponent::SVFClock_GetScale(float* OutScale)
{
    *OutScale = m_ClockScale;
    return true;
}

FSVFFrameInfo& USVFComponent::SVF_GetFrameInfo()
{
    SVFReader->GetFrameInfo(m_frameInfo);
    return m_frameInfo;
}

void USVFComponent::SVF_StartReader()
{
    SVFReader->Start();
}

float USVFComponent::SVF_GetAudioVolume()
{
    return SVFReader->GetAudioVolume();
}

void USVFComponent::SVF_SetAudioVolume(float volume)
{
    SVFReader->SetAudioVolume(volume);
}

void USVFComponent::ChangeDynamicMaterial(UMaterialInstanceDynamic* DynamicMaterialInstance)
{
    if (DynamicMaterialInstance == nullptr)
    {
        WarnSVF("Given DynamicMaterialInstance is null")
            return;
    }

    BaseMaterial = DynamicMaterialInstance->Parent;
    SetMaterial(0, DynamicMaterialInstance);
    DynInstance = DynamicMaterialInstance;
    DynInstance->SetTextureParameterValue(TEXT("Texture"), DynTexture);
}

bool USVFComponent::GetMovies(TArray<FString>& Files)
{
    FString MoviesDirPath = FPaths::ProjectContentDir() / "Movies";
    FPaths::NormalizeDirectoryName(MoviesDirPath);
    IFileManager& FileManager = IFileManager::Get();
    FString Ext("mp4");
    Ext = (Ext.Left(1) == ".") ? "*" + Ext : "*." + Ext;
    FString FinalPath = MoviesDirPath + "/" + Ext;
    FileManager.FindFiles(Files, *FinalPath, true, false);
    return true;
}

void USVFComponent::EnumAudioDevices(TArray<FAudioDeviceInfo>& deviceInfos)
{
#if ENGINE_MINOR_VERSION > 19 && PLATFORM_WINDOWS
    HDEVINFO devInfoSet = SetupDiGetClassDevs(&DEVINTERFACE_AUDIO_RENDER, 0, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT); // DIGCF_PRESENT

    DWORD deviceIndex = 0;
    SP_DEVICE_INTERFACE_DETAIL_DATA* interfaceDetail = nullptr;

    while (true) {
        SP_DEVICE_INTERFACE_DATA devInterface;
        devInterface.cbSize = sizeof(::SP_DEVICE_INTERFACE_DATA);
        if (!SetupDiEnumDeviceInterfaces(devInfoSet, 0,
            &DEVINTERFACE_AUDIO_RENDER, deviceIndex, &devInterface))
        {
            break;
        }

        DWORD size = 0;
        // See how large a buffer we require for the device interface details (ignore error, it should be returning ERROR_INSUFFICIENT_BUFFER
        SetupDiGetDeviceInterfaceDetail(devInfoSet, &devInterface, 0, 0, &size, 0);

        interfaceDetail = (SP_DEVICE_INTERFACE_DETAIL_DATA*)calloc(1, size);
        if (interfaceDetail)
        {
            interfaceDetail->cbSize = sizeof(::SP_DEVICE_INTERFACE_DETAIL_DATA);

            ::SP_DEVINFO_DATA devInfo;
            devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
            if (SetupDiGetDeviceInterfaceDetail(devInfoSet, &devInterface, interfaceDetail, size, 0, &devInfo)) {
                wchar_t friendlyName[2048] = {};
                DWORD propertyDataType;

                FAudioDeviceInfo deviceInfo;

                if (SetupDiGetDeviceRegistryProperty(devInfoSet, &devInfo, SPDRP_FRIENDLYNAME, &propertyDataType, (LPBYTE)friendlyName, 2048, 0))
                {
                    deviceInfo.Name = FString(friendlyName);
                }
                deviceInfo.Id = FString(interfaceDetail->DevicePath);

                deviceInfos.Add(deviceInfo);
                deviceIndex++;
            }
        }
        free(interfaceDetail);
    }
    if (devInfoSet)
    {
        SetupDiDestroyDeviceInfoList(devInfoSet);
    }
#endif
}

bool USVFComponent::GetVertices(TArray<FVector>& Vertices)
{
    if (!SceneProxy)
    {
        return false;
    }
    
    FSVFMeshSceneProxy* LocalSceneProxy = (FSVFMeshSceneProxy*)SceneProxy;
    LocalSceneProxy->GetPositionalVertices(Vertices, SVF_GetFrameInfo().vertexCount);

    return true;
}

#undef LogSVF
#undef WarnSVF
#undef FatalSVF
