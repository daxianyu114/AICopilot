#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AICopilotSettings.generated.h"

UENUM(BlueprintType)
enum class EAICopilotModelProvider : uint8
{
	OpenAI      UMETA(DisplayName = "OpenAI"),
	DeepSeek    UMETA(DisplayName = "DeepSeek"),
	Ollama      UMETA(DisplayName = "Ollama"),
	Gemini      UMETA(DisplayName = "Google Gemini (OpenAI Compatible)"),
	GeminiNative UMETA(DisplayName = "Google Gemini (Native API)"),
	Custom      UMETA(DisplayName = "Custom")
};

/**
 * Settings for the AI Copilot plugin.
 */
UCLASS(Config=Editor, DefaultConfig, meta=(DisplayName="AI Copilot"))
class AICOPILOT_API UAICopilotSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAICopilotSettings();

	/** The model provider to use. */
	UPROPERTY(EditAnywhere, Config, Category = "General", meta = (DisplayName = "Model Provider"))
	EAICopilotModelProvider ModelProvider;

	/** The API URL for the AI service. */
	UPROPERTY(EditAnywhere, Config, Category = "General", meta = (DisplayName = "API URL"))
	FString ApiUrl;

	/** The API Key for the AI service. */
	UPROPERTY(EditAnywhere, Config, Category = "General", meta = (DisplayName = "API Key", ScriptName = "ApiKey"))
	FString ApiKey;

	/** The model name to use (e.g., gpt-3.5-turbo). */
	UPROPERTY(EditAnywhere, Config, Category = "General", meta = (DisplayName = "Model Name"))
	FString ModelName;
};
