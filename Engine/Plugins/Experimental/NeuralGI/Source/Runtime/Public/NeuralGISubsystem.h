#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NeuralGISubsystem.generated.h"

class FNeuralGIViewExtension;

UCLASS()
class NEURALGI_API UNeuralGISubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	TSharedPtr<FNeuralGIViewExtension, ESPMode::ThreadSafe> Extension;
};
