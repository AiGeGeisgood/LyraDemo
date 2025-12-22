// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLyraEditor, Log, All);


class FLyraEditorModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override;
	
	virtual void ShutdownModule() override;
	
	void OnBeginPIE(bool bIsSimulating);
	void OnEndPIE(bool bIsSimulating);
};