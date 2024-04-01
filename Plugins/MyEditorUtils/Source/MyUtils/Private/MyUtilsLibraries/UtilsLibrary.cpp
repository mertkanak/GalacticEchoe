﻿// Copyright (c) Yevhenii Selivanov

#include "MyUtilsLibraries/UtilsLibrary.h"
//---
#include "UnrealClient.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
//---
#if WITH_EDITOR
#include "MyEditorUtilsLibraries/EditorUtilsLibrary.h"
#endif // WITH_EDITOR
//---
#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilsLibrary)

// Returns the current play world
UWorld* UUtilsLibrary::GetPlayWorld(const UObject* OptionalWorldContext)
{
	UWorld* FoundWorld = nullptr;
	if (GEngine)
	{
		FoundWorld = GEngine->GetWorldFromContextObject(OptionalWorldContext, EGetWorldErrorMode::ReturnNull);
		if (!FoundWorld)
		{
			FoundWorld = GEngine->GetCurrentPlayWorld();
		}
	}

#if WITH_EDITOR
	if (!FoundWorld)
	{
		FoundWorld = FEditorUtilsLibrary::GetEditorWorld();
	}
#endif

	if (!ensureMsgf(FoundWorld, TEXT("%s: Can not obtain current world"), *FString(__FUNCTION__)))
	{
		return nullptr;
	}

	return FoundWorld;
}

// Checks, is the current world placed in the editor
bool UUtilsLibrary::IsEditor()
{
#if WITH_EDITOR
	return FEditorUtilsLibrary::IsEditor();
#else
	return false;
#endif
}

// Checks is the current world placed in the editor and the game not started yet
bool UUtilsLibrary::IsEditorNotPieWorld()
{
#if WITH_EDITOR
	return FEditorUtilsLibrary::IsEditorNotPieWorld();
#else
	return false;
#endif
}

// Returns true if game is started in the Editor
bool UUtilsLibrary::IsPIE()
{
#if WITH_EDITOR
	return FEditorUtilsLibrary::IsPIE();
#else
	return false;
#endif
}

// Returns true if is started multiplayer game (server + client(s)) right in the Editor
bool UUtilsLibrary::IsEditorMultiplayer()
{
#if WITH_EDITOR
	return FEditorUtilsLibrary::IsEditorMultiplayer();
#else
	return false;
#endif
}

// Returns the index of current player during editor multiplayer
int32 UUtilsLibrary::GetEditorPlayerIndex()
{
#if WITH_EDITOR
	return FEditorUtilsLibrary::IsEditorMultiplayer();
#else
	return INDEX_NONE;
#endif
}

// Returns true if game was started
bool UUtilsLibrary::HasWorldBegunPlay()
{
	if (IsPIE())
	{
		return true;
	}

	const UWorld* World = GetPlayWorld();
	return World && World->HasBegunPlay();
}

// Returns true if this instance is server
bool UUtilsLibrary::IsServer()
{
	const UWorld* World = GetPlayWorld();
	return World && !World->IsNetMode(NM_Client);
}

// Returns true if viewport is initialized, is always true in PIE, but takes a while in builds
bool UUtilsLibrary::IsViewportInitialized()
{
	UGameViewportClient* GameViewport = GEngine ? GEngine->GameViewport : nullptr;
	FViewport* Viewport = GameViewport ? GameViewport->Viewport : nullptr;
	if (!Viewport)
	{
		return false;
	}

	auto IsZeroViewportSize = [Viewport] { return Viewport->GetSizeXY() == FIntPoint::ZeroValue; };

	if (IsZeroViewportSize())
	{
		// Try update its value by mouse enter event
		GameViewport->MouseEnter(Viewport, FIntPoint::ZeroValue.X, FIntPoint::ZeroValue.Y);
		return !IsZeroViewportSize();
	}

	return true;
}

// Returns the actual screen resolution
FIntPoint UUtilsLibrary::GetViewportResolution()
{
	const FViewport* Viewport = IsViewportInitialized() ? GEngine->GameViewport->Viewport : nullptr;

#if WITH_EDITOR
	if (FEditorUtilsLibrary::IsEditor()
		&& !Viewport)
	{
		Viewport = FEditorUtilsLibrary::GetEditorViewport();
	}
#endif

	return Viewport ? Viewport->GetSizeXY() : FIntPoint::ZeroValue;
}

// Returns 'MaintainYFOV' if Horizontal FOV is currently used while 'MaintainXFOV' for the Vertical one
TEnumAsByte<EAspectRatioAxisConstraint> UUtilsLibrary::GetViewportAspectRatioAxisConstraint()
{
	const APlayerController* PlayerController = GEngine ? GEngine->GetFirstLocalPlayerController(GWorld) : nullptr;
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	return LocalPlayer ? LocalPlayer->AspectRatioAxisConstraint : TEnumAsByte(AspectRatio_MAX);
}

// Abstract method that allows set both static and skeletal meshes to the specified mesh component
void UUtilsLibrary::SetMesh(UMeshComponent* MeshComponent, UStreamableRenderAsset* MeshAsset)
{
	if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
	{
		SkeletalMeshComponent->SetSkeletalMesh(Cast<USkeletalMesh>(MeshAsset));
	}
	else if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent))
	{
		StaticMeshComponent->SetStaticMesh(Cast<UStaticMesh>(MeshAsset));
	}
}

// Returns the first child actor of the specified class
AActor* UUtilsLibrary::GetAttachedActorByClass(const AActor* ParentActor, TSubclassOf<AActor> ChildActorClass, bool bIncludeDescendants/* = false*/)
{
	if (!ensureMsgf(ParentActor, TEXT("ASSERT: [%i] %s:\n'!ParentActor' is not valid!"), __LINE__, *FString(__FUNCTION__)))
	{
		return nullptr;
	}

	TArray<AActor*> AttachedActors;
	ParentActor->GetAttachedActors(AttachedActors);
	if (AttachedActors.IsEmpty())
	{
		return nullptr;
	}

	for (AActor* It : AttachedActors)
	{
		if (It && It->IsA(ChildActorClass))
		{
			return It;
		}

		if (bIncludeDescendants)
		{
			if (AActor* FoundActor = GetAttachedActorByClass(It, ChildActorClass, bIncludeDescendants))
			{
				return FoundActor;
			}
		}
	}

	return nullptr;
}
