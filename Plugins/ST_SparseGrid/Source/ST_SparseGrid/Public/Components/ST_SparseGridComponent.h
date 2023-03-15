// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "ST_SparseGridTypes.h"
#include "ST_SparseGridComponent.generated.h"

/*
* Sparse Grid Object Component
*
* Add this component to any actor, and it will automatically register with the Basic Sparse Grid
* Inherit this component to add your own functionality directly, if required.
*/
UCLASS(BlueprintType, Category = "Sparse Grid", meta = (DisplayName = "Sparse Grid Component", BlueprintSpawnableComponent), HideCategories = (Activation, Sockets, Collision, ComponentReplication, Tags, Variable, Cooking))
class ST_SPARSEGRID_API UST_SparseGridComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	// Constructor
	UST_SparseGridComponent(const FObjectInitializer& OI);

	// Sparse Grid Interface
	FVector GetSparseGridLocation() const;
	const FST_SparseGridData& GetSparseGridData() const { return SparseGridData; }
	FST_SparseGridData& AccessSparseGridData() { return SparseGridData; }

	/*
	* Converts an array of grid components out to an array of their owning actors
	* Returns the total number of elements
	*/
	UFUNCTION(BlueprintCallable, Category = "Sparse Grid|Conversions", meta = (DisplayName = "Convert To Actors"))
	static void GetComponentsOwners(const TArray<UST_SparseGridComponent*>& GridComponents, TArray<AActor*>& Actors);

	/*
	* Converts an array of grid components out to an array of their owning actors, but only if the owning actor is of the given class
	* This is useful to prevent the user having to perform casts in Blueprint
	* Returns the total number of elements
	*/
	UFUNCTION(BlueprintCallable, Category = "Sparse Grid|Conversions", meta = (DisplayName = "Convert To Actors of Class", DeterminesOutputType = "ActorClass", DynamicOutputParam = "Actors"))
	static void GetComponentsOwners_Typed(const TArray<UST_SparseGridComponent*>& GridComponents, TArray<AActor*>& Actors, const TSubclassOf<AActor> ActorClass);

protected:
	// UActorComponent Interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	// Registration
	virtual void RegisterWithSparseGrid();
	virtual void UnRegisterWithSparseGrid();

private:
	// The actual grid reference data
	FST_SparseGridData SparseGridData;
};