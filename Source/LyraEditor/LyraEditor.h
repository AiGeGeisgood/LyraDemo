// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FLyraEditorModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override;
	
	virtual void ShutdownModule() override;
};