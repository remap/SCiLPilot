// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Player.h"
#include "GameFramework/Actor.h"
#include "LaBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class LAINVENCION_API ULaBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
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

	GENERATED_BODY()
};
