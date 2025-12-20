// Copyright Epic Games, Inc. All Rights Reserved.
// Finished.
#pragma once

#include "AttributeSet.h"

#include "LyraAttributeSet.generated.h"


#define UE_API LYRAGAME_API

class AActor;
class ULyraAbilitySystemComponent;
class UObject;
class UWorld;
struct FGameplayEffectSpec;

/**
 * This macro defines a set of helper functions for accessing and initializing attributes.
 *
 * The following example of the macro:
 *		ATTRIBUTE_ACCESSORS(ULyraHealthSet, Health)
 * will create the following functions:
 *		static FGameplayAttribute GetHealthAttribute();
 *		float GetHealth() const;
 *		void SetHealth(float NewVal);
 *		void InitHealth(float NewVal);
 */
/**
 * 此宏定义了一组用于访问和初始化属性的辅助函数。以下这个宏的示例：
 *		ATTRIBUTE_ACCESSORS(ULyraHealthSet, Health)
 * 将会生成以下函数：
 *		静态函数 FGameplayAttribute GetHealthAttribute()；
 *		函数 float GetHealth() ；（该函数为常量函数）
 *		函数 void SetHealth(float NewVal) ；
 *		函数 void InitHealth(float NewVal) ；
 *		
 */
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/** 
 * Delegate used to broadcast attribute events, some of these parameters may be null on clients: 
 * @param EffectInstigator	The original instigating actor for this event
 * @param EffectCauser		The physical actor that caused the change
 * @param EffectSpec		The full effect spec for this change
 * @param EffectMagnitude	The raw magnitude, this is before clamping
 * @param OldValue			The value of the attribute before it was changed
 * @param NewValue			The value after it was changed
*/
/**
 * 委托过去会广播属性事件，其中一些参数在客户端可能是空值：
 * @参数 作用源：此事件的原始触发者
 * @参数 作用原因：导致此变化的实际执行者
 * @参数 效果规格：此变化的完整效果规格
 * @参数 效果强度：原始强度值，即未进行限制之前
 * @参数 原始值：属性更改之前的状态值
 * @参数 新值：属性更改之后的状态值
 * 
 */
DECLARE_MULTICAST_DELEGATE_SixParams(FLyraAttributeEvent,
	AActor* /*EffectInstigator*/,
	AActor* /*EffectCauser*/,
	const FGameplayEffectSpec* /*EffectSpec*/,
	float /*EffectMagnitude*/,
	float /*OldValue*/,
	float /*NewValue*/);


/**
 * ULyraAttributeSet
 *
 *	Base attribute set class for the project.
 * 该项目的基础属性集类。
 *	
 */
UCLASS(MinimalAPI)
class ULyraAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	// UE_API ULyraAttributeSet();

	// UE_API UWorld* GetWorld() const override;
	//
	// UE_API ULyraAbilitySystemComponent* GetLyraAbilitySystemComponent() const;
};






#undef UE_API
