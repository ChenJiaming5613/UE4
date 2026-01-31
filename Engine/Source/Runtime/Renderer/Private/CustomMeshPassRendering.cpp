#include "SceneUtils.h"
#include "ScenePrivate.h"
#include "DeferredShadingRenderer.h"
#include "MeshMaterialShader.h"
#include "MeshPassProcessor.inl"

class FCustomMeshPassVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FCustomMeshPassVS, MeshMaterial);

public:
	FCustomMeshPassVS() {}

	FCustomMeshPassVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer) {}

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5)
				&& Parameters.VertexFactoryType == &FLocalVertexFactory::StaticType;
	}
};

class FCustomMeshPassPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FCustomMeshPassPS, MeshMaterial);

public:
	FCustomMeshPassPS() {}
	
	FCustomMeshPassPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer) {}

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5)
				&& Parameters.VertexFactoryType == &FLocalVertexFactory::StaticType;
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(, FCustomMeshPassVS, TEXT("/Engine/Private/CustomMeshPassShader.usf"), TEXT("MainVS"), SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(, FCustomMeshPassPS, TEXT("/Engine/Private/CustomMeshPassShader.usf"), TEXT("MainPS"), SF_Pixel);

class FCustomMeshPassMeshProcessor : public FMeshPassProcessor
{
public:

	FCustomMeshPassMeshProcessor(const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext);

	virtual void AddMeshBatch(const FMeshBatch& RESTRICT MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy, int32 StaticMeshId = -1) override final;
};

FCustomMeshPassMeshProcessor::FCustomMeshPassMeshProcessor(const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
	: FMeshPassProcessor(Scene, Scene->GetFeatureLevel(), InViewIfDynamicMeshCommand, InDrawListContext)
{
}

void FCustomMeshPassMeshProcessor::AddMeshBatch(const FMeshBatch& MeshBatch, uint64 BatchElementMask, const FPrimitiveSceneProxy* PrimitiveSceneProxy, int32 StaticMeshId)
{
	const FMaterialRenderProxy* FallbackMaterialRenderProxyPtr = nullptr;
	const FMaterial& Material = MeshBatch.MaterialRenderProxy->GetMaterialWithFallback(FeatureLevel, FallbackMaterialRenderProxyPtr);
	const FMaterialRenderProxy& MaterialRenderProxy = FallbackMaterialRenderProxyPtr ? *FallbackMaterialRenderProxyPtr : *MeshBatch.MaterialRenderProxy;

	// Filter: Opaque, Lit, Static Mesh
	{
		if (IsTranslucentBlendMode(Material.GetBlendMode()))
		{
			return;
		}

		if (!Material.GetShadingModels().IsLit())
		{
			return;
		}

		const FVertexFactoryType* VertexFactoryType = MeshBatch.VertexFactory->GetType();
		const bool bIsStaticMesh = VertexFactoryType == &FLocalVertexFactory::StaticType;
		if (!bIsStaticMesh)
		{
			return; 
		}
	}

	TMeshProcessorShaders<
		FCustomMeshPassVS,
		FBaseHS,
		FBaseDS,
		FCustomMeshPassPS> PassShaders;

	const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;
	PassShaders.VertexShader = Material.GetShader<FCustomMeshPassVS>(VertexFactory->GetType());
	PassShaders.PixelShader = Material.GetShader<FCustomMeshPassPS>(VertexFactory->GetType());

	FMeshPassProcessorRenderState PassDrawRenderState;
	PassDrawRenderState.SetViewUniformBuffer(Scene->UniformBuffers.ViewUniformBuffer);
	PassDrawRenderState.SetBlendState(TStaticBlendState<>::GetRHI());
	PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<>::GetRHI());

	const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
	const ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(MeshBatch, Material, OverrideSettings);
	const ERasterizerCullMode MeshCullMode = ComputeMeshCullMode(MeshBatch, Material, OverrideSettings);
	
	FMeshMaterialShaderElementData ShaderElementData;
	ShaderElementData.InitializeMeshMaterialData(ViewIfDynamicMeshCommand, PrimitiveSceneProxy, MeshBatch, StaticMeshId, true);

	const FMeshDrawCommandSortKey SortKey = CalculateMeshStaticSortKey(PassShaders.VertexShader, PassShaders.PixelShader);
	
	BuildMeshDrawCommands(
		MeshBatch,
		BatchElementMask,
		PrimitiveSceneProxy,
		MaterialRenderProxy,
		Material,
		PassDrawRenderState,
		PassShaders,
		MeshFillMode,
		MeshCullMode,
		SortKey,
		EMeshPassFeatures::Default,
		ShaderElementData);
}

FMeshPassProcessor* CreateCustomMeshPassProcessor(const FScene* Scene, const FSceneView* InViewIfDynamicMeshCommand, FMeshPassDrawListContext* InDrawListContext)
{
	return new(FMemStack::Get()) FCustomMeshPassMeshProcessor(Scene, InViewIfDynamicMeshCommand, InDrawListContext);
}

FRegisterPassProcessorCreateFunction RegisterCustomMeshPass(&CreateCustomMeshPassProcessor, EShadingPath::Deferred, EMeshPass::CustomMeshPass, EMeshPassFlags::MainView);

BEGIN_SHADER_PARAMETER_STRUCT(FCustomMeshPassParameters, )
	RENDER_TARGET_BINDING_SLOTS()
END_SHADER_PARAMETER_STRUCT()

void FDeferredShadingSceneRenderer::RenderCustomMeshPass(FRDGBuilder& GraphBuilder, const FRenderTargetBindingSlots& RenderTargets)
{
	RDG_EVENT_SCOPE(GraphBuilder, "CustomMeshPass");

	auto* PassParameters = GraphBuilder.AllocParameters<FCustomMeshPassParameters>();
	PassParameters->RenderTargets = RenderTargets;

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
	{
		FViewInfo& View = Views[ViewIndex];
		RDG_GPU_MASK_SCOPE(GraphBuilder, View.GPUMask);
		RDG_EVENT_SCOPE_CONDITIONAL(GraphBuilder, Views.Num() > 1, "View%d", ViewIndex);

		GraphBuilder.AddPass(
			{},
			PassParameters,
			ERDGPassFlags::Raster,
			[this, &View](FRHICommandList& RHICmdList)
		{
			Scene->UniformBuffers.UpdateViewUniformBuffer(View);
			RHICmdList.SetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1);
			View.ParallelMeshDrawCommandPasses[EMeshPass::CustomMeshPass].DispatchDraw(nullptr, RHICmdList);
		});
	}
}