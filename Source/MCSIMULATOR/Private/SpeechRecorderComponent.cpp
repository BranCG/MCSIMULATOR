#include "SpeechRecorderComponent.h"
#include "AudioCapture.h"
#include "Misc/ScopeLock.h"
#include "Math/UnrealMathUtility.h"

USpeechRecorderComponent::USpeechRecorderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsRecording = false;
	bStreamInitialized = false;
}

void USpeechRecorderComponent::BeginPlay()
{
	Super::BeginPlay();
	AudioCaptureDevice = MakeShared<Audio::FAudioCapture>();
	InitAudioStream();
}

void USpeechRecorderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AudioCaptureDevice.IsValid() && bStreamInitialized)
	{
		if (bIsRecording)
		{
			AudioCaptureDevice->StopStream();
		}
		AudioCaptureDevice->CloseStream();
		bStreamInitialized = false;
	}
	Super::EndPlay(EndPlayReason);
}

void USpeechRecorderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USpeechRecorderComponent::InitAudioStream()
{
	if (!AudioCaptureDevice.IsValid() || bStreamInitialized)
	{
		return;
	}

	Audio::FAudioCaptureDeviceParams Params;
	Params.DeviceIndex = 0; // Default active recording device on Windows/Quest

	Audio::FOnAudioCaptureFunction CaptureCallback = [this](const void* InAudio, int32 NumSamples, int32 NumChannels, int32 SampleRate, double StreamTime, bool bOverrun)
	{
		static bool bLoggedRate = false;
		if (!bLoggedRate)
		{
			bLoggedRate = true;
			UE_LOG(LogTemp, Log, TEXT("MC Simulator: WASAPI Audio Capture stream active at %d Hz, %d channels."), SampleRate, NumChannels);
		}
		const float* FloatAudio = static_cast<const float*>(InAudio);
		OnAudioCaptureCallback(FloatAudio, NumSamples, NumChannels, SampleRate, StreamTime, bOverrun);
	};

	bStreamInitialized = AudioCaptureDevice->OpenAudioCaptureStream(Params, CaptureCallback, 1024);
	if (bStreamInitialized)
	{
		AudioCaptureDevice->StartStream();
		UE_LOG(LogTemp, Log, TEXT("MC Simulator: Pre-initialized and started WASAPI audio capture stream."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MC Simulator: Failed to pre-initialize audio capture stream."));
	}
}

void USpeechRecorderComponent::StartRecording()
{
	if (bIsRecording || !AudioCaptureDevice.IsValid())
	{
		return;
	}

	if (!bStreamInitialized)
	{
		InitAudioStream();
	}

	{
		FScopeLock Lock(&BufferMutex);
		RecordedAudioBuffer.Empty();
	}

	bIsRecording = true;
	OnRecordingStarted.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("MC Simulator: Started accumulating audio buffer."));
}

void USpeechRecorderComponent::StopRecording()
{
	if (!bIsRecording || !AudioCaptureDevice.IsValid())
	{
		return;
	}

	bIsRecording = false;

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
	float MaxSampleAmp = 0.0f;
	
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
		float AbsSample = FMath::Abs(MonoSample);
		if (AbsSample > MaxSampleAmp)
		{
			MaxSampleAmp = AbsSample;
		}

		MonoSample = FMath::Clamp(MonoSample, -1.0f, 1.0f);

		// Scale to 16-bit range
		int16 IntSample = static_cast<int16>(MonoSample * 32767.0f);

		// Append raw bytes (Little Endian)
		RecordedAudioBuffer.Add(IntSample & 0xFF);
		RecordedAudioBuffer.Add((IntSample >> 8) & 0xFF);
	}

	if (MaxSampleAmp > 0.005f)
	{
		UE_LOG(LogTemp, Log, TEXT("MC Simulator: Voice detected in Mic Buffer! Peak Amplitude: %f"), MaxSampleAmp);
	}
}
