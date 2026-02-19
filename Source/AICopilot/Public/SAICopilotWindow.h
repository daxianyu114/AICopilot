#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Http.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Views/SListView.h"

struct FAIChatMessage
{
	FString Role; // "user" or "assistant"
	FString Content;
};

class SAICopilotWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAICopilotWindow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

    // --- New: Context Selection ---
    virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;    

    FReply OnAddFilesClicked();
    FReply OnAddFolderClicked();
    FReply OnRemoveContextItemClicked(TSharedPtr<FString> Item);

    TArray<TSharedPtr<FString>> SelectedContextPaths;
    TSharedPtr<SListView<TSharedPtr<FString>>> ContextListView;
    // ------------------------------

private:
	FReply OnSendClicked();
	FReply OnGenerateBlueprintClicked();
	FReply OnRunPythonClicked();
	FReply OnSettingsClicked();
	FReply OnAddSelectionContextClicked(); // NEW: Handler for selection context
	void OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnBlueprintResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void AddMessage(const FString& Role, const FString& Content);
	void CreateChatBubble(const FString& Role, const FString& Content); // Visualization: Create message bubble

	FReply OnClearChatClicked(); // Function: Clear Chat
	FReply OnSaveChatHistoryClicked(); // Function: Save History
	
	void SetPythonCode(const FString& Code);
	void AppendContext(const FString& NewContext); // NEW: Helper to append context

    // --- Agent Mode ---
    void OnAgentModeChanged(ECheckBoxState NewState);
    ECheckBoxState IsAgentModeChecked() const;
    bool bAgentMode = false;

    // Helper to send messages without user typing
    void SendDirectMessage(const FString& InRole, const FString& InContent);    
    // --- Control Buttons ---
    FReply OnStopClicked();
    FReply OnRetryClicked();
    FReply OnUndoChatClicked(); // "Withdraw" last message
    bool CanStop() const;
    bool CanRetry() const;
    bool CanUndoChat() const;

    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveRequest;    // ------------------

	TSharedPtr<SMultiLineEditableTextBox> InputTextBox;
	TSharedPtr<SScrollBox> ChatScrollBox;
	
	TSharedPtr<SMultiLineEditableTextBox> PythonCodeBox;
	TSharedPtr<SMultiLineEditableTextBox> ContextTextBox; // NEW: To access context box

	TArray<FAIChatMessage> ChatHistory;
	
	// History Tracking for Code
	TArray<FString> CodeHistory;
	int32 CurrentCodeIndex = -1;
	void PushCodeHistory(const FString& NewCode);
	FReply OnUndoCodeClicked();
	FReply OnRedoCodeClicked();
	TSharedPtr<SButton> UndoButton;
	TSharedPtr<SButton> RedoButton;
	void UpdateUndoRedoState();
};
