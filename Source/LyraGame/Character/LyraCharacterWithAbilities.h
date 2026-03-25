// Copyright Epic Games, Inc. All Rights Reserved.
// Finished.
#pragma once

#include "Character/LyraCharacter.h"

#include "LyraCharacterWithAbilities.generated.h"

#define UE_API LYRAGAME_API


// ALyraCharacter typically gets the ability system component from the possessing player state
// This represents a character with a self-contained ability system component.
// ALyraCharacter通常会从拥有者的玩家状态中获取能力系统组件
// 这表示该角色拥有独立的能力系统组件。
UCLASS(MinimalAPI, Blueprintable)
class ALyraCharacterWithAbilities : public ALyraCharacter
{
	GENERATED_BODY()

public:
	/*
	 * 构造函数
	 * 1.创建GAS组件,设置网络同步模式
	 * 2.创建生命值组件
	 * 3.创建战斗组件
	 * 4.设置更新频率
	 */
	UE_API ALyraCharacterWithAbilities(const FObjectInitializer& ObjectInitializer);

	// 初始化ASC
	UE_API virtual void PostInitializeComponents() override;
	// 获取ASC
	// UE_API virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	
private:

	// The ability system component sub-object used by player characters.
	// 用于玩家角色的“能力系统”子组件。
	// UPROPERTY(VisibleAnywhere, Category = "Lyra|PlayerState")
	// TObjectPtr<ULyraAbilitySystemComponent> AbilitySystemComponent;
	
	// Health attribute set used by this actor.
	// 该角色所使用的健康属性设定。
	// UPROPERTY()
	// TObjectPtr<const class ULyraHealthSet> HealthSet;
	// Combat attribute set used by this actor.
	// 此角色所使用的战斗属性设定。
	// UPROPERTY()
	// TObjectPtr<const class ULyraCombatSet> CombatSet;

};





#undef UE_API