#include "SpeechRecorderComponent.h"
#include "AudioCapture.h"
#include "Misc/ScopeLock.h"
#include "Math/UnrealMathUtility.h"

USpeechRecorderComponent::USpeechRecorderComponent()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsRecording = false;
}

void USpeechRecorderComponent::BeginPlay()
{
	Super::BeginPlay();
	AudioCaptureDevice = MakeShared<Audio::FAudioCapture>();
}

void USpeechRecorderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bIsRecording)
	{
		StopRecording();
	}
	Super::EndPlay(EndPlayReason);
}

void USpeechRecorderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USpeechRecorderComponent::StartRecording()
{
	if (bIsRecording || !AudioCaptureDevice.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MC Simulator: Already recording or audio capture device is invalid."));
		return;
	}

	{
		FScopeLock Lock(&BufferMutex);
		RecordedAudioBuffer.Empty();
	}

	Audio::FAudioCaptureDeviceParams Params;
	Params.DeviceIndex = 0; // Default recording device on Quest/Windows

	// Set up the audio capture callback lambda
	Audio::FOnAudioCaptureFunction CaptureCallback = [this](const float* InAudio, int32 NumSamples, int32 NumChannels, int32 SampleRate, double StreamTime, bool bOverrun)
	{
		OnAudioCaptureCallback(InAudio, NumSamples, NumChannels, SampleRate, StreamTime, bOverrun);
	};

	bool bStreamOpened = AudioCaptureDevice->OpenDefaultAudioStream(Params, CaptureCallback);
	if (bStreamOpened)
	{
		bool bStreamStarted = AudioCaptureDevice->StartStream();
		if (bStreamStarted)
		{
			bIsRecording = true;
			OnRecordingStarted.Broadcast();
			UE_LOG(LogTemp, Log, TEXT("MC Simulator: Started recording audio stream successfully."));
		}
		else
		{
			AudioCaptureDevice->CloseStream();
			UE_LOG(LogTemp, Error, TEXT("MC Simulator: Failed to start audio stream."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MC Simulator: Failed to open default audio capture stream."));
	}
}

void USpeechRecorderComponent::StopRecording()
{
	if (!bIsRecording || !AudioCaptureDevice.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MC Simulator: Not recording. Cannot stop."));
		return;
	}

	bIsRecording = false;

	AudioCaptureDevice->StopStream();
	AudioCaptureDevice->CloseStream();

	TArray<uint8> OutputPCMData;
	{
		FScopeLock Lock(&BufferMutex);
		OutputPCMData = RecordedAudioBuffer;
		RecordedAudioBuffer.Empty();
	}

	OnRecordingStopped.Broadcast(OutputPCMData);
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: Stopped audio capture. Broadcasted %d bytes of PCM data."), OutputPCMData.Num());
}

void USpeechRecorderComponent::OnAudioCaptureCallback(const float* InAudioIn, int32 NumSamples, int32 NumChannels, int32 SampleRate, double StreamTime, bool bOverrun)
{
	if (!bIsRecording || NumSamples <= 0 || NumChannels <= 0)
	{
		return;
	}

	FScopeLock Lock(&BufferMutex);

	// Downmix multi-channel to Mono and convert 32-bit Float (-1.0 to 1.0) to 16-bit PCM (signed short: -32768 to 32767)
	int32 NumFrames = NumSamples / NumChannels;
	
	// Pre-allocate space in buffer to improve performance
	RecordedAudioBuffer.Reserve(RecordedAudioBuffer.Num() + (NumFrames * 2));

	for (int32 Frame = 0; Frame < NumFrames; ++Frame)
	{
		float SampleSum = 0.f;
		for (int32 Channel = 0; Channel < NumChannels; ++Channel)
		{
			SampleSum += InAudioIn[Frame * NumChannels + Channel];
		}
		float MonoSample = SampleSum / NumChannels;
		MonoSample = FMath::Clamp(MonoSample, -1.0f, 1.0f);

		// Scale to 16-bit range
		int16 IntSample = static_cast<int16>(MonoSample * 32767.0f);

		// Append raw bytes (Little Endian)
		RecordedAudioBuffer.Add(IntSample & 0xFF);
		RecordedAudioBuffer.Add((IntSample >> 8) & 0xFF);
	}
}
