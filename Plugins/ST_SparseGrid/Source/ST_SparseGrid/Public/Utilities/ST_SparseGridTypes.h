// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "ST_SparseGridTypes.generated.h"

ST_SPARSEGRID_API DECLARE_LOG_CATEGORY_EXTERN(LogST_SparseGrid, Log, All);
ST_SPARSEGRID_API DECLARE_LOG_CATEGORY_EXTERN(LogST_SparseGridManager, Log, All);

#define SPARSE_GRID_DEBUG !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

// Whether to enable grid bounds checking
// This allows searches to be rejected faster if they take place outside of the grid object bounds
// In some cases (high objects counts and/or high numbers of queries) this can be slower, profile for best results.
#define ENABLE_GRID_BOUNDS 1

/////////////////////////////
///// Console Variables /////
/////////////////////////////

#if SPARSE_GRID_DEBUG
#include "HAL/IConsoleManager.h"

// Sparse Grid Console vars wrapped in a utility class
// Allows them to be more easily exported, widely accessible, and prevents multiple console object registration issues at runtime.
class ST_SPARSEGRID_API ST_SparseGridCVars
{
public:
	// Enable Drawing Debug Grid (can be slow with high cell counts. Use the heat-map where possible)
	static TAutoConsoleVariable<int32> CVarDrawDebug;

	// Length of time to draw query information
	static TAutoConsoleVariable<float> CVarQueryDebugTime;

#if ENABLE_GRID_BOUNDS
	// Enable drawing Frame Bounds
	static TAutoConsoleVariable<int32> CVarDrawBounds;
#endif

	// Draw debug grid as filled quads (slower)
	static TAutoConsoleVariable<int32> CVarFillGrid;

	// Draw links between registered components and their cells
	static TAutoConsoleVariable<int32> CVarDrawLinks;

	// Draw debug cell information above the cell
	static TAutoConsoleVariable<int32> CVarDrawInfo;

	// Draw Debug Grid Height in world
	static TAutoConsoleVariable<float> CVarDebugGridHeight;

	// Debug Line Thickness
	static TAutoConsoleVariable<float> CVarDebugGridThickness;

	// Cell must have this many objects to be considered 'Hot'
	static TAutoConsoleVariable<int32> CVarDebugHotThreshold;

	// Cell colour when "Hot" (at or above DebugHotThreshold)
	static TAutoConsoleVariable<FString> CVarHotHex;

	// Cell colour when "cold" (emtpy)
	static TAutoConsoleVariable<FString> CVarColdHex;
};
#endif

//////////////////////////////////////
///// Sparse Grid Cell Reference /////
//////////////////////////////////////

/*
* Objects added to the grid must implement this as a member variable named SparseGridData.
* Objects have a reverse-reference to their cell and grid to make array operations much faster.
*/
struct ST_SPARSEGRID_API FST_SparseGridData
{
public:
	FST_SparseGridData()
		: GridIndex(INDEX_NONE)
		, CellIndex(INDEX_NONE)
		, CellSubIndex(INDEX_NONE)
	{}

	// Validation
	FORCEINLINE bool IsValid() const { return GridIndex != INDEX_NONE && CellIndex != INDEX_NONE && CellSubIndex != INDEX_NONE; }
	FORCEINLINE bool IsClear() const { return GridIndex == INDEX_NONE && CellIndex == INDEX_NONE && CellSubIndex == INDEX_NONE; }

	// Accessors
	FORCEINLINE int32 GetGridIndex() const { return GridIndex; }
	FORCEINLINE void SetGridIndex(const int32 InIndex) { GridIndex = InIndex; }

	FORCEINLINE int32 GetCellIndex() const { return CellIndex; }
	FORCEINLINE void SetCellIndex(const int32 InIndex) { CellIndex = InIndex; }

	FORCEINLINE int32 GetCellSubIndex() const { return CellSubIndex; }
	FORCEINLINE void SetCellSubIndex(const int32 InIndex) { CellSubIndex = InIndex; }

private:
	int32 GridIndex;
	int32 CellIndex;
	int32 CellSubIndex;
};

USTRUCT(BlueprintType, meta = (DisplayName = "2D Grid Ref"))
struct ST_SPARSEGRID_API FST_GridRef2D
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Int2D")	int32 X;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Int2D")	int32 Y;

	FST_GridRef2D()
		: X(0)
		, Y(0)
	{}

	FST_GridRef2D(const int32 InSize)
		: X(InSize)
		, Y(InSize)
	{}

	FST_GridRef2D(const int32 InX, const int32 InY)
		: X(InX)
		, Y(InY)
	{}

	// Equality Operators
	FORCEINLINE bool operator==(const FST_GridRef2D& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}
	FORCEINLINE bool operator!=(const FST_GridRef2D& Other) const
	{
		return X != Other.X && Y != Other.Y;
	}
	
	// Non-Const Math Operators
	FORCEINLINE FST_GridRef2D& operator*=(const int32 InScale)
	{
		X *= InScale;
		Y *= InScale;
		return *this;
	}
	FORCEINLINE FST_GridRef2D& operator/=(const int32 InDivisor)
	{
		X /= InDivisor;
		Y /= InDivisor;
		return *this;
	}
	FORCEINLINE FST_GridRef2D& operator+=(const int32 InAddition)
	{
		X += InAddition;
		Y += InAddition;
		return *this;
	}
	FORCEINLINE FST_GridRef2D& operator-=(const int32 InSubtraction)
	{
		X -= InSubtraction;
		Y -= InSubtraction;
		return *this;
	}

	FORCEINLINE FST_GridRef2D& operator*=(const FST_GridRef2D& Other)
	{
		X *= Other.X;
		Y *= Other.Y;
		return *this;
	}
	FORCEINLINE FST_GridRef2D& operator/=(const FST_GridRef2D& Other)
	{
		X /= Other.X;
		Y /= Other.Y;
		return *this;
	}
	FORCEINLINE FST_GridRef2D& operator+=(const FST_GridRef2D& Other)
	{
		X += Other.X;
		Y += Other.Y;
		return *this;
	}
	FORCEINLINE FST_GridRef2D& operator-=(const FST_GridRef2D& Other)
	{
		X -= Other.X;
		Y -= Other.Y;
		return *this;
	}

	// Const Math Operators
	FORCEINLINE FST_GridRef2D operator*(const int32 InScale) const
	{
		return FST_GridRef2D(*this) *= InScale;
	}
	FORCEINLINE FST_GridRef2D operator/(const int32 InDivisor) const
	{
		return FST_GridRef2D(*this) /= InDivisor;
	}
	FORCEINLINE FST_GridRef2D operator+(const int32 InAddition) const
	{
		return FST_GridRef2D(*this) += InAddition;
	}
	FORCEINLINE FST_GridRef2D operator-(const int32 InSubtraction) const
	{
		return FST_GridRef2D(*this) -= InSubtraction;
	}

	FORCEINLINE FST_GridRef2D operator*(const FST_GridRef2D& Other) const
	{
		return FST_GridRef2D(*this) *= Other;
	}
	FORCEINLINE FST_GridRef2D operator/(const FST_GridRef2D& Other) const
	{
		return FST_GridRef2D(*this) /= Other;
	}
	FORCEINLINE FST_GridRef2D operator+(const FST_GridRef2D& Other) const
	{
		return FST_GridRef2D(*this) += Other;
	}
	FORCEINLINE FST_GridRef2D operator-(const FST_GridRef2D& Other) const
	{
		return FST_GridRef2D(*this) -= Other;
	}

	// FVector2D Conversions
	FORCEINLINE FVector2D ToVector() const { return FVector2D(X, Y); }
};

/* Frame Object Bounds */
struct ST_SPARSEGRID_API FST_SparseGridBounds
{
public:
	FST_SparseGridBounds()
		: FrameMax(FVector::ZeroVector)
		, FrameMin(FVector::ZeroVector)
		, bIsClear(true)
	{}

	// Allow for fast search rejection when search area will not return any objects
	FORCEINLINE bool CanFastReject(const FVector& InWorldPosition, const FVector& InSearchExtent) const
	{
		if (!bIsClear)
		{
			const FVector SearchMin = InWorldPosition - InSearchExtent;
			const FVector SearchMax = InWorldPosition + InSearchExtent;

			return SearchMin.X <= FrameMax.X
				&& SearchMax.X >= FrameMin.X
				&& SearchMin.Y <= FrameMax.Y
				&& SearchMax.Y >= FrameMin.Y
				&& SearchMin.Z <= FrameMax.Z
				&& SearchMax.Z >= FrameMin.Z ? false : true;
		}
		else
		{
			return false;
		}
	}

	// Updates Min and Max
	FORCEINLINE void Update(const FVector& InWorldPosition)
	{
		if (bIsClear)
		{
			bIsClear = false;
			FrameMin = InWorldPosition;
			FrameMax = InWorldPosition;
		}
		else
		{
			FrameMin.X = FMath::Min(FrameMin.X, InWorldPosition.X);
			FrameMin.Y = FMath::Min(FrameMin.Y, InWorldPosition.Y);
			FrameMin.Z = FMath::Min(FrameMin.Z, InWorldPosition.Z);
			FrameMax.X = FMath::Max(FrameMax.X, InWorldPosition.X);
			FrameMax.Y = FMath::Max(FrameMax.Y, InWorldPosition.Y);
			FrameMax.Z = FMath::Max(FrameMax.Z, InWorldPosition.Z);
		}
	}

	FORCEINLINE void GetBoundingBox(FVector& OutCenter, FVector& OutExtent) const
	{
		OutCenter = (FrameMin + FrameMax) * 0.5f;
		OutExtent = FrameMax - OutCenter;
	}

private:
	FVector FrameMax;
	FVector FrameMin;
	uint8 bIsClear : 1;
};

/*
* Sparse Grid Cell Tile
* Used for fast cell lookups
*/
USTRUCT(BlueprintType)
struct ST_SPARSEGRID_API FST_SparseGridCellTile
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "Indices") FST_GridRef2D Start;
	UPROPERTY(BlueprintReadOnly, Category = "Indices") FST_GridRef2D End;

	FST_SparseGridCellTile()
		: Start(INDEX_NONE)
		, End(INDEX_NONE)
	{}

	FST_SparseGridCellTile(const FST_GridRef2D& InStart, const FST_GridRef2D& InEnd)
		: Start(InStart)
		, End(InEnd)
	{}
};