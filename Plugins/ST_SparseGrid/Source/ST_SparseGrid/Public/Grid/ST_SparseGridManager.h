// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "ST_SparseGridManager.generated.h"

// Declarations
class UST_SparseGridData;
class FST_SparseGridModule;

// Sparse-Grid Forward Declaration
// Prevents having to include ST_SparseGrid.h in child classes, but as a caveat some methods cannot be inlined
template<class T>
class TST_SparseGrid;

/*
* Sparse Grid Manager Component
* Contains Common functionality - inherit this class to create your own manager.
* Only one manager can exist at a time - but a manager can contain and update multiple grids.
*
* Component is created using AWorldSettings as an outer, as we have convenient object lifecycle management already.
* Static Accessor function for easy lookups in code and C++
*/
UCLASS(Abstract, Transient, BlueprintType, NotBlueprintable, HideCategories = (Activation, ComponentReplication, ComponentTick, Collision, Tags, Variable, Cooking, Events), meta = (DisplayName = "Sparse Grid Manager"))
class ST_SPARSEGRID_API UST_SparseGridManager : public UActorComponent
{
	GENERATED_BODY()
public:
	// Constructor
	UST_SparseGridManager(const FObjectInitializer& OI);

	/*
	* Static Accessor.
	* Will require casting to the correct Manager Type
	*
	* Tries to get the Grid Instance from world settings.
	* Will return nullptr if the Instance does not exist.
	*/
	UFUNCTION(BlueprintPure, Category = "Sparse Grid", meta = (CompactNodeTitle = "Sparse Grid", DisplayName = "Sparse Grid", Keywords = "Sparse Grid", HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	static UST_SparseGridManager* Get(const UObject* WorldContextObject);

	/*
	* Static Accessor.
	*
	* Tries to get the Grid Instance from world settings.
	* Will assert if the manager doesn't exist.
	*/
	static UST_SparseGridManager* GetChecked(const UObject* WorldContextObject);

private:
	// Allow Module to call Create/Destroy Grid.
	friend FST_SparseGridModule;

	/*
	* Static Creation
	* Creates a Manager and registers it with a given world/level
	*/
	static void CreateGrid(UWorld* InWorld);

	/*
	* Static Destruction
	* Destroys a manager as a world is torn down
	*/
	static void DestroyGrid(UWorld* InWorld);

	/////////////////////
	///// Overrides /////
	/////////////////////
public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override final;
	virtual void SetComponentTickEnabled(bool bEnabled) override final;

protected:
	virtual void OnRegister() override final;
	virtual void OnUnregister() override final;
	virtual bool ShouldActivate() const override final;

#if WITH_EDITOR
	////////////////////////////////////
	///// Editor Debug Information /////
	////////////////////////////////////
public:
	virtual bool GetGridNames(TArray<FName>& OutGridNames) const { return false; }
	virtual bool GetGridPopulationData(const FName InGridName, TArray<uint32>& OutData) const { return false; }
	virtual bool GetGridMemoryInfo(const FName InGridName, int32& OutTotalObjects, uint64& OutRegisterAllocSize, uint64& OutRegisterUsedSize, uint64& OutCellAllocSize, uint64& OutCellUsedSize) const { return false; }
#endif

	////////////////
	///// Grid /////
	////////////////
public:
	/*
	* Purges all grid data and re-initializes the grids.
	* WARNING: This rebuilds all grids which may be costly! Only use if required!
	*/
	UFUNCTION(BlueprintCallable, Category = "Sparse Grid")
	void ForceRebuild()
	{
		UninitializeGrids();
		InitializeGrids();
	}

	/*
	* Whether we have created the sparse grids.
	*/
	UFUNCTION(BlueprintPure, Category = "Sparse Grid")
	FORCEINLINE bool AreGridsInitialized() const { return bGridsInitialized; }

	/*
	* Grid Data
	*/
	FORCEINLINE const UST_SparseGridData* GetGridConfig() const { return GridConfig.Get(); }

protected:
	virtual void CreateGrids() {}
	virtual void DestroyGrids() {}
	virtual void UpdateGrids() {}

private:
	void InitializeGrids();
	void UninitializeGrids();
	uint8 bGridsInitialized : 1;

	UPROPERTY(Transient)
	TWeakObjectPtr<const UST_SparseGridData> GridConfig;
};