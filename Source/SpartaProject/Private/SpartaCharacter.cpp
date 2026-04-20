#include "SpartaCharacter.h"
#include "SpartaPlayerController.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"
#include "SpartaGameState.h"
#include "Components/ProgressBar.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

ASpartaCharacter::ASpartaCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComp->SetupAttachment(RootComponent);
	SpringArmComp->TargetArmLength = 300.0f;
	SpringArmComp->bUsePawnControlRotation = true;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(GetMesh());
	OverheadWidget->SetWidgetSpace(EWidgetSpace::Screen);

	NormalSpeed = 600.0f;
	SprintSpeedMultiplier = 1.5f;
	SprintSpeed = NormalSpeed * SprintSpeedMultiplier;

	GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;

	// 초기 체력 설정
	MaxHealth = 100.0f;
	Health = MaxHealth;

	// 입력 반전 기본값
	bIsControlsReversed = false;
	isSlowed = false;
	IsBlinded = false;
}

void ASpartaCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdateHPBar(Health, MaxHealth);
}

void ASpartaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Enhanced InputComponent로 캐스팅
	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// IA를 가져오기 위해 현재 소유 중인 Controller를 ASpartaPlayerController로 캐스팅
		if (ASpartaPlayerController* PlayerController = Cast<ASpartaPlayerController>(GetController()))
		{
			if (PlayerController->MoveAction)
			{
				// IA_Move 액션 키를 "키를 누르고 있는 동안" Move() 호출
				EnhancedInput->BindAction(
					PlayerController->MoveAction,
					ETriggerEvent::Triggered,
					this,
					&ASpartaCharacter::Move
				);
			}

			if (PlayerController->JumpAction)
			{
				// IA_Jump 액션 키를 "키를 누르고 있는 동안" StartJump() 호출
				EnhancedInput->BindAction(
					PlayerController->JumpAction,
					ETriggerEvent::Triggered,
					this,
					&ASpartaCharacter::StartJump
				);

				// IA_Jump 액션 키에서 "손을 뗀 순간" StopJump() 호출
				EnhancedInput->BindAction(
					PlayerController->JumpAction,
					ETriggerEvent::Completed,
					this,
					&ASpartaCharacter::StopJump
				);
			}

			if (PlayerController->LookAction)
			{
				// IA_Look 액션 마우스가 "움직일 때" Look() 호출
				EnhancedInput->BindAction(
					PlayerController->LookAction,
					ETriggerEvent::Triggered,
					this,
					&ASpartaCharacter::Look
				);
			}

			if (PlayerController->SprintAction)
			{
				// IA_Sprint 액션 키를 "누르고 있는 동안" StartSprint() 호출
				EnhancedInput->BindAction(
					PlayerController->SprintAction,
					ETriggerEvent::Triggered,
					this,
					&ASpartaCharacter::StartSprint
				);
				// IA_Sprint 액션 키에서 "손을 뗀 순간" StopSprint() 호출
				EnhancedInput->BindAction(
					PlayerController->SprintAction,
					ETriggerEvent::Completed,
					this,
					&ASpartaCharacter::StopSprint
				);
			}
		}
	}
}

void ASpartaCharacter::Move(const FInputActionValue& value)
{
	if (!Controller) return;
	// 입력을 수정해야 하므로 non-const로 받음
	FVector2D MoveInput = value.Get<FVector2D>();

	// 입력 반전 상태이면 X,Y 모두 반전
	if (bIsControlsReversed)
	{
		MoveInput *= -1.0f;
	}

	if (!FMath::IsNearlyZero(MoveInput.X))
	{
		AddMovementInput(GetActorForwardVector(), MoveInput.X);
	}

	if (!FMath::IsNearlyZero(MoveInput.Y))
	{
		AddMovementInput(GetActorRightVector(), MoveInput.Y);
	}
}

void ASpartaCharacter::StartJump(const FInputActionValue& value)
{
	if (value.Get<bool>())
	{
		Jump();
	}
}

void ASpartaCharacter::StopJump(const FInputActionValue& value)
{
	if (!value.Get<bool>())
	{
		StopJumping();
	}
}

void ASpartaCharacter::Look(const FInputActionValue& value)
{
	FVector2D LookInput = value.Get<FVector2D>();
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void ASpartaCharacter::StartSprint(const FInputActionValue& value)
{
	if (isSlowed) return; // 느려지는 효과가 적용된 상태에서는 달리기 불가능
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void ASpartaCharacter::StopSprint(const FInputActionValue& value)
{
	if (isSlowed) return; // 느려지는 효과가 적용된 상태에서는 달리기 불가능
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
	}
}

int32 ASpartaCharacter::GetHealth() const
{
	return Health;
}

// 체력 회복 함수
void ASpartaCharacter::AddHealth(float Amount)
{
	// 체력을 회복시킴. 최대 체력을 초과하지 않도록 제한함
	Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
	UpdateHPBar(Health, MaxHealth);
}

void ASpartaCharacter::ApplySlowingEffect()
{
	if (GetCharacterMovement())
	{
		isSlowed = true;
		GetCharacterMovement()->MaxWalkSpeed *= 0.5f; // 이동 속도를 50%로 감소
		// 타이머를 설정하여 일정 시간 후에 원래 속도로 복구
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this]()
			{
				if (GetCharacterMovement())
				{
					GetCharacterMovement()->MaxWalkSpeed = NormalSpeed; // 원래 속도로 복구
					isSlowed = false; // 느려지는 효과 해제
				}
			}, 5.0f, false); // 5초 후에 실행
	}
}

void ASpartaCharacter::ApplyReverseControlEffect()
{
	// 입력 반전 적용
	bIsControlsReversed = true;

	// 5초 후 원상복구
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			bIsControlsReversed = false;
		}, 5.0f, false);
}

// 데미지 처리 함수
float ASpartaCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 기본 데미지 처리 로직 호출 (필수는 아님)
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 체력을 데미지만큼 감소시키고, 0 이하로 떨어지지 않도록 Clamp
	Health = FMath::Clamp(Health - DamageAmount, 0.0f, MaxHealth);
	UpdateHPBar(Health, MaxHealth);

	// 체력이 0 이하가 되면 사망 처리
	if (Health <= 0.0f)
	{
		OnDeath();
	}

	// 실제 적용된 데미지를 반환
	return ActualDamage;
}

// 사망 처리 함수
void ASpartaCharacter::OnDeath()
{
	ASpartaGameState* SpartaGameState = GetWorld() ? GetWorld()->GetGameState<ASpartaGameState>() : nullptr;
	if (SpartaGameState)
	{
		SpartaGameState->OnGameOver();
	}
}

void ASpartaCharacter::UpdateHPBar(float CurrentHP, float MaxHP)
{
	if (OverheadWidget)
	{
		// 1. 위젯 컴포넌트에서 실제 UserWidget 클래스를 가져옵니다.
		UUserWidget* MyUserWidget = Cast<UUserWidget>(OverheadWidget->GetUserWidgetObject());

		if (MyUserWidget)
		{
			// 2. 위젯 안에서 이름이 "HPBar"인 프로그레스 바를 찾습니다.
			// (블루프린트 위젯에서 ProgressBar의 변수 이름이 "HPBar"여야 하며, 'Is Variable'이 체크되어 있어야 합니다.)
			UProgressBar* HPBar = Cast<UProgressBar>(MyUserWidget->GetWidgetFromName(TEXT("OverheadWidget")));

			if (HPBar)
			{
				// 3. 퍼센트 설정 (0.0 ~ 1.0 사이 값)
				float Percent = CurrentHP / MaxHP;
				HPBar->SetPercent(Percent);
			}
		}
	}
}

void ASpartaCharacter::ApplyBlindEffect()
{
	if (ASpartaPlayerController* PC = Cast<ASpartaPlayerController>(GetController()))
	{
		// 로컬 컨트롤러인지 확인 (서버에서 실행되는 경우 PlayerCameraManager가 없을 수 있음)
		if (PC->IsLocalController() && PC->PlayerCameraManager)
		{
			IsBlinded = true;
			// 즉시 페이드 인 (0 -> 1)
			PC->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, 0.1f, FLinearColor::Black, false, true);

			// 2초 후 페이드 아웃 (1 -> 0)
			FTimerHandle TimerHandle;
			TWeakObjectPtr<ASpartaCharacter> WeakSelf(this);
			TWeakObjectPtr<APlayerController> WeakPC(PC);

			// WeakSelf와 WeakPC를 캡처하도록 수정
			FTimerDelegate FadeOutDelegate = FTimerDelegate::CreateLambda([WeakSelf, WeakPC]()
				{
					if (ASpartaCharacter* Self = WeakSelf.Get())
					{
						Self->IsBlinded = false; // 상태 초기화
					}
					if (APlayerController* P = WeakPC.Get())
					{
						if (P->PlayerCameraManager)
						{
							P->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 0.1f, FLinearColor::Black, false, true);
						}
					}
				});
			GetWorldTimerManager().SetTimer(TimerHandle, FadeOutDelegate, 2.0f, false);
		}
	}
}