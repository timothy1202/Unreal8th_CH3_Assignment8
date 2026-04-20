#include "SpawnVolume.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MineItem.h"
#include "SpartaGameState.h"
#include "BombItem.h"

ASpawnVolume::ASpawnVolume()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	RootComponent = Scene;

	SpawningBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawningBox"));
	SpawningBox->SetupAttachment(Scene);

	ItemDataTable = nullptr;

	// Explosion defaults
	ExplosionClass = nullptr;
	ExplosionSpawnInterval = 5.0f;

	TargetWaveNumber = 3;
}

void ASpawnVolume::BeginPlay()
{
	Super::BeginPlay();
}

FVector ASpawnVolume::GetRandomPointInVolume() const
{
	FVector BoxExtent = SpawningBox->GetScaledBoxExtent();
	FVector BoxOrigin = SpawningBox->GetComponentLocation();

	return BoxOrigin + FVector(
		FMath::FRandRange(-BoxExtent.X, BoxExtent.X),
		FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y),
		FMath::FRandRange(-BoxExtent.Z, BoxExtent.Z)
	);

}

AActor* ASpawnVolume::SpawnRandomItem()
{
	if (FItemSpawnRow* SelectedRow = GetRandomItem())
	{
		if (UClass* ActualClass = SelectedRow->ItemClass.Get())
		{
			// 여기서 SpawnItem()을 호출하고, 스폰된 AActor 포인터를 리턴
			return SpawnItem(ActualClass);
		}
	}
	return nullptr;
}

FItemSpawnRow* ASpawnVolume::GetRandomItem() const
{
	if (!ItemDataTable) return nullptr;

	// 1) 모든 Row(행) 가져오기
	TArray<FItemSpawnRow*> AllRows;
	static const FString ContextString(TEXT("ItemSpawnContext"));
	ItemDataTable->GetAllRows(ContextString, AllRows);

	if (AllRows.IsEmpty()) return nullptr;

	// 2) 전체 확률 합 구하기
	float TotalChance = 0.0f; // 초기화
	for (const FItemSpawnRow* Row : AllRows) // AllRows 배열의 각 Row를 순회
	{
		if (Row) // Row가 유효한지 확인
		{
			TotalChance += Row->SpawnChance; // SpawnChance 값을 TotalChance에 더하기
		}
	}

	// 3) 0 ~ TotalChance 사이 랜덤 값
	const float RandValue = FMath::FRandRange(0.0f, TotalChance);
	float AccumulateChance = 0.0f;

	// 4) 누적 확률로 아이템 선택
	for (FItemSpawnRow* Row : AllRows)
	{
		AccumulateChance += Row->SpawnChance;
		if (RandValue <= AccumulateChance)
		{
			return Row;
		}
	}

	return nullptr;
}

AActor* ASpawnVolume::SpawnItem(TSubclassOf<AActor> ItemClass)
{
	if (!ItemClass) return nullptr;

	// SpawnActor가 성공하면 스폰된 액터의 포인터가 반환됨
	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(
		ItemClass,
		GetRandomPointInVolume(),
		FRotator::ZeroRotator
	);

	return SpawnedActor;
}

// --- Explosion spawning implementation ---

void ASpawnVolume::StartSpawningExplosions()
{
	if (!ExplosionClass) return;

	// 이미 타이머가 돌고 있으면 재설정
	GetWorld()->GetTimerManager().ClearTimer(ExplosionSpawnTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		ExplosionSpawnTimerHandle,
		this,
		&ASpawnVolume::SpawnRandomExplosion,
		FMath::Max(0.01f, ExplosionSpawnInterval),
		true,
		0.0f
	);

}


void ASpawnVolume::StopSpawningExplosions()
{
	GetWorld()->GetTimerManager().ClearTimer(ExplosionSpawnTimerHandle);
}

void ASpawnVolume::SpawnRandomExplosion()
{
	ASpartaGameState* GS = GetWorld() ? GetWorld()->GetGameState<ASpartaGameState>() : nullptr;
	if (!GS) return;
	
	const int32 CurrentWaveOneBased = GS->CurrentWaveIndex + 1; // GameState는 0-based이므로 +1
	if (!ExplosionClass || !SpawningBox) return; // SpawningBox가 유효한지 확인
	
	// 1. 박스의 중심점과 크기(Extent) 가져오기
	FVector BoxLocation = SpawningBox->GetComponentLocation();
	FVector BoxExtent = SpawningBox->GetScaledBoxExtent();
	
	// 2. 박스의 최소/최대 지점을 계산하여 FBox 생성
	FBox Bounds(BoxLocation - BoxExtent, BoxLocation + BoxExtent);

	// 5. 5번 반복하여 스폰
	for (int32 i = 0; i < 5; ++i)
	{
		// 박스 범위 내에서 랜덤 위치 생성
		FVector SpawnLocation = FMath::RandPointInBox(Bounds);

		// 액터 스폰
		AActor* Spawned = GetWorld()->SpawnActor<AActor>(ExplosionClass, SpawnLocation, FRotator::ZeroRotator);

		// 스폰 성공 확인 및 초기화
		if (Spawned)
		{
			if (ABombItem* Bomb = Cast<ABombItem>(Spawned))
			{
				UE_LOG(LogTemp, Warning, TEXT("Spawned explosion #%d at %s"), i + 1, *SpawnLocation.ToString());
				Bomb->ActivateItem(nullptr);
			}
		}
	}
}