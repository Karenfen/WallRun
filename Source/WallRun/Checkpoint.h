// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Checkpoint.generated.h"


class AWallRunCharacter;


UCLASS()
class WALLRUN_API ACheckpoint : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	class UStaticMeshComponent* TriggerMesh;

	// trigger for save
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	class USphereComponent* HitCollider;

	// for player start rotation 
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	class UArrowComponent* NewDirection;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Components")
	class UPointLightComponent * ActiveLight;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Effects")
	class UAudioComponent* AudioSaving;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|New deadly height")
	float NewDeadlyHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time to die after activate")
	float TimeToDie = 1.0f;
	
public:	
	// Sets default values for this actor's properties
	ACheckpoint();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	bool Seving(AWallRunCharacter* player);

	UFUNCTION()
		void OnTriggerOverlapBegin(UPrimitiveComponent* OverlappedComp,
			AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
			bool bFromSweep, const	FHitResult& SweepResult);

	void SaveComplete() { Destroy(); };

private:
	FVector NewStartPoint = FVector::ZeroVector;
	FTimerHandle DestroyTimer;
};
