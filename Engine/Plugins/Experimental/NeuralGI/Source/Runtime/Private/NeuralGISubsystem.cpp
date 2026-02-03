#include "NeuralGISubsystem.h"
#include "SceneViewExtension.h"
#include "NeuralGIViewExtension.h"

void UNeuralGISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Extension = FSceneViewExtensions::NewExtension<FNeuralGIViewExtension>();
	UE_LOG(LogTemp, Log, TEXT("NeuralGI: Subsystem initialized!"));
}

void UNeuralGISubsystem::Deinitialize()
{
	Super::Deinitialize();
	UE_LOG(LogTemp, Log, TEXT("NeuralGI: Subsystem deinitialize!"));
}
