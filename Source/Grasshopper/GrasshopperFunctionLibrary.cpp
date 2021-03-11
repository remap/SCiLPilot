// Fill out your copyright notice in the Description page of Project Settings.


#include "GrasshopperFunctionLibrary.h"

AActor* UGrasshopperFunctionLibrary::getNetOwner(const AActor* a)
{
	if (a)
		return (AActor*)a->GetNetOwner();
	return nullptr;
}

UPlayer* UGrasshopperFunctionLibrary::getNetOwningPlayer(AActor*a)
{
	if (a)
		return a->GetNetOwningPlayer();
	return nullptr;
}

AActor* UGrasshopperFunctionLibrary::getActorOwner(AActor*a)
{
	if (a)
		return a->GetOwner();
	return nullptr;
}

APlayerController* UGrasshopperFunctionLibrary::GetActorPlayerController(AActor* a)
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