// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridCommands.h"
#include "Editor/EditorStyle/Public/EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "NSSparseGridCommands"

///////////////////////
///// Constructor /////
///////////////////////

FST_SparseGridCommands::FST_SparseGridCommands()
	: TCommands<FST_SparseGridCommands>
	(
		"SparseGridEditor",
		LOCTEXT("SparseGridEditor", "Sparse Grid Editor"),
		NAME_None,
		FEditorStyle::GetStyleSetName()
	)
{}

////////////////////
///// Commands /////
////////////////////

void FST_SparseGridCommands::RegisterCommands()
{
	UI_COMMAND(Cmd_FitToWorld, "Fit To World", "", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(Cmd_ShowHeatmap, "Show Heatmap", "", EUserInterfaceActionType::Button, FInputChord());

	// Utilities
	UI_COMMAND(Cmd_SupportThread, "Support Thread", "", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(Cmd_Documentation, "Open Documentation", "", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(Cmd_VisitWebsite, "Visit Website", "", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE