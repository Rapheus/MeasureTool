using UnrealBuildTool;

public class MeasureTool : ModuleRules
{
	public MeasureTool(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "ProceduralMeshComponent", "UMG"
		});
	}
}
