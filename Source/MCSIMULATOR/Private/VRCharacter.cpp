#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "SpeechRecorderComponent.h"

#if PLATFORM_ANDROID
#include "AndroidPermissionFunctionLibrary.h"
#include "AndroidPermissionCallbackProxy.h"
#endif

AVRCharacter::AVRCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// VR Pawn setup
	GetCapsuleComponent()->InitCapsuleSize(30.0f, 96.0f);
	
	// Create VR floor offset root
	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetCapsuleComponent());
	// Offset VRRoot to the bottom of the capsule (floor)
	VRRoot->SetRelativeLocation(FVector(0.f, 0.f, -96.0f));

	// Camera
	VRCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VRCamera"));
	VRCamera->SetupAttachment(VRRoot);
	VRCamera->bUsePawnControlRotation = false;

	// Controllers
	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VRRoot);
	LeftController->MotionSource = FName("Left");

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VRRoot);
	RightController->MotionSource = FName("Right");

	// Hand visuals
	LeftHandVisual = CreateDefaultSubobject<USceneComponent>(TEXT("LeftHandVisual"));
	LeftHandVisual->SetupAttachment(LeftController);

	RightHandVisual = CreateDefaultSubobject<USceneComponent>(TEXT("RightHandVisual"));
	RightHandVisual->SetupAttachment(RightController);

	// Mic Recorder Component
	SpeechRecorder = CreateDefaultSubobject<USpeechRecorderComponent>(TEXT("SpeechRecorder"));
}

void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Add input mapping context
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (VRMappingContext)
			{
				Subsystem->AddMappingContext(VRMappingContext, 0);
				UE_LOG(LogTemp, Log, TEXT("MC Simulator: Successfully applied Enhanced Input Mapping Context."));
			}
		}
	}

	// Ask for Microphone access (essential for Quest build)
	RequestMicrophonePermission();
}

void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (TriggerAction)
		{
			EnhancedInputComponent->BindAction(TriggerAction, ETriggerEvent::Triggered, this, &AVRCharacter::OnTriggerTriggered);
		}
		if (GrabAction)
		{
			EnhancedInputComponent->BindAction(GrabAction, ETriggerEvent::Started, this, &AVRCharacter::OnGrabTriggered);
			EnhancedInputComponent->BindAction(GrabAction, ETriggerEvent::Completed, this, &AVRCharacter::OnGrabReleased);
		}
	}
}

void AVRCharacter::RequestMicrophonePermission()
{
#if PLATFORM_ANDROID
	TArray<FString> Permissions;
	Permissions.Add(TEXT("android.permission.RECORD_AUDIO"));
	
	UAndroidPermissionCallbackProxy* CallbackProxy = UAndroidPermissionFunctionLibrary::AcquirePermissions(Permissions);
	if (CallbackProxy)
	{
		CallbackProxy->OnPermissionsGrantedDelegate.AddDynamic(this, &AVRCharacter::OnPermissionRequestCompleted);
		UE_LOG(LogTemp, Log, TEXT("MC Simulator: Requested Android microphone permissions at runtime."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MC Simulator: Failed to create Android permission callback proxy."));
	}
#else
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: Skip runtime microphone permissions (not on Quest/Android)."));
#endif
}

void AVRCharacter::OnPermissionRequestCompleted(const TArray<FString>& Permissions, const TArray<bool>& GrantResults)
{
	for (int32 i = 0; i < Permissions.Num(); ++i)
	{
		if (Permissions[i] == TEXT("android.permission.RECORD_AUDIO"))
		{
			if (GrantResults.IsValidIndex(i) && GrantResults[i])
			{
				UE_LOG(LogTemp, Log, TEXT("MC Simulator: Microphone permission GRANTED. Ready to record voice."));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("MC Simulator: Microphone permission DENIED. Public speaking recording won't function."));
			}
		}
	}
}

void AVRCharacter::OnTriggerTriggered()
{
	// Trigger custom interaction or start speaking
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: VR Trigger activated."));
}

void AVRCharacter::OnGrabTriggered()
{
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: VR Grab started."));
}

void AVRCharacter::OnGrabReleased()
{
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: VR Grab released."));
}
