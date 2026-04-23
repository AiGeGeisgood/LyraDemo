// Copyright Epic Games, Inc. All Rights Reserved.
// Finished.
#pragma once

#include "Camera/PlayerCameraManager.h"

#include "LyraUICameraManagerComponent.generated.h"

class ALyraPlayerCameraManager;

class AActor;
class AHUD;
class APlayerController;
class FDebugDisplayInfo;
class UCanvas;
class UObject;

UCLASS( Transient, Within=LyraPlayerCameraManager )
class ULyraUICameraManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 获取UI相机管理组件
	static ULyraUICameraManagerComponent* GetComponent(APlayerController* PC);

public:
	// 构造函数 注入回调的钩子
	ULyraUICameraManagerComponent();

	// 无作用
	virtual void InitializeComponent() override;

	// 是否正在设置视角Actor
	bool IsSettingViewTarget() const { return bUpdatingViewTarget; }
	// 获取视角对象
	AActor* GetViewTarget() const { return ViewTarget; }
	// 重新设置一个视角对象
	void SetViewTarget(AActor* InViewTarget, FViewTargetTransitionParams TransitionParams = FViewTargetTransitionParams());


	// 是否需要更新视角对象
	bool NeedsToUpdateViewTarget() const;

	
	// 无
	// 嵌入到相机管理组件 可以修改特定需要的视角
	void UpdateViewTarget(struct FTViewTarget& OutVT, float DeltaTime);
	

	// 无
	void OnShowDebugInfo(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos);

	
private:
	
	UPROPERTY(Transient)
	TObjectPtr<AActor> ViewTarget;
	
	UPROPERTY(Transient)
	bool bUpdatingViewTarget;

	
};




