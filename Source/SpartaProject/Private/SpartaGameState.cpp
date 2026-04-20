#include "SpartaGameState.h"
#include "Kismet/GameplayStatics.h"
#include "SpawnVolume.h"
#include "SpartaGameInstance.h"
#include "CoinItem.h"
#include "BaseItem.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "SpartaPlayerController.h"
#include "Math/UnrealMathUtility.h"
#include "SpartaCharacter.h"

ASpartaGameState::ASpartaGameState()
{
	Score = 0;
	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;
	// 기본 값 (StartLevel에서 LevelDurations에 따라 덮어씌워짐)
	LevelDuration = 90.0f;
	// 기본 레벨별 시간: Level1=90, Level2=60, Level3=30
	LevelDurations = { 90.0f, 60.0f, 30.0f };

	CurrentLevelIndex = 0;
	MaxLevels = 3;
	CurrentWaveIndex = 0;
	MaxWaves = 3;

	// 기본 웨이브 스폰/감소 설정
	BaseItemsPerWave = 12;
	ItemDecayMultiplier = 0.8f; // 웨이브가 증가할 때마다 아이템 수에 곱해짐
	WaveTimeDecayMultiplier = 0.85f; // 웨이브가 증가할 때마다 웨이브 시간이 곱해짐

	bPersistCoinsInLevel = false;
	bPersistHealthInLevel = true;
}

void ASpartaGameState::BeginPlay()
{
	Super::BeginPlay();

	// 게임 시작 시 첫 레벨부터 진행
	StartLevel();
	GetWorldTimerManager().SetTimer(
		HUDUpdateTimerHandle,
		this,
		&ASpartaGameState::UpdateHUD,
		0.1f,
		true
	);
}

int32 ASpartaGameState::GetScore() const
{
	return Score;
}

void ASpartaGameState::AddScore(int32 Amount)
{
	// 레벨(또는 게임 상태) 내 즉시 반영용 점수 업데이트
	Score += Amount;

	if(UGameInstance* GameInstance = GetGameInstance())
	{
		if(USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance))
		{
			SpartaGameInstance->AddToScore(Amount);
		}
	}
}

void ASpartaGameState::StartLevel()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			SpartaPlayerController->ShowGameHUD();
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance))
		{
			CurrentLevelIndex = SpartaGameInstance->CurrentLevelIndex;
		}
	}

	// 레벨별 시간 적용 (LevelDurations에 정의된 값 사용, 없으면 기존 LevelDuration 유지)
	if (LevelDurations.IsValidIndex(CurrentLevelIndex))
	{
		LevelDuration = LevelDurations[CurrentLevelIndex];
	}

	// 한 레벨 전체가 시작될 때만 코인/체력 카운트 초기화
	SpawnedCoinCount = 0;
	CollectedCoinCount = 0;

	// 레벨 내부 웨이브 인덱스 초기화
	CurrentWaveIndex = 0;

	// 전체 레벨 제한 타이머 설정 (레벨 전체 시간이 지나면 강제 종료)
	GetWorldTimerManager().SetTimer(
		LevelTimerHandle,
		this,
		&ASpartaGameState::OnLevelTimeUp,
		LevelDuration,
		false
	);

	// 첫 웨이브 시작
	StartWave();
}


void ASpartaGameState::StartWave()
{
	// 현재 레벨 이름이 "MenuLevel"이면 on-screen 메시지 출력하지 않음
	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(GetWorld());
	if (!CurrentLevelName.Equals(TEXT("MenuLevel"), ESearchCase::IgnoreCase))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Starting Wave %d"), CurrentWaveIndex + 1));
		}
	}

	// bPersistCoinsInLevel이 false일 때만 기존 아이템을 제거하고 카운트 리셋
	if (!bPersistCoinsInLevel)
	{
		// 모든 기존 아이템을 제거 (웨이브마다 초기화)
		TArray<AActor*> FoundItems;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseItem::StaticClass(), FoundItems);
		for (AActor* Actor : FoundItems)
		{
			if (!Actor) continue;
			if (ABaseItem* Item = Cast<ABaseItem>(Actor))
			{
				Item->DestroyItem();
			}
			else
			{
				Actor->Destroy();
			}
		}

		// 코인 카운트 리셋 (레벨 전체에서 누적하려면 bPersistCoinsInLevel = true로 설정)
		SpawnedCoinCount = 0;
		CollectedCoinCount = 0;
	}

	// 해당 웨이브에서 스폰할 아이템 수 계산 (감소 로직 유지)
	int32 ItemsThisWave = FMath::Max(1, FMath::RoundToInt(BaseItemsPerWave * FMath::Pow(ItemDecayMultiplier, static_cast<float>(CurrentWaveIndex))));

	// 웨이브당 아이템 스폰
	SpawnItemsForWave(ItemsThisWave);

	// 웨이브 시간 계산
	float BaseWaveDuration = (MaxWaves > 0) ? (LevelDuration / static_cast<float>(MaxWaves)) : LevelDuration;
	float WaveDuration = BaseWaveDuration * FMath::Pow(WaveTimeDecayMultiplier, static_cast<float>(CurrentWaveIndex));
	WaveDuration = FMath::Max(WaveDuration, 1.0f);

	// 웨이브 타이머 설정
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);
	GetWorldTimerManager().SetTimer(
		WaveTimerHandle,
		this,
		&ASpartaGameState::OnWaveTimeUp,
		WaveDuration,
		false
	);

	UE_LOG(LogTemp, Warning, TEXT("StartWave: %d ItemsThisWave: %d WaveDuration: %.2f"), CurrentWaveIndex + 1, ItemsThisWave, WaveDuration);
}

void ASpartaGameState::SpawnItemsForWave(int32 Count)
{
	// 현재 맵에 배치된 모든 SpawnVolume을 찾아서 Count개를 스폰
	TArray<AActor*> FoundVolumes;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnVolume::StaticClass(), FoundVolumes);

	for (int32 i = 0; i < Count; i++)
	{
		if (FoundVolumes.Num() > 0)
		{
			// 첫번째 SpawnVolume 사용. 필요시 랜덤 선택으로 확장 가능
			ASpawnVolume* SpawnVolume = Cast<ASpawnVolume>(FoundVolumes[0]);
			if (SpawnVolume)
			{
				AActor* SpawnedActor = SpawnVolume->SpawnRandomItem();
				// 만약 스폰된 액터가 코인 타입이라면 SpawnedCoinCount 증가 (레벨 내 누적)
				if (SpawnedActor && SpawnedActor->IsA(ACoinItem::StaticClass()))
				{
					SpawnedCoinCount++;
				}
			}
		}
	}
}

void ASpartaGameState::OnWaveTimeUp()
{
	// 웨이브 시간이 끝나면 웨이브 종료 처리
	EndWave();
}

void ASpartaGameState::EndWave()
{
	// 웨이브 타이머 해제
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);

	// 다음 웨이브로 이동
	CurrentWaveIndex++;

	// 모든 웨이브를 돌았으면 레벨 종료
	if (CurrentWaveIndex >= MaxWaves)
	{
		EndLevel();
		return;
	}

	// 다음 웨이브 시작 (기존 아이템은 유지되므로 SpawnedCoinCount는 누적됨)
	StartWave();
}

void ASpartaGameState::OnLevelTimeUp()
{
	// 시간이 다 되면 레벨을 종료
	EndLevel();
}

void ASpartaGameState::OnCoinCollected()
{
	CollectedCoinCount++;

	UE_LOG(LogTemp, Warning, TEXT("Coin Collected: %d / %d"),
		CollectedCoinCount,
		SpawnedCoinCount);

	// 레벨 내 누적된 스폰 코인을 전부 주웠다면 즉시 레벨 종료 (bPersistCoinsInLevel=true 인 경우)
	if (bPersistCoinsInLevel)
	{
		if (SpawnedCoinCount > 0 && CollectedCoinCount >= SpawnedCoinCount)
		{
			EndLevel();
		}
	}
	else
	{
		// 웨이브 단위로 취급하는 경우, 현재 웨이브의 코인을 전부 주웠다면 웨이브 종료
		if (SpawnedCoinCount > 0 && CollectedCoinCount >= SpawnedCoinCount)
		{
			EndWave();
		}
	}
}

void ASpartaGameState::EndLevel()
{
	// 전체 레벨 타이머 및 웨이브 타이머 해제
	GetWorldTimerManager().ClearTimer(LevelTimerHandle);
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance))
		{
			AddScore(Score);
			CurrentLevelIndex++;
			SpartaGameInstance->CurrentLevelIndex = CurrentLevelIndex;
		}
	}

	// 모든 레벨을 다 돌았다면 게임 오버 처리
	if (CurrentLevelIndex >= MaxLevels)
	{
		OnGameOver();
		return;
	}

	// 레벨 맵 이름이 있으면 해당 맵 불러오기
	if (LevelMapNames.IsValidIndex(CurrentLevelIndex))
	{
		UGameplayStatics::OpenLevel(GetWorld(), LevelMapNames[CurrentLevelIndex]);
	}
	else
	{
		// 맵 이름이 없으면 게임오버
		OnGameOver();
	}
}

void ASpartaGameState::OnGameOver()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			SpartaPlayerController->SetPause(true);
			SpartaPlayerController->ShowMainMenu(true);
		}
	}
}

void ASpartaGameState::UpdateHUD()
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (ASpartaPlayerController* SpartaPlayerController = Cast<ASpartaPlayerController>(PlayerController))
		{
			if (UUserWidget* HUDWidget = SpartaPlayerController->GetHUDWidget())
			{
				if (UTextBlock* TimeText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Time"))))
				{
					float WaveRemaining = GetWorldTimerManager().GetTimerRemaining(WaveTimerHandle);
					TimeText->SetText(FText::FromString(FString::Printf(TEXT("Wave: %.1f"), WaveRemaining)));
				}

				if (UTextBlock* CoinText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Coin"))))
				{
					CoinText->SetText(FText::FromString(FString::Printf(TEXT("Coins: %d / %d"), CollectedCoinCount, SpawnedCoinCount)));
				}

				if (UTextBlock* ScoreText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Score"))))
				{
					if (UGameInstance* GameInstance = GetGameInstance())
					{
						USpartaGameInstance* SpartaGameInstance = Cast<USpartaGameInstance>(GameInstance);
						if (SpartaGameInstance)
						{
							ScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), SpartaGameInstance->TotalScore)));
						}
					}
				}

				if (UTextBlock* LevelIndexText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Level"))))
				{
					LevelIndexText->SetText(FText::FromString(FString::Printf(TEXT("Level: %d"), CurrentLevelIndex + 1)));
				}
				if (UTextBlock* WaveIndexText = Cast<UTextBlock>(HUDWidget->GetWidgetFromName(TEXT("Wave"))))
				{
					WaveIndexText->SetText(FText::FromString(FString::Printf(TEXT("Wave: %d/%d"), CurrentWaveIndex + 1, MaxWaves)));
				}

				// 플레이어 폰(캐릭터)에서 상태 가져오기
				ASpartaCharacter* SpartaCharacter = nullptr;
				if (APawn* Pawn = PlayerController->GetPawn())
				{
					SpartaCharacter = Cast<ASpartaCharacter>(Pawn);
				}

				// ReverseControlItem: 캐릭터의 bIsControlsReversed에 따라 Visible/Hidden
				if (UWidget* ReverseWidget = HUDWidget->GetWidgetFromName(TEXT("ReverseControllerItem")))
				{
					ReverseWidget->SetVisibility((SpartaCharacter && SpartaCharacter->bIsControlsReversed)
						? ESlateVisibility::Visible
						: ESlateVisibility::Hidden);
				}

				// SlowingItem: 캐릭터의 isSlowed에 따라 Visible/Hidden
				if (UWidget* SlowingWidget = HUDWidget->GetWidgetFromName(TEXT("SlowingItem")))
				{
					SlowingWidget->SetVisibility((SpartaCharacter && SpartaCharacter->isSlowed)
						? ESlateVisibility::Visible
						: ESlateVisibility::Hidden);
				}

				// BlindItem: 캐릭터의 IsBlinded에 따라 Visible/Hidden
				if (UWidget* BlindWidget = HUDWidget->GetWidgetFromName(TEXT("BlindItem")))
				{
					BlindWidget->SetVisibility((SpartaCharacter && SpartaCharacter->IsBlinded)
						? ESlateVisibility::Visible
						: ESlateVisibility::Hidden);
				}
			}
		}
	}
}