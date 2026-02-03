// Copyright Epic Games, Inc. All Rights Reserved.

#include "NeuralGIEditorCommands.h"

#define LOCTEXT_NAMESPACE "FNeuralGIEditorModule"

void FNeuralGIEditorCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "[NeuralGI Editor] Export", "Export VLM Data", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
