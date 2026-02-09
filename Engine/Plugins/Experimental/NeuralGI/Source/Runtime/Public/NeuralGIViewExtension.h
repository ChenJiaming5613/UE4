#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

class FNeuralGIViewExtension final : public FSceneViewExtensionBase
{
public:
	explicit FNeuralGIViewExtension(const FAutoRegister& AutoRegister);
	
	virtual ~FNeuralGIViewExtension() override;

	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override {}
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override {}

	void InitResources(TResourceArray<float>& DataBufferCPU);
	
private:
	void ReleaseResources();
	
	FStructuredBufferRHIRef VolumetricLightmapMLPBuffer;
	FShaderResourceViewRHIRef VolumetricLightmapMLPBufferSRV;
};
