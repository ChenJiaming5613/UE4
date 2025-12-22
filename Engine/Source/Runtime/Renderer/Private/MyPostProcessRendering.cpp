#include "MyPostProcessRendering.h"
#include "ScreenPass.h"

class FMyPostProcessPS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FMyPostProcessPS, Global);

	SHADER_USE_PARAMETER_STRUCT(FMyPostProcessPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, ScreenColorTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, ScreenColorSampler)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_SHADER_TYPE(, FMyPostProcessPS, TEXT("/Engine/Private/MyPostProcess.usf"), TEXT("MainPS"), SF_Pixel)

void RenderMyPostProcess(FRDGBuilder& GraphBuilder, const FViewInfo& View, FRDGTextureRef InputColorTexture,
						 FRDGTextureRef ViewFamilyTexture)
{
	FRDGTextureRef CopiedViewFamilyTexture = GraphBuilder.CreateTexture(ViewFamilyTexture->Desc, TEXT("CopiedViewFamilyTexture"));
	AddCopyTexturePass(GraphBuilder, ViewFamilyTexture, CopiedViewFamilyTexture);

	TShaderMapRef<FMyPostProcessPS> PixelShader(View.ShaderMap);
	FMyPostProcessPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FMyPostProcessPS::FParameters>();
	PassParameters->View = View.ViewUniformBuffer;
	PassParameters->ScreenColorTexture = CopiedViewFamilyTexture;
	PassParameters->ScreenColorSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::CreateRHI();
	PassParameters->RenderTargets[0] = FRenderTargetBinding(ViewFamilyTexture, ERenderTargetLoadAction::ENoAction);

	AddDrawScreenPass(GraphBuilder, RDG_EVENT_NAME("MyPostProcessPass"), View,
		FScreenPassTextureViewport(ViewFamilyTexture),
		FScreenPassTextureViewport(ViewFamilyTexture),
		PixelShader, PassParameters
	);
}