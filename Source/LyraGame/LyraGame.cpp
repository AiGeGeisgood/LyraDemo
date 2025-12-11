// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraGame.h"
#include "Modules/ModuleManager.h"




void FLyraGameModule::StartupModule()
{
	FDefaultGameModuleImpl::StartupModule();
}

void FLyraGameModule::ShutdownModule()
{
	FDefaultGameModuleImpl::ShutdownModule();
}


IMPLEMENT_PRIMARY_GAME_MODULE(FLyraGameModule, LyraGame, "LyraGame");
