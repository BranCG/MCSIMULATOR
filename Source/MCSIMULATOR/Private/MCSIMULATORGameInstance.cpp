#include "MCSIMULATORGameInstance.h"
#include "Kismet/GameplayStatics.h"

UMCSIMULATORGameInstance::UMCSIMULATORGameInstance()
{
	// Default to Auditorium scenario
	SelectedScenarioId = TEXT("Auditorio");
	SelectedScenarioName = TEXT("Auditorio Principal");
	SelectedAIContext = TEXT("Presentación oral masiva frente a jurados y audiencia en un auditorio principal.");
	PreviousMapName = TEXT("AuditorioMap");
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

void UMCSIMULATORGameInstance::OpenRelaxationScene()
{
	UWorld* World = GetWorld();
	if (World)
	{
		PreviousMapName = World->GetMapName();
		PreviousMapName.RemoveFromStart(World->StreamingLevelsPrefix);
		UE_LOG(LogTemp, Log, TEXT("MC Simulator: Opening Relaxation scene from %s"), *PreviousMapName);
		UGameplayStatics::OpenLevel(this, FName("RelaxMap"));
	}
}

void UMCSIMULATORGameInstance::ReturnFromRelaxation()
{
	FName TargetMap = PreviousMapName.IsEmpty() ? FName("AuditorioMap") : FName(*PreviousMapName);
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: Returning from Relaxation to %s"), *TargetMap.ToString());
	UGameplayStatics::OpenLevel(this, TargetMap);
}
