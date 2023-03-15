// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridData.h"
#include "ST_SparseGridManager_Basic.h"

// Extras
#include "SceneManagement.h"

#if WITH_EDITOR
#include "Logging/TokenizedMessage.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Misc/MapErrors.h"
#include "../DrawUtil.h"
#endif

#define LOCTEXT_NAMESPACE "NSSparseGridData"
#define ENABLE_DRAW_DEBUG

///////////////////////
///// Constructor /////
///////////////////////

UST_SparseGridData::UST_SparseGridData(const FObjectInitializer& OI)
	: Super(OI)
{
	GridOrigin = FST_GridRef2D(-2500, -2500);
	NumCellsX = 5;
	NumCellsY = 5;
	CellSize = 1000;
	RegisterAllocSize = 128;
	CellAllocSize = 16;

	ManagerClass = UST_SparseGridManager_Basic::StaticClass();

	RegisterAllocShrinkMultiplier = 0;
	CellAllocShrinkMultiplier = 1;

#if WITH_EDITORONLY_DATA
	DrawAltitude = 0.f;
	BoxExtent = 1500.f;
	GridColour = FLinearColor(1.f, 0.1f, 0.02f, 1.f);
#endif
}

////////////////////////////
///// Editor Interface /////
////////////////////////////

#if WITH_EDITORONLY_DATA
FText UST_SparseGridData::GetGridDiagnosticText() const
{
	FNumberFormattingOptions FormatOps;
	FormatOps.SetMaximumFractionalDigits(2);
	FormatOps.SetMinimumFractionalDigits(2);

	FFormatOrderedArguments OrderedArgs;
	OrderedArgs.Add(FText::FromString(LINE_TERMINATOR));
	OrderedArgs.Add(FText::Format(LOCTEXT("CellCount", "Num Cells: {0}"), FText::AsNumber(NumCellsX * NumCellsY)));
	OrderedArgs.Add(FText::Format(LOCTEXT("GridSize", "Total Size: {0}M x {1}M"), FText::AsNumber((CellSize * NumCellsX) / 100.f, &FormatOps), FText::AsNumber((CellSize * NumCellsY) / 100.f, &FormatOps)));

	return FText::Format(LOCTEXT("GridDiagnostics", "{1}{0}{2}"), OrderedArgs);
}

void UST_SparseGridData::FitToWorldBounds(const FVector& InWorldMin, const FVector& InWorldMax)
{
	const FVector2D Size = FVector2D(InWorldMax) - FVector2D(InWorldMin);
	if (Size.X > 0.f && Size.Y > 0.f)
	{
		const int32 IdealCellSize = FMath::CeilToInt(FMath::GetMappedRangeValueClamped(FVector2D(1000.f, 250000.f), FVector2D(400.f, 16000.f), (Size.X + Size.Y) * 0.5f));

		const int32 NumRequiredX = FMath::CeilToInt(Size.X / IdealCellSize);
		const int32 NumRequiredY = FMath::CeilToInt(Size.Y / IdealCellSize);

		SetGridOrigin(FST_GridRef2D(FMath::FloorToInt(InWorldMin.X), FMath::FloorToInt(InWorldMin.Y)));
		SetNumCellsX(NumRequiredX);
		SetNumCellsY(NumRequiredY);
		SetCellSize(IdealCellSize);
	}
}

void UST_SparseGridData::ModifyDensity(int32 InDelta)
{
	if (InDelta == 0) { return; }

	// Clamp Delta so we don't exceed min/max number of cells on either axis
	if (InDelta > 0)
	{
		InDelta = FMath::Clamp(InDelta, 0, 192 - FMath::Max(NumCellsX, NumCellsY));
	}
	else
	{
		InDelta = FMath::Clamp(InDelta, -FMath::Min(NumCellsX, NumCellsY) + 1, 0);
	}

	const FST_GridRef2D CurrentGridSize = FST_GridRef2D(NumCellsX, NumCellsY) * CellSize;

	NumCellsX += InDelta;
	NumCellsY += InDelta;

	// Adjust Cell Size so we still cover the same area (or larger) if required
	CellSize = FMath::Max(FMath::CeilToInt(CurrentGridSize.X / NumCellsX), FMath::CeilToInt(CurrentGridSize.Y / NumCellsY));
}

/////////////////////
///// Rendering /////
/////////////////////

void UST_SparseGridData::Render(FPrimitiveDrawInterface* InPDI) const
{
	check(InPDI);
	if (DrawAs == EST_SGVisualizerType::DDT_BoxGrid)
	{
		Render_Box(InPDI);
	}
	else if(DrawAs == EST_SGVisualizerType::DDT_FlatGrid)
	{
		Render_Grid(InPDI, DrawAltitude);
	}
	else if (DrawAs == EST_SGVisualizerType::DDT_FlatGridSprite)
	{
		Render_Grid_Sprite(InPDI, DrawAltitude);
	}
}

void UST_SparseGridData::Render_Box(FPrimitiveDrawInterface* PDI) const
{
	check(PDI);

	const float BaseAltitude = DrawAltitude - (BoxExtent);
	const float TopAltitude = DrawAltitude + (BoxExtent);

	Render_Grid(PDI, BaseAltitude);
	Render_Grid(PDI, TopAltitude);

	for (int32 CIdx = 0; CIdx <= NumCellsY; CIdx++)
	{
		const float Y = GridOrigin.Y + (CIdx * CellSize);
		for (int32 RIdx = 0; RIdx <= NumCellsX; RIdx++)
		{
			const float X = GridOrigin.X + (RIdx * CellSize);

			const FVector VertTop = FVector(X, Y, TopAltitude);
			const FVector VertBase = FVector(X, Y, BaseAltitude);
			PDI->DrawLine(VertBase, VertTop, GridColour, SDPG_World, 1.f, 0.f, true);
		}
	}
}

void UST_SparseGridData::Render_Grid(FPrimitiveDrawInterface* PDI, const float InAltitude) const
{
	check(PDI);

	const FST_GridRef2D GridMax = GridOrigin + (FST_GridRef2D(NumCellsX, NumCellsY) * CellSize);

	// We can draw these as solid lines from start to end
	for (int32 CIdx = 0; CIdx <= NumCellsY; CIdx++)
	{
		const float Y = GridOrigin.Y + (CIdx * CellSize);

		const FVector LineStart = FVector(GridOrigin.X, Y, InAltitude);
		const FVector LineEnd = FVector(GridMax.X, Y, InAltitude);
		PDI->DrawLine(LineStart, LineEnd, GridColour, SDPG_World, 1.f, 0.f, true);
	}

	for (int32 RIdx = 0; RIdx <= NumCellsX; RIdx++)
	{
		const float X = GridOrigin.X + (RIdx * CellSize);

		const FVector LineStart = FVector(X, GridOrigin.Y, InAltitude);
		const FVector LineEnd = FVector(X, GridMax.Y, InAltitude);
		PDI->DrawLine(LineStart, LineEnd, GridColour, SDPG_World, 1.f, 0.f, true);
	}
}


void UST_SparseGridData::Render_Grid_Sprite(FPrimitiveDrawInterface* PDI, const float InAltitude) const
{	
	check(PDI);
	DrawUtil::DrawFlush(m_EditorWorld, true, -1.0f, SDPG_World);
	UTexture2D* VertexTexture = GEngine->DefaultBSPVertexTexture;	
	const FST_GridRef2D GridMax = GridOrigin + (FST_GridRef2D(NumCellsX, NumCellsY) * CellSize);
	const float halfSize = CellSize / 2.0f;
	const FColor Color = FColor::Blue;
	for (int32 RIdx = 0; RIdx < NumCellsY; RIdx++) 
	{
		for (int32 RIdy = 0; RIdy < NumCellsX; RIdy++)
		{
			const float Y = GridOrigin.Y + (RIdx * CellSize);
			const float X = GridOrigin.X + (RIdy * CellSize);
			const FVector CenterPosition = FVector(X+halfSize,Y+halfSize,InAltitude);
			//PDI->DrawSprite(CenterPosition, CellSize, CellSize, VertexTexture->Resource, GridColour, SDPG_Foreground, 0.0f, 0.0f, 0.0f, 0.0f, SE_BLEND_MaskedDistanceField);
			const FPlane plane(0, 0,1, CenterPosition.Z);
			DrawUtil::DrawDebugSolidPlane(m_EditorWorld, plane, CenterPosition, CellSize, Color, true, -1.0f, SDPG_World);
			//DrawDebugSolidPlane(GEngine->GetWorld(), plane,CenterPosition, CellSize, Color,false,-1.0f, SDPG_World);
		}
	}
	
	Render_Grid(PDI, InAltitude+1.0f);
}

void UST_SparseGridData::RegisterEditorWorld(const UWorld* EditorWorld)
{
	m_EditorWorld = EditorWorld;
}

#endif

#undef LOCTEXT_NAMESPACE