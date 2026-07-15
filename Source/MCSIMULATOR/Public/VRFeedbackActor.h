#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpeechAnalyzer.h"
#include "VRFeedbackActor.generated.h"

class UWidgetComponent;
class UStaticMeshComponent;

/**
 * 3D Interactive Screen Actor placed in the virtual auditorium or meeting room.
 * Displays evaluation scores, speech metrics, transcription, and feedback.
 */
UCLASS()
class MCSIMULATOR_API AVRFeedbackActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AVRFeedbackActor();

protected:
	virtual void BeginPlay() override;

public:	
	// Mesh representing physical TV, monitor, or blackboard frame
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR Screen|Components")
	UStaticMeshComponent* ScreenMesh;

	// 3D UI component to render user interface widgets in the virtual world
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR Screen|Components")
	UWidgetComponent* WidgetComponent;

	// Update the widget display with new speech analysis results
	UFUNCTION(BlueprintCallable, Category = "VR Screen|Actions")
	void DisplayAnalysisResults(const FSpeechAnalysisResult& Results);

	// Event for Blueprint designers to handle visual transitions, sound effects, or custom widget behaviors
	UFUNCTION(BlueprintImplementableEvent, Category = "VR Screen|Events")
	void OnAnalysisResultsReceived(const FSpeechAnalysisResult& Results);

	// Reset screen back to standby or recording prompt state
	UFUNCTION(BlueprintCallable, Category = "VR Screen|Actions")
	void ResetScreen();

	// Event for Blueprint designers to handle screen resetting
	UFUNCTION(BlueprintImplementableEvent, Category = "VR Screen|Events")
	void OnScreenReset();
};
