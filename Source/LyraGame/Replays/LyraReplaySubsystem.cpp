// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraReplaySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/DemoNetDriver.h"
#include "Internationalization/Text.h"
#include "Misc/DateTime.h"
#include "CommonUISettings.h"
#include "ICommonUIModule.h"
#include "LyraLogChannels.h"
#include "Player/LyraLocalPlayer.h"
#include "Settings/LyraSettingsLocal.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraReplaySubsystem)


UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Platform_Trait_ReplaySupport, "Platform.Trait.ReplaySupport");

ULyraReplaySubsystem::ULyraReplaySubsystem()
{
	
}

bool ULyraReplaySubsystem::DoesPlatformSupportReplays()
{
	if (ICommonUIModule::GetSettings().GetPlatformTraits().HasTag(GetPlatformSupportTraitTag()))
	{
		return true;
	}
	return false;
}

FGameplayTag ULyraReplaySubsystem::GetPlatformSupportTraitTag()
{
	return TAG_Platform_Trait_ReplaySupport.GetTag();
}

void ULyraReplaySubsystem::PlayReplay(ULyraReplayListEntry* Replay)
{
	if (Replay != nullptr)
	{
		FString DemoName = Replay->StreamInfo.Name;
		GetGameInstance()->PlayReplay(DemoName);
	}
}
void ULyraReplaySubsystem::RecordClientReplay(APlayerController* PlayerController)
{
	// 确保上下文正确
	if (ensure(DoesPlatformSupportReplays() && PlayerController))
	{
		
		// 生成一个友好方便阅读的名称
		FText FriendlyNameText = 
			FText::Format(NSLOCTEXT("Lyra", "LyraReplayName_Format", "Client Replay {0}"), 
				FText::AsDateTime(FDateTime::UtcNow(), EDateTimeStyle::Short, EDateTimeStyle::Short));
		
		// 转发到GameInstance处理
		/**
		* 以指定的自定义名称和友称名称开始录制回放。*
		* @参数 InName：若不为空，则作为回放的标识符使用，该名称需唯一。若为空，则回放流式传输实现将自动生成一个名称。
		* @参数 FriendlyName：可选（可为空）的描述性名称，用于回放。该名称无需保证唯一性。
		* @参数 AdditionalOptions：附加的 URL 选项，将附加到回放网络驱动程序将处理的 URL 上。通常为空。
		* @参数 AnalyticsProvider：可选的指向分析提供商的指针，若设置则也会传递给回放流式传输。*/
		GetGameInstance()->StartRecordingReplay(FString(), FriendlyNameText.ToString());

		if (ULyraLocalPlayer* LyraLocalPlayer = Cast<ULyraLocalPlayer>(PlayerController->GetLocalPlayer()))
		{
			// Start a cleanup of existing saved streams
			// 开始清理现有的已保存流
			// 读取本地设置中关于回放数量的保存
			
			// int32 NumToKeep = LyraLocalPlayer->GetLocalSettings()->GetNumberOfReplaysToKeep();
			// CleanupLocalReplays(LyraLocalPlayer, NumToKeep);
		}

	}
	

}

void ULyraReplaySubsystem::CleanupLocalReplays(ULocalPlayer* LocalPlayer, int32 NumReplaysToKeep)
{
	// TODO this was only tested with the generic file streamer and may not fully work with the save game streamer
	// This only handles one delete at a time, and will loop until it gets an error or goes below NumReplaysToKeep
	// It does it this way because each delete may involve a server or save game query that invalidates the replay list
	
	// 注意：此功能仅在通用文件传输器上进行了测试，可能无法与保存游戏传输器完全兼容
	// 此功能每次仅处理一次删除操作，并会循环执行，直到出现错误或删除次数低于“保留游戏次数”设定值
	// 之所以采用这种方式处理，是因为每次删除操作可能涉及服务器或保存游戏查询，这会致使游戏回放列表失效
	if (LocalPlayer != nullptr && LocalPlayerDeletingReplays == nullptr && NumReplaysToKeep != 0)
	{
		LocalPlayerDeletingReplays = LocalPlayer;
		DeletingReplaysNumberToKeep = NumReplaysToKeep;

		CurrentReplayStreamer = FNetworkReplayStreaming::Get().GetFactory().CreateReplayStreamer();
		
		if (CurrentReplayStreamer.IsValid())
		{
			// Use the default version to get old version replays as well
			// 使用默认版本可同时获取旧版本的回放记录
			FNetworkReplayVersion EnumerateStreamsVersion;

			CurrentReplayStreamer->EnumerateStreams(EnumerateStreamsVersion,
				LocalPlayer->GetPlatformUserIndex(),
				FString(), TArray<FString>(), 
				FEnumerateStreamsCallback::CreateUObject(this, &ThisClass::OnEnumerateStreamsCompleteForDelete));
		}

	}

}


void ULyraReplaySubsystem::OnEnumerateStreamsCompleteForDelete(const FEnumerateStreamsResult& Result)
{
	if (!CurrentReplayStreamer.IsValid() || !IsValid(LocalPlayerDeletingReplays))
	{
		// Lost context, don't do anything
		// 没有背景信息，不要做任何事
		return;
	}
	
	TArray<FNetworkReplayStreamInfo> StreamsToDelete;
	for (const FNetworkReplayStreamInfo& StreamInfo : Result.FoundStreams)
	{
		// Never delete keep streams
		// 请勿删除保留的流数据
		if (!StreamInfo.bShouldKeep)
		{
			StreamsToDelete.Add(StreamInfo);
		}
	}
	
	// Sort by date
	// 按日期排序

	
	Algo::SortBy(StreamsToDelete, [](const FNetworkReplayStreamInfo& Data) { return Data.Timestamp.GetTicks(); }, TGreater<>());

	if (UDemoNetDriver* DemoDriver = GetDemoDriver())
	{
		if (DemoDriver->IsRecording())
		{
			// If we're recording, the live stream may or may not show up in the query which affects the keep count
			// Add a fake live stream if the active one is missing from the results
			
			// 如果正在进行录制，则实时直播内容可能会出现在查询结果中，也可能不会出现，这会影响计数的准确性
			// 如果活跃的直播流未出现在查询结果中，则添加一个虚拟的直播流以补充完整
			// bIsLive 如果该流正在直播且游戏尚未结束，则为真。
			if (StreamsToDelete.Num() > 0 && !StreamsToDelete[0].bIsLive)
			{
				StreamsToDelete.Insert(FNetworkReplayStreamInfo(), 0);
			}
		}
	}

	if (StreamsToDelete.Num() > DeletingReplaysNumberToKeep)
	{
		// Delete the first replay above the limit, if successful it won't be in the loop during the next loop
		// If unsuccessful, it will stop looping
		// 删除超出限制范围内的第一条回放，如果操作成功，那么在下一次循环中它将不会再次出现
		// 如果操作失败，则循环将停止
		FString ReplayName = StreamsToDelete[DeletingReplaysNumberToKeep].Name;
		UE_LOG(LogLyra, Log, TEXT("LyraReplaySubsystem asked to delete replay %s"), *ReplayName);
		CurrentReplayStreamer->DeleteFinishedStream(ReplayName,
			LocalPlayerDeletingReplays->GetPlatformUserIndex(), 
			FDeleteFinishedStreamCallback::CreateUObject(this, &ThisClass::OnDeleteReplay));
	}
	else
	{
		// We're below the limit so stop iterating
		// 我们的数量已满足要求，所以停止迭代吧
		
		CurrentReplayStreamer = nullptr;
		LocalPlayerDeletingReplays = nullptr;
		DeletingReplaysNumberToKeep = 0;
	}

}


void ULyraReplaySubsystem::OnDeleteReplay(const FDeleteFinishedStreamResult& DeleteResult)
{
	
	if (!CurrentReplayStreamer.IsValid() || !IsValid(LocalPlayerDeletingReplays))
	{
		// Lost context, don't do anything
		// 没有背景信息，不要做任何事
		return;
	}
	if (DeleteResult.WasSuccessful())
	{
		// Enumerate list again to see if we're under the limit yet
		// 再次列出这个列表，看看是否已经超出了限制范围
		FNetworkReplayVersion EnumerateStreamsVersion;

		CurrentReplayStreamer->EnumerateStreams(EnumerateStreamsVersion, 
			LocalPlayerDeletingReplays->GetPlatformUserIndex(),
			FString(), 
			TArray<FString>(), 
			FEnumerateStreamsCallback::CreateUObject(this, &ThisClass::OnEnumerateStreamsCompleteForDelete));
	}
	else
	{
		// Failed, stop trying to delete anything else
		// TODO properly integrate with platform-specific error reporting
		// 失败了，停止尝试删除其他任何内容
		// TODO 要求与特定平台的错误报告系统实现完美整合
		UE_LOG(LogLyra, Warning, TEXT("Failed to delete replay with error %d!"), (int32)DeleteResult.Result);

		CurrentReplayStreamer = nullptr;
		LocalPlayerDeletingReplays = nullptr;
		DeletingReplaysNumberToKeep = 0;
	}
}

void ULyraReplaySubsystem::SeekInActiveReplay(float TimeInSeconds)
{
	if (UDemoNetDriver* DemoDriver = GetDemoDriver())
	{
		DemoDriver->GotoTimeInSeconds(TimeInSeconds);
	}
}


float ULyraReplaySubsystem::GetReplayLengthInSeconds() const
{
	if (UDemoNetDriver* DemoDriver = GetDemoDriver())
	{
		return DemoDriver->GetDemoTotalTime();
	}
	return 0.0f;
}

float ULyraReplaySubsystem::GetReplayCurrentTime() const
{
	if (UDemoNetDriver* DemoDriver = GetDemoDriver())
	{
		return DemoDriver->GetDemoCurrentTime();
	}
	return 0.0f;
}

UDemoNetDriver* ULyraReplaySubsystem::GetDemoDriver() const
{
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		return World->GetDemoNetDriver();
	}
	return nullptr;
}
