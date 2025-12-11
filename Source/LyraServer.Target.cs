// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class LyraServerTarget : TargetRules
{
	public LyraServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
	
		ExtraModuleNames.Add("LyraGame");
		
		LyraGameTarget.ApplySharedLyraTargetSettings(this);
	}
}
