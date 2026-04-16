

#include "SpartaGameMode.h"
#include "SpartaCharacter.h"
#include "SpartaPlayerController.h"
#include "SpartaProject/Public/SpartaPlayerController.h"
#include "SpartaGameState.h"

ASpartaGameMode::ASpartaGameMode()
{
	DefaultPawnClass = ASpartaCharacter::StaticClass();
	PlayerControllerClass =  ASpartaPlayerController::StaticClass();
	// 우리가 만든 GameState로 설정
	GameStateClass = ASpartaGameState::StaticClass();
}
