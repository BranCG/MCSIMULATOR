#include "MCSIMULATORGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

UMCSIMULATORGameInstance::UMCSIMULATORGameInstance()
{
	// Default to Auditorium scenario
	SelectedScenarioId = TEXT("Auditorio");
	SelectedScenarioName = TEXT("Auditorio Principal");
	SelectedAIContext = TEXT("Presentación oral masiva frente a jurados y audiencia en un auditorio principal.");
	PreviousMapName = TEXT("AuditorioMap");
	bHasCachedAnalysis = false;
}

void UMCSIMULATORGameInstance::SaveAnalysisResult(const FSpeechAnalysisResult& Result)
{
	CachedAnalysisResult = Result;
	bHasCachedAnalysis = true;
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: Speech Analysis Result saved to GameInstance memory. Score: %.1f"), Result.OverallScore);
}

bool UMCSIMULATORGameInstance::GetCachedAnalysisResult(FSpeechAnalysisResult& OutResult)
{
	if (bHasCachedAnalysis)
	{
		OutResult = CachedAnalysisResult;
		return true;
	}
	return false;
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
		UE_LOG(LogTemp, Log, TEXT("MC Simulator: Opening Relaxation scene from %s with smooth fade"), *PreviousMapName);

		// Smooth camera fade out to black
		if (APlayerCameraManager* CamManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			CamManager->StartCameraFade(0.f, 1.f, 1.2f, FLinearColor::Black, false, true);
		}

		// Open level after short fade delay
		FTimerHandle FadeTimerHandle;
		World->GetTimerManager().SetTimer(FadeTimerHandle, [this]()
		{
			UGameplayStatics::OpenLevel(this, FName("RelaxMap"));
		}, 1.0f, false);
	}
}

void UMCSIMULATORGameInstance::ReturnFromRelaxation()
{
	UWorld* World = GetWorld();
	FName TargetMap = PreviousMapName.IsEmpty() ? FName("AuditorioMap") : FName(*PreviousMapName);
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: Returning from Relaxation to %s with smooth fade"), *TargetMap.ToString());

	if (World)
	{
		if (APlayerCameraManager* CamManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
		{
			CamManager->StartCameraFade(0.f, 1.f, 1.2f, FLinearColor::Black, false, true);
		}

		FTimerHandle FadeTimerHandle;
		World->GetTimerManager().SetTimer(FadeTimerHandle, [this, TargetMap]()
		{
			UGameplayStatics::OpenLevel(this, TargetMap);
		}, 1.0f, false);
	}
	else
	{
		UGameplayStatics::OpenLevel(this, TargetMap);
	}
}
