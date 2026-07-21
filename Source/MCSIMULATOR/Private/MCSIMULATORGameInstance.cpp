// Copyright BranCG 2026. All Rights Reserved.

#include "MCSIMULATORGameInstance.h"

UMCSIMULATORGameInstance::UMCSIMULATORGameInstance()
{
	// Default to Auditorium scenario
	SelectedScenarioId = TEXT("Auditorio");
	SelectedScenarioName = TEXT("Auditorio Principal");
	SelectedAIContext = TEXT("Presentación oral masiva frente a jurados y audiencia en un auditorio principal.");
}

void UMCSIMULATORGameInstance::SetCurrentScenario(const FString& ScenarioId, const FString& ScenarioName, const FString& AIContext)
{
	SelectedScenarioId = ScenarioId;
	SelectedScenarioName = ScenarioName;
	SelectedAIContext = AIContext;

	UE_LOG(LogTemp, Log, TEXT("MC Simulator: Selected Scenario updated -> ID: %s | Name: %s | AI Context: %s"),
		*SelectedScenarioId, *SelectedScenarioName, *SelectedAIContext);
}

FString UMCSIMULATORGameInstance::GetActiveAIContext() const
{
	return SelectedAIContext;
}
