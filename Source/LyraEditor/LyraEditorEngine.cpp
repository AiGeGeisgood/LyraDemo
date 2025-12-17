// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraEditorEngine.h"


#include "Development/LyraDeveloperSettings.h"
#include "Development/LyraPlatformEmulationSettings.h"
#include "Engine/GameInstance.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameModes/LyraWorldSettings.h"
#include "Settings/ContentBrowserSettings.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraEditorEngine)
class IEngineLoop;

#define LOCTEXT_NAMESPACE "LyraEditor"

ULyraEditorEngine::ULyraEditorEngine(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void ULyraEditorEngine::Init(IEngineLoop* InEngineLoop)
{
	Super::Init(InEngineLoop);
}

void ULyraEditorEngine::Start()
{
	Super::Start();
}

void ULyraEditorEngine::Tick(float DeltaSeconds, bool bIdleMode)
{
	Super::Tick(DeltaSeconds, bIdleMode);

	FirstTickSetup();
}

FGameInstancePIEResult ULyraEditorEngine::PreCreatePIEInstances(const bool bAnyBlueprintErrors,
	const bool bStartInSpectatorMode, const float PIEStartTime, const bool bSupportsOnlinePIE,
	int32& InNumOnlinePIEInstances)
{

	FGameInstancePIEResult Result = Super::PreCreatePIEServerInstance(bAnyBlueprintErrors, bStartInSpectatorMode, PIEStartTime, bSupportsOnlinePIE, InNumOnlinePIEInstances);

	return Result;
}

void ULyraEditorEngine::FirstTickSetup()
{
	if (bFirstTickSetup)
	{
		return;
	}

	bFirstTickSetup = true;

	// Force show plugin content on load.
	GetMutableDefault<UContentBrowserSettings>()->SetDisplayPluginFolders(true);
	
}
#undef LOCTEXT_NAMESPACE
