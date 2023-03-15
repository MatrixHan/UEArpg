// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridComponent.h"
#include "ST_SparseGrid.h"
#include "ST_SparseGridManager_Basic.h"

// Extras
#include "GameFramework/Actor.h"

///////////////////////
///// Constructor /////
///////////////////////

UST_SparseGridComponent::UST_SparseGridComponent(const FObjectInitializer& OI)
	: Super(OI)
	, SparseGridData(FST_SparseGridData())
{
	// Disable Ticking
	PrimaryComponentTick.bCanEverTick = false;
}

///////////////////////////
///// Grid Management /////
///////////////////////////

FVector UST_SparseGridComponent::GetSparseGridLocation() const
{
	const AActor* lOwner = GetOwner();
	checkf(lOwner != nullptr, TEXT("GetSparseGridLocation() - Invalid Owner!"));

	return lOwner->GetActorLocation();
}

////////////////////////
///// Registration /////
////////////////////////

void UST_SparseGridComponent::RegisterWithSparseGrid()
{
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		UST_SparseGridManager_Basic* BasicManager = Cast<UST_SparseGridManager_Basic>(UST_SparseGridManager::Get(this));
		if (BasicManager && BasicManager->AreGridsInitialized())
		{
			BasicManager->GetSparseGrid_Basic()->Add(this);
		}
	}
}

void UST_SparseGridComponent::UnRegisterWithSparseGrid()
{
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		UST_SparseGridManager_Basic* BasicManager = Cast<UST_SparseGridManager_Basic>(UST_SparseGridManager::Get(this));
		if (BasicManager && BasicManager->AreGridsInitialized())
		{
			BasicManager->GetSparseGrid_Basic()->Remove(this);
		}
	}
}

/////////////////////
///// Overrides /////
/////////////////////

void UST_SparseGridComponent::OnRegister()
{
	Super::OnRegister();

	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		RegisterWithSparseGrid();
	}
}

void UST_SparseGridComponent::OnUnregister()
{
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		UnRegisterWithSparseGrid();
	}

	Super::OnUnregister();
}

///////////////////////
///// Conversions /////
///////////////////////

void UST_SparseGridComponent::GetComponentsOwners(const TArray<UST_SparseGridComponent*>& GridComponents, TArray<AActor*>& Actors)
{
	// Single Allocation
	Actors.Reset(GridComponents.Num());

	for (const UST_SparseGridComponent* ComponentItr : GridComponents)
	{
		Actors.Add(ComponentItr->GetOwner());
	}
}

void UST_SparseGridComponent::GetComponentsOwners_Typed(const TArray<UST_SparseGridComponent*>& GridComponents, TArray<AActor*>& Actors, const TSubclassOf<AActor> ActorClass)
{
	// Even though the allocation will be larger than required, this is most likely faster
	Actors.Reset(GridComponents.Num());

	for (const UST_SparseGridComponent* ComponentItr : GridComponents)
	{
		AActor* Owner = ComponentItr->GetOwner();
		if (!Owner->GetClass()->IsChildOf(ActorClass))
		{
			continue;
		}

		Actors.Add(Owner);
	}
}