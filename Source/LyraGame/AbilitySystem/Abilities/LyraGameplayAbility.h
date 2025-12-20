// Copyright Epic Games, Inc. All Rights Reserved.
// 001 Not Finished!
#pragma once

#include "Abilities/GameplayAbility.h"

#include "LyraGameplayAbility.generated.h"

#define UE_API LYRAGAME_API

struct FGameplayAbilityActivationInfo;
struct FGameplayAbilitySpec;
struct FGameplayAbilitySpecHandle;

class AActor;
class AController;
class ALyraCharacter;
class ALyraPlayerController;
class APlayerController;
class FText;
class ILyraAbilitySourceInterface;
class UAnimMontage;
class ULyraAbilityCost;
class ULyraAbilitySystemComponent;
class ULyraCameraMode;
class ULyraHeroComponent;
class UObject;
struct FFrame;
struct FGameplayAbilityActorInfo;
struct FGameplayEffectSpec;
struct FGameplayEventData;


/**
 * ELyraAbilityActivationPolicy
 *
 *	Defines how an ability is meant to activate.
 *	定义了该能力应如何被激活的方式。
 */
UENUM(BlueprintType)
enum class ELyraAbilityActivationPolicy : uint8
{
	// Try to activate the ability when the input is triggered.
	// 在输入被触发时尝试激活该能力。
	OnInputTriggered,

	// Continually try to activate the ability while the input is active.
	// 在输入处于激活状态时，持续尝试激活该能力。
	WhileInputActive,

	// Try to activate the ability when an avatar is assigned.
	// 当为角色分配了形象时，尝试激活该能力。
	OnSpawn
};

/**
 * ELyraAbilityActivationGroup
 *
 *	Defines how an ability activates in relation to other abilities.
 * 定义了某一能力与其他能力激活方式之间的关系。
 *	
 */
UENUM(BlueprintType)
enum class ELyraAbilityActivationGroup : uint8
{
	// Ability runs independently of all other abilities.
	// 能力是独立于其他所有能力存在的。
	Independent,

	// Ability is canceled and replaced by other exclusive abilities.
	// 能力被取消，并被其他独特的能力所取代。
	Exclusive_Replaceable,

	// Ability blocks all other exclusive abilities from activating.
	// 该技能会阻止其他所有特殊能力的激活。
	Exclusive_Blocking,

	MAX	UMETA(Hidden)
};


/** Failure reason that can be used to play an animation montage when a failure occurs */
/** 可用于在出现故障时播放动画剪辑的故障原因 */
USTRUCT(BlueprintType)
struct FLyraAbilityMontageFailureMessage
{
	GENERATED_BODY()

public:
	// Player controller that failed to activate the ability, if the AbilitySystemComponent was player owned
	// 如果拥有能力系统的组件属于玩家所有，而玩家控制器未能激活该能力的话
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<APlayerController> PlayerController = nullptr;

	// Avatar actor that failed to activate the ability
	// 虚拟角色未能激活该能力
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> AvatarActor = nullptr;

	// All the reasons why this ability has failed
	// 这种能力未能发挥作用的所有原因
	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer FailureTags;

	// 失败可以用于播放的蒙太奇
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UAnimMontage> FailureMontage = nullptr;
};



/**
 * ULyraGameplayAbility
 *
 *	The base gameplay ability class used by this project.
 */
UCLASS(MinimalAPI, Abstract, HideCategories = Input, Meta = (ShortTooltip = "The base gameplay ability class used by this project."))
class ULyraGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

	

	


	
};



#undef UE_API
