// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "ST_SparseGridTypes.h"

// Required
#include "Engine/World.h"
#include "UObject/UObjectIterator.h"

#if SPARSE_GRID_DEBUG
#include "DrawDebugHelpers.h"
#endif

/////////////////////
///// Profiling /////
/////////////////////

DECLARE_STATS_GROUP(TEXT("Sparse Grid"), STATGROUP_SparseGrid, STATCAT_Advanced);

// General
DECLARE_CYCLE_STAT(TEXT("Update Grid"), STAT_UpdateGrid, STATGROUP_SparseGrid);
DECLARE_CYCLE_STAT(TEXT("Query Populations"), STAT_QueryPopulation, STATGROUP_SparseGrid);
DECLARE_CYCLE_STAT(TEXT("Query Tile"), STAT_QueryTile, STATGROUP_SparseGrid);

// Queries
DECLARE_CYCLE_STAT(TEXT("Query Grid - Sphere"), STAT_QueryGrid_Sphere, STATGROUP_SparseGrid);
DECLARE_CYCLE_STAT(TEXT("Query Grid - Capsule"), STAT_QueryGrid_Capsule, STATGROUP_SparseGrid);
DECLARE_CYCLE_STAT(TEXT("Query Grid - Box"), STAT_QueryGrid_Box, STATGROUP_SparseGrid);
DECLARE_CYCLE_STAT(TEXT("Query Grid - Rotated Box"), STAT_QueryGrid_RotatedBox, STATGROUP_SparseGrid);
DECLARE_CYCLE_STAT(TEXT("Query Grid - Cone"), STAT_QueryGrid_Cone, STATGROUP_SparseGrid);

// Forward-Declarations
template<class T>
class TST_SparseGrid;

template<class T>
class TST_SparseGridCell;

#if SPARSE_GRID_DEBUG
#define GATHER_DEBUG_PARAMETERS																									\
	const UWorld* DebugWorld = GetGridWorld();																					\
	check(DebugWorld);																											\
	const float DrawQueryTime = ST_SparseGridCVars::CVarQueryDebugTime.GetValueOnGameThread();									\
	const float DrawQueryThickness = ST_SparseGridCVars::CVarDebugGridThickness.GetValueOnGameThread();
#endif

////////////////////////////
///// Sparse Grid Cell /////
////////////////////////////

template<class T>
class TST_SparseGridCell
{
public:
	TST_SparseGridCell(const int32 InAllocSize, const int32 InShrinkMultiplier)
		: CellObjects(TArray<T*>())
		, AllocSize(InAllocSize)
		, ShrinkMultiplier(InShrinkMultiplier)
	{}

	/*
	* Adds an element to the grid cell.
	* Adding will fail if the element already belongs to another cell.
	*/
	void Add(T* InObject)
	{
		checkf(InObject != nullptr, TEXT("TST_SparseGridCell::Add - Invalid Object!"));
		checkfSlow(InObject->GetSparseGridData().GetCellSubIndex() == INDEX_NONE, TEXT("TST_SparseGridCell::Add - Object Already In Another Cell!"));

		// Allocate in Blocks
		if (CellObjects.GetSlack() <= 0)
		{
			CellObjects.Reserve(CellObjects.Max() + AllocSize);
			UE_LOG(LogST_SparseGrid, VeryVerbose, TEXT("Cell Objects Resized! '%i' Max Objects."), CellObjects.Max());
		}

		// Add the new item, and store the cell index
		InObject->AccessSparseGridData().SetCellSubIndex(CellObjects.Add(InObject));
	}

	/*
	* Removes an element from the grid cell
	* This will change the order of items within the cell.
	*/
	void Remove(T* InObject)
	{
		checkf(InObject != nullptr, TEXT("TST_SparseGridCell::Remove - Invalid Object!"));
		checkfSlow(CellObjects.IsValidIndex(InObject->GetSparseGridData().GetCellSubIndex()) && InObject == CellObjects[InObject->GetSparseGridData().GetCellSubIndex()], TEXT("TST_SparseGridCell::Remove - Invalid Object At Cell Sub Index '%s'!"), InObject->GetSparseGridData().GetCellSubIndex());

		if (InObject != CellObjects.Last())
		{
			const int32 SwapIndex = InObject->GetSparseGridData().GetCellSubIndex();
			CellObjects.Swap(SwapIndex, CellObjects.Num() - 1);

			T* LastObject = CellObjects[SwapIndex];
			checkfSlow(LastObject != nullptr, TEXT("Invalid Cell Component"));

			LastObject->AccessSparseGridData().SetCellSubIndex(SwapIndex);
		}

		checkfSlow(CellObjects.Last() == InObject, TEXT("TST_SparseGridCell::Remove - Old Object Not Last Element In Array!"));

		CellObjects.RemoveAt(CellObjects.Num() - 1, 1, false);
		InObject->AccessSparseGridData().SetCellSubIndex(INDEX_NONE);

		// Shrink in Blocks Too
		const int32 Slack = CellObjects.GetSlack();
		if (ShrinkMultiplier >= 0 && Slack % AllocSize == 0 && Slack > AllocSize * ShrinkMultiplier)
		{
			CellObjects.Shrink();
		}
	}

	FORCEINLINE const TArray<T*>& GetObjects() const
	{
		return CellObjects;
	}

#if WITH_EDITOR
	void GetMemoryInfo(uint64& OutAlloc, uint64& OutUsed) const
	{
		OutAlloc = sizeof(void*) * CellObjects.Max();
		OutUsed = sizeof(void*) * CellObjects.Num();
	}
#endif

private:
	// Allow Private Access
	friend class TST_SparseGrid<T>;

	TArray<T*> CellObjects;
	int32 AllocSize;
	int32 ShrinkMultiplier;

	// Required for TSharedPtr<>
 	TST_SparseGridCell()
		: CellObjects(TArray<T*>())
 		, AllocSize(16)
 		, ShrinkMultiplier(1)
 	{}
};

//////////////////////////////////////
///// Sparse Grid Cell Container /////
//////////////////////////////////////

template<class T>
class TST_SparseGrid
{
	//////////////////////
	////// Lifecycle /////
	//////////////////////
public:
	// Constructor
	TST_SparseGrid(
			const UWorld* InGridWorld,
			const FST_GridRef2D& InGridOrigin,
			const FST_GridRef2D& InNumCells,
			const int32 InCellSize,
			const int32 InRegisterAllocSize,
			const int32 InRegisterShrinkMultiplier,
			const int32 InCellAllocSize,
			const int32 InCellShrinkMultiplier)
		: GridWorld(InGridWorld)
		, GridOrigin(InGridOrigin)
		, NumCells(InNumCells)
		, CellSize(InCellSize)
		, RegisterAllocSize(InRegisterAllocSize)
		, RegisterAllocShrinkMultiplier(InRegisterShrinkMultiplier)
	{
		// Ensure we have *some* cells, to prevent divide by zero errors
		check(GridWorld.IsValid() && NumCells.X > 0 && NumCells.Y > 0 && CellSize > 0);

		// Create Cells
		const int32 TotalCells = InNumCells.X * InNumCells.Y;

		GridCells.Reserve(TotalCells);
		for (int32 CellIdx = 0; CellIdx < TotalCells; CellIdx++)
		{
			GridCells.Add(TST_SparseGridCell<T>(InCellAllocSize, InCellShrinkMultiplier));
		}

		// Initialize Culling Properties
		const float HalfCellSize = (float)CellSize * 0.5f;
		CellBoundsRadius = FVector2D(HalfCellSize, HalfCellSize).Size();
		CellBoundsRadiusSqrd = CellBoundsRadius * CellBoundsRadius;
	}

	// Destructor
	~TST_SparseGrid()
	{
		GridWorld = nullptr;
		Empty();
		GridCells.Empty();
	}

protected:
	// Default Constructor
	// Protected - Cannot be instantiated directly.
	// Required for TSharedPtr<>
	TST_SparseGrid()
		: GridWorld(nullptr)
		, RegisteredObjects(TArray<T>())
		, GridCells(TArray<T>())
		, GridOrigin(FST_GridRef2D(-2500, -2500))
		, NumCells(FST_GridRef2D(5))
		, CellSize(1000)
		, RegisterAllocSize(128)
		, RegisterAllocShrinkMultiplier(1)
		, CellBoundsRadius(0.f)
		, CellBoundsRadiusSqrd(0.f)
	{}

	//////////////////////////////
	///// Grid Functionality /////
	//////////////////////////////
public:
	/*
	* Updates object placement in the grid.
	* Typically once per-frame for each grid instance.
	*/
	void Update()
	{
		SCOPE_CYCLE_COUNTER(STAT_UpdateGrid)

#if ENABLE_GRID_BOUNDS
		// Reset Bounds
		ObjectBounds = FST_SparseGridBounds();
#endif

		for (T* ObjectItr : RegisteredObjects)
		{
			checkSlow(ObjectItr != nullptr);

			const FVector WorldPosition = ObjectItr->GetSparseGridLocation();

#if ENABLE_GRID_BOUNDS
			// Update Bounds
			ObjectBounds.Update(WorldPosition);
#endif

			const int32 DesiredCell = WorldToCell(FVector2D(WorldPosition));
			const int32 CurrentCell = ObjectItr->GetSparseGridData().GetCellIndex();

			if (DesiredCell != CurrentCell)
			{
				UE_LOG(LogST_SparseGrid, VeryVerbose, TEXT("Moving Object '%s' from Cell ID '%i' to Cell ID '%i'"), *GetNameSafe(ObjectItr), CurrentCell, DesiredCell);

				GridCells[CurrentCell].Remove(ObjectItr);
				GridCells[DesiredCell].Add(ObjectItr);

				ObjectItr->AccessSparseGridData().SetCellIndex(DesiredCell);
			}
		}
	}

	/*
	* Registers an object with the grid.
	* Returns true if successfully registered (or already registered)
	*/
	bool Add(T* InObject)
	{
		checkf(InObject != nullptr, TEXT("Invalid Object!"));
		checkf(InObject->GetWorld() == GetGridWorld(), TEXT("Invalid Object World!"));

		if (InObject->GetSparseGridData().IsValid())
		{
			// If the object has valid data, it should *only* exist in this grid.
			const int32 GridIndex = InObject->GetSparseGridData().GetGridIndex();
			if (RegisteredObjects.IsValidIndex(GridIndex) && RegisteredObjects[GridIndex] == InObject)
			{
				UE_LOG(LogST_SparseGrid, Verbose, TEXT("'%s' is already registered!"), *GetNameSafe(InObject));
				return true;
			}
			else
			{
				UE_LOG(LogST_SparseGrid, Warning, TEXT("'%s' is already registered in another grid!"), *GetNameSafe(InObject));
				return false;
			}
		}
		else
		{
			UE_LOG(LogST_SparseGrid, VeryVerbose, TEXT("Registering Sparse Grid Object: '%s'"), *GetNameSafe(InObject));

			// Grow Array if Required
			if (RegisteredObjects.GetSlack() <= 0)
			{
				RegisteredObjects.Reserve(RegisteredObjects.Max() + RegisterAllocSize);
				UE_LOG(LogST_SparseGrid, Verbose, TEXT("Registered Grid Objects Array Resized! '%i' Max elements."), RegisteredObjects.Max());
			}

			InObject->AccessSparseGridData().SetGridIndex(RegisteredObjects.Add(InObject));

			// Add to Cell (Ensure Is Valid)
			const int32 DesiredCell = WorldToCell(FVector2D(InObject->GetSparseGridLocation()));
			checkfSlow(GridCells.IsValidIndex(DesiredCell), TEXT("Object '%s' at position '%s' cannot be registered in Sparse Grid Cell '%i'"), *GetNameSafe(InObject), *InObject->GetSparseGridLocation().ToString(), DesiredCell);

			InObject->AccessSparseGridData().SetCellIndex(DesiredCell);
			GridCells[DesiredCell].Add(InObject);

			return true;
		}
	}

	/*
	* Unregisters an object with the grid. 
	* Returns true if successfully unregistered (or already unregistered)
	*/
	bool Remove(T* InObject)
	{
		checkf(InObject != nullptr, TEXT("Invalid Component!"));

		if (InObject->GetSparseGridData().IsClear())
		{
			UE_LOG(LogST_SparseGrid, Verbose, TEXT("'%s' is not registered!"), *GetNameSafe(InObject));
			return true;
		}
		else
		{
			// If the object has valid data, we can only unregister if it belongs to this grid.
			const int32 GridIndex = InObject->GetSparseGridData().GetGridIndex();
			if (!RegisteredObjects.IsValidIndex(GridIndex) || RegisteredObjects[GridIndex] != InObject)
			{
				UE_LOG(LogST_SparseGrid, Warning, TEXT("'%s' is registered in another grid - cannot unregister!"), *GetNameSafe(InObject));
				return false;
			}
			else
			{
				UE_LOG(LogST_SparseGrid, VeryVerbose, TEXT("Unregistering Sparse Grid Object: '%s'"), *GetNameSafe(InObject));

				// Remove from Cell
				const int32 CurrentCell = InObject->GetSparseGridData().GetCellIndex();
				InObject->AccessSparseGridData().SetCellIndex(INDEX_NONE);
				GridCells[CurrentCell].Remove(InObject);

				// We want to move the component the end of the registered array
				// Need to swap if it's not already there
				if (InObject != RegisteredObjects.Last())
				{
					const int32 SwapIndex = InObject->GetSparseGridData().GetGridIndex();
					RegisteredObjects.Swap(SwapIndex, RegisteredObjects.Num() - 1);

					// Update the component we swapped with so it has the correct index
					T* LastObject = RegisteredObjects[SwapIndex];
					checkSlow(LastObject != nullptr);

					LastObject->AccessSparseGridData().SetGridIndex(SwapIndex);
				}

				// Check Last Element is the InObject
				checkfSlow(RegisteredObjects.Last() == InObject, TEXT("Old Object Not Last In Array!"));

				// Remove and clear cell index
				RegisteredObjects.RemoveAt(RegisteredObjects.Num() - 1, 1, false);
				InObject->AccessSparseGridData().SetGridIndex(INDEX_NONE);

				// Shrink if required
				const int32 Slack = RegisteredObjects.GetSlack();
				if (Slack >= 0 && Slack % RegisterAllocSize == 0 && Slack > RegisterAllocSize * RegisterAllocShrinkMultiplier)
				{
					RegisteredObjects.Shrink();
				}

				return true;
			}
		}
	}

	/*
	* Initializes the sparse grid with all grid objects in it's assigned world
	* If bAllowChildClasses is true, then we will also register child classes of the given type (must be true for blueprint classes)
	* This function is expensive, do not use it often.
	*/
	void Init(const bool bAllowChildClasses = false)
	{
		const UWorld* lWorld = GetGridWorld();
		check(lWorld);

		for (TObjectIterator<T> SGIterator; SGIterator; ++SGIterator)
		{
			T* ObjectItr = *SGIterator;
			if (!ObjectItr || ObjectItr->GetWorld() != lWorld || ObjectItr->IsPendingKillOrUnreachable())
			{
				continue;
			}

			if (!bAllowChildClasses && ExactCast<T>(ObjectItr) == nullptr)
			{
				UE_LOG(LogST_SparseGrid, VeryVerbose, TEXT("Skipped Sparse Grid registration of '%s' because it does not match exact class '%s'"), *GetNameSafe(ObjectItr), *GetNameSafe(T::StaticClass()));
				continue;
			}

			if (ObjectItr->GetSparseGridData().IsClear())
			{
				Add(ObjectItr);
			}
			else
			{
				UE_LOG(LogST_SparseGrid, VeryVerbose, TEXT("Skipped Sparse Grid registration of '%s' because it is already registered."), *GetNameSafe(ObjectItr));
			}
		}
	}

	/*
	* Unregisters all registered objects.
	* Does not destroy cells but clears all allocations.
	*/
	void Empty()
	{
		for (TST_SparseGridCell<T>& CellItr : GridCells)
		{
			CellItr.CellObjects.Empty();
		}

		for (T* ObjectItr : RegisteredObjects)
		{
			checkSlow(ObjectItr != nullptr);

			ObjectItr->AccessSparseGridData().SetGridIndex(INDEX_NONE);
			ObjectItr->AccessSparseGridData().SetCellIndex(INDEX_NONE);
			ObjectItr->AccessSparseGridData().SetCellSubIndex(INDEX_NONE);
		}

		RegisteredObjects.Empty();
	}

	//////////////////////
	///// Properties /////
	//////////////////////
public:
	FORCEINLINE const UWorld* GetGridWorld() const
	{
		return GridWorld.Get();
	}

	FORCEINLINE FST_GridRef2D GetNumCells() const
	{
		return NumCells;
	}

	FORCEINLINE FST_GridRef2D GetGridOrigin() const
	{
		return GridOrigin;
	}

	FORCEINLINE FST_GridRef2D GetGridMax() const
	{
		return GridOrigin + (NumCells * CellSize);
	}

	FORCEINLINE int32 GetCellSize() const
	{
		return CellSize;
	}

	// TODO: Make const TST_SparseGridCell
	FORCEINLINE const TArray<TST_SparseGridCell<T>>& GetGridCells() const
	{
		return GridCells;
	}

	FORCEINLINE const TArray<T*>& GetRegisteredObjects() const
	{
		return RegisteredObjects;
	}

private:
	/*
	* The world this grid belongs to.
	*/
	TWeakObjectPtr<const UWorld> GridWorld;

	/*
	* All objects registered in the grid.
	*/
	TArray<T*> RegisteredObjects;

	/*
	* All cells in the grid.
	*/
	TArray<TST_SparseGridCell<T>> GridCells;

	/*
	* Origin of the Grid in World-Space
	* This should be adjusted so that the grid fully encompasses the playable area of the world.
	* Objects outside of the grid will be clamped to boundary cells.
	*/
	FST_GridRef2D GridOrigin;

	/*
	* Number of Grid Cells in the X and Y Directions.
	* This value should be adjusted so that the cells fully encompass the playable area of the world.
	* Objects outside of the grid will be clamped to boundary cells.
	*/
	FST_GridRef2D NumCells;

	/*
	* Dictates the world-space size of the cells in the 2D Spatial Grid.
	* This value should be tuned based on world size and average density of objects.
	*
	* Higher values are better for large maps and low object density.
	* Lower Values are better for small maps and high object density.
	*/
	int32 CellSize;

	/////////////////////////////
	///// Memory Management /////
	/////////////////////////////
private:
	int32 RegisterAllocSize;
	int32 RegisterAllocShrinkMultiplier;

	/////////////////////
	///// Utilities /////
	/////////////////////
public:
	FORCEINLINE int32 WorldToCell(FVector2D InWorldXY) const
	{
		InWorldXY = InWorldXY - GridOrigin.ToVector();

		return GetCellIndex(FST_GridRef2D(
			FMath::Clamp(FMath::FloorToInt(InWorldXY.X / CellSize), 0, NumCells.X - 1),
			FMath::Clamp(FMath::FloorToInt(InWorldXY.Y / CellSize), 0, NumCells.Y - 1)));
	}

	FORCEINLINE int32 GetCellIndex(const FST_GridRef2D& CellXY) const
	{
		return CellXY.Y + CellXY.X * NumCells.Y;
		// CellXY.X + CellXY.Y * NumCells.X;
	}

	FORCEINLINE bool IsValidCellIndex(const int32 CellID) const
	{
		return GridCells.IsValidIndex(CellID);
	}

	FORCEINLINE FST_GridRef2D GetCellXY(const int32 CellID) const
	{
		return IsValidCellIndex(CellID) ? FST_GridRef2D(CellID & NumCells.Y, CellID / NumCells.Y) : FST_GridRef2D(INDEX_NONE);
		//return IsValidCellIndex(CellID) ? FST_GridRef2D(CellID & NumCells.X, CellID / NumCells.X) : FST_GridRef2D(INDEX_NONE);
	}

	FORCEINLINE FVector2D GetCellCenter(const FST_GridRef2D& CellXY) const
	{
		return FST_GridRef2D(GridOrigin + (CellXY * CellSize)).ToVector() + ((float)CellSize * 0.5f);
	}

	FORCEINLINE FST_GridRef2D GetCellMin(const FST_GridRef2D& CellXY) const
	{
		return GridOrigin + (CellXY * CellSize);
	}

	FORCEINLINE FST_GridRef2D GetCellMax(const FST_GridRef2D& CellXY) const
	{
		return GridOrigin + CellSize + (CellXY * CellSize);
	}

	FORCEINLINE bool IsBoundaryCell(const FST_GridRef2D& CellXY) const
	{
		return (CellXY.X == 0 || CellXY.Y == 0 || CellXY.Y == NumCells.Y - 1 || CellXY.X == NumCells.X - 1);
	}

	/*
	* Get Populations of each cell in cell order
	* Useful for drawing heat-map data
	*/
	void GetGridCellPopulations(TArray<uint32>& OutPopulation) const
	{
		SCOPE_CYCLE_COUNTER(STAT_QueryPopulation);

		const int32 NumGridCells = GridCells.Num();
		OutPopulation.Reset(NumGridCells);

		for (int32 CellId = 0; CellId < NumGridCells; CellId++)
		{
			OutPopulation.Add(static_cast<uint32>(GridCells[CellId].GetObjects().Num()));
		}
	}

	//////////////////////////
	///// Search Culling /////
	//////////////////////////
public:
	/*
	* Fast index-based rejection of cells using an AABB Bounding Box
	* Ensures that cell-rejection performance is constant regardless of cell count.
	*
	* @param WorldSearchOrigin	- World-Space XY position of the search
	* @param WorldSearchExtents	- World-Space extents of the search around the search origin.
	* @param bClampGridEdge		- If true, searches taking place outside of the grid bounds will still search boundary tiles.
	* @return					- Tile of grid cells
	*/
	FST_SparseGridCellTile GetSearchTile(const FVector2D& WorldSearchOrigin, const FVector2D& WorldSearchExtents, const bool bClampGridEdge = true) const
	{
		SCOPE_CYCLE_COUNTER(STAT_QueryTile);

		// Work out an axis-aligned bounding box for this location, given the range we're searching in
		FVector2D SearchBoundsMin = WorldSearchOrigin - WorldSearchExtents;
		FVector2D SearchBoundsMax = WorldSearchOrigin + WorldSearchExtents;

		// Grid Max
		const FST_GridRef2D GridMax = GetGridMax();

		// Clamp the bounds to the playable area. It can be rectangular or square.
		SearchBoundsMin.X = FMath::Clamp<float>(SearchBoundsMin.X, GridOrigin.X, GridMax.X);
		SearchBoundsMin.Y = FMath::Clamp<float>(SearchBoundsMin.Y, GridOrigin.Y, GridMax.Y);
		SearchBoundsMax.X = FMath::Clamp<float>(SearchBoundsMax.X, GridOrigin.X, GridMax.X);
		SearchBoundsMax.Y = FMath::Clamp<float>(SearchBoundsMax.Y, GridOrigin.Y, GridMax.Y);

		// Offset back to Origin
		SearchBoundsMin = SearchBoundsMin - GridOrigin.ToVector();
		SearchBoundsMax = SearchBoundsMax - GridOrigin.ToVector();

		// Work out the start and end indices
		FST_SparseGridCellTile ReturnVal = FST_SparseGridCellTile(
			FST_GridRef2D(FMath::FloorToInt(SearchBoundsMin.X / CellSize), FMath::FloorToInt(SearchBoundsMin.Y / CellSize)),
			FST_GridRef2D(FMath::CeilToInt(SearchBoundsMax.X / CellSize), FMath::CeilToInt(SearchBoundsMax.Y / CellSize)));

		// Any objects outside of the grid are still linked to the nearest Edge Tile
		// If we are searching outside the grid, then we probably also want to search the edge tiles
		if (bClampGridEdge)
		{
			ReturnVal.Start.X = FMath::Clamp(ReturnVal.Start.X, 0, NumCells.X - 1);
			ReturnVal.Start.Y = FMath::Clamp(ReturnVal.Start.Y, 0, NumCells.Y - 1);
			ReturnVal.End.X = FMath::Clamp(ReturnVal.End.X, 1, NumCells.X);
			ReturnVal.End.Y = FMath::Clamp(ReturnVal.End.Y, 1, NumCells.Y);
		}

		return ReturnVal;
	}

	FORCEINLINE float GetCellBoundsRadius() const
	{
		return CellBoundsRadius;
	}

	FORCEINLINE float GetCellBoundsRadiusSquared() const
	{
		return CellBoundsRadiusSqrd;
	}

#if ENABLE_GRID_BOUNDS
	FORCEINLINE const FST_SparseGridBounds& GetObjectBounds() const
	{
		return ObjectBounds;
	}
#endif

	/*
	* Cull cells based on distance to point (cell bounds radius)
	* Currently used for sphere tests.
	*/
	FORCEINLINE bool CullCell_Range(const FST_GridRef2D& CellXY, const FVector2D& InPoint, const double Distance) const
	{
		// Skip cell culling for boundary cells, since it won't work if we're outside the grid
		if (IsBoundaryCell(CellXY)) { return false; }

		return FVector2D(GetCellCenter(CellXY) - InPoint).SizeSquared() > FMath::Square(Distance + CellBoundsRadius);
	}

	/*
	* Cull cells based on distance from line (Cell Bounds Radius)
	* Currently used for Capsule and Cone tests
	*/
	bool CullCell_Line(const FST_GridRef2D& CellXY, const FVector2D& InLineStart, const FVector2D& InLineEnd, const float DistFromLine) const
	{
		// Skip cell culling for boundary cells, since it won't work if we're outside the grid
		if (IsBoundaryCell(CellXY)) { return false; }

		const FVector2D CellCenter = GetCellCenter(CellXY);
		const FVector2D ClosestPoint = FMath::ClosestPointOnSegment2D(CellCenter, InLineStart, InLineEnd);

		const double DSqrd = FVector2D(CellCenter - ClosestPoint).SizeSquared();
		const double XSqrd = FMath::Square(DistFromLine + CellBoundsRadius);

		return DSqrd > XSqrd;
	}

private:
#if ENABLE_GRID_BOUNDS
	/*
	* Object Bounds
	* Allows fast-rejection for searches taking place way above or below the current area of objects
	*/
	FST_SparseGridBounds ObjectBounds;
#endif

	double CellBoundsRadius;
	double CellBoundsRadiusSqrd;

	//////////////////////////
	///// Search Queries /////
	//////////////////////////
public:
	/*
	* Sphere Query
	* Returns all registered objects within a sphere.
	*/
	template<class AllocatorType>
	void QueryGrid_Sphere(TArray<T*, AllocatorType>& OutObjects, const FVector& InWorldLocation, const float InSphereRadius, const bool bDrawDebug = false) const
	{
		SCOPE_CYCLE_COUNTER(STAT_QueryGrid_Sphere)

#if SPARSE_GRID_DEBUG
		GATHER_DEBUG_PARAMETERS;

		if (bDrawDebug) { DrawDebugSphere(DebugWorld, InWorldLocation, InSphereRadius, 12, FColor::Green, false, DrawQueryTime, 0, DrawQueryThickness); }
#endif

#if ENABLE_GRID_BOUNDS
		// Skip if the search bounds do not overlap object bounds, as we won't return anything anyway
		if (ObjectBounds.CanFastReject(InWorldLocation, FVector(InSphereRadius, InSphereRadius, InSphereRadius))) { return; }
#endif

		const FVector2D TileBoundsXY = FVector2D(InWorldLocation.X, InWorldLocation.Y);
		const FST_SparseGridCellTile Tile = GetSearchTile(TileBoundsXY, FVector2D(InSphereRadius, InSphereRadius));

		const FST_GridRef2D GridMax = GetGridMax();
		const FVector2D XYClamped = FVector2D(FMath::Clamp<float>(TileBoundsXY.X, GridOrigin.X, GridMax.X), FMath::Clamp<float>(TileBoundsXY.Y, GridOrigin.Y, GridMax.Y));

		const double DistSqrd = InSphereRadius * InSphereRadius;
		for (int32 CIdx = Tile.Start.Y; CIdx < Tile.End.Y; CIdx++)
		{
			for (int32 RIdx = Tile.Start.X; RIdx < Tile.End.X; RIdx++)
			{
				const FST_GridRef2D CellXY = FST_GridRef2D(RIdx, CIdx);
				const int32 CellIndex = GetCellIndex(CellXY);
				if (GridCells[CellIndex].GetObjects().Num() && !CullCell_Range(CellXY, XYClamped, InSphereRadius))
				{
#if SPARSE_GRID_DEBUG
					if (bDrawDebug) { DrawDebugCell(CellXY, FLinearColor(0.f, 1.f, 0.f, 0.25f)); }
#endif
					for (T* ObjectItr : GridCells[CellIndex].GetObjects())
					{
						const FVector ObjectLoc = ObjectItr->GetSparseGridLocation();
						if (FVector::DistSquared(InWorldLocation, ObjectLoc) <= DistSqrd)
						{
#if SPARSE_GRID_DEBUG
							if (bDrawDebug) { DrawDebugLine(DebugWorld, InWorldLocation, ObjectLoc, FColor::Green, false, DrawQueryTime, 0, DrawQueryThickness); }
#endif

							OutObjects.Add(ObjectItr);
						}
					}
				}
#if SPARSE_GRID_DEBUG
				else if (bDrawDebug) { DrawDebugCell(CellXY, FLinearColor(1.f, 0.f, 0.f, 0.25f), DrawQueryTime); }
#endif
			}
		}
	}

	/*
	* Capsule Query
	* Returns all registered objects within an orientated capsule.
	*/
	template<class AllocatorType>
	void QueryGrid_Capsule(TArray<T*, AllocatorType>& OutObjects, const FVector& InWorldLocation, const FVector& InUpAxis, float InCapsuleRadius, float InCapsuleHalfHeight, const bool bDrawDebug = false) const
	{
		SCOPE_CYCLE_COUNTER(STAT_QueryGrid_Capsule)

		// Clamp to sensible values
		InCapsuleHalfHeight = FMath::Max3(0.f, InCapsuleHalfHeight, InCapsuleRadius);
		InCapsuleRadius = FMath::Clamp(InCapsuleRadius, 0.f, InCapsuleHalfHeight);

		const FMatrix CapsuleToWorld = FTransform(FRotationMatrix::MakeFromZ(InUpAxis).ToQuat(), InWorldLocation).ToMatrixNoScale();
		const FVector CapsuleBoundsExtents = FVector(InCapsuleRadius, InCapsuleRadius, InCapsuleHalfHeight);
		const FBoxSphereBounds AABB = FBoxSphereBounds(FBox(-CapsuleBoundsExtents, CapsuleBoundsExtents)).TransformBy(CapsuleToWorld);

#if SPARSE_GRID_DEBUG
		GATHER_DEBUG_PARAMETERS;

		if (bDrawDebug)
		{
			DrawDebugBox(DebugWorld, InWorldLocation, CapsuleBoundsExtents, CapsuleToWorld.ToQuat(), FColor::Orange, false, DrawQueryTime, 0, DrawQueryThickness);;
			DrawDebugCapsule(DebugWorld, InWorldLocation, InCapsuleHalfHeight, InCapsuleRadius, FRotationMatrix::MakeFromZ(InUpAxis).ToQuat(), FColor::Green, false, DrawQueryTime, 0, DrawQueryThickness);
		}
#endif

#if ENABLE_GRID_BOUNDS
		// Skip if the search bounds do not overlap object bounds, as we won't return anything anyway
		if (ObjectBounds.CanFastReject(InWorldLocation, AABB.BoxExtent)) { return; }
#endif

		const FVector2D TileBoundsXY = FVector2D(InWorldLocation.X, InWorldLocation.Y);
		const FST_SparseGridCellTile Tile = GetSearchTile(TileBoundsXY, FVector2D(AABB.BoxExtent.X, AABB.BoxExtent.Y));

		const FVector Dir = InUpAxis * (InCapsuleHalfHeight - InCapsuleRadius);
		const FVector CapsuleStart = InWorldLocation + Dir;
		const FVector CapsuleEnd = InWorldLocation - Dir;
		const float RSq = InCapsuleRadius * InCapsuleRadius;

		const FST_GridRef2D GridMax = GetGridMax();
		const FVector2D XYClamped = FVector2D(FMath::Clamp<float>(TileBoundsXY.X, GridOrigin.X, GridMax.X), FMath::Clamp<float>(TileBoundsXY.Y, GridOrigin.Y, GridMax.Y));
		const FVector2D CullStart = XYClamped + FVector2D(Dir);
		const FVector2D CullEnd = XYClamped - FVector2D(Dir);

		for (int32 CIdx = Tile.Start.Y; CIdx < Tile.End.Y; CIdx++)
		{
			for (int32 RIdx = Tile.Start.X; RIdx < Tile.End.X; RIdx++)
			{
				const FST_GridRef2D CellXY = FST_GridRef2D(RIdx, CIdx);
				const int32 CellIndex = GetCellIndex(CellXY);
				if (GridCells[CellIndex].GetObjects().Num() && !CullCell_Line(CellXY, CullStart, CullEnd, InCapsuleRadius))
				{
#if SPARSE_GRID_DEBUG
					if (bDrawDebug) { DrawDebugCell(CellXY, FLinearColor(0.f, 1.f, 0.f, 0.25f), DrawQueryTime); }
#endif

					for (T* ObjectItr : GridCells[CellIndex].GetObjects())
					{
						const FVector Location = ObjectItr->GetSparseGridLocation();
						const FVector ClosestPoint = FMath::ClosestPointOnSegment(Location, CapsuleStart, CapsuleEnd);

						if (FVector::DistSquared(ClosestPoint, Location) <= RSq)
						{
#if SPARSE_GRID_DEBUG
							if (bDrawDebug) { DrawDebugLine(DebugWorld, ClosestPoint, Location, FLinearColor(0.f, 1.f, 0.f, 0.25f).ToFColor(false), false, DrawQueryThickness, 0, DrawQueryThickness); }
#endif
							OutObjects.Add(ObjectItr);
						}
					}
				}
#if SPARSE_GRID_DEBUG
				else if (bDrawDebug) { DrawDebugCell(CellXY, FLinearColor(1.f, 0.f, 0.f, 0.25f), DrawQueryTime); }
#endif
			}
		}
	}

	/*
	* Box Query
	* Returns all registered objects in an axis-aligned bounding box.
	*/
	template<class AllocatorType>
	void QueryGrid_Box(TArray<T*, AllocatorType>& OutObjects, const FVector& InWorldLocation, const FVector& InBoxExtents, const bool bDrawDebug = false) const
	{
		SCOPE_CYCLE_COUNTER(STAT_QueryGrid_Box)

		const FVector ExtentMin = InWorldLocation - InBoxExtents;
		const FVector ExtentMax = InWorldLocation + InBoxExtents;

#if SPARSE_GRID_DEBUG
		GATHER_DEBUG_PARAMETERS;

		if (bDrawDebug) { DrawDebugBox(DebugWorld, InWorldLocation, InBoxExtents, FColor::Blue, false, DrawQueryTime, 0, DrawQueryThickness); }
#endif

#if ENABLE_GRID_BOUNDS
		if (ObjectBounds.CanFastReject(InWorldLocation, InBoxExtents)) { return; }
#endif

		// Axis-Aligned Tiles
		const FVector2D TileBoundsXY = FVector2D(InBoxExtents.X, InBoxExtents.Y);
		const FST_SparseGridCellTile Tile = GetSearchTile(FVector2D(InWorldLocation), FVector2D(InBoxExtents));

		// Iterate All Tiles
		for (int32 CIdx = Tile.Start.Y; CIdx < Tile.End.Y; CIdx++)
		{
			for (int32 RIdx = Tile.Start.X; RIdx < Tile.End.X; RIdx++)
			{
#if SPARSE_GRID_DEBUG
				if (bDrawDebug) { DrawDebugCell(FST_GridRef2D(RIdx, CIdx), FLinearColor(0.f, 1.f, 0.f, 0.25f), DrawQueryTime); }
#endif
				const int32 CellIndex = GetCellIndex(FST_GridRef2D(RIdx, CIdx));
				for (T* ObjectItr : GridCells[CellIndex].GetObjects())
				{
					const FVector Location = ObjectItr->GetSparseGridLocation();
					if (Location.X >= ExtentMin.X && Location.X <= ExtentMax.X && Location.Y >= ExtentMin.Y && Location.Y <= ExtentMax.Y && Location.Z >= ExtentMin.Z && Location.Z <= ExtentMax.Z)
					{
#if SPARSE_GRID_DEBUG
						if (bDrawDebug) { DrawDebugLine(DebugWorld, InWorldLocation, ObjectItr->GetSparseGridLocation(), FColor::Green, false, DrawQueryTime, 0, DrawQueryThickness); }
#endif
						OutObjects.Add(ObjectItr);
					}
				}
			}
		}
	}

	/*
	* Rotated Box Query
	* Finds all registered objects within an non-axis-aligned bounding box.
	*/
	template<class AllocatorType>
	void QueryGrid_RotatedBox(TArray<T*, AllocatorType>& OutObjects, const FVector& InWorldLocation, const FQuat& InBoxRotation, const FVector& InBoxExtents, const bool bDrawDebug = false) const
	{
		// Skip transforms if rotation is zero
		if (InBoxRotation.IsIdentity())
		{
			QueryGrid_Box(OutObjects, InWorldLocation, InBoxExtents, bDrawDebug);
			return;
		}

		SCOPE_CYCLE_COUNTER(STAT_QueryGrid_RotatedBox)

		const FMatrix BoxToWorld = FTransform(InBoxRotation, InWorldLocation).ToMatrixNoScale();
		const FBoxSphereBounds AABB = FBoxSphereBounds(FBox(-InBoxExtents, InBoxExtents)).TransformBy(BoxToWorld);

#if SPARSE_GRID_DEBUG
		GATHER_DEBUG_PARAMETERS;

		if (bDrawDebug)
		{
			DrawDebugBox(DebugWorld, InWorldLocation, InBoxExtents, InBoxRotation, FColor::Green, false, DrawQueryTime, 0, DrawQueryThickness);
			DrawDebugBox(DebugWorld, AABB.Origin, AABB.BoxExtent, FQuat::Identity, FColor::Red, false, DrawQueryTime, 0, DrawQueryThickness);
		}
#endif

#if ENABLE_GRID_BOUNDS
		if (ObjectBounds.CanFastReject(InWorldLocation, AABB.BoxExtent)) { return; }
#endif

		const FVector2D TileBoundsXY = FVector2D(InWorldLocation.X, InWorldLocation.Y);
		const FST_SparseGridCellTile Tile = GetSearchTile(TileBoundsXY, FVector2D(AABB.BoxExtent.X, AABB.BoxExtent.Y));

		for (int32 CIdx = Tile.Start.Y; CIdx < Tile.End.Y; CIdx++)
		{
			for (int32 RIdx = Tile.Start.X; RIdx < Tile.End.X; RIdx++)
			{
#if SPARSE_GRID_DEBUG
				if (bDrawDebug) { DrawDebugCell(FST_GridRef2D(RIdx, CIdx), FLinearColor(0.f, 1.f, 0.f, 0.25f), DrawQueryTime); }
#endif
				const int32 CellIndex = GetCellIndex(FST_GridRef2D(RIdx, CIdx));
				for (T* ObjectItr : GridCells[CellIndex].GetObjects())
				{
					const FVector TransformedLocation = BoxToWorld.InverseTransformPosition(ObjectItr->GetSparseGridLocation());
					if (TransformedLocation.X >= -InBoxExtents.X && TransformedLocation.X <= InBoxExtents.X
						&& TransformedLocation.Y >= -InBoxExtents.Y && TransformedLocation.Y <= InBoxExtents.Y
						&& TransformedLocation.Z >= -InBoxExtents.Z && TransformedLocation.Z <= InBoxExtents.Z)
					{
#if SPARSE_GRID_DEBUG
						if (bDrawDebug) { DrawDebugLine(DebugWorld, InWorldLocation, ObjectItr->GetSparseGridLocation(), FColor::Green, false, DrawQueryTime, 0, DrawQueryThickness); }
#endif

						OutObjects.Add(ObjectItr);
					}
				}
			}
		}
	}

	/*
	* Cone Query
	* Finds all registered objects within a cone.
	*/
	template<class AllocatorType>
	void QueryGrid_Cone(TArray<T*, AllocatorType>& OutObjects, const FVector& InWorldLocation, const float InConeLength, const float InConeHalfAngleRadians, const FVector& InAxis, const bool bDrawDebug = false) const
	{
		SCOPE_CYCLE_COUNTER(STAT_QueryGrid_Cone)

		const FVector ConeCenter = InWorldLocation + InAxis * (InConeLength * 0.5f);
		const float ConeEndRadius = InConeLength * FMath::Tan(InConeHalfAngleRadians);
		const FMatrix ConeToWorld = FTransform(FRotationMatrix::MakeFromX(InAxis).ToQuat(), ConeCenter).ToMatrixNoScale();
		const FVector ConeBoundsExtents = FVector(InConeLength * 0.5f, ConeEndRadius, ConeEndRadius);
		const FBoxSphereBounds AABB = FBoxSphereBounds(FBox(-ConeBoundsExtents, ConeBoundsExtents)).TransformBy(ConeToWorld);

#if SPARSE_GRID_DEBUG
		GATHER_DEBUG_PARAMETERS;

		if (bDrawDebug)
		{
			DrawDebugBox(DebugWorld, ConeCenter, ConeBoundsExtents, ConeToWorld.ToQuat(), FColor::Orange, false, DrawQueryTime, 0, DrawQueryThickness);
			DrawDebugCone(DebugWorld, InWorldLocation, InAxis, InConeLength, InConeHalfAngleRadians, InConeHalfAngleRadians, 16, FColor::Green, false, DrawQueryTime, 0, DrawQueryThickness);
			DrawDebugBox(DebugWorld, AABB.Origin, AABB.BoxExtent, FQuat::Identity, FColor::Red, false, DrawQueryTime, 0, DrawQueryThickness);
		}
#endif

#if ENABLE_GRID_BOUNDS
		if (ObjectBounds.CanFastReject(InWorldLocation, AABB.BoxExtent)) { return; }
#endif

		const FVector2D TileBoundsXY = FVector2D(ConeCenter.X, ConeCenter.Y);
		const FST_SparseGridCellTile Tile = GetSearchTile(TileBoundsXY, FVector2D(AABB.BoxExtent.X, AABB.BoxExtent.Y));

		const FST_GridRef2D GridMax = GetGridMax();
		const FVector2D ConeEnd2D = FVector2D(InAxis) * InConeLength;
		const FVector2D LineStart2D = FVector2D(FMath::Clamp<float>(InWorldLocation.X, GridOrigin.X, GridMax.X), FMath::Clamp<float>(InWorldLocation.Y, GridOrigin.Y, GridMax.Y));
		const FVector2D LineEnd2D = FVector2D(FMath::Clamp<float>(ConeEnd2D.X, GridOrigin.X, GridMax.X), FMath::Clamp<float>(ConeEnd2D.Y, GridOrigin.Y, GridMax.Y));

		const float ConeLenSq = InConeLength * InConeLength;
		const float ConeCos = FMath::Cos(InConeHalfAngleRadians);

		for (int32 CIdx = Tile.Start.Y; CIdx < Tile.End.Y; CIdx++)
		{
			for (int32 RIdx = Tile.Start.X; RIdx < Tile.End.X; RIdx++)
			{
				const FST_GridRef2D CellXY = FST_GridRef2D(RIdx, CIdx);
				const int32 CellIndex = GetCellIndex(CellXY);
				if (GridCells[CellIndex].GetObjects().Num() && !CullCell_Line(CellXY, LineStart2D, LineEnd2D, ConeEndRadius))
				{
#if SPARSE_GRID_DEBUG
					if (bDrawDebug) { DrawDebugCell(CellXY, FLinearColor(0.f, 1.f, 0.f, 0.25f), DrawQueryTime); }
#endif
					for (T* ObjectItr : GridCells[CellIndex].GetObjects())
					{
						const FVector OwnerLocation = ObjectItr->GetSparseGridLocation();
						const float DSqrd = FVector::DistSquared(InWorldLocation, OwnerLocation);
						if (DSqrd > ConeLenSq)
						{
							continue;
						}

						const float Dot = FVector::DotProduct(InAxis, FVector(OwnerLocation - InWorldLocation).GetSafeNormal());
						if (Dot < ConeCos)
						{
							continue;
						}

#if SPARSE_GRID_DEBUG
						if (bDrawDebug) { DrawDebugLine(DebugWorld, InWorldLocation, OwnerLocation, FColor::Green, false, DrawQueryTime, 0, DrawQueryThickness); }
#endif

						OutObjects.Add(ObjectItr);
					}
				}
#if SPARSE_GRID_DEBUG
				else if (bDrawDebug) { DrawDebugCell(CellXY, FLinearColor(1.f, 0.f, 0.f, 0.25f), DrawQueryTime); }
#endif
			}
		}
	}

#if WITH_EDITOR
	//////////////////
	///// Editor /////
	//////////////////
public:
	void GetEditorDebugInfo(int32& OutTotalObjects, uint64& OutRegisterAlloc, uint64& OutRegisterUsed, uint64& OutCellAlloc, uint64& OutCellUsed) const
	{
		OutTotalObjects = GetRegisteredObjects().Num();
		OutRegisterAlloc = sizeof(void*) * GetRegisteredObjects().Max();
		OutRegisterUsed = sizeof(void*) * GetRegisteredObjects().Num();

		OutCellAlloc = 0;
		OutCellUsed = 0;
		for (const TST_SparseGridCell<T>& Cells : GetGridCells())
		{
			uint64 A, U;
			Cells.GetMemoryInfo(A, U);

			OutCellAlloc += A;
			OutCellUsed += U;
		}
	}
#endif

#if SPARSE_GRID_DEBUG
	/////////////////////
	///// Debugging /////
	/////////////////////
public:
	void DrawDebugGrid() const
	{
		// Grab Console Colours
		const FLinearColor DebugHotColour = FLinearColor::FromSRGBColor(FColor::FromHex(ST_SparseGridCVars::CVarHotHex.GetValueOnGameThread()));
		const FLinearColor DebugColdColour = FLinearColor::FromSRGBColor(FColor::FromHex(ST_SparseGridCVars::CVarColdHex.GetValueOnGameThread()));

#if ENABLE_GRID_BOUNDS
		if (ST_SparseGridCVars::CVarDrawBounds.GetValueOnGameThread())
		{
			FVector Center;
			FVector Extent;
			ObjectBounds.GetBoundingBox(Center, Extent);

			DrawDebugBox(GetGridWorld(), Center, Extent, FColor::Blue, false, -1.f, 0, ST_SparseGridCVars::CVarDebugGridThickness.GetValueOnGameThread());
		}
#endif

		// Draw each Grid Cell
		for (int32 ColIdx = 0; ColIdx < NumCells.Y; ColIdx++)
		{
			for (int32 RowIdx = 0; RowIdx < NumCells.X; RowIdx++)
			{
				const FST_GridRef2D CellXY = FST_GridRef2D(RowIdx, ColIdx);
				const FVector2D CellOrigin = GetCellMin(CellXY).ToVector();

				const int32 CellIndex = GetCellIndex(CellXY);
				const int32 CellCount = GridCells[CellIndex].GetObjects().Num();

				// Colorize Grid By Cell Object Count
				const float Progress = FMath::Clamp((float)CellCount / (float)FMath::Max(ST_SparseGridCVars::CVarDebugHotThreshold.GetValueOnGameThread(), 1), 0.f, 1.f);
				const FColor CellColour = FLinearColor::LerpUsingHSV(DebugColdColour, DebugHotColour, Progress).ToFColor(true);
				const FVector CellCenter = FVector(GetCellCenter(CellXY), ST_SparseGridCVars::CVarDebugGridHeight.GetValueOnGameThread());

				if (ST_SparseGridCVars::CVarFillGrid.GetValueOnGameThread() == 0)
				{
					const float HalfThickness = ST_SparseGridCVars::CVarDebugGridThickness.GetValueOnGameThread() * 0.5f;

					// Vertices. Offset by line thickness to avoid overlap
					const FVector Vert1 = FVector(CellOrigin + HalfThickness, ST_SparseGridCVars::CVarDebugGridHeight.GetValueOnGameThread());
					const FVector Vert2 = FVector(CellOrigin + FVector2D(CellSize, 0.f) + FVector2D(-HalfThickness, HalfThickness), ST_SparseGridCVars::CVarDebugGridHeight.GetValueOnGameThread());
					const FVector Vert3 = FVector(CellOrigin + CellSize - HalfThickness, ST_SparseGridCVars::CVarDebugGridHeight.GetValueOnGameThread());
					const FVector Vert4 = FVector(CellOrigin + FVector2D(0.f, CellSize) + FVector2D(HalfThickness, -HalfThickness), ST_SparseGridCVars::CVarDebugGridHeight.GetValueOnGameThread());

					DrawDebugLine(GetGridWorld(), Vert1, Vert2, CellColour, false, -1.f, 0, ST_SparseGridCVars::CVarDebugGridThickness.GetValueOnGameThread());
					DrawDebugLine(GetGridWorld(), Vert2, Vert3, CellColour, false, -1.f, 0, ST_SparseGridCVars::CVarDebugGridThickness.GetValueOnGameThread());
					DrawDebugLine(GetGridWorld(), Vert3, Vert4, CellColour, false, -1.f, 0, ST_SparseGridCVars::CVarDebugGridThickness.GetValueOnGameThread());
					DrawDebugLine(GetGridWorld(), Vert4, Vert1, CellColour, false, -1.f, 0, ST_SparseGridCVars::CVarDebugGridThickness.GetValueOnGameThread());
				}
				else
				{
					DrawDebugSolidPlane(GetGridWorld(), FPlane(CellCenter, FVector::UpVector), CellCenter, FVector2D(CellSize, CellSize) * 0.5f, CellColour);
				}

				if (ST_SparseGridCVars::CVarDrawLinks.GetValueOnGameThread())
				{
					for (const T* ObjectItr : GridCells[CellIndex].GetObjects())
					{
						checkSlow(ObjectItr != nullptr);
						DrawDebugLine(GetGridWorld(), CellCenter, ObjectItr->GetSparseGridLocation(), FColor::Orange, false, -1.f, 0, ST_SparseGridCVars::CVarDebugGridThickness.GetValueOnGameThread());
					}
				}

				if (ST_SparseGridCVars::CVarDrawInfo.GetValueOnGameThread())
				{
					const FVector TextLocation = CellCenter + FVector(0.f, 0.f, 128.f);
					DrawDebugString(GetGridWorld(), TextLocation, FString::Printf(TEXT("Cell ID: '%i'\nObjects: '%i'"), CellIndex, CellCount), nullptr, CellColour, GetGridWorld()->GetDeltaSeconds(), true);
				}
			}
		}
	}

	void DrawDebugCell(const FST_GridRef2D& CellXY, const FLinearColor& Colour, const float DrawTime = -1.f) const
	{
		const FVector CellCenter = FVector(GetCellCenter(CellXY), ST_SparseGridCVars::CVarDebugGridHeight.GetValueOnGameThread() + 1.f);
		FTransform CircleTrans = FTransform(FRotationMatrix::MakeFromX(FVector::UpVector).Rotator(), CellCenter, FVector(1.f));

		DrawDebugSolidPlane(GetGridWorld(), FPlane(CellCenter, FVector::UpVector), CellCenter, FVector2D(CellSize, CellSize) * 0.5f, Colour.ToFColor(true), false, DrawTime);
		DrawDebugCircle(GetGridWorld(), CircleTrans.ToMatrixWithScale(), CellBoundsRadius, 64, Colour.ToFColor(true), false, DrawTime, 0, 2.f, false);
	}
#endif
};

#if SPARSE_GRID_DEBUG
#undef GATHER_DEBUG_PARAMETERS
#endif