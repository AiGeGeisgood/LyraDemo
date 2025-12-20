// Copyright Epic Games, Inc. All Rights Reserved.
// Finished.
#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "LyraInputConfig.generated.h"


class UInputAction;
class UObject;
struct FFrame;

/**
 * FLyraInputAction
 *
 *	Struct used to map a input action to a gameplay input tag.
 *  该结构体用于将输入操作映射到游戏玩法的输入标签上。
 */
USTRUCT(BlueprintType)
struct FLyraInputAction
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};



/**
 * ULyraInputConfig
 *
 *	Non-mutable data asset that contains input configuration properties.
 *	不可变的数据资产，其中包含输入配置属性。
 */
UCLASS(BlueprintType, Const)
class ULyraInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	// ULyraInputConfig(const FObjectInitializer& ObjectInitializer);

	// 找到基本输入的Tag所映射的输入资产
	// UFUNCTION(BlueprintCallable, Category = "Lyra|Pawn")
	// const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;
	//
	// // 找到技能输入的Tag所映射的输入资产
	// UFUNCTION(BlueprintCallable, Category = "Lyra|Pawn")
	// const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound = true) const;



	
public:
	// List of input actions used by the owner.  These input actions are mapped to a gameplay tag and must be manually bound.
	// 所有者使用的输入操作列表。这些输入操作会映射到游戏玩法标签上，并且必须手动进行绑定。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FLyraInputAction> NativeInputActions;

	// List of input actions used by the owner.  These input actions are mapped to a gameplay tag and are automatically bound to abilities with matching input tags.
	// 该玩家所使用的输入操作列表。这些输入操作被映射到游戏玩法标签上，并会自动与具有相同输入标签的技能绑定在一起。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
	TArray<FLyraInputAction> AbilityInputActions;
	
};