// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "LaInvencionGameInstance.generated.h"

UENUM(BlueprintType)
enum ConnectionStatus
{
  Disconnected  UMETA(DisplayName = "Disconnected"),
  Connecting    UMETA(DisplayName = "Connecting"),
  Connected     UMETA(DisplayName = "Connected"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGameInstanceConnectionStatusUpdate, ConnectionStatus, oldStatus, ConnectionStatus, newStatus);

/**
 * 
 */
UCLASS()
class LAINVENCION_API ULaInvencionGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
    
    bool HandleOpenCommand(const TCHAR * Cmd,
                           FOutputDevice & Ar,
                           UWorld * InWorld) override;
    void LoadComplete(const float LoadTime,
                      const FString & MapName) override;
    
    UFUNCTION(BlueprintCallable)
    ConnectionStatus getConnectionStatus() const;
    
//    UFUNCTION(BlueprintCallable)
//    void setConnectionStatus(ConnectionStatus status);
    
    UPROPERTY(BlueprintAssignable)
    FGameInstanceConnectionStatusUpdate OnConnectionStatusUpdated;
    
private:
    ConnectionStatus connectionStatus_;
    
    void updateConnectionStatus(ConnectionStatus newStatus);

};
