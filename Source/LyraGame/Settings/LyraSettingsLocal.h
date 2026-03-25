// Copyright Epic Games, Inc. All Rights Reserved.
// Finished.
// 001 Not Finished.只写了基础类
// 002 写了三个命令行注册的代码.因为在平台模拟设置中用到了,所以先写了.
// 003 添加了一个音频设备切换的代理
// 004 添加了一个Experience加载后调用方法
#pragma once

#include "GameFramework/GameUserSettings.h"
#include "InputCoreTypes.h"

#include "LyraSettingsLocal.generated.h"

enum class ECommonInputType : uint8;
enum class ELyraDisplayablePerformanceStat : uint8;
enum class ELyraStatDisplayMode : uint8;

class ULyraLocalPlayer;
class UObject;
class USoundControlBus;
class USoundControlBusMix;
struct FFrame;

USTRUCT()
struct FLyraScalabilitySnapshot
{
	GENERATED_BODY()

	FLyraScalabilitySnapshot();

	//用于保存引擎可扩展性组状态的结构
	//通过调用 GetQualityLevels() 方法可以获取实际的引擎状态。
	Scalability::FQualityLevels Qualities;

	// 是否激活
	bool bActive = false;
	// 是否重写
	bool bHasOverrides = false;
};



/**
 * UGameUserSettings:
 * Stores user settings for a game (for example graphics and sound settings), with the ability to save and load to and from a file.
 *
 * 为游戏保存用户设置（例如图形和声音设置），并具备将设置保存至文件、从文件加载设置以及从文件中加载并应用设置的功能。
 * 
 */
/**
 * ULyraSettingsLocal
 */

UCLASS()
class ULyraSettingsLocal : public UGameUserSettings
{
	GENERATED_BODY()

public:

	// 构造函数
	ULyraSettingsLocal();

	static ULyraSettingsLocal* Get();

//
// 	//~UObject interface
// 	virtual void BeginDestroy() override;
// 	//~End of UObject interface
//
//
// 	
// 	//~UGameUserSettings interface
//
//
// 	// 恢复到默认状态
// 	virtual void SetToDefaults() override;
//
// 	/** 从持久存储中加载用户设置   */
// 	virtual void LoadSettings(bool bForceReload) override;
//
// 	/** 将当前视频模式设置（全屏模式/分辨率）标记为用户已确认的设置 */
// 	virtual void ConfirmVideoMode() override;
//
// 	/** 返回实际的帧率限制值（默认情况下会返回 FrameRateLimit 成员的值） */
// 	virtual float GetEffectiveFrameRateLimit() override;
//
// 	/** 此函数会将所有设置重置为当前系统的设置 */
// 	virtual void ResetToCurrentSettings() override;
// 	
// 	// 应用非分辨率设置
// 	virtual void ApplyNonResolutionSettings() override;
//
// 	// 返回整体的可扩展性等级（若设置为自定义，则可能返回 -1）
// 	virtual int32 GetOverallScalabilityLevel() const override;
// 	
// 	// 根据单一的整体质量标准一次性更改所有可扩展性设置
// 	// @参数 值 0：低，1：中等，2：高，3：史诗级，4：电影级
// 	virtual void SetOverallScalabilityLevel(int32 Value) override;
// 	
// 	//~End of UGameUserSettings interface
//
// 	// 这个方法在ExperienceManagerComponent里面体验完全加载后调用
// 	void OnExperienceLoaded();
// 	
// 	// 热更完成之后 主动在HotFixManager中调用 重新加载可能发生的变动设置
// 	void OnHotfixDeviceProfileApplied();
// 	
// 	//////////////////////////////////////////////////////////////////
// 	// Frontend state
// 	// 前端状态
// public:
// 	// 设置是否使用前端性能测试设置 在GameFeatureAction中去添加调用
// 	void SetShouldUseFrontendPerformanceSettings(bool bInFrontEnd);
//
// protected:
// 	// 是否可以使用前端性能测试设置
// 	bool ShouldUseFrontendPerformanceSettings() const;
//
// private:
// 	// 前端是否有开启性能测试的意图
// 	bool bInFrontEndForPerformancePurposes = false;
//
// 	//////////////////////////////////////////////////////////////////
// 	// Performance stats
// 	// 性能数据
// public:
// 	/** Returns the display mode for the specified performance stat */
// 	/** 返回指定性能指标的显示模式 */
// 	ELyraStatDisplayMode GetPerfStatDisplayState(ELyraDisplayablePerformanceStat Stat) const;
//
// 	/** Sets the display mode for the specified performance stat */
// 	/** 为指定的性能指标设置显示模式 */
// 	void SetPerfStatDisplayState(ELyraDisplayablePerformanceStat Stat, ELyraStatDisplayMode DisplayMode);
//
//
// 	
// 	/** Fired when the display state for a performance stat has changed, or the settings are applied */
// 	/** 当性能指标的显示状态发生变化，或者设置被应用时触发此事件 */
// 	DECLARE_EVENT(ULyraSettingsLocal, FPerfStatSettingsChanged);
// 	
// 	// 获取性能显示指标模式切换的代理进行绑定
// 	FPerfStatSettingsChanged& OnPerfStatDisplayStateChanged() { return PerfStatSettingsChangedEvent; }
// 		
// public:
//
//
// 	// Latency flash indicators
// 	// 延迟闪烁指示器
// 	static bool DoesPlatformSupportLatencyMarkers();
//
// 	// 延迟闪烁指示器设置变更的代理
// 	DECLARE_EVENT(ULyraSettingsLocal, FLatencyFlashInidicatorSettingChanged);
//
// 	// 开启延迟闪烁指示器功能
// 	UFUNCTION()
// 	void SetEnableLatencyFlashIndicators(const bool bNewVal);
//
// 	// 获取是否开启了延迟闪烁指示器
// 	UFUNCTION()
// 	bool GetEnableLatencyFlashIndicators() const { return bEnableLatencyFlashIndicators; }
//
//
// 	// 获取延迟闪烁指示器 变更的代理
// 	FLatencyFlashInidicatorSettingChanged& OnLatencyFlashInidicatorSettingsChangedEvent()
// 	{
// 		return LatencyFlashInidicatorSettingsChangedEvent;
// 	}
// 	
// 	
// 	// Latency tracking stats
// 	// 延迟跟踪统计数据
// 	
// 	static bool DoesPlatformSupportLatencyTrackingStats();
//
//
// 	// 延迟跟踪统计数据
// 	DECLARE_EVENT(ULyraSettingsLocal, FLatencyStatEnabledSettingChanged);
//
// 	
// 	// 延迟跟踪数据的代理
// 	FLatencyStatEnabledSettingChanged& OnLatencyStatIndicatorSettingsChangedEvent()
// 	{
// 		return LatencyStatIndicatorSettingsChangedEvent;
// 	}
// 	
// 	// 开启延迟跟踪数据
// 	UFUNCTION()
// 	void SetEnableLatencyTrackingStats(const bool bNewVal);
// 	
// 	// 获取是否开启了延迟跟踪数据统计
// 	UFUNCTION()
// 	bool GetEnableLatencyTrackingStats() const { return bEnableLatencyTrackingStats; }
//
//
// private:
// 	
// 	// 应用延迟追踪的性能追踪设置
// 	void ApplyLatencyTrackingStatSetting();
// 	
// 	// List of stats to display in the HUD
// 	// 屏幕顶部显示的统计数据列表
// 	UPROPERTY(Config)
// 	TMap<ELyraDisplayablePerformanceStat, ELyraStatDisplayMode> DisplayStatList;
// 	
// 	// Event for display stat widget containers to bind to
// 	// 用于显示状态小部件容器的事件，用于进行绑定操作
// 	FPerfStatSettingsChanged PerfStatSettingsChangedEvent;
//
// 	// If true, enable latency flash markers which can be used to measure input latency.
// 	// 若为真，则启用延迟闪烁标记，这些标记可用于测量输入延迟。
// 	UPROPERTY(Config)
// 	bool bEnableLatencyFlashIndicators = false;
// 	
//
// 	// Event for when the latency flash indicator setting had changed for player input to bind to.
// 	// 当玩家输入绑定的延迟闪烁指示器设置发生更改时触发的事件。
// 	FLatencyFlashInidicatorSettingChanged LatencyFlashInidicatorSettingsChangedEvent;
//
//
// 	// Event for when the latency stats being toggled on or off has changed
// 	// 当延迟统计数据的开启或关闭状态发生改变时的事件
// 	FLatencyStatEnabledSettingChanged LatencyStatIndicatorSettingsChangedEvent;
//
// 	
// 	
// 	// If true, then the game will track latency stats via ILatencyMarkerModule modules.
// 	// This enables you to view some more latency oriented performance stats.
// 	// The default value is set to true if the platform supports it, false otherwise.
// 	// 如果为真，则游戏将通过 ILatencyMarkerModule 模块来跟踪延迟数据。
// 	// 这使您能够查看更多与延迟相关的性能数据。
// 	// 默认值会根据平台支持情况而定：若平台支持则设为真，否则设为假。
// 	UPROPERTY(Config)
// 	bool bEnableLatencyTrackingStats;
//
// 	
// 	
// 	//////////////////////////////////////////////////////////////////
// 	// Brightness/Gamma
// 	// 亮度/伽马值
// public:
// 	// 获取伽马值
// 	UFUNCTION()
// 	float GetDisplayGamma() const;
//
// 	//设置伽马值
// 	UFUNCTION()
// 	void SetDisplayGamma(float InGamma);
//
// private:
// 	// 应用伽马值设置
// 	void ApplyDisplayGamma();
// 	
// 	// 伽马值
// 	UPROPERTY(Config)
// 	float DisplayGamma = 2.2;
//
//
// 	//////////////////////////////////////////////////////////////////
// 	// Display
// 	// 显示
// public:
// 	// 获取电池使用时的帧率限制
// 	UFUNCTION()
// 	float GetFrameRateLimit_OnBattery() const;
//
// 	// 设置电池使用的帧率限制
// 	UFUNCTION()
// 	void SetFrameRateLimit_OnBattery(float NewLimitFPS);
//
// 	// 获取菜单显示时的帧率
// 	UFUNCTION()
// 	float GetFrameRateLimit_InMenu() const;
// 	
// 	// 设置菜单显示时的帧率
// 	UFUNCTION()
// 	void SetFrameRateLimit_InMenu(float NewLimitFPS);
//
// 	// 获取后台时的帧率
// 	UFUNCTION()
// 	float GetFrameRateLimit_WhenBackgrounded() const;
//
// 	// 设置后台时的帧率
// 	UFUNCTION()
// 	void SetFrameRateLimit_WhenBackgrounded(float NewLimitFPS);
//
// 	// 获取一般帧率限制
// 	UFUNCTION()
// 	float GetFrameRateLimit_Always() const;
// 	// 设置一般帧率限制
// 	UFUNCTION()
// 	void SetFrameRateLimit_Always(float NewLimitFPS);
//
// protected:
// 	
// 	// 更新帧率限制
// 	void UpdateEffectiveFrameRateLimit();
// 	
// private:
// 	// 电池使用帧率限制
// 	UPROPERTY(Config)
// 	float FrameRateLimit_OnBattery;
//
// 	//菜单帧率限制
// 	UPROPERTY(Config)
// 	float FrameRateLimit_InMenu;
//
// 	//后台帧率限制
// 	UPROPERTY(Config)
// 	float FrameRateLimit_WhenBackgrounded;
//
// 	
// 	//////////////////////////////////////////////////////////////////
// 	// Display - Mobile quality settings
// 	// 显示 - 手机质量设置
//
// public:
//
// 	// 获取默认移动端帧率
// 	static int32 GetDefaultMobileFrameRate();
// 	
// 	// 获取移动端最大帧率
// 	static int32 GetMaxMobileFrameRate();
//
// 	// 是否在移动端支持该测试帧率
// 	static bool IsSupportedMobileFramePace(int32 TestFPS);
//
// 	// Returns the first frame rate at which overall quality is restricted/limited by the current device profile
// 	// 返回当前设备配置所限制/约束整体质量的首个帧率值
// 	int32 GetFirstFrameRateWithQualityLimit() const;
//
//
// 	// Returns the lowest quality at which there's a limit on the overall frame rate (or -1 if there is no limit)
// 	// 返回整体帧率存在上限的最低质量等级（若不存在上限，则返回 -1）
// 	int32 GetLowestQualityWithFrameRateLimit() const;
//
// 	// 重置移动端为默认设置
// 	void ResetToMobileDeviceDefaults();
//
// 	// 获取最高的支持质量级别
// 	int32 GetMaxSupportedOverallQualityLevel() const;
//
// 	
//
// private:
// 	// 设置移动端帧率
// 	void SetMobileFPSMode(int32 NewLimitFPS);
//
//
// 	// 根据FPS设置移动端分辨率质量
// 	void ClampMobileResolutionQuality(int32 TargetFPS);
// 	
// 	// 重设移动端分辨率质量,根据新旧FPS
// 	void RemapMobileResolutionQuality(int32 FromFPS, int32 ToFPS);
// 	
// 	// 设置移动端帧率质量,根据FPS的帧率,根据需要可以是否写入后台
// 	void ClampMobileFPSQualityLevels(bool bWriteBack);
// 	
// 	// 设置移动端质量
// 	void ClampMobileQuality();
//
// 	// 返回整数可扩展性设置中的最大级别（忽略分辨率质量） 在移动端使用
// 	int32 GetHighestLevelOfAnyScalabilityChannel() const;
//
//
// 	/* Modifies the input levels based on the active mode's overrides */
// 	/* 根据当前模式的设置对输入级别进行调整 */
// 	void OverrideQualityLevelsToScalabilityMode(const FLyraScalabilitySnapshot& InMode,
// 												Scalability::FQualityLevels& InOutLevels);
//
// 	
// 	/* Clamps the input levels based on the active device profile's default allowed levels */
// 	/* 根据当前设备配置文件的默认允许范围对输入电平进行限制 */
// 	void ClampQualityLevelsToDeviceProfile(const Scalability::FQualityLevels& ClampLevels,
// 										   Scalability::FQualityLevels& InOutLevels);
//
// 	
// public:
// 	// 获取需求的移动端帧率限制
// 	int32 GetDesiredMobileFrameRateLimit() const { return DesiredMobileFrameRateLimit; }
//
// 	// 设置新的移动端限制帧率
// 	void SetDesiredMobileFrameRateLimit(int32 NewLimitFPS);
// 	
// private:
//
//
// 	// 移动平台帧率限制
// 	UPROPERTY(Config)
// 	int32 MobileFrameRateLimit = 30;
//
// 	// 设备默认的拓展性设置
// 	FLyraScalabilitySnapshot DeviceDefaultScalabilitySettings;
// 	
// 	// 是否正在重写质量设置
// 	bool bSettingOverallQualityGuard = false;
// 	
// 	// 需求的移动平台帧率限制
// 	int32 DesiredMobileFrameRateLimit = 0;
//
// 	
// private:
// 	//////////////////////////////////////////////////////////////////
// 	// Display - Console quality presets
// 	// 显示 - 控制台品质预设
// 	
// public:
//
// 	// 获取需求的设备配置文件质量后缀
// 	UFUNCTION()
// 	FString GetDesiredDeviceProfileQualitySuffix() const;
//
// 	// 设置需求的设备配置文件质量后缀
// 	UFUNCTION()
// 	void SetDesiredDeviceProfileQualitySuffix(const FString& InDesiredSuffix);
//
//
// protected:
// 	
// 	/** Updates device profiles, FPS mode etc for the current game mode */
// 	/** 根据当前游戏模式更新设备配置、帧率模式等信息 */
// 	void UpdateGameModeDeviceProfileAndFps();
// 	
// 	// 更新命令行端帧速管理
// 	void UpdateConsoleFramePacing();
//
// 	// 更新桌面端帧数管理
// 	void UpdateDesktopFramePacing();
//
// 	// 更新移动端帧数股那里
// 	void UpdateMobileFramePacing();
//
// 	// 更新动态分辨率帧时间
// 	void UpdateDynamicResFrameTime(float TargetFPS);
//
//
//
//
//
//
//
//
//
//
// 	
// private:
// 	// 需求的驱动文件后缀
// 	UPROPERTY(Transient)
// 	FString DesiredUserChosenDeviceProfileSuffix;
//
// 	// 现在使用的设备配置文件经过重写后缀
// 	UPROPERTY(Transient)
// 	FString CurrentAppliedDeviceProfileOverrideSuffix;
//
//
// 	// 用户选择的驱动文件后缀
// 	UPROPERTY(config)
// 	FString UserChosenDeviceProfileSuffix;
//
//
// 	
// 	//////////////////////////////////////////////////////////////////
// 	// Audio - Volume
// 	// 音量 - 音量调节
// 	
// public:
// 	
// 	// 定义音频设备切换的代理
// 	DECLARE_EVENT_OneParam(ULyraSettingsLocal, FAudioDeviceChanged, const FString& /*DeviceId*/);
//
// 	// 音频设备变换的代理
// 	FAudioDeviceChanged OnAudioOutputDeviceChanged;
//
// public:
// 	/** Returns if we're using headphone mode (HRTF) **/
// 	/** 返回我们是否处于耳机模式（头部相关传递函数）的状态 **/
// 	UFUNCTION()
// 	bool IsHeadphoneModeEnabled() const;
//
//
// 	/** Enables or disables headphone mode (HRTF) - NOTE this setting will be overruled if au.DisableBinauralSpatialization is set */
// 	/** 开启或关闭耳机模式（头部相关传递函数） - 注意：若将 au.DisableBinauralSpatialization 设置为“开启”，则此设置将被忽略 */
// 	UFUNCTION()
// 	void SetHeadphoneModeEnabled(bool bEnabled);	
//
// 	
// 	/** Returns if we can enable/disable headphone mode (i.e., if it's not forced on or off by the platform) */
// 	/** 返回是否能够启用/禁用耳机模式（即，是否不受平台强制设定而自动开启或关闭） */
// 	UFUNCTION()
// 	bool CanModifyHeadphoneModeEnabled() const;
//
// 	
// public:
// 	/** Whether we *want* to use headphone mode (HRTF); may or may not actually be applied **/
// 	/** 我们是否*需要*启用耳机模式（基于头骨反射率的音频处理技术）；该功能可能会启用也可能不会启用 **/
// 	UPROPERTY(Transient)
// 	bool bDesiredHeadphoneMode;
//
// 	
// private:
// 	/** Whether to use headphone mode (HRTF) **/
// 	/** 是否启用耳机模式（头相关传输函数） **/
// 	UPROPERTY(config)
// 	bool bUseHeadphoneMode;
// 	
// 	
// public:
// 	/** Returns if we're using High Dynamic Range Audio mode (HDR Audio) **/
// 	/** 返回我们是否正在使用高动态范围音频模式（HDR 音频） **/
// 	UFUNCTION()
// 	bool IsHDRAudioModeEnabled() const;
//
// 	/** Enables or disables High Dynamic Range Audio mode (HDR Audio) */
// 	/** 启用或禁用高动态范围音频模式（HDR 音频） */
// 	UFUNCTION()
// 	void SetHDRAudioModeEnabled(bool bEnabled);
// 	/** Whether to use High Dynamic Range Audio mode (HDR Audio) **/
// 	/** 是否启用高动态范围音频模式（HDR 音频） **/
// 	UPROPERTY(config)
// 	bool bUseHDRAudioMode;
//
// public:
//
// 	/** Returns true if this platform can run the auto benchmark */
// 	/** 如果此平台能够运行自动基准测试，则返回 true */
// 	UFUNCTION(BlueprintCallable, Category = Settings)
// 	bool CanRunAutoBenchmark() const;
//
//
// 	/** Returns true if this user should run the auto benchmark as it has never been run */
// 	/** 如果此用户从未运行过自动基准测试，则返回 true 表示应执行该自动基准测试 */
// 	UFUNCTION(BlueprintCallable, Category = Settings)
// 	bool ShouldRunAutoBenchmarkAtStartup() const;
//
//
// 	/** Run the auto benchmark, optionally saving right away */
// 	/** 运行自动基准测试，可选择立即保存结果 */
// 	UFUNCTION(BlueprintCallable, Category = Settings)
// 	void RunAutoBenchmark(bool bSaveImmediately);
//
// 	/** Apply just the quality scalability settings */
// 	/** 仅应用质量可扩展性设置 */
// 	void ApplyScalabilitySettings();
// 	
//
// 	
// 	
// 	
// 	// 获取整体音量大小
// 	UFUNCTION()
// 	float GetOverallVolume() const;
// 	// 设置整体音量大小
// 	UFUNCTION()
// 	void SetOverallVolume(float InVolume);
//
// 	// 获取音乐音量大小
// 	UFUNCTION()
// 	float GetMusicVolume() const;
// 	// 设置音乐音量大小
// 	UFUNCTION()
// 	void SetMusicVolume(float InVolume);
//
// 	// 获取音效音量大小
// 	UFUNCTION()
// 	float GetSoundFXVolume() const;
// 	// 设置音效音量大小
// 	UFUNCTION()
// 	void SetSoundFXVolume(float InVolume);
//
//
// 	// 获取对话音量大小
// 	UFUNCTION()
// 	float GetDialogueVolume() const;
// 	// 设置对话音量大小
// 	UFUNCTION()
// 	void SetDialogueVolume(float InVolume);
// 	
// 	// 获取聊天音量大小
// 	UFUNCTION()
// 	float GetVoiceChatVolume() const;
// 	//设置聊天音量大小
// 	UFUNCTION()
// 	void SetVoiceChatVolume(float InVolume);
//
//
// 	
// 	//////////////////////////////////////////////////////////////////
// 	// Audio - Sound
// 	// 音频 - 声音
// public:
// 	/** Returns the user's audio device id */
// 	/** 返回用户的音频设备 ID */
// 	UFUNCTION()
// 	FString GetAudioOutputDeviceId() const { return AudioOutputDeviceId; }
//
//
// 	/** Sets the user's audio device by id */
// 	/** 根据 ID 设置用户的音频设备 */
// 	UFUNCTION()
// 	void SetAudioOutputDeviceId(const FString& InAudioOutputDeviceId);
// 	
// private:
// 	
// 	// 音频设备ID
// 	UPROPERTY(Config)
// 	FString AudioOutputDeviceId;
//
// 	// 多余代码 注释掉
// 	// void SetVolumeForSoundClass(FName ChannelName, float InVolume);
//
//
// 	//////////////////////////////////////////////////////////////////
// 	// Safezone
// 	// 安全区
//
// public:
// 	
// 	//是否设置了安全区
// 	UFUNCTION()
// 	bool IsSafeZoneSet() const { return SafeZoneScale != -1; }
//
// 	//获取安全区大小
// 	UFUNCTION()
// 	float GetSafeZone() const { return SafeZoneScale >= 0 ? SafeZoneScale : 0; }
//
// 	// 设置安全区
// 	UFUNCTION()
// 	void SetSafeZone(float Value)
// 	{
// 		SafeZoneScale = Value;
// 		ApplySafeZoneScale();
// 	}
//
// 	
// 	// 应用安全区
// 	void ApplySafeZoneScale();
//
// 	
// 	
// private:
//
// 	
// 	// 设置指定控制总线的音量大小
// 	void SetVolumeForControlBus(USoundControlBus* InSoundControlBus, float InVolume);
//
// 	
// 	//////////////////////////////////////////////////////////////////
// 	// Keybindings
// 	// 快捷键设置
//
// public:
// 	// Sets the controller representation to use, a single platform might support multiple kinds of controllers.  For
// 	// example, Win64 games could be played with both an XBox or Playstation controller.
// 	// 设置控制器的使用类型，由于同一平台可能支持多种类型的控制器，所以需要指定具体使用的控制器类型。对于
// 	// 例如，Windows 64 位游戏既可以用 Xbox 控制器也可以用 PlayStation 控制器来玩。
// 	UFUNCTION()
// 	void SetControllerPlatform(const FName InControllerPlatform);
// 	UFUNCTION()
// 	FName GetControllerPlatform() const;
//
// 	
// 	
// private:
// 	
// 	// 加载用户控制总线混合
// 	void LoadUserControlBusMix();
//
// 	// 整体音量值
// 	UPROPERTY(Config)
// 	float OverallVolume = 1.0f;
//
// 	// 音乐音量值
// 	UPROPERTY(Config)
// 	float MusicVolume = 1.0f;
// 	
// 	// 音效音量值
// 	UPROPERTY(Config)
// 	float SoundFXVolume = 1.0f;
//
// 	// 对话音量值
// 	UPROPERTY(Config)
// 	float DialogueVolume = 1.0f;
//
// 	// 聊天音量值
// 	UPROPERTY(Config)
// 	float VoiceChatVolume = 1.0f;
//
// 	// 临时变量 防止GC 持有音频总线的指针
// 	UPROPERTY(Transient)
// 	TMap<FName/*SoundClassName*/, TObjectPtr<USoundControlBus>> ControlBusMap;
//
// 	// 临时变量 防止GC 用于持有当前的控制总线混合
// 	UPROPERTY(Transient)
// 	TObjectPtr<USoundControlBusMix> ControlBusMix = nullptr;
//
// 	// 控制总线混合是否已经加载
// 	UPROPERTY(Transient)
// 	bool bSoundControlBusMixLoaded;
//
// 	// 安全区规模大小
// 	UPROPERTY(Config)
// 	float SafeZoneScale = -1;
//
// 	/**
// 	 * The name of the controller the player is using.  This is maps to the name of a UCommonInputBaseControllerData
// 	 * that is available on this current platform.  The gamepad data are registered per platform, you'll find them
// 	 * in <Platform>Game.ini files listed under +ControllerData=...
// 	 */
// 	/**
// 	 * 玩家所使用的控制器的名称。
// 	 * 这与当前平台可用的 UCommonInputBaseControllerData 的名称相对应。
// 	 * 游戏手柄数据是按平台进行注册的，您可以在 <平台>Game.ini 文件中找到它们，这些文件位于 +ControllerData=... 项下。
// 	 * 
// 	 */
// 	UPROPERTY(Config)
// 	FName ControllerPlatform;
//
//
// 	UPROPERTY(Config)
// 	FName ControllerPreset = TEXT("Default");
//
// 	
// 	/** The name of the current input config that the user has selected. */
// 	/** 当前用户所选输入配置的名称。*/
// 	UPROPERTY(Config)
// 	FName InputConfigName = TEXT("Default");
// 	
//
// 	
// 	// Replays
// 	// 回放
// public:
// 	// 是否自动记录回放
// 	UFUNCTION()
// 	bool ShouldAutoRecordReplays() const { return bShouldAutoRecordReplays; }
//
// 	// 设置是否自动记录回放
// 	UFUNCTION()
// 	void SetShouldAutoRecordReplays(bool bEnabled) { bShouldAutoRecordReplays = bEnabled; }
//
// 	// 获取保持的回放数量
// 	UFUNCTION()
// 	int32 GetNumberOfReplaysToKeep() const { return NumberOfReplaysToKeep; }
//
// 	// 设置保持的回放数量
// 	UFUNCTION()
// 	void SetNumberOfReplaysToKeep(int32 InNumberOfReplays) { NumberOfReplaysToKeep = InNumberOfReplays; }
// 	
// 	
//
// 	
// private:
// 	// 是否应该自动记录回放
// 	UPROPERTY(Config)
// 	bool bShouldAutoRecordReplays = false;
//
// 	// 回放的保持数量
// 	UPROPERTY(Config)
// 	int32 NumberOfReplaysToKeep = 5;
//
//
// 	
//
// private:
//
// 	// 应用焦点发生变化时 进行帧率设置
// 	void OnAppActivationStateChanged(bool bIsActive);
// 	
// 	// 重新应用可能的设备文件配置变动
// 	void ReapplyThingsDueToPossibleDeviceProfileChange();
//
//
// private:
// 	// 应用焦点切换时激活代理的句柄
// 	FDelegateHandle OnApplicationActivationStateChangedHandle;

	
};