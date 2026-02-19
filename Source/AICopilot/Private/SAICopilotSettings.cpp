#include "SAICopilotSettings.h"
#include "AICopilotSettings.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "AICopilotSettings"

void SAICopilotSettings::Construct(const FArguments& InArgs)
{
	const UAICopilotSettings* Settings = GetDefault<UAICopilotSettings>();

	// Initialize Provider Options
	ProviderOptions.Add(MakeShareable(new FString("OpenAI")));
	ProviderOptions.Add(MakeShareable(new FString("DeepSeek")));
	ProviderOptions.Add(MakeShareable(new FString("Ollama")));
	ProviderOptions.Add(MakeShareable(new FString("Gemini (OpenAI)")));
	ProviderOptions.Add(MakeShareable(new FString("Gemini (Native)")));
	ProviderOptions.Add(MakeShareable(new FString("Custom")));

	// Determine current selection based on settings
	TSharedPtr<FString> CurrentProvider = ProviderOptions[0]; // Default to OpenAI
	if (Settings->ModelProvider == EAICopilotModelProvider::DeepSeek)
	{
		CurrentProvider = ProviderOptions[1];
	}
	else if (Settings->ModelProvider == EAICopilotModelProvider::Ollama)
	{
		CurrentProvider = ProviderOptions[2];
	}
	else if (Settings->ModelProvider == EAICopilotModelProvider::Gemini)
	{
		CurrentProvider = ProviderOptions[3];
	}
	else if (Settings->ModelProvider == EAICopilotModelProvider::GeminiNative)
	{
		CurrentProvider = ProviderOptions[4];
	}
	else if (Settings->ModelProvider == EAICopilotModelProvider::Custom)
	{
		CurrentProvider = ProviderOptions[5];
	}
	// Note: OpenAI is index 0

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10)
		[
			SNew(SGridPanel)
			.FillColumn(1, 1.0f)

			// Provider Selection
			+ SGridPanel::Slot(0, 0)
			.Padding(5)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("ProviderLabel", "AI Provider:"))
			]
			+ SGridPanel::Slot(1, 0)
			.Padding(5)
			[
				SAssignNew(ProviderComboBox, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&ProviderOptions)
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(*Item));
				})
				.OnSelectionChanged(this, &SAICopilotSettings::OnModelProviderChanged)
				.InitiallySelectedItem(CurrentProvider)
				[
					SAssignNew(ProviderComboText, STextBlock)
					.Text(FText::FromString(*CurrentProvider))
				]
			]

			// API URL
			+ SGridPanel::Slot(0, 1)
			.Padding(5)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("ApiUrlLabel", "API URL:"))
			]
			+ SGridPanel::Slot(1, 1)
			.Padding(5)
			[
				SAssignNew(ApiUrlTextBox, SEditableTextBox)
				.Text(FText::FromString(Settings->ApiUrl))
			]

			// API Key
			+ SGridPanel::Slot(0, 2)
			.Padding(5)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("ApiKeyLabel", "API Key:"))
			]
			+ SGridPanel::Slot(1, 2)
			.Padding(5)
			[
				SAssignNew(ApiKeyTextBox, SEditableTextBox)
				.Text(FText::FromString(Settings->ApiKey))
				.IsPassword(true)
			]

			// Model Name
			+ SGridPanel::Slot(0, 3)
			.Padding(5)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("ModelNameLabel", "Model Name:"))
			]
			+ SGridPanel::Slot(1, 3)
			.Padding(5)
			[
				SAssignNew(ModelNameTextBox, SEditableTextBox)
				.Text(FText::FromString(Settings->ModelName))
			]
		]

		// Save Button
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(10)
		.HAlign(HAlign_Right)
		[
			SNew(SButton)
			.Text(LOCTEXT("SaveButton", "Save Settings"))
			.OnClicked(this, &SAICopilotSettings::OnSaveClicked)
		]
	];
}

void SAICopilotSettings::OnModelProviderChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	if (NewValue.IsValid())
	{
		ProviderComboText->SetText(FText::FromString(*NewValue));
		UpdateDefaultSettingsForProvider(*NewValue);
	}
}

void SAICopilotSettings::UpdateDefaultSettingsForProvider(const FString& Provider)
{
	if (Provider == "OpenAI")
	{
		ApiUrlTextBox->SetText(FText::FromString("https://api.openai.com/v1/chat/completions"));
		ModelNameTextBox->SetText(FText::FromString("gpt-3.5-turbo"));
	}
	else if (Provider == "DeepSeek")
	{
		ApiUrlTextBox->SetText(FText::FromString("https://api.deepseek.com/chat/completions"));
		ModelNameTextBox->SetText(FText::FromString("deepseek-chat"));
	}
	else if (Provider == "Ollama")
	{
		ApiUrlTextBox->SetText(FText::FromString("http://localhost:11434/v1/chat/completions"));
		ModelNameTextBox->SetText(FText::FromString("llama3"));
	}
	else if (Provider == "Gemini (OpenAI)")
	{
		// Use Google's OpenAI-compatible endpoint
		ApiUrlTextBox->SetText(FText::FromString("https://generativelanguage.googleapis.com/v1beta/openai/chat/completions"));
		ModelNameTextBox->SetText(FText::FromString("gemini-1.5-flash"));
	}
	else if (Provider == "Gemini (Native)")
	{
		// Use Google's Native endpoint (Requires key in query params usually, but we will handle it)
		// Note: The URL usually ends with :generateContent
		ApiUrlTextBox->SetText(FText::FromString("https://generativelanguage.googleapis.com/v1beta/models/gemini-pro:generateContent"));
		ModelNameTextBox->SetText(FText::FromString("gemini-pro"));
	}
	// Custom: Do nothing, let user edit freely
}

FReply SAICopilotSettings::OnSaveClicked()
{
	UAICopilotSettings* Settings = GetMutableDefault<UAICopilotSettings>();
	
	Settings->ApiUrl = ApiUrlTextBox->GetText().ToString();
	Settings->ApiKey = ApiKeyTextBox->GetText().ToString();
	Settings->ModelName = ModelNameTextBox->GetText().ToString();
	
	FString SelectedProvider = ProviderComboText->GetText().ToString();
	if (SelectedProvider == "OpenAI")
	{
		Settings->ModelProvider = EAICopilotModelProvider::OpenAI;
	}
	else if (SelectedProvider == "DeepSeek")
	{
		Settings->ModelProvider = EAICopilotModelProvider::DeepSeek;
	}
	else if (SelectedProvider == "Ollama")
	{
		Settings->ModelProvider = EAICopilotModelProvider::Ollama;
	}
	else if (SelectedProvider == "Gemini (OpenAI)")
	{
		Settings->ModelProvider = EAICopilotModelProvider::Gemini;
	}
	else if (SelectedProvider == "Gemini (Native)")
	{
		Settings->ModelProvider = EAICopilotModelProvider::GeminiNative;
	}
	else
	{
		Settings->ModelProvider = EAICopilotModelProvider::Custom;
	}

	Settings->SaveConfig();

	if (ParentWindow.IsValid())
	{
		ParentWindow.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
