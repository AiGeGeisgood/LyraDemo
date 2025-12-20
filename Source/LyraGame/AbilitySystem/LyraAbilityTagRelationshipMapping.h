// Copyright Epic Games, Inc. All Rights Reserved.
// Finished.
#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "LyraAbilityTagRelationshipMapping.generated.h"

class UObject;


/** Struct that defines the relationship between different ability tags */
/** 定义不同能力标签之间关系的结构体 */
USTRUCT()
struct FLyraAbilityTagRelationship
{
	GENERATED_BODY()

	/** The tag that this container relationship is about. Single tag, but abilities can have multiple of these */
	/** 此容器关系所涉及的标签。单个标签，但某些功能可以有多个此类标签 */
	UPROPERTY(EditAnywhere, Category = Ability, meta = (Categories = "Gameplay.Action"))
	FGameplayTag AbilityTag;

	/** The other ability tags that will be blocked by any ability using this tag */
	/** 任何使用此标签的能力都会屏蔽的其他能力标签 */
	UPROPERTY(EditAnywhere, Category = Ability)
	FGameplayTagContainer AbilityTagsToBlock;

	/** The other ability tags that will be canceled by any ability using this tag */
	/** 任何使用此标签的技能都会取消的其他能力标签 */
	UPROPERTY(EditAnywhere, Category = Ability)
	FGameplayTagContainer AbilityTagsToCancel;

	/** If an ability has the tag, this is implicitly added to the activation required tags of the ability */
	/** 如果某个能力带有特定标签，那么该标签会自动添加到该能力所需的激活条件标签列表中 */
	UPROPERTY(EditAnywhere, Category = Ability)
	FGameplayTagContainer ActivationRequiredTags;

	/** If an ability has the tag, this is implicitly added to the activation blocked tags of the ability */
	/** 如果某个能力带有该标签，则该标签会自动添加到该能力的激活受阻标签列表中 */
	UPROPERTY(EditAnywhere, Category = Ability)
	FGameplayTagContainer ActivationBlockedTags;

	
};

/** Mapping of how ability tags block or cancel other abilities */
/** 描述能力标签如何阻止或取消其他能力的映射关系 */
UCLASS()
class ULyraAbilityTagRelationshipMapping : public UDataAsset
{
	GENERATED_BODY()

private:
	/** The list of relationships between different gameplay tags (which ones block or cancel others) */
	/** 不同游戏玩法标签之间的关系列表（哪些标签会阻止或取消其他标签的作用） */
	UPROPERTY(EditAnywhere, Category = Ability, meta=(TitleProperty="AbilityTag"))
	TArray<FLyraAbilityTagRelationship> AbilityTagRelationships;

public:
	/** Given a set of ability tags, parse the tag relationship and fill out tags to block and cancel */
	/** 给定一组能力标签，解析标签之间的关系，并填写出用于阻止和取消的操作标签 */
	// AbilityTags会阻塞那些Tag
	// AbilityTags会取消那些Tag
	void GetAbilityTagsToBlockAndCancel(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer* OutTagsToBlock, FGameplayTagContainer* OutTagsToCancel) const;

	/** Given a set of ability tags, add additional required and blocking tags */
	/** 给定一组能力标签，添加额外的必填标签和限制标签 */
	// AbilityTags激活需要哪些tag
	// AbilityTags会被那些tag所阻塞
	void GetRequiredAndBlockedActivationTags(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer* OutActivationRequired, FGameplayTagContainer* OutActivationBlocked) const;

	/** Returns true if the specified ability tags are canceled by the passed in action tag */
	/** 如果指定的能力标签被传入的动作标签取消，则返回 true */
	// ActionTag是否会被AbilityTags所取消
	bool IsAbilityCancelledByTag(const FGameplayTagContainer& AbilityTags, const FGameplayTag& ActionTag) const;
	



	
};
