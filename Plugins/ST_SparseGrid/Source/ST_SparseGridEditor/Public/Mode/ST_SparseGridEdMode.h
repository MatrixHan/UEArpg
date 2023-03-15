// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdMode.h"
#include "Framework/Commands/UICommandList.h"

// Declarations
class UST_SparseGridData;

/*
* Sparse Grid Editor Mode
* Convenient editing tools for the Sparse Grid
*/
class FST_SparseGridEdMode : public FEdMode
{
public:
	// Constructor & Destructor
	FST_SparseGridEdMode();
	virtual ~FST_SparseGridEdMode();

	// Easy Accessors
	static FST_SparseGridEdMode* GetSparseGridEditorMode();
	UST_SparseGridData* GetContextGridData() const;
	FORCEINLINE const TArray<UClass*>& GetGridDataClasses() const { return GridDataClasses; }

	// EdMode Interface
	virtual void Enter() override;
	virtual void Exit() override;
	virtual bool UsesToolkits() const override { return true; }
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const override { return false; }

	// Toolkit Commands
	TSharedRef<FUICommandList> GetUICommandList() const;

protected:
	// Cached Variables
	void PopulateGridDataClasses();
	TArray<UClass*> GridDataClasses;

	// Callback for when the world is changed
	FDelegateHandle OnWorldChangeDelegateHandle;
	FDelegateHandle OnLevelsChangeDelegateHandle;

	void HandleWorldChanged(bool ShouldExitMode);
	void HandleLevelsChanged(bool ShouldExitMode);
};