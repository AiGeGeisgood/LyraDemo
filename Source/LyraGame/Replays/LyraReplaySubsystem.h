// Copyright Epic Games, Inc. All Rights Reserved.
// Finished.
// 001 Not Finished. 只写了基础类.
#pragma once

#include "NetworkReplayStreaming.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"

#include "LyraReplaySubsystem.generated.h"

#define UE_API LYRAGAME_API

class UDemoNetDriver;
class APlayerController;
class ULocalPlayer;
struct FFrame;


/** An available replay for display in the UI */
/** 一个可在用户界面中显示的可用回放片段 */
UCLASS(MinimalAPI, BlueprintType)
class ULyraReplayListEntry : public UObject
{
	GENERATED_BODY()

public:
	// 用于存储有关流的信息的结构体，该信息来自搜索结果。
	FNetworkReplayStreamInfo StreamInfo;

	/** The UI friendly name of the stream */
	/** 流的用户友好名称 */
	UFUNCTION(BlueprintPure, Category=Replays)
	FString GetFriendlyName() const { return StreamInfo.FriendlyName; }

	/** The date and time the stream was recorded */
	/** 录制该流媒体的时间和日期 */
	UFUNCTION(BlueprintPure, Category=Replays)
	FDateTime GetTimestamp() const { return StreamInfo.Timestamp; }

	/** The duration of the stream in MS */
	/** 流的持续时间（以毫秒为单位） */
	UFUNCTION(BlueprintPure, Category=Replays)
	FTimespan GetDuration() const { return FTimespan::FromMilliseconds(StreamInfo.LengthInMS); }

	/** Number of viewers viewing this stream */
	/** 正在观看此流的观众数量 */
	UFUNCTION(BlueprintPure, Category=Replays)
	int32 GetNumViewers() const { return StreamInfo.NumViewers; }

	/** True if the stream is live and the game hasn't completed yet */
	/** 如果流媒体正在直播且游戏尚未结束，则返回 true */
	UFUNCTION(BlueprintPure, Category=Replays)
	bool GetIsLive() const { return StreamInfo.bIsLive; }
};

/** Results of querying for replays list of results for the UI */
/** 查询回放列表结果以供用户界面使用 */
UCLASS(MinimalAPI, BlueprintType)
class ULyraReplayList : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category=Replays)
	TArray<TObjectPtr<ULyraReplayListEntry>> Results;
};







UCLASS(MinimalAPI)
class ULyraReplaySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// 构造函数 无
	UE_API ULyraReplaySubsystem();

	/** Returns true if this platform supports replays at all */
	/** 返回值为真表示此平台完全支持回放功能 */
	UFUNCTION(BlueprintCallable, Category = Replays, BlueprintPure = false)
	static UE_API bool DoesPlatformSupportReplays();

	/** Returns the trait tag for platform support, used in options */
	/** 返回用于平台支持的特性标签，用于在选项中使用 */
	static UE_API FGameplayTag GetPlatformSupportTraitTag();
	
	/** Loads the appropriate map and plays a replay */
	/** 加载相应的地图并播放回放 */
	// 转发到GameInstance处理即可
	// 由蓝图空间调用
	UFUNCTION(BlueprintCallable, Category=Replays)
	UE_API void PlayReplay(ULyraReplayListEntry* Replay);

	
	/** Starts recording a client replay, and handles any file cleanup needed */
	/** 开始录制客户端回放，并处理任何所需的文件清理工作 */
	// 转发到GameInstance处理
	UFUNCTION(BlueprintCallable, Category = Replays)
	UE_API void RecordClientReplay(APlayerController* PlayerController);

	/** Starts deleting local replays starting with the oldest until there are NumReplaysToKeep or fewer */
	/** 开始删除本地回放文件，从最旧的开始依次删除，直至剩余数量达到“NumReplaysToKeep”或更少为止 */
	UFUNCTION(BlueprintCallable, Category = Replays)
	UE_API void CleanupLocalReplays(ULocalPlayer* LocalPlayer, int32 NumReplaysToKeep);

	/** Move forward or back in currently playing replay */
	/** 在当前播放的回放中向前或向后移动 */
	// 转发到UDemoNetDriver处理
	UFUNCTION(BlueprintCallable, Category=Replays)
	UE_API void SeekInActiveReplay(float TimeInSeconds);

	/** Gets length of current replay */
	/** 获取当前回放的长度 */
	// 转发到UDemoNetDriver处理
	UFUNCTION(BlueprintCallable, Category = Replays, BlueprintPure = false)
	UE_API float GetReplayLengthInSeconds() const;

	/** Gets current playback time */
	/** 获取当前播放时间 */
	// 转发到UDemoNetDriver处理
	UFUNCTION(BlueprintCallable, Category=Replays, BlueprintPure=false)
	UE_API float GetReplayCurrentTime() const;
	
	
private:
	// 网络回放流的通用接口
	// 当将委托作为参数提供时，预期实现者会在完成操作后调用该委托，并通过传递给委托的适当结果类型来表示成功或失败。
	TSharedPtr<INetworkReplayStreamer> CurrentReplayStreamer;

	// 正在处理删除的本地玩家对象
	UPROPERTY()
	TObjectPtr<ULocalPlayer> LocalPlayerDeletingReplays;

	// 要求保持的回放数量上限
	int32 DeletingReplaysNumberToKeep;
	
	// 用于记录和回放游戏进程的模拟网络驱动程序
	UDemoNetDriver* GetDemoDriver() const;

	// 获取回放结果完成的回调以便进行删除
	void OnEnumerateStreamsCompleteForDelete(const FEnumerateStreamsResult& Result);
	// 删除完成 递归查询再删除 
	void OnDeleteReplay(const FDeleteFinishedStreamResult& DeleteResult);
	
	
};

#undef UE_API



