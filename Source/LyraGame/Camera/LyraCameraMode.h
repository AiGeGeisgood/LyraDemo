// Copyright Epic Games, Inc. All Rights Reserved.
// 001. 只创建了空类
// Finished.
#pragma once

#include "Engine/World.h"
#include "GameplayTagContainer.h"

#include "LyraCameraMode.generated.h"


#define UE_API LYRAGAME_API

class AActor;
class UCanvas;
class ULyraCameraComponent;


/**
 * ELyraCameraModeBlendFunction
 *
 *	Blend function used for transitioning between camera modes.
 *	用于在相机模式之间过渡的混合函数。
 */
UENUM(BlueprintType)
enum class ELyraCameraModeBlendFunction : uint8
{
	// Does a simple linear interpolation.
	// 进行简单的线性插值。
	Linear,

	// Immediately accelerates, but smoothly decelerates into the target.  Ease amount controlled by the exponent.
	// 立即加速，但会平稳地减速直至抵达目标位置。减速幅度由指数值控制。
	EaseIn,

	// Smoothly accelerates, but does not decelerate into the target.  Ease amount controlled by the exponent.
	// 以平稳的方式加速，但不会减速直至抵达目标位置。减速幅度由指数值控制。
	EaseOut,

	// Smoothly accelerates and decelerates.  Ease amount controlled by the exponent.
	// 能平稳地加速和减速。减速幅度由指数值控制。
	EaseInOut,

	COUNT	UMETA(Hidden)
};

/**
 * FLyraCameraModeView
 *
 *	View data produced by the camera mode that is used to blend camera modes.
 *  查看由用于混合不同拍摄模式的相机模式所生成的数据。
 *	
 */
struct FLyraCameraModeView
{
public:

	// FLyraCameraModeView();


	// 将自己与其他的相机模式混合数据进行混合
	// void Blend(const FLyraCameraModeView& Other, float OtherWeight);
	
public:

	// 位置
	FVector Location;
	
	// 旋转
	FRotator Rotation;
	
	// 控制旋转
	FRotator ControlRotation;
	
	// FOV
	float FieldOfView;
	
};




/**
 * ULyraCameraMode
 *
 *	Base class for all camera modes.
 *	所有相机模式的基类。
 */
UCLASS(MinimalAPI, Abstract, NotBlueprintable)
class ULyraCameraMode : public UObject
{
	GENERATED_BODY()

public:
	
	// 构造函数 初始化成员变量
	// UE_API ULyraCameraMode();


	// 获取相机组件 通过Outer去获取
// 	UE_API ULyraCameraComponent* GetLyraCameraComponent() const;
//
//
// 	// 获取世界
// 	UE_API virtual UWorld* GetWorld() const override;
// 	
// 	// 获取持有这个相机模式的Actor
// 	UE_API AActor* GetTargetActor() const;
// 	
// 	// 获取相机模式的视角数据
// 	const FLyraCameraModeView& GetCameraModeView() const { return View; }
//
// 	// Called when this camera mode is activated on the camera mode stack.
// 	// 当此相机模式在相机模式栈中被激活时触发此事件。
// 	virtual void OnActivation() {};
//
// 	// Called when this camera mode is deactivated on the camera mode stack.
// 	// 当此相机模式在相机模式栈中被停用时触发此事件。
// 	virtual void OnDeactivation() {};
//
// 	// 更新相机模式
// 	UE_API void UpdateCameraMode(float DeltaTime);
//
// 	// 获取混合时间
// 	float GetBlendTime() const { return BlendTime; }
// 	// 获取混合权重
// 	float GetBlendWeight() const { return BlendWeight; }
//
// 	// 设置混合权重 需要根据当前相机的混合模式为设置
// 	// 外部调用 调整相机模式时使用
// 	// 正常是由Tick更新
// 	UE_API void SetBlendWeight(float Weight);
//
// 	// 获取到相机模式的tag 
// 	FGameplayTag GetCameraTypeTag() const
// 	{
// 		return CameraTypeTag;
// 	}
//
// 	// 绘制调试信息 由相机容器调用
// 	UE_API virtual void DrawDebug(UCanvas* Canvas) const;
//
// protected:
// 	// 获取到相机的位置 考虑到蹲伏时胶囊提会发生变化
// 	UE_API virtual FVector GetPivotLocation() const;
// 	// 获取到相机的旋转
// 	UE_API virtual FRotator GetPivotRotation() const;
//
// 	// 更新视角数据
// 	UE_API virtual void UpdateView(float DeltaTime);
//
// 	// 更新混合数据
// 	UE_API virtual void UpdateBlending(float DeltaTime);
	

	
protected:
	// A tag that can be queried by gameplay code that cares when a kind of camera mode is active
	// without having to ask about a specific mode (e.g., when aiming downsights to get more accuracy)
	// 一个标签，可通过游戏代码进行查询，该代码能够了解某种摄像模式是否处于激活状态
	// 无需专门询问某一特定模式（例如，在瞄准时降低视角以提高准确性时）
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	FGameplayTag CameraTypeTag;

	// View output produced by the camera mode.
	// 查看由相机模式生成的输出内容。
	FLyraCameraModeView View;


	// The horizontal field of view (in degrees).
	// 水平视角范围（以度为单位）
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "5.0", UIMax = "170", ClampMin = "5.0", ClampMax = "170.0"))
	float FieldOfView;

	
	// Minimum view pitch (in degrees).
	// 最小视角倾斜度（以度为单位）
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMin;

	// Maximum view pitch (in degrees).
	// 最大视角仰角（以度为单位）
	UPROPERTY(EditDefaultsOnly, Category = "View", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMax;


	// How long it takes to blend in this mode.
	// 在这种模式下完成混合所需的时间是多久。
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	float BlendTime;

	// Function used for blending.
	// 用于混合的函数。
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	ELyraCameraModeBlendFunction BlendFunction;


	// Exponent used by blend functions to control the shape of the curve.
	// 用于控制曲线形状的混合函数所使用的指数值。
	UPROPERTY(EditDefaultsOnly, Category = "Blending")
	float BlendExponent;
	
	// Linear blend alpha used to determine the blend weight.
	// 使用线性混合透明度来确定混合权重。
	float BlendAlpha;

	// Blend weight calculated using the blend alpha and function.
	// 利用混合透明度和函数计算混合权重。
	float BlendWeight;


	
protected:
	/** If true, skips all interpolation and puts camera in ideal location.  Automatically set to false next frame. */
	/** 若为真，则跳过所有插值操作，并将摄像机置于理想位置。该设置会自动在下一帧变为假。*/
	// 子类拓展使用
	UPROPERTY(transient)
	uint32 bResetInterpolation:1;


	
};

/**
 * ULyraCameraModeStack
 *
 *	Stack used for blending camera modes.
 *	用于切换相机模式的堆栈功能。
 *	注意,这里不要按照堆或者栈的思想去了解,就当成一个特殊的容器就行.要不然会很奇怪.
 */
UCLASS()
class ULyraCameraModeStack : public UObject
{
	GENERATED_BODY()

public:
	
// 	// 构造函数 默认激活自己的功能 避免重复激活 
// 	ULyraCameraModeStack();
// 	
// 	// 激活 转发给相机模式 触发激活函数
// 	void ActivateStack();
// 	// 取消激活 通知相机模式它们正在被停用。
// 	void DeactivateStack();
//
// 	// 当前相机容器是否激活
// 	bool IsStackActivate() const { return bIsActive; }
//
// 	// 将一个新的相机模式的类推送过来,进行实例化,注意内部会去重.
// 	void PushCameraMode(TSubclassOf<ULyraCameraMode> CameraModeClass);
//
//
// 	// 评估相机的栈
// 	// 移除已经被使用完毕的相机混入模式
// 	// 获取最终的相机模式混入数据
// 	// 由相机组件调用
// 	bool EvaluateStack(float DeltaTime, FLyraCameraModeView& OutCameraModeView);
//
// 	// 绘制相机调试信息
// 	void DrawDebug(UCanvas* Canvas) const;
// 	
// 	// Gets the tag associated with the top layer and the blend weight of it
// 	// 获取顶层所关联的标签及其混合权重
// 	void GetBlendInfo(float& OutWeightOfTopLayer, FGameplayTag& OutTagOfTopLayer) const;
//
// protected:
//
// 	// 获取相机模式的实例
// 	// 对象池
// 	ULyraCameraMode* GetCameraModeInstance(TSubclassOf<ULyraCameraMode> CameraModeClass);
// 	
// 	// 内部功能实现函数 由EvaluateStack调用
// 	// 更新相机模式的栈容器权重
// 	void UpdateStack(float DeltaTime);
// 	// 内部功能实现函数 由EvaluateStack调用
// 	// 获取最终混合的相机视角效果
// 	void BlendStack(FLyraCameraModeView& OutCameraModeView) const;

	
protected:

	bool bIsActive;

	// 相机模式的对象池子 避免重复重建
	UPROPERTY()
	TArray<TObjectPtr<ULyraCameraMode>> CameraModeInstances;

	// 相机模式的容器,正在使用的对象们
	UPROPERTY()
	TArray<TObjectPtr<ULyraCameraMode>> CameraModeStack;

};





#undef UE_API
