#include "VRFeedbackActor.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"

AVRFeedbackActor::AVRFeedbackActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root mesh for the physical casing of the display screen
	ScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenMesh"));
	RootComponent = ScreenMesh;

	// 3D world space Widget component
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(RootComponent);
	
	// Optimization and clarity settings for virtual reality readability
	WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	WidgetComponent->SetDrawSize(FVector2D(1920.f, 1080.f));         // Standard 16:9 HD canvas
	WidgetComponent->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.15f)); // Scaled to reasonable size
	WidgetComponent->SetDrawAtDesiredSize(false);
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	// Translucent blend mode is recommended for anti-aliasing text in VR
	WidgetComponent->SetGeometryMode(EWidgetGeometryMode::Plane);
}

void AVRFeedbackActor::BeginPlay()
{
	Super::BeginPlay();
}

void AVRFeedbackActor::DisplayAnalysisResults(const FSpeechAnalysisResult& Results)
{
	// Broadcast event for UI designers to run animations, play audio, and populate text boxes in Blueprints
	OnAnalysisResultsReceived(Results);
	
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: Displaying results on 3D VR Screen. Overall Score: %0.1f | Coherence: %0.1f | Filler words: %d"), 
		Results.OverallScore, Results.CoherenceScore, Results.FillerWordsCount);
}

void AVRFeedbackActor::ResetScreen()
{
	// Broadcast event to return the 3D widget to its starting idle or prompt state
	OnScreenReset();
	
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: VR Screen reset to standby mode."));
}
