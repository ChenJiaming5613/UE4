// Copyright Epic Games, Inc. All Rights Reserved.

#include "NeuralGI.h"

#include "Interfaces/IPluginManager.h"
#include "CanvasItem.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"

#define LOCTEXT_NAMESPACE "FNeuralGIModule"

extern TAutoConsoleVariable<int32> CVarNeuralGIEnable;

void FNeuralGIModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("NeuralGI"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugins/NeuralGI"), PluginShaderDir);
	DrawDebugDelegateHandle = UDebugDrawService::Register(TEXT("Editor"), FDebugDrawDelegate::CreateRaw(this, &FNeuralGIModule::DrawDebugInfo));
}

void FNeuralGIModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UDebugDrawService::Unregister(DrawDebugDelegateHandle);
}

void FNeuralGIModule::DrawDebugInfo(UCanvas* Canvas, APlayerController* PC) const
{
	check(IsInGameThread());
	const int32 CurrentMode = CVarNeuralGIEnable.GetValueOnGameThread();
	if (CurrentMode == 0)
	{
		return;
	}

	const FString StatusText = TEXT("Enable Neural GI");
	const FLinearColor TextColor = FLinearColor::Green;
	
	if (Canvas && GEngine)
	{
		const UFont* Font = GEngine->GetSmallFont();
		FCanvasTextItem TextItem(FVector2D::ZeroVector, FText::FromString(StatusText), Font, TextColor);
		TextItem.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(TextItem, 50.0f, 50.0f);
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNeuralGIModule, NeuralGI)