// Copyright (C) James Baxter. All Rights Reserved.

#include "ST_SparseGridStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/CoreStyle.h"

#define IMAGE_PLUGIN_BRUSH(RelativePath, ...) FSlateImageBrush(FST_SparseGridStyle::PluginResources(RelativePath, ".png"), __VA_ARGS__ )

TSharedPtr<FSlateStyleSet> FST_SparseGridStyle::SGStyleInstance = nullptr;
const FName FST_SparseGridStyle::SGStyleName = FName(TEXT("SGStyle"));

void FST_SparseGridStyle::Initialize()
{
	if (SGStyleInstance.IsValid()) { return; }

	SGStyleInstance = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	SGStyleInstance->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
	SGStyleInstance->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	// Icons
	const FVector2D Icon40x40 = FVector2D(40.f, 40.f);
	const FVector2D Icon20x20 = FVector2D(20.f, 20.f);
	const FVector2D Icon16x16 = FVector2D(16.f, 16.f);

	SGStyleInstance->Set("SparseGrid.TabIcon", new IMAGE_PLUGIN_BRUSH("Icons/SGIcon", Icon40x40));
	SGStyleInstance->Set("SparseGrid.TabIcon.Small", new IMAGE_PLUGIN_BRUSH("Icons/SGIcon", Icon20x20));
	SGStyleInstance->Set("SparseGrid.TabIcon.Menu", new IMAGE_PLUGIN_BRUSH("Icons/SGIcon", Icon16x16));

	SGStyleInstance->Set("SparseGrid.StormtideIcon", new IMAGE_PLUGIN_BRUSH("Icons/STIcon", Icon40x40));
	SGStyleInstance->Set("SparseGrid.StormtideIcon.Small", new IMAGE_PLUGIN_BRUSH("Icons/STIcon", Icon20x20));
	SGStyleInstance->Set("SparseGrid.StormtideIcon.Menu", new IMAGE_PLUGIN_BRUSH("Icons/STIcon", Icon16x16));

	SGStyleInstance->Set("SparseGrid.HeatmapIcon", new IMAGE_PLUGIN_BRUSH("Icons/HMIcon", Icon40x40));
	SGStyleInstance->Set("SparseGrid.HeatmapIcon.Small", new IMAGE_PLUGIN_BRUSH("Icons/HMIcon", Icon20x20));
	SGStyleInstance->Set("SparseGrid.HeatmapIcon.Menu", new IMAGE_PLUGIN_BRUSH("Icons/HMIcon", Icon16x16));

	FSlateStyleRegistry::RegisterSlateStyle(*SGStyleInstance.Get());
}

void FST_SparseGridStyle::Shutdown()
{
	if (SGStyleInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*SGStyleInstance.Get());
		ensure(SGStyleInstance.IsUnique());
		SGStyleInstance.Reset();
	}
}

TSharedPtr<ISlateStyle> FST_SparseGridStyle::Get()
{
	return SGStyleInstance;
}

const FSlateBrush* FST_SparseGridStyle::GetBrush(FName BrushName)
{
	if (SGStyleInstance.IsValid())
	{
		return SGStyleInstance->GetBrush(BrushName);
	}

	return nullptr;
}

const FSlateFontInfo FST_SparseGridStyle::GetPluginFont(uint16 FontSize)
{
	return FCoreStyle::GetDefaultFontStyle("Light", FontSize);
	//return FSlateFontInfo(FPaths::EngineContentDir() / ("Slate/Fonts/Roboto-Light", ".ttf"), FontSize);
}

FString FST_SparseGridStyle::PluginResources(const FString& RelativePath, const ANSICHAR* Extension)
{
	static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("ST_SparseGrid"))->GetBaseDir() / TEXT("Resources");
	return (ContentDir / RelativePath) + Extension;
}

#undef IMAGE_PLUGIN_BRUSH