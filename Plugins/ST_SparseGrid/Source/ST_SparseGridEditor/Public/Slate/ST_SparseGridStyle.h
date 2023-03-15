// Copyright (C) James Baxter. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

// Declarations
class FSlateStyleSet;
class ISlateStyle;

/*
* Plugin Style Set
*/
class FST_SparseGridStyle
{
public:
	static void Initialize();
	static void Shutdown();
	static TSharedPtr<ISlateStyle> Get();
	static FName GetStyleSetName() { return SGStyleName; }

	static const FSlateBrush* GetBrush(FName BrushName);
	static const FSlateFontInfo GetPluginFont(uint16 FontSize);
	
private:
	static FString PluginResources(const FString& RelativePath, const ANSICHAR* Extension);
	static TSharedPtr<FSlateStyleSet> SGStyleInstance;
	static const FName SGStyleName;
};