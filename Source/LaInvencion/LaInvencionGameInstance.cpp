// Fill out your copyright notice in the Description page of Project Settings.


#include "LaInvencionGameInstance.h"
#include "DDLog.h"
#include "DDManager.h"

ConnectionStatus
ULaInvencionGameInstance::getConnectionStatus() const
{
    return connectionStatus_;
}

//void
//ULaInvencionGameInstance::setConnectionStatus(ConnectionStatus status)
//{
//    connectionStatus_ = status;
//}

bool
ULaInvencionGameInstance::HandleOpenCommand(const TCHAR * Cmd, FOutputDevice & Ar, UWorld * InWorld)
{
    DLOG_TRACE("Open level command received");
    
    if (Cmd)
        DLOG_TRACE("Cmd: {}", TCHAR_TO_ANSI(Cmd));

    if (InWorld)
        DLOG_TRACE("World: {}", TCHAR_TO_ANSI(*InWorld->GetMapName()));
    
    updateConnectionStatus(Connecting);
    
    return UGameInstance::HandleOpenCommand(Cmd, Ar, InWorld);
}

void
ULaInvencionGameInstance::LoadComplete(const float LoadTime, const FString & MapName)
{
    DLOG_TRACE("LoadComplete: time {} map name {}",
               LoadTime, TCHAR_TO_ANSI(*MapName));
    UWorld *world = GetWorld();
    
    // check if we really connected
    if (world)
    {
        ENetMode mode = world->GetNetMode();
        switch (mode) {
            case NM_Standalone:
                updateConnectionStatus(Disconnected);
                DLOG_TRACE("Client is not connected");
                break;
            case NM_Client:
                updateConnectionStatus(Connected);
                DLOG_TRACE("Client has connected to a server");
                break;
            case NM_ListenServer:
                updateConnectionStatus(Connected);
                DLOG_TRACE("Client is a listen server");
                break;
            case NM_DedicatedServer:
                updateConnectionStatus(Connected);
                DLOG_TRACE("Client is a dedicated server");
                break;
            default:
                // do nothing
                break;
        }
    }
}

void
ULaInvencionGameInstance::updateConnectionStatus(ConnectionStatus newStatus)
{
    if (connectionStatus_ != newStatus)
    {
        ConnectionStatus oldStatus = connectionStatus_;
        connectionStatus_ = newStatus;
        
        // notify observers
        OnConnectionStatusUpdated.Broadcast(oldStatus, connectionStatus_);
    }
}
