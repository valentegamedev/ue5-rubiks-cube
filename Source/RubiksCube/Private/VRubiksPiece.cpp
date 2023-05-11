// Fill out your copyright notice in the Description page of Project Settings.


#include "VRubiksPiece.h"

// Sets default values
AVRubiksPiece::AVRubiksPiece()
{
	PrimaryActorTick.bCanEverTick = false;
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh"));
	SetRootComponent(StaticMeshComponent);
}

void AVRubiksPiece::SetFaceMaterial(int32 Index, UMaterialInstance* Material)
{
	StaticMeshComponent->SetMaterial(Index, Material);
}

// Called when the game starts or when spawned
void AVRubiksPiece::BeginPlay()
{
	Super::BeginPlay();
}

float AVRubiksPiece::GetSideWidth()
{
	return StaticMeshComponent->GetStaticMesh()->GetBounds().BoxExtent.X*2;
}


