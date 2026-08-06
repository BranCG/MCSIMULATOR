// Copyright BranCG 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SpeechAnalyzer.h"
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

	// Store map name prior to entering relaxation session
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MC Simulator|Scenario")
	FString PreviousMapName;

	// Cached Speech Analysis Result from Gemini AI
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MC Simulator|Analysis")
	FSpeechAnalysisResult CachedAnalysisResult;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MC Simulator|Analysis")
	bool bHasCachedAnalysis = false;

	// User Session Data from VR Web Auth
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MC Simulator|Session")
	FString UserNickname;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MC Simulator|Session")
	bool bIsAuthenticated = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MC Simulator|Session")
	bool bUsePresentationSlides = false;

	// Save analysis result to persistent GameInstance memory
	UFUNCTION(BlueprintCallable, Category = "MC Simulator|Analysis")
	void SaveAnalysisResult(const FSpeechAnalysisResult& Result);

	// Get cached analysis result if available
	UFUNCTION(BlueprintCallable, Category = "MC Simulator|Analysis")
	bool GetCachedAnalysisResult(FSpeechAnalysisResult& OutResult);

	// Helper function to set current scenario state
	UFUNCTION(BlueprintCallable, Category = "MC Simulator|Scenario")
	void SetCurrentScenario(const FString& ScenarioId, const FString& ScenarioName, const FString& AIContext);

	// Get active AI evaluation context string
	UFUNCTION(BlueprintPure, Category = "MC Simulator|Scenario")
	FString GetActiveAIContext() const;

	// Transition to Guided Breathing Relaxation Map
	UFUNCTION(BlueprintCallable, Category = "MC Simulator|Navigation")
	void OpenRelaxationScene();

	// Transition back from Relaxation Map to Scenario or Menu
	UFUNCTION(BlueprintCallable, Category = "MC Simulator|Navigation")
	void ReturnFromRelaxation();
};
