// Copyright (C) Stormtide Ltd. 2018. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Logging/LogMacros.h"

class FST_SparseGridEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static const FName HeatmapTabName;	
};

DECLARE_LOG_CATEGORY_EXTERN(LogST_SparseGridEditor, Log, All);