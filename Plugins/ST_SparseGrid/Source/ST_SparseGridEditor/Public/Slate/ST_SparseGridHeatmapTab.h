// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "UObject/GCObject.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

// Declarations
class SST_SparseGridHeatMap;
class UST_SparseGridManager;
class IDetailsView;
class UST_SparseGridHeatmapProxy;
class SDockTab;
class STextComboBox;
class FSpawnTabArgs;

/*
* Sparse-Grid Heatmap Tool
* Used to debug and profile Sparse Grid performance in editor
*/
class SST_SparseGridHeatmapTab : public SCompoundWidget, public FGCObject
{
public:
	// Construction
	SLATE_BEGIN_ARGS(SST_SparseGridHeatmapTab) {}
	SLATE_END_ARGS()

		SST_SparseGridHeatmapTab();
	virtual ~SST_SparseGridHeatmapTab();

	// Slate Construct
	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);

	// FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	// Static Tab Creator
	static TSharedRef<SDockTab> CreateTab(const FSpawnTabArgs& InArgs);

protected:
	// Child Widgets
	TSharedRef<SWidget> CreateHeatmapWidget();
	TSharedRef<SWidget> CreateToolbar();
	TSharedRef<SWidget> CreateDiagnostics();

	TSharedPtr<SST_SparseGridHeatMap> HeatmapWidget;
	TSharedPtr<SWidget> DiagnosticsWidget;

	// List of Managers we can debug, and the corresponding options
	TSharedPtr<STextComboBox> ManagersComboBox;
	TArray<TSharedPtr<FString>> ManagerOptions;
	TArray<TWeakObjectPtr<UST_SparseGridManager>> Managers;

	// List of Grids we can debug, and the corresponding options
	TSharedPtr<STextComboBox> GridsComboBox;
	TArray<TSharedPtr<FString>> GridOptions;
	TArray<FName> Grids;

	// Option Generation
	void GenerateManagerOptions(bool bRestoreSelection);
	void GenerateGridOptions();

	// Option Selection
	TSharedPtr<FString> GetInitialManagerOption();
	TSharedPtr<FString> GetInitialGridOption();
	void OnManagerSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnGridSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void UpdateDebugTarget();

	bool IsManagerComboBoxEnabled() const;
	bool IsGridComboBoxEnabled() const;

	// Diagnostics Functionality
	void ToggleDiagnostics();
	bool AreDiagnosticsVisible() const;
	EVisibility GetDiagnosticsVisibility() const;
	uint8 bShowingDiagnostics : 1;

	// Diagnostics Data
	// Note: We used to use SIZE_T for this, but Mac doesn't like that.
	uint64 CellAllocSize;
	uint64 CellUsedSize;
	uint64 TotalAllocSize;
	uint64 TotalUsedSize;
	int32 TotalObjects;

	FText Diagnostics_GetCellMemoryText() const;
	TOptional<float> Diagnostics_GetCellMemoryRatio() const;
	FText Diagnostics_GetTotalObjectsText() const;
	FText Diagnostics_GetTotalMemoryText() const;
	TOptional<float> Diagnostics_GetTotalMemoryRatio() const;
	
	// Delegates
	void OnPIEStarted(bool bIsSimulating);
	void OnPIEEnded(bool bIsSimulating);
	void OnMapChanged(uint32 InMapChangeFlags);
	void OnNewCurrentLevel();

	FDelegateHandle OnPieStartedDelegateHandle;
	FDelegateHandle OnPieEndedDelegateHandle;
	FDelegateHandle OnMapChangedDelegateHandle;
	FDelegateHandle OnNewCurrentLevelDelegateHandle;

	// Current Object Pointers
	void ExportHeatMap();
	bool HasManagerSelected() const { return CurrentDebuggingManager.IsValid(); }
	TWeakObjectPtr<UST_SparseGridManager> CurrentDebuggingManager;
	FName CurrentDebuggingGrid;

	// Details Panel Proxy Object
	void RefreshProxyDetailsPanel();
	TSharedPtr<IDetailsView> Proxy_DetailsPanel;
	UST_SparseGridHeatmapProxy* EditorProxy;
};