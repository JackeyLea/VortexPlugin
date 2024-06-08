using System.IO;
using UnrealBuildTool;

public class VortexStudio : ModuleRules
{
    public VortexStudio(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Win64", "include"));

            // Add the import library
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "Win64", "lib", "VortexIntegration.lib"));

            // Delay-load the DLL, so we can load it from the right place first
            PublicDelayLoadDLLs.Add("VortexIntegration.dll");
        }
    }
}
