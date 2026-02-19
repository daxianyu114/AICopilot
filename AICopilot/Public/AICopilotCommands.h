#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "AICopilotStyle.h"

class FAICopilotCommands : public TCommands<FAICopilotCommands>
{
public:

	FAICopilotCommands()
		: TCommands<FAICopilotCommands>(TEXT("AICopilot"), NSLOCTEXT("Contexts", "AICopilot", "AICopilot Plugin"), NAME_None, FAICopilotStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

	public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};
