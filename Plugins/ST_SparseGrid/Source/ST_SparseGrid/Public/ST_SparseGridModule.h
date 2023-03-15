// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Delegates/IDelegateInstance.h"
#include "Engine/World.h"

class FST_SparseGridModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	static void OnWorldInitialized(UWorld* InWorld, const UWorld::InitializationValues InValues);
	static void OnPostDuplicate(UWorld* InWorld, bool bDuplicateForPIE, TMap<UObject*, UObject*>& ReplacementMap, TArray<UObject*>& ObjectsToFixReferences);
	static void OnWorldCleanup(UWorld* InWorld, bool bSessionEnded, bool bCleanupResources);

	FDelegateHandle OnWorldInitializedDelegateHandle;
	FDelegateHandle OnWorldDuplicatedDelegateHandle;
	FDelegateHandle OnWorldCleanupDelegateHandle;
};