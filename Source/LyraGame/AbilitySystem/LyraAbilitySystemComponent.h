// Copyright Epic Games, Inc. All Rights Reserved.
// Finished.
// 001 目前只需要创建基础类即可.
// 002 补充了几个外部调用的空白函数
#pragma once

#include "Abilities/LyraGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"

#include "LyraAbilitySystemComponent.generated.h"

#define UE_API LYRAGAME_API

class AActor;
class UGameplayAbility;
class ULyraAbilityTagRelationshipMapping;
class UObject;
struct FFrame;
struct FGameplayAbilityTargetDataHandle;

LYRAGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_AbilityInputBlocked);

/**
 * ULyraAbilitySystemComponent
 *
 *	Base ability system component class used by this project.
 * 本项目所使用的基础能力系统组件类。
 */
UCLASS(MinimalAPI)
class ULyraAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:

	UE_API ULyraAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

// 	//~UActorComponent interface
// 	// 在项目定义的全局GAS系统中取消该对象的注册
// 	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
// 	//~End of UActorComponent interface
//
// 	/**
// 	 * 初始化了“能力”对象的“角色信息”——这是一个用于保存有关我们所作用对象以及控制我们的人的信息的结构。
// 	 * 主控角色是实际拥有此组件的实体。
// 	 * 虚拟角色是我们在现实世界中所扮演的物理角色。通常是一个兵兵（Pawn），但也可能是塔（Tower）、建筑（Building）、炮塔（Turret）等，可能与主控角色相同。
// 	 * 
// 	 */
// 	UE_API virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;
//
// 	// 根据传入的函数 来判断当前的能力是否需要取消
//
// 	typedef TFunctionRef<bool(const ULyraGameplayAbility* LyraAbility, FGameplayAbilitySpecHandle Handle)> TShouldCancelAbilityFunc;
// 	UE_API void CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc, bool bReplicateCancelAbility);
//
// 	// 取消那些由输入激活的能力
// 	UE_API void CancelInputActivatedAbilities(bool bReplicateCancelAbility);
//
//
// 	
// 	// 根据增强型输入系统通过Hero组件传递过来的输入资产Tag来添加按下和释放的句柄
// 	UE_API void AbilityInputTagPressed(const FGameplayTag& InputTag);
// 	UE_API void AbilityInputTagReleased(const FGameplayTag& InputTag);
//
//
// 	// 通过玩家控制器处理输入调用 ,每帧去处理输入Tag句柄的容器
// 	UE_API void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
// 	// 清理所有输入的句柄
// 	UE_API void ClearAbilityInput();
//
// 	// 判断传入的这个类别的能力是否可以激活 由GA调用
// 	UE_API bool IsActivationGroupBlocked(ELyraAbilityActivationGroup Group) const;
// 	// 将该能力的类别计数进行增加验证 由GA调用
// 	UE_API void AddAbilityToActivationGroup(ELyraAbilityActivationGroup Group, ULyraGameplayAbility* LyraAbility);
// 	// 将该能力的类别计数进行移除验证 由GA调用
// 	UE_API void RemoveAbilityFromActivationGroup(ELyraAbilityActivationGroup Group, ULyraGameplayAbility* LyraAbility);
// 	// 根据类别去取消之前冲突的能力
// 	UE_API void CancelActivationGroupAbilities(ELyraAbilityActivationGroup Group, ULyraGameplayAbility* IgnoreLyraAbility, bool bReplicateCancelAbility);
//
// 	// Uses a gameplay effect to add the specified dynamic granted tag.
// 	// 使用游戏玩法效果来添加指定的动态授予标签。
// 	UE_API void AddDynamicTagGameplayEffect(const FGameplayTag& Tag);
//
// 	// Removes all active instances of the gameplay effect that was used to add the specified dynamic granted tag.
// 	// 清除所有使用指定动态授予标签来添加游戏效果时所激活的实例。
// 	UE_API void RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag);
//
//
// 	/** Gets the ability target data associated with the given ability handle and activation info */
// 	/** 获取与给定能力标识符及激活信息相关联的能力目标数据 */
// 	// 主要用户获取能力激活的上下文 是否有命中对象等等
// 	UE_API void GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle,
// 		FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle);
//
//
// 	
// 	
// 	/** Sets the current tag relationship mapping, if null it will clear it out */
// 	/** 设置当前的标签关系映射，如果为 null，则会将其清除 */
// 	UE_API void SetTagRelationshipMapping(ULyraAbilityTagRelationshipMapping* NewMapping);
//
//
// 	/** Looks at ability tags and gathers additional required and blocking tags */\
// 	/** 查看能力标签，并收集所需的附加标签和阻碍性标签 */
// 	// 通过TagRelationship来收集 来确认当前能力是否可以释放!
// 	UE_API void GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags,
// 		FGameplayTagContainer& OutActivationRequired,
// 		FGameplayTagContainer& OutActivationBlocked) const;
//
// 	
//
// 	
// 	// 尝试激活出生时应当使用的能力
// 	// 需要对应的能力类型为生成时激活
// 	UE_API void TryActivateAbilitiesOnSpawn();
//
// protected:
// 	/** 对于本地玩家始终会调用此方法。若游戏能力中设置了“bReplicateInputDirectly”标志，则在服务器端也会调用此方法。*/
// 	UE_API virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
// 	/** 对于本地玩家始终会调用此方法。若游戏能力中设置了“bReplicateInputDirectly”标志，则在服务器端也会调用此方法。*/
// 	UE_API virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
//
// 	// 能力激活时通知 添加到能力类别里面
// 	UE_API virtual void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
// 	// 能力激活失败时通知 根据情况是否需要通知给其他客户端
// 	UE_API virtual void NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason) override;
// 	// 能力结束时通知 从能力类别中移除
// 	UE_API virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled) override;
//
// 	// 重写的方法 主要是把我们自定义的TagRelationshipMapping嵌入引擎的流程中
// 	UE_API virtual void ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags,
// 		UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags,
// 		bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags) override;
//
// 	// 无
// 	UE_API virtual void HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bCanBeCanceled) override;
//
// 	
// 	/** Notify client that an ability failed to activate */
// 	/** 向客户端通知某项能力未能激活 */
// 	UFUNCTION(Client, Unreliable)
// 	UE_API void ClientNotifyAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);
// 	// 实际执行某些能力未能激活
// 	UE_API void HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason);
	

protected:

	// If set, this table is used to look up tag relationships for activate and cancel
	// 若已设置，此表用于查找激活和取消操作的标签关系
	UPROPERTY()
	TObjectPtr<ULyraAbilityTagRelationshipMapping> TagRelationshipMapping;

	// Handles to abilities that had their input pressed this frame.
	// 本帧中已按下输入的技能句柄。
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	// Handles to abilities that had their input released this frame.
	// 当前帧中已解除输入的技能的处理指针。
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	// Handles to abilities that have their input held.
	// 持有输入操作的技能的处理程序。
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
	
	// Number of abilities running in each activation group.
	// 每个激活组中运行的能力数量。
	int32 ActivationGroupCounts[(uint8)ELyraAbilityActivationGroup::MAX];
	
	
};




#undef UE_API