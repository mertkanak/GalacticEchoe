// Fill out your copyright notice in the Description page of Project Settings.

#include "WallActor.h"

#include "Bomber.h"
#include "Components/StaticMeshComponent.h"
#include "MapComponent.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
AWallActor::AWallActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Initialize Root Component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));

	// Initialize MapComponent
	MapComponent = CreateDefaultSubobject<UMapComponent>(TEXT("MapComponent"));

	// Initialize wall mesh
	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
	WallMesh->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> WallMeshFinder(TEXT("/Game/Bomber/Assets/Meshes/WallMesh"));
	if (WallMeshFinder.Succeeded())
	{
		WallMesh->SetStaticMesh(WallMeshFinder.Object);
	}
}

void AWallActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (IS_VALID(MapComponent) == false)  // this component is not valid for owner construction
	{
		return;
	}

	// Update this actor on the Level Map
	MapComponent->OnMapComponentConstruction();
}

#if WITH_EDITOR
void AWallActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished == false					 // Not yet finished
		|| IS_VALID(MapComponent) == false)  // is not valid for updates on the map
	{
		return;
	}
	UE_LOG_STR(this, "[Editor]PostEditMove", "-> \t UpdateSelfOnMap");

	// Update this actor on the Level Map
	MapComponent->UpdateSelfOnMap();
}
#endif  //WITH_EDITOR [Editor]

// Called when the game starts or when spawned
void AWallActor::BeginPlay()
{
	Super::BeginPlay();

	// Binding to the event, that triggered when the actor has been explicitly destroyed
	OnDestroyed.AddDynamic(this, &AWallActor::OnBoxDestroyed);
}

void AWallActor::OnBoxDestroyed(AActor* DestroyedActor)
{
	// Should not be destroyed in game
	checkNoEntry();
}
