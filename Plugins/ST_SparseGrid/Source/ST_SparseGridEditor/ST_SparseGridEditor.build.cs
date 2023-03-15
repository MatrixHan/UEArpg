// Copyright (C) James Baxter. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class ST_SparseGridEditor : ModuleRules
{
    public ST_SparseGridEditor(ReadOnlyTargetRules ROTargetRules) : base(ROTargetRules)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnforceIWYU = true;

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Mode"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Slate"));

        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Mode"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Slate"));

        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core",
            "CoreUObject", 
            "Engine",
            "UnrealEd",
            "ST_SparseGrid" 
        });
        PrivateDependencyModuleNames.AddRange(new string[] {
            "InputCore",
            "Slate",
            "SlateCore",
            "PropertyEditor", 
            "LevelEditor",
            "EditorStyle",
            "Projects",
            "RHI",
            "RenderCore",
            "KismetWidgets" 
        });
    }
}