#include "SpeechAnalyzer.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Base64.h"

USpeechAnalyzer::USpeechAnalyzer()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Default configuration
	ApiEndpointUrl = TEXT("http://127.0.0.1:5000/api/analyze");
	EvaluationContext = TEXT("General");
}

void USpeechAnalyzer::BeginPlay()
{
	Super::BeginPlay();
}

void USpeechAnalyzer::RequestSpeechAnalysis(const TArray<uint8>& RawPCMData)
{
	if (RawPCMData.Num() == 0)
	{
		OnAnalysisFailed.Broadcast(TEXT("Speech audio data is empty. Capture failed."));
		return;
	}

	if (ApiEndpointUrl.IsEmpty())
	{
		OnAnalysisFailed.Broadcast(TEXT("API Endpoint URL is not configured."));
		return;
	}

	// 1. Encode raw PCM 16-bit audio bytes to Base64
	FString AudioBase64 = FBase64::Encode(RawPCMData);

	// 2. Prepare JSON request body
	TSharedPtr<FJsonObject> JsonRequest = MakeShareable(new FJsonObject());
	JsonRequest->SetStringField(TEXT("audio_base64"), AudioBase64);
	JsonRequest->SetStringField(TEXT("context"), EvaluationContext);
	JsonRequest->SetNumberField(TEXT("sample_rate"), 16000.0); // 16kHz is standard for speech-to-text
	JsonRequest->SetNumberField(TEXT("channels"), 1.0);        // Mono

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	if (!FJsonSerializer::Serialize(JsonRequest.ToSharedRef(), Writer))
	{
		OnAnalysisFailed.Broadcast(TEXT("Failed to serialize JSON request payload."));
		return;
	}

	// 3. Send Async HTTP Request
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &USpeechAnalyzer::OnAnalysisResponseReceived);
	Request->SetURL(ApiEndpointUrl);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	
	if (!ApiKey.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	}

	Request->SetContentAsString(RequestBody);
	
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: Sending HTTP request to %s (Length: %d bytes)."), *ApiEndpointUrl, RequestBody.Len());
	
	if (!Request->ProcessRequest())
	{
		OnAnalysisFailed.Broadcast(TEXT("Failed to initiate HTTP network connection."));
	}
}

void USpeechAnalyzer::OnAnalysisResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnAnalysisFailed.Broadcast(TEXT("HTTP request failed. Connection refused or timed out."));
		return;
	}

	int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode != 200)
	{
		FString ErrorText = FString::Printf(TEXT("Server returned error status code: %d"), ResponseCode);
		UE_LOG(LogTemp, Warning, TEXT("MC Simulator: %s"), *ErrorText);
		OnAnalysisFailed.Broadcast(ErrorText);
		return;
	}

	FString ResponseBody = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		FSpeechAnalysisResult Result;

		// Extract fields with type safety
		JsonObject->TryGetStringField(TEXT("transcript"), Result.Transcript);

		double TempOverallScore = 0.0;
		JsonObject->TryGetNumberField(TEXT("overall_score"), TempOverallScore);
		Result.OverallScore = static_cast<float>(TempOverallScore);

		double TempCoherenceScore = 0.0;
		JsonObject->TryGetNumberField(TEXT("coherence_score"), TempCoherenceScore);
		Result.CoherenceScore = static_cast<float>(TempCoherenceScore);

		int32 TempFillerWordsCount = 0;
		JsonObject->TryGetIntegerField(TEXT("filler_words_count"), TempFillerWordsCount);
		Result.FillerWordsCount = TempFillerWordsCount;

		JsonObject->TryGetStringField(TEXT("nervousness_feedback"), Result.NervousnessFeedback);
		JsonObject->TryGetStringField(TEXT("semantic_feedback"), Result.SemanticFeedback);

		// Extract Recommendations array
		const TArray<TSharedPtr<FJsonValue>>* RecommendationsArray = nullptr;
		if (JsonObject->TryGetArrayField(TEXT("recommendations"), RecommendationsArray) && RecommendationsArray)
		{
			for (const TSharedPtr<FJsonValue>& Val : *RecommendationsArray)
			{
				if (Val.IsValid())
				{
					Result.Recommendations.Add(Val->AsString());
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("MC Simulator: Semantic speech analysis completed successfully."));
		OnAnalysisCompleted.Broadcast(Result);
	}
	else
	{
		OnAnalysisFailed.Broadcast(TEXT("Failed to parse JSON response content."));
	}
}
