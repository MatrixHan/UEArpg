// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/*
* Sparse Grid Editor Actions
*/
class FST_SparseGridCommands : public TCommands<FST_SparseGridCommands>
{
public:
	// Constructor
	FST_SparseGridCommands();

	// TCommands Interface
	virtual void RegisterCommands() override;

	// Tools
	TSharedPtr<FUICommandInfo> Cmd_FitToWorld;
	TSharedPtr<FUICommandInfo> Cmd_ShowHeatmap;

	// Utility
	TSharedPtr<FUICommandInfo> Cmd_VisitWebsite;
	TSharedPtr<FUICommandInfo> Cmd_Documentation;
	TSharedPtr<FUICommandInfo> Cmd_SupportThread;
};