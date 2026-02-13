#pragma once
#include "AssetTypeActions_Base.h"
#include "NeuralGIAsset.h"

class FAssetTypeActions_NeuralGIAsset final : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override
	{
		return NSLOCTEXT("AssetTypeActions" , "FAssetTypeActions_NeuralGIAsset" , "NeuralGIAsset");
	}

	virtual uint32 GetCategories() override { return EAssetTypeCategories::Misc; }

	virtual FColor GetTypeColor() const override { return FColor(0, 255, 0); }

	virtual UClass* GetSupportedClass() const override { return UNeuralGIAsset::StaticClass(); }
};
