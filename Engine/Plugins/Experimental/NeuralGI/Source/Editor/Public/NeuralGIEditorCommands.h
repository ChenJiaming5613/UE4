// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "NeuralGIEditorStyle.h"

class FNeuralGIEditorCommands : public TCommands<FNeuralGIEditorCommands>
{
public:

	FNeuralGIEditorCommands()
		: TCommands<FNeuralGIEditorCommands>(TEXT("NeuralGIEditor"), NSLOCTEXT("Contexts", "NeuralGIEditor", "NeuralGIEditor Plugin"), NAME_None, FNeuralGIEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
