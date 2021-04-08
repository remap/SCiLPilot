// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SVFActor.generated.h"

UCLASS(HideCategories = (Input), ShowCategories = ("Input|MouseInput", "Input|TouchInput"), ComponentWrapperClass, Meta = (ChildCanTick))
class UNREALSVF_API ASVFActor : public AActor
{
    GENERATED_UCLASS_BODY()
public:

    /** Returns SVFComponent subobject **/
    class USVFComponent* GetSVFComponent() const { return SVFComponent; }

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;

    /**
    Can be called after calling DestroyActor on SVFActors to  force garbage collection
    to clean up SVFActor its SVFComponent. Should probably be called sparingly.
    */
    UFUNCTION(BlueprintCallable, Category = "SVFActor")
    static void UE4_ForceGarbageCollection();

    UFUNCTION(BlueprintCallable, Category = "SVFActor", Meta = (AllowPrivateAccess = "true", DisplayName = "Destroy SVF Component"))
    void DestroySVFComponent();

    UFUNCTION(BlueprintCallable, Category = "SVFActor", Meta = (AllowPrivateAccess = "true", DisplayName = "Set Mobility"))
    void SetRuntimeMeshMobility(EComponentMobility::Type NewMobility);

    UFUNCTION(BlueprintCallable, Category = "SVFActor", Meta = (AllowPrivateAccess = "true", DisplayName = "Get Mobility"))
    EComponentMobility::Type GetRuntimeMeshMobility();

private:
    UPROPERTY(Category = "SVFActor", VisibleAnywhere, BlueprintReadOnly, Meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|SVF", AllowPrivateAccess = "true"))
    class USVFComponent* SVFComponent;
};