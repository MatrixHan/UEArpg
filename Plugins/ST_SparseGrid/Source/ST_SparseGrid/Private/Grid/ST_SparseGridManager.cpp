// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridManager.h"
#include "ST_SparseGridData.h"

// Engine
#include "Engine/Engine.h"
#include "GameFramework/WorldSettings.h"

///////////////////////
///// Constructor /////
///////////////////////

UST_SparseGridManager::UST_SparseGridManager(const FObjectInitializer& OI)
	: Super(OI)
{
	bGridsInitialized = false;
	bAutoActivate = true;

	// Update all cells after Physics has run
	// Run as a high-priority tick, so queries can use most up-to-date data
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	PrimaryComponentTick.bHighPriority = true;
}

/////////////////////
///// Accessors /////
/////////////////////

UST_SparseGridManager* UST_SparseGridManager::Get(const UObject* WorldContextObject)
{
	const UWorld* lWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (lWorld && lWorld->PersistentLevel)
	{
		const AWorldSettings* lWorldSettings = lWorld->PersistentLevel->GetWorldSettings(false);
		if (lWorldSettings)
		{
			UST_SparseGridManager* lManager = lWorldSettings->FindComponentByClass<UST_SparseGridManager>();
			if (lManager)
			{
				return lManager;
			}
			else
			{
				UE_LOG(LogST_SparseGridManager, Verbose, TEXT("UST_SparseGridManager::Get() - Manager Invalid"));
			}
		}
		else
		{
			UE_LOG(LogST_SparseGridManager, Verbose, TEXT("UST_SparseGridManager::Get() - World Settings Invalid"));
		}
	}
	else
	{
		UE_LOG(LogST_SparseGridManager, Verbose, TEXT("UST_SparseGridManager::Get() - World Invalid"));
	}

	return nullptr;
}

UST_SparseGridManager* UST_SparseGridManager::GetChecked(const UObject* WorldContextObject)
{
	const UWorld* lWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	checkf(lWorld != nullptr, TEXT("UST_SparseGridManager::GetChecked() - World Invalid"));

	const AWorldSettings* lWorldSettings = lWorld->GetWorldSettings(false, false);
	checkf(lWorldSettings != nullptr, TEXT("UST_SparseGridManager::GetChecked() - World Settings Invalid"));

	UST_SparseGridManager* lManager = lWorldSettings->FindComponentByClass<UST_SparseGridManager>();
	checkf(lManager != nullptr, TEXT("UST_SparseGridManager::GetChecked() - Manager Invalid"));

	return lManager;
}

////////////////////
///// Creation /////
////////////////////

void UST_SparseGridManager::CreateGrid(UWorld* InWorld)
{
	if (InWorld && InWorld->IsGameWorld() && InWorld->PersistentLevel)
	{
		const UST_SparseGridData* LevelGridData = InWorld->PersistentLevel->GetAssetUserData<UST_SparseGridData>();
		if (LevelGridData)
		{
			AWorldSettings* lWorldSettings = InWorld->PersistentLevel->GetWorldSettings(false);
			if (lWorldSettings && !lWorldSettings->FindComponentByClass<UST_SparseGridManager>())
			{
				UST_SparseGridManager* NewManager = NewObject<UST_SparseGridManager>(lWorldSettings, LevelGridData->GetManagerClass(), NAME_None, RF_Transient);
				checkf(NewManager != nullptr, TEXT("Invalid Sparse Grid Manager"));
				
				NewManager->GridConfig = LevelGridData;
				NewManager->RegisterComponent();
			}
		}
	}
}

void UST_SparseGridManager::DestroyGrid(UWorld* InWorld)
{
	if (InWorld && InWorld->IsGameWorld() && InWorld->PersistentLevel)
	{
		const AWorldSettings* lWorldSettings = InWorld->PersistentLevel->GetWorldSettings(false);
		if (lWorldSettings)
		{
			UST_SparseGridManager* Manager = lWorldSettings->FindComponentByClass<UST_SparseGridManager>();
			if (Manager)
			{
				Manager->GridConfig = nullptr;
				Manager->DestroyComponent();
			}
		}
	}
}

////////////////////////
///// Registration /////
////////////////////////

void UST_SparseGridManager::OnRegister()
{
	Super::OnRegister();

	const UWorld* lWorld = GetWorld();
	if (lWorld && lWorld->IsGameWorld())
	{
		const AWorldSettings* WSOwner = Cast<AWorldSettings>(GetOwner());
		if (!WSOwner)
		{
			UE_LOG(LogST_SparseGridManager, Fatal, TEXT("Sparse Grid Manager Must Be Owned By World Settings!"));
			return;
		}

		// Build the Grid
		InitializeGrids();
	}
}

void UST_SparseGridManager::OnUnregister()
{
	UninitializeGrids();

	Super::OnUnregister();
}

bool UST_SparseGridManager::ShouldActivate() const
{
	const AWorldSettings* WSOwner = Cast<AWorldSettings>(GetOwner());
	if (!WSOwner || !WSOwner->IsInPersistentLevel())
	{
		return false;
	}

	return Super::ShouldActivate();
}

/////////////////////////
///// Grid Updating /////
/////////////////////////

void UST_SparseGridManager::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (AreGridsInitialized())
	{
		UpdateGrids();
	}
}

void UST_SparseGridManager::SetComponentTickEnabled(bool bEnabled)
{
	// Ensure we can't tick if we're in a streaming level
	const AWorldSettings* WSOwner = Cast<AWorldSettings>(GetOwner());
	checkf(WSOwner != nullptr, TEXT("Invalid Owner"));

	bEnabled &= WSOwner->GetWorld() && WSOwner->IsInPersistentLevel();
	Super::SetComponentTickEnabled(bEnabled);
}

////////////////////////////
///// Grid Interaction /////
////////////////////////////

void UST_SparseGridManager::InitializeGrids()
{
	const AWorldSettings* WSOwner = Cast<AWorldSettings>(GetOwner());
	if (!AreGridsInitialized()
		&& WSOwner
		&& WSOwner->GetWorld()
		&& WSOwner->IsInPersistentLevel()
		&& GridConfig.IsValid())
	{
		if (GridConfig->GetCellSize() > 0 && GridConfig->GetNumCellsX() > 0 && GridConfig->GetNumCellsY() > 0)
		{
			CreateGrids();
			bGridsInitialized = true;

			// Enable Ticking
			if (!IsComponentTickEnabled())
			{
				SetComponentTickEnabled(true);
			}
		}
	}
}

void UST_SparseGridManager::UninitializeGrids()
{
	if (AreGridsInitialized())
	{
		if (IsComponentTickEnabled())
		{
			SetComponentTickEnabled(false);
		}

		DestroyGrids();
	}
}