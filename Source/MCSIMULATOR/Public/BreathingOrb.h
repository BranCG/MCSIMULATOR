#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BreathingOrb.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class USoundBase;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBreathingSessionCompleted);

UENUM(BlueprintType)
enum class EBreathingState : uint8
{
	Idle,
	Intro,
	Inhale,
	Hold,
	Exhale,
	Transition,
	Outro,
	Completed
};

/**
 * 3D Breathing Orb Actor for VR Anti-Anxiety Relaxation Session (4-7-8 Technique).
 * Manages smooth scaling, light pulsing, audio synchronization, and 2-round cycles.
 */
UCLASS()
class MCSIMULATOR_API ABreathingOrb : public AActor
{
	GENERATED_BODY()
	
public:	
	ABreathingOrb();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// Visual components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breathing Orb|Components")
	UStaticMeshComponent* OrbMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breathing Orb|Components")
	UPointLightComponent* OrbLight;

	// Audio assets for the 2-round guided meditation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breathing Orb|Audio")
	USoundBase* SoundIntro;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breathing Orb|Audio")
	USoundBase* SoundInhale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breathing Orb|Audio")
	USoundBase* SoundHold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breathing Orb|Audio")
	USoundBase* SoundExhale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breathing Orb|Audio")
	USoundBase* SoundTransition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breathing Orb|Audio")
	USoundBase* SoundOutro;

	// Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breathing Orb|Settings")
	float MinScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breathing Orb|Settings")
	float MaxScale = 1.6f;

	UPROPERTY(BlueprintReadOnly, Category = "Breathing Orb|State")
	EBreathingState CurrentState = EBreathingState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Breathing Orb|State")
	int32 CurrentRound = 1;

	UPROPERTY(BlueprintAssignable, Category = "Breathing Orb|Events")
	FOnBreathingSessionCompleted OnSessionCompleted;

	// Actions
	UFUNCTION(BlueprintCallable, Category = "Breathing Orb|Actions")
	void StartBreathingSession();

private:
	UPROPERTY()
	UAudioComponent* ActiveAudioComponent = nullptr;

	float StateTimer = 0.f;
	float TargetDuration = 0.f;
	float CurrentScale = 0.5f;
	bool bSessionActive = false;

	void AdvanceState();
	void SetBreathingState(EBreathingState NewState, float Duration);
	void PlaySoundForState(EBreathingState State);
};
