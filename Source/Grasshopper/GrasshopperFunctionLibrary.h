// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Player.h"
#include "GameFramework/Actor.h"
#include "GrasshopperFunctionLibrary.generated.h"

DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FActorComparator, const AActor*, actorA, const AActor*, actorB);

/**
 * 
 */
UCLASS()
class GRASSHOPPER_API UGrasshopperFunctionLibrary : public UBlueprintFunctionLibrary
{
	
public:
	UFUNCTION(BlueprintCallable)
	static AActor* getNetOwner(const AActor *a);

	UFUNCTION(BlueprintCallable)
	static UPlayer* getNetOwningPlayer(AActor *a);

	UFUNCTION(BlueprintCallable)
	static AActor* getActorOwner(AActor *a);

	UFUNCTION(BlueprintCallable)
	static APlayerController* GetActorPlayerController(AActor* a);

	UFUNCTION(BlueprintCallable)
	static FString getCodeCommit();

	UFUNCTION(BlueprintCallable)
	static FString getGitDescribe();

	UFUNCTION(BlueprintCallable)
	static FString getCodeBranch();

	UFUNCTION(BlueprintCallable)
	static void sortObjectArray(const TArray<AActor*>& array, TArray<AActor*>& outArray, FActorComparator cmp);

	GENERATED_BODY()
};
