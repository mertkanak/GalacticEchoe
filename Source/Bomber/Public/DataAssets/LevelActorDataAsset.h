﻿// Copyright (c) Yevhenii Selivanov.

#pragma once

#include "Engine/DataAsset.h"
//---
#include "Bomber.h"
#include "Engine/EngineTypes.h" // ECollisionResponse
//---
#include "LevelActorDataAsset.generated.h"

/**
 * The base archetype of level actor rows. Is implemented in player, item rows, etc.
 */
UCLASS(Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew, Const, AutoExpandCategories=("C++"))
class BOMBER_API ULevelActorRow : public UObject
{
	GENERATED_BODY()

public:
	/** The level where should be used a mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Row", meta = (ShowOnlyInnerProperties))
	ELevelType LevelType = ELT::None;

	/** The static mesh, skeletal mesh or texture */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Row", meta = (ShowOnlyInnerProperties, ExposeOnSpawn = "true"))
	TObjectPtr<class UStreamableRenderAsset> Mesh = nullptr;

	/** Returns true if this row contains valid data. */
	UFUNCTION(BlueprintPure, Category = "C++")
	virtual bool IsValid() const { return Mesh != nullptr; }

protected:
#if WITH_EDITOR
	/** Called to handle row changes. */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITOR
};

/**
 * The base data asset for the Bomber's data.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, Const, AutoExpandCategories=("C++"))
class BOMBER_API UBomberDataAsset : public UDataAsset
{
	GENERATED_BODY()

protected:
#if WITH_EDITOR
	/** Called to notify on any data asset changes. */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITOR
};

/**
 * The base data asset for any level actor that contains the main data about them.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class BOMBER_API ULevelActorDataAsset : public UBomberDataAsset
{
	GENERATED_BODY()

public:
	/** Return rows by specified level types in the bitmask. */
	UFUNCTION(BlueprintPure, Category = "C++")
	void GetRowsByLevelType(
		TArray<ULevelActorRow*>& OutRows,
		UPARAM(meta = (Bitmask, BitmaskEnum = "/Script/Bomber.ELevelType")) int32 LevelsTypesBitmask) const;

	/** Returns first found row by given predicate function. */
	const ULevelActorRow* GetRowByPredicate(const TFunctionRef<bool(const ULevelActorRow&)>& Predicate) const;

	template <typename T>
	const FORCEINLINE T* GetRowByPredicate(const TFunctionRef<bool(const ULevelActorRow&)>& Predicate) const { return Cast<T>(GetRowByPredicate(Predicate)); }

	/** Return first found row by specified level types. */
	UFUNCTION(BlueprintPure, Category = "C++")
	const FORCEINLINE ULevelActorRow* GetRowByLevelType(ELevelType LevelType) const { return GetRowByPredicate([LevelType](const ULevelActorRow& RowIt) { return RowIt.LevelType == LevelType || RowIt.LevelType == ELT::Max; }); }

	template <typename T>
	const FORCEINLINE T* GetRowByLevelType(ELevelType LevelType) const { return Cast<T>(GetRowByLevelType(LevelType)); }

	/** Return first found row by specified mesh. */
	UFUNCTION(BlueprintPure, Category = "C++")
	const FORCEINLINE ULevelActorRow* GetRowByMesh(const class UStreamableRenderAsset* Mesh) const { return GetRowByPredicate([Mesh](const ULevelActorRow& RowIt) { return RowIt.Mesh == Mesh; }); }

	/** Returns overall number of contained rows. */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE int32 GetRowsNum() const { return RowsInternal.Num(); }

	/** Returns the class of an actor, whose data is described by this data asset. */
	UFUNCTION(BlueprintPure, Category = "C++")
	UClass* GetActorClass() const;

	/** Returns the actor type of an actor, whose data is described by this data asset. */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE EActorType GetActorType() const { return ActorTypeInternal; }

	/** Returns a extent size of the collision box of an actor, whose data is described by this data asset. */
	UFUNCTION(BlueprintPure, Category = "C++")
	const FORCEINLINE FVector& GetCollisionExtent() const { return CollisionExtentInternal; }

	/** Returns a response type of the collision box of an actor, whose data is described by this data asset. */
	UFUNCTION(BlueprintPure, Category = "C++")
	FORCEINLINE ECollisionResponse GetCollisionResponse() const { return CollisionResponseInternal; }

protected:
	/** DevelopmentOnly: internal class of rows, is overriden by child data assets, used on adding new row. */
	UPROPERTY(BlueprintReadOnly, Category = "C++", meta = (BlueprintProtected, DisplayName = "Row Class", DevelopmentOnly))
	TObjectPtr<UClass> RowClassInternal = ULevelActorRow::StaticClass();

	/** All rows contained by this data asset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, meta = (BlueprintProtected, DisplayName = "Rows", ShowOnlyInnerProperties))
	TArray<TObjectPtr<class ULevelActorRow>> RowsInternal;

	/** Class of an actor, whose data is described by this data asset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Actor Class", ShowOnlyInnerProperties))
	TSoftClassPtr<class AActor> ActorClassInternal = nullptr;

	/** Actor type of an actor, whose data is described by this data asset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Actor Type", ShowOnlyInnerProperties))
	EActorType ActorTypeInternal = EAT::None;

	/** Extent size of the collision box of an actor, whose data is described by this data asset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Collision Extent", ShowOnlyInnerProperties))
	FVector CollisionExtentInternal = FVector(100.f);

	/** Response type of the collision box of an actor, whose data is described by this data asset. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (BlueprintProtected, DisplayName = "Collision Response", ShowOnlyInnerProperties))
	TEnumAsByte<ECollisionResponse> CollisionResponseInternal = ECR_Overlap;

#if WITH_EDITOR
	/** Handle adding new rows. */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif	//WITH_EDITOR
};
