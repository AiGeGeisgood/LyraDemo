// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraSettingsLocal.h"
#include "Engine/Engine.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "CommonInputSubsystem.h"
#include "GenericPlatform/GenericPlatformFramePacer.h"
#include "Player/LyraLocalPlayer.h"
#include "Performance/LatencyMarkerModule.h"
#include "Performance/LyraPerformanceStatTypes.h"
#include "ICommonUIModule.h"
#include "CommonUISettings.h"
#include "SoundControlBusMix.h"
#include "Widgets/Layout/SSafeZone.h"
#include "Performance/LyraPerformanceSettings.h"
#include "DeviceProfiles/DeviceProfileManager.h"
#include "DeviceProfiles/DeviceProfile.h"
#include "HAL/PlatformFramePacer.h"
#include "Development/LyraPlatformEmulationSettings.h"
#include "SoundControlBus.h"
#include "AudioModulationStatics.h"
#include "Audio/LyraAudioSettings.h"
#include "Audio/LyraAudioMixEffectsSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraSettingsLocal)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Platform_Trait_BinauralSettingControlledByOS,
                              "Platform.Trait.BinauralSettingControlledByOS");

namespace PerfStatTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Platform_Trait_SupportsLatencyStats, "Platform.Trait.SupportsLatencyStats");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Platform_Trait_SupportsLatencyMarkers, "Platform.Trait.SupportsLatencyMarkers");
}



//////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
static TAutoConsoleVariable<bool> CVarApplyFrameRateSettingsInPIE(TEXT("Lyra.Settings.ApplyFrameRateSettingsInPIE"),
                                                                  false,
                                                                  TEXT("Should we apply frame rate settings in PIE?"),
                                                                  ECVF_Default);

static TAutoConsoleVariable<bool> CVarApplyFrontEndPerformanceOptionsInPIE(
	TEXT("Lyra.Settings.ApplyFrontEndPerformanceOptionsInPIE"),
	false,
	TEXT("Do we apply front-end specific performance options in PIE?"),
	ECVF_Default);

static TAutoConsoleVariable<bool> CVarApplyDeviceProfilesInPIE(TEXT("Lyra.Settings.ApplyDeviceProfilesInPIE"),
                                                               false,
                                                               TEXT(
	                                                               "Should we apply experience/platform emulated device profiles in PIE?"),
                                                               ECVF_Default);
#endif


//////////////////////////////////////////////////////////////////////
// Console frame pacing

static TAutoConsoleVariable<int32> CVarDeviceProfileDrivenTargetFps(
	TEXT("Lyra.DeviceProfile.Console.TargetFPS"),
	-1,
	TEXT("Target FPS when being driven by device profile"),
	ECVF_Default | ECVF_Preview);

static TAutoConsoleVariable<int32> CVarDeviceProfileDrivenFrameSyncType(
	TEXT("Lyra.DeviceProfile.Console.FrameSyncType"),
	-1,
	TEXT("Sync type when being driven by device profile. Corresponds to r.GTSyncType"),
	ECVF_Default | ECVF_Preview);


//////////////////////////////////////////////////////////////////////
// Mobile frame pacing

static TAutoConsoleVariable<int32> CVarDeviceProfileDrivenMobileDefaultFrameRate(
	TEXT("Lyra.DeviceProfile.Mobile.DefaultFrameRate"),
	30,
	TEXT("Default FPS when being driven by device profile"),
	ECVF_Default | ECVF_Preview);

static TAutoConsoleVariable<int32> CVarDeviceProfileDrivenMobileMaxFrameRate(
	TEXT("Lyra.DeviceProfile.Mobile.MaxFrameRate"),
	30,
	TEXT("Max FPS when being driven by device profile"),
	ECVF_Default | ECVF_Preview);


//////////////////////////////////////////////////////////////////////
// “关于分辨率质量的限制列表，格式为“帧率：最高质量，帧率2：最高质量2……”，当帧率达到或超过阈值时即生效”
static TAutoConsoleVariable<FString> CVarMobileQualityLimits(
	TEXT("Lyra.DeviceProfile.Mobile.OverallQualityLimits"),
	TEXT(""),
	TEXT(
		"List of limits on resolution quality of the form \"FPS:MaxQuality,FPS2:MaxQuality2,...\", kicking in when FPS is at or above the threshold"),
	ECVF_Default | ECVF_Preview);


// “关于分辨率质量的限制列表，格式为“帧率：最高分辨率质量，帧率2：最高分辨率质量2，……”，当帧率达到或超过阈值时即生效”
static TAutoConsoleVariable<FString> CVarMobileResolutionQualityLimits(
	TEXT("Lyra.DeviceProfile.Mobile.ResolutionQualityLimits"),
	TEXT(""),
	TEXT(
		"List of limits on resolution quality of the form \"FPS:MaxResQuality,FPS2:MaxResQuality2,...\", kicking in when FPS is at or above the threshold"),
	ECVF_Default | ECVF_Preview);


// “关于分辨率质量的限制列表，格式为“帧率：建议值，帧率2：建议值2……”，当帧率达到或超过阈值时即生效”
static TAutoConsoleVariable<FString> CVarMobileResolutionQualityRecommendation(
	TEXT("Lyra.DeviceProfile.Mobile.ResolutionQualityRecommendation"),
	TEXT("0:75"),
	TEXT(
		"List of limits on resolution quality of the form \"FPS:Recommendation,FPS2:Recommendation2,...\", kicking in when FPS is at or above the threshold"),
	ECVF_Default | ECVF_Preview);


//////////////////////////////////////////////////////////////////////
FLyraScalabilitySnapshot::FLyraScalabilitySnapshot()
{
	static_assert(sizeof(Scalability::FQualityLevels) == 88,
	              "This function may need to be updated to account for new members");

	Qualities.ResolutionQuality = -1.0f;
	Qualities.ViewDistanceQuality = -1;
	Qualities.AntiAliasingQuality = -1;
	Qualities.ShadowQuality = -1;
	Qualities.GlobalIlluminationQuality = -1;
	Qualities.ReflectionQuality = -1;
	Qualities.PostProcessQuality = -1;
	Qualities.TextureQuality = -1;
	Qualities.EffectsQuality = -1;
	Qualities.FoliageQuality = -1;
	Qualities.ShadingQuality = -1;
}

//////////////////////////////////////////////////////////////////////
// 移动端质量包装
template <typename T>
struct TMobileQualityWrapper
{
private:
	// 默认值
	T DefaultValue;

	// 关联的命令行变量
	TAutoConsoleVariable<FString>& WatchedVar;

	// 上次关联的命令行变量观测到的值
	FString LastSeenCVarString;

	// 阈值的键值对
	struct FLimitPair
	{
		int32 Limit = 0;
		T Value = T(0);
	};

	// 阈值键值对的容器
	TArray<FLimitPair> Thresholds;

public:
	// 构造函数 指定默认值 关联命令行变量 必须是字符串类型
	TMobileQualityWrapper(T InDefaultValue, TAutoConsoleVariable<FString>& InWatchedVar)
		: DefaultValue(InDefaultValue)
		  , WatchedVar(InWatchedVar)
	{
	}

	// 传入键值 返回符合要求的值 否则就返回默认值
	T Query(int32 TestValue)
	{
		// 更新缓存 获取到最新的值
		UpdateCache();

		for (const FLimitPair& Pair : Thresholds)
		{
			if (TestValue >= Pair.Limit)
			{
				return Pair.Value;
			}
		}

		return DefaultValue;
	}

	// Returns the first threshold value or INDEX_NONE if there aren't any
	// 返回第一个阈值值，若没有则返回 INDEX_NONE
	int32 GetFirstThreshold()
	{
		UpdateCache();
		return (Thresholds.Num() > 0) ? Thresholds[0].Limit : INDEX_NONE;
	}

	// Returns the lowest value of all the pairs or DefaultIfNoPairs if there are no pairs
	// 返回所有配对项中的最小值；若无配对项，则返回“默认值（若无默认值则返回空值）”
	T GetLowestValue(T DefaultIfNoPairs)
	{
		UpdateCache();

		T Result = DefaultIfNoPairs;
		bool bFirstValue = true;
		for (const FLimitPair& Pair : Thresholds)
		{
			if (bFirstValue)
			{
				Result = Pair.Value;
				bFirstValue = false;
			}
			else
			{
				Result = FMath::Min(Result, Pair.Value);
			}
		}

		return Result;
	}

private:
	// 更新缓存
	void UpdateCache()
	{
		// 获取到最新的字符串
		const FString CurrentValue = WatchedVar.GetValueOnGameThread();
		// 应该与之前的字符串不一致
		if (!CurrentValue.Equals(LastSeenCVarString, ESearchCase::CaseSensitive))
		{
			// 存储此次的值作为前值
			LastSeenCVarString = CurrentValue;


			// 重置容器
			Thresholds.Reset();

			// Parse the thresholds
			// 切割阈值
			int32 ScanIndex = 0;

			while (ScanIndex < LastSeenCVarString.Len())
			{
				const int32 ColonIndex = LastSeenCVarString.Find(
					TEXT(":"), ESearchCase::CaseSensitive, ESearchDir::FromStart, ScanIndex);

				if (ColonIndex > 0)
				{
					const int32 CommaIndex = LastSeenCVarString.Find(
						TEXT(","), ESearchCase::CaseSensitive, ESearchDir::FromStart, ColonIndex);
					const int32 EndOfPairIndex = (CommaIndex != INDEX_NONE) ? CommaIndex : LastSeenCVarString.Len();

					FLimitPair Pair;
					LexFromString(Pair.Limit, *LastSeenCVarString.Mid(ScanIndex, ColonIndex - ScanIndex));
					LexFromString(Pair.Value, *LastSeenCVarString.Mid(ColonIndex + 1, EndOfPairIndex - ColonIndex - 1));

					Thresholds.Add(Pair);

					ScanIndex = EndOfPairIndex + 1;
				}
				else
				{
					UE_LOG(LogConsoleResponse, Error, TEXT("Malformed value for '%s'='%s', expecting a ':'"),
					       *IConsoleManager::Get().FindConsoleObjectName(WatchedVar.AsVariable()),
					       *LastSeenCVarString);
					Thresholds.Reset();
					break;
				}
			}


			// Sort the pairs
			// 排序
			Thresholds.Sort([](const FLimitPair A, const FLimitPair B) { return A.Limit < B.Limit; });
		}
	}
};


namespace LyraSettingsHelpers
{
	// 检测当前平台特性
	bool HasPlatformTrait(FGameplayTag Tag)
	{
		return ICommonUIModule::GetSettings().GetPlatformTraits().HasTag(Tag);
	}

	// Returns the max level from the integer scalability settings (ignores ResolutionQuality)
	// 返回整数可扩展性设置中的最大级别（忽略分辨率质量）
	int32 GetHighestLevelOfAnyScalabilityChannel(const Scalability::FQualityLevels& ScalabilityQuality)
	{
		static_assert(sizeof(Scalability::FQualityLevels) == 88,
		              "This function may need to be updated to account for new members");

		int32 MaxScalability = ScalabilityQuality.ViewDistanceQuality;
		MaxScalability = FMath::Max(MaxScalability, ScalabilityQuality.AntiAliasingQuality);
		MaxScalability = FMath::Max(MaxScalability, ScalabilityQuality.ShadowQuality);
		MaxScalability = FMath::Max(MaxScalability, ScalabilityQuality.GlobalIlluminationQuality);
		MaxScalability = FMath::Max(MaxScalability, ScalabilityQuality.ReflectionQuality);
		MaxScalability = FMath::Max(MaxScalability, ScalabilityQuality.PostProcessQuality);
		MaxScalability = FMath::Max(MaxScalability, ScalabilityQuality.TextureQuality);
		MaxScalability = FMath::Max(MaxScalability, ScalabilityQuality.EffectsQuality);
		MaxScalability = FMath::Max(MaxScalability, ScalabilityQuality.FoliageQuality);
		MaxScalability = FMath::Max(MaxScalability, ScalabilityQuality.ShadingQuality);


		return (MaxScalability >= 0) ? MaxScalability : -1;
	}

	// 从设备文件配置拓展性能设置
	void FillScalabilitySettingsFromDeviceProfile(FLyraScalabilitySnapshot& Mode, const FString& Suffix = FString())
	{
		static_assert(sizeof(Scalability::FQualityLevels) == 88,
		              "This function may need to be updated to account for new members");

		// Default out before filling so we can correctly mark non-overridden scalability values.
		// It's technically possible to swap device profile when testing so safest to clear and refill

		// 在填充数据之前先进行默认输出，这样我们就能准确地标记未被覆盖的可扩展性值。
		// 在测试时确实有可能更换设备配置文件，因此最安全的做法是先清空再重新填充。

		Mode = FLyraScalabilitySnapshot();


		Mode.bHasOverrides |= UDeviceProfileManager::GetScalabilityCVar(
			FString::Printf(TEXT("sg.ResolutionQuality%s"), *Suffix), Mode.Qualities.ResolutionQuality);
		Mode.bHasOverrides |= UDeviceProfileManager::GetScalabilityCVar(
			FString::Printf(TEXT("sg.ViewDistanceQuality%s"), *Suffix), Mode.Qualities.ViewDistanceQuality);
		Mode.bHasOverrides |= UDeviceProfileManager::GetScalabilityCVar(
			FString::Printf(TEXT("sg.AntiAliasingQuality%s"), *Suffix), Mode.Qualities.AntiAliasingQuality);
		Mode.bHasOverrides |= UDeviceProfileManager::GetScalabilityCVar(
			FString::Printf(TEXT("sg.ShadowQuality%s"), *Suffix), Mode.Qualities.ShadowQuality);
		Mode.bHasOverrides |= UDeviceProfileManager::GetScalabilityCVar(
			FString::Printf(TEXT("sg.GlobalIlluminationQuality%s"), *Suffix), Mode.Qualities.GlobalIlluminationQuality);
		Mode.bHasOverrides |= UDeviceProfileManager::GetScalabilityCVar(
			FString::Printf(TEXT("sg.ReflectionQuality%s"), *Suffix), Mode.Qualities.ReflectionQuality);
		Mode.bHasOverrides |= UDeviceProfileManager::GetScalabilityCVar(
			FString::Printf(TEXT("sg.PostProcessQuality%s"), *Suffix), Mode.Qualities.PostProcessQuality);
		Mode.bHasOverrides |= UDeviceProfileManager::GetScalabilityCVar(
			FString::Printf(TEXT("sg.TextureQuality%s"), *Suffix), Mode.Qualities.TextureQuality);
		Mode.bHasOverrides |= UDeviceProfileManager::GetScalabilityCVar(
			FString::Printf(TEXT("sg.EffectsQuality%s"), *Suffix), Mode.Qualities.EffectsQuality);
		Mode.bHasOverrides |= UDeviceProfileManager::GetScalabilityCVar(
			FString::Printf(TEXT("sg.FoliageQuality%s"), *Suffix), Mode.Qualities.FoliageQuality);
		Mode.bHasOverrides |= UDeviceProfileManager::GetScalabilityCVar(
			FString::Printf(TEXT("sg.ShadingQuality%s"), *Suffix), Mode.Qualities.ShadingQuality);
	}

	TMobileQualityWrapper<int32> OverallQualityLimits(-1, CVarMobileQualityLimits);
	TMobileQualityWrapper<float> ResolutionQualityLimits(100.0f, CVarMobileResolutionQualityLimits);
	TMobileQualityWrapper<float> ResolutionQualityRecommendations(75.0f, CVarMobileResolutionQualityRecommendation);


	// 获取该对应帧数的画面质量限制
	int32 GetApplicableOverallQualityLimit(int32 FrameRate)
	{
		return OverallQualityLimits.Query(FrameRate);
	}

	// 获取该对应帧数的画面分辨率质量限制
	float GetApplicableResolutionQualityLimit(int32 FrameRate)
	{
		return ResolutionQualityLimits.Query(FrameRate);
	}

	// 获取该对应帧数的建议画面分辨率质量限制
	float GetApplicableResolutionQualityRecommendation(int32 FrameRate)
	{
		return ResolutionQualityRecommendations.Query(FrameRate);
	}

	// 约束帧数,通过兼容的画面质量
	int32 ConstrainFrameRateToBeCompatibleWithOverallQuality(int32 FrameRate, int32 OverallQuality)
	{
		// const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
		// const TArray<int32>& PossibleRates = PlatformSettings->MobileFrameRateLimits;

		// Choose the closest frame rate (without going over) to the user preferred one that is supported and compatible with the desired overall quality
		// 选择与用户偏好帧率最为接近（且不超过该值）的帧率，该帧率需支持且与预期的整体画质相兼容。
		// int32 LimitIndex = PossibleRates.FindLastByPredicate([=](const int32& TestRate)
		// {
		// 	// 帧数用户值必须高于等于我们的设定值
		// 	const bool bAtOrBelowDesiredRate = (TestRate <= FrameRate);
		//
		// 	// 获取我们的设定值对应的画面质量
		// 	const int32 LimitQuality = GetApplicableResolutionQualityLimit(TestRate);
		//
		// 	// 我们的设定的画面之必须有效
		// 	// 我们设定的画面质量必须高于等于用户的需求质量
		// 	const bool bQualityDoesntExceedLimit = (LimitQuality < 0) || (OverallQuality <= LimitQuality);
		//
		//
		// 	// 我们的设定的帧数必须项目支持且硬件平台支持
		// 	const bool bIsSupported = ULyraSettingsLocal::IsSupportedMobileFramePace(TestRate);
		//
		// 	// 三者都满足的情况下.这组条件可以条件可以使用.
		// 	return bAtOrBelowDesiredRate && bQualityDoesntExceedLimit && bIsSupported;
		// });

		// return PossibleRates.IsValidIndex(LimitIndex)
		// 	       ? PossibleRates[LimitIndex]
		// 	       : ULyraSettingsLocal::GetDefaultMobileFrameRate();
		return 60;
	}

	// Returns the first frame rate at which overall quality is restricted/limited by the current device profile
	// 返回当前设备配置所限制/约束整体质量的首个帧率值
	int32 GetFirstFrameRateWithQualityLimit()
	{
		return OverallQualityLimits.GetFirstThreshold();
	}


	// Returns the lowest quality at which there's a limit on the overall frame rate (or -1 if there is no limit)
	// 返回整体帧率存在上限的最低质量等级（若不存在上限，则返回 -1）
	int32 GetLowestQualityWithFrameRateLimit()
	{
		return OverallQualityLimits.GetLowestValue(-1);
	}
}

//////////////////////////////////////////////////////////////////////
///
ULyraSettingsLocal::ULyraSettingsLocal()
{
	//如果不是CDO对象,并且当前App已经初始化好了
	// if (!HasAnyFlags(RF_ClassDefaultObject) && FSlateApplication::IsInitialized())
	// {
	// 	// 当APP的焦点状态发生变化了触发回调
	// 	OnApplicationActivationStateChangedHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().
	// 																		 AddUObject(
	// 																			 this,
	// 																			 &ThisClass::OnAppActivationStateChanged);
	//
	// 	
	// }


	/**
	 * 若设置为“false”，则不会将可扩展性设置（即“可扩展性质量”）视为用户设置
	 * 即这些设置不会被保存并应用。
	 *
	 * 读取平台渲染设置,设置是是否支持精细化的视频质量设置
	 */
	// bEnableScalabilitySettings = ULyraPlatformSpecificRenderingSettings::Get()->bSupportsGranularVideoQualitySettings;


	// SetToDefaults();
}

ULyraSettingsLocal* ULyraSettingsLocal::Get()
{
	return GEngine ? CastChecked<ULyraSettingsLocal>(GEngine->GetGameUserSettings()) : nullptr;
}
//
// void ULyraSettingsLocal::BeginDestroy()
// {
// 	if (FSlateApplication::IsInitialized())
// 	{
//
// 		FSlateApplication::Get().OnApplicationActivationStateChanged().
// 								 Remove(OnApplicationActivationStateChangedHandle);
// 		
// 	}
//
//
//
// 	
// 	Super::BeginDestroy();
// }
//
// void ULyraSettingsLocal::SetToDefaults()
// {
// 	Super::SetToDefaults();
//
// 	
// 	/** 是否启用耳机模式（头相关传输函数） **/
// 	bUseHeadphoneMode = false;
//
// 	/** 是否启用高动态范围音频模式（HDR 音频） **/
// 	bUseHDRAudioMode = false;
//
// 	// 是否已经加载SoundControlBusMix
// 	bSoundControlBusMixLoaded = false;
//
// 	// 如果为真，则游戏将通过 ILatencyMarkerModule 模块来记录延迟数据。
// 	// 这使您能够查看更多与延迟相关的性能数据。
// 	// 默认值会根据平台支持情况而定，若平台支持则设为真，否则设为假。
// 	 bEnableLatencyTrackingStats = ULyraSettingsLocal::DoesPlatformSupportLatencyTrackingStats();
// 	
// 	const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
//
// 	UserChosenDeviceProfileSuffix = PlatformSettings->DefaultDeviceProfileSuffix;
// 	DesiredUserChosenDeviceProfileSuffix = UserChosenDeviceProfileSuffix;
//
// 	// 菜单帧率限制
// 	FrameRateLimit_InMenu = 144.0f;
//
// 	// 背景帧率限制
// 	FrameRateLimit_WhenBackgrounded = 30.0f;
//
// 	// 电池帧率限制
// 	FrameRateLimit_OnBattery = 60.0f;
//
// 	// 移动平台帧率限制
// 	MobileFrameRateLimit = GetDefaultMobileFrameRate();
// 	
// 	
// 	// 需求的移动平台帧率限制
// 	DesiredMobileFrameRateLimit = MobileFrameRateLimit;
// 	
//
// 	
// }
//
// void ULyraSettingsLocal::LoadSettings(bool bForceReload)
// {
// 	Super::LoadSettings(bForceReload);
//
// 	// Console platforms use rhi.SyncInterval to limit framerate
// 	// 控制台平台通过 rhi.SyncInterval 来限制帧率
// 	const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
// 	if (PlatformSettings->FramePacingMode == ELyraFramePacingMode::ConsoleStyle)
// 	{
// 		/** 帧率限制 */
// 		FrameRateLimit = 0.0f;
// 	}
//
// 	// Enable HRTF if needed
// 	// 如有需要，则启用头相关滤波器功能
// 	bDesiredHeadphoneMode = bUseHeadphoneMode;
// 	SetHeadphoneModeEnabled(bUseHeadphoneMode);
//
// 	// 应用潜在的性能追踪设置
// 	ApplyLatencyTrackingStatSetting();
//
// 	// 获取驱动文件后缀
// 	DesiredUserChosenDeviceProfileSuffix = UserChosenDeviceProfileSuffix;
//
// 	// 填充默认的平台拓展性能配置
// 	LyraSettingsHelpers::FillScalabilitySettingsFromDeviceProfile(DeviceDefaultScalabilitySettings);
//
// 	// 获取移动端需求帧率
// 	DesiredMobileFrameRateLimit = MobileFrameRateLimit;
//
// 	// 调整移动端质量
// 	ClampMobileQuality();
// 	
//
// 	// 性能配置发生了改变
// 	PerfStatSettingsChangedEvent.Broadcast();
//
// 	
// }
//
// void ULyraSettingsLocal::ConfirmVideoMode()
// {
// 	Super::ConfirmVideoMode();
//
// 	SetMobileFPSMode(DesiredMobileFrameRateLimit);
// }
//
// // Combines two limits, always taking the minimum of the two (with special handling for values of <= 0 meaning unlimited)
// float CombineFrameRateLimits(float Limit1, float Limit2)
// {
// 	if (Limit1 <= 0.0f)
// 	{
// 		return Limit2;
// 	}
// 	else if (Limit2 <= 0.0f)
// 	{
// 		return Limit1;
// 	}
// 	else
// 	{
// 		return FMath::Min(Limit1, Limit2);
// 	}
// }
//
//
// float ULyraSettingsLocal::GetEffectiveFrameRateLimit()
// {
// 	const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
//
// #if WITH_EDITOR
// 	if (GIsEditor && !CVarApplyFrameRateSettingsInPIE.GetValueOnGameThread())
// 	{
// 		return Super::GetEffectiveFrameRateLimit();
// 	}
//
// #endif
//
// 	if (PlatformSettings->FramePacingMode == ELyraFramePacingMode::ConsoleStyle)
// 	{
// 		return 0.0f;
// 	}
//
//
// 	float EffectiveFrameRateLimit = Super::GetEffectiveFrameRateLimit();
//
//
// 	if (ShouldUseFrontendPerformanceSettings())
// 	{
// 		// 选择两者的最小值
// 		EffectiveFrameRateLimit = CombineFrameRateLimits(EffectiveFrameRateLimit, FrameRateLimit_InMenu);
// 	}
//
// 	if (PlatformSettings->FramePacingMode == ELyraFramePacingMode::DesktopStyle)
// 	{
// 		// 是否使用电池
// 		if (FPlatformMisc::IsRunningOnBattery())
// 		{
// 			EffectiveFrameRateLimit = CombineFrameRateLimits(EffectiveFrameRateLimit, FrameRateLimit_OnBattery);
// 		}
// 		// 是否是在后台
// 		if (FSlateApplication::IsInitialized() && !FSlateApplication::Get().IsActive())
// 		{
// 			EffectiveFrameRateLimit = CombineFrameRateLimits(EffectiveFrameRateLimit, FrameRateLimit_WhenBackgrounded);
// 		}
// 	}
//
// 	return EffectiveFrameRateLimit;
// }
//
// void ULyraSettingsLocal::ResetToCurrentSettings()
// {
// 	Super::ResetToCurrentSettings();
//
// 	bDesiredHeadphoneMode = bUseHeadphoneMode;
//
// 	UserChosenDeviceProfileSuffix = DesiredUserChosenDeviceProfileSuffix;
//
// 	MobileFrameRateLimit = DesiredMobileFrameRateLimit;
// 	
// 	
// }
//
// void ULyraSettingsLocal::ApplyNonResolutionSettings()
// {
// 	Super::ApplyNonResolutionSettings();
//
// 	// Check if Control Bus Mix references have been loaded,
// 	// Might be false if applying non resolution settings without touching any of the setters from UI
//
// 	// 检查控制总线混合参考是否已加载，
// 	// 如果未更改任何来自用户界面的设置而直接应用非解析设置，则该值可能为假。
//
// 	if (!bSoundControlBusMixLoaded)
// 	{
// 		LoadUserControlBusMix();
// 	}
//
// 	// In this section, update each Control Bus to the currently cached UI settings
// 	// 在此部分，将每个控制总线更新为当前缓存的用户界面设置。
// 	{
// 		// 重新设置总体音量大小
// 		if (TObjectPtr<USoundControlBus>* ControlBusDblPtr = ControlBusMap.Find(TEXT("Overall")))
// 		{
// 			if (USoundControlBus* ControlBusPtr = *ControlBusDblPtr)
// 			{
// 				SetVolumeForControlBus(ControlBusPtr, OverallVolume);
// 			}
// 		}
//
// 		// 重新设置音乐音量大小
// 		if (TObjectPtr<USoundControlBus>* ControlBusDblPtr = ControlBusMap.Find(TEXT("Music")))
// 		{
// 			if (USoundControlBus* ControlBusPtr = *ControlBusDblPtr)
// 			{
// 				SetVolumeForControlBus(ControlBusPtr, MusicVolume);
// 			}
// 		}
//
// 		// 重新设置音效音量大小
// 		if (TObjectPtr<USoundControlBus>* ControlBusDblPtr = ControlBusMap.Find(TEXT("SoundFX")))
// 		{
// 			if (USoundControlBus* ControlBusPtr = *ControlBusDblPtr)
// 			{
// 				SetVolumeForControlBus(ControlBusPtr, SoundFXVolume);
// 			}
// 		}
//
//
// 		// 重新设置对话音量大小
// 		if (TObjectPtr<USoundControlBus>* ControlBusDblPtr = ControlBusMap.Find(TEXT("Dialogue")))
// 		{
// 			if (USoundControlBus* ControlBusPtr = *ControlBusDblPtr)
// 			{
// 				SetVolumeForControlBus(ControlBusPtr, DialogueVolume);
// 			}
// 		}
//
// 		// 重新设置聊天音量大小
// 		if (TObjectPtr<USoundControlBus>* ControlBusDblPtr = ControlBusMap.Find(TEXT("VoiceChat")))
// 		{
// 			if (USoundControlBus* ControlBusPtr = *ControlBusDblPtr)
// 			{
// 				SetVolumeForControlBus(ControlBusPtr, VoiceChatVolume);
// 			}
// 		}
// 	}
//
// 	// 设置输入类型
// 	if (UCommonInputSubsystem* InputSubsystem = UCommonInputSubsystem::Get(GetTypedOuter<ULocalPlayer>()))
// 	{
// 		InputSubsystem->SetGamepadInputType(ControllerPlatform);
// 	}
//
// 	// 设置是否开启耳机模式
// 	if (bUseHeadphoneMode != bDesiredHeadphoneMode)
// 	{
// 		SetHeadphoneModeEnabled(bDesiredHeadphoneMode);
// 	}
//
// 	// 设置用户选择平台后缀
// 	if (DesiredUserChosenDeviceProfileSuffix != UserChosenDeviceProfileSuffix)
// 	{
// 		UserChosenDeviceProfileSuffix = DesiredUserChosenDeviceProfileSuffix;
// 	}
//
// 	/**
// 	 * 检查此应用程序是否能够进行任何渲染操作。
// 	 * 某些应用程序类型永远不会进行渲染，而对于其他应用程序，其这种行为可以通过切换至 NullRHI 来控制。
// 	 * 这可用于做出诸如忽略在服务器或在无窗口模式下运行的游戏中的无意义代码路径之类的决策（例如自动化测试）。*
// 	 * @返回值：如果应用程序能够进行渲染则返回 true，否则返回 false。
// 	 * 
// 	 */
// 	if (FApp::CanEverRender())
// 	{
// 		// 设置伽马值
// 		ApplyDisplayGamma();
//
// 		// 设置完全区
// 		ApplySafeZoneScale();
//
//
// 		// 根据游戏模式配置文件和帧数
// 		UpdateGameModeDeviceProfileAndFps();
// 	}
//
//
// 	// 变更显示状态的控件
// 	PerfStatSettingsChangedEvent.Broadcast();
// }
//
// int32 ULyraSettingsLocal::GetOverallScalabilityLevel() const
// {
// 	int32 Result = Super::GetOverallScalabilityLevel();
//
// 	const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
// 	if (PlatformSettings->FramePacingMode == ELyraFramePacingMode::MobileStyle)
// 	{
// 		// 获取最高的一个级别
// 		Result = GetHighestLevelOfAnyScalabilityChannel();
// 		
// 	}
// 	
// 	return Result;
//
//
// 	
// }
//
// void ULyraSettingsLocal::SetOverallScalabilityLevel(int32 Value)
// {
// 	/**
// 	 * 用于保存/恢复值的异常安全保护机制。
// 	 * 常用于确保即使代码在未来提前退出，值也能被恢复。
// 	 * 使用方法：
// 	 *  	TGuardValue<bool> GuardSomeBool(bSomeBool， false)； // 将 bSomeBool 设置为 false，并在析构函数中恢复其值。
// 	 *  	
// 	 */
// 	TGuardValue Guard(bSettingOverallQualityGuard, true);
//
// 	Value = FMath::Clamp(Value, 0, 3);
//
// 	float CurrentMobileResolutionQuality = ScalabilityQuality.ResolutionQuality;
//
// 	Super::SetOverallScalabilityLevel(Value);
//
// 	const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
// 	if (PlatformSettings->FramePacingMode == ELyraFramePacingMode::MobileStyle)
// 	{
// 		// Restore the resolution quality, mobile decouples this from overall quality
// 		// 恢复分辨率质量，移动端将此功能与整体质量分离开来
// 		ScalabilityQuality.ResolutionQuality = CurrentMobileResolutionQuality;
//
// 		// Changing the overall quality can end up adjusting the frame rate on mobile since there are limits
// 		// 改变整体画质可能会导致在移动端调整帧率，因为存在一定的限制。
// 		const int32 ConstrainedFrameRateLimit = LyraSettingsHelpers::ConstrainFrameRateToBeCompatibleWithOverallQuality(
// 			DesiredMobileFrameRateLimit, Value);
//
// 		if (ConstrainedFrameRateLimit != DesiredMobileFrameRateLimit)
// 		{
// 			SetDesiredMobileFrameRateLimit(ConstrainedFrameRateLimit);
// 		}
//
// 		
// 	}
// 	
// 	
// }

void ULyraSettingsLocal::OnExperienceLoaded()
{
	// ReapplyThingsDueToPossibleDeviceProfileChange();
}

// void ULyraSettingsLocal::OnHotfixDeviceProfileApplied()
// {
// 	ReapplyThingsDueToPossibleDeviceProfileChange();
// }
//
// void ULyraSettingsLocal::SetShouldUseFrontendPerformanceSettings(bool bInFrontEnd)
// {
// 	bInFrontEndForPerformancePurposes = bInFrontEnd;
// 	UpdateEffectiveFrameRateLimit();
// }
//
// bool ULyraSettingsLocal::ShouldUseFrontendPerformanceSettings() const
// {
// #if WITH_EDITOR
// 	// 编辑器下如果没有通过命令行开启 则无法使用
// 	if (GIsEditor && !CVarApplyFrontEndPerformanceOptionsInPIE.GetValueOnGameThread())
// 	{
// 		return false;
// 	}
//
// #endif
// 	return bInFrontEndForPerformancePurposes;
// }
//
// ELyraStatDisplayMode ULyraSettingsLocal::GetPerfStatDisplayState(ELyraDisplayablePerformanceStat Stat) const
// {
// 	if (const ELyraStatDisplayMode* pMode = DisplayStatList.Find(Stat))
// 	{
// 		return *pMode;
// 	}
// 	else
// 	{
// 		return ELyraStatDisplayMode::Hidden;
// 	}
// }
//
// void ULyraSettingsLocal::SetPerfStatDisplayState(ELyraDisplayablePerformanceStat Stat, ELyraStatDisplayMode DisplayMode)
// {
// 	if (DisplayMode == ELyraStatDisplayMode::Hidden)
// 	{
// 		DisplayStatList.Remove(Stat);
// 	}
// 	else
// 	{
// 		DisplayStatList.FindOrAdd(Stat) = DisplayMode;
// 	}
//
// 	PerfStatSettingsChangedEvent.Broadcast();
// }
//
// void ULyraSettingsLocal::ApplyLatencyTrackingStatSetting()
// {
// 	// Since this function will be called on load of the settings, we check if the slate app is initalized.
// 	// If it isn't then we are not in a target which can even have latency stats (like a headless cooker) so we
// 	// will exit early and do nothing.
// 	// 由于此函数将在设置加载时被调用，因此我们先检查滑板应用程序是否已初始化。
// 	// 如果尚未初始化，那么当前环境显然不具备可以记录延迟数据的条件（比如无显示的烹饪设备），所以我们将提前退出并不做任何处理。
// 	if (!FSlateApplication::IsInitialized())
// 	{
// 		return;
// 	}
//
// 	// Don't bother doing anything if the platform doesn't even support tracking stats.
// 	// 如果该平台甚至都不支持统计追踪功能，那就别费心去做任何事情了。
// 	if (!DoesPlatformSupportLatencyTrackingStats())
// 	{
// 		return;
// 	}
//
// 	// Actually enable or disable the latency marker modules based on this setting
// 	// 实际上会根据此设置来启用或禁用延迟标记模块
// 	TArray<ILatencyMarkerModule*> LatencyMarkerModules = IModularFeatures::Get().GetModularFeatureImplementations<
// 		ILatencyMarkerModule>(ILatencyMarkerModule::GetModularFeatureName());
//
// 	
// 	for (ILatencyMarkerModule* LatencyMarkerModule : LatencyMarkerModules)
// 	{
//
// 		/*
// 		 * bEnableLatencyTrackingStats
// 		 *
// 		 * 如果为真，则游戏将通过 ILatencyMarkerModule 模块来记录延迟数据。
// 		 * 这使您能够查看更多与延迟相关的性能数据。
// 		 * 默认值会根据平台支持情况而定，若平台支持则设为真，否则设为假。
// 		 */
// 		LatencyMarkerModule->SetEnabled(bEnableLatencyTrackingStats);
//
// 		
// 	}
// 	
// 	// 打印一个日志 开启或禁用了 多少潜在性能追踪标记模块.
// 	UE_CLOG(!LatencyMarkerModules.IsEmpty(),
// 			LogConsoleResponse,
// 			Log,
// 			TEXT("%s %d Latency Marker Module(s)"),
// 			bEnableLatencyTrackingStats ? TEXT("Enabled") : TEXT("Disabled"), LatencyMarkerModules.Num());
//
//
// 	
// }
//
// bool ULyraSettingsLocal::DoesPlatformSupportLatencyTrackingStats()
// {
// 	// 通过平台特征获取
// 	return ICommonUIModule::GetSettings().GetPlatformTraits().HasTag(
// 		PerfStatTags::TAG_Platform_Trait_SupportsLatencyStats);
// }
//
// void ULyraSettingsLocal::SetEnableLatencyTrackingStats(const bool bNewVal)
// {
// 	if (bNewVal != bEnableLatencyTrackingStats)
// 	{
// 		bEnableLatencyTrackingStats = bNewVal;
//
// 		ApplyLatencyTrackingStatSetting();
//
// 		
// 		LatencyStatIndicatorSettingsChangedEvent.Broadcast();
// 	}
//
// 	
// }
//
// bool ULyraSettingsLocal::DoesPlatformSupportLatencyMarkers()
// {
// 	return ICommonUIModule::GetSettings().GetPlatformTraits().HasTag(
// 	PerfStatTags::TAG_Platform_Trait_SupportsLatencyMarkers);
// }
//
// void ULyraSettingsLocal::SetEnableLatencyFlashIndicators(const bool bNewVal)
// {
// 	if (bNewVal != bEnableLatencyFlashIndicators)
// 	{
// 		bEnableLatencyFlashIndicators = bNewVal;
// 		LatencyFlashInidicatorSettingsChangedEvent.Broadcast();
// 		
// 	}
//
// 	
// }
//
// float ULyraSettingsLocal::GetDisplayGamma() const
// {
// 	return DisplayGamma;
// }
//
// void ULyraSettingsLocal::SetDisplayGamma(float InGamma)
// {
// 	DisplayGamma = InGamma;
// 	ApplyDisplayGamma();
// }
//
// void ULyraSettingsLocal::ApplyDisplayGamma()
// {
// 	if (GEngine)
// 	{
// 		GEngine->DisplayGamma = DisplayGamma;
// 	}
// }
//
// void ULyraSettingsLocal::ClampMobileResolutionQuality(int32 TargetFPS)
// {
// 	// Clamp mobile resolution quality
// 	// 对移动设备的分辨率质量进行限制
// 	float MaxMobileResQuality = LyraSettingsHelpers::GetApplicableResolutionQualityLimit(TargetFPS);
// 	float CurrentScaleNormalized = 0.0f;
// 	float CurrentScaleValue = 0.0f;
// 	float MinScaleValue = 0.0f;
// 	float MaxScaleValue = 0.0f;
// 	GetResolutionScaleInformationEx(CurrentScaleNormalized, CurrentScaleValue, MinScaleValue, MaxScaleValue);
// 	if (CurrentScaleValue > MaxMobileResQuality)
// 	{
// 		UE_LOG(LogConsoleResponse, Log, TEXT("clamping mobile resolution quality max res: %f, %f, %f, %f, %f"),
// 			   CurrentScaleNormalized, CurrentScaleValue, MinScaleValue, MaxScaleValue, MaxMobileResQuality);
// 		// 设置当前的分辨率比例
// 		SetResolutionScaleValueEx(MaxMobileResQuality);
// 		
// 	}
// 	
//
// 	
// }
//
// void ULyraSettingsLocal::RemapMobileResolutionQuality(int32 FromFPS, int32 ToFPS)
// {
// 	// Mobile resolution quality slider is a normalized value that is lerped between min quality, max quality.
// 	// max quality can change depending on FPS mode. This code remaps the quality when FPS mode changes so that the normalized
// 	// value remains the same within the new range.
//
// 	// 移动设备分辨率质量滑块是一个标准化的数值，其值会在最低质量与最高质量之间进行线性插值。
// 	// 最高质量可能会根据帧率模式的不同而有所变化。此代码在帧率模式改变时会重新映射质量，以确保标准化数值在新的范围内保持不变。
// 	float CurrentScaleNormalized = 0.0f;
// 	float CurrentScaleValue = 0.0f;
// 	float MinScaleValue = 0.0f;
// 	float MaxScaleValue = 0.0f;
// 	GetResolutionScaleInformationEx(CurrentScaleNormalized, CurrentScaleValue, MinScaleValue, MaxScaleValue);
// 	float FromMaxMobileResQuality = LyraSettingsHelpers::GetApplicableResolutionQualityLimit(FromFPS);
// 	float ToMaxMobileResQuality = LyraSettingsHelpers::GetApplicableResolutionQualityLimit(ToFPS);
//
// 	float FromMobileScaledNormalizedValue = (CurrentScaleValue - MinScaleValue) / (FromMaxMobileResQuality -
// 		MinScaleValue);
// 	float ToResQuality = FMath::Lerp(MinScaleValue, ToMaxMobileResQuality, FromMobileScaledNormalizedValue);
//
// 	UE_LOG(LogConsoleResponse, Log, TEXT("Remap mobile resolution quality %f, %f, (%d,%d)"), CurrentScaleValue,
// 		   ToResQuality, FromFPS, ToFPS);
// 	// 设置当前的分辨率比例
// 	SetResolutionScaleValueEx(ToResQuality);
// 	
// }
//
// void ULyraSettingsLocal::ClampMobileFPSQualityLevels(bool bWriteBack)
// {
// 	const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
// 	if (PlatformSettings->FramePacingMode == ELyraFramePacingMode::MobileStyle)
// 	{
// 		const int32 QualityLimit = LyraSettingsHelpers::GetApplicableOverallQualityLimit(DesiredMobileFrameRateLimit);
// 		const int32 CurrentQualityLevel = GetHighestLevelOfAnyScalabilityChannel();
//
// 		// 如果超过了质量限制 就必须重设
// 		if ((QualityLimit >= 0) && (CurrentQualityLevel > QualityLimit))
// 		{
// 			SetOverallScalabilityLevel(QualityLimit);
//
// 			if (bWriteBack)
// 			{
//
// 				Scalability::SetQualityLevels(ScalabilityQuality);
// 			}
// 			UE_LOG(LogConsoleResponse, Log, TEXT("Mobile FPS clamped overall quality (%d -> %d)."), CurrentQualityLevel,
// 				   QualityLimit);
// 			
// 		}
// 		
// 		
// 	}
//
// 	
// 	
// }
//
// void ULyraSettingsLocal::ClampMobileQuality()
// {
// 	const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
// 	if (PlatformSettings->FramePacingMode == ELyraFramePacingMode::MobileStyle)
// 	{
// 		// Clamp the resultant settings to the device default, it's known viable maximum.
// 		// This is a clamp rather than override to preserve allowed user settings
//
// 		// 将生成的设置限制为设备的默认值，因为这是已知的可行的最大值。
// 		// 这是一种限制操作，而非覆盖操作，目的是保留用户允许设置的完整性。
//
// 		Scalability::FQualityLevels CurrentLevels = Scalability::GetQualityLevels();
//
// 		/** On mobile, disables the 3D Resolution clamp that reverts the setting set by the user on boot.*/
// 		/** 在移动端，会禁用 3D 分辨率限制功能，该功能会恢复用户在开机时所设置的参数值。*/
//
// 		bool bMobileDisableResolutionReset = true;
// 		if (bMobileDisableResolutionReset)
// 		{
// 			DeviceDefaultScalabilitySettings.Qualities.ResolutionQuality = CurrentLevels.ResolutionQuality;
// 		}
//
// 		/* 根据当前设备配置文件的默认允许范围对输入电平进行限制 */
// 		ClampQualityLevelsToDeviceProfile(DeviceDefaultScalabilitySettings.Qualities, /*inout*/ CurrentLevels);
//
// 		/** 这是设置当前状态的唯一推荐方法——切勿直接设置游戏变量（CVars） **/
// 		Scalability::SetQualityLevels(CurrentLevels);
//
// 		// Clamp quality levels if required at the current frame rate
// 		// 根据当前帧率对质量级别进行限制（如有必要）
// 		ClampMobileFPSQualityLevels(/*bWriteBack=*/ true);
// 		
//
// 		// 拿到最大的移动端帧率
// 		const int32 MaxMobileFrameRate = GetMaxMobileFrameRate();
//
// 		// 拿到默认的移动端帧率
// 		const int32 DefaultMobileFrameRate = GetDefaultMobileFrameRate();
//
// 		// 确保默认的帧率小于最大的
// 		ensureMsgf(DefaultMobileFrameRate <= MaxMobileFrameRate,
// 				   TEXT("Default mobile frame rate (%d) is higher than the maximum mobile frame rate (%d)!"),
// 				   DefaultMobileFrameRate, MaxMobileFrameRate);
//
// 		// Choose the closest supported frame rate to the user desired setting without going over the device imposed limit
// 		// 选择与用户期望设置最接近且不超过设备限制的可支持帧率。
// 		const TArray<int32>& PossibleRates = PlatformSettings->MobileFrameRateLimits;
// 		const int32 LimitIndex = PossibleRates.FindLastByPredicate([this](const int32& TestRate)
// 		{
// 			return (TestRate <= DesiredMobileFrameRateLimit) && IsSupportedMobileFramePace(TestRate);
// 		});
// 		
// 		const int32 ActualLimitFPS = PossibleRates.IsValidIndex(LimitIndex)
// 								 ? PossibleRates[LimitIndex]
// 								 : GetDefaultMobileFrameRate();
//
// 		// 根据帧率设置分辨率
// 		ClampMobileResolutionQuality(ActualLimitFPS);
// 		
// 		
// 		
// 	}
// }
//
// int32 ULyraSettingsLocal::GetHighestLevelOfAnyScalabilityChannel() const
// {
// 	return LyraSettingsHelpers::GetHighestLevelOfAnyScalabilityChannel(ScalabilityQuality);
// }
//
// void ULyraSettingsLocal::OverrideQualityLevelsToScalabilityMode(const FLyraScalabilitySnapshot& InMode,
// 	Scalability::FQualityLevels& InOutLevels)
// {
// 	static_assert(sizeof(Scalability::FQualityLevels) == 88,
// 				  "This function may need to be updated to account for new members");
//
// 	// Overrides any valid (non-negative) settings
// 	InOutLevels.ResolutionQuality = (InMode.Qualities.ResolutionQuality >= 0.f)
// 		                                ? InMode.Qualities.ResolutionQuality
// 		                                : InOutLevels.ResolutionQuality;
// 	InOutLevels.ViewDistanceQuality = (InMode.Qualities.ViewDistanceQuality >= 0)
// 		                                  ? InMode.Qualities.ViewDistanceQuality
// 		                                  : InOutLevels.ViewDistanceQuality;
// 	InOutLevels.AntiAliasingQuality = (InMode.Qualities.AntiAliasingQuality >= 0)
// 		                                  ? InMode.Qualities.AntiAliasingQuality
// 		                                  : InOutLevels.AntiAliasingQuality;
// 	InOutLevels.ShadowQuality = (InMode.Qualities.ShadowQuality >= 0)
// 		                            ? InMode.Qualities.ShadowQuality
// 		                            : InOutLevels.ShadowQuality;
// 	InOutLevels.GlobalIlluminationQuality = (InMode.Qualities.GlobalIlluminationQuality >= 0)
// 		                                        ? InMode.Qualities.GlobalIlluminationQuality
// 		                                        : InOutLevels.GlobalIlluminationQuality;
// 	InOutLevels.ReflectionQuality = (InMode.Qualities.ReflectionQuality >= 0)
// 		                                ? InMode.Qualities.ReflectionQuality
// 		                                : InOutLevels.ReflectionQuality;
// 	InOutLevels.PostProcessQuality = (InMode.Qualities.PostProcessQuality >= 0)
// 		                                 ? InMode.Qualities.PostProcessQuality
// 		                                 : InOutLevels.PostProcessQuality;
// 	InOutLevels.TextureQuality = (InMode.Qualities.TextureQuality >= 0)
// 		                             ? InMode.Qualities.TextureQuality
// 		                             : InOutLevels.TextureQuality;
// 	InOutLevels.EffectsQuality = (InMode.Qualities.EffectsQuality >= 0)
// 		                             ? InMode.Qualities.EffectsQuality
// 		                             : InOutLevels.EffectsQuality;
// 	InOutLevels.FoliageQuality = (InMode.Qualities.FoliageQuality >= 0)
// 		                             ? InMode.Qualities.FoliageQuality
// 		                             : InOutLevels.FoliageQuality;
// 	InOutLevels.ShadingQuality = (InMode.Qualities.ShadingQuality >= 0)
// 		                             ? InMode.Qualities.ShadingQuality
// 		                             : InOutLevels.ShadingQuality;
// 	
//
// 	
// }
//
// void ULyraSettingsLocal::ClampQualityLevelsToDeviceProfile(const Scalability::FQualityLevels& ClampLevels,
//                                                            Scalability::FQualityLevels& InOutLevels)
// {
// 	static_assert(sizeof(Scalability::FQualityLevels) == 88,
// 	              "This function may need to be updated to account for new members");
//
//
// 	// Clamps any valid (non-negative) settings
// 	// 对所有有效的（非负）设置进行限制
// 	InOutLevels.ResolutionQuality = (ClampLevels.ResolutionQuality >= 0.f)
// 		                                ? FMath::Min(ClampLevels.ResolutionQuality, InOutLevels.ResolutionQuality)
// 		                                : InOutLevels.ResolutionQuality;
// 	InOutLevels.ViewDistanceQuality = (ClampLevels.ViewDistanceQuality >= 0)
// 		                                  ? FMath::Min(ClampLevels.ViewDistanceQuality, InOutLevels.ViewDistanceQuality)
// 		                                  : InOutLevels.ViewDistanceQuality;
// 	InOutLevels.AntiAliasingQuality = (ClampLevels.AntiAliasingQuality >= 0)
// 		                                  ? FMath::Min(ClampLevels.AntiAliasingQuality, InOutLevels.AntiAliasingQuality)
// 		                                  : InOutLevels.AntiAliasingQuality;
// 	InOutLevels.ShadowQuality = (ClampLevels.ShadowQuality >= 0)
// 		                            ? FMath::Min(ClampLevels.ShadowQuality, InOutLevels.ShadowQuality)
// 		                            : InOutLevels.ShadowQuality;
// 	InOutLevels.GlobalIlluminationQuality = (ClampLevels.GlobalIlluminationQuality >= 0)
// 		                                        ? FMath::Min(ClampLevels.GlobalIlluminationQuality,
// 		                                                     InOutLevels.GlobalIlluminationQuality)
// 		                                        : InOutLevels.GlobalIlluminationQuality;
// 	InOutLevels.ReflectionQuality = (ClampLevels.ReflectionQuality >= 0)
// 		                                ? FMath::Min(ClampLevels.ReflectionQuality, InOutLevels.ReflectionQuality)
// 		                                : InOutLevels.ReflectionQuality;
// 	InOutLevels.PostProcessQuality = (ClampLevels.PostProcessQuality >= 0)
// 		                                 ? FMath::Min(ClampLevels.PostProcessQuality, InOutLevels.PostProcessQuality)
// 		                                 : InOutLevels.PostProcessQuality;
// 	InOutLevels.TextureQuality = (ClampLevels.TextureQuality >= 0)
// 		                             ? FMath::Min(ClampLevels.TextureQuality, InOutLevels.TextureQuality)
// 		                             : InOutLevels.TextureQuality;
// 	InOutLevels.EffectsQuality = (ClampLevels.EffectsQuality >= 0)
// 		                             ? FMath::Min(ClampLevels.EffectsQuality, InOutLevels.EffectsQuality)
// 		                             : InOutLevels.EffectsQuality;
// 	InOutLevels.FoliageQuality = (ClampLevels.FoliageQuality >= 0)
// 		                             ? FMath::Min(ClampLevels.FoliageQuality, InOutLevels.FoliageQuality)
// 		                             : InOutLevels.FoliageQuality;
// 	InOutLevels.ShadingQuality = (ClampLevels.ShadingQuality >= 0)
// 		                             ? FMath::Min(ClampLevels.ShadingQuality, InOutLevels.ShadingQuality)
// 		                             : InOutLevels.ShadingQuality;
// }
//
// void ULyraSettingsLocal::SetDesiredMobileFrameRateLimit(int32 NewLimitFPS)
// {
// 	const int32 OldLimitFPS = DesiredMobileFrameRateLimit;
//
// 	RemapMobileResolutionQuality(OldLimitFPS, NewLimitFPS);
//
// 	DesiredMobileFrameRateLimit = NewLimitFPS;
// 	
// 	ClampMobileFPSQualityLevels(/*bWriteBack=*/ false);
// }
//
// FString ULyraSettingsLocal::GetDesiredDeviceProfileQualitySuffix() const
// {
// 	return DesiredUserChosenDeviceProfileSuffix;
// }
//
// void ULyraSettingsLocal::SetDesiredDeviceProfileQualitySuffix(const FString& InDesiredSuffix)
// {
// 	DesiredUserChosenDeviceProfileSuffix = InDesiredSuffix;
// }
//
// void ULyraSettingsLocal::UpdateGameModeDeviceProfileAndFps()
// {
// #if WITH_EDITOR
//
// 	// 如果没有在编辑器下开启平台模拟 则跳过
// 	if (GIsEditor && !CVarApplyDeviceProfilesInPIE.GetValueOnGameThread())
// 	{
// 		return;
// 	}
//
//
// #endif
//
// 	UDeviceProfileManager& Manager = UDeviceProfileManager::Get();
//
// 	// 获取到平台特定渲染配置
// 	const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
//
// 	// 读取用户自定义的配置
// 	const TArray<FLyraQualityDeviceProfileVariant>& UserFacingVariants = PlatformSettings->
// 		UserFacingDeviceProfileOptions;
//
// 	//@TODO: Might want to allow specific experiences to specify a suffix to attempt to use as well
// 	// The code below will handle searching with this suffix (alone or in conjunction with the frame rate), but nothing sets it right now
// 	//@待办事项：或许应该允许特定的体验设定一个后缀，以便尝试使用该后缀进行搜索（无论是单独使用还是与帧率结合使用），但目前还没有任何设置能够实现这一点。
// 	FString ExperienceSuffix;
//
// 	// Make sure the chosen setting is supported for the current display, walking down the list to try fallbacks
// 	// 确保所选设置适用于当前的显示设备，并逐个检查列表中的选项以尝试使用备用设置
// 	const int32 PlatformMaxRefreshRate = FPlatformMisc::GetMaxRefreshRate();
//
// 	// Lambda函数 比对后缀是否一致
// 	int32 SuffixIndex = UserFacingVariants.IndexOfByPredicate([&](const FLyraQualityDeviceProfileVariant& Data)
// 	{
// 		return Data.DeviceProfileSuffix == UserChosenDeviceProfileSuffix;
// 	});
//
// 	// 循环 保证能找到一个小于设备最大刷新率的索引,这样整个配置才可用
// 	while (UserFacingVariants.IsValidIndex(SuffixIndex))
// 	{
// 		if (PlatformMaxRefreshRate >= UserFacingVariants[SuffixIndex].MinRefreshRate)
// 		{
// 			break;
// 		}
// 		else
// 		{
// 			--SuffixIndex;
// 		}
// 	}
//
// 	// 获取一个有效用户配置后缀
// 	//  此处应该是空的
// 	const FString EffectiveUserSuffix = UserFacingVariants.IsValidIndex(SuffixIndex)
// 		                                    ? UserFacingVariants[SuffixIndex].DeviceProfileSuffix
// 		                                    : PlatformSettings->DefaultDeviceProfileSuffix;
//
//
// 	// Build up a list of names to try
// 	// 构建一个需要尝试的姓名列表
//
// 	// 此处是空的 所以应该是没有
// 	const bool bHadUserSuffix = !EffectiveUserSuffix.IsEmpty();
//
// 	// 此处是空的 并没有接入Experience的设置
// 	const bool bHadExperienceSuffix = !ExperienceSuffix.IsEmpty();
//
// 	/**
// 	 * 获取所选设备的配置文件名称，该名称可以是平台名称，也可以是设备配置文件选择模块所提供的名称。*
// 	 * @返回 选中的配置文件。
// 	 * 
// 	 */
// 	FString BasePlatformName = UDeviceProfileManager::GetPlatformDeviceProfileName();
//
// 	FName PlatformName; // Default unless in editor// 默认情况下如此，但在编辑器中则不然
// #if WITH_EDITOR
// 	if (GIsEditor)
// 	{
// 		// 编辑器的情况下去拿到平台的模拟设置
// 		const ULyraPlatformEmulationSettings* Settings = GetDefault<ULyraPlatformEmulationSettings>();
// 		const FName PretendBaseDeviceProfile = Settings->GetPretendBaseDeviceProfile();
//
//
// 		if (PretendBaseDeviceProfile != NAME_None)
// 		{
// 			BasePlatformName = PretendBaseDeviceProfile.ToString();
// 		}
//
// 		PlatformName = Settings->GetPretendPlatformName();
// 	}
// #endif
//
//
// 	TArray<FString> ComposedNamesToFind;
//
// 	// 如果既有体验设置也有用户设置
// 	if (bHadExperienceSuffix && bHadUserSuffix)
// 	{
// 		ComposedNamesToFind.Add(BasePlatformName + TEXT("_") + ExperienceSuffix + TEXT("_") + EffectiveUserSuffix);
// 	}
//
// 	// 有用户设置
// 	if (bHadUserSuffix)
// 	{
// 		ComposedNamesToFind.Add(BasePlatformName + TEXT("_") + EffectiveUserSuffix);
// 	}
//
// 	// 有体验设置
// 	if (bHadExperienceSuffix)
// 	{
// 		ComposedNamesToFind.Add(BasePlatformName + TEXT("_") + ExperienceSuffix);
// 	}
//
// 	// 编辑器下
// 	if (GIsEditor)
// 	{
// 		ComposedNamesToFind.Add(BasePlatformName);
// 	}
//
// 	// See if any of the potential device profiles actually exists
// 	// 查看是否有任何潜在的设备配置确实存在
//
// 	FString ActualProfileToApply;
// 	for (const FString& TestProfileName : ComposedNamesToFind)
// 	{
// 		/**
// 		 * 用于检查是否可调用“CreateProfile”函数来使用指定的设备配置文件的测试操作。*
// 		 * @参数 ProfileName - 账户名称。
// 		 * @参数 ProfileToCopy - 要复制的账户名称。*
// 		  * @返回创建的配置文件。
// 		 * 
// 		 */
// 		if (Manager.HasLoadableProfileName(TestProfileName, PlatformName))
// 		{
// 			ActualProfileToApply = TestProfileName;
//
// 			/**
// 			 * 根据姓名查找相应的资料。
// 			 * @参数 ProfileName - 要查找的配置文件名称。
// 			 * @参数 bCreateProfileOnFail - 若对象尚未存在，则是否从配置中创建该配置文件。
// 			 * @参数 OptionalPlatformName - 用于加载的平台名称。
// 			 * @返回 找到的配置文件。
// 			 * 
// 			 */
// 			UDeviceProfile* Profile = Manager.FindProfile(TestProfileName, /*bCreateOnFail=*/ false);
// 			if (Profile == nullptr)
// 			{
// 				/**
// 				 * 从副本中创建设备配置文件的副本。
// 				 * @参数 ProfileName - 账户名称。
// 				 * @参数 ProfileToCopy - 要复制的账户名称。
// 				 * @返回创建的配置文件。
// 				 * 
// 				 */
// 				Profile = Manager.CreateProfile(TestProfileName, TEXT(""), TestProfileName, *PlatformName.ToString());
// 			}
// 			UE_LOG(LogConsoleResponse, Log, TEXT("Profile %s exists"), *Profile->GetName());
// 			break;
// 		}
// 	}
//
// 	UE_LOG(LogConsoleResponse, Log,
// 	       TEXT(
// 		       "UpdateGameModeDeviceProfileAndFps MaxRefreshRate=%d, ExperienceSuffix='%s', UserPicked='%s'->'%s', PlatformBase='%s', AppliedActual='%s'"
// 	       ),
// 	       PlatformMaxRefreshRate, *ExperienceSuffix, *UserChosenDeviceProfileSuffix, *EffectiveUserSuffix,
// 	       *BasePlatformName, *ActualProfileToApply);
//
// 	// Apply the device profile if it's different to what we currently have
// 	// 如果设备的配置与我们当前所使用的配置不同，则应用该设备配置。
// 	if (ActualProfileToApply != CurrentAppliedDeviceProfileOverrideSuffix)
// 	{
// 		if (Manager.GetActiveDeviceProfileName() != ActualProfileToApply)
// 		{
// 			// Restore the default first
// 			// 恢复默认设置（首先）
// 			if (GIsEditor)
// 			{
// #if ALLOW_OTHER_PLATFORM_CONFIG
// 				/**
// 				 * 恢复预览状态。
// 				 * 
// 				 */
// 				Manager.RestorePreviewDeviceProfile();
// #endif
// 			}
// 			else
// 			{
// 				/**
// 				 * 将设备配置恢复为该设备的默认设置
// 				 */
// 				Manager.RestoreDefaultDeviceProfile();
// 			}
//
// 			// Apply the new one (if it wasn't the default)
// 			// 应用新的设置（如果该设置并非默认值）
//
// 			if (Manager.GetActiveDeviceProfileName() != ActualProfileToApply)
// 			{
// 				UDeviceProfile* NewDeviceProfile = Manager.FindProfile(ActualProfileToApply);
// 				ensureMsgf(NewDeviceProfile != nullptr, TEXT("DeviceProfile %s not found "), *ActualProfileToApply);
//
// 				if (NewDeviceProfile)
// 				{
// 					if (GIsEditor)
// 					{
// #if ALLOW_OTHER_PLATFORM_CONFIG
// 						UE_LOG(LogConsoleResponse, Log, TEXT("Overriding *preview* device profile to %s"),
// 						       *ActualProfileToApply);
//
// 						Manager.SetPreviewDeviceProfile(NewDeviceProfile);
//
// 						// Reload the default settings from the pretend profile
// 						// 从模拟配置文件中重新加载默认设置
// 						// 这一步就是把当前设置填充到我们的默认拓展里面,因为我们根据配置文件进行了修改.
// 						LyraSettingsHelpers::FillScalabilitySettingsFromDeviceProfile(DeviceDefaultScalabilitySettings);
// #endif
// 					}
// 					else
// 					{
// 						UE_LOG(LogConsoleResponse, Log, TEXT("Overriding device profile to %s"), *ActualProfileToApply);
// 						Manager.SetOverrideDeviceProfile(NewDeviceProfile);
//
// 						if (!bEnableScalabilitySettings)
// 						{
// 							// We don't support persistence of the scalability settings but at least we may
// 							// provide up to date values if anybody queries them using the settings API.
//
// 							// 我们不支持对可扩展性设置的持久保存功能，但至少如果有人通过设置 API 进行查询，我们仍能够提供最新的值。
// 							// 这里因为是没有开启自定义拓展性能设置,但是我们可以缓存一下.
// 							ScalabilityQuality = Scalability::GetQualityLevels();
// 						}
// 					}
// 				}
// 			}
// 		}
//
// 		CurrentAppliedDeviceProfileOverrideSuffix = ActualProfileToApply;
// 	}
//
// 	// 现在根据我们的帧率控制模型来修改帧率
// 	switch (PlatformSettings->FramePacingMode)
// 	{
// 	// 移动端模式
// 	case ELyraFramePacingMode::MobileStyle:
// 		UpdateMobileFramePacing();
// 		break;
// 	// 控制台模式
// 	case ELyraFramePacingMode::ConsoleStyle:
// 		UpdateConsoleFramePacing();
// 		break;
// 	// 桌面模式
// 	case ELyraFramePacingMode::DesktopStyle:
// 		UpdateDesktopFramePacing();
// 		break;
// 	}
// }
//
// void ULyraSettingsLocal::UpdateConsoleFramePacing()
// {
// 	// Apply device-profile-driven frame sync and frame pace
// 	// 应用基于设备配置文件的帧同步和帧频率设置
// 	const int32 FrameSyncType = CVarDeviceProfileDrivenFrameSyncType.GetValueOnGameThread();
// 	if (FrameSyncType != -1)
// 	{
// 		UE_LOG(LogConsoleResponse, Log, TEXT("Setting frame sync mode to %d."), FrameSyncType);
// 		SetSyncTypeCVar(FrameSyncType);
// 	}
//
// 	const int32 TargetFPS = CVarDeviceProfileDrivenTargetFps.GetValueOnGameThread();
// 	if (TargetFPS != -1)
// 	{
// 		UE_LOG(LogConsoleResponse, Log, TEXT("Setting frame pace to %d Hz."), TargetFPS);
// 		FPlatformRHIFramePacer::SetFramePace(TargetFPS);
//
// 		// Set the CSV metadata and analytics Fps mode strings
// 		// 设置 CSV 元数据和分析帧率模式字符串
// #if CSV_PROFILER
// 		const FString TargetFramerateString = FString::Printf(TEXT("%d"), TargetFPS);
// 		CSV_METADATA(TEXT("TargetFramerate"), *TargetFramerateString);
// #endif
// 	}
// }
//
// void ULyraSettingsLocal::UpdateDesktopFramePacing()
// {
// 	// For desktop the frame rate limit is handled by the parent class based on the value already
// 	// applied via UpdateEffectiveFrameRateLimit()
// 	// So this function is only doing 'second order' effects of desktop frame pacing preferences
// 	// 对于桌面端而言，帧率限制是由父类根据通过“UpdateEffectiveFrameRateLimit()”函数已应用的值来处理的
// 	// 因此，此函数仅执行桌面端帧率控制偏好所产生“二级”影响的效果
//
// 	const float TargetFPS = GetEffectiveFrameRateLimit();
// 	const float ClampedFPS = (TargetFPS <= 0.0f) ? 60.0f : FMath::Clamp(TargetFPS, 30.0f, 60.0f);
//
// 	UpdateDynamicResFrameTime(ClampedFPS);
// }
//
// void ULyraSettingsLocal::UpdateMobileFramePacing()
// {
// 	//@TODO: Handle different limits for in-front-end or low-battery mode on mobile
// 	//@待办事项：针对移动端的前端模式或低电量模式，处理不同的限制条件。
//
// 	// Choose the closest supported frame rate to the user desired setting without going over the device imposed limit
// 	// 选择与用户期望设置最接近且不超过设备限制的可支持帧率。
// 	const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
// 	const TArray<int32>& PossibleRates = PlatformSettings->MobileFrameRateLimits;
//
// 	// Lambda函数 获取到最大可用的帧率
// 	const int32 LimitIndex = PossibleRates.FindLastByPredicate([this](const int32& TestRate)
// 	{
// 		return (TestRate <= MobileFrameRateLimit) && IsSupportedMobileFramePace(TestRate);
// 	});
//
// 	// 是否可用,不可用的话就采用默认的配置即可
// 	const int32 TargetFPS = PossibleRates.IsValidIndex(LimitIndex)
// 		                        ? PossibleRates[LimitIndex]
// 		                        : GetDefaultMobileFrameRate();
//
// 	UE_LOG(LogConsoleResponse, Log, TEXT("Setting frame pace to %d Hz."), TargetFPS);
// 	/**
// 	 * 设定我们希望达到的运行速度（30 表示 30 帧每秒，0 表示无固定节奏）。
// 	 * 通用实现会根据 FPlatformMisc:：GetMaxRefreshRate() 的值来设置 rhi.SyncInterval 的值。
// 	 * @返回我们将采用的跑步速度。
// 	 * 
// 	 */
// 	FPlatformRHIFramePacer::SetFramePace(TargetFPS);
//
// 	// 设置移动端质量
// 	ClampMobileQuality();
//
// 	// 更新动态分辨率帧时间
// 	UpdateDynamicResFrameTime((float)TargetFPS);
// }
//
// void ULyraSettingsLocal::UpdateDynamicResFrameTime(float TargetFPS)
// {
// 	static IConsoleVariable* CVarDyResFrameTimeBudget = IConsoleManager::Get().FindConsoleVariable(
// 		TEXT("r.DynamicRes.FrameTimeBudget"));
//
// 	if (CVarDyResFrameTimeBudget)
// 	{
// 		if (ensure(TargetFPS > 0.0f))
// 		{
// 			const float DyResFrameTimeBudget = 1000.0f / TargetFPS;
// 			CVarDyResFrameTimeBudget->Set(DyResFrameTimeBudget, ECVF_SetByGameSetting);
// 		}
// 	}
// }
//
// float ULyraSettingsLocal::GetFrameRateLimit_OnBattery() const
// {
// 	return FrameRateLimit_OnBattery;
// }
//
// void ULyraSettingsLocal::SetFrameRateLimit_OnBattery(float NewLimitFPS)
// {
// 	FrameRateLimit_OnBattery = NewLimitFPS;
// 	UpdateEffectiveFrameRateLimit();
// }
//
// float ULyraSettingsLocal::GetFrameRateLimit_InMenu() const
// {
// 	return FrameRateLimit_InMenu;
// }
//
// void ULyraSettingsLocal::SetFrameRateLimit_InMenu(float NewLimitFPS)
// {
// 	FrameRateLimit_InMenu = NewLimitFPS;
// 	UpdateEffectiveFrameRateLimit();
// }
//
// float ULyraSettingsLocal::GetFrameRateLimit_WhenBackgrounded() const
// {
// 	return FrameRateLimit_WhenBackgrounded;
// }
//
// void ULyraSettingsLocal::SetFrameRateLimit_WhenBackgrounded(float NewLimitFPS)
// {
// 	FrameRateLimit_WhenBackgrounded = NewLimitFPS;
// 	UpdateEffectiveFrameRateLimit();
// }
//
// float ULyraSettingsLocal::GetFrameRateLimit_Always() const
// {
// 	return GetFrameRateLimit();
// }
//
// void ULyraSettingsLocal::SetFrameRateLimit_Always(float NewLimitFPS)
// {
// 	SetFrameRateLimit(NewLimitFPS);
// 	UpdateEffectiveFrameRateLimit();
// }
//
// void ULyraSettingsLocal::UpdateEffectiveFrameRateLimit()
// {
// 	// DS服务器不需要进行该项设置
// 	if (!IsRunningDedicatedServer())
// 	{
// 		// 设置最大帧率
// 		SetFrameRateLimitCVar(GetEffectiveFrameRateLimit());
// 	}
// }
//
//
// int32 ULyraSettingsLocal::GetDefaultMobileFrameRate()
// {
// 	return CVarDeviceProfileDrivenMobileDefaultFrameRate.GetValueOnGameThread();
// }
//
// int32 ULyraSettingsLocal::GetMaxMobileFrameRate()
// {
// 	return CVarDeviceProfileDrivenMobileMaxFrameRate.GetValueOnGameThread();
// }
//
// bool ULyraSettingsLocal::IsSupportedMobileFramePace(int32 TestFPS)
// {
// 	// 是否是默认的
// 	const bool bIsDefault = (TestFPS == GetDefaultMobileFrameRate());
//
// 	// 是否超出限制
// 	const bool bDoesNotExceedLimit = (TestFPS <= GetMaxMobileFrameRate());
//
// 	/**
// 	 * FPlatformRHIFramePacer::SupportsFramePace(TestFPS)
// 	 * 返回该硬件是否能够以指定的帧率进行帧率同步的处理
// 	 * 
// 	 */
// 	// Allow all paces in the editor, as we'd only be doing this when simulating another platform
// 	// 允许编辑器中所有操作，因为我们只会在这种情况下进行此类操作（即在模拟其他平台时）
// 	const bool bIsSupportedPace = FPlatformRHIFramePacer::SupportsFramePace(TestFPS) || GIsEditor;
//
//
// 	return bIsDefault || (bDoesNotExceedLimit && bIsSupportedPace);
// }
//
// int32 ULyraSettingsLocal::GetFirstFrameRateWithQualityLimit() const
// {
// 	return LyraSettingsHelpers::GetFirstFrameRateWithQualityLimit();
// 	
// }
//
// int32 ULyraSettingsLocal::GetLowestQualityWithFrameRateLimit() const
// {
// 	return LyraSettingsHelpers::GetLowestQualityWithFrameRateLimit();
// }
//
// void ULyraSettingsLocal::ResetToMobileDeviceDefaults()
// {
// 	// Reset frame rate
// 	// 重置帧率
// 	DesiredMobileFrameRateLimit = GetDefaultMobileFrameRate();
// 	MobileFrameRateLimit = DesiredMobileFrameRateLimit;
//
// 	// Reset scalability
// 	// 恢复可扩展性
// 	Scalability::FQualityLevels DefaultLevels = Scalability::GetQualityLevels();
// 	OverrideQualityLevelsToScalabilityMode(DeviceDefaultScalabilitySettings, DefaultLevels);
// 	ScalabilityQuality = DefaultLevels;
// 	
//
// 	// Apply
// 	// 应用
// 	UpdateGameModeDeviceProfileAndFps();
//
// 	
// }
//
// int32 ULyraSettingsLocal::GetMaxSupportedOverallQualityLevel() const
// {
// 	const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
// 	if ((PlatformSettings->FramePacingMode == ELyraFramePacingMode::MobileStyle) && DeviceDefaultScalabilitySettings.
// 		bHasOverrides)
// 	{
// 		return LyraSettingsHelpers::GetHighestLevelOfAnyScalabilityChannel(DeviceDefaultScalabilitySettings.Qualities);
// 	}
// 	else
// 	{
// 		return 3;
// 	}
// }
//
// void ULyraSettingsLocal::SetMobileFPSMode(int32 NewLimitFPS)
// {
// 	const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
// 	if (PlatformSettings->FramePacingMode == ELyraFramePacingMode::MobileStyle)
// 	{
// 		if (MobileFrameRateLimit != NewLimitFPS)
// 		{
// 			MobileFrameRateLimit = NewLimitFPS;
// 			UpdateGameModeDeviceProfileAndFps();
// 		}
//
// 		DesiredMobileFrameRateLimit = MobileFrameRateLimit;
// 	}
// }
//
// bool ULyraSettingsLocal::IsHeadphoneModeEnabled() const
// {
// 	return bUseHeadphoneMode;
// }
//
// void ULyraSettingsLocal::SetHeadphoneModeEnabled(bool bEnabled)
// {
// 	if (CanModifyHeadphoneModeEnabled())
// 	{
// 		static IConsoleVariable* BinauralSpatializationDisabledCVar = IConsoleManager::Get().FindConsoleVariable(
// 			TEXT("au.DisableBinauralSpatialization"));
// 		if (BinauralSpatializationDisabledCVar)
// 		{
// 			BinauralSpatializationDisabledCVar->Set(!bEnabled, ECVF_SetByGameSetting);
//
// 			// Only save settings if the setting actually changed
// 			// 仅在设置内容实际发生更改时才进行保存操作
// 			if (bUseHeadphoneMode != bEnabled)
// 			{
// 				bUseHeadphoneMode = bEnabled;
// 				/** 将用户设置保存至持久存储中（此操作会自动作为“应用设置”流程的一部分进行） */
// 				SaveSettings();
// 			}
// 		}
// 	}
// }
//
// bool ULyraSettingsLocal::CanModifyHeadphoneModeEnabled() const
// {
// 	// 读取命令行变量 两耳空间化是否被禁用.
// 	static IConsoleVariable* BinauralSpatializationDisabledCVar = IConsoleManager::Get().FindConsoleVariable(
// 		TEXT("au.DisableBinauralSpatialization"));
//
// 	// 是否可用 并追踪一下这个值的修改状态.
// 	// BinauralSpatializationDisabledCVar这个是一个指针,必须要这个对象指针存在才行.
// 	const bool bHRTFOptionAvailable = BinauralSpatializationDisabledCVar && ((BinauralSpatializationDisabledCVar->
// 		GetFlags() & EConsoleVariableFlags::ECVF_SetByMask) <= EConsoleVariableFlags::ECVF_SetByGameSetting);
//
//
// 	// 是否有平台特征 两耳通道被系统控制
// 	const bool bBinauralSettingControlledByOS = LyraSettingsHelpers::HasPlatformTrait(
// 		TAG_Platform_Trait_BinauralSettingControlledByOS);
//
// 	return bHRTFOptionAvailable && !bBinauralSettingControlledByOS;
// }
//
//
// bool ULyraSettingsLocal::IsHDRAudioModeEnabled() const
// {
// 	return bUseHDRAudioMode;
// }
//
// void ULyraSettingsLocal::SetHDRAudioModeEnabled(bool bEnabled)
// {
// 	bUseHDRAudioMode = bEnabled;
//
// 	if (GEngine)
// 	{
// 		if (const UWorld* World = GEngine->GetCurrentPlayWorld())
// 		{
// 			if (ULyraAudioMixEffectsSubsystem* LyraAudioMixEffectsSubsystem = World->GetSubsystem<
// 				ULyraAudioMixEffectsSubsystem>())
// 			{
// 				LyraAudioMixEffectsSubsystem->ApplyDynamicRangeEffectsChains(bEnabled);
// 			}
// 		}
// 	}
// }
//
// bool ULyraSettingsLocal::CanRunAutoBenchmark() const
// {
// 	const ULyraPlatformSpecificRenderingSettings* PlatformSettings = ULyraPlatformSpecificRenderingSettings::Get();
// 	return PlatformSettings->bSupportsAutomaticVideoQualityBenchmark;
// }
//
// bool ULyraSettingsLocal::ShouldRunAutoBenchmarkAtStartup() const
// {
// 	if (!CanRunAutoBenchmark())
// 	{
// 		return false;
// 	}
// 	
// 	if (LastCPUBenchmarkResult != -1)
// 	{
// 		// Already run and loaded
// 		// 已经运行并加载完成
// 		return false;
// 	}
//
// 	
// 	return true;
// }
//
// void ULyraSettingsLocal::RunAutoBenchmark(bool bSaveImmediately)
// {
// 	RunHardwareBenchmark();
// 	
// 	// Always apply, optionally save
// 	// 始终启用（可选：保存）
// 	ApplyScalabilitySettings();
// 	ApplyLatencyTrackingStatSetting();
//
// 	if (bSaveImmediately)
// 	{
// 		SaveSettings();
// 	}
// 	
// }
//
// void ULyraSettingsLocal::ApplyScalabilitySettings()
// {
// 	Scalability::SetQualityLevels(ScalabilityQuality);
// }
//
// float ULyraSettingsLocal::GetOverallVolume() const
// {
// 	return OverallVolume;
// }
//
// void ULyraSettingsLocal::SetOverallVolume(float InVolume)
// {
// 	// Cache the incoming volume value
// 	// 将传入的音量值缓存起来
// 	OverallVolume = InVolume;
//
//
// 	// Check to see if references to the control buses and control bus mixes have been loaded yet
// 	// Will likely need to be loaded if this function is the first time a setter has been called from the UI
// 	// 检查一下是否已经加载了控制总线和控制总线混合配置的相关内容
// 	// 如果此函数是首次从用户界面调用设置器函数，则很可能需要先加载这些内容
//
// 	if (!bSoundControlBusMixLoaded)
// 	{
// 		LoadUserControlBusMix();
// 	}
//
// 	// Ensure it's been loaded before continuing
// 	// 确保在继续操作之前该内容已加载完成
// 	ensureMsgf(bSoundControlBusMixLoaded, TEXT("UserControlBusMix Settings Failed to Load."));
//
// 	// Locate the locally cached bus and set the volume on it
// 	// 找到本地缓存的播放器并设置其音量
// 	if (TObjectPtr<USoundControlBus>* ControlBusDblPtr = ControlBusMap.Find(TEXT("Overall")))
// 	{
// 		if (USoundControlBus* ControlBusPtr = *ControlBusDblPtr)
// 		{
// 			SetVolumeForControlBus(ControlBusPtr, OverallVolume);
// 		}
// 	}
// }
//
// float ULyraSettingsLocal::GetMusicVolume() const
// {
// 	return MusicVolume;
// }
//
// void ULyraSettingsLocal::SetMusicVolume(float InVolume)
// {
// 	// Cache the incoming volume value
// 	// 将传入的音量值缓存起来
// 	MusicVolume = InVolume;
//
//
// 	// Check to see if references to the control buses and control bus mixes have been loaded yet
// 	// Will likely need to be loaded if this function is the first time a setter has been called from the UI
// 	// 检查一下是否已经加载了控制总线和控制总线混合配置的相关内容
// 	// 如果此函数是首次从用户界面调用设置器函数，则很可能需要先加载这些内容
// 	if (!bSoundControlBusMixLoaded)
// 	{
// 		LoadUserControlBusMix();
// 	}
//
// 	// Ensure it's been loaded before continuing
// 	// 确保在继续操作之前该内容已加载完成
// 	ensureMsgf(bSoundControlBusMixLoaded, TEXT("UserControlBusMix Settings Failed to Load."));
//
// 	// Locate the locally cached bus and set the volume on it
// 	// 找到本地缓存的播放器并设置其音量
// 	if (TObjectPtr<USoundControlBus>* ControlBusDblPtr = ControlBusMap.Find(TEXT("Music")))
// 	{
// 		if (USoundControlBus* ControlBusPtr = *ControlBusDblPtr)
// 		{
// 			SetVolumeForControlBus(ControlBusPtr, MusicVolume);
// 		}
// 	}
// }
//
// float ULyraSettingsLocal::GetSoundFXVolume() const
// {
// 	return SoundFXVolume;
// }
//
// void ULyraSettingsLocal::SetSoundFXVolume(float InVolume)
// {
// 	// Cache the incoming volume value
// 	// 将传入的音量值缓存起来
// 	SoundFXVolume = InVolume;
//
// 	// Check to see if references to the control buses and control bus mixes have been loaded yet
// 	// Will likely need to be loaded if this function is the first time a setter has been called from the UI
// 	// 检查一下是否已经加载了控制总线和控制总线混合配置的相关内容
// 	// 如果此函数是首次从用户界面调用设置器函数，则很可能需要先加载这些内容
// 	if (!bSoundControlBusMixLoaded)
// 	{
// 		LoadUserControlBusMix();
// 	}
//
// 	// Ensure it's been loaded before continuing
// 	// 确保在继续操作之前该内容已加载完成
// 	ensureMsgf(bSoundControlBusMixLoaded, TEXT("UserControlBusMix Settings Failed to Load."));
//
// 	// Locate the locally cached bus and set the volume on it
// 	// 找到本地缓存的播放器并设置其音量
// 	if (TObjectPtr<USoundControlBus>* ControlBusDblPtr = ControlBusMap.Find(TEXT("SoundFX")))
// 	{
// 		if (USoundControlBus* ControlBusPtr = *ControlBusDblPtr)
// 		{
// 			SetVolumeForControlBus(ControlBusPtr, SoundFXVolume);
// 		}
// 	}
// }
//
// float ULyraSettingsLocal::GetDialogueVolume() const
// {
// 	return DialogueVolume;
// }
//
// void ULyraSettingsLocal::SetDialogueVolume(float InVolume)
// {
// 	// Cache the incoming volume value
// 	// 将传入的音量值缓存起来
// 	DialogueVolume = InVolume;
//
// 	// Check to see if references to the control buses and control bus mixes have been loaded yet
// 	// Will likely need to be loaded if this function is the first time a setter has been called from the UI
// 	// 检查一下是否已经加载了控制总线和控制总线混合配置的相关内容
// 	// 如果此函数是首次从用户界面调用设置器函数，则很可能需要先加载这些内容
// 	if (!bSoundControlBusMixLoaded)
// 	{
// 		LoadUserControlBusMix();
// 	}
//
// 	// Ensure it's been loaded before continuing
// 	// 确保在继续操作之前该内容已加载完成
// 	ensureMsgf(bSoundControlBusMixLoaded, TEXT("UserControlBusMix Settings Failed to Load."));
//
// 	// Locate the locally cached bus and set the volume on it
// 	// 找到本地缓存的播放器并设置其音量
// 	if (TObjectPtr<USoundControlBus>* ControlBusDblPtr = ControlBusMap.Find(TEXT("Dialogue")))
// 	{
// 		if (USoundControlBus* ControlBusPtr = *ControlBusDblPtr)
// 		{
// 			SetVolumeForControlBus(ControlBusPtr, DialogueVolume);
// 		}
// 	}
// }
//
// float ULyraSettingsLocal::GetVoiceChatVolume() const
// {
// 	return VoiceChatVolume;
// }
//
// void ULyraSettingsLocal::SetVoiceChatVolume(float InVolume)
// {
// 	// Cache the incoming volume value
// 	// 将传入的音量值缓存起来
// 	VoiceChatVolume = InVolume;
//
// 	// Check to see if references to the control buses and control bus mixes have been loaded yet
// 	// Will likely need to be loaded if this function is the first time a setter has been called from the UI
// 	// 检查一下是否已经加载了控制总线和控制总线混合配置的相关内容
// 	// 如果此函数是首次从用户界面调用设置器函数，则很可能需要先加载这些内容
// 	if (!bSoundControlBusMixLoaded)
// 	{
// 		LoadUserControlBusMix();
// 	}
//
// 	// Ensure it's been loaded before continuing
// 	// 确保在继续操作之前该内容已加载完成
// 	ensureMsgf(bSoundControlBusMixLoaded, TEXT("UserControlBusMix Settings Failed to Load."));
//
// 	// Locate the locally cached bus and set the volume on it
// 	// 找到本地缓存的播放器并设置其音量
// 	if (TObjectPtr<USoundControlBus>* ControlBusDblPtr = ControlBusMap.Find(TEXT("VoiceChat")))
// 	{
// 		if (USoundControlBus* ControlBusPtr = *ControlBusDblPtr)
// 		{
// 			SetVolumeForControlBus(ControlBusPtr, VoiceChatVolume);
// 		}
// 	}
// }
//
// void ULyraSettingsLocal::SetAudioOutputDeviceId(const FString& InAudioOutputDeviceId)
// {
// 	AudioOutputDeviceId = InAudioOutputDeviceId;
// 	OnAudioOutputDeviceChanged.Broadcast(InAudioOutputDeviceId);
// }
//
// void ULyraSettingsLocal::ApplySafeZoneScale()
// {
// 	SSafeZone::SetGlobalSafeZoneScale(GetSafeZone());
// }
//
// void ULyraSettingsLocal::SetVolumeForControlBus(USoundControlBus* InSoundControlBus, float InVolume)
// {
// 	// Check to see if references to the control buses and control bus mixes have been loaded yet
// 	// Will likely need to be loaded if this function is the first time a setter has been called
//
// 	// 检查一下是否已经加载了控制总线和控制总线混合配置的相关内容
// 	// 如果此函数是首次调用设置器函数，那么很可能需要先加载这些内容
// 	if (!bSoundControlBusMixLoaded)
// 	{
// 		LoadUserControlBusMix();
// 	}
//
//
// 	// Ensure it's been loaded before continuing
// 	// 确保在继续操作之前该内容已加载完成
// 	ensureMsgf(bSoundControlBusMixLoaded, TEXT("UserControlBusMix Settings Failed to Load."));
//
// 	// Assuming everything has been loaded correctly, we retrieve the world and use AudioModulationStatics to update the Control Bus Volume values and
// 	// apply the settings to the cached User Control Bus Mix
//
// 	// 假设所有内容都已正确加载，我们获取世界信息，并使用 AudioModulationStatics 来更新控制总线音量值，并将设置应用到缓存的用户控制总线混音中。
// 	if (GEngine && InSoundControlBus && bSoundControlBusMixLoaded)
// 	{
// 		if (const UWorld* AudioWorld = GEngine->GetCurrentPlayWorld())
// 		{
// 			// 确保控制混合总线存在
// 			ensureMsgf(ControlBusMix, TEXT("Control Bus Mix failed to load."));
//
// 			// Create and set the Control Bus Mix Stage Parameters
// 			// 创建并设置控制总线混合阶段参数
// 			FSoundControlBusMixStage UpdatedControlBusMixStage;
// 			UpdatedControlBusMixStage.Bus = InSoundControlBus;
// 			UpdatedControlBusMixStage.Value.TargetValue = InVolume;
// 			UpdatedControlBusMixStage.Value.AttackTime = 0.01f;
// 			UpdatedControlBusMixStage.Value.ReleaseTime = 0.01f;
//
//
// 			// Add the Control Bus Mix Stage to an Array as the UpdateMix function requires
// 			// 将控制总线混合阶段添加到数组中，因为更新混合函数有此要求
// 			TArray<FSoundControlBusMixStage> UpdatedMixStageArray;
// 			UpdatedMixStageArray.Add(UpdatedControlBusMixStage);
//
//
// 			// Modify the matching bus Mix Stage parameters on the User Control Bus Mix
// 			// 在用户控制总线混合模块中修改匹配总线的混合阶段参数
// 			UAudioModulationStatics::UpdateMix(AudioWorld, ControlBusMix, UpdatedMixStageArray);
// 		}
// 	}
// }
//
// void ULyraSettingsLocal::SetControllerPlatform(const FName InControllerPlatform)
// {
// 	if (ControllerPlatform != InControllerPlatform)
// 	{
// 		ControllerPlatform = InControllerPlatform;
//
// 		// Apply the change to the common input subsystem so that we refresh any input icons we're using.
// 		// 将此更改应用于通用输入子系统，以便我们更新正在使用的任何输入图标。
// 		if (UCommonInputSubsystem* InputSubsystem = UCommonInputSubsystem::Get(GetTypedOuter<ULocalPlayer>()))
// 		{
// 			InputSubsystem->SetGamepadInputType(ControllerPlatform);
// 		}
// 	}
// }
//
// FName ULyraSettingsLocal::GetControllerPlatform() const
// {
// 	return ControllerPlatform;
// }
//
//
// void ULyraSettingsLocal::LoadUserControlBusMix()
// {
// 	if (GEngine)
// 	{
// 		if (const UWorld* World = GEngine->GetCurrentPlayWorld())
// 		{
// 			// 拿到音频设置
// 			if (const ULyraAudioSettings* LyraAudioSettings = GetDefault<ULyraAudioSettings>())
// 			{
// 				USoundControlBus* OverallControlBus = nullptr;
// 				USoundControlBus* MusicControlBus = nullptr;
// 				USoundControlBus* SoundFXControlBus = nullptr;
// 				USoundControlBus* DialogueControlBus = nullptr;
// 				USoundControlBus* VoiceChatControlBus = nullptr;
//
//
// 				ControlBusMap.Empty();
//
//
// 				// 加载总体音量的总线
// 				if (UObject* ObjPath = LyraAudioSettings->OverallVolumeControlBus.TryLoad())
// 				{
// 					if (USoundControlBus* SoundControlBus = Cast<USoundControlBus>(ObjPath))
// 					{
// 						OverallControlBus = SoundControlBus;
// 						ControlBusMap.Add(TEXT("Overall"), OverallControlBus);
// 					}
// 					else
// 					{
// 						ensureMsgf(SoundControlBus,
// 						           TEXT("Overall Control Bus reference missing from Lyra Audio Settings."));
// 					}
// 				}
//
// 				// 加载音乐音量的总线
// 				if (UObject* ObjPath = LyraAudioSettings->MusicVolumeControlBus.TryLoad())
// 				{
// 					if (USoundControlBus* SoundControlBus = Cast<USoundControlBus>(ObjPath))
// 					{
// 						MusicControlBus = SoundControlBus;
// 						ControlBusMap.Add(TEXT("Music"), MusicControlBus);
// 					}
// 					else
// 					{
// 						ensureMsgf(SoundControlBus,
// 						           TEXT("Music Control Bus reference missing from Lyra Audio Settings."));
// 					}
// 				}
//
// 				//加载特效音量的总线
// 				if (UObject* ObjPath = LyraAudioSettings->SoundFXVolumeControlBus.TryLoad())
// 				{
// 					if (USoundControlBus* SoundControlBus = Cast<USoundControlBus>(ObjPath))
// 					{
// 						SoundFXControlBus = SoundControlBus;
// 						ControlBusMap.Add(TEXT("SoundFX"), SoundFXControlBus);
// 					}
// 					else
// 					{
// 						ensureMsgf(SoundControlBus,
// 						           TEXT("SoundFX Control Bus reference missing from Lyra Audio Settings."));
// 					}
// 				}
//
// 				// 加载对话音量的总线
// 				if (UObject* ObjPath = LyraAudioSettings->DialogueVolumeControlBus.TryLoad())
// 				{
// 					if (USoundControlBus* SoundControlBus = Cast<USoundControlBus>(ObjPath))
// 					{
// 						DialogueControlBus = SoundControlBus;
// 						ControlBusMap.Add(TEXT("Dialogue"), DialogueControlBus);
// 					}
// 					else
// 					{
// 						ensureMsgf(SoundControlBus,
// 						           TEXT("Dialogue Control Bus reference missing from Lyra Audio Settings."));
// 					}
// 				}
//
// 				// 加载聊天音量的总线
// 				if (UObject* ObjPath = LyraAudioSettings->VoiceChatVolumeControlBus.TryLoad())
// 				{
// 					if (USoundControlBus* SoundControlBus = Cast<USoundControlBus>(ObjPath))
// 					{
// 						VoiceChatControlBus = SoundControlBus;
// 						ControlBusMap.Add(TEXT("VoiceChat"), VoiceChatControlBus);
// 					}
// 					else
// 					{
// 						ensureMsgf(SoundControlBus,
// 						           TEXT("VoiceChat Control Bus reference missing from Lyra Audio Settings."));
// 					}
// 				}
//
// 				// 加载用户设置总线的混合
// 				if (UObject* ObjPath = LyraAudioSettings->UserSettingsControlBusMix.TryLoad())
// 				{
// 					if (USoundControlBusMix* SoundControlBusMix = Cast<USoundControlBusMix>(ObjPath))
// 					{
// 						ControlBusMix = SoundControlBusMix;
// 						/** Creates a stage used to mix a control bus.
// 						 * @param Bus - Bus stage is in charge of applying mix value to.
// 						 * @param Value - Value for added bus stage to target when mix is active.
// 						 * @param AttackTime - Time in seconds for stage to mix in.
// 						 * @param ReleaseTime - Time in seconds for stage to mix out.
// 						 */
// 						/** 创建一个用于混合控制总线的舞台。
// 						 * @参数  Bus - 总线舞台负责将混合值应用到目标上。
// 						 * @参数  Value - 当混合功能开启时，添加到舞台的目标值。
// 						 * @参数  AttackTime - 舞台混合进来的时长（以秒为单位）。
// 						 * @参数  ReleaseTime - 舞台混合出去的时长（以秒为单位）。
// 						 * 
// 						 */
// 						const FSoundControlBusMixStage OverallControlBusMixStage =
// 							UAudioModulationStatics::CreateBusMixStage(World, OverallControlBus, OverallVolume);
// 						const FSoundControlBusMixStage MusicControlBusMixStage =
// 							UAudioModulationStatics::CreateBusMixStage(World, MusicControlBus, MusicVolume);
// 						const FSoundControlBusMixStage SoundFXControlBusMixStage =
// 							UAudioModulationStatics::CreateBusMixStage(World, SoundFXControlBus, SoundFXVolume);
// 						const FSoundControlBusMixStage DialogueControlBusMixStage =
// 							UAudioModulationStatics::CreateBusMixStage(World, DialogueControlBus, DialogueVolume);
// 						const FSoundControlBusMixStage VoiceChatControlBusMixStage =
// 							UAudioModulationStatics::CreateBusMixStage(World, VoiceChatControlBus, VoiceChatVolume);
//
// 						// 准备一个容器包含这些混入的舞台总线
// 						TArray<FSoundControlBusMixStage> ControlBusMixStageArray;
// 						ControlBusMixStageArray.Add(OverallControlBusMixStage);
// 						ControlBusMixStageArray.Add(MusicControlBusMixStage);
// 						ControlBusMixStageArray.Add(SoundFXControlBusMixStage);
// 						ControlBusMixStageArray.Add(DialogueControlBusMixStage);
// 						ControlBusMixStageArray.Add(VoiceChatControlBusMixStage);
//
//
// 						/** Sets a Control Bus Mix with the provided stage data, if the stages
// 						 *  are provided in an active instance proxy of the mix. 
// 						 *  Does not update UObject definition of the mix. 
// 						 * @param Mix - Mix to update
// 						 * @param Stages - Stages to set.  If stage's bus is not referenced by mix, stage's update request is ignored.
// 						 * @param FadeTime - Fade time to user when interpolating between current value and new values.
// 						 *					 If negative, falls back to last fade time set on stage. If fade time never set on stage,
// 						 *					 uses attack time set on stage in mix asset.
// 						 * @param Duration - Amount of time the Mix is activated for. When the Mix has been active for the given time,
// 						 *					 automatically deactivates itself. When less than 0, the duration is infinite
// 						 *					 (i.e. mix will stay active until manually deactivated).
// 						 * @param bRetriggerOnActivation - If true, if this mix is already active when Activate is called,
// 						 *								  stages will return to their default values before activating.
// 						 */
//
// 						/**
// 						 * 根据提供的阶段数据设置控制总线混合模式，前提是这些阶段是在混合模式的活跃实例代理中提供的。
// 						 * 不会更新混合模式的 UObject 定义。
// 						 * @param Mix - 需要更新的混合模式
// 						 * @param Stages - 需要设置的阶段。如果阶段的总线未被混合模式引用，则将忽略该阶段的更新请求。
// 						 * @param FadeTime - 用于在当前值和新值之间进行插值的淡出时间。如果为负数，则回退到阶段上设置的最后淡出时间。如果阶段上从未设置淡出时间，则使用混合资产中设置的攻击时间。
// 						 * @param Duration - 混合模式激活的持续时间。当混合模式已激活指定时间后，会自动停止。如果小于 0，则持续时间为无限（即混合模式将一直激活，直到手动停止）。
// 						 * @param bRetriggerOnActivation - 如果为真，当调用 Activate 时如果此混合模式已经处于激活状态，阶段将返回到其默认值后再进行激活。
// 						 */
// 						// 更新混合总线
// 						UAudioModulationStatics::UpdateMix(World, ControlBusMix, ControlBusMixStageArray);
//
// 						// 加载完毕
// 						bSoundControlBusMixLoaded = true;
// 					}
//
// 					else
// 					{
// 						// 用户控制总线缺失!
// 						ensureMsgf(SoundControlBusMix,
// 						           TEXT("User Settings Control Bus Mix reference missing from Lyra Audio Settings."));
// 					}
// 				}
// 			}
// 		}
// 	}
// }
//
// void ULyraSettingsLocal::OnAppActivationStateChanged(bool bIsActive)
// {
// 	// We might want to adjust the frame rate when the app loses/gains focus on multi-window platforms
// 	// 在多窗口平台上，当应用程序失去/获得焦点时，我们可能需要调整帧率。
// 	UpdateEffectiveFrameRateLimit();
// }
//
// void ULyraSettingsLocal::ReapplyThingsDueToPossibleDeviceProfileChange()
// {
// 	ApplyNonResolutionSettings();
// }
