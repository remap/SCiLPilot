// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SVFTypes.h"
#include "UObject/Interface.h"
#include "SVFCallbackInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USVFCallbackInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 *
 */
class UNREALSVF_API ISVFCallbackInterface
{
    GENERATED_BODY()

        // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    // ~ ISVFReaderStateCallback interface

    /** Method called when reader changes state */
    virtual bool OnStateChange(EUSVFReaderState oldState, EUSVFReaderState newState) PURE_VIRTUAL(ISVFCallbackInterface::OnStateChange, return true; );

    // ~ End: ISVFReaderStateCallback


    // ~ ISVFReaderInternalStateCallback interface

    /** Method called when reader encounters internal error */
    virtual void OnError(const FString& ErrorString, int32 hrError) PURE_VIRTUAL(ISVFCallbackInterface::OnError, );

    /** Method called when EOS in the reading thread */
    virtual void OnEndOfStream(uint32 remainingVideoSampleCount) PURE_VIRTUAL(ISVFCallbackInterface::OnEndOfStream, );

    // ~ End: ISVFReaderInternalStateCallback


    virtual void UpdateSVFStatistics(uint32 unsuccessfulReadFrameCount, uint32 lastReadFrame, uint32 droppedFrameCount) PURE_VIRTUAL(ISVFCallbackInterface::UpdateSVFStatistics, );

};
