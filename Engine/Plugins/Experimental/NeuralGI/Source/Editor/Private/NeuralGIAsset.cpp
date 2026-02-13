#include "NeuralGIAsset.h"

UNeuralGIAsset::UNeuralGIAsset(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InputDimension = 3;
	OutputDimension = 3;
	HiddenLayers = {64, 64, 64, 64, 64, 64};
	ActivationFunction = ENeuralActivationFunction::ReLU;
}

int32 UNeuralGIAsset::GetExpectedParameterCount() const
{
	int32 TotalParams = 0;
	int32 CurrentInputDim = InputDimension;

	for (const int32 HiddenDim : HiddenLayers)
	{
		// Weights: Previous_Layer_Dim * Current_Layer_Dim
		// Biases:  Current_Layer_Dim
		TotalParams += (CurrentInputDim * HiddenDim) + HiddenDim;
		CurrentInputDim = HiddenDim;
	}

	TotalParams += (CurrentInputDim * OutputDimension) + OutputDimension;

	return TotalParams;
}
