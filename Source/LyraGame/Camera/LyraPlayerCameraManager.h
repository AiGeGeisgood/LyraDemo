// Copyright Epic Games, Inc. All Rights Reserved.
// Finished.
// 001 Not Finished. 只写了一个基础类.
#pragma once

#include "Camera/PlayerCameraManager.h"

#include "LyraPlayerCameraManager.generated.h"



class FDebugDisplayInfo;
class UCanvas;
class UObject;


// 默认限制值
#define LYRA_CAMERA_DEFAULT_FOV			(80.0f)
#define LYRA_CAMERA_DEFAULT_PITCH_MIN	(-89.0f)
#define LYRA_CAMERA_DEFAULT_PITCH_MAX	(89.0f)


class ULyraUICameraManagerComponent;
/**
 * ALyraPlayerCameraManager
 *
 *	The base player camera manager class used by this project.
 *  本项目所使用的基础玩家摄像机管理类。
 */
UCLASS(notplaceable, MinimalAPI)
class ALyraPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()
	
public:

	
	// 构造函数 初始化成员变量
	ALyraPlayerCameraManager(const FObjectInitializer& ObjectInitializer);

	// 获取UI相机管理组件
	ULyraUICameraManagerComponent* GetUICameraComponent() const;

protected:
	// 根据给定的目标视点计算出新的视角位置。
	// 可以关注一下它的父类调用
	// 这里主要是嵌入UI相机管理
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	
	// 转发给相机 绘制调试信息
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

	
private:
	/** The UI Camera Component, controls the camera when UI is doing something important that gameplay doesn't get priority over. */
	/** 该 UI 摄像头组件用于在游戏玩法无法优先处理某些重要事项时控制摄像机。*/
	UPROPERTY(Transient)
	TObjectPtr<ULyraUICameraManagerComponent> UICamera;
	
	
};

