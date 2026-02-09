#include "NeuralGIViewExtension.h"
#include "SceneView.h"

static TAutoConsoleVariable<int32> CVarNeuralGIEnable(
	TEXT("r.NeuralGI.Enable"),
	0,
	TEXT("Controls the Neural GI injection.\n")
	TEXT(" 0: Disable (sets vector to 0,0,0,0)\n")
	TEXT(" 1: Enable (sets vector to 1,0,0,0)"),
	ECVF_RenderThreadSafe
);

FNeuralGIViewExtension::FNeuralGIViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
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

void FNeuralGIViewExtension::InitResources(TResourceArray<float>& DataBufferCPU)
{
	if (DataBufferCPU.Num() == 0)
	{
		return;
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
