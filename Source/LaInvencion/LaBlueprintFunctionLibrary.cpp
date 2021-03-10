// Fill out your copyright notice in the Description page of Project Settings.


#include "LaBlueprintFunctionLibrary.h"

AActor* ULaBlueprintFunctionLibrary::getNetOwner(const AActor* a)
{
	if (a)
		return (AActor*)a->GetNetOwner();
	return nullptr;
}

UPlayer* ULaBlueprintFunctionLibrary::getNetOwningPlayer(AActor*a)
{
	if (a)
		return a->GetNetOwningPlayer();
	return nullptr;
}

AActor* ULaBlueprintFunctionLibrary::getActorOwner(AActor*a)
{
	if (a)
		return a->GetOwner();
	return nullptr;
}

APlayerController* ULaBlueprintFunctionLibrary::GetActorPlayerController(AActor* a)
{
	if (a) 
	{
		UPlayer *player = a->GetNetOwningPlayer();
		if (player && a->GetWorld())
		{
			return player->GetPlayerController(a->GetWorld());
		}
	}
		
	return nullptr;
}