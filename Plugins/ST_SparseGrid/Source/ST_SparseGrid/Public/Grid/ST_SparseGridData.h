// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "Engine/AssetUserData.h"
#include "Runtime/CoreUObject/Public/Templates/SubclassOf.h"
#include "ST_SparseGridTypes.h"
#include "ST_SparseGridData.generated.h"

// Declarations
class UST_SparseGridManager;

#if WITH_EDITORONLY_DATA
UENUM(BlueprintType)
enum class EST_SGVisualizerType : uint8
{
	DDT_FlatGrid			UMETA(DisplayName = "Flat Grid"),
	DDT_BoxGrid				UMETA(DisplayName = "Box Grid"),
	DDT_FlatGridSprite		UMETA(DisplayName = "Flat Grid Sprite"),
};
#endif

/*
* Basic Grid Data Class
* Sub-class to add custom data if required.
*/
UCLASS(meta = (DisplayName = "Sparse Grid Data"))
class ST_SPARSEGRID_API UST_SparseGridData : public UAssetUserData
{
	GENERATED_BODY()
public:
	// Constructor
	UST_SparseGridData(const FObjectInitializer& OI);

#if WITH_EDITORONLY_DATA
	virtual FText GetGridDiagnosticText() const;
	virtual void Render(FPrimitiveDrawInterface* InPDI) const;
	virtual void FitToWorldBounds(const FVector& InWorldMin, const FVector& InWorldMax);
	virtual void ModifyDensity(int32 InDelta);

	FORCEINLINE EST_SGVisualizerType GetDrawType() const { return DrawAs; }
	FORCEINLINE float GetDrawAltitude() const { return DrawAltitude; }
	FORCEINLINE float GetBoxExtent() const { return BoxExtent; }
	FORCEINLINE const FLinearColor& GetGridColour() const { return GridColour; }
	void RegisterEditorWorld(const UWorld* EditorWorld);
#endif

	FORCEINLINE const TSubclassOf<UST_SparseGridManager> GetManagerClass() const { return ManagerClass; }
	FORCEINLINE int32 GetRegisterAllocSize() const { return RegisterAllocSize; }
	FORCEINLINE int32 GetCellAllocSize() const { return CellAllocSize; }
	FORCEINLINE int32 GetRegisterAllocShrinkMultiplier() const { return RegisterAllocShrinkMultiplier; }
	FORCEINLINE int32 GetCellAllocShrinkMultiplier() const { return CellAllocShrinkMultiplier; }
	FORCEINLINE const FST_GridRef2D& GetGridOrigin() const { return GridOrigin; }
	FORCEINLINE int32 GetNumCellsX() const { return NumCellsX; }
	FORCEINLINE int32 GetNumCellsY() const { return NumCellsY; }
	FORCEINLINE int32 GetCellSize() const { return CellSize; }

	FORCEINLINE void SetRegisterAllocSize(int32 InRegisterAllocSize) { RegisterAllocSize = InRegisterAllocSize; }
	FORCEINLINE void SetCellAllocSize(int32 InCellAllocSize) { CellAllocSize = InCellAllocSize; }
	FORCEINLINE void SetGridOrigin(const FST_GridRef2D& InGridOrigin) { GridOrigin = InGridOrigin; }
	FORCEINLINE void SetNumCellsX(int32 InNumCellsX) { NumCellsX = InNumCellsX; }
	FORCEINLINE void SetNumCellsY(int32 InNumCellsY) { NumCellsY = InNumCellsY; }
	FORCEINLINE void SetCellSize(int32 InCellSize) { CellSize = InCellSize; }

protected:
	/* The manager class we create when using this grid data, or for the current world. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Grid Properties", NoClear)
	TSubclassOf<UST_SparseGridManager> ManagerClass;

	/*
	* Origin of the grid in world-space
	*/
	UPROPERTY(EditAnywhere, Category = "Grid Properties")
	FST_GridRef2D GridOrigin;

	/*
	* Number of Grid Cells in the X Direction.
	* This value should be adjusted so that the cells fully encompass the playable area of the world.
	* Objects outside of the grid will be clamped to boundary cells.
	*/
	UPROPERTY(EditAnywhere, Category = "Grid Properties", meta = (ClampMin = "1.0", UIMin = "1.0", ClampMax = "192", UIMax = "192"))
	int32 NumCellsX;

	/*
	* Number of Grid Cells in the Y Direction.
	* This value should be adjusted so that the cells fully encompass the playable area of the world.
	* Objects outside of the grid will be clamped to boundary cells.
	*/
	UPROPERTY(EditAnywhere, Category = "Grid Properties", meta = (ClampMin = "1.0", UIMin = "1.0", ClampMax = "192", UIMax = "192"))
	int32 NumCellsY;

	/*
	* Dictates the world-space size of the cells in the Spatial Grid.
	* This value should be tuned based on world size and average density of objects.
	*
	* Higher values are better for large maps and low object density.
	* Lower Values are better for small maps and high object density.
	*/
	UPROPERTY(EditAnywhere, Category = "Grid Properties", meta = (ClampMin = "100.0", ClampMax = "16000.0", UIMin = "100.0", UIMax = "16000.0"))
	int32 CellSize;

	/*
	* Allocation Block Size for array of registered grid components
	*
	* Higher values are better for large total object counts or if the number of grid components changed often.
	* Lower values are better for small total object counts.
	*/
	UPROPERTY(EditAnywhere, Category = "Memory Management", meta = (ClampMin = "16", ClampMax = "4096", UIMin = "16", UIMax = "4096"))
	int32 RegisterAllocSize;

	/*
	* Allocation Block Size for array of grid components in a cell
	*
	* Higher values are better for large object counts and/or high object density.
	* Lower values are better for low object counts and/or low object density.
	*/
	UPROPERTY(EditAnywhere, Category = "Memory Management", meta = (ClampMin = "8", ClampMax = "128", UIMin = "8", UIMax = "128"))
	int32 CellAllocSize;
	
	/*
	* Deallocation Control for array of registered grid components
	*1
	* -1 = Never Shrink. List will grow to accommodate new items, but can never shrink.
	* 0 = Always Shrink. List will always shrink to the nearest multiplier of RegisterAllocSize that can accommodate all items.
	* # = Variable Shrink. List will shrink to the nearest multiplier of RegisterAllocSize that can accommodate all items, but only if the slack is above # * RegisterAllocSize
	*
	*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Memory Management", meta = (ClampMin = "-1", ClampMax = "64", UIMin = "-1", UIMax = "64"))
	int32 RegisterAllocShrinkMultiplier;

	/*
	* Deallocation control for array of registered grid components in a cell
	*
	* -1 = Never Shrink. Cell will grow to accommodate new items, but can never shrink.
	* 0 = Always Shrink. Cell will always shrink to the nearest multiplier of CellAllocSize that can accommodate all items.
	* # = Variable Shrink. Cell will shrink to the nearest multiplier of CellAllocSize that can accommodate all items, but only if the slack is above # * CellAllocSize
	*
	*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Memory Management", meta = (ClampMin = "-1", ClampMax = "64", UIMin = "-1", UIMax = "64"))
	int32 CellAllocShrinkMultiplier;

	///////////////////////////////
	///// Debug Visualization /////
	///////////////////////////////

#if WITH_EDITORONLY_DATA
	// How to draw the debug grid
	UPROPERTY(EditAnywhere, NonTransactional, Category = "Visualization")
	EST_SGVisualizerType DrawAs;

	// Altitude of the 2D Debug Grid
	UPROPERTY(EditAnywhere, NonTransactional, Category = "Visualization")
	float DrawAltitude;

	// Z Extent of the box grid
	UPROPERTY(EditAnywhere, NonTransactional, Category = "Visualization", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float BoxExtent;

	// Draw Colour
	UPROPERTY(EditAnywhere, NonTransactional, Category = "Visualization")
	FLinearColor GridColour;

	// Rendering
	void Render_Box(FPrimitiveDrawInterface* PDI) const;
	void Render_Grid(FPrimitiveDrawInterface* PDI, const float InAltitude) const;
	void Render_Grid_Sprite(FPrimitiveDrawInterface* PDI, const float InAltitude) const;	
private:
	const UWorld* m_EditorWorld;
#endif
};