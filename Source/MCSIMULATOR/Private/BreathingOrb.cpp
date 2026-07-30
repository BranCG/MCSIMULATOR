#include "BreathingOrb.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"

ABreathingOrb::ABreathingOrb()
{
	PrimaryActorTick.bCanEverTick = true;

	OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
	RootComponent = OrbMesh;
	OrbMesh->SetCollisionProfileName(TEXT("NoCollision"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		OrbMesh->SetStaticMesh(SphereMeshAsset.Object);
	}

	OrbLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("OrbLight"));
	OrbLight->SetupAttachment(OrbMesh);
	OrbLight->SetIntensity(10000.f);
	OrbLight->SetLightColor(FLinearColor(0.1f, 0.8f, 1.0f)); // Cyan aura
	OrbLight->SetAttenuationRadius(1500.f);
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
	float IntroDuration = (SoundIntro && SoundIntro->GetDuration() > 0.f) ? SoundIntro->GetDuration() + 1.6f : 17.f;
	SetBreathingState(EBreathingState::Intro, IntroDuration);
}

void ABreathingOrb::SetBreathingState(EBreathingState NewState, float Duration)
{
	CurrentState = NewState;
	StateTimer = 0.f;
	TargetDuration = Duration;

	PlaySoundForState(NewState);
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: Breathing Orb state changed to %d (Duration %.2f, Round %d)"), (int32)NewState, Duration, CurrentRound);
}

void ABreathingOrb::AdvanceState()
{
	switch (CurrentState)
	{
	case EBreathingState::Intro:
	{
		float InhaleDuration = (SoundInhale && SoundInhale->GetDuration() > 0.f) ? SoundInhale->GetDuration() + 0.2f : 4.5f;
		SetBreathingState(EBreathingState::Inhale, InhaleDuration);
		break;
	}

	case EBreathingState::Inhale:
	{
		float HoldDuration = (SoundHold && SoundHold->GetDuration() > 0.f) ? SoundHold->GetDuration() + 0.2f : 7.5f;
		SetBreathingState(EBreathingState::Hold, HoldDuration);
		break;
	}

	case EBreathingState::Hold:
	{
		float ExhaleDuration = (SoundExhale && SoundExhale->GetDuration() > 0.f) ? SoundExhale->GetDuration() + 0.2f : 8.5f;
		SetBreathingState(EBreathingState::Exhale, ExhaleDuration);
		break;
	}

	case EBreathingState::Exhale:
		if (CurrentRound == 1)
		{
			CurrentRound = 2;
			float TransDuration = (SoundTransition && SoundTransition->GetDuration() > 0.f) ? SoundTransition->GetDuration() + 0.3f : 5.5f;
			SetBreathingState(EBreathingState::Transition, TransDuration);
		}
		else
		{
			float OutroDuration = (SoundOutro && SoundOutro->GetDuration() > 0.f) ? SoundOutro->GetDuration() + 0.3f : 8.5f;
			SetBreathingState(EBreathingState::Outro, OutroDuration);
		}
		break;

	case EBreathingState::Transition:
	{
		float InhaleDuration = (SoundInhale && SoundInhale->GetDuration() > 0.f) ? SoundInhale->GetDuration() + 0.2f : 4.5f;
		SetBreathingState(EBreathingState::Inhale, InhaleDuration);
		break;
	}

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

	// Stop previous playing audio to prevent any overlapping voices
	if (ActiveAudioComponent)
	{
		ActiveAudioComponent->Stop();
		ActiveAudioComponent = nullptr;
	}

	if (TargetSound)
	{
		if (State == EBreathingState::Intro)
		{
			FTimerHandle IntroDelayHandle;
			GetWorldTimerManager().SetTimer(IntroDelayHandle, [this, TargetSound]()
			{
				if (TargetSound && bSessionActive && CurrentState == EBreathingState::Intro)
				{
					ActiveAudioComponent = UGameplayStatics::SpawnSound2D(this, TargetSound);
				}
			}, 1.2f, false);
		}
		else
		{
			ActiveAudioComponent = UGameplayStatics::SpawnSound2D(this, TargetSound);
		}
	}
}
