using System.IO;

using UnrealBuildTool;

public class VortexEditor : ModuleRules
{
    // editor module build rules
    public VortexEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "VortexStudio" });
        PrivateDependencyModuleNames.AddRange(new string[] { "VortexRuntime", "AssetTools", "AssetRegistry", "UnrealEd", "Slate", "SlateCore", "Projects", "RawMesh", "RenderCore", "MeshDescription", "StaticMeshDescription" });
    }
}
