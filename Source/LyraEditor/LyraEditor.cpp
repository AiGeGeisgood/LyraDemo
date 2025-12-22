// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraEditor.h"

#include "GameModes/LyraExperienceManager.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogLyraEditor);

void FLyraEditorModule::StartupModule()
{
	FDefaultGameModuleImpl::StartupModule();
	if (!IsRunningGame())
	{
		FEditorDelegates::BeginPIE.AddRaw(this, &FLyraEditorModule::OnBeginPIE);
		FEditorDelegates::EndPIE.AddRaw(this, &FLyraEditorModule::OnEndPIE);
	}
}

void FLyraEditorModule::ShutdownModule()
{
	FDefaultGameModuleImpl::ShutdownModule();
}

void FLyraEditorModule::OnBeginPIE(bool bIsSimulating)
{
	ULyraExperienceManager* ExperienceManager = GEngine->GetEngineSubsystem<ULyraExperienceManager>();
	check(ExperienceManager);
	ExperienceManager->OnPlayInEditorBegun();
}
	
void FLyraEditorModule::OnEndPIE(bool bIsSimulating)
{
		
}

IMPLEMENT_MODULE(FLyraEditorModule, LyraEditor);

#undef LOCTEXT_NAMESPACE