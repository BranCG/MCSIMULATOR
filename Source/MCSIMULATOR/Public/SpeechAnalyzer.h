#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "SpeechAnalyzer.generated.h"

USTRUCT(BlueprintType)
struct FSpeechAnalysisResult
{
	GENERATED_BODY()

	// The transcribed text from the voice recording
	UPROPERTY(BlueprintReadOnly, Category = "Speech Simulator|Analysis")
	FString Transcript;

	// Overall score from 0.f to 100.f
	UPROPERTY(BlueprintReadOnly, Category = "Speech Simulator|Analysis")
	float OverallScore = 0.f;

	// Semantic coherence score from 0.f to 100.f
	UPROPERTY(BlueprintReadOnly, Category = "Speech Simulator|Analysis")
	float CoherenceScore = 0.f;

	// Number of filler words detected (e.g., "eh", "este", "bueno")
	UPROPERTY(BlueprintReadOnly, Category = "Speech Simulator|Analysis")
	int32 FillerWordsCount = 0;

	// Feedback regarding speed, filler words, or nervous ticks
	UPROPERTY(BlueprintReadOnly, Category = "Speech Simulator|Analysis")
	FString NervousnessFeedback;

	// Detailed analysis of the content coherence and technical terminology
	UPROPERTY(BlueprintReadOnly, Category = "Speech Simulator|Analysis")
	FString SemanticFeedback;

	// Actionable recommendations for the presenter
	UPROPERTY(BlueprintReadOnly, Category = "Speech Simulator|Analysis")
	FString Recommendations;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpeechAnalysisCompleted, const FSpeechAnalysisResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpeechAnalysisFailed, const FString&, ErrorMessage);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MCSIMULATOR_API USpeechAnalyzer : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpeechAnalyzer();

protected:
	virtual void BeginPlay() override;

public:	
	// Server Endpoint URL for Speech Analysis API
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speech Simulator|API Settings")
	FString ApiEndpointUrl;

	// Optional Authorization Key
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speech Simulator|API Settings")
	FString ApiKey;

	// Mode of evaluation: "Tesis", "Entrevista", "Negocios", "General"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speech Simulator|API Settings")
	FString EvaluationContext;

	// Sends raw 16-bit Mono PCM audio data as Base64 JSON payload to the backend
	UFUNCTION(BlueprintCallable, Category = "Speech Simulator|Analyzer")
	void RequestSpeechAnalysis(const TArray<uint8>& RawPCMData);

	// Delegates for Blueprint events
	UPROPERTY(BlueprintAssignable, Category = "Speech Simulator|Analyzer")
	FOnSpeechAnalysisCompleted OnAnalysisCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Speech Simulator|Analyzer")
	FOnSpeechAnalysisFailed OnAnalysisFailed;

private:
	// HTTP response handler callback
	void OnAnalysisResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
