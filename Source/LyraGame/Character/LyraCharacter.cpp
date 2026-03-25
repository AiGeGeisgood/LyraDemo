// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraCharacter.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Camera/LyraCameraComponent.h"
#include "Character/LyraHealthComponent.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "LyraCharacterMovementComponent.h"
#include "LyraGameplayTags.h"
#include "LyraLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "Player/LyraPlayerController.h"
#include "Player/LyraPlayerState.h"
#include "System/LyraSignificanceManager.h"
#include "TimerManager.h"
#include "GameFramework/SpringArmComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraCharacter)

class AActor;
class FLifetimeProperty;
class IRepChangedPropertyTracker;
class UInputComponent;

// 胶囊体碰撞预设文件
static FName NAME_LyraCharacterCollisionProfile_Capsule(TEXT("LyraPawnCapsule"));
// 网格体碰撞预设文件
static FName NAME_LyraCharacterCollisionProfile_Mesh(TEXT("LyraPawnMesh"));


FSharedRepMovement::FSharedRepMovement()
{
	// 允许对复制位置向量的压缩级别进行调整。只有在出现视觉瑕疵的情况下，您才需要更改此默认值。
	RepMovement.LocationQuantizationLevel = EVectorQuantization::RoundTwoDecimals;
}

bool FSharedRepMovement::FillForCharacter(ACharacter* Character)
{
	if (USceneComponent* PawnRootComponent = Character->GetRootComponent())
	{
		UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement();

		// 根据角色所在的“世界”位置，将本地起始位置重新定位至零世界原点值。
		// 在世界空间中的位置
		RepMovement.Location = FRepMovement::RebaseOntoZeroOrigin(PawnRootComponent->GetComponentLocation(), Character);
		/** 返回组件在世界空间中的旋转角度 */
		// 当前的旋转状态
		RepMovement.Rotation = PawnRootComponent->GetComponentRotation();
		/** 当前更新组件的速度。*/
		// 世界空间中组件的速度
		// RepMovement.LinearVelocity = CharacterMovement->Velocity;
		// // 移动模式打包 主要是用于节约带宽传输
		// RepMovementMode = CharacterMovement->PackNetworkMovementMode();
		/** GetProxyIsJumpForceApplied:表示此角色当前正受到跳跃力的作用（若 JumpMaxHoldTime 不为零）。IsJumpProvidingForce() 函数也会处理此情况。此函数返回 bProxyIsJumpForceApplied 属性 */
		/** JumpForceTimeRemaining:当 JumpMaxHoldTime 不为零时，剩余的跳跃力量持续时间。*/
		bProxyIsJumpForceApplied = Character->GetProxyIsJumpForceApplied() || (Character->JumpForceTimeRemaining >
			0.0f);
		/** 由角色移动操作设定，用于指示当前该角色处于蹲伏状态。此函数返回 bIsCrouched 值。*/
		bIsCrouched = Character->IsCrouched();

		// Timestamp is sent as zero if unused
		// 若未使用，则时间戳将被发送为零值
		/**
		 * bNetworkAlwaysReplicateTransformUpdateTimestamp
		 * 服务器上使用的标志，用于决定是否始终将“ReplicatedServerLastTransformUpdateTimeStamp”数据复制给客户端。
		 * 通常只有在角色移动的网络平滑模式设置为线性平滑（在服务器端）时才会发送此数据，以节省带宽。
		 * 将此标志设置为“真”将强制该时间戳进行复制，无论服务器是否知晓平滑模式，或者如果该时间戳用于其他目的的话。
		 *
		 */
		// NetworkSmoothingMode:网络游戏中模拟代理对象的平滑处理模式。
		// Linear:从源点到目标点进行线性插值。
		// if ((CharacterMovement->NetworkSmoothingMode == ENetworkSmoothingMode::Linear) || CharacterMovement->
		// 	bNetworkAlwaysReplicateTransformUpdateTimestamp)
		// {
		// 	RepTimeStamp = CharacterMovement->GetServerLastTransformUpdateTimeStamp();
		// }
		// else
		// {
		// 	RepTimeStamp = 0.f;
		// }

		return true;
	}


	return false;
}

bool FSharedRepMovement::Equals(const FSharedRepMovement& Other, ACharacter* Character) const
{
	if (RepMovement.Location != Other.RepMovement.Location)
	{
		return false;
	}

	if (RepMovement.Rotation != Other.RepMovement.Rotation)
	{
		return false;
	}

	if (RepMovement.LinearVelocity != Other.RepMovement.LinearVelocity)
	{
		return false;
	}

	if (RepMovementMode != Other.RepMovementMode)
	{
		return false;
	}

	if (bProxyIsJumpForceApplied != Other.bProxyIsJumpForceApplied)
	{
		return false;
	}

	if (bIsCrouched != Other.bIsCrouched)
	{
		return false;
	}

	return true;
}

bool FSharedRepMovement::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	// 填充数据
	RepMovement.NetSerialize(Ar, Map, bOutSuccess);
	Ar << RepMovementMode;
	Ar << bProxyIsJumpForceApplied;
	Ar << bIsCrouched;

	// Timestamp, if non-zero.
	// 时间戳（若非零值）
	uint8 bHasTimeStamp = (RepTimeStamp != 0.f);
	Ar.SerializeBits(&bHasTimeStamp, 1);
	if (bHasTimeStamp)
	{
		Ar << RepTimeStamp;
	}
	else
	{
		RepTimeStamp = 0.f;
	}

	return true;
}

ALyraCharacter::ALyraCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer/*.SetDefaultSubobjectClass<ULyraCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName)*/)
{
	// Avoid ticking characters if possible.
	// 如有可能，请避免勾选相关选项。
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;


	// 设定该角色与客户端视角之间的最大距离的平方值，此值将作为该角色的相关性指标，并将进行复制处理。
	SetNetCullDistanceSquared(900000000.0f);


	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);

	// 设置胶囊的大小，但不会触发渲染或物理更新。当在类构造函数中初始化组件时，这是首选的方法。
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);

	// 设置碰撞配置文件名称
	// 在构造函数中调用此函数时会设置“配置文件名称”属性
	// 这将使当前的“碰撞配置文件名称”变为此处指定的名称，并覆盖原有的碰撞设置	
	CapsuleComp->SetCollisionProfileName(NAME_LyraCharacterCollisionProfile_Capsule);


	USkeletalMeshComponent* MeshComp = GetMesh();
	check(MeshComp);

	// 将网格旋转至 X 方向朝前，因为其在导出时是以 Y 方向朝前的方式呈现的。
	// 注意这里是 Pitch Yaw Roll ,在蓝图里面呈现的顺序是 Roll Pitch Yaw
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	// Rotate mesh to be X forward since it is exported as Y forward.
	// 设置碰撞配置文件名称
	MeshComp->SetCollisionProfileName(NAME_LyraCharacterCollisionProfile_Mesh);

	// 修改移动组件的参数,注意这里使用的是我们自己的移动组件,需要通过ObjectInitializer去指定默认类!
	// ULyraCharacterMovementComponent* LyraMoveComp = CastChecked<
	// 	ULyraCharacterMovementComponent>(GetCharacterMovement());
	// 自定义重力值。对于该角色，重力值会乘以这个数值。
	// LyraMoveComp->GravityScale = 1.0f;
	// 最大加速度（速度的变化率）
	// LyraMoveComp->MaxAcceleration = 2400.0f;
	// 用于乘以制动时摩擦力实际值的系数。此系数适用于当前所使用的任何摩擦力值，其可能取决于 bUseSeparateBrakingFriction 的设置。
	// 备注:出于历史原因，默认值为 2，而值为 1 时则会得出正确的阻力计算公式。
	// LyraMoveComp->BrakingFrictionFactor = 1.0f;

	/**
	 *	制动时所应用的摩擦（阻力）系数（在加速值为 0 的情况下，或者当角色超过最大速度时）；实际使用的值是该系数乘以“制动摩擦系数”。
	 *	在制动过程中，此属性允许您控制在地面移动时所施加的摩擦力大小，即施加一个与当前速度成比例的反向力。制动由摩擦（与速度相关的阻力）和恒定的减速组成。
	 *	这是当前值，在所有移动模式中均使用；如果不需要此值，可以对其进行覆盖或在移动模式改变时使用“单独制动摩擦系数”选项。
	 *	备注:只有在“bUseSeparateBrakingFriction”设置为“真”的情况下才使用，否则将使用当前的摩擦力值，例如“GroundFriction”。
	 */
	// LyraMoveComp->BrakingFriction = 6.0f;

	/**
	 * 影响移动控制的设置。
	 * 数值越大，方向改变的速度就越快。
	 * 如果“使用单独的制动摩擦力”选项为“否”，那么它还会影响在制动时更快地停止的能力（当加速度为零时），此时该值会乘以“制动摩擦力系数”。
	 * 在制动时，此属性允许您控制在地面移动时施加的摩擦力大小，施加一个与当前速度成比例的反向力。
	 * 这可以用来模拟诸如冰面或油面等滑溜溜的表面，通过更改该值（可能基于站立的物体所接触的材质）来实现。
	 */
	// LyraMoveComp->GroundFriction = 8.0f;

	// 行走时减速且不加速。这是一种持续的反向力，会以固定的速度值直接降低速度。
	// LyraMoveComp->BrakingDecelerationWalking = 1400.0f;

	/**
	 * 如果情况属实，则根据控制器所期望的旋转方向（通常是 Controller->ControlRotation）平稳地将角色旋转过去，使用 RotationRate 作为旋转变化的速率。
	 * 此设置会覆盖 OrientRotationToMovement。
	 * 通常您需要确保其他设置已清除，例如 Character 上的 bUseControllerRotationYaw。
	 */
	// LyraMoveComp->bUseControllerDesiredRotation = false;


	/**
	 * 	如果情况属实，则应将角色朝向加速度的方向旋转，使用“旋转速率”作为旋转变化的速率。
	 * 	此设置会覆盖“UseControllerDesiredRotation”选项。通常您需要确保其他设置已清除，例如在角色上设置“bUseControllerRotationYaw”。
	 */
	// LyraMoveComp->bOrientRotationToMovement = false;

	// 每秒的旋转变化量，当 UseControllerDesiredRotation 或 OrientRotationToMovement 为真时使用。对于无限旋转速率和瞬间旋转，可设置负值。
	// LyraMoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);

	// 根节点动画时是否允许物理旋转
	// LyraMoveComp->bAllowPhysicsRotationDuringAnimRootMotion = false;

	/**
	 * 人工智能导航/路径规划中所使用的“智能体”（或“兵卒”）的表示属性。
	 * 定义组件可如何移动的属性。
	 * 若为真，则此兵种具备蹲伏的能力。
	 * 
	 */
	// LyraMoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;

	// 如果属实的话，角色在蹲伏状态下是可以从边缘处滑落下去的。
	// LyraMoveComp->bCanWalkOffLedgesWhenCrouching = true;

	// 在蹲伏时设置碰撞半高度，并更新相关计算。
	// LyraMoveComp->SetCrouchedHalfHeight(65.0f);

	// 创建角色拓展组件
	// PawnExtComponent = CreateDefaultSubobject<ULyraPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	// ASC初始化完成后调用,用于初始化生命值组件绑定属性,和Tag容器接口的初始化
	// PawnExtComponent->OnAbilitySystemInitialized_RegisterAndCall(
		// FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	// ASC移除后调用,用于生命值组件取消绑定属性
	// PawnExtComponent->OnAbilitySystemUninitialized_Register(
		// FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));


	// 创建生命值组件
	// HealthComponent = CreateDefaultSubobject<ULyraHealthComponent>(TEXT("HealthComponent"));
	// 绑定生命值组件上关于死亡的流程,开始死亡时禁用移动和关闭碰撞
	// HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
	// 绑定生命值组件上关于死亡的流程,死亡完成时,出发蓝图调用,并最终销毁
	// HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);

	// 创建相机组件
	 // CameraComponent = CreateDefaultSubobject<ULyraCameraComponent>(TEXT("CameraComponent"));
	// 将该组件的位置相对于其父组件进行设定
	 // CameraComponent->SetRelativeLocation(FVector(-300.0f, 0.0f, 75.0f));

	// 临时代码 已删掉
	/*{
		CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
		CameraBoom->SetupAttachment(RootComponent);
		CameraBoom->TargetArmLength = 400.0f;
		CameraBoom->bUsePawnControlRotation = true;

		CameraComponent = CreateDefaultSubobject<ULyraCameraComponent>(TEXT("FollowCamera"));
		CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
		CameraComponent->bUsePawnControlRotation = false;
	}*/


	/** 若为真，则当此兵种由玩家控制器控制时，其俯仰角度将与控制器的控制旋转角度保持一致。*/
	bUseControllerRotationPitch = false;

	// 如果情况属实，那么这个兵的偏航角度将会被更新，以与控制器的控制旋转偏航角度保持一致（如果该兵是由玩家控制器控制的）
	bUseControllerRotationYaw = true;

	// 如果情况属实，那么这个“兵”的动作将会被更新，使其与“控制器”的“控制旋转”动作相匹配（如果该“兵”是由“玩家控制器”控制的的话）。
	bUseControllerRotationRoll = false;


	// 基底眼睛所在位置距离碰撞中心的垂直高度。
	BaseEyeHeight = 80.0f;

	// 默认的蹲伏时眼睛高度
	CrouchedEyeHeight = 50.0f;
}

ALyraPlayerController* ALyraCharacter::GetLyraPlayerController() const
{
	return CastChecked<ALyraPlayerController>(GetController(), ECastCheckedType::NullAllowed);
}

ALyraPlayerState* ALyraCharacter::GetLyraPlayerState() const
{
	return CastChecked<ALyraPlayerState>(GetPlayerState(), ECastCheckedType::NullAllowed);
}

// ULyraAbilitySystemComponent* ALyraCharacter::GetLyraAbilitySystemComponent() const
// {
// 	return Cast<ULyraAbilitySystemComponent>(GetAbilitySystemComponent());
// }
//
// UAbilitySystemComponent* ALyraCharacter::GetAbilitySystemComponent() const
// {
// 	if (PawnExtComponent == nullptr)
// 	{
// 		return nullptr;
// 	}
//
// 	return PawnExtComponent->GetLyraAbilitySystemComponent();
// }
//
// void ALyraCharacter::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
// {
// 	if (const ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
// 	{
// 		LyraASC->GetOwnedGameplayTags(TagContainer);
// 	}
// }
//
// bool ALyraCharacter::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
// {
// 	if (const ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
// 	{
// 		return LyraASC->HasMatchingGameplayTag(TagToCheck);
// 	}
//
// 	return false;
// }
//
// bool ALyraCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
// {
// 	if (const ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
// 	{
// 		return LyraASC->HasAllMatchingGameplayTags(TagContainer);
// 	}
//
// 	return false;
// }
//
// bool ALyraCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
// {
// 	if (const ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
// 	{
// 		return LyraASC->HasAnyMatchingGameplayTags(TagContainer);
// 	}
//
// 	return false;
// }
//
// void ALyraCharacter::ToggleCrouch()
// {
// 	const ULyraCharacterMovementComponent* LyraMoveComp = CastChecked<ULyraCharacterMovementComponent>(
// 		GetCharacterMovement());
//
// 	if (IsCrouched() || LyraMoveComp->bWantsToCrouch)
// 	{
// 		UnCrouch();
// 	}
// 	else if (LyraMoveComp->IsMovingOnGround())
// 	{
// 		Crouch();
// 	}
// }
//
// void ALyraCharacter::PreInitializeComponents()
// {
// 	Super::PreInitializeComponents();
// }
//
// void ALyraCharacter::BeginPlay()
// {
// 	Super::BeginPlay();
//
// 	UWorld* World = GetWorld();
//
// 	const bool bRegisterWithSignificanceManager = !IsNetMode(NM_DedicatedServer);
// 	if (bRegisterWithSignificanceManager)
// 	{
// 		if (ULyraSignificanceManager* SignificanceManager = USignificanceManager::Get<ULyraSignificanceManager>(World))
// 		{
// 			//@TODO: SignificanceManager->RegisterObject(this, (EFortSignificanceType)SignificanceType);
// 		}
// 	}
// }
//
// void ALyraCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
// {
// 	Super::EndPlay(EndPlayReason);
//
// 	UWorld* World = GetWorld();
//
// 	const bool bRegisterWithSignificanceManager = !IsNetMode(NM_DedicatedServer);
// 	if (bRegisterWithSignificanceManager)
// 	{
// 		if (ULyraSignificanceManager* SignificanceManager = USignificanceManager::Get<ULyraSignificanceManager>(World))
// 		{
// 			SignificanceManager->UnregisterObject(this);
// 		}
// 	}
// }
//
// void ALyraCharacter::Reset()
// {
// 	DisableMovementAndCollision();
//
// 	K2_OnReset();
//
// 	UninitAndDestroy();
// }
//
// void ALyraCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
// {
// 	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//
// 	// 注意这里的同步规则,只同步模拟对象
// 	DOREPLIFETIME_CONDITION(ThisClass, ReplicatedAcceleration, COND_SimulatedOnly);
//
// 	DOREPLIFETIME(ThisClass, MyTeamID)
// }
//
// void ALyraCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
// {
// 	Super::PreReplication(ChangedPropertyTracker);
//
// 	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
// 	{
// 		// Compress Acceleration: XY components as direction + magnitude, Z component as direct value
// 		// 压缩加速：XY 分量表示方向和大小，Z 分量表示直接数值
// 		const double MaxAccel = MovementComponent->MaxAcceleration;
// 		const FVector CurrentAccel = MovementComponent->GetCurrentAcceleration();
// 		double AccelXYRadians, AccelXYMagnitude;
// 		// 将给定的笛卡尔坐标对转换为极坐标系。
// 		FMath::CartesianToPolar(CurrentAccel.X, CurrentAccel.Y, AccelXYMagnitude, AccelXYRadians);
//
// 		ReplicatedAcceleration.AccelXYRadians = FMath::FloorToInt((AccelXYRadians / TWO_PI) * 255.0);
// 		// [0, 2PI] -> [0, 255]
// 		ReplicatedAcceleration.AccelXYMagnitude = FMath::FloorToInt((AccelXYMagnitude / MaxAccel) * 255.0);
// 		// [0, MaxAccel] -> [0, 255]
// 		ReplicatedAcceleration.AccelZ = FMath::FloorToInt((CurrentAccel.Z / MaxAccel) * 127.0);
// 		// [-MaxAccel, MaxAccel] -> [-127, 127]
// 	}
// }
//
// void ALyraCharacter::NotifyControllerChanged()
// {
// 	const FGenericTeamId OldTeamId = GetGenericTeamId();
//
// 	Super::NotifyControllerChanged();
//
// 	// Update our team ID based on the controller
// 	// 根据控制器更新我们的团队编号
// 	if (HasAuthority() && (GetController() != nullptr))
// 	{
// 		if (ILyraTeamAgentInterface* ControllerWithTeam = Cast<ILyraTeamAgentInterface>(GetController()))
// 		{
// 			MyTeamID = ControllerWithTeam->GetGenericTeamId();
// 			ConditionalBroadcastTeamChanged(this, OldTeamId, MyTeamID);
// 		}
// 	}
// }
//
// void ALyraCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamID)
// {
// 	if (GetController() == nullptr)
// 	{
// 		// 无控制器
// 		if (HasAuthority())
// 		{
// 			// 服务器上
// 			// 证明它只是一个有队伍的独立对象,直接变更它及它的子项即可!
// 			const FGenericTeamId OldTeamID = MyTeamID;
// 			MyTeamID = NewTeamID;
// 			ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
// 		}
// 		else
// 		{
// 			// 客户端不允许变更 不具备该权限
// 			UE_LOG(LogLyraTeams, Error, TEXT("You can't set the team ID on a character (%s) except on the authority"),
// 			       *GetPathNameSafe(this));
// 		}
// 	}
// 	else
// 	{
// 		// 有控制器,不应当在这里直接变更自己的队伍ID
// 		UE_LOG(LogLyraTeams, Error,
// 		       TEXT("You can't set the team ID on a possessed character (%s); it's driven by the associated controller"
// 		       ), *GetPathNameSafe(this));
// 	}
// }
//
// FGenericTeamId ALyraCharacter::GetGenericTeamId() const
// {
// 	return MyTeamID;
// }
//
// FOnLyraTeamIndexChangedDelegate* ALyraCharacter::GetOnTeamIndexChangedDelegate()
// {
// 	return &OnTeamChangedDelegate;
// }
//
// void ALyraCharacter::FastSharedReplication_Implementation(const FSharedRepMovement& SharedRepMovement)
// {
// 	/** 如果我们当前正在播放回放，则返回 true */
// 	if (GetWorld()->IsPlayingReplay())
// 	{
// 		return;
// 	}
//
// 	// Timestamp is checked to reject old moves.
// 	// 会检查时间戳以剔除过时的交易记录。
// 	if (GetLocalRole() == ROLE_SimulatedProxy)
// 	{
// 		// Timestamp
// 		// 时间戳
// 		/** 用于记录角色移动的“服务器上最新变换更新时间戳”的值，该值会被复制到模拟代理中。*/
// 		SetReplicatedServerLastTransformUpdateTimeStamp(SharedRepMovement.RepTimeStamp);
//
// 		// Movement mode
// 		if (GetReplicatedMovementMode() != SharedRepMovement.RepMovementMode)
// 		{
// 			SetReplicatedMovementMode(SharedRepMovement.RepMovementMode);
// 			/** 当网络化移动模式已完成复制时为真。*/
// 			GetCharacterMovement()->bNetworkMovementModeChanged = true;
// 			// 当接收到模拟代理的网络复制更新时为真。
// 			GetCharacterMovement()->bNetworkUpdateReceived = true;
// 		}
//
// 		// Location, Rotation, Velocity, etc.
// 		// 位置、旋转、速度等
// 		/**
// 		 * GetReplicatedMovement_Mutable
// 		 * 获取对“ReplicatedMovement”对象的引用，并期望该对象会被修改。*
// 		 * 这样做是为了避免子类需要直接访问“复制运动”属性
// 		 * 因此，之后可以将其设为私有属性。
// 		 * 
// 		 */
// 		FRepMovement& MutableRepMovement = GetReplicatedMovement_Mutable();
// 		MutableRepMovement = SharedRepMovement.RepMovement;
//
// 		// This also sets LastRepMovement
// 		// 这样也设置了“最后移动状态”
// 		OnRep_ReplicatedMovement();
//
//
// 		// Jump force
// 		// 跳跃力
// 		// 表示该角色当前正受到跳跃力的作用（若 JumpMaxHoldTime 不为零）。IsJumpProvidingForce() 函数也会处理此情况。此函数会设置 bProxyIsJumpForceApplied
// 		SetProxyIsJumpForceApplied(SharedRepMovement.bProxyIsJumpForceApplied);
//
//
// 		// Crouch
// 		// 蹲下
// 		if (IsCrouched() != SharedRepMovement.bIsCrouched)
// 		{
// 			SetIsCrouched(SharedRepMovement.bIsCrouched);
// 			OnRep_IsCrouched();
// 		}
// 	}
// }
//
// bool ALyraCharacter::UpdateSharedReplication()
// {
// 	if (GetLocalRole() == ROLE_Authority)
// 	{
// 		FSharedRepMovement SharedMovement;
// 		if (SharedMovement.FillForCharacter(this))
// 		{
// 			// Only call FastSharedReplication if data has changed since the last frame.
// 			// Skipping this call will cause replication to reuse the same bunch that we previously
// 			// produced, but not send it to clients that already received. (But a new client who has not received
// 			// it, will get it this frame)
//
// 			// 仅在数据自上一帧以来发生更改时才调用 FastSharedReplication 函数。
// 			// 忽略此调用将导致复制操作重复使用我们之前生成的相同数据组，但不会将其发送给已经接收过该数据的客户端。（但对于尚未接收该数据的新客户端，它将在本帧中获得该数据）
// 			if (!SharedMovement.Equals(LastSharedReplication, this))
// 			{
// 				LastSharedReplication = SharedMovement;
// 				/**
// 				 * 将 CharacterMovement 的 MovementMode（以及自定义模式）设置为可进行网络复制的值，以便在模拟代理中使用。
// 				 * 使用 CharacterMovementComponent:：UnpackNetworkMovementMode() 函数来将其转换为可读形式。
// 				 * 此函数会设置可压缩的 ReplicatedMovementMode 值。
// 				 * 
// 				 */
// 				SetReplicatedMovementMode(SharedMovement.RepMovementMode);
//
// 				FastSharedReplication(SharedMovement);
// 			}
//
// 			return true;
// 		}
// 	}
//
// 	// We cannot fastrep right now. Don't send anything.
// 	// 目前我们无法快速处理。请不要发送任何信息。
// 	return false;
// }
//
// void ALyraCharacter::OnAbilitySystemInitialized()
// {
// 	ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent();
// 	check(LyraASC);
//
// 	HealthComponent->InitializeWithAbilitySystem(LyraASC);
//
// 	InitializeGameplayTags();
// }
//
// void ALyraCharacter::OnAbilitySystemUninitialized()
// {
// 	HealthComponent->UninitializeFromAbilitySystem();
// }
//
// void ALyraCharacter::PossessedBy(AController* NewController)
// {
// 	const FGenericTeamId OldTeamID = MyTeamID;
//
// 	Super::PossessedBy(NewController);
//
// 	PawnExtComponent->HandleControllerChanged();
//
// 	// Grab the current team ID and listen for future changes
// 	// 获取当前团队的 ID 并监听未来的变化
// 	if (ILyraTeamAgentInterface* ControllerAsTeamProvider = Cast<ILyraTeamAgentInterface>(NewController))
// 	{
// 		MyTeamID = ControllerAsTeamProvider->GetGenericTeamId();
// 		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnControllerChangedTeam);
// 	}
// 	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
// }
//
// void ALyraCharacter::UnPossessed()
// {
// 	AController* const OldController = GetController();
//
// 	// Stop listening for changes from the old controller
// 	// 停止监听来自旧控制器的更改信息
// 	const FGenericTeamId OldTeamID = MyTeamID;
// 	if (ILyraTeamAgentInterface* ControllerAsTeamProvider = Cast<ILyraTeamAgentInterface>(OldController))
// 	{
// 		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().RemoveAll(this);
// 	}
//
// 	Super::UnPossessed();
//
// 	PawnExtComponent->HandleControllerChanged();
//
// 	// Determine what the new team ID should be afterwards
// 	// 接下来确定新的团队编号应该是多少
// 	MyTeamID = DetermineNewTeamAfterPossessionEnds(OldTeamID);
// 	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
// }
//
// void ALyraCharacter::OnRep_Controller()
// {
// 	Super::OnRep_Controller();
//
// 	PawnExtComponent->HandleControllerChanged();
// }
//
// void ALyraCharacter::OnRep_PlayerState()
// {
// 	Super::OnRep_PlayerState();
//
// 	PawnExtComponent->HandlePlayerStateReplicated();
// }
//
// void ALyraCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
// {
// 	Super::SetupPlayerInputComponent(PlayerInputComponent);
//
// 	PawnExtComponent->SetupPlayerInputComponent();
// }
//
// void ALyraCharacter::InitializeGameplayTags()
// {
// 	// Clear tags that may be lingering on the ability system from the previous pawn.
// 	// 清除之前角色在能力系统中可能遗留的无效标签。
// 	if (ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
// 	{
// 		for (const TPair<uint8, FGameplayTag>& TagMapping : LyraGameplayTags::MovementModeTagMap)
// 		{
// 			if (TagMapping.Value.IsValid())
// 			{
// 				LyraASC->SetLooseGameplayTagCount(TagMapping.Value, 0);
// 			}
// 		}
//
// 		for (const TPair<uint8, FGameplayTag>& TagMapping : LyraGameplayTags::CustomMovementModeTagMap)
// 		{
// 			if (TagMapping.Value.IsValid())
// 			{
// 				LyraASC->SetLooseGameplayTagCount(TagMapping.Value, 0);
// 			}
// 		}
//
// 		ULyraCharacterMovementComponent* LyraMoveComp = CastChecked<ULyraCharacterMovementComponent>(
// 			GetCharacterMovement());
// 		SetMovementModeTag(LyraMoveComp->MovementMode, LyraMoveComp->CustomMovementMode, true);
// 	}
// }
//
// void ALyraCharacter::FellOutOfWorld(const class UDamageType& dmgType)
// {
// 	HealthComponent->DamageSelfDestruct(/*bFellOutOfWorld=*/ true);
// }
//
// void ALyraCharacter::OnDeathStarted(AActor* OwningActor)
// {
// 	DisableMovementAndCollision();
// }
//
// void ALyraCharacter::OnDeathFinished(AActor* OwningActor)
// {
// 	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyDueToDeath);
// }
//
// void ALyraCharacter::DisableMovementAndCollision()
// {
// 	if (GetController())
// 	{
// 		GetController()->SetIgnoreMoveInput(true);
// 	}
//
//
// 	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
// 	check(CapsuleComp);
// 	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
// 	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);
//
// 	ULyraCharacterMovementComponent* LyraMoveComp = CastChecked<
// 		ULyraCharacterMovementComponent>(GetCharacterMovement());
// 	LyraMoveComp->StopMovementImmediately();
// 	LyraMoveComp->DisableMovement();
// }
//
// void ALyraCharacter::DestroyDueToDeath()
// {
// 	K2_OnDeathFinished();
//
// 	UninitAndDestroy();
// }
//
// void ALyraCharacter::UninitAndDestroy()
// {
// 	if (GetLocalRole() == ROLE_Authority)
// 	{
// 		/** 调用此函数可安全地将兵卒从其控制器处分离出来，要知道我们很快就会被摧毁。*/
// 		DetachFromControllerPendingDestroy();
// 		SetLifeSpan(0.1f);
// 	}
//
// 	// Uninitialize the ASC if we're still the avatar actor (otherwise another pawn already did it when they became the avatar actor)
// 	// 如果我们仍是角色扮演者，则解除 ASC 的初始化（否则，当其他玩家成为角色扮演者时，他们已经完成了这一操作）
// 	if (ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
// 	{
// 		if (LyraASC->GetAvatarActor() == this)
// 		{
// 			PawnExtComponent->UninitializeAbilitySystem();
// 		}
// 	}
// 	// 将该角色设置为游戏中的隐藏状态 true 关联组件也隐藏
// 	SetActorHiddenInGame(true);
// }
//
// void ALyraCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
// {
// 	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
//
// 	ULyraCharacterMovementComponent* LyraMoveComp = CastChecked<
// 		ULyraCharacterMovementComponent>(GetCharacterMovement());
//
// 	SetMovementModeTag(PrevMovementMode, PreviousCustomMode, false);
// 	SetMovementModeTag(LyraMoveComp->MovementMode, LyraMoveComp->CustomMovementMode, true);
// }
//
// void ALyraCharacter::SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnabled)
// {
// 	if (ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
// 	{
// 		const FGameplayTag* MovementModeTag = nullptr;
// 		if (MovementMode == MOVE_Custom)
// 		{
// 			MovementModeTag = LyraGameplayTags::CustomMovementModeTagMap.Find(CustomMovementMode);
// 		}
// 		else
// 		{
// 			MovementModeTag = LyraGameplayTags::MovementModeTagMap.Find(MovementMode);
// 		}
//
// 		if (MovementModeTag && MovementModeTag->IsValid())
// 		{
// 			LyraASC->SetLooseGameplayTagCount(*MovementModeTag, (bTagEnabled ? 1 : 0));
// 		}
// 	}
// }
//
// void ALyraCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
// {
// 	if (ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
// 	{
// 		LyraASC->SetLooseGameplayTagCount(LyraGameplayTags::Status_Crouching, 1);
// 	}
//
//
// 	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
// }
//
// void ALyraCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
// {
// 	if (ULyraAbilitySystemComponent* LyraASC = GetLyraAbilitySystemComponent())
// 	{
// 		LyraASC->SetLooseGameplayTagCount(LyraGameplayTags::Status_Crouching, 0);
// 	}
//
// 	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
// }
//
// bool ALyraCharacter::CanJumpInternal_Implementation() const
// {
// 	// same as ACharacter's implementation but without the crouch check
// 	// 与 ACharacter 的实现方式相同，但不包含蹲伏检查的部分
// 	// 	return !IsCrouched() && JumpIsAllowedInternal();
// 	return JumpIsAllowedInternal();
// }
//
// void ALyraCharacter::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
// {
// 	const FGenericTeamId MyOldTeamID = MyTeamID;
// 	MyTeamID = IntegerToGenericTeamId(NewTeam);
// 	ConditionalBroadcastTeamChanged(this, MyOldTeamID, MyTeamID);
// }
//
// void ALyraCharacter::OnRep_ReplicatedAcceleration()
// {
// 	if (ULyraCharacterMovementComponent* LyraMovementComponent = Cast<ULyraCharacterMovementComponent>(
// 		GetCharacterMovement()))
// 	{
// 		// Decompress Acceleration
// 		// 解压加速作用
// 		const double MaxAccel = LyraMovementComponent->MaxAcceleration;
// 		const double AccelXYMagnitude = double(ReplicatedAcceleration.AccelXYMagnitude) * MaxAccel / 255.0;
// 		// [0, 255] -> [0, MaxAccel]
// 		const double AccelXYRadians = double(ReplicatedAcceleration.AccelXYRadians) * TWO_PI / 255.0;
// 		// [0, 255] -> [0, 2PI]
//
// 		FVector UnpackedAcceleration(FVector::ZeroVector);
// 		FMath::PolarToCartesian(AccelXYMagnitude, AccelXYRadians, UnpackedAcceleration.X, UnpackedAcceleration.Y);
// 		UnpackedAcceleration.Z = double(ReplicatedAcceleration.AccelZ) * MaxAccel / 127.0;
// 		// [-127, 127] -> [-MaxAccel, MaxAccel]
//
// 		LyraMovementComponent->SetReplicatedAcceleration(UnpackedAcceleration);
// 	}
// }
//
// void ALyraCharacter::OnRep_MyTeamID(FGenericTeamId OldTeamID)
// {
// 	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
// }
