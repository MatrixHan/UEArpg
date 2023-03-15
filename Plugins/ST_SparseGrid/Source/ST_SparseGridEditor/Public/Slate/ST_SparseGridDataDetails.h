// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/SlateEnums.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;
class UST_SparseGridData;
struct EVisibility;

class FST_SparseGridDataDetails : public IDetailCustomization
{
public:
	// Creates the Details Panel Instance
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

protected:
	// Customizations
	void CustomizeGridProperties(IDetailLayoutBuilder& DetailBuilder);
	void CustomizeMemoryManagement(IDetailLayoutBuilder& DetailBuilder);
	void CustomizeVisualization(IDetailLayoutBuilder& DetailBuilder);

	// Property Bindings
	TOptional<int32> GetGridOriginX() const;
	TOptional<int32> GetGridOriginY() const;
	TOptional<int32> GetNumCellsX() const;
	TOptional<int32> GetNumCellsY() const;

	EVisibility GetBoxExtentVisibility() const;

	// Property Setters
	void SetGridOriginX(int32 InOriginX, ETextCommit::Type CommitInfo);
	void SetGridOriginY(int32 InOriginY, ETextCommit::Type CommitInfo);
	void SetNumCellsX(int32 InNumCellsX, ETextCommit::Type CommitInfo);
	void SetNumCellsY(int32 InNumCellsY, ETextCommit::Type CommitInfo);

	FText GetCellShrinkTooltipText() const;
	FText GetListShrinkTooltipText() const;

	// Cached SparseGridData Object
	TWeakObjectPtr<UST_SparseGridData> Cached_DataObject;
};