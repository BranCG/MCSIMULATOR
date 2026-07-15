#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MCSIMULATORGameMode.generated.h"

/**
 * Base Game Mode for MC Simulator.
 */
UCLASS()
class MCSIMULATOR_API AMCSIMULATORGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMCSIMULATORGameMode();
};
