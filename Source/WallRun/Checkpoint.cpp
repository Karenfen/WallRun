// Fill out your copyright notice in the Description page of Project Settings.


#include "Checkpoint.h"
#include "WallRunCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "TimerManager.h"
#include "Components/ArrowComponent.h"



// Sets default values
ACheckpoint::ACheckpoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// init components

	TriggerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Trigger visualiser"));
	RootComponent = TriggerMesh;

	NewDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Direction for player"));
	NewDirection->SetupAttachment(TriggerMesh);

	HitCollider = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger collision"));
	HitCollider->SetupAttachment(TriggerMesh);
	HitCollider->OnComponentBeginOverlap.AddDynamic(this, &ACheckpoint::OnTriggerOverlapBegin);

	ActiveLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	ActiveLight->SetupAttachment(TriggerMesh);

	AudioSaving = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio effect"));
	AudioSaving->SetupAttachment(TriggerMesh);
	AudioSaving->SetAutoActivate(false);
}

// Called when the game starts or when spawned
void ACheckpoint::BeginPlay()
{
	Super::BeginPlay();

	NewStartPoint = GetActorLocation();
}

bool ACheckpoint::Seving(AWallRunCharacter* player)
{
	if (IsValid(player))
	{
		NewStartPoint.Z = player->GetActorLocation().Z;

		player->SaveCheckpoint(NewStartPoint, GetActorRotation(), NewDeadlyHeight);

		return true;
	}
	
	return false;
}

void ACheckpoint::OnTriggerOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const	FHitResult& SweepResult)
{
	AWallRunCharacter* Player = Cast<AWallRunCharacter>(OtherActor);
	if (Player)
	{
		if (Seving(Player))
		{
			if (AudioSaving)
			{
				AudioSaving->Play();
			}
			TriggerMesh->SetHiddenInGame(true);
			HitCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ActiveLight->SetHiddenInGame(true);
			GetWorld()->GetTimerManager().SetTimer(DestroyTimer, this, &ACheckpoint::SaveComplete, TimeToDie, false);
		}
	}
}