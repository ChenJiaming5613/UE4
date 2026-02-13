#include "NeuralGIViewExtension.h"
#include "SceneView.h"
#include "ShaderParameterStruct.h"

static TAutoConsoleVariable<int32> CVarNeuralGIEnable(
	TEXT("r.NeuralGI.Enable"),
	0,
	TEXT("Controls the Neural GI Mode.\n")
	TEXT(" 0: Disable (sets vector to 0,0,0,0)\n")
	TEXT(" 1: Enable Inference Per Frame (sets vector to 1,0,0,0)")
	TEXT(" 2: Enable Inference Once (sets vector to 2,0,0,0)"),
	ECVF_RenderThreadSafe
);

class FNeuralGIInferenceCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FNeuralGIInferenceCS, Global);
	
	SHADER_USE_PARAMETER_STRUCT(FNeuralGIInferenceCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER_SRV(StructuredBuffer<float>, InVolumetricLightmapMLPBuffer) 
		SHADER_PARAMETER_UAV(RWTexture3D<float4>, OutVolumetricLightmapMLPTexture)
		SHADER_PARAMETER(FIntVector, Dimensions)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_SHADER_TYPE(, FNeuralGIInferenceCS, TEXT("/Plugins/NeuralGI/NeuralGIInference.usf"), TEXT("MainCS"), SF_Compute)

FNeuralGIViewExtension::FNeuralGIViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister), Dimensions(40, 40, 40)
{
}

FNeuralGIViewExtension::~FNeuralGIViewExtension()
{
	ReleaseResources();
}

void FNeuralGIViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	const int32 Mode = CVarNeuralGIEnable.GetValueOnGameThread();
	if (VolumetricLightmapMLPBufferSRV.IsValid() && VolumetricLightmapMLPTexture.IsValid() && Mode > 0)
	{
		InView.VolumetricLightmapMLPBuffer = VolumetricLightmapMLPBufferSRV;
		InView.VolumetricLightmapMLPTexture = VolumetricLightmapMLPTexture;
		InView.VolumetricLightmapMLPInfoVector = FVector4(Mode * 1.0f, 0, 0, 0);
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
	
	ENQUEUE_RENDER_COMMAND(InitializeVolumetricLightmapMLP)
	(
		[this, DataBufferCPU](FRHICommandListImmediate& RHICmdList) mutable
		{
			{
				FRHIResourceCreateInfo CreateInfoData;
			   CreateInfoData.ResourceArray = &DataBufferCPU;
			   CreateInfoData.DebugName = TEXT("VolumetricLightmapMLPBuffer");
			   VolumetricLightmapMLPBuffer.SafeRelease();
			   VolumetricLightmapMLPBufferSRV.SafeRelease();
			   VolumetricLightmapMLPBuffer = RHICreateStructuredBuffer(sizeof(float), DataBufferCPU.GetResourceDataSize(),
																	   BUF_StructuredBuffer | BUF_ShaderResource |
																	   BUF_Static, ERHIAccess::SRVMask, CreateInfoData);
			   VolumetricLightmapMLPBufferSRV = RHICreateShaderResourceView(VolumetricLightmapMLPBuffer.GetReference());
			}
			{
				FRHIResourceCreateInfo CreateInfo;
				CreateInfo.DebugName = TEXT("VolumetricLightmapMLPTexture");
				VolumetricLightmapMLPTexture.SafeRelease();
				VolumetricLightmapMLPTextureUAV.SafeRelease();
				VolumetricLightmapMLPTexture = RHICreateTexture3D(
					Dimensions.X, Dimensions.Y, Dimensions.Z, 
					PF_A32B32G32R32F,
					1,
					TexCreate_ShaderResource | TexCreate_UAV,
					CreateInfo);
				VolumetricLightmapMLPTextureUAV = RHICreateUnorderedAccessView(VolumetricLightmapMLPTexture.GetReference());
			}
			{
				const TShaderMapRef<FNeuralGIInferenceCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
				RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());
				FNeuralGIInferenceCS::FParameters PassParameters;
				PassParameters.InVolumetricLightmapMLPBuffer = VolumetricLightmapMLPBufferSRV.GetReference(); 
				PassParameters.OutVolumetricLightmapMLPTexture = VolumetricLightmapMLPTextureUAV.GetReference();
				PassParameters.Dimensions = Dimensions;

				SetShaderParameters(
					RHICmdList, 
					ComputeShader, 
					ComputeShader.GetComputeShader(), 
					PassParameters
				);

				const FIntVector GroupCount(
					FMath::DivideAndRoundUp(Dimensions.X, 8),
					FMath::DivideAndRoundUp(Dimensions.Y, 8),
					FMath::DivideAndRoundUp(Dimensions.Z, 8)
				);
				RHICmdList.DispatchComputeShader(GroupCount.X, GroupCount.Y, GroupCount.Z);
				RHICmdList.Transition(FRHITransitionInfo(
					VolumetricLightmapMLPTexture.GetReference(), 
					ERHIAccess::Unknown,
					ERHIAccess::SRVCompute | ERHIAccess::SRVGraphics
				));
			}
		}
	);
}

void FNeuralGIViewExtension::ReleaseResources()
{
	VolumetricLightmapMLPBuffer.SafeRelease();
	VolumetricLightmapMLPBufferSRV.SafeRelease();
	VolumetricLightmapMLPTexture.SafeRelease();
	VolumetricLightmapMLPTextureUAV.SafeRelease();
}
