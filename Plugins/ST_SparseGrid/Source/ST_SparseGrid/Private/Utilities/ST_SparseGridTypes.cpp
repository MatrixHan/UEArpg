// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridTypes.h"

///////////////////
///// Logging /////
///////////////////

DEFINE_LOG_CATEGORY(LogST_SparseGrid);
DEFINE_LOG_CATEGORY(LogST_SparseGridManager);

/////////////////////////////
///// Console Variables /////
/////////////////////////////

#if SPARSE_GRID_DEBUG
TAutoConsoleVariable<int32> ST_SparseGridCVars::CVarDrawDebug = TAutoConsoleVariable<int32>(
	TEXT("SparseGrid.DrawDebug"),
	0,
	TEXT("Enable/Disable the drawing of the Debug Grid\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

TAutoConsoleVariable<float> ST_SparseGridCVars::CVarQueryDebugTime = TAutoConsoleVariable<float>(
	TEXT("SparseGrid.QueryDebugTime"),
	-1.f,
	TEXT("Length of time to draw debug information\n"),
	ECVF_Cheat);

#if ENABLE_GRID_BOUNDS
TAutoConsoleVariable<int32> ST_SparseGridCVars::CVarDrawBounds = TAutoConsoleVariable<int32>(
	TEXT("SparseGrid.DrawBounds"),
	0,
	TEXT("Enable/Disable the drawing of the Registered Object Bounds\n")
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);
#endif

TAutoConsoleVariable<int32> ST_SparseGridCVars::CVarFillGrid = TAutoConsoleVariable<int32>(
	TEXT("SparseGrid.FillGrid"),
	0,
	TEXT("Fill the grid segments with colour\n")
	TEXT("0: Edged Only, 1: Filled"),
	ECVF_Cheat);

TAutoConsoleVariable<int32> ST_SparseGridCVars::CVarDrawLinks = TAutoConsoleVariable<int32>(
	TEXT("SparseGrid.DrawLinks"),
	0,
	TEXT("Draw link lines between components and their grid references\n")
	TEXT("0: Disable, 1: Enabled"),
	ECVF_Cheat);

TAutoConsoleVariable<int32> ST_SparseGridCVars::CVarDrawInfo = TAutoConsoleVariable<int32>(
	TEXT("SparseGrid.DrawInfo"),
	0,
	TEXT("Draw cell information such as ID and number of objects\n")
	TEXT("0: Disable, 1: Enabled"),
	ECVF_Cheat);

TAutoConsoleVariable<float> ST_SparseGridCVars::CVarDebugGridHeight = TAutoConsoleVariable<float>(
	TEXT("SparseGrid.GridHeight"),
	0.f,
	TEXT("Altitude of the debug grid"),
	ECVF_Cheat);

TAutoConsoleVariable<float> ST_SparseGridCVars::CVarDebugGridThickness = TAutoConsoleVariable<float>(
	TEXT("SparseGrid.DebugGridThickness"),
	64.f,
	TEXT("Thickness of the debug grid\n"),
	ECVF_Cheat);

TAutoConsoleVariable<int32> ST_SparseGridCVars::CVarDebugHotThreshold = TAutoConsoleVariable<int32>(
	TEXT("SparseGrid.HotThreshold"),
	16,
	TEXT("Max objects in a cell for full colourisation\n"),
	ECVF_Cheat);

TAutoConsoleVariable<FString> ST_SparseGridCVars::CVarHotHex = TAutoConsoleVariable<FString>(
	TEXT("SparseGrid.HotHex"),
	*FLinearColor(1.f, 0.f, 0.f, 0.75f).ToFColor(true).ToHex(),
	TEXT("Hex Value of Hot Colour\n"),
	ECVF_Cheat);

TAutoConsoleVariable<FString> ST_SparseGridCVars::CVarColdHex = TAutoConsoleVariable<FString>(
	TEXT("SparseGrid.ColdHex"),
	*FLinearColor(0.f, 1.f, 0.f, 0.75f).ToFColor(true).ToHex(),
	TEXT("Hex Value of Cold Colour\n"),
	ECVF_Cheat);
#endif