﻿// Copyright (c) Yevhenii Selivanov

#include "Components/MyCameraComponent.h"
//---
#include "Controllers/MyPlayerController.h"
#include "Engine/MyGameViewportClient.h"
#include "GameFramework/MyGameStateBase.h"
#include "Globals/GameStateDataAsset.h"
#include "MyUtilsLibraries/UtilsLibrary.h"
#include "UtilityLibraries/CellsUtilsLibrary.h"
#include "UtilityLibraries/SingletonLibrary.h"
//---
#if WITH_EDITOR
#include "EditorUtilsLibrary.h"
#endif

// If set, returns additional FOV modifier scaled by level size and current screen aspect ratio
void FCameraDistanceParams::CalculateFitViewAdditiveAngle(float& InOutFOV) const
{
	if (!FitViewAdditiveAngle)
	{
		return;
	}

	// Calculate multiplier to fit with aspect ratios of any screen
	constexpr float VerticalModifier = -1.f;
	constexpr float HorizontalModifier = 1.f;
	const bool bIsVerticalScreen = UUtilsLibrary::GetViewportAspectRatioAxisConstraint() == AspectRatio_MaintainXFOV;
	const float AspectRatioMultiplier = bIsVerticalScreen ? VerticalModifier : HorizontalModifier;

	InOutFOV = InOutFOV - FitViewAdditiveAngle * AspectRatioMultiplier;
}

// If set, truncates specified distance to allowed minimal one
void FCameraDistanceParams::LimitToMinDistance(float& InOutCameraDistance) const
{
	if (MinDistance)
	{
		InOutCameraDistance = FMath::Max(InOutCameraDistance, MinDistance);
	}
}

// Calculates the distance how far away the camera should be placed to fit the given view for specified FOV
float FCameraDistanceParams::CalculateDistanceToFitViewToFOV(const FVector2D& ViewSizeUU, float CameraFOV)
{
	const float NewFOV = FMath::Tan(FMath::DegreesToRadians(CameraFOV / 2.f));

	// Find horizontal and vertical distance for levels and screens with any aspect ratios
	// So the camera will be able to align vertical grid to the wide screen as well as horizontal grid to the vertical screen
	const bool bIsWideScreen = UUtilsLibrary::GetViewportAspectRatioAxisConstraint() == AspectRatio_MaintainYFOV;
	const float HorizontalDistance = bIsWideScreen ? ViewSizeUU.X / NewFOV : (ViewSizeUU.Y / 2.f) * NewFOV; // view is wider than higher on wide or vertical screen
	const float VerticalDistance = bIsWideScreen ? ViewSizeUU.X / (2.f * NewFOV) : ViewSizeUU.Y / NewFOV;   // view is longer than wider on wide or vertical screen

	return FMath::Max(HorizontalDistance, VerticalDistance);
}

// Sets default values
UMyCameraComponent::UMyCameraComponent()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// Set transform defaults
	static const FVector DefaultRelativeLocation(0.F, 0.F, 1000.F);
	SetRelativeLocation_Direct(DefaultRelativeLocation);
	static const FRotator DefaultRelativeRotation(-90.0F, 0.0F, -90.0F);
	SetRelativeRotation_Direct(DefaultRelativeRotation);
	SetUsingAbsoluteScale(true);

	// Camera defaults
	SetConstraintAspectRatio(false); // viewport without black borders
#if WITH_EDITOR // [Editor]
	bCameraMeshHiddenInGame = !UEditorUtilsLibrary::IsEditor();
#endif

	// Disable Eye Adaptation
	PostProcessSettings.bOverride_AutoExposureMinBrightness = true;
	PostProcessSettings.AutoExposureMinBrightness = 1;
	PostProcessSettings.bOverride_AutoExposureMaxBrightness = true;
	PostProcessSettings.AutoExposureMaxBrightness = 1;
}

/** Returns current FOV of camera manager that is more reliable than own FOV. */
float UMyCameraComponent::GetCameraManagerFOV() const
{
	const AMyPlayerController* PC = USingletonLibrary::GetLocalPlayerController();
	const APlayerCameraManager* PlayerCameraManager = PC ? PC->PlayerCameraManager : nullptr;
	return PlayerCameraManager ? PlayerCameraManager->GetFOVAngle() : FieldOfView;
}

// Set the location between players
bool UMyCameraComponent::UpdateLocation(float DeltaTime/* = 0.f*/)
{
	auto MoveCamera = [this, DeltaTime](FVector NewLocation)
	{
		if (DeltaTime)
		{
			NewLocation = FMath::Lerp(GetComponentLocation(), NewLocation, DeltaTime);
		}
		SetWorldLocation(NewLocation);
	};

	// If true, the camera will be forced moving to the start position
	if (bIsCameraLockedOnCenterInternal
	    || !USingletonLibrary::GetAlivePlayersNum()
	    || bForceStartInternal)
	{
		static constexpr float Tolerance = 10.f;
		const FVector CameraWorldLocation = GetComponentLocation();
		const FVector CameraLockedLocation = GetCameraLockedLocation();
		const bool bShouldLerp = !CameraWorldLocation.Equals(CameraLockedLocation, Tolerance);
		if (bShouldLerp)
		{
			MoveCamera(CameraLockedLocation);
		}

		// return false to disable tick on finishing
		return bShouldLerp;
	}

	// Distance finding between players
	MoveCamera(GetCameraLocationBetweenPlayers());

	return true;
}

// Calls to set following camera by player locations
void UMyCameraComponent::SetCameraLockedOnCenter(bool bInCameraLockedOnCenter)
{
	bIsCameraLockedOnCenterInternal = bInCameraLockedOnCenter;

	// Enable camera if should be unlocked
	if (!bInCameraLockedOnCenter
	    && !IsComponentTickEnabled()
	    && AMyGameStateBase::GetCurrentGameState() == ECurrentGameState::InGame)
	{
		SetComponentTickEnabled(true);
	}
}

// Allows to tweak distance calculation from camera to the level during the game
void UMyCameraComponent::SetCameraDistanceParams(const FCameraDistanceParams& InCameraDistanceParams)
{
	DistanceParamsInternal = InCameraDistanceParams;

	// Update camera location to apply new distance params
	UpdateLocation();
}

// Calculates how faw away the camera should be placed from specified cells
float UMyCameraComponent::GetCameraDistanceToCells(const FCells& Cells) const
{
	float CurrentFOV = GetCameraManagerFOV();

	// If is set in params, additional FOV modifier will be applied
	DistanceParamsInternal.CalculateFitViewAdditiveAngle(/*InOut*/CurrentFOV);

	// Instead of changing real FOV, we can just change the distance to the camera to avoid the fisheye effect.
	// Calculate how far away the camera should be placed to fit the given view by specified FOV
	const FVector2D ViewSizeUU = FCell::GetCellArraySize(Cells) * FCell::CellSize;
	float CameraDistance = FCameraDistanceParams::CalculateDistanceToFitViewToFOV(ViewSizeUU, CurrentFOV);

	// If is set in params, cut camera distance by min value
	DistanceParamsInternal.LimitToMinDistance(/*InOut*/CameraDistance);

	return CameraDistance;
}

// Returns the center camera location between all specified cells
FVector UMyCameraComponent::GetCameraLocationBetweenCells(const FCells& Cells) const
{
	FVector NewLocation = FCell::GetCellArrayCenter(Cells).Location;
	NewLocation.Z += GetCameraDistanceToCells(Cells);
	return NewLocation;
}

// Returns the center location between all players and bots
FVector UMyCameraComponent::GetCameraLocationBetweenPlayers() const
{
	const FCells PlayersCells = UCellsUtilsLibrary::GetAllCellsWithActors(TO_FLAG(EAT::Player));
	return GetCameraLocationBetweenCells(PlayersCells);
}

// Returns the default location between all players and bots
FVector UMyCameraComponent::GetCameraLockedLocation() const
{
	const FCells CornerCells = UCellsUtilsLibrary::GetCornerCellsOnLevel();
	return GetCameraLocationBetweenCells(CornerCells);
}

// Called every frame
void UMyCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!UpdateLocation(DeltaTime))
	{
		SetComponentTickEnabled(false);
	}
}

// Called when the game starts or when spawned
void UMyCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen states to manage the tick
	if (AMyGameStateBase* MyGameState = USingletonLibrary::GetMyGameState())
	{
		MyGameState->OnGameStateChanged.AddDynamic(this, &ThisClass::OnGameStateChanged);
	}

	// Listen to recalculate camera location
	if (UMyGameViewportClient* GameViewportClient = USingletonLibrary::GetGameViewportClient())
	{
		GameViewportClient->OnAspectRatioChanged.AddUniqueDynamic(this, &ThisClass::OnAspectRatioChanged);
	}
}

// Listen game states to manage the tick
void UMyCameraComponent::OnGameStateChanged(ECurrentGameState CurrentGameState)
{
	bool bShouldTick = false;

	switch (CurrentGameState)
	{
		case ECurrentGameState::GameStarting:
		{
			PossessCamera();
			bShouldTick = true;
			break;
		}
		case ECurrentGameState::EndGame:
		{
			bForceStartInternal = true;
			bShouldTick = true;
			break;
		}
		case ECurrentGameState::InGame:
		{
			bForceStartInternal = false;
			bShouldTick = true;
			break;
		}
		default:
			break;
	}

	SetComponentTickEnabled(bShouldTick);
}

// Listen to recalculate camera location when screen aspect ratio was changed
void UMyCameraComponent::OnAspectRatioChanged(float NewAspectRatio)
{
	UpdateLocation();
}

// Starts viewing through this camera
void UMyCameraComponent::PossessCamera()
{
	AActor* Owner = GetOwner();
	AMyPlayerController* MyPC = USingletonLibrary::GetLocalPlayerController();
	if (!ensureMsgf(Owner, TEXT("ASSERT: 'Owner' is not valid"))
	    || !ensureMsgf(MyPC, TEXT("ASSERT: 'MyPC' is not valid")))
	{
		return;
	}

	const float BlendTime = UGameStateDataAsset::Get().GetStartingCountdown();
	MyPC->SetViewTargetWithBlend(Owner, BlendTime);
}
