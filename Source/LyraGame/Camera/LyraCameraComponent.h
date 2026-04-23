// Copyright Epic Games, Inc. All Rights Reserved.
// Finished.
// 001 添加了空白类和 相机获取的代理

#pragma once

#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"

#include "LyraCameraComponent.generated.h"


class UCanvas;
class ULyraCameraMode;
class ULyraCameraModeStack;
class UObject;
struct FFrame;
struct FGameplayTag;
struct FMinimalViewInfo;
template <class TClass> class TSubclassOf;

DECLARE_DELEGATE_RetVal(TSubclassOf<ULyraCameraMode>, FLyraCameraModeDelegate);


/**
 * ULyraCameraComponent
 *
 *	The base camera component class used by this project.
 */
UCLASS()
class ULyraCameraComponent : public UCameraComponent
{
	GENERATED_BODY()

public:
	// 构造函数 初始化成员变量
	ULyraCameraComponent(const FObjectInitializer& ObjectInitializer);

// 	// Returns the camera component if one exists on the specified actor.
// 	UFUNCTION(BlueprintPure, Category = "Lyra|Camera")
// 	static ULyraCameraComponent* FindCameraComponent(const AActor* Actor)
// 	{ return (Actor ? Actor->FindComponentByClass<ULyraCameraComponent>() : nullptr); }
//
//
//
// 	// Returns the target actor that the camera is looking at.
// 	// 返回摄像机所注视的目标角色。
// 	virtual AActor* GetTargetActor() const { return GetOwner(); }
//
//
// 	
// 	// Delegate used to query for the best camera mode.
// 	// 该委托用于查询最佳拍摄模式。
// 	FLyraCameraModeDelegate DetermineCameraModeDelegate;
//
// 	// Add an offset to the field of view.  The offset is only for one frame, it gets cleared once it is applied.
// 	// 对视野范围添加偏移量。该偏移量仅适用于当前帧，一旦应用便会被清除。
// 	void AddFieldOfViewOffset(float FovOffset) { FieldOfViewOffset += FovOffset; }
// 	
//
// 	// 绘制Debug信息,由相机管理器调用
// 	virtual void DrawDebug(UCanvas* Canvas) const;
// 	
// 	// Gets the tag associated with the top layer and the blend weight of it
// 	// 获取顶层所关联的标签及其混合权重
// 	// 主要用于开镜时 获取我们相机混入的程度 来决定加成的程度
// 	void GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const;
//
// protected:
//
// 	/**
// 	 * 当组件被注册时触发此事件，此时场景已设置完成，但尚未调用 CreateRenderState_Concurrent 或 OnCreatePhysicsState 方法。
// 	 * 确保有一个相机模式的栈容器
// 	 */
// 	virtual void OnRegister() override;
//
// 	/**
// 	 * 返回相机的视角。
// 	 * 由相机类调用。子类化并进行后期处理以添加任何效果。
// 	 * 
// 	 */
// 	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;
// 	
//
// 	// 读取代理,获取当前最新指定的相机模式,并尝试推送到栈中进行使用,内部已经进行对相机模式的去重使用
// 	virtual void UpdateCameraModes();
//
// protected:
// 	// Stack used to blend the camera modes.
// 	// 用于切换相机模式的栈结构。
// 	UPROPERTY()
// 	TObjectPtr<ULyraCameraModeStack> CameraModeStack;
//
// 	// Offset applied to the field of view.  The offset is only for one frame, it gets cleared once it is applied.
// 	// 对视野范围所做的偏移。该偏移仅适用于当前一帧，一旦应用完毕便会自动清除。
// 	float FieldOfViewOffset;
	
	
};