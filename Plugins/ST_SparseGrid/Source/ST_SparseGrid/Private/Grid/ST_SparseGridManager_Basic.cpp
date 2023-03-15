// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridManager_Basic.h"
#include "ST_SparseGrid.h"
#include "ST_SparseGridData.h"
#include "ST_SparseGridComponent.h"

#if WITH_EDITOR
const FName UST_SparseGridManager_Basic::GRIDNAME_Basic = FName("Default");
#endif

///////////////////////
///// Constructor /////
///////////////////////

UST_SparseGridManager_Basic::UST_SparseGridManager_Basic(const FObjectInitializer& OI)
	: Super(OI)
{}

///////////////////////////////
///// Grid Initialization /////
///////////////////////////////

void UST_SparseGridManager_Basic::CreateGrids()
{
	const UST_SparseGridData* BasicData = GetGridConfig();

	SparseGridData_Basic = MakeShareable(new TST_SparseGrid<UST_SparseGridComponent>(
		GetWorld(),
		BasicData->GetGridOrigin(),
		FST_GridRef2D(BasicData->GetNumCellsX(), BasicData->GetNumCellsY()),
		BasicData->GetCellSize(),
		BasicData->GetRegisterAllocSize(),
		BasicData->GetRegisterAllocShrinkMultiplier(),
		BasicData->GetCellAllocSize(),
		BasicData->GetCellAllocShrinkMultiplier()));

	SparseGridData_Basic->Init(false);
}

void UST_SparseGridManager_Basic::DestroyGrids()
{
	SparseGridData_Basic.Reset();
}

void UST_SparseGridManager_Basic::UpdateGrids()
{
	GetSparseGrid_Basic()->Update();

#if SPARSE_GRID_DEBUG
	if (ST_SparseGridCVars::CVarDrawDebug.GetValueOnGameThread())
	{
		GetSparseGrid_Basic()->DrawDebugGrid();
	}
#endif
}

//////////////////
///// Editor /////
//////////////////

#if WITH_EDITOR
bool UST_SparseGridManager_Basic::GetGridNames(TArray<FName>& OutGridNames) const
{
	OutGridNames.Reset(1);
	OutGridNames.Add(GRIDNAME_Basic);

	return true;
}

bool UST_SparseGridManager_Basic::GetGridPopulationData(const FName InGridName, TArray<uint32>& OutData) const
{
	if (ensure(InGridName == GRIDNAME_Basic))
	{
		GetSparseGrid_Basic()->GetGridCellPopulations(OutData);
		return true;
	}

	return false;
}

bool UST_SparseGridManager_Basic::GetGridMemoryInfo(const FName InGridName, int32& OutTotalObjects, uint64& OutRegisterAllocSize, uint64& OutRegisterUsedSize, uint64& OutCellAllocSize, uint64& OutCellUsedSize) const
{
	if (ensure(InGridName == GRIDNAME_Basic))
	{
		GetSparseGrid_Basic()->GetEditorDebugInfo(OutTotalObjects, OutRegisterAllocSize, OutRegisterUsedSize, OutCellAllocSize, OutCellUsedSize);
		return true;
	}

	return false;
}
#endif

////////////////////////////
///// Blueprint Access /////
////////////////////////////

const TArray<UST_SparseGridComponent*>& UST_SparseGridManager_Basic::GetGridComponents() const
{
	check(AreGridsInitialized());
	return GetSparseGrid_Basic()->GetRegisteredObjects();
}

FST_GridRef2D UST_SparseGridManager_Basic::K2_GetGridMax() const
{
	return AreGridsInitialized() ? GetSparseGrid_Basic()->GetGridMax() : FST_GridRef2D(INDEX_NONE);
}

int32 UST_SparseGridManager_Basic::K2_WorldToCell(FVector2D InWorldXY) const
{
	return AreGridsInitialized() ? GetSparseGrid_Basic()->WorldToCell(InWorldXY) : INDEX_NONE;
}

FST_SparseGridCellTile UST_SparseGridManager_Basic::GetSearchTile(const FVector2D& WorldSearchOrigin, const FVector2D& WorldSearchExtents) const
{
	return AreGridsInitialized() ? GetSparseGrid_Basic()->GetSearchTile(WorldSearchOrigin, WorldSearchExtents) : FST_SparseGridCellTile();
}

void UST_SparseGridManager_Basic::K2_GetCellComponents(const int32 CellID, TArray<UST_SparseGridComponent*>& Components) const
{
	Components.Reset();
	if (GetSparseGrid_Basic()->GetGridCells().IsValidIndex(CellID))
	{
		Components = GetSparseGrid_Basic()->GetGridCells()[CellID].GetObjects();
	}
}

void UST_SparseGridManager_Basic::K2_GetTileComponents(const FST_SparseGridCellTile& Tile, TArray<UST_SparseGridComponent*>& Components) const
{
	Components.Reset();
	
	for (int32 CIdx = Tile.Start.Y; CIdx < Tile.End.Y; CIdx++)
	{
		for (int32 RIdx = Tile.Start.X; RIdx < Tile.End.X; RIdx++)
		{
			const FST_GridRef2D CellXY = FST_GridRef2D(RIdx, CIdx);
			const int32 CellIndex = GetSparseGrid_Basic()->GetCellIndex(CellXY);
			if (GetSparseGrid_Basic()->GetGridCells().IsValidIndex(CellIndex))
			{
				Components.Append(GetSparseGrid_Basic()->GetGridCells()[CellIndex].GetObjects());
			}
		}
	}
}

float UST_SparseGridManager_Basic::K2_GetCellBoundsRadius() const
{
	return AreGridsInitialized() ? GetSparseGrid_Basic()->GetCellBoundsRadius() : -1.f;
}

FST_GridRef2D UST_SparseGridManager_Basic::K2_ConvertToGridRef2D(const int32 CellID) const
{
	return AreGridsInitialized() ? GetSparseGrid_Basic()->GetCellXY(CellID) : FST_GridRef2D(INDEX_NONE);
}

int32 UST_SparseGridManager_Basic::K2_ConvertToCellID(const FST_GridRef2D& XY) const
{
	return AreGridsInitialized() ? GetSparseGrid_Basic()->GetCellIndex(XY) : INDEX_NONE;
}

//////////////////////////
///// Search Queries /////
//////////////////////////

bool UST_SparseGridManager_Basic::K2_GetComponents_Sphere(const UObject* WorldContextObject, TArray<UST_SparseGridComponent*>& GridComponents, const FVector& InWorldLocation, const float InSphereRadius, const bool bDrawDebug /*= false*/)
{
	GridComponents.Reset();

	const UST_SparseGridManager_Basic* BasicManager = Cast<UST_SparseGridManager_Basic>(UST_SparseGridManager::Get(WorldContextObject));
	if (BasicManager && BasicManager->AreGridsInitialized())
	{
		BasicManager->GetSparseGrid_Basic()->QueryGrid_Sphere(GridComponents, InWorldLocation, InSphereRadius, bDrawDebug);
		return true;
	}

	return false;
}

bool UST_SparseGridManager_Basic::K2_GetComponents_Capsule(const UObject* WorldContextObject, TArray<UST_SparseGridComponent*>& GridComponents, const FVector& WorldLocation, const FVector& CapsuleAxis, const float CapsuleRadius, const float CapsuleHalfHeight, const bool bDrawDebug /*= false*/)
{
	GridComponents.Reset();

	const UST_SparseGridManager_Basic* BasicManager = Cast<UST_SparseGridManager_Basic>(UST_SparseGridManager::Get(WorldContextObject));
	if (BasicManager && BasicManager->AreGridsInitialized())
	{
		BasicManager->GetSparseGrid_Basic()->QueryGrid_Capsule(GridComponents, WorldLocation, CapsuleAxis, CapsuleRadius, CapsuleHalfHeight, bDrawDebug);
		return true;
	}

	return false;
}

bool UST_SparseGridManager_Basic::K2_GetComponents_Box(const UObject* WorldContextObject, TArray<UST_SparseGridComponent*>& GridComponents, const FVector& InWorldLocation, const FVector& InBoxExtents, const bool bDrawDebug /*= false*/)
{
	GridComponents.Reset();

	const UST_SparseGridManager_Basic* BasicManager = Cast<UST_SparseGridManager_Basic>(UST_SparseGridManager::Get(WorldContextObject));
	if (BasicManager && BasicManager->AreGridsInitialized())
	{
		BasicManager->GetSparseGrid_Basic()->QueryGrid_Box(GridComponents, InWorldLocation, InBoxExtents, bDrawDebug);
		return true;
	}

	return false;
}

bool UST_SparseGridManager_Basic::K2_GetComponents_RotatedBox(const UObject* WorldContextObject, TArray<UST_SparseGridComponent*>& GridComponents, const FVector& InWorldLocation, const FRotator& InBoxRotation, const FVector& InBoxExtents, const bool bDrawDebug /*= false*/)
{
	GridComponents.Reset();

	const UST_SparseGridManager_Basic* BasicManager = Cast<UST_SparseGridManager_Basic>(UST_SparseGridManager::Get(WorldContextObject));
	if (BasicManager && BasicManager->AreGridsInitialized())
	{
		BasicManager->GetSparseGrid_Basic()->QueryGrid_RotatedBox(GridComponents, InWorldLocation, InBoxRotation.Quaternion(), InBoxExtents, bDrawDebug);
		return true;
	}

	return false;
}

bool UST_SparseGridManager_Basic::K2_GetComponents_Cone(const UObject* WorldContextObject, TArray<UST_SparseGridComponent*>& GridComponents, const FVector& InWorldLocation, const float InConeLength, const float InConeHalfAngleRadians, const FVector& InAxis, const bool bDrawDebug /*= false*/)
{
	GridComponents.Reset();

	const UST_SparseGridManager_Basic* BasicManager = Cast<UST_SparseGridManager_Basic>(UST_SparseGridManager::Get(WorldContextObject));
	if (BasicManager && BasicManager->AreGridsInitialized())
	{
		BasicManager->GetSparseGrid_Basic()->QueryGrid_Cone(GridComponents, InWorldLocation, InConeLength, InConeHalfAngleRadians, InAxis, bDrawDebug);
		return true;
	}

	return false;
}

//////////////////////////////////
///// Example Search Queries /////
//////////////////////////////////

// void UST_SparseGrid::GetComponents_Sphere(TArray<UST_SGComponent*>& OutComponents, const FVector& InWorldLocation, const float InSphereRadius, const bool bDrawDebug /*= false*/) const
// {
// 	SCOPE_CYCLE_COUNTER(STAT_QueryGrid_Sphere)
// 
// 	OutComponents.Reset();
// 
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 	const UWorld* DebugWorld = GetWorld();
// 	if (bDrawDebug) { DrawDebugSphere(DebugWorld, InWorldLocation, InSphereRadius, 12, FColor::Green, false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness); }
// #endif
// 
// #if USE_FAST_BOUNDS_SEARCH_REJECTION
// 	// Skip if the search is way outside of object bounds
// 	if (Grid.GetObjectBounds().CanFastReject(InWorldLocation, FVector(InSphereRadius, InSphereRadius, InSphereRadius))) { return; }		
// #endif
// 
// 	// Find Tile Indices
// 	const FVector2D TileBoundsXY = FVector2D(InWorldLocation.X, InWorldLocation.Y);
// 	const FST_SparseGridCellTile Tile = Grid.GetSearchTile(TileBoundsXY, FVector2D(InSphereRadius, InSphereRadius));
// 
// 	// Determine Cell Culling Properties
// 	const FST_GridRef2D GridMax = GetGridMax();
// 	const FVector2D XYClamped = FVector2D(FMath::Clamp<float>(TileBoundsXY.X, GridOrigin.X, GridMax.X), FMath::Clamp<float>(TileBoundsXY.Y, GridOrigin.Y, GridMax.Y));
// 
// 	// Determine Object Culling Properties
// 	const double DistSqrd = InSphereRadius * InSphereRadius;
// 
// 	for (int32 CIdx = Tile.Start.Y; CIdx < Tile.End.Y; CIdx++)
// 	{
// 		for (int32 RIdx = Tile.Start.X; RIdx < Tile.End.X; RIdx++)
// 		{
// 			const FST_GridRef2D CellXY = FST_GridRef2D(RIdx, CIdx);
// 			if (!Grid.CullCell_Range(CellXY, XYClamped, InSphereRadius))
// 			{
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 				if (bDrawDebug) { DrawHighlightedTile(CellXY, FLinearColor(0.f, 1.f, 0.f, 0.25f)); }
// #endif
// 				const int32 CellIndex = Grid.GetCellIndex(CellXY);
// 				for (UST_SGComponent* ComponentItr : Grid.GetGridCells()[CellIndex].GetObjects())
// 				{
// 					const FVector ObjectLoc = ComponentItr->GetSparseGridLocation();
// 					if (FVector::DistSquared(InWorldLocation, ObjectLoc) <= DistSqrd)
// 					{
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 						if (bDrawDebug) { DrawDebugLine(DebugWorld, InWorldLocation, ObjectLoc, FColor::Green, false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness); }
// #endif
// 						OutComponents.Add(ComponentItr);
// 					}
// 				}
// 			}
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 			else if (bDrawDebug) { DrawHighlightedTile(CIdx, RIdx, FLinearColor(1.f, 0.f, 0.f, 0.25f)); }
// #endif
// 		}
// 	}
// }
// 
// void UST_SparseGrid::GetComponents_Capsule(TArray<UST_SGComponent*>& OutComponents, const FVector& InWorldLocation, const FVector& InUpAxis, float InCapsuleRadius, float InCapsuleHalfHeight, const bool bDrawDebug /*= false*/) const
// {
// 	SCOPE_CYCLE_COUNTER(STAT_QueryGrid_Capsule)
// 
// 	OutComponents.Reset();
// 
// 	// Clamp to sensible values
// 	InCapsuleHalfHeight = FMath::Max3(0.f, InCapsuleHalfHeight, InCapsuleRadius);
// 	InCapsuleRadius = FMath::Clamp(InCapsuleRadius, 0.f, InCapsuleHalfHeight);
// 
// 	const FMatrix CapsuleToWorld = FTransform(FRotationMatrix::MakeFromZ(InUpAxis).ToQuat(), InWorldLocation).ToMatrixNoScale();
// 	const FVector CapsuleBoundsExtents = FVector(InCapsuleRadius, InCapsuleRadius, InCapsuleHalfHeight);
// 	const FBoxSphereBounds AABB = FBoxSphereBounds(FBox(-CapsuleBoundsExtents, CapsuleBoundsExtents)).TransformBy(CapsuleToWorld);
// 
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 	const UWorld* DebugWorld = GetWorld();
// 	if (bDrawDebug)
// 	{
// 		DrawDebugBox(DebugWorld, InWorldLocation, CapsuleBoundsExtents, CapsuleToWorld.ToQuat(), FColor::Orange, false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness);;
// 		DrawDebugCapsule(DebugWorld, InWorldLocation, InCapsuleHalfHeight, InCapsuleRadius, FRotationMatrix::MakeFromZ(InUpAxis).ToQuat(), FColor::Green, false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness);
// 	}
// #endif
// 
// #if USE_FAST_BOUNDS_SEARCH_REJECTION
// 	// Skip if the search is way outside of object bounds
// 	if (FrameBounds.CanFastReject(InWorldLocation, AABB.BoxExtent)) { return; }
// #endif
// 
// 	// Find Tile Indices for Shape AABB
// 	const FVector2D TileBoundsXY = FVector2D(InWorldLocation.X, InWorldLocation.Y);
// 	const FST_SparseGridCellTile Tile = GetSearchTile(TileBoundsXY, FVector2D(AABB.BoxExtent.X, AABB.BoxExtent.Y));
// 
// 	// Determine Object Culling Properties
// 	const FVector Dir = InUpAxis * (InCapsuleHalfHeight - InCapsuleRadius);
// 	const FVector CapsuleStart = InWorldLocation + Dir;
// 	const FVector CapsuleEnd = InWorldLocation - Dir;
// 	const float RSq = InCapsuleRadius * InCapsuleRadius;
// 
// 	// Determine Cell Culling Properties
// 	const FST_GridRef2D GridMax = GetGridMax();
// 	const FVector2D XYClamped = FVector2D(FMath::Clamp<float>(TileBoundsXY.X, GridOrigin.X, GridMax.X), FMath::Clamp<float>(TileBoundsXY.Y, GridOrigin.Y, GridMax.Y));
// 	const FVector2D CullStart = XYClamped + FVector2D(Dir);
// 	const FVector2D CullEnd = XYClamped - FVector2D(Dir);
// 
// 	for (int32 CIdx = Tile.Start.Y; CIdx < Tile.End.Y; CIdx++)
// 	{
// 		for (int32 RIdx = Tile.Start.X; RIdx < Tile.End.X; RIdx++)
// 		{
// 			if (!CullCell_Line(CIdx, RIdx, CullStart, CullEnd, InCapsuleRadius))
// 			{
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 				if (bDrawDebug) { DrawHighlightedTile(CIdx, RIdx, FLinearColor(0.f, 1.f, 0.f, 0.25f)); }
// #endif
// 				const int32 CellIndex = RIdx + CIdx * GetNumCellsX();
// 				for (UST_SGComponent* ComponentItr : GetGridCells()[CellIndex].GetCellComponents())
// 				{
// 					const FVector Location = ComponentItr->GetSparseGridLocation();
// 					const FVector ClosestPoint = FMath::ClosestPointOnSegment(Location, CapsuleStart, CapsuleEnd);
// 
// 					if (FVector::DistSquared(ClosestPoint, Location) <= RSq)
// 					{
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 						if (bDrawDebug) { DrawDebugLine(DebugWorld, ClosestPoint, Location, FLinearColor(0.f, 1.f, 0.f, 0.25f).ToFColor(false), false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness); }
// #endif
// 						OutComponents.Add(ComponentItr);
// 					}
// 				}
// 			}
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 			else if (bDrawDebug) { DrawHighlightedTile(CIdx, RIdx, FLinearColor(1.f, 0.f, 0.f, 0.25f)); }
// #endif
// 		}
//  	}
// }
// 
// void UST_SparseGrid::GetComponents_Box(TArray<UST_SGComponent*>& OutComponents, const FVector& InWorldLocation, const FVector& InBoxExtents, const bool bDrawDebug /*= false*/) const
// {
// 	SCOPE_CYCLE_COUNTER(STAT_QueryGrid_Box)
// 
// 	OutComponents.Reset();
// 
// 	const FVector ExtentMin = InWorldLocation - InBoxExtents;
// 	const FVector ExtentMax = InWorldLocation + InBoxExtents;
// 
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 	if (bDrawDebug) { DrawDebugBox(GetWorld(), InWorldLocation, InBoxExtents, FColor::Blue, false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness); }
// #endif
// 
// #if USE_FAST_BOUNDS_SEARCH_REJECTION
// 	// Skip if the search is way outside of object bounds
// 	if (FrameBounds.CanFastReject(InWorldLocation, InBoxExtents)) { return; }
// #endif
// 
// 	// Axis-Aligned Tiles
// 	const FVector2D TileBoundsXY = FVector2D(InBoxExtents.X, InBoxExtents.Y);
// 	const FST_SparseGridCellTile Tile = GetSearchTile(FVector2D(InWorldLocation), FVector2D(InBoxExtents));
// 
// 	// Iterate All Tiles
// 	for (int32 CIdx = Tile.Start.Y; CIdx < Tile.End.Y; CIdx++)
// 	{
// 		for (int32 RIdx = Tile.Start.X; RIdx < Tile.End.X; RIdx++)
// 		{
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 			if (bDrawDebug) { DrawHighlightedTile(CIdx, RIdx, FLinearColor(0.05f, 0.5f, 1.f, 1.f)); }
// #endif
// 			const int32 CellIndex = RIdx + CIdx * GetNumCellsX();
// 			for (UST_SGComponent* ComponentItr : GetGridCells()[CellIndex].GetCellComponents())
// 			{
// 				const FVector Location = ComponentItr->GetSparseGridLocation();
// 				if (Location.X < ExtentMin.X || Location.X > ExtentMax.X || Location.Y < ExtentMin.Y || Location.Y > ExtentMax.Y || Location.Z < ExtentMin.Z || Location.Z > ExtentMax.Z)
// 				{
// 					OutComponents.Add(ComponentItr);
// 				}
// 			}
// 		}
// 	}
// }
// 
// void UST_SparseGrid::GetComponents_RotatedBox(TArray<UST_SGComponent*>& OutComponents, const FVector& InWorldLocation, const FRotator& InBoxRotation, const FVector& InBoxExtents, const bool bDrawDebug /*= false*/) const
// {
// 	SCOPE_CYCLE_COUNTER(STAT_QueryGrid_RotatedBox)
// 
// 	OutComponents.Reset();
// 
// 	// Box Transform and AABB
// 	const FMatrix BoxToWorld = FTransform(InBoxRotation, InWorldLocation).ToMatrixNoScale();
// 	const FBoxSphereBounds AABB = FBoxSphereBounds(FBox(-InBoxExtents, InBoxExtents)).TransformBy(BoxToWorld);
// 
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 	const UWorld* DebugWorld = GetWorld();
// 	if (bDrawDebug)
// 	{
// 		DrawDebugBox(DebugWorld, InWorldLocation, InBoxExtents, InBoxRotation.Quaternion(), FColor::Green, false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness);
// 		DrawDebugBox(DebugWorld, AABB.Origin, AABB.BoxExtent, FQuat::Identity, FColor::Red, false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness);
// 	}
// #endif
// 
// #if USE_FAST_BOUNDS_SEARCH_REJECTION
// 	// Skip if the search is way outside of object bounds
// 	if (FrameBounds.CanFastReject(InWorldLocation, AABB.BoxExtent)) { return; }
// #endif
// 
// 	const FVector2D TileBoundsXY = FVector2D(InWorldLocation.X, InWorldLocation.Y);
// 	const FST_SparseGridCellTile Tile = GetSearchTile(TileBoundsXY, FVector2D(AABB.BoxExtent.X, AABB.BoxExtent.Y));
// 
// 	for (int32 CIdx = Tile.Start.Y; CIdx < Tile.End.Y; CIdx++)
// 	{
// 		for (int32 RIdx = Tile.Start.X; RIdx < Tile.End.X; RIdx++)
// 		{
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 			if (bDrawDebug) { DrawHighlightedTile(CIdx, RIdx, FLinearColor(1.f, 0.f, 0.f, 0.25f)); }
// #endif
// 			const int32 CellIndex = RIdx + CIdx * GetNumCellsX();
// 			for (UST_SGComponent* ComponentItr : GetGridCells()[CellIndex].GetCellComponents())
// 			{
// 				const FVector TransformedLocation = BoxToWorld.InverseTransformPosition(ComponentItr->GetSparseGridLocation());
// 				if (TransformedLocation.X < -InBoxExtents.X || TransformedLocation.X > InBoxExtents.X
// 					|| TransformedLocation.Y < -InBoxExtents.Y || TransformedLocation.Y > InBoxExtents.Y
// 					|| TransformedLocation.Z < -InBoxExtents.Z || TransformedLocation.Z > InBoxExtents.Z)
// 				{
// 					continue;
// 				}
// 
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 				if (bDrawDebug) { DrawDebugLine(DebugWorld, InWorldLocation, ComponentItr->GetSparseGridLocation(), FColor::Green, false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness); }
// #endif
// 				OutComponents.Add(ComponentItr);
// 			}
// 		}
// 	}
// }
// 
// void UST_SparseGrid::GetComponents_Cone(TArray<UST_SGComponent*>& OutComponents, const FVector& InWorldLocation, const float InConeLength, const float InConeHalfAngleRadians, const FVector& InAxis, const bool bDrawDebug /*= false*/) const
// {
// 	SCOPE_CYCLE_COUNTER(STAT_QueryGrid_Cone)
// 
// 	OutComponents.Reset();
// 
// 	// Cone Transform and AABB
// 	const FVector ConeCenter = InWorldLocation + InAxis * (InConeLength * 0.5f);
// 	const float ConeEndRadius = InConeLength * FMath::Tan(InConeHalfAngleRadians);
// 	const FMatrix ConeToWorld = FTransform(FRotationMatrix::MakeFromX(InAxis).ToQuat(), ConeCenter).ToMatrixNoScale();
// 	const FVector ConeBoundsExtents = FVector(InConeLength * 0.5f, ConeEndRadius, ConeEndRadius);
// 	const FBoxSphereBounds AABB = FBoxSphereBounds(FBox(-ConeBoundsExtents, ConeBoundsExtents)).TransformBy(ConeToWorld);
// 
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 	const UWorld* DebugWorld = GetWorld();
// 	if (bDrawDebug)
// 	{
// 		DrawDebugBox(DebugWorld, ConeCenter, ConeBoundsExtents, ConeToWorld.ToQuat(), FColor::Orange, false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness);
// 		DrawDebugCone(DebugWorld, InWorldLocation, InAxis, InConeLength, InConeHalfAngleRadians, InConeHalfAngleRadians, 16, FColor::Green, false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness);
// 		DrawDebugBox(DebugWorld, AABB.Origin, AABB.BoxExtent, FQuat::Identity, FColor::Red, false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness);
// 	}
// #endif
// 
// #if USE_FAST_BOUNDS_SEARCH_REJECTION
// 	// Skip if the search is way outside of object bounds
// 	if (FrameBounds.CanFastReject(InWorldLocation, AABB.BoxExtent)) { return; }
// #endif
// 
// 	// Find Tile Indices For Shape AABB
// 	const FVector2D TileBoundsXY = FVector2D(ConeCenter.X, ConeCenter.Y);
// 	const FST_SparseGridCellTile Tile = GetSearchTile(TileBoundsXY, FVector2D(AABB.BoxExtent.X, AABB.BoxExtent.Y));
// 
// 	// Determine Cell Culling Properties
// 	const FST_GridRef2D GridMax = GetGridMax();
// 	const FVector2D XYClamped = FVector2D(FMath::Clamp<float>(TileBoundsXY.X, GridOrigin.X, GridMax.X), FMath::Clamp<float>(TileBoundsXY.Y, GridOrigin.Y, GridMax.Y));
// 	const FVector2D ConeStart2D = XYClamped;
// 	const FVector2D ConeEnd2D = FVector2D(XYClamped + (FVector2D(InAxis) * InConeLength));
// 
// 	// Determine Object Culling Properties
// 	const float ConeLenSq = InConeLength * InConeLength;
// 	const float ConeCos = FMath::Cos(InConeHalfAngleRadians);
// 
// 	for (int32 CIdx = Tile.Start.Y; CIdx < Tile.End.Y; CIdx++)
// 	{
// 		for (int32 RIdx = Tile.Start.X; RIdx < Tile.End.X; RIdx++)
// 		{
// 			if (!CullCell_Line(CIdx, RIdx, ConeStart2D, ConeEnd2D, ConeEndRadius))
// 			{
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 				if (bDrawDebug) { DrawHighlightedTile(CIdx, RIdx, FLinearColor(0.f, 1.f, 0.f, 0.25f)); }
// #endif
// 				const int32 CellIndex = RIdx + CIdx * GetNumCellsX();
// 				for (UST_SGComponent* ComponentItr : GetGridCells()[CellIndex].GetCellComponents())
// 				{
// 					const FVector OwnerLocation = ComponentItr->GetSparseGridLocation();
// 					if (FVector::DistSquared(InWorldLocation, OwnerLocation) > ConeLenSq)
// 					{
// 						continue;
// 					}
// 
// 					// TODO: Remove Square Root?
// 					if (FVector::DotProduct(InAxis, FVector(OwnerLocation - InWorldLocation).GetSafeNormal()) < ConeCos)
// 					{
// 						continue;
// 					}
// 
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 					if (bDrawDebug) { DrawDebugLine(DebugWorld, InWorldLocation, OwnerLocation, FColor::Green, false, ST_SparseGridCVars::SG_QueryDebugTime, 0, ST_SparseGridCVars::SG_LineThickness); }
// #endif
// 					OutComponents.Add(ComponentItr);
// 				}
// 			}
// #if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
// 			else if (bDrawDebug) { DrawHighlightedTile(CIdx, RIdx, FLinearColor(1.f, 0.f, 0.f, 0.25f)); }
// #endif
// 		}
// 	}
// }