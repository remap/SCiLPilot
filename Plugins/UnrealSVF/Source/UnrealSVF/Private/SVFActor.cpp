// Copyright (C) Microsoft Corporation. All rights reserved.

#include "SVFActor.h"
#include "UnrealSVF.h"
#include "SVFComponent.h"
#include "Engine/Engine.h"
#include "Runtime/Launch/Resources/Version.h"

ASVFActor::ASVFActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if ENGINE_MINOR_VERSION < 24
    bCanBeDamaged = false;
#else
    SetCanBeDamaged(false);
#endif

    SVFComponent = CreateDefaultSubobject<USVFComponent>(TEXT("SVFComponent"));
    SVFComponent->SetCollisionProfileName(FName(TEXT("BlockAll")));
    SVFComponent->Mobility = EComponentMobility::Static;

    // SVF content is captured in mm scale, this converts it to UE4 scale
    SVFComponent->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));

#if ENGINE_MINOR_VERSION > 19
    SVFComponent->SetGenerateOverlapEvents(false);
#else
    SVFComponent->bGenerateOverlapEvents = false;
#endif

    RootComponent = SVFComponent;
}

void ASVFActor::OnConstruction(const FTransform& Transform)
{
}

void ASVFActor::BeginPlay()
{
    Super::BeginPlay();

    bool bIsGameWorld = GetWorld() && GetWorld()->IsGameWorld() && !GetWorld()->IsPreviewWorld() && !GetWorld()->IsEditorWorld();

    if (SVFComponent)
    {
    }
}

void ASVFActor::UE4_ForceGarbageCollection()
{
    GEngine->ForceGarbageCollection();
}

void ASVFActor::DestroySVFComponent()
{
    if (SVFComponent)
    {
        SVFComponent->DestroyComponent();
    }
}


void ASVFActor::SetRuntimeMeshMobility(EComponentMobility::Type NewMobility)
{
    if (SVFComponent)
    {
        SVFComponent->SetMobility(NewMobility);
    }
}

EComponentMobility::Type ASVFActor::GetRuntimeMeshMobility()
{
    if (SVFComponent)
    {
        return SVFComponent->Mobility;
    }
    return EComponentMobility::Static;
}
