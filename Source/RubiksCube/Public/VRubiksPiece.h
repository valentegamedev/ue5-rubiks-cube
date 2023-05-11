// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRubiksPiece.generated.h"

UCLASS()
class RUBIKSCUBE_API AVRubiksPiece : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rubiks", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* StaticMeshComponent;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:	
	// Sets default values for this actor's properties
	AVRubiksPiece();

	UFUNCTION(BlueprintCallable, Category = "Rubiks")
	void SetFaceMaterial(int32 Index, UMaterialInstance* Material);

	UFUNCTION(BlueprintPure, Category = "Rubiks")
	float GetSideWidth();
	
};
