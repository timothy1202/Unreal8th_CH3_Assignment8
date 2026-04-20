// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SpartaGameState.generated.h"

UCLASS()
class SPARTAPROJECT_API ASpartaGameState : public AGameState
{
	GENERATED_BODY()
public:
	ASpartaGameState();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Score")
	int32 Score;
	// 현재 레벨에서 스폰된 코인 개수 (레벨 내에서 누적)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
	int32 SpawnedCoinCount;
	// 플레이어가 수집한 코인 개수 (레벨 내에서 누적)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coin")
	int32 CollectedCoinCount;
	// 각 레벨이 유지되는 시간 (초 단위) - 현재 사용 중인 값 (StartLevel에서 LevelDurations에 따라 설정됨)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	float LevelDuration;
	// 레벨별 시간 배열 (Index 0 -> Level1, 1 -> Level2, ...)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TArray<float> LevelDurations;
	// 현재 진행 중인 레벨 인덱스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
	int32 CurrentLevelIndex;
	// 전체 레벨의 개수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
	int32 MaxLevels;
	// 실제 레벨 맵 이름 배열. 여기 있는 인덱스를 차례대로 연동
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TArray<FName> LevelMapNames;
	// 현재 웨이브 인덱스 (0-based)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
	int32 CurrentWaveIndex;
	// 한 레벨 안의 최대 웨이브 수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level")
	int32 MaxWaves;

	// 웨이브 스폰 관련: 기본값과 감소 계수 (웨이브가 올라갈수록 곱해짐)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Wave")
	int32 BaseItemsPerWave;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Wave", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float ItemDecayMultiplier;

	// 웨이브 시간 감소 계수 (0.0 ~ 1.0). 1.0이면 시간 고정, 0.8이면 매 웨이브 80% 씩 감소
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Wave", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float WaveTimeDecayMultiplier;

	// 매 레벨 전체 타이머 및 웨이브 타이머 핸들
	FTimerHandle LevelTimerHandle;
	FTimerHandle WaveTimerHandle;
	FTimerHandle HUDUpdateTimerHandle;

	// 코인/체력 유지 옵션 (레벨 내에서 이어지게 할지)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Persistence")
	bool bPersistCoinsInLevel;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level|Persistence")
	bool bPersistHealthInLevel;

	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetScore() const;
	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddScore(int32 Amount);
	// 게임이 완전히 끝났을 때 (모든 레벨 종료) 실행되는 함수
	UFUNCTION(BlueprintCallable, Category = "Level")
	void OnGameOver();

	// 레벨을 시작할 때 호출 (레벨 단위 초기화 후 첫 웨이브 시작)
	UFUNCTION(BlueprintCallable, Category = "Level")
	void StartLevel();
	// 웨이브를 시작할 때 호출 (아이템 추가 스폰)
	void StartWave();
	// 웨이브 시간이 끝났을 때 호출
	void OnWaveTimeUp();
	// 웨이브 종료 처리 (다음 웨이브 시작 또는 레벨 종료)
	void EndWave();

	// 레벨 제한 시간이 만료되었을 때 호출 (레벨 강제 종료)
	void OnLevelTimeUp();
	// 웨이브 당 아이템 스폰 실제 처리 (Count 만큼 스폰)
	void SpawnItemsForWave(int32 Count);

	// 코인을 주웠을 때 호출
	void OnCoinCollected();
	// 레벨을 강제 종료하고 다음 레벨로 이동
	void EndLevel();
	void UpdateHUD();
	int32 GetCurrentWave() const { return CurrentWaveIndex + 1; }
	void MakeSpike();
	void MakeBomb();
};
