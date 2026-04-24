// Copyright Epic Games, Inc. All Rights Reserved.

#include "LyraSettingsShared.h"

#include "Framework/Application/SlateApplication.h"
#include "Internationalization/Culture.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Player/LyraLocalPlayer.h"
#include "Rendering/SlateRenderer.h"
#include "SubtitleDisplaySubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraSettingsShared)

// 共享设置所使用的名字
static FString SHARED_SETTINGS_SLOT_NAME = TEXT("SharedGameSettings");

namespace LyraSettingsSharedCVars
{
	static float DefaultGamepadLeftStickInnerDeadZone = 0.25f;
	static FAutoConsoleVariableRef CVarGamepadLeftStickInnerDeadZone(
		TEXT("gpad.DefaultLeftStickInnerDeadZone"),
		DefaultGamepadLeftStickInnerDeadZone,
		TEXT("Gamepad left stick inner deadzone")
	);

	static float DefaultGamepadRightStickInnerDeadZone = 0.25f;
	static FAutoConsoleVariableRef CVarGamepadRightStickInnerDeadZone(
		TEXT("gpad.DefaultRightStickInnerDeadZone"),
		DefaultGamepadRightStickInnerDeadZone,
		TEXT("Gamepad right stick inner deadzone")
	);	
}

ULyraSettingsShared::ULyraSettingsShared()
{
	/** 当当前文化发生改变时进行广播 */
	FInternationalization::Get().OnCultureChanged().AddUObject(this, &ThisClass::OnCultureChanged);
	
	GamepadMoveStickDeadZone = LyraSettingsSharedCVars::DefaultGamepadLeftStickInnerDeadZone;
	GamepadLookStickDeadZone = LyraSettingsSharedCVars::DefaultGamepadRightStickInnerDeadZone;
}

int32 ULyraSettingsShared::GetLatestDataVersion() const
{
	// 0 = before subclassing ULocalPlayerSaveGame
	// 1 = first proper version

	// 0 = 在对 ULocalPlayerSaveGame 进行子类化之前
	// 1 = 第一个正式版本
	
	return 1;
}

ULyraSettingsShared* ULyraSettingsShared::CreateTemporarySettings(const ULyraLocalPlayer* LocalPlayer)
{
	// This is not loaded from disk but should be set up to save
	// 这并非从磁盘加载而来，而是应当进行设置以便进行保存操作。

	// 这里是直接调用的父类的方法
	ULyraSettingsShared* SharedSettings = Cast<ULyraSettingsShared>(CreateNewSaveGameForLocalPlayer(ULyraSettingsShared::StaticClass(), LocalPlayer, SHARED_SETTINGS_SLOT_NAME));

	SharedSettings->ApplySettings();
	
	return SharedSettings;
}

ULyraSettingsShared* ULyraSettingsShared::LoadOrCreateSettings(const ULyraLocalPlayer* LocalPlayer)
{
	// This will stall the main thread while it loads
	// 这会在加载过程中使主线程暂停运行

	ULyraSettingsShared* SharedSettings = Cast<ULyraSettingsShared>(LoadOrCreateSaveGameForLocalPlayer(ULyraSettingsShared::StaticClass(), LocalPlayer, SHARED_SETTINGS_SLOT_NAME));

	SharedSettings->ApplySettings();
	
	return SharedSettings;
}

bool ULyraSettingsShared::AsyncLoadOrCreateSettings(const ULyraLocalPlayer* LocalPlayer,
	FOnSettingsLoadedEvent Delegate)
{
	FOnLocalPlayerSaveGameLoadedNative Lambda = FOnLocalPlayerSaveGameLoadedNative::CreateLambda([Delegate](ULocalPlayerSaveGame* LoadedSave)
	{
		ULyraSettingsShared* LoadedSettings = CastChecked<ULyraSettingsShared>(LoadedSave);

		LoadedSettings->ApplySettings();

		Delegate.ExecuteIfBound(LoadedSettings);
	}

	);

	return ULocalPlayerSaveGame::AsyncLoadOrCreateSaveGameForLocalPlayer(ULyraSettingsShared::StaticClass(), LocalPlayer, SHARED_SETTINGS_SLOT_NAME, Lambda);

}

void ULyraSettingsShared::SaveSettings()
{
	// Schedule an async save because it's okay if it fails
	// 安排一次异步保存操作，因为即便保存失败也没关系。
	AsyncSaveGameToSlotForLocalPlayer();

	// TODO_BH: Move this to the serialize function instead with a bumped version number
	// 待办事项_BH：将此内容移至序列化函数中，并使用更高的版本号进行处理
	if (UEnhancedInputLocalPlayerSubsystem* System = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwningPlayer))
	{
		if (UEnhancedInputUserSettings* InputSettings = System->GetUserSettings())
		{
			InputSettings->AsyncSaveSettings();
		}
	}

}

void ULyraSettingsShared::ApplySettings()
{

	// 应用字幕设置
	ApplySubtitleOptions();

	// 应用后台音量设置
	ApplyBackgroundAudioSettings();
	

	// 应用文化设置
	ApplyCultureSettings();


	
	
	// 应用输入设置
	if (UEnhancedInputLocalPlayerSubsystem* System = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwningPlayer))
	{
		if (UEnhancedInputUserSettings* InputSettings = System->GetUserSettings())
		{
			InputSettings->ApplySettings();
		}
	}
	
}

EColorBlindMode ULyraSettingsShared::GetColorBlindMode() const
{
	return ColorBlindMode;
}

void ULyraSettingsShared::SetColorBlindMode(EColorBlindMode InMode)
{
	if (ColorBlindMode != InMode)
	{
		ColorBlindMode = InMode;
		FSlateApplication::Get().GetRenderer()->SetColorVisionDeficiencyType(
			(EColorVisionDeficiency)(int32)ColorBlindMode, (int32)ColorBlindStrength, true, false);
	}
}

int32 ULyraSettingsShared::GetColorBlindStrength() const
{
	return ColorBlindStrength;
}

void ULyraSettingsShared::SetColorBlindStrength(int32 InColorBlindStrength)
{
	InColorBlindStrength = FMath::Clamp(InColorBlindStrength, 0, 10);
	if (ColorBlindStrength != InColorBlindStrength)
	{
		ColorBlindStrength = InColorBlindStrength;
		FSlateApplication::Get().GetRenderer()->SetColorVisionDeficiencyType(
			(EColorVisionDeficiency)(int32)ColorBlindMode, (int32)ColorBlindStrength, true, false);
	}
}

void ULyraSettingsShared::ApplySubtitleOptions()
{
	// 获取字幕展示的子系统
	if (USubtitleDisplaySubsystem* SubtitleSystem = USubtitleDisplaySubsystem::Get(OwningPlayer))
	{
		// 字幕格式
		FSubtitleFormat SubtitleFormat;
		SubtitleFormat.SubtitleTextSize = SubtitleTextSize;
		SubtitleFormat.SubtitleTextColor = SubtitleTextColor;
		SubtitleFormat.SubtitleTextBorder = SubtitleTextBorder;
		SubtitleFormat.SubtitleBackgroundOpacity = SubtitleBackgroundOpacity;

		SubtitleSystem->SetSubtitleDisplayOptions(SubtitleFormat);
		
	}
	
}

void ULyraSettingsShared::SetAllowAudioInBackgroundSetting(ELyraAllowBackgroundAudioSetting NewValue)
{
	if (ChangeValueAndDirty(AllowAudioInBackground, NewValue))
	{

		ApplyBackgroundAudioSettings();
	}
	
}

void ULyraSettingsShared::ApplyBackgroundAudioSettings()
{
	if (OwningPlayer && OwningPlayer->IsPrimaryPlayer())
	{
		/**
		 * 设置未聚焦音量倍数
		 * 
		 */
		FApp::SetUnfocusedVolumeMultiplier((AllowAudioInBackground != ELyraAllowBackgroundAudioSetting::Off) ? 1.0f : 0.0f);

		
	}	

	
}

const FString& ULyraSettingsShared::GetPendingCulture() const
{

	return  PendingCulture;
}

void ULyraSettingsShared::SetPendingCulture(const FString& NewCulture)
{
	PendingCulture = NewCulture;
	bResetToDefaultCulture = false;
	bIsDirty = true;
}

void ULyraSettingsShared::OnCultureChanged()
{
	ClearPendingCulture();
	bResetToDefaultCulture = false;
}

void ULyraSettingsShared::ClearPendingCulture()
{
	PendingCulture.Reset();
}

bool ULyraSettingsShared::IsUsingDefaultCulture() const
{
	FString Culture;

	GConfig->GetString(TEXT("Internationalization"), TEXT("Culture"), Culture, GGameUserSettingsIni);

	return Culture.IsEmpty();
	
}

void ULyraSettingsShared::ResetToDefaultCulture()
{
	ClearPendingCulture();
	bResetToDefaultCulture = true;
	bIsDirty = true;
	
}

void ULyraSettingsShared::ApplyCultureSettings()
{
	// 是否需要重置到默认
	if (bResetToDefaultCulture)
	{
		const FCulturePtr SystemDefaultCulture = FInternationalization::Get().GetDefaultCulture();
		check(SystemDefaultCulture.IsValid());


		const FString CultureToApply = SystemDefaultCulture->GetName();

	
		/**
		 * 按名称设置当前文化。
		 * @注意 此函数功能强大，会同时设置语言和区域设置，并清除可能已设置的任何资产组文化。
		 * @注意 在核心/引擎代码中应避免使用 SetCurrentCulture，因为这可能会覆盖编辑器/游戏用户的设置。
		 * 
		 */	
		if (FInternationalization::Get().SetCurrentCulture(CultureToApply))
		{
			// Clear this string
			GConfig->RemoveKey(TEXT("Internationalization"), TEXT("Culture"), GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
			
		}
		bResetToDefaultCulture = false;
		
	}
	else if (!PendingCulture.IsEmpty())
	{
		// SetCurrentCulture may trigger PendingCulture to be cleared (if a culture change is broadcast) so we take a copy of it to work with
		// 设置当前文化可能会导致“待处理文化”被清除（如果进行了文化变更广播的话），因此我们先复制一份该文化信息以便进行处理。
		const FString CultureToApply = PendingCulture;
		if (FInternationalization::Get().SetCurrentCulture(CultureToApply))
		{
			// Note: This is intentionally saved to the users config
			// We need to localize text before the player logs in and very early in the loading screen


			// 注意：此内容是特意保存至用户配置文件中的。
			// 我们需要在玩家登录之前以及在加载界面的早期阶段对文本进行本地化处理。
			GConfig->SetString(TEXT("Internationalization"), TEXT("Culture"), *CultureToApply, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);

		}
		
		ClearPendingCulture();
		
	}


	
}

void ULyraSettingsShared::ResetCultureToCurrentSettings()
{
	ClearPendingCulture();
	bResetToDefaultCulture = false;
	
}

void ULyraSettingsShared::ApplyInputSensitivity()
{
}
