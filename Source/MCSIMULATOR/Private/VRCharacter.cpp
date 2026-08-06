#include "VRCharacter.h"
#include "BreathingOrb.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "MCSIMULATORGameInstance.h"
#include "MotionControllerComponent.h"
#include "SpeechRecorderComponent.h"
#include "VRFeedbackActor.h"

#if PLATFORM_ANDROID
#include "AndroidPermissionCallbackProxy.h"
#include "AndroidPermissionFunctionLibrary.h"

#endif

AVRCharacter::AVRCharacter() {
  PrimaryActorTick.bCanEverTick = true;

  static ConstructorHelpers::FClassFinder<ABreathingOrb> OrbBPFinder(TEXT("/Game/Blueprints/BP_BreathingOrb.BP_BreathingOrb_C"));
  if (OrbBPFinder.Succeeded())
  {
      BreathingOrbClass = OrbBPFinder.Class;
  }

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
  LeftController = CreateDefaultSubobject<UMotionControllerComponent>(
      TEXT("LeftController"));
  LeftController->SetupAttachment(VRRoot);
  LeftController->MotionSource = FName("Left");

  RightController = CreateDefaultSubobject<UMotionControllerComponent>(
      TEXT("RightController"));
  RightController->SetupAttachment(VRRoot);
  RightController->MotionSource = FName("Right");

  // Hand visuals
  LeftHandVisual =
      CreateDefaultSubobject<USceneComponent>(TEXT("LeftHandVisual"));
  LeftHandVisual->SetupAttachment(LeftController);

  RightHandVisual =
      CreateDefaultSubobject<USceneComponent>(TEXT("RightHandVisual"));
  RightHandVisual->SetupAttachment(RightController);

  // Mic Recorder Component
  SpeechRecorder =
      CreateDefaultSubobject<USpeechRecorderComponent>(TEXT("SpeechRecorder"));
}

void AVRCharacter::BeginPlay() {
  Super::BeginPlay();

  // Add input mapping context
  if (APlayerController *PC = Cast<APlayerController>(GetController())) {
    if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
                PC->GetLocalPlayer())) {
      if (VRMappingContext) {
        Subsystem->AddMappingContext(VRMappingContext, 0);
        UE_LOG(LogTemp, Log,
               TEXT("MC Simulator: Successfully applied Enhanced Input Mapping "
                    "Context."));
      }
    }
  }

  // Ask for Microphone access (essential for Quest build)
  RequestMicrophonePermission();

  // Auto-lock locomotion in LobbyMap if user is not authenticated yet
  if (UWorld* World = GetWorld()) {
    FString MapName = World->GetMapName();
    MapName.RemoveFromStart(World->StreamingLevelsPrefix);

    if (MapName.Contains(TEXT("Lobby"))) {
      if (UMCSIMULATORGameInstance* GI = Cast<UMCSIMULATORGameInstance>(UGameplayStatics::GetGameInstance(World))) {
        if (!GI->bIsAuthenticated) {
          SetLocomotionEnabled(false);
          UE_LOG(LogTemp, Log, TEXT("MC Simulator VR: Movement automatically locked in LobbyMap until authentication."));
        }
      }
    }
  }
}

void AVRCharacter::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

void AVRCharacter::SetupPlayerInputComponent(
    UInputComponent *PlayerInputComponent) {
  Super::SetupPlayerInputComponent(PlayerInputComponent);

  if (UEnhancedInputComponent *EnhancedInputComponent =
          Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
    if (TriggerAction) {
      EnhancedInputComponent->BindAction(TriggerAction, ETriggerEvent::Started,
                                         this, &AVRCharacter::OnTriggerStarted);
      EnhancedInputComponent->BindAction(TriggerAction,
                                         ETriggerEvent::Completed, this,
                                         &AVRCharacter::OnTriggerReleased);
    }
    if (GrabAction) {
      EnhancedInputComponent->BindAction(GrabAction, ETriggerEvent::Started,
                                         this, &AVRCharacter::OnGrabTriggered);
      EnhancedInputComponent->BindAction(GrabAction, ETriggerEvent::Completed,
                                         this, &AVRCharacter::OnGrabReleased);
    }
    if (MoveAction) {
      EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered,
                                         this, &AVRCharacter::MoveInputAction);
    }
  }
}

void AVRCharacter::SetLocomotionEnabled(bool bEnable) {
  bLocomotionEnabled = bEnable;
  if (UCharacterMovementComponent* MovementComp = GetCharacterMovement()) {
    if (bEnable) {
      MovementComp->SetMovementMode(MOVE_Walking);
    } else {
      MovementComp->DisableMovement();
    }
  }
  UE_LOG(LogTemp, Log, TEXT("MC Simulator VR Locomotion status: %s"), bEnable ? TEXT("ENABLED") : TEXT("DISABLED (Locked)"));
}

void AVRCharacter::Move(const FVector2D &Value) {
  if (!bLocomotionEnabled) return;

  if (Value.SizeSquared() > 0.001f && VRCamera) {
    const FRotator YawRotation(0.f, VRCamera->GetComponentRotation().Yaw, 0.f);
    const FVector ForwardDirection =
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection =
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(ForwardDirection, Value.Y);
    AddMovementInput(RightDirection, Value.X);
  }
}

void AVRCharacter::MoveInputAction(const FInputActionValue &Value) {
  FVector2D MovementVector = Value.Get<FVector2D>();
  Move(MovementVector);
}

void AVRCharacter::RequestMicrophonePermission() {
#if PLATFORM_ANDROID
  TArray<FString> Permissions;
  Permissions.Add(TEXT("android.permission.RECORD_AUDIO"));

  UAndroidPermissionCallbackProxy *CallbackProxy =
      UAndroidPermissionFunctionLibrary::AcquirePermissions(Permissions);
  if (CallbackProxy) {
    CallbackProxy->OnPermissionsGrantedDelegate.AddDynamic(
        this, &AVRCharacter::OnPermissionRequestCompleted);
    UE_LOG(LogTemp, Log,
           TEXT("MC Simulator: Requested Android microphone permissions at "
                "runtime."));
  } else {
    UE_LOG(LogTemp, Warning,
           TEXT("MC Simulator: Failed to create Android permission callback "
                "proxy."));
  }
#else
  UE_LOG(LogTemp, Log,
         TEXT("MC Simulator: Skip runtime microphone permissions (not on "
              "Quest/Android)."));
#endif
}

void AVRCharacter::OnPermissionRequestCompleted(
    const TArray<FString> &Permissions, const TArray<bool> &GrantResults) {
  for (int32 i = 0; i < Permissions.Num(); ++i) {
    if (Permissions[i] == TEXT("android.permission.RECORD_AUDIO")) {
      if (GrantResults.IsValidIndex(i) && GrantResults[i]) {
        UE_LOG(LogTemp, Log,
               TEXT("MC Simulator: Microphone permission GRANTED. Ready to "
                    "record voice."));
      } else {
        UE_LOG(LogTemp, Warning,
               TEXT("MC Simulator: Microphone permission DENIED. Public "
                    "speaking recording won't function."));
      }
    }
  }
}

void AVRCharacter::StartRecordingSpeech() {
  UE_LOG(LogTemp, Log, TEXT("MC Simulator: StartRecordingSpeech invoked."));
  if (SpeechRecorder) {
    SpeechRecorder->StartRecording();
  }
}

void AVRCharacter::StopRecordingSpeech() {
  UE_LOG(LogTemp, Log, TEXT("MC Simulator: StopRecordingSpeech invoked."));
  if (SpeechRecorder) {
    SpeechRecorder->StopRecording();
  }
}

void AVRCharacter::OnTriggerStarted() {
  // Trigger action logic (e.g. pointer click)
}

void AVRCharacter::OnTriggerReleased() {
  // Release action logic
}

void AVRCharacter::OnGrabTriggered() {
  UE_LOG(LogTemp, Log, TEXT("MC Simulator: VR Grab started."));
}

void AVRCharacter::OnGrabReleased() {
  UE_LOG(LogTemp, Log, TEXT("MC Simulator: VR Grab released."));
}

void AVRCharacter::StartInLevelBreathingSession() {
  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  UE_LOG(LogTemp, Log,
         TEXT("MC Simulator: Initiating in-level breathing relaxation session "
              "without map reloading."));

  // Smooth camera fade to dark
  if (APlayerCameraManager *CamManager =
          UGameplayStatics::GetPlayerCameraManager(this, 0)) {
    CamManager->StartCameraFade(0.f, 0.7f, 0.8f, FLinearColor::Black, false,
                                true);
  }

  // Calculate 1.5m position right in front of player VR Gaze
  FVector SpawnLoc =
      VRCamera->GetComponentLocation() + VRCamera->GetForwardVector() * 150.f;
  FRotator SpawnRot = VRCamera->GetComponentRotation();

  UClass* TargetClass = BreathingOrbClass ? *BreathingOrbClass : nullptr;
  if (!TargetClass)
  {
      TargetClass = StaticLoadClass(ABreathingOrb::StaticClass(), nullptr, TEXT("/Game/Blueprints/BP_BreathingOrb.BP_BreathingOrb_C"));
  }
  if (!TargetClass)
  {
      TargetClass = ABreathingOrb::StaticClass();
  }

  // Find or Spawn ABreathingOrb using configured Blueprint class (BP_BreathingOrb)
  ABreathingOrb *Orb = Cast<ABreathingOrb>(
      UGameplayStatics::GetActorOfClass(World, TargetClass));
  if (!Orb) {
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Orb = World->SpawnActor<ABreathingOrb>(TargetClass,
                                           SpawnLoc, SpawnRot, SpawnParams);
  } else {
    Orb->SetActorLocationAndRotation(SpawnLoc, SpawnRot);
    Orb->SetActorHiddenInGame(false);
  }

  if (Orb) {
    // Listen for session completion to bring back feedback screen
    Orb->OnSessionCompleted.RemoveAll(this);
    Orb->OnSessionCompleted.AddDynamic(
        this, &AVRCharacter::OnInLevelBreathingCompleted);

    Orb->StartBreathingSession();
  }
}

void AVRCharacter::OnInLevelBreathingCompleted() {
  UWorld *World = GetWorld();
  if (!World) {
    return;
  }

  UE_LOG(LogTemp, Log,
         TEXT("MC Simulator: In-level breathing session completed. Revealing "
              "3D Feedback screen."));

  // Restore camera fade
  if (APlayerCameraManager *CamManager =
          UGameplayStatics::GetPlayerCameraManager(this, 0)) {
    CamManager->StartCameraFade(0.7f, 0.f, 1.0f, FLinearColor::Black, false,
                                true);
  }

  // Hide Breathing Orb
  if (ABreathingOrb *Orb =
          Cast<ABreathingOrb>(UGameplayStatics::GetActorOfClass(
              World, ABreathingOrb::StaticClass()))) {
    Orb->SetActorHiddenInGame(true);
  }

  // Check if cached feedback results exist in GameInstance and display on all VRFeedbackActor screens
  if (UMCSIMULATORGameInstance *GI =
          Cast<UMCSIMULATORGameInstance>(GetGameInstance())) {
    FSpeechAnalysisResult CachedResult;
    if (GI->GetCachedAnalysisResult(CachedResult)) {
      TArray<AActor*> FeedbackActors;
      UGameplayStatics::GetAllActorsOfClass(World, AVRFeedbackActor::StaticClass(), FeedbackActors);
      for (AActor* Actor : FeedbackActors) {
        if (AVRFeedbackActor* FeedbackActor = Cast<AVRFeedbackActor>(Actor)) {
          FeedbackActor->DisplayAnalysisResults(CachedResult);
          UE_LOG(LogTemp, Log,
                 TEXT("MC Simulator: Successfully projected cached speech "
                      "feedback on 3D VR Screen."));
        }
      }
    }
  }
}
