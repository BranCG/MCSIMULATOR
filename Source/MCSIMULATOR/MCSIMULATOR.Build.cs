using UnrealBuildTool;

public class MCSIMULATOR : ModuleRules
{
	public MCSIMULATOR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput", 
			"HeadMountedDisplay",
			"XRBase",
			"HTTP", 
			"Json", 
			"JsonUtilities", 
			"AudioCapture"
		});

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PublicDependencyModuleNames.Add("AndroidPermission");
		}

		PrivateDependencyModuleNames.AddRange(new string[] {  });
		
		// Uncomment if you are using Slate UI elements in C++
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}
