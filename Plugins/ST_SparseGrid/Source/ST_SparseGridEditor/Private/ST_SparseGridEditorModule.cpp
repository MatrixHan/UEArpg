// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridEditorModule.h"

// Extras
#include "ST_SparseGridStyle.h"
#include "ST_SparseGridHeatmapTab.h"
#include "ST_SparseGridEdMode.h"
#include "ST_SparseGridCommands.h"
#include "ST_SparseGridData.h"
#include "ST_SparseGridDataDetails.h"

// Editor
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"

// Property Editor
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/PropertyEditor/Public/PropertyEditorDelegates.h"

IMPLEMENT_MODULE(FST_SparseGridEditorModule, ST_SparseGridEditor)
DEFINE_LOG_CATEGORY(LogST_SparseGridEditor);

#define LOCTEXT_NAMESPACE "NSSGModule"

// Statics
const FName FST_SparseGridEditorModule::HeatmapTabName = FName("HeatmapDebugger");

////////////////////////////
///// Module Interface /////
////////////////////////////

void FST_SparseGridEditorModule::StartupModule()
{
	FST_SparseGridStyle::Initialize();
	FST_SparseGridCommands::Register();

	// Register Editor Mode
	FEditorModeRegistry::Get().RegisterMode<FST_SparseGridEdMode>(
		FName(TEXT("SparseGridEditorMode")),
		NSLOCTEXT("SparseGridModule", "ModeName", "Sparse Grid"),
		FSlateIcon(FST_SparseGridStyle::GetStyleSetName(), FName(TEXT("SparseGrid.TabIcon")), FName(TEXT("SparseGrid.TabIcon.Small"))),
		true);

	// Register Heatmap debug tool
	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(HeatmapTabName, FOnSpawnTab::CreateStatic(&SST_SparseGridHeatmapTab::CreateTab))
		.SetDisplayName(LOCTEXT("HeatmapTool", "Sparse Grid Heatmap Tool"))
		.SetTooltipText(LOCTEXT("HeatmapToolTooltip", "Opens the sparse grid heatmap density tool."))
		.SetIcon(FSlateIcon(FST_SparseGridStyle::GetStyleSetName(), "SparseGrid.HeatmapIcon.Menu"))
		.SetGroup(MenuStructure.GetDeveloperToolsDebugCategory());

	// Register Sparse Grid Data Details Panel Customization
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(UST_SparseGridData::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FST_SparseGridDataDetails::MakeInstance));
}

void FST_SparseGridEditorModule::ShutdownModule()
{
	FEditorModeRegistry::Get().UnregisterMode(FName(TEXT("SparseGridEditorMode")));

	FST_SparseGridCommands::Unregister();
	FST_SparseGridStyle::Shutdown();

	// Unregister Heatmap Debug Tool
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(HeatmapTabName);
}

#undef LOCTEXT_NAMESPACE