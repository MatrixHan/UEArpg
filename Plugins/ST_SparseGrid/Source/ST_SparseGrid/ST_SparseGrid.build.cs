// Copyright (C) James Baxter. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class ST_SparseGrid : ModuleRules
{
    public ST_SparseGrid(ReadOnlyTargetRules ROTargetRules) : base(ROTargetRules)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnforceIWYU = true;

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Components"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Grid"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public/Utilities"));

        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Components"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Grid"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private/Utilities"));

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine"});
    }
}