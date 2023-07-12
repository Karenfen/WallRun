// Copyright Epic Games, Inc. All Rights Reserved.

#include "WallRunCharacter.h"
#include "WallRunProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AWallRunCharacter

AWallRunCharacter::AWallRunCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.
}

void AWallRunCharacter::Tick(float Deltatime)
{
	Super::Tick(Deltatime);

	if (bIsWallRunning)
	{
		UpdateWallRun();
	}

	CameraTiltTimeline.TickTimeline(Deltatime);

	if (IsMustDie())
	{
		Die();
	}
}

void AWallRunCharacter::Jump()
{
	if (bIsWallRunning)
	{
		FVector JumpDirrection = FVector::ZeroVector;

		if (CurrentWallRunSide == WallRunSide::RIGHT)
		{
			JumpDirrection = FVector::CrossProduct(CurrentWallRunDirection, FVector::UpVector).GetSafeNormal();
		}
		else
		{
			JumpDirrection = FVector::CrossProduct(FVector::UpVector, CurrentWallRunDirection).GetSafeNormal();
		}

		JumpDirrection += FVector::UpVector;
		float jumpVelocity = GetCharacterMovement()->JumpZVelocity;

		if (bIsBoost)
		{
			JumpDirrection += CurrentWallRunDirection;
		}
		
		LaunchCharacter(jumpVelocity * JumpDirrection.GetSafeNormal(), false, true);
		StopWallRun();
	}
	else
	{
		Super::Jump();
	}
}

void AWallRunCharacter::Die()
{
	StopWallRun();

	SetActorLocation(checpoint);
	GetController()->SetControlRotation(startRatate);
}

void AWallRunCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	GetCharacterMovement()->SetPlaneConstraintEnabled(true);

	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AWallRunCharacter::OnCharacterCapsulHit);

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// setup camera tilt
	if (IsValid(CameraTiltCurv))
	{
		FOnTimelineFloat TimeLineCallBack;
		TimeLineCallBack.BindUFunction(this, FName("UpdateCameraTilt"));
		CameraTiltTimeline.AddInterpFloat(CameraTiltCurv, TimeLineCallBack);
	}

	// set velosity for boost
	standartSpeed = GetCharacterMovement()->MaxWalkSpeed;
	boostSpeed = standartSpeed * BoostScale;

	// set start point
	checpoint = GetActorLocation();
	startRatate = GetControlRotation();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AWallRunCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AWallRunCharacter::OnFire);

	// Bind boost event
	PlayerInputComponent->BindAction("Boost", IE_Pressed, this, &AWallRunCharacter::BoostActivate);
	PlayerInputComponent->BindAction("Boost", IE_Released, this, &AWallRunCharacter::BoostEnd);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AWallRunCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AWallRunCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AWallRunCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AWallRunCharacter::LookUpAtRate);
}

void AWallRunCharacter::OnFire()
{
	// try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			const FRotator SpawnRotation = GetControlRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

			// spawn the projectile at the muzzle
			World->SpawnActor<AWallRunProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
		}
	}

	// try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AWallRunCharacter::MoveForward(float Value)
{
	forwardAxis = Value;

	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector() * BoostScale, Value);
	}
}

void AWallRunCharacter::MoveRight(float Value)
{
	rightAxis = Value;

	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AWallRunCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AWallRunCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AWallRunCharacter::OnCharacterCapsulHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bIsWallRunning)
	{
		return;
	}

	if (!bIsWallRunAvaible)
	{
		return;
	}

	if (!GetCharacterMovement()->IsFalling())
	{
		return;
	}

	FVector HitNormal = Hit.ImpactNormal;
	WallRunSide runSide = WallRunSide::NONE;
	FVector Direction = FVector::ZeroVector;

	if (!IsSurfaceWallRunable(HitNormal))
		return;

	GetWallRunSideAndDirection(HitNormal, runSide, Direction);

	if (!AreRequaredKeysDown(runSide))
	{
		return;
	}

	StartWallRun(runSide, Direction);
}

void AWallRunCharacter::GetWallRunSideAndDirection(const FVector& HitNormal, WallRunSide& runSide, FVector& Direction) const
{
	if (FVector::DotProduct(HitNormal, GetActorRightVector()) > 0.0f)
	{
		runSide = WallRunSide::LEFT;
		Direction = FVector::CrossProduct(HitNormal, FVector::UpVector).GetSafeNormal();
	}
	else
	{
		runSide = WallRunSide::RIGHT;
		Direction = FVector::CrossProduct(FVector::UpVector, HitNormal).GetSafeNormal();
	}
}

bool AWallRunCharacter::IsSurfaceWallRunable(const FVector& surfaceNormal) const
{
	if (surfaceNormal.Z > GetCharacterMovement()->GetWalkableFloorZ() || surfaceNormal.Z < -0.005f)
		return false;

	return true;
}

bool AWallRunCharacter::AreRequaredKeysDown(WallRunSide side) const
{
	if (forwardAxis < 0.1f)
	{
		return false;
	}

	if (side == WallRunSide::LEFT && rightAxis > 0.1f)
	{
		return false;
	}

	if (side == WallRunSide::RIGHT && rightAxis < -0.1f)
	{
		return false;
	}

	return true;
}

void AWallRunCharacter::StartWallRun(WallRunSide side, const FVector& direction)
{
	BeginCameraTilt();

	bIsWallRunning = true;
	CurrentWallRunDirection = direction;
	CurrentWallRunSide = side;

	GetCharacterMovement()->SetPlaneConstraintNormal(FVector::UpVector);

	GetWorld()->GetTimerManager().SetTimer(WallRunTimer, this, &AWallRunCharacter::StopWallRun, MaxWallRunTime, false);
}

void AWallRunCharacter::StopWallRun()
{
	EndCameraTilt();

	bIsWallRunning = false;

	GetCharacterMovement()->SetPlaneConstraintNormal(FVector::ZeroVector);

	StartReloadingWallRun();
}

void AWallRunCharacter::UpdateWallRun()
{
	if (!AreRequaredKeysDown(CurrentWallRunSide))
	{
		StopWallRun();
		return;
	}

	FHitResult lineTraceResult;
	
	FVector lineTraceDirection = CurrentWallRunSide == WallRunSide::RIGHT ? GetActorRightVector() : -GetActorRightVector();
	float lineTraceDistance = 200.0f;

	FVector startTrace = GetActorLocation();
	FVector endTrace = startTrace + lineTraceDirection * lineTraceDistance;

	FCollisionQueryParams traceParams;
	traceParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(lineTraceResult, startTrace, endTrace, ECC_Visibility, traceParams))
	{
		WallRunSide newRunSide = WallRunSide::NONE;
		FVector newDirection = FVector::ZeroVector;

		GetWallRunSideAndDirection(lineTraceResult.Normal, newRunSide, newDirection);

		if (newRunSide != CurrentWallRunSide)
		{
			StopWallRun();
			GetWorld()->GetTimerManager().ClearTimer(WallRunTimer);
		}
		else
		{
			CurrentWallRunDirection = newDirection;
			GetCharacterMovement()->Velocity = GetCharacterMovement()->GetMaxSpeed() * CurrentWallRunDirection;
		}

	}
	else
	{
		StopWallRun();
	}
}

void AWallRunCharacter::StartReloadingWallRun()
{
	bIsWallRunAvaible = false;
	GetWorld()->GetTimerManager().ClearTimer(WallRunTimer);
	GetWorld()->GetTimerManager().SetTimer(WallRunReloadTimer, this, &AWallRunCharacter::EndReloadingWallRun, ReloadingWallRunTime, false);
}

void AWallRunCharacter::EndReloadingWallRun()
{
	bIsWallRunAvaible = true;
}

void AWallRunCharacter::UpdateCameraTilt(float value)
{
	FRotator CurrentControlRotation = GetControlRotation();
	CurrentControlRotation.Roll = CurrentWallRunSide == WallRunSide::LEFT ? value : -value;
	GetController()->SetControlRotation(CurrentControlRotation);
}

void AWallRunCharacter::SaveCheckpoint(const FVector& position, const FRotator& newRotation, float newDeadlyHeight)
{
	checpoint = position; 
	startRatate = newRotation;
	DeadlyHeight = newDeadlyHeight;
}

void AWallRunCharacter::BoostActivate()
{
	bIsBoost = true; 
	GetCharacterMovement()->MaxWalkSpeed = boostSpeed;
}

void AWallRunCharacter::BoostEnd()
{
	bIsBoost = false;  
	GetCharacterMovement()->MaxWalkSpeed = standartSpeed;
}

bool AWallRunCharacter::IsMustDie()
{
	return GetActorLocation().Z <= DeadlyHeight;
}
