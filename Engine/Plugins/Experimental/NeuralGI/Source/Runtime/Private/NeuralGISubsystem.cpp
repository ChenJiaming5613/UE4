#include "NeuralGISubsystem.h"
#include "Interfaces/IPluginManager.h"
#include "SceneViewExtension.h"
#include "NeuralGIViewExtension.h"

void LoadBinData(const FString& FilePath, TResourceArray<float>& FloatResourceArray)
{
	const FString PluginDir = IPluginManager::Get().FindPlugin(TEXT("NeuralGI"))->GetBaseDir();

	const TUniquePtr<FArchive> FileReader(IFileManager::Get().CreateFileReader(*FilePath));
	if (FileReader)
	{
		const int64 FileSize = FileReader->TotalSize();
		const int32 NumFloats = FileSize / sizeof(float);

		if (NumFloats > 0)
		{
			FloatResourceArray.Empty(NumFloats);
			FloatResourceArray.AddUninitialized(NumFloats);
			FileReader->Serialize(FloatResourceArray.GetData(), FileSize);
		}
		FileReader->Close();
        
		UE_LOG(LogTemp, Log, TEXT("Successfully loaded %d floats from plugin directory."), NumFloats);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load bin file at: %s"), *FilePath);
	}
}

void UNeuralGISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Extension = FSceneViewExtensions::NewExtension<FNeuralGIViewExtension>();
	TResourceArray<float> DataBufferCPU;
	{
		const UWorld* World = GetWorld();
		if (!World) return;

		const FString CurrentPackageName = World->GetOutermost()->GetName();
		const FString CleanPackageName = UWorld::RemovePIEPrefix(CurrentPackageName);
		const FString MapAbsPath = FPackageName::LongPackageNameToFilename(CleanPackageName, FPackageName::GetMapPackageExtension());
		FString MapDir = FPaths::GetPath(MapAbsPath);
		const FString LevelName = FPackageName::GetShortName(CleanPackageName);
		const FString BinFilePath = FPaths::Combine(MapDir, LevelName + TEXT("_VLM-MLP.bin"));
		LoadBinData(BinFilePath, DataBufferCPU);
	}
	Extension->InitResources(DataBufferCPU);
	UE_LOG(LogTemp, Log, TEXT("NeuralGI: Subsystem initialized!"));
}

void UNeuralGISubsystem::Deinitialize()
{
	Super::Deinitialize();
	UE_LOG(LogTemp, Log, TEXT("NeuralGI: Subsystem deinitialize!"));
}

bool UNeuralGISubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game
		|| WorldType == EWorldType::PIE
		|| WorldType == EWorldType::Editor;
}
