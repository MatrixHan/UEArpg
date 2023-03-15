// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/BaseToolkit.h"

// Declarations
class FEdMode;
class SWidget;
class SST_SparseGridToolkitWidget;

class FST_SparseGridToolkit : public FModeToolkit
{
public:
	// Constructor
	FST_SparseGridToolkit();

	// FModeToolkit Interface
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost) override;
	virtual FName GetToolkitFName() const override { return FName("SparseGridTK"); }
	virtual FText GetBaseToolkitName() const override { return NSLOCTEXT("SparseGridToolkit", "DisplayName", "Sparse Grid"); }
	virtual FEdMode* GetEditorMode() const override;
	virtual TSharedPtr<SWidget> GetInlineContent() const override;

	// Allow Property Editing?
	bool CanEditGridProperties() const;

	// Combo Box Data
	TSharedRef<SWidget> GenerateGridMenuContent();
	TSharedRef<SWidget> GenerateDensityMenuContent() const;
	
private:
	// Tools
	void OnFitToWorld();
	void OnCreateGrid(UClass* InGridDataClass);
	void OnRemoveGrid();

	void OnModifyDensity(const int32 InDensityModifier, const ETextCommit::Type CommitType = ETextCommit::Default) const;
	void AddDensityEntry(FMenuBuilder& InBuilder, const int32 InModifier) const;
	TSharedRef<SWidget> CreateDensityCustomEntry() const;

	// Handy Tab Spawner Button
	void OnShowHeatmap();

	// Other
	void OnVisitWebsite();
	void OnViewDocumentation();
	void OnViewSupportThread();

	// Cached Vars
	void RefreshGridCreationState();
	uint8 bHasAnyGridData : 1;

	// Widget Ref
	TSharedPtr<SST_SparseGridToolkitWidget> ModeToolsWidget;
};