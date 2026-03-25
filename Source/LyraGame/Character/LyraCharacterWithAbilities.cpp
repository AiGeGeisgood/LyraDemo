// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraCharacterWithAbilities.h"


#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Async/TaskGraphInterfaces.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraCharacterWithAbilities)



ALyraCharacterWithAbilities::ALyraCharacterWithAbilities(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// 创建ASC
	// AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<ULyraAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	// AbilitySystemComponent->SetIsReplicated(true);
	// AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	

	// These attribute sets will be detected by AbilitySystemComponent::InitializeComponent. Keeping a reference so that the sets don't get garbage collected before that.
	// 这些属性集将由 AbilitySystemComponent::InitializeComponent 方法进行检测。为此保留一个引用，以确保这些属性集在该方法执行之前不会被垃圾回收。
	// HealthSet = CreateDefaultSubobject<ULyraHealthSet>(TEXT("HealthSet"));
	// CombatSet = CreateDefaultSubobject<ULyraCombatSet>(TEXT("CombatSet"));

	// AbilitySystemComponent needs to be updated at a high frequency.
	// 能力系统组件需要以较高的频率进行更新。
	SetNetUpdateFrequency(100.0f);
	

	
}

void ALyraCharacterWithAbilities::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// check(AbilitySystemComponent);
	// 初始化能力演员信息
	// AbilitySystemComponent->InitAbilityActorInfo(this, this);
}


// UAbilitySystemComponent* ALyraCharacterWithAbilities::GetAbilitySystemComponent() const
// {
// 	// return AbilitySystemComponent;
// 	return nullptr;
// }

