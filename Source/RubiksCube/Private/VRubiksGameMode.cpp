// Fill out your copyright notice in the Description page of Project Settings.


#include "VRubiksGameMode.h"

#include "VRubiksPlayerController.h"

AVRubiksGameMode::AVRubiksGameMode()
{
	PlayerControllerClass = AVRubiksPlayerController::StaticClass();
}
