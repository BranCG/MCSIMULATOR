#include "SpeechAnalyzer.h"
#include "MCSIMULATORGameInstance.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Base64.h"
#include "Async/Async.h"
#include "Kismet/GameplayStatics.h"

USpeechAnalyzer::USpeechAnalyzer()
{
	PrimaryComponentTick.bCanEverTick = false;
	
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

	FString LocalApiEndpoint = ApiEndpointUrl;
	FString LocalApiKey = ApiKey;
	FString LocalContext = EvaluationContext;

	// If context is general/default, retrieve active persistent scenario context from GameInstance
	if (LocalContext.Equals(TEXT("General"), ESearchCase::IgnoreCase) || LocalContext.IsEmpty())
	{
		if (UWorld* World = GetWorld())
		{
			if (UMCSIMULATORGameInstance* GI = Cast<UMCSIMULATORGameInstance>(UGameplayStatics::GetGameInstance(World)))
			{
				LocalContext = GI->GetActiveAIContext();
			}
		}
	}

	// Execute heavy Base64 conversion and JSON stringification in a background thread to prevent VR GameThread hitching
	Async(EAsyncExecution::Thread, [this, RawPCMData, LocalApiEndpoint, LocalApiKey, LocalContext]()
	{
		FString AudioBase64 = FBase64::Encode(RawPCMData);

		TSharedPtr<FJsonObject> JsonRequest = MakeShareable(new FJsonObject());
		JsonRequest->SetStringField(TEXT("audio_base64"), AudioBase64);
		JsonRequest->SetStringField(TEXT("context"), LocalContext);
		JsonRequest->SetNumberField(TEXT("sample_rate"), 16000.0);
		JsonRequest->SetNumberField(TEXT("channels"), 1.0);

		FString RequestBody;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
		if (!FJsonSerializer::Serialize(JsonRequest.ToSharedRef(), Writer))
		{
			AsyncTask(ENamedThreads::GameThread, [this]()
			{
				OnAnalysisFailed.Broadcast(TEXT("Failed to serialize JSON request payload."));
			});
			return;
		}

		// Dispatch the HTTP request back on the main GameThread
		AsyncTask(ENamedThreads::GameThread, [this, RequestBody, LocalApiEndpoint, LocalApiKey]()
		{
			TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
			Request->OnProcessRequestComplete().BindUObject(this, &USpeechAnalyzer::OnAnalysisResponseReceived);
			Request->SetURL(LocalApiEndpoint);
			Request->SetVerb(TEXT("POST"));
			Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
			
			if (!LocalApiKey.IsEmpty())
			{
				Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *LocalApiKey));
			}

			Request->SetContentAsString(RequestBody);
			
			UE_LOG(LogTemp, Log, TEXT("MC Simulator: Sending HTTP request to %s (Length: %d bytes)."), *LocalApiEndpoint, RequestBody.Len());
			
			if (!Request->ProcessRequest())
			{
				OnAnalysisFailed.Broadcast(TEXT("Failed to initiate HTTP network connection."));
			}
		});
	});
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

		double TempFillerWordsCount = 0.0;
		JsonObject->TryGetNumberField(TEXT("filler_words_count"), TempFillerWordsCount);
		Result.FillerWordsCount = static_cast<int32>(TempFillerWordsCount);

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
