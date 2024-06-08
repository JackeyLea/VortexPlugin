using System.IO;

using UnrealBuildTool;

public class VortexRuntime : ModuleRules
{
    // runtime module build rules
    public VortexRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Landscape", "Projects", "VortexStudio", "ProceduralMeshComponent", "DeveloperSettings", "RHI", "RenderCore" });
        SetupModulePhysicsSupport(Target);
        if (Target.bBuildEditor == true)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }
    }
}
