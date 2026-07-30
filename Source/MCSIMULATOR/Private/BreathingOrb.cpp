#include "BreathingOrb.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ABreathingOrb::ABreathingOrb()
{
	PrimaryActorTick.bCanEverTick = true;

	OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
	RootComponent = OrbMesh;

	OrbLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("OrbLight"));
	OrbLight->SetupAttachment(OrbMesh);
	OrbLight->SetIntensity(3000.f);
	OrbLight->SetLightColor(FLinearColor(0.1f, 0.8f, 1.0f)); // Cyan aura
	OrbLight->SetAttenuationRadius(800.f);
}

void ABreathingOrb::BeginPlay()
{
	Super::BeginPlay();
	OrbMesh->SetWorldScale3D(FVector(MinScale));
}

void ABreathingOrb::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bSessionActive)
	{
		return;
	}

	StateTimer += DeltaTime;

	// Scale & Light lerp based on current state
	float Alpha = (TargetDuration > 0.f) ? FMath::Clamp(StateTimer / TargetDuration, 0.f, 1.f) : 1.f;

	switch (CurrentState)
	{
	case EBreathingState::Inhale:
		CurrentScale = FMath::Lerp(MinScale, MaxScale, Alpha);
		OrbLight->SetLightColor(FLinearColor::LerpUsingHSV(FLinearColor(0.1f, 0.8f, 1.0f), FLinearColor(0.7f, 0.2f, 1.0f), Alpha));
		break;

	case EBreathingState::Hold:
	{
		// Gentle pulse while holding breath
		float Pulse = FMath::Sin(StateTimer * 3.f) * 0.05f;
		CurrentScale = MaxScale + Pulse;
		break;
	}

	case EBreathingState::Exhale:
		CurrentScale = FMath::Lerp(MaxScale, MinScale, Alpha);
		OrbLight->SetLightColor(FLinearColor::LerpUsingHSV(FLinearColor(0.7f, 0.2f, 1.0f), FLinearColor(0.1f, 0.8f, 1.0f), Alpha));
		break;

	default:
		break;
	}

	OrbMesh->SetWorldScale3D(FVector(CurrentScale));

	// Check if state duration finished
	if (StateTimer >= TargetDuration)
	{
		AdvanceState();
	}
}

void ABreathingOrb::StartBreathingSession()
{
	if (bSessionActive)
	{
		return;
	}

	bSessionActive = true;
	CurrentRound = 1;
	// Intro duration: ~10 seconds
	SetBreathingState(EBreathingState::Intro, 10.5f);
}

void ABreathingOrb::SetBreathingState(EBreathingState NewState, float Duration)
{
	CurrentState = NewState;
	StateTimer = 0.f;
	TargetDuration = Duration;

	PlaySoundForState(NewState);
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: Breathing Orb state changed to %d (Round %d)"), (int32)NewState, CurrentRound);
}

void ABreathingOrb::AdvanceState()
{
	switch (CurrentState)
	{
	case EBreathingState::Intro:
		SetBreathingState(EBreathingState::Inhale, 4.2f); // 4 seconds inhale
		break;

	case EBreathingState::Inhale:
		SetBreathingState(EBreathingState::Hold, 7.2f); // 7 seconds hold
		break;

	case EBreathingState::Hold:
		SetBreathingState(EBreathingState::Exhale, 8.2f); // 8 seconds exhale
		break;

	case EBreathingState::Exhale:
		if (CurrentRound == 1)
		{
			CurrentRound = 2;
			SetBreathingState(EBreathingState::Transition, 5.5f); // Transition to Round 2
		}
		else
		{
			SetBreathingState(EBreathingState::Outro, 8.5f); // Outro message
		}
		break;

	case EBreathingState::Transition:
		SetBreathingState(EBreathingState::Inhale, 4.2f); // Round 2 Inhale
		break;

	case EBreathingState::Outro:
		CurrentState = EBreathingState::Completed;
		bSessionActive = false;
		OnSessionCompleted.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("MC Simulator: 2-round breathing session completed."));
		break;

	default:
		break;
	}
}

void ABreathingOrb::PlaySoundForState(EBreathingState State)
{
	USoundBase* TargetSound = nullptr;

	switch (State)
	{
	case EBreathingState::Intro:
		TargetSound = SoundIntro;
		break;
	case EBreathingState::Inhale:
		TargetSound = SoundInhale;
		break;
	case EBreathingState::Hold:
		TargetSound = SoundHold;
		break;
	case EBreathingState::Exhale:
		TargetSound = SoundExhale;
		break;
	case EBreathingState::Transition:
		TargetSound = SoundTransition;
		break;
	case EBreathingState::Outro:
		TargetSound = SoundOutro;
		break;
	default:
		break;
	}

	if (TargetSound)
	{
		UGameplayStatics::PlaySound2D(this, TargetSound);
	}
}
