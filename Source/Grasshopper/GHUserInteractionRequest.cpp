// Fill out your copyright notice in the Description page of Project Settings.


#include "GHUserInteractionRequest.h"
#include "Misc/Guid.h"

UGHUserInteractionRequest::UGHUserInteractionRequest() : 
	Super()
    , requestId_(FGuid::NewGuid().ToString())
	, created_(FDateTime::Now())
{

}

FString UGHUserInteractionRequest::getRequestId() const
{
	return requestId_;
}

FDateTime UGHUserInteractionRequest::getCreatedTime() const
{
	return created_;
}