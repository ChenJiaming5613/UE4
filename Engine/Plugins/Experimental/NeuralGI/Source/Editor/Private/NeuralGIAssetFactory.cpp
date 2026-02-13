#include "NeuralGIAssetFactory.h"
#include "AssetTypeCategories.h"
#include "NeuralGIAsset.h"

UNeuralGIAssetFactory::UNeuralGIAssetFactory(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	SupportedClass = UNeuralGIAsset::StaticClass();

	bCreateNew = true;
}

UObject* UNeuralGIAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UNeuralGIAsset>(InParent, InClass, InName, Flags);
}

