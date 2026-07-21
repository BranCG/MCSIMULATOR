// Copyright BranCG 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MCSIMULATORGameInstance.generated.h"

/**
 * Custom GameInstance that persists selected scenario settings and AI evaluation context
 * across map transitions in Unreal Engine.
 */
UCLASS()
class MCSIMULATOR_API UMCSIMULATORGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UMCSIMULATORGameInstance();

	// Currently selected scenario ID (e.g. "Auditorio", "Classroom", "Interview")
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MC Simulator|Scenario")
	FString SelectedScenarioId;

	// Friendly name of scenario for UI (e.g. "Auditorio Principal")
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MC Simulator|Scenario")
	FString SelectedScenarioName;

	// Detailed context sent to AI API (e.g. "Presentación de impacto frente a gran audiencia en auditorio")
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MC Simulator|Scenario")
	FString SelectedAIContext;

	// Helper function to set current scenario state
	UFUNCTION(BlueprintCallable, Category = "MC Simulator|Scenario")
	void SetCurrentScenario(const FString& ScenarioId, const FString& ScenarioName, const FString& AIContext);

	// Get active AI evaluation context string
	UFUNCTION(BlueprintPure, Category = "MC Simulator|Scenario")
	FString GetActiveAIContext() const;
};
