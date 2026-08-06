// Copyright BranCG 2026. All Rights Reserved.

#include "VRAuthClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "MCSIMULATORGameInstance.h"
#include "Kismet/GameplayStatics.h"

UVRAuthClient::UVRAuthClient()
{
	PrimaryComponentTick.bCanEverTick = false;
	VerifyTokenEndpointUrl = TEXT("https://mcsimulator.fimchile.cl/api/auth/verify-token");

	FString ConfigUrl;
	if (GConfig && GConfig->GetString(TEXT("MCSIMULATOR.Network"), TEXT("VerifyTokenUrl"), ConfigUrl, GEngineIni))
	{
		if (!ConfigUrl.IsEmpty())
		{
			VerifyTokenEndpointUrl = ConfigUrl;
		}
	}
}

void UVRAuthClient::RequestTokenVerification(const FString& Nickname, const FString& Token4Digits)
{
	if (Nickname.IsEmpty() || Token4Digits.IsEmpty())
	{
		OnAuthFailed.Broadcast(TEXT("Debes ingresar tu Nickname y el Token de 4 dígitos."));
		return;
	}

	FString CleanNick = Nickname.TrimStartAndEnd();
	if (!CleanNick.StartsWith(TEXT("@")))
	{
		CleanNick = TEXT("@") + CleanNick;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(VerifyTokenEndpointUrl);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	JsonObject->SetStringField(TEXT("nickname"), CleanNick);
	JsonObject->SetStringField(TEXT("token"), Token4Digits.TrimStartAndEnd());

	FString OutputJsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputJsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	HttpRequest->SetContentAsString(OutputJsonString);
	HttpRequest->OnProcessRequestComplete().BindUObject(this, &UVRAuthClient::OnTokenVerifyResponseReceived);

	UE_LOG(LogTemp, Log, TEXT("MC Simulator VR Auth: Enviando verificación a %s para %s"), *VerifyTokenEndpointUrl, *CleanNick);
	HttpRequest->ProcessRequest();
}

void UVRAuthClient::OnTokenVerifyResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnAuthFailed.Broadcast(TEXT("Error de conexión con el servidor cloud en https://mcsimulator.fimchile.cl"));
		return;
	}

	int32 ResponseCode = Response->GetResponseCode();
	FString ResponseBody = Response->GetContentAsString();
	UE_LOG(LogTemp, Log, TEXT("MC Simulator VR Auth Respuesta [%d]: %s"), ResponseCode, *ResponseBody);

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		bool bValid = JsonObject->HasTypedField<EJson::Boolean>(TEXT("valid")) ? JsonObject->GetBoolField(TEXT("valid")) : false;
		if (bValid && ResponseCode == 200)
		{
			FString Nickname = JsonObject->HasField(TEXT("nickname")) ? JsonObject->GetStringField(TEXT("nickname")) : TEXT("@usuario");
			FString Message = JsonObject->HasField(TEXT("message")) ? JsonObject->GetStringField(TEXT("message")) : TEXT("Autenticación exitosa");

			// Guardar estado de sesión en GameInstance
			if (UMCSIMULATORGameInstance* GI = Cast<UMCSIMULATORGameInstance>(UGameplayStatics::GetGameInstance(GetWorld())))
			{
				GI->UserNickname = Nickname;
				GI->bIsAuthenticated = true;
			}

			OnAuthSuccess.Broadcast(Nickname, Message);
		}
		else
		{
			FString ErrorMsg = JsonObject->HasField(TEXT("error")) ? JsonObject->GetStringField(TEXT("error")) : TEXT("Token de 4 dígitos incorrecto o expirado.");
			OnAuthFailed.Broadcast(ErrorMsg);
		}
	}
	else
	{
		OnAuthFailed.Broadcast(TEXT("Respuesta no válida del servidor."));
	}
}
