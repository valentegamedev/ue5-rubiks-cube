// Fill out your copyright notice in the Description page of Project Settings.


#include "VRubiksPlayerController.h"

// Sets default values
AVRubiksPlayerController::AVRubiksPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AVRubiksPlayerController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = true;
	bEnableClickEvents = true;
}

