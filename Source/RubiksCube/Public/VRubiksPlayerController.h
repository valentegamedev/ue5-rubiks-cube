// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRubiksPlayerController.generated.h"

UCLASS()
class RUBIKSCUBE_API AVRubiksPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	// Sets default values for this character's properties
	AVRubiksPlayerController();

protected:
	
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
};

