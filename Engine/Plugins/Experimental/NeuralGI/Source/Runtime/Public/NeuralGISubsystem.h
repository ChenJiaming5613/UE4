#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "NeuralGISubsystem.generated.h"

class FNeuralGIViewExtension;

UCLASS()
class NEURALGI_API UNeuralGISubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	TSharedPtr<FNeuralGIViewExtension, ESPMode::ThreadSafe> Extension;
};
