#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "SVFClockInterface.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "SVFComponent.generated.h"

class ISVFSimpleInterface;

UCLASS(
    Blueprintable,
    BlueprintType,
    EditInlineNew,
    ClassGroup = UnrealSVF,
    meta = (BlueprintSpawnableComponent),
    hideCategories = (Input, Physics),
    showCategories = (Mobility),
    Config = UnrealSVF
)
class UNREALSVF_API USVFComponent : public UMeshComponent, public IInterface_CollisionDataProvider, public ISVFClockInterface
{
    GENERATED_BODY()

public:

    USVFComponent();

    virtual void OnRegister() override;

    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction) override;

    virtual void DestroyComponent(bool bPromoteChildren = false) override;

    bool IsFileOpened() const;

    ISVFSimpleInterface* GetSVFReader()
    {
        return SVFReader;
    }

    TSharedPtr<FFrameData> GetLastFrameData()
    {
        return LastFrameData;
    }

    // Returns the greater between file's max vertex count and user-defined default vertex count
    int32 GetMaxVertexCount()
    {
        return FMath::Max(DefaultVertexCount, FileInfo.MaxVertexCount);
    }

    // Returns the greater between file's max index count and user-defined default index count
    int32 GetMaxIndexCount()
    {
        return FMath::Max(DefaultIndexCount, FileInfo.MaxIndexCount);
    }

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    FORCEINLINE bool HasValidReader()
    {
        return SVFReader != NULL;
    }

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    bool SVF_IsPlaying();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_Play();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_Pause();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_Stop();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_SetPlayRate(float InPlayRate = 1.f);

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    float SVF_GetPlayRate();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    bool SVF_CanSeek();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    float SVF_GetCurrentPositionPercent();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    FTimespan SVF_GetCurrentPosition();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_SeekToPercent(float SeekToPercent);

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_SeekToTime(FTimespan ToTime);

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_SeekToFrame(int32 toFrameId);

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_Rewind();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    FTimespan SVF_GetDuration() const;

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    FSVFFileInfo SVF_FileInfo() const { return FileInfo; }

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_AsyncClose();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_Close();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_AsyncOpen(bool InPlayWhenReady);

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    bool SVF_Open(const FString& FileName, bool InStartPlayingImmediately = false);

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_StartReader();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    FSVFFrameInfo& SVF_GetFrameInfo();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    float SVF_GetAudioVolume();

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void SVF_SetAudioVolume(float volume);

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void EnumAudioDevices(TArray<FAudioDeviceInfo>& deviceInfos);

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    void ChangeDynamicMaterial(UMaterialInstanceDynamic* DynamicMaterialInstance);

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    static bool GetMovies(TArray<FString>& Files);

    UFUNCTION(BlueprintCallable, Category = "SVFComponent")
    bool GetVertices(TArray<FVector>& Vertices);

protected:

    UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = SVF, Meta = (ExposeOnSpawn = true))
    bool bAutoPlay = true;

    UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = SVF, Meta = (ExposeOnSpawn = true))
    int32 DefaultVertexCount = 20000;

    UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = SVF, Meta = (ExposeOnSpawn = true))
    int32 DefaultIndexCount = 60000;

    UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = SVF, Meta = (ExposeOnSpawn = true))
    bool bUpdateTextureLessOften;

    UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = SVF)
    int32 UpdateFrameInterval;

    UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = Debug)
    uint32 DisableUpdateMesh : 1;

    UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = Debug)
    uint32 DisableUpdateTexture : 1;

    UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = Debug)
    uint32 DisableGetNextFrame : 1;

    UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = Debug)
    uint32 DisableAll : 1;

    // Whether the settings from chosen preset should be used
    UPROPERTY(/*EditAnyWhere,*/ EditInstanceOnly, BlueprintReadOnly, Category = SVF)
    uint32 bOverride_PresetMode : 1;

    UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = SVF, Meta = (ExposeOnSpawn = true))
    bool bUseHardwareTextureCopy = true;

    // TODO: This doesn't work without cached mode
    // This uses significant amount of disk space and RAM, intended only for edit mode
    //UPROPERTY(EditAnyWhere, BlueprintReadonly, Category = SVF)
    //bool EnableSequencerSeek = false;

    // Float to be lerpable, but always casted to integer
    //UPROPERTY(EditAnyWhere, BlueprintReadWrite, Interp, Category = SVF)
    //float sequencerFrameID = 0.f;
    //int32 lastSequencerFrameID = 0;

    // Path to SVF-video file. The path MUST be relative to the "Content" project folder.
    UPROPERTY(EditAnyWhere, BlueprintReadWrite, Category = SVF, Meta = (ExposeOnSpawn = true, RelativeToGameContentDir))
    FFilePath RelativeFilePathFromContent;
    UPROPERTY(Meta = (RelativeToGameContentDir))
    FString RelativeFilePath_EditorPreview;

    UPROPERTY(/*EditAnyWhere,*/ EditInstanceOnly, BlueprintReadWrite, Category = SVF)
    EUSVFOpenPreset PresetMode = EUSVFOpenPreset::DefaultPreset_DropIfNeed;

    UPROPERTY(EditAnyWhere, BlueprintReadOnly, Category = SVF)
    FSVFOpenInfo OpenInfo;

    UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = SVF)
    FSVFFileInfo FileInfo;

    UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = SVF)
    FSVFStatus m_status;

    FSVFFrameInfo m_frameInfo;

    FString OpenedFilePath = TEXT("");

    bool IsWorldPlaying() const;

    static bool ValidateFilePath(const FString& InFilePath);

    void CloseCurrent(bool InAsync = false);
    bool OpenFilePath();
#if WITH_EDITOR
    bool GeneratePreview();
#endif
    void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

    void GenerateMesh();
    void UpdateMaterial();
    void UpdateMaterialEditor();
    UPROPERTY(Transient)
    UObject* SVFReaderObject = nullptr;

    ISVFSimpleInterface* SVFReader = nullptr;
    TSharedPtr<FFrameData> LastFrameData;
    EUSVFReaderState LastState = EUSVFReaderState::Unknown;
    bool bIsPlaying = false;
    bool bIsBeginPlayback = false;
    bool bUpdateTexture = true;

    UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = SVF)
    UTexture2D* DynTexture = nullptr;

    UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = SVF)
    UTexture2D* DynTexturePreview = nullptr;

    UPROPERTY(Transient)
    UMaterialInstanceDynamic* DynInstance = nullptr;
    UPROPERTY(Transient)
    UMaterialInterface* DefaultUnlitMaterial = nullptr;
    UPROPERTY(VisibleAnyWhere, BlueprintReadOnly, Category = SVF)
    UMaterialInterface* BaseMaterial = nullptr;

    FDelegateHandle PauseHandle;
    FDelegateHandle ResumeHandle;
    bool wasPlayingBeforeEnteringBackground;
    void HandleApplicationHasEnteredForeground();
    void HandleApplicationWillEnterBackground();

#if WITH_EDITOR
    void HandlePausePIE(bool bIsSimuloating);
    void HandleResumePIE(bool bIsSimulating);
#endif

public:
    // ~ ISVFClockInterface

    virtual bool SVFClock_Initialize() override { return true; }

    virtual bool SVFClock_GetPresentationTime(int64* OutTime) override;

    virtual bool SVFClock_GetTime(int64* OutTime) override;

    virtual bool SVFClock_Start() override;

    virtual bool SVFClock_Stop() override;

    virtual bool SVFClock_GetState(EUSVFClockState* OutState) override;

    virtual bool SVFClock_Shutdown() override;

    virtual bool SVFClock_SetPresentationOffset(int64 InT) override;

    virtual bool SVFClock_SnapPresentationOffset() override;

    virtual bool SVFClock_GetPresentationOffset(int64* OutTime) override;

    virtual bool SVFClock_SetScale(float ClockScale) override;

    virtual bool SVFClock_GetScale(float* OutScale) override;

protected:

    EUSVFClockState ClockState = EUSVFClockState::Stopped;
    float ClockCurrentTime = 0.f;
    float m_ClockScale = 1.f;
    int64 PresentationOffset = 0L;
    int32 PauseCounter = 0;
    const int32 PauseCounterResetValue = 5;

    // ~ END: ISVFClockInterface

public:

    // Collision data
    UPROPERTY(Instanced)
    class UBodySetup* MeshBodySetup;

    virtual int32 GetNumMaterials() const override;

    virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
    virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
    virtual bool WantsNegXTriMesh() override {
        return false;
    }

    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual class UBodySetup* GetBodySetup() override;

    // Ensure MeshBodySetup is allocated and configured
    void UpdateBodySetup();
    void UpdateCollision();

    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

    int32 LastVerticesNum = 0;
    int32 LastIndicesNum = 0;

};
