// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridModule.h"
#include "ST_SparseGridManager.h"

// Extras
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FST_SparseGridModule, ST_SparseGrid)

void FST_SparseGridModule::StartupModule()
{
	OnWorldInitializedDelegateHandle = FWorldDelegates::OnPostWorldInitialization.AddStatic(FST_SparseGridModule::OnWorldInitialized);
	OnWorldDuplicatedDelegateHandle = FWorldDelegates::OnPostDuplicate.AddStatic(FST_SparseGridModule::OnPostDuplicate);
	OnWorldCleanupDelegateHandle = FWorldDelegates::OnWorldCleanup.AddStatic(FST_SparseGridModule::OnWorldCleanup);
}

void FST_SparseGridModule::ShutdownModule()
{
	FWorldDelegates::OnPostWorldInitialization.Remove(OnWorldInitializedDelegateHandle);
	FWorldDelegates::OnPostDuplicate.Remove(OnWorldDuplicatedDelegateHandle);
	FWorldDelegates::OnWorldCleanup.Remove(OnWorldCleanupDelegateHandle);
}

///////////////////
///// Testing /////
///////////////////

void FST_SparseGridModule::OnWorldInitialized(UWorld* InWorld, const UWorld::InitializationValues InValues)
{
	UST_SparseGridManager::CreateGrid(InWorld);
}

void FST_SparseGridModule::OnPostDuplicate(UWorld* InWorld, bool bDuplicateForPIE, TMap<UObject*, UObject*>& ReplacementMap, TArray<UObject*>& ObjectsToFixReferences)
{
	UST_SparseGridManager::CreateGrid(InWorld);
}

void FST_SparseGridModule::OnWorldCleanup(UWorld* InWorld, bool bSessionEnded, bool bCleanupResources)
{
	UST_SparseGridManager::DestroyGrid(InWorld);
}