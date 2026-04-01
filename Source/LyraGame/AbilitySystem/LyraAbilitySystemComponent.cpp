// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraAbilitySystemComponent.h"

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"
#include "AbilitySystem/LyraAbilityTagRelationshipMapping.h"
#include "Animation/LyraAnimInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "LyraGlobalAbilitySystem.h"
#include "LyraLogChannels.h"
#include "System/LyraAssetManager.h"
#include "System/LyraGameData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraAbilitySystemComponent)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked");

ULyraAbilitySystemComponent::ULyraAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();

	FMemory::Memset(ActivationGroupCounts, 0, sizeof(ActivationGroupCounts));
}
//
// void ULyraAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
// {
// 	if (ULyraGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<ULyraGlobalAbilitySystem>(GetWorld()))
// 	{
// 		GlobalAbilitySystem->UnregisterASC(this);
// 	}
//
// 	Super::EndPlay(EndPlayReason);
// }
//
// void ULyraAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
// {
// 	/**
// 	 *	FGameplayAbilityActorInfo
// 	 * 与使用某种能力的演员相关联的缓存数据。
// 	 *  - 在 InitFromActor 方法中从一个 AActor* 初始化而来。
// 	 *  - 能力利用此信息来确定要作用于的演员对象。例如，而不是与特定的演员类绑定。
// 	 *  - 这些通常以指针的形式传递，以支持多态性。
// 	 *  - 项目可以重写 UAbilitySystemGlobals:：AllocAbilityActorInfo 来覆盖创建的默认结构类型。*
// 	*/
// 	/**
// 	 *  AbilityActorInfo
// 	 *  存储有关拥有者角色的相关缓存数据，这些数据是能力类频繁需要访问的（如移动组件、网格组件、动画实例等）
// 	 *  
// 	 */
// 	FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
// 	check(ActorInfo);
// 	check(InOwnerActor);
//
//
// 	// 是否是产生了替身的更替
// 	const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor);
//
// 	// 先执行父类的
// 	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
//
// 	// 因为有替身的切换, 所以我们项目自定义的ASC流程需要重新初始化
// 	if (bHasNewPawnAvatar)
// 	{
// 		/**
// 		 * ActivatableAbilities
// 		 * 我们能够激活的能力。
// 		 * - 这将包括非实例化能力的 CDO 以及每次执行时的实例化能力。
// 		 * - 角色实例化能力将是实际的实例（而非 CDO）*
// 		 * 这个数组并非是系统正常运行所必需的。它只是一个便于“赋予角色能力”的辅助功能。但能力也可以独立于“能力系统组件”发挥作用。
// 		 * 例如，可以编写一种能力，使其能够作用于“静态网格角色”。只要该能力不需要实例化或其他任何能力系统组件所能提供的功能，那么它就无需该组件就能正常运行。
// 		 * 
// 		 */
//
// 		// Notify all abilities that a new pawn avatar has been set
// 		// 通知所有能力，新的兵种形象已设定完成
// 		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
// 		{
// 			// 禁用过期警告
// 			PRAGMA_DISABLE_DEPRECATION_WARNINGS
// 			// 对于GA而言NonInstanced这种实例化类型已经过期了,请不要再使用
// 			ensureMsgf(
// 				AbilitySpec.Ability && AbilitySpec.Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::
// 				NonInstanced,
// 				TEXT(
// 					"InitAbilityActorInfo: All Abilities should be Instanced (NonInstanced is being deprecated due to usability issues)."
// 				));
// 			PRAGMA_ENABLE_DEPRECATION_WARNINGS
//
// 			TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
// 			for (UGameplayAbility* AbilityInstance : Instances)
// 			{
// 				ULyraGameplayAbility* LyraAbilityInstance = Cast<ULyraGameplayAbility>(AbilityInstance);
//
// 				if (LyraAbilityInstance)
// 				{
// 					// Ability instances may be missing for replays
// 					// 回放中可能存在能力实例缺失的情况
// 					LyraAbilityInstance->OnPawnAvatarSet();
// 				}
// 			}
// 		}
// 		// Register with the global system once we actually have a pawn avatar. We wait until this time since some globally-applied effects may require an avatar.
// 		// 当我们真正拥有角色形象时，便向全局系统进行注册。我们在此时才进行注册，因为某些全局生效的效果可能需要角色形象的存在。
// 		if (ULyraGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<ULyraGlobalAbilitySystem>(GetWorld()))
// 		{
// 			GlobalAbilitySystem->RegisterASC(this);
// 		}
//
// 		// 重新初始化动画蓝图 因为我们绑定的ASC变了 所以绑定的Tag来源变了
// 		if (ULyraAnimInstance* LyraAnimInst = Cast<ULyraAnimInstance>(ActorInfo->GetAnimInstance()))
// 		{
// 			LyraAnimInst->InitializeWithAbilitySystem(this);
// 		}
//
// 		// 激活那些pawn生成是需要激活的能力
// 		TryActivateAbilitiesOnSpawn();
// 	}
// }
//
// void ULyraAbilitySystemComponent::CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc,
//                                                         bool bReplicateCancelAbility)
// {
// 	ABILITYLIST_SCOPE_LOCK();
// 	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
// 	{
// 		// 该能力是非激活状态 可以跳过
// 		if (!AbilitySpec.IsActive())
// 		{
// 			continue;
// 		}
//
// 		// 没有授予的能力 跳过
// 		ULyraGameplayAbility* LyraAbilityCDO = Cast<ULyraGameplayAbility>(AbilitySpec.Ability);
// 		if (!LyraAbilityCDO)
// 		{
// 			UE_LOG(LogLyraAbilitySystem, Error,
// 			       TEXT("CancelAbilitiesByFunc: Non-LyraGameplayAbility %s was Granted to ASC. Skipping."),
// 			       *AbilitySpec.Ability.GetName());
// 			continue;
// 		}
//
// 		// 过期的能力非实例化策略 可以跳过
// 		PRAGMA_DISABLE_DEPRECATION_WARNINGS
// 		ensureMsgf(AbilitySpec.Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced,
// 		           TEXT(
// 			           "CancelAbilitiesByFunc: All Abilities should be Instanced (NonInstanced is being deprecated due to usability issues)."
// 		           ));
// 		PRAGMA_ENABLE_DEPRECATION_WARNINGS
//
// 		// Cancel all the spawned instances.
// 		// 取消所有生成的实例。
// 		TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
//
// 		for (UGameplayAbility* AbilityInstance : Instances)
// 		{
// 			ULyraGameplayAbility* LyraAbilityInstance = CastChecked<ULyraGameplayAbility>(AbilityInstance);
//
//
// 			// 根据传入的lambda是否需要取消
// 			if (ShouldCancelFunc(LyraAbilityInstance, AbilitySpec.Handle))
// 			{
// 				// 是否可以取消
// 				if (LyraAbilityInstance->CanBeCanceled())
// 				{
// 					/** 销毁按执行次序生成的实例化能力。每个角色的实例化能力应“重置”。任何处于激活状态的能力状态任务都会接收到“能力状态中断”事件。非实例化能力——我们该怎么办？*/
// 					LyraAbilityInstance->CancelAbility(AbilitySpec.Handle,
// 					                                   AbilityActorInfo.Get(),
// 					                                   LyraAbilityInstance->GetCurrentActivationInfo(),
// 					                                   bReplicateCancelAbility);
// 				}
// 				else
// 				{
// 					// 不可被取消,但是需要取消,报错
// 					UE_LOG(LogLyraAbilitySystem, Error,
// 					       TEXT("CancelAbilitiesByFunc: Can't cancel ability [%s] because CanBeCanceled is false."),
// 					       *LyraAbilityInstance->GetName());
// 				}
// 			}
// 		}
// 	}
// }
//
// void ULyraAbilitySystemComponent::CancelInputActivatedAbilities(bool bReplicateCancelAbility)
// {
// 	auto ShouldCancelFunc = [this](const ULyraGameplayAbility* LyraAbility, FGameplayAbilitySpecHandle Handle)
// 	{
// 		// 判断当前能力的激活策略
// 		const ELyraAbilityActivationPolicy ActivationPolicy = LyraAbility->GetActivationPolicy();
// 		return ((ActivationPolicy == ELyraAbilityActivationPolicy::OnInputTriggered) || (ActivationPolicy == ELyraAbilityActivationPolicy::WhileInputActive));
// 	};
//
// 	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
// }
//
// void ULyraAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
// {
// 	if (InputTag.IsValid())
// 	{
// 		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
// 		{
// 			// 注意这个GetDynamicSpecSourceTags,这个是在注册能力的时候需要传入的
// 			if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
// 			{
// 				InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
// 				InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
// 			}
// 		}
// 	}
// }
//
// void ULyraAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
// {
// 	if (InputTag.IsValid())
// 	{
// 		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
// 		{
// 			// 这个Tag很重要GetDynamicSpecSourceTags.需要在能力注册时就给它注册进去
// 			if (AbilitySpec.Ability && (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag)))
// 			{
// 				InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
// 				InputHeldSpecHandles.Remove(AbilitySpec.Handle);
// 			}
// 		}
// 	}
// }
//
// void ULyraAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
// {
// 	// 是否有阻塞能力输入的tag
// 	if (HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked))
// 	{
// 		ClearAbilityInput();
// 		return;
// 	}
//
// 	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
// 	AbilitiesToActivate.Reset();
//
// 	//@TODO: See if we can use FScopedServerAbilityRPCBatcher ScopedRPCBatcher in some of these loops
// 	// 待办事项：查看是否可以在这些循环中使用 FScopedServerAbilityRPCBatcher ScopedRPCBatcher
//
//
// 	//
// 	// Process all abilities that activate when the input is held.
// 	// 处理所有在输入保持按住状态时激活的能力。
// 	//
// 	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
// 	{
// 		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
// 		{
// 			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
// 			{
// 				const ULyraGameplayAbility* LyraAbilityCDO = Cast<ULyraGameplayAbility>(AbilitySpec->Ability);
// 				if (LyraAbilityCDO && LyraAbilityCDO->GetActivationPolicy() ==
// 					ELyraAbilityActivationPolicy::WhileInputActive)
// 				{
// 					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
// 				}
// 			}
// 		}
// 	}
//
// 	//
// 	// Process all abilities that had their input pressed this frame.
// 	// 处理本帧中输入被按下的所有能力。
// 	//
//
// 	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
// 	{
// 		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
// 		{
// 			if (AbilitySpec->Ability)
// 			{
// 				AbilitySpec->InputPressed = true;
//
// 				if (AbilitySpec->IsActive())
// 				{
// 					// Ability is active so pass along the input event.
// 					// 能力已激活，因此将输入事件传递出去。
// 					// 这里很重要 如果没有正确执行的话 在GA_Hero_Jump里面无法正确的监听到Wait input press/Wait input Release这俩个节点
// 					AbilitySpecInputPressed(*AbilitySpec);
// 				}
// 				else
// 				{
// 					// 能力还没有激活,所以等下就去激活
//
// 					const ULyraGameplayAbility* LyraAbilityCDO = Cast<ULyraGameplayAbility>(AbilitySpec->Ability);
//
// 					if (LyraAbilityCDO && LyraAbilityCDO->GetActivationPolicy() ==
// 						ELyraAbilityActivationPolicy::OnInputTriggered)
// 					{
// 						AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
// 					}
// 				}
// 			}
// 		}
// 	}
//
// 	//
// 	// Try to activate all the abilities that are from presses and holds.
// 	// We do it all at once so that held inputs don't activate the ability
// 	// and then also send a input event to the ability because of the press.
// 	//
//
// 	//
// 	// 尝试激活所有通过按压和保持操作而获得的技能。
// 	// 我们一次性完成所有操作，这样保持状态下的输入就不会激活该技能，
// 	// 同时还会因为按压操作而向该技能发送输入事件。
// 	// //
// 	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
// 	{
// 		TryActivateAbility(AbilitySpecHandle);
// 	}
//
// 	//
// 	// Process all abilities that had their input released this frame.
// 	// 处理本帧中已释放输入的全部能力。
// 	//
// 	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
// 	{
// 		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
// 		{
// 			if (AbilitySpec->Ability)
// 			{
// 				AbilitySpec->InputPressed = false;
//
// 				if (AbilitySpec->IsActive())
// 				{
// 					// Ability is active so pass along the input event.
// 					// 能力已激活，因此将输入事件传递出去。
// 					// 这里很重要 如果没有正确执行的话 在GA_Hero_Jump里面无法正确的监听到Wait input press/Wait input Release这俩个节点
// 					AbilitySpecInputReleased(*AbilitySpec);
// 				}
// 			}
// 		}
// 	}
// 	//
// 	// Clear the cached ability handles.
// 	// 清除缓存中的能力句柄。
// 	//
// 	InputPressedSpecHandles.Reset();
// 	InputReleasedSpecHandles.Reset();
//
//
// 	// 注意这里不需要管InputHeldSpecHandles,它是由按键释放的时候去容器中进行移除.
// 	// 这三个容器名字很像不要搞混了!!!
// }
//
// void ULyraAbilitySystemComponent::ClearAbilityInput()
// {
// 	InputPressedSpecHandles.Reset();
// 	InputReleasedSpecHandles.Reset();
// 	InputHeldSpecHandles.Reset();
// }
//
// bool ULyraAbilitySystemComponent::IsActivationGroupBlocked(ELyraAbilityActivationGroup Group) const
// {
// 	bool bBlocked = false;
//
// 	switch (Group)
// 	{
// 	case ELyraAbilityActivationGroup::Independent:
// 		// Independent abilities are never blocked.
// 		// 独立的能力永远不会受到阻碍。
// 		bBlocked = false;
// 		break;
//
// 	case ELyraAbilityActivationGroup::Exclusive_Replaceable:
// 	case ELyraAbilityActivationGroup::Exclusive_Blocking:
// 		// Exclusive abilities can activate if nothing is blocking.
// 		// 若无任何阻碍物阻挡，则专属技能即可激活。
// 		// 意思是当前只要没有阻塞的能力在生效就可以释放该能力.
// 		bBlocked = (ActivationGroupCounts[(uint8)ELyraAbilityActivationGroup::Exclusive_Blocking] > 0);
// 		break;
//
// 	default:
// 		checkf(false, TEXT("IsActivationGroupBlocked: Invalid ActivationGroup [%d]\n"), (uint8)Group);
// 		break;	
// 		
// 		
// 	}
// 	
// 	return bBlocked;
// }
//
// void ULyraAbilitySystemComponent::AddAbilityToActivationGroup(ELyraAbilityActivationGroup Group,
// 	ULyraGameplayAbility* LyraAbility)
// {
// 	// 合法性判定
// 	check(LyraAbility);
// 	check(ActivationGroupCounts[(uint8)Group] < INT32_MAX);
//
// 	// 该能力类别的计数增加
// 	ActivationGroupCounts[(uint8)Group]++;
//
// 	const bool bReplicateCancelAbility = false;
//
//
// 	switch (Group)
// 	{
// 	case ELyraAbilityActivationGroup::Independent:
// 		// Independent abilities do not cancel any other abilities.
// 		// 独立能力不会抵消任何其他能力。
// 		break;
//
// 	case ELyraAbilityActivationGroup::Exclusive_Replaceable:
// 	case ELyraAbilityActivationGroup::Exclusive_Blocking:
// 		// 如果当前的能力具有类别的替换或者阻塞 则需要去关闭之前处于对应类别的能力
// 		// 因为不可能是阻塞,因为是阻塞根本就没办法激活,所以只可能是取消类别,把之前的这个可替换的能力取消即可
// 		CancelActivationGroupAbilities(ELyraAbilityActivationGroup::Exclusive_Replaceable, LyraAbility, bReplicateCancelAbility);
// 		break;
//
//
// 	default:
// 		checkf(false, TEXT("AddAbilityToActivationGroup: Invalid ActivationGroup [%d]\n"), (uint8)Group);
// 		break;		
//
//
// 	}
// 	
// 	// 阻塞和可替换 他们合计不能超过1!!!!!
// 	const int32 ExclusiveCount = ActivationGroupCounts[(uint8)ELyraAbilityActivationGroup::Exclusive_Replaceable] + ActivationGroupCounts[(uint8)ELyraAbilityActivationGroup::Exclusive_Blocking];
// 	if (!ensure(ExclusiveCount <= 1))
// 	{
// 		UE_LOG(LogLyraAbilitySystem, Error, TEXT("AddAbilityToActivationGroup: Multiple exclusive abilities are running."));
// 	}
// 	
// }
//
// void ULyraAbilitySystemComponent::RemoveAbilityFromActivationGroup(ELyraAbilityActivationGroup Group,
// 	ULyraGameplayAbility* LyraAbility)
// {
// 	check(LyraAbility);
// 	check(ActivationGroupCounts[(uint8)Group] > 0);
// 	// 计数减少即可
// 	ActivationGroupCounts[(uint8)Group]--;
// 	
// }
//
// void ULyraAbilitySystemComponent::CancelActivationGroupAbilities(ELyraAbilityActivationGroup Group,
// 	ULyraGameplayAbility* IgnoreLyraAbility, bool bReplicateCancelAbility)
// {
// 	auto ShouldCancelFunc = [this, Group, IgnoreLyraAbility](const ULyraGameplayAbility* LyraAbility, FGameplayAbilitySpecHandle Handle)
// 	{
// 		return ((LyraAbility->GetActivationGroup() == Group) && (LyraAbility != IgnoreLyraAbility));
// 	};
//
// 	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
// 	
// }
//
// void ULyraAbilitySystemComponent::AddDynamicTagGameplayEffect(const FGameplayTag& Tag)
// {
// 	// 这里需要读取我们全局使用的默认GE类别
// 	const TSubclassOf<UGameplayEffect> DynamicTagGE = ULyraAssetManager::GetSubclass(ULyraGameData::Get().DynamicTagGameplayEffect);
//
// 	if (!DynamicTagGE)
// 	{
// 		UE_LOG(LogLyraAbilitySystem, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to find DynamicTagGameplayEffect [%s]."),
// 			*ULyraGameData::Get().DynamicTagGameplayEffect.GetAssetName());
// 		return;
// 	}
// 	/** 获取一个可直接应用于其他对象的输出游戏效果规格。*/
// 	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(DynamicTagGE, 1.0f, MakeEffectContext());
// 	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
//
// 	if (!Spec)
// 	{
// 		UE_LOG(LogLyraAbilitySystem, Warning, TEXT("AddDynamicTagGameplayEffect: Unable to make outgoing spec for [%s]."), *GetNameSafe(DynamicTagGE));
// 		return;
// 	}
// 	
// 	/** DynamicGrantedTags 所授予的、且并非源自 UGameplayEffect 定义的标签。这些标签会进行复制。*/
// 	Spec->DynamicGrantedTags.AddTag(Tag);
//
// 	/** 将之前创建的游戏玩法效果规格应用于此组件 */
// 	ApplyGameplayEffectSpecToSelf(*Spec);
// 	
// 	
// }
//
// void ULyraAbilitySystemComponent::RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag)
// {
// 	const TSubclassOf<UGameplayEffect> DynamicTagGE = ULyraAssetManager::GetSubclass(ULyraGameData::Get().DynamicTagGameplayEffect);
// 	if (!DynamicTagGE)
// 	{
// 		UE_LOG(LogLyraAbilitySystem, Warning, TEXT("RemoveDynamicTagGameplayEffect: Unable to find gameplay effect [%s]."), *ULyraGameData::Get().DynamicTagGameplayEffect.GetAssetName());
// 		return;
// 	}
// 	/** 创建一个效果查询，该查询将根据给定的标签与活跃游戏效果所属标签之间的共同标签情况进行匹配 */
// 	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));
// 	Query.EffectDefinition = DynamicTagGE;
//
// 	RemoveActiveEffects(Query);
// }
//
// void ULyraAbilitySystemComponent::GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle,
// 	FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle)
// {
//
// 	/** AbilityTargetDataMap 将能力规格映射到目标数据。用于跟踪复制数据和回调操作 */
// 	TSharedPtr<FAbilityReplicatedDataCache> ReplicatedData =
// 		AbilityTargetDataMap.Find(FGameplayAbilitySpecHandleAndPredictionKey(AbilityHandle, ActivationInfo.GetActivationPredictionKey()));
// 	if (ReplicatedData.IsValid())
// 	{
// 		OutTargetDataHandle = ReplicatedData->TargetData;
// 	}
// }
//
//
// void ULyraAbilitySystemComponent::SetTagRelationshipMapping(ULyraAbilityTagRelationshipMapping* NewMapping)
// {
// 	TagRelationshipMapping = NewMapping;
// }
//
// void ULyraAbilitySystemComponent::GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags,
// 	FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const
// {
// 	if (TagRelationshipMapping)
// 	{
// 		TagRelationshipMapping->GetRequiredAndBlockedActivationTags(AbilityTags, &OutActivationRequired, &OutActivationBlocked);
// 	}
// }
//
// void ULyraAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
// {
// 	/** 用于防止我们在遍历能力组件中的能力时，意外删除该组件中的能力 */
// 	ABILITYLIST_SCOPE_LOCK();
//
// 	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
// 	{
// 		if (const ULyraGameplayAbility* LyraAbilityCDO = Cast<ULyraGameplayAbility>(AbilitySpec.Ability))
// 		{
// 			LyraAbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
// 		}
// 	}
// }
//
// void ULyraAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
// {
// 	Super::AbilitySpecInputPressed(Spec);
//
// 	// We don't support UGameplayAbility::bReplicateInputDirectly.
// 	// Use replicated events instead so that the WaitInputPress ability task works.
// 	// 我们不支持 UGameplayAbility::bReplicateInputDirectly 这个特性。
// 	// 请改用复制事件的方式，以便“等待输入按下”能力任务能够正常运行。
// 	if (Spec.IsActive())
// 	{
// 		PRAGMA_DISABLE_DEPRECATION_WARNINGS
// 		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
// 		FPredictionKey OriginalPredictionKey = Instance
// 			                                       ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey()
// 			                                       : Spec.ActivationInfo.GetActivationPredictionKey();
// 		PRAGMA_ENABLE_DEPRECATION_WARNINGS
//
// 		// Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
// 		// 调用“输入按下”事件。此操作在此处未作详细说明。若有人在监听，则可将“输入按下”事件发送至服务器。
// 		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, OriginalPredictionKey);
// 	}
// }
//
// void ULyraAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
// {
// 	Super::AbilitySpecInputReleased(Spec);
//
// 	// We don't support UGameplayAbility::bReplicateInputDirectly.
// 	// Use replicated events instead so that the WaitInputRelease ability task works.
// 	// 我们不支持 UGameplayAbility::bReplicateInputDirectly 这个特性。
// 	// 请改用复制事件的方式，以便“等待输入释放”能力任务能够正常运行。
// 	if (Spec.IsActive())
// 	{
// 		PRAGMA_DISABLE_DEPRECATION_WARNINGS
// 		const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
// 		FPredictionKey OriginalPredictionKey = Instance
// 			                                       ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey()
// 			                                       : Spec.ActivationInfo.GetActivationPredictionKey();
// 		PRAGMA_ENABLE_DEPRECATION_WARNINGS
//
// 		// Invoke the InputReleased event. This is not replicated here. If someone is listening, they may replicate the InputReleased event to the server.
// 		// 调用“输入释放”事件。此操作在此处未作详细说明。若有人在监听，则可将“输入释放”事件发送至服务器。
// 		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, OriginalPredictionKey);
// 	}
// }
//
// void ULyraAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle,
// 	UGameplayAbility* Ability)
// {
// 	Super::NotifyAbilityActivated(Handle, Ability);
//
// 	if (ULyraGameplayAbility* LyraAbility = Cast<ULyraGameplayAbility>(Ability))
// 	{
// 		AddAbilityToActivationGroup(LyraAbility->GetActivationGroup(), LyraAbility);
// 	}
// }
//
// void ULyraAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle,
// 	UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
// {
// 	Super::NotifyAbilityFailed(Handle, Ability, FailureReason);
//
// 	if (APawn* Avatar = Cast<APawn>(GetAvatarActor()))
// 	{
// 		/**
// 		 *IsSupportedForNetworking
// 		 *	我们只能复制以下内容的引用：
// 		 *	- CDO 和数据资产（例如，静态、非实例化的游戏能力）
// 		 *	- 正在实例化的且将被在客户端创建的那些能力。*
// 		  *	其他情况下则不予以支持，而是会在客户端重新生成。
// 		 *	
// 		 */
// 		// 如果这个Pawn不是本地控制的,但是这个能力又是需要网络的.那么我们就手动一个RPC通知出去
// 		if (!Avatar->IsLocallyControlled() && Ability->IsSupportedForNetworking())
// 		{
// 			ClientNotifyAbilityFailed(Ability, FailureReason);
// 			return;
// 		}
// 	}
// 	
// 	HandleAbilityFailed(Ability, FailureReason);
// }
//
// void ULyraAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability,
// 	bool bWasCancelled)
// {
// 	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);
//
// 	if (ULyraGameplayAbility* LyraAbility = Cast<ULyraGameplayAbility>(Ability))
// 	{
// 		RemoveAbilityFromActivationGroup(LyraAbility->GetActivationGroup(), LyraAbility);
// 	}
// 	
// }
//
// void ULyraAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags,
// 	UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags,
// 	bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags)
// {
// 	FGameplayTagContainer ModifiedBlockTags = BlockTags;
// 	FGameplayTagContainer ModifiedCancelTags = CancelTags;
//
// 	if (TagRelationshipMapping)
// 	{
// 		// Use the mapping to expand the ability tags into block and cancel tag
// 		// 利用此映射将能力标签扩展为块状标签并取消标签显示
// 		TagRelationshipMapping->GetAbilityTagsToBlockAndCancel(AbilityTags, &ModifiedBlockTags, &ModifiedCancelTags);
// 	}
//
// 	Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags, ModifiedBlockTags, bExecuteCancelTags, ModifiedCancelTags);
//
// 	//@TODO: Apply any special logic like blocking input or movement
// 	//@注意事项：应用任何特殊的逻辑，例如阻止输入或移动操作
// 	
// }
//
// void ULyraAbilitySystemComponent::HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags,
// 	UGameplayAbility* RequestingAbility, bool bCanBeCanceled)
// {
// 	Super::HandleChangeAbilityCanBeCanceled(AbilityTags, RequestingAbility, bCanBeCanceled);
//
// 	//@TODO: Apply any special logic like blocking input or movement
// 	//@注意事项：应用任何特殊的逻辑，例如阻止输入或移动操作
// }
//
// void ULyraAbilitySystemComponent::ClientNotifyAbilityFailed_Implementation(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
// {
// 	HandleAbilityFailed(Ability, FailureReason);
// }
//
// void ULyraAbilitySystemComponent::HandleAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
// {
// 	//UE_LOG(LogLyraAbilitySystem, Warning, TEXT("Ability %s failed to activate (tags: %s)"), *GetPathNameSafe(Ability), *FailureReason.ToString());
//
// 	// 转发给GA处理
// 	if (const ULyraGameplayAbility* LyraAbility = Cast<const ULyraGameplayAbility>(Ability))
// 	{
// 		LyraAbility->OnAbilityFailedToActivate(FailureReason);
// 	}	
// }



