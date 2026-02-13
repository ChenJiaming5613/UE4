#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "NeuralGIAssetFactory.generated.h"

UCLASS()
class UNeuralGIAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	explicit UNeuralGIAssetFactory(const FObjectInitializer& ObjectInitializer);

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
