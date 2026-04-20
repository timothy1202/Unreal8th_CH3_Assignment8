#include "SpikeItem.h"
#include "Kismet/GameplayStatics.h"
#include "SpartaCharacter.h"
#include "TimerManager.h"
#include "Components/SphereComponent.h"
#include "SpartaGameState.h"
#include "Engine/World.h"

ASpikeItem::ASpikeItem()
{
	ItemType = "SpikeItem";

	StaticMesh->SetVisibility(false);
	bIsActive = false;
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ASpikeItem::BeginPlay()
{
	Super::BeginPlay();

	// 일정 간격으로 토글 시작
	if (ToggleInterval <= 0.0f)
	{
		ToggleInterval = 3.0f;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	if (ASpartaGameState* SpartaState = Cast<ASpartaGameState>(World->GetGameState()))
	{
		if (SpartaState->GetCurrentWave() < 2)
		{
			return;
		}
	}

	// 즉시 첫 토글 상태(원하면 호출을 지연시킬 수 있음)
	// 현재는 타이머 시작 시 첫 호출은 ToggleInterval 후이므로, 초기 보이지 않는 상태 유지
}

void ASpikeItem::ToggleActiveState()
{
	bIsActive = !bIsActive;

	// StaticMesh 보임/숨김
	StaticMesh->SetVisibility(bIsActive);

	// 박스 콜리전 활성/비활성
	if (bIsActive)
	{
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ASpikeItem::StartSpike()
{
	GetWorldTimerManager().SetTimer(
		ToggleTimerHandle,
		this,
		&ASpikeItem::ToggleActiveState,
		ToggleInterval,
		true
	);
}

void ASpikeItem::ActivateItem(AActor* Activator)
{
	Super::ActivateItem(Activator);

	if (!bIsActive) return;

	if (Activator && Activator->ActorHasTag("Player"))
	{
		// 데미지 적용
		const float SpikeDamage = 20.0f;
		UGameplayStatics::ApplyDamage(
			Activator,
			SpikeDamage,
			nullptr,
			this,
			UDamageType::StaticClass()
		);

		// 살짝 밀어내기 (플레이어가 ACharacter 기반임을 가정)
		if (ASpartaCharacter* Player = Cast<ASpartaCharacter>(Activator))
		{
			// Spike에서 플레이어 방향으로의 수평 방향을 계산
			FVector Direction = Player->GetActorLocation() - GetActorLocation();
			Direction.Z = 0.0f;
			Direction = Direction.GetSafeNormal();

			const float KnockbackStrength = 600.0f; // 필요시 조정
			const float UpwardBoost = 200.0f; // 약간 뜨게 할 값

			FVector LaunchVelocity = Direction * KnockbackStrength + FVector(0.0f, 0.0f, UpwardBoost);
			// X/Y 및 Z 적용을 덮어쓰도록 호출 (작은 밀림 효과)
			Player->LaunchCharacter(LaunchVelocity, true, true);
		}

	}

}
