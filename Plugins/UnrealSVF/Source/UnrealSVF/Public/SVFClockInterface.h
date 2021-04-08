// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SVFTypes.h"
#include "UObject/Interface.h"
#include "SVFClockInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USVFClockInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 *
 */
class UNREALSVF_API ISVFClockInterface
{
    GENERATED_BODY()

        // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    virtual bool SVFClock_Initialize() PURE_VIRTUAL(ISVFClockInterface::Initialize, return false; );

    /** Returns the current presentation time (Time-PresentationOffset) */
    virtual bool SVFClock_GetPresentationTime(int64* OutTime) PURE_VIRTUAL(ISVFClockInterface::GetPresentationTime, return false; );

    /** Returns the clocks underlying time */
    virtual bool SVFClock_GetTime(int64* OutTime) PURE_VIRTUAL(ISVFClockInterface::GetTime, return false; );

    /** Starts the clock */
    virtual bool SVFClock_Start() PURE_VIRTUAL(ISVFClockInterface::Start, return false; );

    /** Stops the clock */
    virtual bool SVFClock_Stop() PURE_VIRTUAL(ISVFClockInterface::Stop, return false; );

    /** Returns the clocks current state */
    virtual bool SVFClock_GetState(EUSVFClockState* OutState) PURE_VIRTUAL(ISVFClockInterface::GetState, return false; );

    /** Shutdown the clock */
    virtual bool SVFClock_Shutdown() PURE_VIRTUAL(ISVFClockInterface::Shutdown, return false; );

    /** Sets the presentation offset. This value is added to the clocks time */
    virtual bool SVFClock_SetPresentationOffset(int64 InT) PURE_VIRTUAL(ISVFClockInterface::SetPresentationOffset, return false; );

    /** Sets the presentation offset to the clocks current time. This basically resets the clocks relative time back to 0. */
    virtual bool SVFClock_SnapPresentationOffset() PURE_VIRTUAL(ISVFClockInterface::SnapPresentationOffset, return false; );

    /** Returns the current presentation offset */
    virtual bool SVFClock_GetPresentationOffset(int64* OutTime) PURE_VIRTUAL(ISVFClockInterface::GetPresentationOffset, return false; );

    /** Sets the clock scaling factor (making the clock run faster or slower) */
    virtual bool SVFClock_SetScale(float InScale) PURE_VIRTUAL(ISVFClockInterface::SetScale, return false; );

    /** Returns the clock's current scaling factor */
    virtual bool SVFClock_GetScale(float* OutScale) PURE_VIRTUAL(ISVFClockInterface::GetScale, return false; );

};
