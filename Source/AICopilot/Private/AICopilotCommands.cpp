#include "AICopilotCommands.h"

#define LOCTEXT_NAMESPACE "FAICopilotModule"

void FAICopilotCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "AICopilot", "Bring up AICopilot window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
