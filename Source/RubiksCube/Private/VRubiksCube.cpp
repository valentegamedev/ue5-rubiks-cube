// Fill out your copyright notice in the Description page of Project Settings.


#include "VRubiksCube.h"
#include "FCTween.h"
#include "VRubiksPiece.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

// Sets default values
AVRubiksCube::AVRubiksCube()
{
	PrimaryActorTick.bCanEverTick = false;

	ScrambleCounter = 0;
	Steps = 0;
	
	Size = 3; //Set default cube size
	bIsInteractionEnabled = true;

	ClickedPiece = nullptr;
    bIsCameraMoving = false;
	
	DummySceneComponent = CreateDefaultSubobject <USceneComponent>(FName("Dummy Root"));
	SetRootComponent(DummySceneComponent);

	RotatorSceneComponent = CreateDefaultSubobject<USceneComponent>(FName("Piece Rotator"));
	RotatorSceneComponent->SetupAttachment(GetRootComponent());

    SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(FName("Spring Arm"));
    SpringArmComponent->SetupAttachment(GetRootComponent());
    
    SpringArmComponent->bDoCollisionTest = false;
    SpringArmComponent->bInheritPitch= false;
    SpringArmComponent->bInheritRoll = false;
    SpringArmComponent->bInheritYaw = false;
    
    CameraComponent = CreateDefaultSubobject<UCameraComponent>(FName("Camera"));
    CameraComponent->SetupAttachment(SpringArmComponent);
}

void AVRubiksCube::SetSize(int32 NewSize)
{
	Size = FMath::Clamp(NewSize, 2, 16);
	Build();
}

int32 AVRubiksCube::GetSize()
{
	return Size;
}

void AVRubiksCube::Build()
{
	//Clear any tweening animations
	FCTween::ClearActiveTweens();
	SetActorScale3D(FVector::OneVector);
	bIsScrambling = false;
	bIsAnimating = false;

	//Recreate the cube
	OnCubeChanged.Broadcast(0);
	DestroyPieces();
	GeneratePieces();

	//Add a little scaling animation
	FCTween::Play(
	1.05f,
	1.0f,
	[&](float t)
	{
		SetActorScale3D(FVector::OneVector * t);
	},
	0.3f,
	EFCEase::OutBack);
}

// Called when the game starts or when spawned
void AVRubiksCube::BeginPlay()
{
	Super::BeginPlay();
	Build();
}

void AVRubiksCube::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	
	if (UEnhancedInputComponent* PlayerEnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (InteractAction)
		{
			PlayerEnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &AVRubiksCube::Input_Interact);
			PlayerEnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &AVRubiksCube::Input_EndInteract);
		}
	}
}

void AVRubiksCube::DestroyPieces()
{
	PiecesToRotate.Empty();
	for (int32 x = 0; x < Pieces.Num(); x++) {
		Pieces[x]->Destroy();
	}

	Pieces.Empty();
	RotatorSceneComponent->SetRelativeRotation(FRotator(0, 0, 0));
}

void AVRubiksCube::GeneratePieces()
{
	//Set new cube size
	Steps = 0;
	UWorld * World = GetWorld();
	
	//Create cube based on its size
	for (int32 i = 0; i < Size; i++) {
		for (int32 j = 0; j < Size; j++) {
			for (int32 k = 0; k < Size; k++) {
				//Spawn only pieces that belongs to a wall
				if (World && (i == 0 || i == Size-1 || j == 0 || j == Size-1 || k ==0 || k == Size-1)) { 
					//Spawn a rubiks piece
					FActorSpawnParameters Params;
					Params.Owner = this;
					AVRubiksPiece * NewPiece = World->SpawnActor<AVRubiksPiece>(PieceClass, FVector::ZeroVector, FRotator(0.0f, 0.0f, 0.0f), Params);
					NewPiece->SetActorLocation(FVector(NewPiece->GetSideWidth() * j, NewPiece->GetSideWidth() * i, NewPiece->GetSideWidth() * k));
					NewPiece->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform, NAME_None);
					NewPiece->Tags.Add(PIECE_TAG);
					
					UpdatePieceMaterials(NewPiece, j, i, k);
					Pieces.Add(NewPiece);
				}
			}
		}
	}

	//Set PieceRotator and camera's arm to the center of the new cube
	float CubePieceSideWidth = Pieces[0]->GetSideWidth();
	float CubeSideWidth = CubePieceSideWidth * GetSize();
	float CubeSideCenter = (CubeSideWidth / 2) - (CubePieceSideWidth / 2); //Offset it a little because the origin of the piece it's in the center of the mesh
	FVector CubeCenter = FVector(CubeSideCenter);
	
	RotatorSceneComponent->SetRelativeLocation(CubeCenter);
	SpringArmComponent->SetRelativeLocation(CubeCenter);
	SpringArmComponent->SetRelativeRotation(FRotator(-30, 0, 0));
	SpringArmComponent->AddWorldRotation(FRotator(0, -45, 0));
	SpringArmComponent->TargetArmLength = CubeSideWidth * 2;
}

void AVRubiksCube::Scramble()
{
	//Choose a random group based on a axis
	int32 Random = FMath::RandRange(0, 2);
	EPieceGroup RotationGroupAxis = EPieceGroup::X;
	FRotator Angle = FRotator::ZeroRotator;
	switch (Random)
	{
	case 0:
		RotationGroupAxis = EPieceGroup::X;
		Angle = FRotator(0, 0, 90);
		break;
	case 1:
		RotationGroupAxis = EPieceGroup::Y;
		Angle = FRotator(90, 0, 0);
		break;
	case 2:
		RotationGroupAxis = EPieceGroup::Z;
		Angle = FRotator(0, 90, 0);
		break;
	default:
		break;
	}

	//Choose a random piece for the group
	Random = FMath::RandRange(0, Pieces.Num() - 1);
	AVRubiksPiece * RandomPiece = Pieces[Random];

	//Choose a random direction
	Random = FMath::RandRange(0, 1);
	if (Random) {
		Angle *= -1;
	}
	
	//Scramble!
	RotateGroup(RandomPiece, RotationGroupAxis, Angle, .25f);
}

void AVRubiksCube::UpdatePieceMaterials(AVRubiksPiece* Piece, int32 X, int32 Y, int32 Z)
{
	if (X == 0){ //Front
		Piece->SetFaceMaterial(0, FaceMaterials[0]);
	}
	if (X == (Size-1)) { //Back
		Piece->SetFaceMaterial(1, FaceMaterials[1]);
	}
	
	if (Y == 0) { //Left
		Piece->SetFaceMaterial(2, FaceMaterials[2]);
	}
	if (Y == (Size-1)) { //Right
		Piece->SetFaceMaterial(3, FaceMaterials[3]);
	}

	if (Z == (Size-1)) { //Up
		Piece->SetFaceMaterial(4, FaceMaterials[4]);
	}
	if (Z == 0) { //Down
		Piece->SetFaceMaterial(5, FaceMaterials[5]);
	}
}

void AVRubiksCube::Scramble(int32 TotalSteps)
{
	//Not scramble if it is already scrambling
	if (bIsScrambling || bIsAnimating) {
		return;
	}

	//Start scramble chain
	ScrambleCounter = TotalSteps;
	bIsScrambling = true;
	Steps = 0;
	OnCubeChanged.Broadcast(Steps);
	Scramble();
}

int32 AVRubiksCube::GetSteps()
{
	return Steps;
}

bool AVRubiksCube::IsCubeSolved()
{
	AVRubiksPiece* Piece = Pieces[0];
	FVector ForwardVector = Piece->GetActorForwardVector();
	FVector UpVector = Piece->GetActorUpVector();
	FVector RightVector = Piece->GetActorRightVector();
	bool bIsSolved = true;
	for (int32 x = 1; x < Pieces.Num(); x++) {
		AVRubiksPiece* PieceToCompare = Pieces[x];
		if (!ForwardVector.Equals(PieceToCompare->GetActorForwardVector(), 0.01f)
			|| !UpVector.Equals(PieceToCompare->GetActorUpVector(), 0.01f)
			|| !RightVector.Equals(PieceToCompare->GetActorRightVector(), 0.01f)) {
				bIsSolved = false;
			break;
		}
	}
	
	return bIsSolved;
}

void AVRubiksCube::Input_Interact(const FInputActionValue& InputActionValue)
{
	if (bIsScrambling) {
		return;
	}
	
	//Get player controller
	APlayerController * PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	
	if (bIsInteractionEnabled && !bIsAnimating){ //Start movement detection
		FVector MouseWorldPosition;
		FVector MouseWorldDirection;

		//Project mouse position from screen to 3d world
		PC->DeprojectMousePositionToWorld(MouseWorldPosition, MouseWorldDirection);

		//Calculating the end of the ray
		FVector TraceEnd = MouseWorldPosition + (MouseWorldDirection * 100000); //long ray

		FHitResult HitResult;
		FCollisionQueryParams TraceParams;
		TraceParams.bTraceComplex = false;

		//Trace a ray to find a Rubiks piece
		if (GetWorld()->LineTraceSingleByChannel(HitResult, MouseWorldPosition, TraceEnd, ECC_Visibility, TraceParams) && !bIsCameraMoving) { // && !IsCubeSolved()
			//Verifies if the object is a rubiks piece
			if (HitResult.GetActor()->Tags.Contains(PIECE_TAG)) {
				if (ClickedPiece == nullptr) {
					ClickedPiece = Cast<AVRubiksPiece>(HitResult.GetActor());
					ClickedWorldPosition = HitResult.ImpactPoint;
					ClickedWorldNormal = HitResult.ImpactNormal;
				} else { //Already dragging the mouse over a piece
					FVector Direction = HitResult.ImpactPoint - ClickedWorldPosition;
					int32 DragDistance = Direction.Size();
					if (DragDistance > DRAG_DISTANCE) { //Detect movement
						FVector NormalizedDirection = Direction.GetSafeNormal();
						bIsInteractionEnabled = false;
						//Start the rotation process
						Steps++;
						RotateFromPiece(ClickedPiece, ClickedWorldNormal, NormalizedDirection);
					}
				}
			}			
        } else if (ClickedPiece == nullptr){ //Camera movement
            bIsCameraMoving = true;
			//Get mouse movement axis for camera rotation
        	FVector CameraMovement;
			PC->GetInputMouseDelta(CameraMovement.X, CameraMovement.Y);
			//Rotate the camera
        	SpringArmComponent->AddWorldRotation(FRotator(0, CameraMovement.X * 2, 0));
        	SpringArmComponent->AddRelativeRotation(FRotator(CameraMovement.Y * 2, 0, 0));
        	//Clamp Y rotation
        	FRotator SpringArmRotation = SpringArmComponent->GetComponentRotation();
        	SpringArmComponent->SetRelativeRotation(FRotator(FMath::Clamp(SpringArmRotation.Pitch, -CAMERA_Y_ANGLE_LIMIT, CAMERA_Y_ANGLE_LIMIT), SpringArmRotation.Yaw, SpringArmRotation.Roll));
        }
	}
}

void AVRubiksCube::Input_EndInteract(const FInputActionValue& InputActionValue)
{
	//Reset state
	bIsInteractionEnabled = true;
	bIsCameraMoving = false;
	ClickedPiece = nullptr;
	ClickedWorldPosition = FVector::ZeroVector;
	ClickedWorldNormal = FVector::ZeroVector;
}

void AVRubiksCube::RotateFromPiece(AVRubiksPiece * Piece, FVector Normal, FVector Direction)
{ 
	if (Normal.Equals(FVector::UpVector)) { //Top Face
		if(FMath::Abs(Direction.X) > 0.9f) {
			RotateGroup(Piece, EPieceGroup::Y, FRotator(FMath::Sign(Direction.X) * -90, 0, 0));
		} else if (FMath::Abs(Direction.Y) > 0.9f) {
			RotateGroup(Piece, EPieceGroup::X, FRotator(0, 0, FMath::Sign(Direction.Y) * 90));
		}
	}
	else if (Normal.Equals(FVector::DownVector)) { //Bottom Face
		if(FMath::Abs(Direction.X) > 0.9f) {
			RotateGroup(Piece, EPieceGroup::Y, FRotator(FMath::Sign(Direction.X) * 90, 0, 0));
		} else if (FMath::Abs(Direction.Y) > 0.9f) {
			RotateGroup(Piece, EPieceGroup::X, FRotator(0, 0, FMath::Sign(Direction.Y) * -90));
		}
	}
	else if (Normal.Equals(FVector::ForwardVector)) { //Back face
		if(FMath::Abs(Direction.Z) > 0.9f) {
			RotateGroup(Piece, EPieceGroup::Y, FRotator(FMath::Sign(Direction.Z) * 90, 0, 0));
		} else if (FMath::Abs(Direction.Y) > 0.9f) {
			RotateGroup(Piece, EPieceGroup::Z, FRotator(0, FMath::Sign(Direction.Y) * 90, 0));
		}
	}
	else if (Normal.Equals(FVector::BackwardVector)) { //Front Face
		if(FMath::Abs(Direction.Z) > 0.9f) {
			RotateGroup(Piece, EPieceGroup::Y, FRotator(FMath::Sign(Direction.Z) * -90, 0, 0));
		} else if (FMath::Abs(Direction.Y) > 0.9f) {
			RotateGroup(Piece, EPieceGroup::Z, FRotator(0, FMath::Sign(Direction.Y) * -90, 0));
		}
	}
	else if (Normal.Equals(FVector::LeftVector)) { //Left Face
		if(FMath::Abs(Direction.Z) > 0.9f) {
			RotateGroup(Piece, EPieceGroup::X, FRotator(0, 0, FMath::Sign(Direction.Z) * 90));
		} else if (FMath::Abs(Direction.X) > 0.9f) {
			RotateGroup(Piece, EPieceGroup::Z, FRotator(0, FMath::Sign(Direction.X) * 90, 0));
		}
	}
	else if (Normal.Equals(FVector::RightVector)) { //Right Face
		if(FMath::Abs(Direction.Z) > 0.9f) {
			RotateGroup(Piece, EPieceGroup::X, FRotator(0, 0, FMath::Sign(Direction.Z) * -90));
		} else if (FMath::Abs(Direction.X) > 0.9f) {
			RotateGroup(Piece, EPieceGroup::Z, FRotator(0, FMath::Sign(Direction.X) * -90, 0));
		}
	}
    
}

void AVRubiksCube::RotateGroup(AVRubiksPiece * Piece, EPieceGroup GroupAxis, FRotator Rotation, float Speed)
{
	//Clean array of the pieces that will rotate
	PiecesToRotate.Empty();

	//Remove parent from all pieces
	for (int32 x = 0; x < Pieces.Num(); x++) {
		Pieces[x]->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	//Reset rotation from PieceRotator
	RotatorSceneComponent->SetRelativeRotation(FRotator(0, 0, 0));
	//Add all pieces from the same group as the given piece to the PiecesToRotate array (used fabs just to prevent possible minimal erros)
	for (int32 x = 0; x < Pieces.Num(); x++) {
		if (GroupAxis == EPieceGroup::X && FMath::Abs(Pieces[x]->GetActorLocation().X - Piece->GetActorLocation().X) < 5.0f) {
			PiecesToRotate.Add(Pieces[x]);
		} else if (GroupAxis == EPieceGroup::Y && FMath::Abs(Pieces[x]->GetActorLocation().Y - Piece->GetActorLocation().Y) < 5.0f) {
			PiecesToRotate.Add(Pieces[x]);
		} else if (GroupAxis == EPieceGroup::Z && FMath::Abs(Pieces[x]->GetActorLocation().Z - Piece->GetActorLocation().Z) < 5.0f) {
			PiecesToRotate.Add(Pieces[x]);
		}
	}

	//Set all the pieces to rotate as child of the PieceRotator
	for (int32 x = 0; x < PiecesToRotate.Num(); x++) {
		PiecesToRotate[x]->AttachToComponent(RotatorSceneComponent, FAttachmentTransformRules::KeepWorldTransform, NAME_None);
	}

	//Rotate PieceRotator
	bIsAnimating = true;
	ClickedPiece = nullptr;
	ClickedWorldNormal = FVector::ZeroVector;
	ClickedWorldPosition = FVector::ZeroVector;
	
	FCTween::Play(
	FRotator(0, 0, 0).Quaternion(),
	Rotation.Quaternion(),
	[&](FQuat t)
	{
		RotatorSceneComponent->SetWorldRotation(t.Rotator());
	},
	Speed,
	EFCEase::OutBack)->SetOnComplete([&]() {
		if (!bIsScrambling) {
			bIsAnimating = false;
			bIsInteractionEnabled = true;
			OnCubeChanged.Broadcast(GetSteps());

			if(IsCubeSolved())
			{
				OnCubeSolved.Broadcast();
			}
		}
		else {
			ScrambleCounter--;
			if (ScrambleCounter >= 0) {
				Scramble();
			}
			else {
				bIsScrambling = false;
				bIsAnimating = false;
				bIsInteractionEnabled = true;
			}
		}
	});
}