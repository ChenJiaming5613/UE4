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

// static TAutoConsoleVariable<int32> CVarNeuralGICompare(
// 	TEXT("r.NeuralGI.Compare"),
// 	0,
// 	TEXT("Controls the Neural GI Compare.\n")
// 	TEXT(" 0: Disable\n")
// 	TEXT(" 1: Enable"),
// 	ECVF_RenderThreadSafe
// );

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

class FFillTestCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFillTestCS, Global);
	
	SHADER_USE_PARAMETER_STRUCT(FFillTestCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_UAV(RWTexture3D<float4>, OutVolumetricLightmapMLPTexture)
		SHADER_PARAMETER(FIntVector, Dimensions)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_SHADER_TYPE(, FFillTestCS, TEXT("/Plugins/NeuralGI/FillTest.usf"), TEXT("MainCS"), SF_Compute)


FNeuralGIViewExtension::FNeuralGIViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister), Dimensions(32, 32, 32)
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

void FNeuralGIViewExtension::PreRenderBasePass_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView,
	bool bDepthBufferIsPopulated)
{
	DispatchInferenceCS_RenderThread(RHICmdList, InView, bDepthBufferIsPopulated);
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
		}
	);
}

void FNeuralGIViewExtension::DispatchInferenceCS_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView,
	bool bDepthBufferIsPopulated) const
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
	RHICmdList.Transition(FRHITransitionInfo(
		VolumetricLightmapMLPTexture.GetReference(),
		ERHIAccess::Unknown,
		ERHIAccess::UAVCompute
	));
	RHICmdList.DispatchComputeShader(GroupCount.X, GroupCount.Y, GroupCount.Z);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(
		VolumetricLightmapMLPTexture.GetReference(), 
		ERHIAccess::UAVCompute,
		ERHIAccess::SRVCompute | ERHIAccess::SRVGraphics
	));
}

void FNeuralGIViewExtension::DispatchFillTestCS_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView,
	bool bDepthBufferIsPopulated) const
{
	const TShaderMapRef<FFillTestCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());
	FFillTestCS::FParameters PassParameters;
	PassParameters.View = InView.ViewUniformBuffer;
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
	RHICmdList.Transition(FRHITransitionInfo(
		VolumetricLightmapMLPTexture.GetReference(),
		ERHIAccess::Unknown,
		ERHIAccess::UAVCompute
	));
	RHICmdList.DispatchComputeShader(GroupCount.X, GroupCount.Y, GroupCount.Z);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(
		VolumetricLightmapMLPTexture.GetReference(), 
		ERHIAccess::UAVCompute,
		ERHIAccess::SRVCompute | ERHIAccess::SRVGraphics
	));
}

void FNeuralGIViewExtension::ReleaseResources()
{
	VolumetricLightmapMLPBuffer.SafeRelease();
	VolumetricLightmapMLPBufferSRV.SafeRelease();
	VolumetricLightmapMLPTexture.SafeRelease();
	VolumetricLightmapMLPTextureUAV.SafeRelease();
}
