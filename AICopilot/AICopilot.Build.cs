using UnrealBuildTool;

public class AICopilot : ModuleRules
{
	public AICopilot(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"HTTP",
				"Json",
				"DesktopPlatform",
				"MainFrame",
				"LiveCoding",
				"JsonUtilities",
				"DeveloperSettings",
				"PythonScriptPlugin",
				"BlueprintGraph",
				"Kismet",
				"UnrealEd",
				"ApplicationCore" // For Clipboard operations
			}
			);
	}
}
