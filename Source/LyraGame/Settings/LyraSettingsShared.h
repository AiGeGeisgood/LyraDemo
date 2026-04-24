// Copyright Epic Games, Inc. All Rights Reserved.
// Finished.
// 001 Not Finished.定义了基本类
// 002 添加了几个方法给给外部LocalPlayer调用,但是没有写实现.
// 003 大部分代码已添加 涉及到Lyra输入的未添加
#pragma once

#include "GameFramework/SaveGame.h"
#include "SubtitleDisplayOptions.h"

#include "UObject/ObjectPtr.h"
#include "LyraSettingsShared.generated.h"


class UObject;
struct FFrame;

// 色盲模式
UENUM(BlueprintType)
enum class EColorBlindMode : uint8
{
	// 关闭
	Off,
	// Deuteranope (green weak/blind)
	// 褐色盲（绿色视觉较弱/失明）
	Deuteranope,
	// Protanope (red weak/blind)
	// 蓝红色弱视/盲视者
	Protanope,
	// Tritanope(blue weak / bind)
	// 三色盲（蓝色较弱/易褪色）
	Tritanope
};


// 允许的后台音乐设置
UENUM(BlueprintType)
enum class ELyraAllowBackgroundAudioSetting : uint8
{
	// 关闭
	Off,
	// 所有
	AllSounds,
	// 最大数量
	Num UMETA(Hidden),
};


// 灵敏度
UENUM(BlueprintType)
enum class ELyraGamepadSensitivity : uint8
{
	// 无效
	Invalid = 0		UMETA(Hidden),

	// 慢
	Slow			UMETA(DisplayName = "01 - Slow"),

	// 慢+
	SlowPlus		UMETA(DisplayName = "02 - Slow+"),

	// 慢++
	SlowPlusPlus	UMETA(DisplayName = "03 - Slow++"),

	// 正常
	Normal			UMETA(DisplayName = "04 - Normal"),

	// 正常+
	NormalPlus		UMETA(DisplayName = "05 - Normal+"),

	// 正常++
	NormalPlusPlus	UMETA(DisplayName = "06 - Normal++"),

	// 快
	Fast			UMETA(DisplayName = "07 - Fast"),

	// 快++
	FastPlus		UMETA(DisplayName = "08 - Fast+"),

	// 快++
	FastPlusPlus	UMETA(DisplayName = "09 - Fast++"),

	// 疯狂
	Insane			UMETA(DisplayName = "10 - Insane"),

	MAX				UMETA(Hidden),
};



class ULyraLocalPlayer;

/**
 * ULyraSettingsShared - The "Shared" settings are stored as part of the USaveGame system, these settings are not machine
 * specific like the local settings, and are safe to store in the cloud - and 'share' them.  Using the save game system
 * we can also store settings per player, so things like controller keybind preferences should go here, because if those
 * are stored in the local settings all users would get them.
 *
 * ULyraSettingsShared - “共享”设置被存储在 USaveGame 系统中，与本地设置不同，这些设置并非针对特定机器的，而且可以安全地存储在云端并进行“共享”。通过使用保存游戏系统，我们还可以为每个玩家存储设置，比如控制器按键绑定偏好就应存放在这里，因为如果这些设置存放在本地设置中，那么所有用户都会获取到它们。
 */
UCLASS()
class ULyraSettingsShared : public ULocalPlayerSaveGame
{
	GENERATED_BODY()

public:

	// 共享设置变更代理 用于控制器绑定获取是否开启力反馈
	DECLARE_EVENT_OneParam(ULyraSettingsShared, FOnSettingChangedEvent, ULyraSettingsShared* Settings);
	FOnSettingChangedEvent OnSettingChanged;


public:

	// 构造函数 绑定本地化变更代理 初始化手柄盲区值
	ULyraSettingsShared();


	//~ULocalPlayerSaveGame interface
	// 修改存档版本值
	int32 GetLatestDataVersion() const override;
	//~End of ULocalPlayerSaveGame interface



	// 是否有修改
	bool IsDirty() const { return bIsDirty; }

	// 清楚修改标记
	void ClearDirtyFlag() { bIsDirty = false; }

	
	/** Creates a temporary settings object, this will be replaced by one loaded from the user's save game */
	/** 创建一个临时的设置对象，该对象将被从用户的游戏存档中加载的设置对象所替换 */
	static ULyraSettingsShared* CreateTemporarySettings(const ULyraLocalPlayer* LocalPlayer);

	
	/** Synchronously loads a settings object, this is not valid to call before login */
	/** 异步加载一个设置对象，此操作在登录之前不可执行 */
	static ULyraSettingsShared* LoadOrCreateSettings(const ULyraLocalPlayer* LocalPlayer);

	DECLARE_DELEGATE_OneParam(FOnSettingsLoadedEvent, ULyraSettingsShared* Settings);

	/** Starts an async load of the settings object, calls Delegate on completion */
	/** 启动对设置对象的异步加载过程，完成时调用委托函数 */
	static bool AsyncLoadOrCreateSettings(const ULyraLocalPlayer* LocalPlayer, FOnSettingsLoadedEvent Delegate);

	/** Saves the settings to disk */
	/** 将设置保存至磁盘 */
	void SaveSettings();


	/** Applies the current settings to the player */
	/** 将当前设置应用到玩家身上 */
	void ApplySettings();

	
public:
	////////////////////////////////////////////////////////
	// Color Blind Options
	// 色盲选项
		
		
	// 获取色盲模式
	UFUNCTION()
	EColorBlindMode GetColorBlindMode() const;

	// 设置色盲模式
	UFUNCTION()
	void SetColorBlindMode(EColorBlindMode InMode);

	// 获取色盲模式强度
	UFUNCTION()
	int32 GetColorBlindStrength() const;

	// 设置蛇魔模式强度
	UFUNCTION()
	void SetColorBlindStrength(int32 InColorBlindStrength);



private:
	// 色盲模式
	UPROPERTY()
	EColorBlindMode ColorBlindMode = EColorBlindMode::Off;

	UPROPERTY()
	int32 ColorBlindStrength = 10;



	////////////////////////////////////////////////////////
	// Subtitles
	// 字幕
public:

	// 获取字幕是否开启
	UFUNCTION()
	bool GetSubtitlesEnabled() const { return bEnableSubtitles; }


	// 开启字幕
	UFUNCTION()
	void SetSubtitlesEnabled(bool Value){ 
		if (ChangeValueAndDirty(bEnableSubtitles, Value))
		{
			GEngine->bSubtitlesEnabled=Value;
		}
	}

	// 获取字体大小
	UFUNCTION()
	ESubtitleDisplayTextSize GetSubtitlesTextSize() const { return SubtitleTextSize; }

	// 设置字体大小
	UFUNCTION()
	void SetSubtitlesTextSize(ESubtitleDisplayTextSize Value) { ChangeValueAndDirty(SubtitleTextSize, Value); ApplySubtitleOptions(); }


	// 获取字体颜色
	UFUNCTION()
	ESubtitleDisplayTextColor GetSubtitlesTextColor() const { return SubtitleTextColor; }

	// 设置字体颜色
	UFUNCTION()
	void SetSubtitlesTextColor(ESubtitleDisplayTextColor Value) { ChangeValueAndDirty(SubtitleTextColor, Value); ApplySubtitleOptions(); }


	// 获取字体边框
	UFUNCTION()
	ESubtitleDisplayTextBorder GetSubtitlesTextBorder() const { return SubtitleTextBorder; }

	// 设置字体边框
	UFUNCTION()
	void SetSubtitlesTextBorder(ESubtitleDisplayTextBorder Value) { ChangeValueAndDirty(SubtitleTextBorder, Value); ApplySubtitleOptions(); }


	// 获取字幕背景和透明度
	UFUNCTION()
	ESubtitleDisplayBackgroundOpacity GetSubtitlesBackgroundOpacity() const { return SubtitleBackgroundOpacity; }

	// 设置题目背景和透明度
	UFUNCTION()
	void SetSubtitlesBackgroundOpacity(ESubtitleDisplayBackgroundOpacity Value) { ChangeValueAndDirty(SubtitleBackgroundOpacity, Value); ApplySubtitleOptions(); }




	// 应用字幕设置
	void ApplySubtitleOptions();

private:

	// 是否开启字幕
	UPROPERTY()
	bool bEnableSubtitles = true;

	// 字体大小
	UPROPERTY()
	ESubtitleDisplayTextSize SubtitleTextSize = ESubtitleDisplayTextSize::Medium;

	// 字体颜色
	UPROPERTY()
	ESubtitleDisplayTextColor SubtitleTextColor = ESubtitleDisplayTextColor::White;

	// 字体边框
	UPROPERTY()
	ESubtitleDisplayTextBorder SubtitleTextBorder = ESubtitleDisplayTextBorder::None;


	// 字体背景透明度
	UPROPERTY()
	ESubtitleDisplayBackgroundOpacity SubtitleBackgroundOpacity = ESubtitleDisplayBackgroundOpacity::Medium;


	////////////////////////////////////////////////////////
	// Shared audio settings
public:
	// 获取是否允许后台音频
	UFUNCTION()
	ELyraAllowBackgroundAudioSetting GetAllowAudioInBackgroundSetting() const { return AllowAudioInBackground; }

	// 设置后台音量
	UFUNCTION()
	void SetAllowAudioInBackgroundSetting(ELyraAllowBackgroundAudioSetting NewValue);

	// 应用后台音量设置
	void ApplyBackgroundAudioSettings();

private:
	// 后台音量设置模式
	UPROPERTY()
	ELyraAllowBackgroundAudioSetting AllowAudioInBackground = ELyraAllowBackgroundAudioSetting::Off;

	////////////////////////////////////////////////////////
	// Culture / language
	// 文化 / 语言

public:

	/** Gets the pending culture */
	/** 获取待选文化 */
	const FString& GetPendingCulture() const;

	/** Sets the pending culture to apply */
	/** 设置要应用的待定文化 */
	void SetPendingCulture(const FString& NewCulture);

	// Called when the culture changes.
	// 当文化发生改变时触发此事件。
	void OnCultureChanged();

	/** Clears the pending culture to apply */
	/** 清除待应用的语言设置 */
	void  ClearPendingCulture();

	// 是否在使用默认的文化
	bool IsUsingDefaultCulture() const;

	// 恢复至默认文化
	void ResetToDefaultCulture();
	// 是否需要恢复至默认文化
	bool ShouldResetToDefaultCulture() const { return bResetToDefaultCulture; }

	// 应用文化设置
	void ApplyCultureSettings();

	// 恢复到现在得设置 放弃更改
	void ResetCultureToCurrentSettings();

private:
	/** The pending culture to apply */
	/** 正在应用的待定文化 */
	UPROPERTY(Transient)
	FString PendingCulture;

	/* If true, resets the culture to default. */
	/* 若为真，则将语言设置重置为默认值。*/
	bool bResetToDefaultCulture = false;

	////////////////////////////////////////////////////////
	// Gamepad Sensitivity
	// 游戏手柄灵敏度
public:

	// 设置鼠标水平（x）轴的灵敏度。设置值越高，使用鼠标左右移动时，摄像机移动的速度就越快。
	UFUNCTION()
	double GetMouseSensitivityX() const { return MouseSensitivityX; }
	UFUNCTION()
	void SetMouseSensitivityX(double NewValue) { ChangeValueAndDirty(MouseSensitivityX, NewValue); ApplyInputSensitivity(); }


	// 设置鼠标垂直（y 轴）方向的灵敏度。设置值越高，使用鼠标上下移动时，摄像机移动的速度就越快。
	UFUNCTION()
	double GetMouseSensitivityY() const { return MouseSensitivityY; }
	UFUNCTION()
	void SetMouseSensitivityY(double NewValue) { ChangeValueAndDirty(MouseSensitivityY, NewValue); ApplyInputSensitivity(); }

	// 设置在进行目标攻击时降低鼠标灵敏度的调节值。将数值设为 100% 时，在进行目标攻击时不会出现操作迟缓的情况。数值越低，在进行目标攻击时操作迟缓的情况就越明显。
	UFUNCTION()
	double GetTargetingMultiplier() const { return TargetingMultiplier; }
	UFUNCTION()
	void SetTargetingMultiplier(double NewValue) { ChangeValueAndDirty(TargetingMultiplier, NewValue); ApplyInputSensitivity(); }


	// 启用垂直视角轴的反转功能。
	UFUNCTION()
	bool GetInvertVerticalAxis() const { return bInvertVerticalAxis; }
	UFUNCTION()
	void SetInvertVerticalAxis(bool NewValue) { ChangeValueAndDirty(bInvertVerticalAxis, NewValue); ApplyInputSensitivity(); }


	// 启用水平视角轴的反转功能。
	UFUNCTION()
	bool GetInvertHorizontalAxis() const { return bInvertHorizontalAxis; }
	UFUNCTION()
	void SetInvertHorizontalAxis(bool NewValue) { ChangeValueAndDirty(bInvertHorizontalAxis, NewValue); ApplyInputSensitivity(); }
	
private:
	/** Holds the mouse horizontal sensitivity */
	/** 存储鼠标水平灵敏度值 */
	UPROPERTY()
	double MouseSensitivityX = 1.0;

	/** Holds the mouse vertical sensitivity */
	/** 存储鼠标垂直灵敏度值 */
	UPROPERTY()
	double MouseSensitivityY = 1.0;

	/** Multiplier applied while Aiming down sights. */
	/** 在瞄准瞄准镜时所使用的乘数。*/
	UPROPERTY()
	double TargetingMultiplier = 0.5;

	/** If true then the vertical look axis should be inverted */
	/** 如果为真，则垂直观察轴应进行反转 */
	UPROPERTY()
	bool bInvertVerticalAxis = false;

	/** If true then the horizontal look axis should be inverted */
	/** 如果为真，则水平观察轴应进行反转 */
	UPROPERTY()
	bool bInvertHorizontalAxis = false;

	////////////////////////////////////////////////////////
	// Gamepad Sensitivity
	// 游戏手柄灵敏度
public:
	// 你的视角旋转的速度有多快。
	UFUNCTION()
	ELyraGamepadSensitivity GetGamepadLookSensitivityPreset() const { return GamepadLookSensitivityPreset; }
	UFUNCTION()
	void SetLookSensitivityPreset(ELyraGamepadSensitivity NewValue) { ChangeValueAndDirty(GamepadLookSensitivityPreset, NewValue); ApplyInputSensitivity(); }

	// 在瞄准瞄准镜时，你的视野旋转的速度有多快。
	UFUNCTION()
	ELyraGamepadSensitivity GetGamepadTargetingSensitivityPreset() const { return GamepadTargetingSensitivityPreset; }
	UFUNCTION()
	void SetGamepadTargetingSensitivityPreset(ELyraGamepadSensitivity NewValue) { ChangeValueAndDirty(GamepadTargetingSensitivityPreset, NewValue); ApplyInputSensitivity(); }


	void ApplyInputSensitivity();

private:
	UPROPERTY()
	ELyraGamepadSensitivity GamepadLookSensitivityPreset = ELyraGamepadSensitivity::Normal;
	UPROPERTY()
	ELyraGamepadSensitivity GamepadTargetingSensitivityPreset = ELyraGamepadSensitivity::Normal;



	////////////////////////////////////////////////////////
	// Gamepad Vibration
	// 手柄震动

public:

	// 获取力反馈
	UFUNCTION()
	bool GetForceFeedbackEnabled() const { return bForceFeedbackEnabled; }

	// 更改力反馈
	UFUNCTION()
	void SetForceFeedbackEnabled(const bool NewValue) { ChangeValueAndDirty(bForceFeedbackEnabled, NewValue); }
	

private:
	/** Is force feedback enabled when a controller is being used? */
	/** 当使用控制器时，力反馈功能是否已开启？*/
	UPROPERTY()
	bool bForceFeedbackEnabled = true;

	////////////////////////////////////////////////////////
	// Gamepad Deadzone
	// 游戏手柄死区

public:
	/** Getter for gamepad move stick dead zone value. */
	/** 获取游戏手柄移动摇杆的无效区域值的获取器。*/
	UFUNCTION()
	float GetGamepadMoveStickDeadZone() const { return GamepadMoveStickDeadZone; }

	/** Setter for gamepad move stick dead zone value. */
	/** 游戏手柄移动杆死区值的设置器。*/
	UFUNCTION()
	void SetGamepadMoveStickDeadZone(const float NewValue) { ChangeValueAndDirty(GamepadMoveStickDeadZone, NewValue); }

	/** Getter for gamepad look stick dead zone value. */
	/** 获取游戏手柄瞄准杆盲区值的获取器。*/
	UFUNCTION()
	float GetGamepadLookStickDeadZone() const { return GamepadLookStickDeadZone; }

	/** Setter for gamepad look stick dead zone value. */
	/** 游戏手柄视角操纵杆死区值的设置器。*/
	UFUNCTION()
	void SetGamepadLookStickDeadZone(const float NewValue) { ChangeValueAndDirty(GamepadLookStickDeadZone, NewValue); }




private:
	/** Holds the gamepad move stick dead zone value. */
	/** 保存游戏手柄移动操纵杆的死区值。*/
	UPROPERTY()
	float GamepadMoveStickDeadZone;

	/** Holds the gamepad look stick dead zone value. */
	/** 存储游戏手柄瞄准杆的盲区值。*/
	UPROPERTY()
	float GamepadLookStickDeadZone;


	////////////////////////////////////////////////////////
	// Gamepad Trigger Haptics
	// 游戏手柄触觉反馈
public:
	UFUNCTION()
	bool GetTriggerHapticsEnabled() const { return bTriggerHapticsEnabled; }
	UFUNCTION()
	void SetTriggerHapticsEnabled(const bool NewValue) { ChangeValueAndDirty(bTriggerHapticsEnabled, NewValue); }

	UFUNCTION()
	bool GetTriggerPullUsesHapticThreshold() const { return bTriggerPullUsesHapticThreshold; }
	UFUNCTION()
	void SetTriggerPullUsesHapticThreshold(const bool NewValue) { ChangeValueAndDirty(bTriggerPullUsesHapticThreshold, NewValue); }

	UFUNCTION()
	uint8 GetTriggerHapticStrength() const { return TriggerHapticStrength; }
	UFUNCTION()
	void SetTriggerHapticStrength(const uint8 NewValue) { ChangeValueAndDirty(TriggerHapticStrength, NewValue); }

	UFUNCTION()
	uint8 GetTriggerHapticStartPosition() const { return TriggerHapticStartPosition; }
	UFUNCTION()
	void SetTriggerHapticStartPosition(const uint8 NewValue) { ChangeValueAndDirty(TriggerHapticStartPosition, NewValue); }
	



private:
	/** Are trigger haptics enabled? */
	/** 触觉触发功能已启用吗？*/
	UPROPERTY()
	bool bTriggerHapticsEnabled = false;
	
	/** Does the game use the haptic feedback as its threshold for judging button presses? */
	/** 游戏是否将触觉反馈作为判断按键操作的阈值？*/
	UPROPERTY()
	bool bTriggerPullUsesHapticThreshold = true;
	
	/** The strength of the trigger haptic effects. */
	/** 触发器触觉效果的强度。*/
	UPROPERTY()
	uint8 TriggerHapticStrength = 8;
	/** The start position of the trigger haptic effects */
	/** 触发式震动效果的起始位置 */
	UPROPERTY()
	uint8 TriggerHapticStartPosition = 0;


	////////////////////////////////////////////////////////
	/// Dirty and Change Reporting
	/// 污染与变更报告
private:
	// 如果值有改变 则改变并标记为脏
	template<typename T>
	bool ChangeValueAndDirty(T& CurrentValue, const T& NewValue)
	{
		if (CurrentValue != NewValue)
		{
			CurrentValue = NewValue;
			bIsDirty = true;
			OnSettingChanged.Broadcast(this);
			
			return true;
		}

		return false;
	}




	bool bIsDirty = false;

};