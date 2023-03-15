// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "UObject/GCObject.h"
#include "Widgets/SLeafWidget.h"
#include "Styling/SlateBrush.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

// Declarations
class UST_SparseGridManager;
class UTexture2D;

/*
* Sparse-Grid Heat-Map
* Heat-map widget for debugging / viewing grid density and population
*/
class SST_SparseGridHeatMap : public SLeafWidget, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SST_SparseGridHeatMap)
		: _HotColour(FLinearColor::Red)
		, _ColdColour(FLinearColor::Green)
		, _HotThreshold(16.f)
	{}

		/* Bindings */
		SLATE_ATTRIBUTE(FLinearColor, HotColour);
		SLATE_ATTRIBUTE(FLinearColor, ColdColour);
		SLATE_ATTRIBUTE(float, HotThreshold);
	SLATE_END_ARGS()

	// Default Constructor
	SST_SparseGridHeatMap();

	// SWidget Interface
	void Construct(const FArguments& InArgs);
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// Design
	void SetHotColour(const FLinearColor& InHotColour);
	void SetColdColour(const FLinearColor& InColdColour);
	void SetHotThreshold(const float InHotThreshold);
	void SetGridManager(UST_SparseGridManager* InGrid, const FName InGridID);

	void CheckBuffer();

	// FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	FORCEINLINE const UTexture2D* GetDataBuffer() const { return DataBuffer; }

protected:
	// Data Buffer
	FSlateBrush Image;
	UTexture2D* DataBuffer;	
	void CreateBuffer();
	void DestroyBuffer();

	// Grid Reference
	TWeakObjectPtr<UST_SparseGridManager> GridManager;
	FName GridID;
	TArray<uint32> PixelColours;
	SIZE_T CachedSizeBytes;

	// Design Attributes
	TAttribute<FLinearColor> ColdColour;
	TAttribute<FLinearColor> HotColour;
	TAttribute<float> HotThreshold;
};