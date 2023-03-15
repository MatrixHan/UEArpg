// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "ST_SparseGridTypes.h"
#include "ST_SparseGridManager.h"
#include "ST_SparseGridManager_Basic.generated.h"

// Declarations
class UST_SparseGridComponent;

/*
* Sparse Grid Manager Basic
* Sorts Components into a single sparse grid for fast lookups.
*
* A working example of the Sparse Grid System
* Projects will typically create something similar to this specific to the project, or they can use this implementation for basic behavior if they like.
*/
UCLASS(meta = (DisplayName = "Sparse Grid - Basic"))
class ST_SPARSEGRID_API UST_SparseGridManager_Basic : public UST_SparseGridManager
{
	GENERATED_BODY()
public:
	// Constructor
	UST_SparseGridManager_Basic(const FObjectInitializer& OI);

	/*
	* Static Accessor.
	*
	* Tries to get the Manager Instance as a Sparse Grid - Basic from world settings.
	* Will return nullptr if the Instance does not exist.
	*/
	UFUNCTION(BlueprintPure, Category = "Sparse Grid|Basic", meta = (CompactNodeTitle = "Sparse Grid - Basic", DisplayName = "Sparse Grid - Basic", Keywords = "Sparse Grid Basic", HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	static UST_SparseGridManager_Basic* K2_Get(const UObject* WorldContextObject) { return Cast<UST_SparseGridManager_Basic>(UST_SparseGridManager::Get(WorldContextObject)); }

	// UST_SparseGridManager Interface
	virtual void CreateGrids() override;
	virtual void DestroyGrids() override;
	virtual void UpdateGrids() override; 

#if WITH_EDITOR
	//////////////////
	///// Editor /////
	//////////////////
public:
	static const FName GRIDNAME_Basic;

	virtual bool GetGridNames(TArray<FName>& OutGridNames) const override;
	virtual bool GetGridPopulationData(const FName InGridName, TArray<uint32>& OutData) const override;
	virtual bool GetGridMemoryInfo(const FName InGridName, int32& OutTotalObjects, uint64& OutRegisterAllocSize, uint64& OutRegisterUsedSize, uint64& OutCellAllocSize, uint64& OutCellUsedSize) const override;
#endif

	/////////////////////
	///// Grid Data /////
	/////////////////////
public:
	/*
	* Grid Data Accessor (C++ Only)
	*/
	TSharedRef<TST_SparseGrid<UST_SparseGridComponent>> GetSparseGrid_Basic() const { return SparseGridData_Basic.ToSharedRef(); }
	
	/*
	* Gets all objects currently registered in the grid
	* Array cannot be modified
	*/
	UFUNCTION(BlueprintPure, Category = "Sparse Grid|Basic", meta = (DisplayName = "Get All Sparse Grid Components"))
	const TArray<UST_SparseGridComponent*>& GetGridComponents() const;

	/*
	* Get World-Space max position of the grid.
	* Blueprint Use Only.
	*/
	UFUNCTION(BlueprintPure, Category = "Sparse Grid|Basic", meta = (DisplayName = "Get Grid Max"))
	FST_GridRef2D K2_GetGridMax() const;

	/*
	* Convert a world-space XY position to a grid cell index.
	* Blueprint Use Only.
	*/
	UFUNCTION(BlueprintPure, Category = "Sparse Grid|Basic", meta = (DisplayName = "World To Cell"))
	int32 K2_WorldToCell(FVector2D InWorldXY) const;

	/*
	* Gets a search query tile
	*/
	UFUNCTION(BlueprintPure, Category = "Sparse Grid|Basic", meta = (DisplayName = "Get Search Query Cell Tile"))
	FST_SparseGridCellTile GetSearchTile(const FVector2D& WorldSearchOrigin, const FVector2D& WorldSearchExtents) const;

	/*
	* Get Components in a given cell.
	* Blueprint use only.
	*/
	UFUNCTION(BlueprintPure, Category = "Sparse Grid|Basic", meta = (DisplayName = "Get Cell Components"))
	void K2_GetCellComponents(const int32 CellID, UPARAM(DisplayName = "Grid Components")TArray<UST_SparseGridComponent*>& Components) const;

	/*
	* Get Components in a given tile.
	* Blueprint use only.
	*/
	UFUNCTION(BlueprintCallable, Category = "Sparse Grid|Basic", meta = (DisplayName = "Get Tile Components"))
	void K2_GetTileComponents(const FST_SparseGridCellTile& Tile, UPARAM(DisplayName = "Grid Components")TArray<UST_SparseGridComponent*>& Components) const;

	/*
	* Gets the cached cell bounding radius
	* Blueprint use only.
	*/
	UFUNCTION(BlueprintCallable, Category = "Sparse Grid|Basic", meta = (DisplayName = "Get Cell Bounds Radius"))
	float K2_GetCellBoundsRadius() const;

	/*
	* Convert a Cell ID to X and Y ID
	* Blueprint use only.
	*/
	UFUNCTION(BlueprintPure, Category = "Sparse Grid|Basic|Conversions", meta = (CompactNodeTitle = "->", DisplayName = "To Grid Ref 2D", BlueprintAutocast))
	FST_GridRef2D K2_ConvertToGridRef2D(const int32 CellID) const;

	/*
	* Convert X and Y ID to Cell ID
	* Blueprint use only.
	*/
	UFUNCTION(BlueprintPure, Category = "Sparse Grid|Basic|Conversions", meta = (CompactNodeTitle = "->", DisplayName = "To Cell ID", BlueprintAutocast))
	int32 K2_ConvertToCellID(const FST_GridRef2D& XY) const;

private:
	/*
	* Pointer to Grid Data
	* Managers can create as many grids as they like for the sorting of different objects.
	*/
	TSharedPtr<TST_SparseGrid<UST_SparseGridComponent>> SparseGridData_Basic;
	
	/////////////////////////////
	///// Blueprint Queries /////
	/////////////////////////////

	/*
	* Gets all registered Sparse Grid objects in a Sphere Shape
	*/
	UFUNCTION(BlueprintCallable, Category = "Sparse Grid|Queries", meta = (DisplayName = "Query Grid [Sphere]", WorldContext = "WorldContextObject"))
	static bool K2_GetComponents_Sphere(const UObject* WorldContextObject, TArray<UST_SparseGridComponent*>& GridComponents, const FVector& WorldLocation, const float SphereRadius, const bool bDrawDebug = false);

	/*
	* Gets all registered Sparse Grid objects in a Sphere Shape
	*/
	UFUNCTION(BlueprintCallable, Category = "Sparse Grid|Queries", meta = (DisplayName = "Query Grid [Capsule]", WorldContext = "WorldContextObject"))
	static bool K2_GetComponents_Capsule(const UObject* WorldContextObject, TArray<UST_SparseGridComponent*>& GridComponents, const FVector& WorldLocation, const FVector& CapsuleAxis, const float CapsuleRadius, const float CapsuleHalfHeight, const bool bDrawDebug = false);

	/*
	* Gets all registered Sparse Grid objects in a AABB Box Shape
	*/
	UFUNCTION(BlueprintCallable, Category = "Sparse Grid|Queries", meta = (DisplayName = "Query Grid [AABB]", WorldContext = "WorldContextObject"))
	static bool K2_GetComponents_Box(const UObject* WorldContextObject, TArray<UST_SparseGridComponent*>& GridComponents, const FVector& WorldLocation, const FVector& BoxExtents, const bool bDrawDebug = false);

	/*
	* Gets all registered Sparse Grid objects in a Rotated Box Shape
	*/
	UFUNCTION(BlueprintCallable, Category = "Sparse Grid|Queries", meta = (DisplayName = "Query Grid [Box]", WorldContext = "WorldContextObject"))
	static bool K2_GetComponents_RotatedBox(const UObject* WorldContextObject, TArray<UST_SparseGridComponent*>& GridComponents, const FVector& WorldLocation, const FRotator& BoxRotation, const FVector& BoxExtents, const bool bDrawDebug = false);

	/*
	* Gets all registered Sparse Grid objects in a Cone Shape
	*/
	UFUNCTION(BlueprintCallable, Category = "Sparse Grid|Queries", meta = (DisplayName = "Query Grid [Cone]", WorldContext = "WorldContextObject"))
	static bool K2_GetComponents_Cone(const UObject* WorldContextObject, TArray<UST_SparseGridComponent*>& GridComponents, const FVector& WorldLocation, const float ConeLength, const float ConeHalfAngleRadians, const FVector& Axis, const bool bDrawDebug = false);
};