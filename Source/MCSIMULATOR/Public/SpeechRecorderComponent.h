#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpeechRecorderComponent.generated.h"

// Forward declare low-level AudioCapture namespace types
namespace Audio
{
	class FAudioCapture;
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpeechRecordingStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpeechRecordingStopped, const TArray<uint8>&, RawPCMData);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MCSIMULATOR_API USpeechRecorderComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	USpeechRecorderComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Start microphone audio capture
	UFUNCTION(BlueprintCallable, Category = "Speech Simulator|Recorder")
	void StartRecording();

	// Stop microphone audio capture and retrieve buffer
	UFUNCTION(BlueprintCallable, Category = "Speech Simulator|Recorder")
	void StopRecording();

	// Check if active
	UFUNCTION(BlueprintPure, Category = "Speech Simulator|Recorder")
	bool IsRecording() const { return bIsRecording; }

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Speech Simulator|Recorder")
	FOnSpeechRecordingStarted OnRecordingStarted;

	UPROPERTY(BlueprintAssignable, Category = "Speech Simulator|Recorder")
	FOnSpeechRecordingStopped OnRecordingStopped;

private:
	bool bIsRecording;
	bool bStreamInitialized;

	void InitAudioStream();
	
	// Low-level audio capture wrapper
	TSharedPtr<Audio::FAudioCapture> AudioCaptureDevice;

	// Mutex or Critical Section for audio buffer safety (audio threads are asynchronous)
	FCriticalSection BufferMutex;

	// Accumulated PCM 16-bit audio data (PCM 16 is widely supported by Speech APIs)
	TArray<uint8> RecordedAudioBuffer;

	// Internal Callback when new audio buffer is received from hardware
	void OnAudioCaptureCallback(const float* InAudioIn, int32 NumSamples, int32 NumChannels, int32 SampleRate, double StreamTime, bool bOverrun);
};
