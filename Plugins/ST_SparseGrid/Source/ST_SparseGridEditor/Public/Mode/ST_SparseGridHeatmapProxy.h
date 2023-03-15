// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "ST_SparseGridHeatmapProxy.generated.h"

/*
* Sparse Grid Editor Proxy Object
*
* Stores all vars related to Sparse Grid Toolkit
* Convenient container allowing us to take advantage of details panel customization
*/
UCLASS(Config=Editor)
class UST_SparseGridHeatmapProxy : public UObject
{
	GENERATED_BODY()
public:
	// Constructor
	UST_SparseGridHeatmapProxy(const FObjectInitializer& OI);

	// Property Access
	FLinearColor GetColdColour() const { return ColdColour; }
	FLinearColor GetHotColour() const { return HotColour; }
	float GetHotThreshold() const { return FMath::CeilToInt(HotThreshold); }
	bool GetUpdateCVars() const { return bUpdateCVars; }

	// UObject Interface
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	void UpdateCVars();

	/*
	* Colour of 'Empty' Cells
	*/
	UPROPERTY(Config, EditAnywhere, Category = "Heatmap Properties")
	FLinearColor ColdColour;

	/*
	* Colour of cells which meet or exceed the Hot Threshold
	*/
	UPROPERTY(Config, EditAnywhere, Category = "Heatmap Properties")
	FLinearColor HotColour;

	/*
	* Number of items in a cell to qualify as 'Hot'
	*/
	UPROPERTY(Config, EditAnywhere, Category = "Heatmap Properties", meta = (ClampMin = "1.0", UIMin = "1.0"))
	int32 HotThreshold;

	/*
	* If true, then Sparse Grid Console Variables will be updated to match the Heatmap Properties
	* This ensures that the 3D Debug Grid matches the Heat Map
	*/
	UPROPERTY(Config, EditAnywhere, Category = "Grid Properties")
	bool bUpdateCVars;
};