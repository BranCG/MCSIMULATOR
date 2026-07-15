using UnrealBuildTool;
using System.Collections.Generic;

public class MCSIMULATOREditorTarget : TargetRules
{
	public MCSIMULATOREditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MCSIMULATOR");
	}
}
