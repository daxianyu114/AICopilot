#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboBox.h" // Added header

class SAICopilotSettings : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAICopilotSettings) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnSaveClicked();
	// Handler for provider change
	void OnModelProviderChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo); 
	void UpdateDefaultSettingsForProvider(const FString& Provider);

	TSharedPtr<SEditableTextBox> ApiUrlTextBox;
	TSharedPtr<SEditableTextBox> ApiKeyTextBox;
	TSharedPtr<SEditableTextBox> ModelNameTextBox;
	
	// NEW: UI Elements for Provider Selection
	TSharedPtr<SComboBox<TSharedPtr<FString>>> ProviderComboBox;
	TSharedPtr<STextBlock> ProviderComboText;
	TArray<TSharedPtr<FString>> ProviderOptions;

	TWeakPtr<SWindow> ParentWindow;

public:
	void SetParentWindow(TSharedPtr<SWindow> InWindow) { ParentWindow = InWindow; }
};
