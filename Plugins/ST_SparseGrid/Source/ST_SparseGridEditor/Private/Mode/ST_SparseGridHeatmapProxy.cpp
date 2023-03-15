// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridHeatmapProxy.h"

///////////////////////
///// Constructor /////
///////////////////////

UST_SparseGridHeatmapProxy::UST_SparseGridHeatmapProxy(const FObjectInitializer& OI)
	: Super(OI)
{
	HotColour = FLinearColor::Red;
	ColdColour = FLinearColor::Green;
	HotThreshold = 16;
}

//////////////////
///// Config /////
//////////////////

void UST_SparseGridHeatmapProxy::PostLoad()
{
	Super::PostLoad();

	UpdateCVars();
}

#if WITH_EDITOR
void UST_SparseGridHeatmapProxy::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UpdateCVars();
}
#endif

/////////////////
///// CVars /////
/////////////////

void UST_SparseGridHeatmapProxy::UpdateCVars()
{
	if (bUpdateCVars)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		static IConsoleManager& CManager = IConsoleManager::Get();

		IConsoleVariable* DebugHotColour_CVar = CManager.FindConsoleVariable(TEXT("SparseGrid.HotHex"));
		if (ensure(DebugHotColour_CVar))
		{
			DebugHotColour_CVar->Set(*HotColour.ToFColor(true).ToHex(), ECVF_SetByConsole);
		}
		
		IConsoleVariable* DebugColdColour_CVar = CManager.FindConsoleVariable(TEXT("SparseGrid.ColdHex"));
		if (ensure(DebugColdColour_CVar))
		{
			DebugColdColour_CVar->Set(*ColdColour.ToFColor(true).ToHex(), ECVF_SetByConsole);
		}

  		IConsoleVariable* DebugThreshold_CVar = CManager.FindConsoleVariable(TEXT("SparseGrid.HotThreshold"));
  		if (ensure(DebugThreshold_CVar))
  		{
			DebugThreshold_CVar->Set(HotThreshold, ECVF_SetByConsole);
  		}
#endif
	}
}