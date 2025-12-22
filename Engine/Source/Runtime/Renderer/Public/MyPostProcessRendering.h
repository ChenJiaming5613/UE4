#pragma once
#include "SceneInterface.h"

void RenderMyPostProcess(FRDGBuilder& GraphBuilder, const FViewInfo& View, FRDGTextureRef InputColorTexture,
                         FRDGTextureRef ViewFamilyTexture);