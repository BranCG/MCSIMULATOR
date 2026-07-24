#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

class UCameraComponent;
class UMotionControllerComponent;
class USpeechRecorderComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class MCSIMULATOR_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AVRCharacter();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// VR Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UCameraComponent* VRCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UMotionControllerComponent* LeftController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	UMotionControllerComponent* RightController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	USceneComponent* LeftHandVisual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	USceneComponent* RightHandVisual;

	// Speech System
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Speech")
	USpeechRecorderComponent* SpeechRecorder;

	// Enhanced Input Configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Input")
	UInputMappingContext* VRMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Input")
	UInputAction* TriggerAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Input")
	UInputAction* GrabAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR|Input")
	UInputAction* MoveAction;

public:
	UFUNCTION(BlueprintCallable, Category = "VR|Movement")
	void Move(const FVector2D& Value);
	UFUNCTION(BlueprintCallable, Category = "VR|Speech")
	void StartRecordingSpeech();

	UFUNCTION(BlueprintCallable, Category = "VR|Speech")
	void StopRecordingSpeech();

protected:
	// Handle runtime permissions for Oculus Quest microphone
	void RequestMicrophonePermission();

	UFUNCTION()
	void OnPermissionRequestCompleted(const TArray<FString>& Permissions, const TArray<bool>& GrantResults);

	// Action bindings
	void MoveInputAction(const struct FInputActionValue& Value);
	void OnTriggerStarted();
	void OnTriggerReleased();
	void OnGrabTriggered();
	void OnGrabReleased();
};
