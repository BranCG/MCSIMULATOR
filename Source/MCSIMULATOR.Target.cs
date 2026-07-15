using UnrealBuildTool;
using System.Collections.Generic;

public class MCSIMULATORTarget : TargetRules
{
	public MCSIMULATORTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MCSIMULATOR");
	}
}
