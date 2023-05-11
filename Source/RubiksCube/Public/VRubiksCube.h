// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRubiksCube.generated.h"

#define DRAG_DISTANCE 15
#define CAMERA_Y_ANGLE_LIMIT 65
#define PIECE_TAG "PIECE_TAG"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCubeChangedSignature, int32, Steps);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCubeSolvedSignature);

UENUM(BlueprintType)
enum EPieceGroup
{
	X UMETA(DisplayName = "Pieces with same X"),
	Y UMETA(DisplayName = "Pieces with same Y"),
	Z UMETA(DisplayName = "Pieces with same Z")
};

class AVRubiksPiece;

UCLASS()
class RUBIKSCUBE_API AVRubiksCube : public APawn
{
	GENERATED_BODY()

	UPROPERTY()
	TArray <AVRubiksPiece*> Pieces;

	UPROPERTY()
	TArray <AVRubiksPiece*> PiecesToRotate;

	UPROPERTY()
	AVRubiksPiece * ClickedPiece;
	
	FVector ClickedWorldPosition;
	
	FVector ClickedWorldNormal;
	
	bool bIsInteractionEnabled;
	
    bool bIsAnimating;
	
    bool bIsCameraMoving;
	
	int32 ScrambleCounter;
	
	int32 Steps;
	
	bool bIsScrambling;

	UPROPERTY(EditAnywhere, BlueprintGetter=GetSize, BlueprintSetter=SetSize, Category = "Rubiks", meta = (AllowPrivateAccess = "true"))
	int32 Size;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubiks", meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rubiks", meta = (AllowPrivateAccess = "true"))
	class UInputAction * InteractAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rubiks", meta = (AllowPrivateAccess = "true"))
	class USceneComponent * DummySceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rubiks", meta = (AllowPrivateAccess = "true"))
	class USceneComponent * RotatorSceneComponent;
    
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rubiks", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent * SpringArmComponent;
    
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rubiks", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent * CameraComponent;

	void Scramble();
	
	void DestroyPieces();
	
	void GeneratePieces();
	
	void UpdatePieceMaterials(AVRubiksPiece* Piece, int32 X, int32 Y, int32 Z);
	
	void RotateFromPiece(AVRubiksPiece * Piece, FVector Normal, FVector Direction);
	
	void RotateGroup(AVRubiksPiece * Piece, EPieceGroup GroupAxis, FRotator Rotation, float Speed = 0.4f);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

public:

	UPROPERTY(BlueprintAssignable, Category = "Rubiks")
	FOnCubeChangedSignature OnCubeChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Rubiks")
	FOnCubeSolvedSignature OnCubeSolved;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rubiks")
	TSubclassOf<AVRubiksPiece> PieceClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rubiks")
	TArray<UMaterialInstance*> FaceMaterials;
	
	// Sets default values for this actor's properties
	AVRubiksCube();

	UFUNCTION(BlueprintSetter, Category = "Rubiks")
	void SetSize(int32 NewSize);
	
	UFUNCTION(BlueprintGetter, Category = "Rubiks")
	int32 GetSize();
	
	UFUNCTION(BlueprintCallable, Category = "Rubiks")
	void Build();
	
	UFUNCTION(BlueprintCallable, Category = "Rubiks")
	void Scramble(int32 TotalSteps);

	UFUNCTION(BlueprintPure, Category = "Rubiks")
	int32 GetSteps();

	UFUNCTION(BlueprintPure, Category = "Rubiks")
	bool IsCubeSolved();

	//Input functions

	UFUNCTION()
	void Input_Interact(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void Input_EndInteract(const FInputActionValue& InputActionValue);
};
