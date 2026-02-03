#include "NeuralGIViewExtension.h"
#include "Interfaces/IPluginManager.h"
#include "SceneView.h"

static TAutoConsoleVariable<int32> CVarNeuralGIEnable(
	TEXT("r.NeuralGI.Enable"),
	0,
	TEXT("Controls the Neural GI injection.\n")
	TEXT(" 0: Disable (sets vector to 0,0,0,0)\n")
	TEXT(" 1: Enable (sets vector to 1,0,0,0)"),
	ECVF_RenderThreadSafe
);

void LoadBinData(TResourceArray<float>& FloatResourceArray)
{
	const FString PluginDir = IPluginManager::Get().FindPlugin(TEXT("NeuralGI"))->GetBaseDir();
	const FString FilePath = FPaths::Combine(*PluginDir, TEXT("Resources"), TEXT("Data"), TEXT("model.bin"));

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

FNeuralGIViewExtension::FNeuralGIViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
	InitResources();
}

FNeuralGIViewExtension::~FNeuralGIViewExtension()
{
	ReleaseResources();
}

void FNeuralGIViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	if (VolumetricLightmapMLPBufferSRV.IsValid() && CVarNeuralGIEnable.GetValueOnGameThread())
	{
		InView.VolumetricLightmapMLPBuffer = VolumetricLightmapMLPBufferSRV;
		InView.VolumetricLightmapMLPInfoVector = FVector4(1.0f, 0, 0, 0);
	}
	else
	{
		InView.VolumetricLightmapMLPInfoVector = FVector4(0, 0, 0, 0);
	}
}

void FNeuralGIViewExtension::InitResources()
{
	TResourceArray<float> DataBufferCPU;
	LoadBinData(DataBufferCPU);
	if (DataBufferCPU.Num() == 0)
	{
		DataBufferCPU.AddZeroed(1024);
		DataBufferCPU[0] = 1.0f;
		DataBufferCPU[1] = 0.0f;
		DataBufferCPU[2] = 1.0f;
		DataBufferCPU[3] = 1.0f;
	}
	
	ENQUEUE_RENDER_COMMAND(AllocateVolumetricLightmapMLPBuffer)
	(
		[this, DataBufferCPU](FRHICommandListImmediate& RHICmdList) mutable
		{
			FRHIResourceCreateInfo CreateInfoData;
			CreateInfoData.ResourceArray = &DataBufferCPU;
			CreateInfoData.DebugName = TEXT("VolumetricLightmapMLPBuffer");
			VolumetricLightmapMLPBuffer.SafeRelease();
			VolumetricLightmapMLPBufferSRV.SafeRelease();
			VolumetricLightmapMLPBuffer = RHICreateStructuredBuffer(sizeof(float), DataBufferCPU.GetResourceDataSize(),
			                                                        BUF_StructuredBuffer | BUF_ShaderResource |
			                                                        BUF_Static, ERHIAccess::SRVMask, CreateInfoData);
			VolumetricLightmapMLPBufferSRV = RHICreateShaderResourceView(VolumetricLightmapMLPBuffer);
		}
	);
}

void FNeuralGIViewExtension::ReleaseResources()
{
	VolumetricLightmapMLPBuffer.SafeRelease();
	VolumetricLightmapMLPBufferSRV.SafeRelease();
}
