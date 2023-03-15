// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridEdMode.h"
#include "ST_SparseGridToolkit.h"

// Editor
#include "EditorModeManager.h"
#include "EditorSupportDelegates.h"
#include "Toolkits/ToolkitManager.h"
#include "Kismet2/KismetEditorUtilities.h"

// Extras
#include "ST_SparseGridData.h"

/////////////////////
///// Lifecycle /////
/////////////////////

FST_SparseGridEdMode::FST_SparseGridEdMode()
	: FEdMode()
{}

FST_SparseGridEdMode::~FST_SparseGridEdMode()
{
	FlushRenderingCommands();
}

/////////////////////
///// Accessors /////
/////////////////////

FST_SparseGridEdMode* FST_SparseGridEdMode::GetSparseGridEditorMode()
{
	return (FST_SparseGridEdMode*)GLevelEditorModeTools().GetActiveMode(FName("SparseGridEditorMode"));
}

UST_SparseGridData* FST_SparseGridEdMode::GetContextGridData() const
{
	const UWorld* lWorld = GetWorld();
	if (lWorld && lWorld->PersistentLevel)
	{
		return lWorld->PersistentLevel->GetAssetUserData<UST_SparseGridData>();
	}

	return nullptr;
}

/////////////////////////////////
///// Editor Mode Interface /////
/////////////////////////////////

void FST_SparseGridEdMode::Enter()
{
	FEdMode::Enter();

	// Select Nothing
	GEditor->SelectNone(false, true);

	// Setup Grid Classes
	PopulateGridDataClasses();

	// Bind Callbacks
	OnWorldChangeDelegateHandle = FEditorSupportDelegates::WorldChange.AddRaw(this, &FST_SparseGridEdMode::HandleWorldChanged, true);
	OnLevelsChangeDelegateHandle = GetWorld()->OnLevelsChanged().AddRaw(this, &FST_SparseGridEdMode::HandleLevelsChanged, true);

	// Create Toolkit
	if (!Toolkit.IsValid())
	{
		Toolkit = MakeShareable(new FST_SparseGridToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}
}

void FST_SparseGridEdMode::Exit()
{
	// Remove Delegates
	FEditorSupportDelegates::WorldChange.Remove(OnWorldChangeDelegateHandle);
	GetWorld()->OnLevelsChanged().Remove(OnLevelsChangeDelegateHandle);

	// Select Nothing
	GEditor->SelectNone(false, true);

	// Close Toolikit
	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}

	// Empty Classes
	GridDataClasses.Empty();

	FEdMode::Exit();
}

void FST_SparseGridEdMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	FEdMode::Render(View, Viewport, PDI);

	const UST_SparseGridData* ContextGrid = GetContextGridData();
	if (ContextGrid)
	{
		ContextGrid->Render(PDI);
	}
}

void FST_SparseGridEdMode::PopulateGridDataClasses()
{
	GridDataClasses.Reset();

	// Find all Grid Sub-Classes
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;

		// Skip deprecated, old or abstract classes
		if (!Class->IsNative() || Class->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_Abstract)) { continue; }

		// Skip Skeleton Classes (Editor Only)
		if (FKismetEditorUtilities::IsClassABlueprintSkeleton(Class)) { continue; }

		// Skip if not a child of the right class
		if (!Class->IsChildOf(UST_SparseGridData::StaticClass())) { continue; }

		GridDataClasses.Add(Class);
	}
}

////////////////////////////
///// Toolkit Commands /////
////////////////////////////

TSharedRef<FUICommandList> FST_SparseGridEdMode::GetUICommandList() const
{
	check(Toolkit.IsValid());
	return Toolkit->GetToolkitCommands();
}

//////////////////////////////////
///// World-Change Callbacks /////
//////////////////////////////////

void FST_SparseGridEdMode::HandleWorldChanged(bool ShouldExitMode)
{
	HandleLevelsChanged(ShouldExitMode);
}

void FST_SparseGridEdMode::HandleLevelsChanged(bool ShouldExitMode)
{
	if (ShouldExitMode)
	{
		RequestDeletion();
	}
}