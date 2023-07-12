// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/TimelineComponent.h"
#include "WallRunCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UMotionControllerComponent;
class UAnimMontage;
class USoundBase;

UENUM()
enum class WallRunSide : uint8
{
	NONE = 0,
	RIGHT,
	LEFT
};

UCLASS(config = Game)
class AWallRunCharacter : public ACharacter
{
	GENERATED_BODY()

		/** Pawn mesh: 1st person view (arms; seen only by self) */
		UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		USceneComponent* FP_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		UCameraComponent* FirstPersonCameraComponent;

public:
	AWallRunCharacter();
	virtual void Tick(float Deltatime) override;
	virtual void Jump() override;
	void Die();
	// set new chackpoint
	void SaveCheckpoint(const FVector& position, const FRotator& newRotation, float newDeadlyHeight);

protected:
	virtual void BeginPlay();

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		TSubclassOf<class AWallRunProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		UAnimMontage* FireAnimation;

protected:

	/** Fires a projectile. */
	void OnFire();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	// property for edit max wall run time from blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float MaxWallRunTime = 1.0f;

	// property for edit rest time after wall run
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Run", meta = (UIMin = 0.0f, ClampMin = 0.0f))
	float ReloadingWallRunTime = 1.0f;

	// property from tilt camera WallRun
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wall Run")
	UCurveFloat* CameraTiltCurv;

	// for speed boost
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float BoostScale = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float DeadlyHeight = 0.0f;

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

private:
	// character capsul hit handler
	UFUNCTION()
	void OnCharacterCapsulHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// get params wall run
	void GetWallRunSideAndDirection(const FVector& HitNormal, WallRunSide& runSide, FVector& Direction) const;

	// check available wallrun
	bool IsSurfaceWallRunable(const FVector& surfaceNormal) const;
	// check buttons pressed for wall run
	bool AreRequaredKeysDown(WallRunSide side) const;

	// wall run metods
	void StartWallRun(WallRunSide side, const FVector& direction);
	void StopWallRun();
	void UpdateWallRun();
	void StartReloadingWallRun();
	void EndReloadingWallRun();

	// camera tilt metods
	FORCEINLINE void BeginCameraTilt() { CameraTiltTimeline.Play(); }
	UFUNCTION()
	void UpdateCameraTilt(float value);
	FORCEINLINE void EndCameraTilt() { CameraTiltTimeline.Reverse(); }

	// for boost running and jump while wallRun
	void BoostActivate();
	void BoostEnd();

	// checking die
	bool IsMustDie();

	// moving axises value from check wall run
	float forwardAxis = 0.0f;
	float rightAxis = 0.0f;

	// wallRun paremeters
	bool bIsWallRunning = false;
	bool bIsWallRunAvaible = true;
	WallRunSide CurrentWallRunSide = WallRunSide::NONE;
	FVector CurrentWallRunDirection = FVector::ZeroVector;

	// boost status
	bool bIsBoost = false;
	float boostSpeed = 0.0f;
	float standartSpeed = 0.0f;

	// checkpoint
	FVector checpoint = FVector::ZeroVector;
	FRotator startRatate = FRotator::ZeroRotator;
	
	// wallrun timer
	FTimerHandle WallRunTimer;
	// wallrun timer for some rest
	FTimerHandle WallRunReloadTimer;
	// camera tilt timeline
	FTimeline CameraTiltTimeline;
};