// Copyright Epic Games, Inc. All Rights Reserved.
// Finished.
// 001 Not Finihed . 只写了构造函数.
// 002 代码基本完成,但是涉及到死亡流程和生命值组件的内容需要后续章节再来讲解
#pragma once

#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "ModularCharacter.h"

#include "Teams/LyraTeamAgentInterface.h"


#include "LyraCharacter.generated.h"


#define UE_API LYRAGAME_API


class AActor;
class AController;
class ALyraPlayerController;
class ALyraPlayerState;
class FLifetimeProperty;
class IRepChangedPropertyTracker;
class UAbilitySystemComponent;
class UInputComponent;
class ULyraAbilitySystemComponent;
class ULyraCameraComponent;
class ULyraHealthComponent;
class ULyraPawnExtensionComponent;
class UObject;
struct FFrame;
struct FGameplayTag;
struct FGameplayTagContainer;

class USpringArmComponent;

/**
 *
 * FLyraReplicatedAcceleration: Compressed representation of acceleration
 * 加速度的压缩表示
 * 
 */
USTRUCT()
struct FLyraReplicatedAcceleration
{
	GENERATED_BODY()

	// X-Y 加速度分量的方向，已量化处理以表示范围[0, 2π]
	UPROPERTY()
	uint8 AccelXYRadians = 0;	// Direction of XY accel component, quantized to represent [0, 2*pi]

	// XY 分量的加速度率，已量化处理以表示范围在 [0， 最大加速度] 之间。
	UPROPERTY()
	uint8 AccelXYMagnitude = 0;	//Accel rate of XY component, quantized to represent [0, MaxAcceleration]

	// 原始的加速度率分量，经过量化处理以表示范围为[-最大加速度值，最大加速度值]的数值。
	UPROPERTY()
	int8 AccelZ = 0;	// Raw Z accel rate component, quantized to represent [-MaxAcceleration, MaxAcceleration]
};

/** The type we use to send FastShared movement updates. */
/** 我们用于发送快速共享移动更新的类型。*/
USTRUCT()
struct FSharedRepMovement
{
	GENERATED_BODY()

	FSharedRepMovement();

	// 从角色填充RepMovement
	bool FillForCharacter(ACharacter* Character);
	// 判断当前同步数据是否与人物的数据相等
	bool Equals(const FSharedRepMovement& Other, ACharacter* Character) const;

	// 填充数据
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	// 我们“根组件”的重复运动数据。该结构用于高效复制，因为速度和位置通常会一起复制（这可以节省一个复制索引），而速度.Z 值通常为零（大多数位置复制是针对行走的兵类角色的）。
	UPROPERTY(Transient)
	FRepMovement RepMovement;

	UPROPERTY(Transient)
	float RepTimeStamp = 0.0f;

	UPROPERTY(Transient)
	uint8 RepMovementMode = 0;

	UPROPERTY(Transient)
	bool bProxyIsJumpForceApplied = false;

	UPROPERTY(Transient)
	bool bIsCrouched = false;
};


// 参考Class.h里面中关于此结构体的描述
// 在FastArray中同步标签时,我们之前使用了WithNetDeltaSerializer -结构体具有一个名为“NetDeltaSerialize”的函数，用于将之前“NetSerialize”操作中状态的差异进行序列化处理。
template<>
struct TStructOpsTypeTraits<FSharedRepMovement> : public TStructOpsTypeTraitsBase2<FSharedRepMovement>
{
	enum
	{
		// 该结构体具有一个“NetSerialize”函数，用于将其状态序列化为用于网络复制的“FArchive”对象。
		WithNetSerializer = true,
		// 结构体有一个名为“NetSerialize”的函数，该函数在进行状态序列化时无需使用包映射。
		WithNetSharedSerialization = true,
	};
};




/**
 * ALyraCharacter
 *
 *	The base character pawn class used by this project.
 *	Responsible for sending events to pawn components.
 *	New behavior should be added via pawn components when possible.
 *
 * 本项目所使用的基础角色兵种类。
 * 负责向兵种组件发送事件。
 * 在可能的情况下，应通过兵种组件添加新行为。
 *	
 */
UCLASS(MinimalAPI, Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))
class ALyraCharacter : public AModularCharacter//,
// public IAbilitySystemInterface,
// public IGameplayCueInterface,
// public IGameplayTagAssetInterface,
// public ILyraTeamAgentInterface
{
	GENERATED_BODY()

public:
	/*
	 * 构造函数
	 * 1.开启Tick
	 * 2.修改Mesh的Transform
	 * 3.修改移动组件参数
	 * 4.创建PawnExtComp管理组件拓展器
	 * 5.创建生命值组件
	 * 6.创建相机组件
	 *  
	 */
	UE_API ALyraCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//获取玩家控制器
	UFUNCTION(BlueprintCallable, Category = "Lyra|Character")
	UE_API ALyraPlayerController* GetLyraPlayerController() const ;

	//获取玩家状态类
	UFUNCTION(BlueprintCallable, Category = "Lyra|Character")
	UE_API ALyraPlayerState* GetLyraPlayerState() const;

	// 获取Lyra的ASC组件
	// UFUNCTION(BlueprintCallable, Category = "Lyra|Character")
	// UE_API ULyraAbilitySystemComponent* GetLyraAbilitySystemComponent() const;
	// 通过角色拓展组件获取ASC组件
	// 注意ASC的组件可能有多个来源,本项目中关于人物主要来自于PlayerState,无论是AI还是玩家
	// 为了组件的初始化时序方便管理,所以都是从角色拓展组件获取
	// UE_API virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	
	// 走ASC 匹配Tag容器
	// UE_API virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	// UE_API virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	// UE_API virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	// UE_API virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	
	
	
	// 切换蹲伏 由英雄组件接收输入系统进行出发
	// UE_API void ToggleCrouch();

	//~AActor interface
	// 无作用
	// UE_API virtual void PreInitializeComponents() override;

	// 无作用 这里添加了重要度管理器的注册 未实现
	// 需要在DefaultEngine.ini中配置
	// [/Script/SignificanceManager.SignificanceManager]
	// SignificanceManagerClassName=/Script/LyraGame.LyraSignificanceManager
	// UE_API virtual void BeginPlay() override;
	// 无作用 这里移除了重要度管理器的注册
	// UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/*
	 * 将角色重置至初始状态 - 用于在不重新加载的情况下重新开始关卡时使用。
	 * 关闭移动和碰撞进行销毁.
	 */
	// UE_API virtual void Reset() override;

	
	
	// UE_API virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// 在复制操作即将发生时调用该函数。仅在服务器端调用，若正在录制客户端回放，则还会针对自主代理进行调用。
	// 用于计算加速度 然后同步这个属性
	// UE_API virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	
	//~End of AActor interface

	//~APawn interface
	// 控制器变更了 所以要重新获取队伍值 并将其改变传递出去
	// UE_API virtual void NotifyControllerChanged() override;
	//~End of APawn interface


	//~ILyraTeamAgentInterface interface
	// 修改队伍ID 理论上不能直接修改 需要接收所属控制器的变动 但是需要考虑没有控制器以及非服务器上的错误反馈
	// UE_API virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	// // 获取队伍ID
	// UE_API virtual FGenericTeamId GetGenericTeamId() const override;
	// // 获取队伍绑定的代理 用于向更下级传递
	// UE_API virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	//~End of ILyraTeamAgentInterface interface
	
	/** RPCs that is called on frames when default property replication is skipped. This replicates a single movement update to everyone. */
	/** 当默认属性复制被跳过时，在帧上调用的远程过程调用。此过程会将单次移动更新复制给所有用户。*/
	// UFUNCTION(NetMulticast, unreliable)
	// UE_API void FastSharedReplication(const FSharedRepMovement& SharedRepMovement);

	// Last FSharedRepMovement we sent, to avoid sending repeatedly.
	// 我们上一次发送的共享移动数据，目的是避免重复发送。
	FSharedRepMovement LastSharedReplication;

	// 由LyraReplicationGraph调用,更新同步的移动信息
	// UE_API virtual bool UpdateSharedReplication();

protected:

	// 由角色拓展组件的代理调用,ASC初始化后触发,更新生命值组件和Tag接口
	// UE_API virtual void OnAbilitySystemInitialized();
	// 由角色拓展组件的代理调用,ASC移除后触发,取消生命值组件的绑定属性
	// UE_API virtual void OnAbilitySystemUninitialized();


	// 当此角色被附身时调用。仅在服务器（或独立运行时）调用。
	// 绑定和传递队伍变更信息.
	// 通知角色拓展组件控制器变更了.  尝试推进角色初始化流程
	// UE_API virtual void PossessedBy(AController* NewController) override;
	// 当我们的控制器不再控制我们时会触发此事件。此事件仅在服务器端（或在独立运行的情况下）才会触发。
	// 重置队伍信息
	// 通知角色拓展组件控制器变更了.
	// UE_API virtual void UnPossessed() override;

	// 通知角色拓展组件 控制器变更了 尝试推进角色初始化流程
	// UE_API virtual void OnRep_Controller() override;
	// 通知角色拓展组件 玩家状态类同步过来了  尝试推进角色初始化流程
	// UE_API virtual void OnRep_PlayerState() override;

	
	/** 允许兵种设置自定义输入绑定。当兵种被玩家控制器占有时（通过调用 CreatePlayerInputComponent() 创建的输入组件实现），会触发此函数。  */
	// 通知角色拓展组件 输入组件设置好了 尝试推进角色初始化流程
	// UE_API virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	

	// 初始化游戏玩法Tag 主要用于移动模式从引擎底层的枚举转换到Tag
	// UE_API void InitializeGameplayTags();

	/** 当角色“安全地”脱离游戏世界（低于“杀戮者”等角色的位置时）时会调用此函数 */
	// 掉出场景限制高度时 会调用这个函数
	// 通过生命值组件 造成一次足以自我销毁的伤害即可
	// UE_API virtual void FellOutOfWorld(const class UDamageType& dmgType) override;



	// Begins the death sequence for the character (disables collision, disables movement, etc...)
	// 开始该角色的死亡流程（会禁用碰撞、禁用移动等功能......）
	// 由生命值组件调用
	// UFUNCTION()
	// UE_API virtual void OnDeathStarted(AActor* OwningActor);
	//
	// // Ends the death sequence for the character (detaches controller, destroys pawn, etc...)
	// // 结束角色的死亡过程（解除控制器控制、销毁角色等操作）
	// // 由生命值组件调用
	// UFUNCTION()
	// UE_API virtual void OnDeathFinished(AActor* OwningActor);
	//
	// // 关闭移动和碰撞
	// UE_API void DisableMovementAndCollision();
	//
	// // 触发摧毁逻辑
	// UE_API void DestroyDueToDeath();
	//
	//
	// // 通过在服务端设置生命周期 执行销毁逻辑
	// UE_API void UninitAndDestroy();
	//
	// // Called when the death sequence for the character has completed
	// // 当角色的死亡流程完成时触发此事件
	// UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnDeathFinished"))
	// UE_API void K2_OnDeathFinished();
	//
	//
	// /**
	//  * 由“角色移动组件”调用，以通知角色移动模式已发生改变。
	//  * @参数  原有移动模式：改变前的移动模式
	//  * @参数  原有自定义模式：若原有移动模式为“自定义”则为自定义模式（仅适用于该情况）
	//   * 
	//  */
	// UE_API virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
	// // 用于从移动组件中读取当前的移动类型,然后转换成Tag赋予给ASC.
	// UE_API void SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnabled);
	//
	//
	// // 当角色蹲下时触发此事件。对于非所属角色，通过“bIsCrouched”属性进行复制来触发此事件。
	// // 添加和移除蹲伏的Tag
	// UE_API virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	// // 当角色停止蹲伏时触发。通过“bIsCrouched”属性在非所属角色中触发此事件。
	// // 添加和移除蹲伏的Tag
	// UE_API virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	//
	// // 这里重写了父类逻辑 移除了蹲伏检查的逻辑 
	// UE_API virtual bool CanJumpInternal_Implementation() const;
	

	
	
private:

	// 拓展组件 极其重要!
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lyra|Character", Meta = (AllowPrivateAccess = "true"))
	// TObjectPtr<ULyraPawnExtensionComponent> PawnExtComponent;
	
	// 生命值组件
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lyra|Character", Meta = (AllowPrivateAccess = "true"))
	// TObjectPtr<ULyraHealthComponent> HealthComponent;

	// 相机组件
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lyra|Character", Meta = (AllowPrivateAccess = "true"))
	// TObjectPtr<ULyraCameraComponent> CameraComponent;

	// 临时代码 已删掉
	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lyra|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;*/
	
	
	// 用于同步的加速度 在服务端进行压缩 在客户端进行解压 然后 传递给移动组件
	// UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedAcceleration)
	// FLyraReplicatedAcceleration ReplicatedAcceleration;

	// 队伍ID
	// UPROPERTY(ReplicatedUsing = OnRep_MyTeamID)
	// FGenericTeamId MyTeamID;
	
	// 向下传递的队伍绑定代理
	// UPROPERTY()
	// FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;
	
protected:
	// Called to determine what happens to the team ID when possession ends
	// 用于确定当控球权结束时团队标识将如何处理的函数
	// virtual FGenericTeamId DetermineNewTeamAfterPossessionEnds(FGenericTeamId OldTeamID) const
	// {
	// 	// This could be changed to return, e.g., OldTeamID if you want to keep it assigned afterwards, or return an ID for some neutral faction, or etc...
	// 	// 这可以修改为返回诸如“OldTeamID”之类的值（如果您希望之后仍保留该团队的标识，则可采用此方式），或者返回某个中立阵营的标识，或者等等。
	// 	return FGenericTeamId::NoTeam;
	// }

private:
	// 队伍信息变更时触发
	// UFUNCTION()
	// UE_API void OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);
	//
	// // 接收成员变量加速度的属性同步,从极坐标转换为笛卡尔坐标
	// UFUNCTION()
	// UE_API void OnRep_ReplicatedAcceleration();
	//
	// // 队伍信息的属性同步
	// UFUNCTION()
	// UE_API void OnRep_MyTeamID(FGenericTeamId OldTeamID);

	
	
};





#undef UE_API