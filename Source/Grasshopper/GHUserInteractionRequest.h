// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GHUserInteractionRequest.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class GRASSHOPPER_API UGHUserInteractionRequest : public UObject
{
	GENERATED_BODY()
	
public:
	UGHUserInteractionRequest();

	UFUNCTION(BlueprintCallable)
	FString getRequestId() const;

	UFUNCTION(BlueprintCallable)
	FDateTime getCreatedTime() const;

private:
	FString requestId_;
	FDateTime created_;
};
