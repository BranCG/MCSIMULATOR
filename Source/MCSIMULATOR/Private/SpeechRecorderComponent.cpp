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

	Audio::FAudioCaptureDeviceParams Params; // Default constructor selects INDEX_NONE (Windows Default Recording Device)

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
		UE_LOG(LogTemp, Log, TEXT("MC Simulator: Pre-initialized and started continuous WASAPI audio capture stream."));
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

	// Re-initialize or refresh stream to wake up Quest Link mic if headset entered sleep/standby
	if (bStreamInitialized)
	{
		AudioCaptureDevice->StopStream();
		AudioCaptureDevice->StartStream();
	}
	else
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

	int32 NumFrames = NumSamples / NumChannels;
	RecordedAudioBuffer.Reserve(RecordedAudioBuffer.Num() + (NumFrames * 2));

	// Test if input is 32-bit float or 16-bit int
	bool bIsFloatFormat = true;
	float MaxFloat = 0.0f;
	for (int32 i = 0; i < FMath::Min(NumSamples, 64); ++i)
	{
		float AbsVal = FMath::Abs(InAudioIn[i]);
		if (AbsVal > MaxFloat)
		{
			MaxFloat = AbsVal;
		}
	}

	// If 32-bit float samples evaluate to 0, check if buffer contains 16-bit PCM integers
	if (MaxFloat < 0.0001f)
	{
		const int16* IntAudio = reinterpret_cast<const int16*>(InAudioIn);
		int16 MaxInt = 0;
		for (int32 i = 0; i < FMath::Min(NumSamples, 64); ++i)
		{
			int16 AbsVal = FMath::Abs(IntAudio[i]);
			if (AbsVal > MaxInt)
			{
				MaxInt = AbsVal;
			}
		}
		if (MaxInt > 10)
		{
			bIsFloatFormat = false;
		}
	}

	if (bIsFloatFormat)
	{
		for (int32 Frame = 0; Frame < NumFrames; ++Frame)
		{
			float SampleSum = 0.f;
			for (int32 Channel = 0; Channel < NumChannels; ++Channel)
			{
				SampleSum += InAudioIn[Frame * NumChannels + Channel];
			}
			float MonoSample = SampleSum / NumChannels;
			MonoSample = FMath::Clamp(MonoSample, -1.0f, 1.0f);
			int16 IntSample = static_cast<int16>(MonoSample * 32767.0f);

			RecordedAudioBuffer.Add(IntSample & 0xFF);
			RecordedAudioBuffer.Add((IntSample >> 8) & 0xFF);
		}
	}
	else
	{
		// Native 16-bit PCM Integer path
		const int16* IntAudio = reinterpret_cast<const int16*>(InAudioIn);
		for (int32 Frame = 0; Frame < NumFrames; ++Frame)
		{
			int32 SampleSum = 0;
			for (int32 Channel = 0; Channel < NumChannels; ++Channel)
			{
				SampleSum += IntAudio[Frame * NumChannels + Channel];
			}
			int16 MonoSample = static_cast<int16>(SampleSum / NumChannels);

			RecordedAudioBuffer.Add(MonoSample & 0xFF);
			RecordedAudioBuffer.Add((MonoSample >> 8) & 0xFF);
		}
	}
}
