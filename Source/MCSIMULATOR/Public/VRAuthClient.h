// Copyright BranCG 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "VRAuthClient.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVRAuthSuccess, const FString&, UserNickname, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVRAuthFailed, const FString&, ErrorMessage);

/**
 * Componente de red C++ para autenticación VR en Unreal Engine mediante Token de 4 dígitos
 * Conecta con el backend cloud en https://mcsimulator.fimchile.cl/api/auth/verify-token
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MCSIMULATOR_API UVRAuthClient : public UActorComponent
{
	GENERATED_BODY()

public:	
	UVRAuthClient();

	// URL Endpoint de verificación de Token VR
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MC Simulator|Network")
	FString VerifyTokenEndpointUrl;

	// Evento de éxito al verificar Token
	UPROPERTY(BlueprintAssignable, Category = "MC Simulator|Events")
	FOnVRAuthSuccess OnAuthSuccess;

	// Evento de falla en autenticación
	UPROPERTY(BlueprintAssignable, Category = "MC Simulator|Events")
	FOnVRAuthFailed OnAuthFailed;

	// Función invocable desde Blueprints/UMG para enviar el Nickname y Token de 4 dígitos
	UFUNCTION(BlueprintCallable, Category = "MC Simulator|Network")
	void RequestTokenVerification(const FString& Nickname, const FString& Token4Digits);

private:
	// Handler de respuesta HTTP del servidor cloud
	void OnTokenVerifyResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};
