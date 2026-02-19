#include "AICopilotSettings.h"

UAICopilotSettings::UAICopilotSettings()
{
	ModelProvider = EAICopilotModelProvider::OpenAI;
	ApiUrl = TEXT("https://api.openai.com/v1/chat/completions");
	ApiKey = TEXT("");
	ModelName = TEXT("gpt-3.5-turbo");
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("AICopilot");
}
