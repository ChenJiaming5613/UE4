#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NeuralGIAsset.generated.h"

UENUM(BlueprintType)
enum class ENeuralActivationFunction : uint8
{
	ReLU        UMETA(DisplayName = "ReLU"),
	SIREN		UMETA(DisplayName = "SIREN"),
	None        UMETA(DisplayName = "None (Linear)")
};

UCLASS()
class UNeuralGIAsset : public UObject
{
	GENERATED_BODY()

public:
	explicit UNeuralGIAsset(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Network Architecture")
	int32 GetExpectedParameterCount() const;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Architecture", meta = (ClampMin = "1", UIMin = "1"))
	int32 InputDimension;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Architecture")
	TArray<int32> HiddenLayers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Architecture", meta = (ClampMin = "1", UIMin = "1"))
	int32 OutputDimension;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network Architecture")
	ENeuralActivationFunction ActivationFunction;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	TArray<float> DataBufferCPU;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metadata")
	TSoftObjectPtr<UWorld> AssociatedLevel;
};
