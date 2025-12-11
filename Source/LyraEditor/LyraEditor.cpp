// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraEditor.h"
#include "Modules/ModuleManager.h"


void FLyraEditorModule::StartupModule()
{
	FDefaultGameModuleImpl::StartupModule();
}

void FLyraEditorModule::ShutdownModule()
{
	FDefaultGameModuleImpl::ShutdownModule();
}

IMPLEMENT_MODULE(FLyraEditorModule, LyraEditor);
