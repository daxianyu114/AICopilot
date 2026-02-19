#include "SAICopilotWindow.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "AICopilotSettings.h"
#include "SAICopilotSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Modules/ModuleManager.h"
#include "IPythonScriptPlugin.h"

// NEW: Headers
#include "HAL/PlatformApplicationMisc.h"
#include "Engine/Selection.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "Exporters/Exporter.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Editor/EditorEngine.h"

FReply SAICopilotWindow::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
	if (Operation.IsValid())
	{
		// Accept general dragging
        return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SAICopilotWindow::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
	if (Operation.IsValid())
	{
        // Try to handle as external file drop (Windows Explorer)
        if(Operation->IsOfType<FExternalDragOperation>())
        {
             TSharedPtr<FExternalDragOperation> DragOp = StaticCastSharedPtr<FExternalDragOperation>(Operation);
             if(DragOp->HasFiles()) 
             {
                 for(const FString& File : DragOp->GetFiles())
                 {
                     SelectedContextPaths.Add(MakeShareable(new FString(File)));
                 }
                 if(ContextListView.IsValid()) ContextListView->RequestListRefresh();
                 return FReply::Handled();
             }
        }
        
        // Try to handle Asset Drag (Content Browser)
        // Note: FAssetDragDropOp requires "ContentBrowser" module dependency usually,
        // but we can check the easier way via AssetRegistry paths if possible or just skip strict type checking
        // Actually, FAssetDragDropOp is in EditorFramework which is available.
        // But headers might be tricky. Let's try minimal assumption or use generic detection if possible.
        // For simplicity, let's just stick to File Picker first as requested. 
        // We added DragOver/Drop stubs to SAICopilotWindow.h, so we must implement them to link.
	}
	return FReply::Unhandled();
}
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "IDirectoryWatcher.h"
#include "DirectoryWatcherModule.h"
#include "Misc/FileHelper.h"

#define LOCTEXT_NAMESPACE "AICopilot"

void SAICopilotWindow::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SVerticalBox)
		
		// Top Toolbar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			
			// Left: Chat Tools
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2)
			[
				SNew(SButton)
				.Text(LOCTEXT("ClearChatButton", "清空"))
				.ToolTipText(LOCTEXT("ClearChatTooltip", "清空聊天记录"))
				.OnClicked(this, &SAICopilotWindow::OnClearChatClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2)
			[
				SNew(SButton)
				.Text(LOCTEXT("SaveChatButton", "保存聊天"))
				.ToolTipText(LOCTEXT("SaveChatTooltip", "保存当前对话到日志"))
				.OnClicked(this, &SAICopilotWindow::OnSaveChatHistoryClicked)
			]

			// Right: Settings
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Right)
			[
				SNew(SButton)
				.Text(LOCTEXT("SettingsButton", "设置"))
				.ToolTipText(LOCTEXT("SettingsTooltip", "配置 API Key 和模型"))
				.OnClicked(this, &SAICopilotWindow::OnSettingsClicked)
			]
		]

		// Chat History Area
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(5)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(5)
			[
				SAssignNew(ChatScrollBox, SScrollBox)
			]
		]

		// Context Input
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ContextLabel", "Project Context:"))
			]

            // --- New: File/Folder Picker List ---
            + SVerticalBox::Slot()
            .AutoHeight()
            .MaxHeight(150) // Increased height for better visibility
            .Padding(0, 5)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                .Padding(2)
                [
                    SAssignNew(ContextListView, SListView<TSharedPtr<FString>>)
                    .ItemHeight(24)
                    .ListItemsSource(&SelectedContextPaths)
                    .OnGenerateRow_Lambda([this](TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
                    {
                        bool bIsDirectory = FPaths::DirectoryExists(*Item);
                        
                        const FSlateBrush* IconBrush = nullptr;
                        FString Ext = FPaths::GetExtension(*Item).ToLower();

                        if (bIsDirectory)
                        {
                            IconBrush = FAppStyle::GetBrush("ContentBrowser.AssetTreeFolderClosed");
                        }
                        else if (Ext == "uasset")
                        {
                             // Try to find a Blueprint icon (LevelEditor.Tabs.Blueprints or ClassIcon.Blueprint)
                             IconBrush = FAppStyle::GetBrush("LevelEditor.Tabs.Blueprints");
                        }
                        else if (Ext == "h" || Ext == "cpp" || Ext == "cs")
                        {
                             // Use a code icon if possible, or generic file
                             IconBrush = FAppStyle::GetBrush("LevelEditor.Tabs.FindResults"); // Looking glass or similar logic
                        }
                        else
                        {
                             IconBrush = FAppStyle::GetBrush("LevelEditor.Tabs.Details");
                        }
                        
                        // Fallback if brush not found (though GetBrush returns default usually)
                        if (!IconBrush) IconBrush = FAppStyle::GetBrush("LevelEditor.Tabs.Details");

                        return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
                        [
                            SNew(SHorizontalBox)
                            // Icon
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(4, 0)
                            [
                                SNew(SImage)
                                .Image(IconBrush)
                                .ColorAndOpacity(FSlateColor::UseForeground())
                            ]
                            // Filename (Truncated path)
                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            .VAlign(VAlign_Center)
                            .Padding(2)
                            [
                                SNew(STextBlock)
                                .Text(FText::FromString(FPaths::GetCleanFilename(*Item).IsEmpty() ? *Item : FPaths::GetCleanFilename(*Item)))
                                .ToolTipText(FText::FromString(*Item))
                            ]
                            // Remove Button
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(2)
                            [
                                SNew(SButton)
                                .ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
                                .ContentPadding(2)
                                .OnClicked_Lambda([this, Item]()
                                {
                                    SelectedContextPaths.Remove(Item);
                                    if(ContextListView.IsValid()) ContextListView->RequestListRefresh();
                                    return FReply::Handled();
                                })
                                [
                                    SNew(STextBlock)
                                    .Text(FText::FromString(TEXT("×")))
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                                    .ColorAndOpacity(FLinearColor::Red)
                                ]
                            ]
                        ];
                    })
                ]
            ]
            // --- End New List ---

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(2)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("AddFiles", "Add Files"))
                    .OnClicked(this, &SAICopilotWindow::OnAddFilesClicked)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(2)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("AddFolder", "Add Folder"))
                    .OnClicked(this, &SAICopilotWindow::OnAddFolderClicked)
                ]
                 + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(2)
                [
                     SNew(SButton)
                    .Text(LOCTEXT("ClearContextList", "Clear Context List"))
                    .OnClicked_Lambda([this]() {
                        SelectedContextPaths.Empty();
                        if(ContextListView.IsValid()) ContextListView->RequestListRefresh();
                        return FReply::Handled();
                    })
                ]
            ]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 5)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SBox)
					.HeightOverride(80)
					[
						SAssignNew(ContextTextBox, SMultiLineEditableTextBox) // Assign to member
						.HintText(LOCTEXT("ContextHint", "Paste text context here, or use buttons above..."))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 0, 0, 0)
				[
					// NEW: Button to Add Selection Context
					SNew(SButton)
					.Text(LOCTEXT("AddContextButton", "Get Selection"))
					.ToolTipText(LOCTEXT("AddContextTooltip", "Get selected Asset/Node text"))
					.OnClicked(this, &SAICopilotWindow::OnAddSelectionContextClicked)
				]
			]
		]

		// Prompt Input Header
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5, 5, 5, 0)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PromptLabel", "发送给 AI 的消息:"))
		]
		
		// Prompt Input Box
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5, 0, 5, 5)
		[
			SNew(SBox)
			.MinDesiredHeight(50)
			[
				SAssignNew(InputTextBox, SMultiLineEditableTextBox)
				.HintText(LOCTEXT("InputHint", "询问代码或蓝图逻辑..."))
				.OnTextCommitted(this, &SAICopilotWindow::OnTextCommitted)
			]
		]

		// Send & Control Buttons
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(2)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("SendButton", "发送"))
				.OnClicked(this, &SAICopilotWindow::OnSendClicked)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(2)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("StopButton", "停止"))
				.ToolTipText(LOCTEXT("StopTooltip", "停止生成/取消请求"))
				.IsEnabled(this, &SAICopilotWindow::CanStop)
				.OnClicked(this, &SAICopilotWindow::OnStopClicked)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(2)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("RetryButton", "重试"))
				.ToolTipText(LOCTEXT("RetryTooltip", "重新发送当前请求(删除上一条AI回复)"))
				.IsEnabled(this, &SAICopilotWindow::CanRetry)
				.OnClicked(this, &SAICopilotWindow::OnRetryClicked)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(2)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("UndoChatButton", "撤回"))
				.ToolTipText(LOCTEXT("UndoChatTooltip", "撤回最后一条用户消息和AI回复"))
				.IsEnabled(this, &SAICopilotWindow::CanUndoChat)
				.OnClicked(this, &SAICopilotWindow::OnUndoChatClicked)
			]
		]
		
		// Utility Buttons
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(2)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("GenBPButton", "生成蓝图 (Python)"))
				.OnClicked(this, &SAICopilotWindow::OnGenerateBlueprintClicked)
			]
		]
		
		// Agent Toggle
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5, 0)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
                .IsChecked_Lambda([this]() { return IsAgentModeChecked(); })
                .OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { OnAgentModeChanged(NewState); })
                .ToolTipText(LOCTEXT("AgentTooltip", "Enable Agent Mode: Automatically sends execution results back to AI until it stops."))
                [
                    SNew(STextBlock).Text(LOCTEXT("AgentMode", "Agent Mode (Auto-Reply)"))
                ]
			]
		]

		// Python Output with Undo/Redo
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PythonCodeLabel", "生成的 Python 代码:"))
				]
				
				// Code Undo/Redo Controls
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew(UndoButton, SButton) // Assign to member
					.Text(FText::FromString(TEXT("< Undo")))
					.ToolTipText(LOCTEXT("UndoCodeTooltip", "撤销代码更改"))
					.OnClicked(this, &SAICopilotWindow::OnUndoCodeClicked)
					.IsEnabled(false) // Initially disabled
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 0, 0, 0)
				[
					SAssignNew(RedoButton, SButton) // Assign to member
					.Text(FText::FromString(TEXT("Redo >")))
					.ToolTipText(LOCTEXT("RedoCodeTooltip", "重做代码更改"))
					.OnClicked(this, &SAICopilotWindow::OnRedoCodeClicked)
					.IsEnabled(false) // Initially disabled
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 5)
			[
				SNew(SBox)
				.MinDesiredHeight(120)
				[
					SAssignNew(PythonCodeBox, SMultiLineEditableTextBox)
					.IsReadOnly(false)
					.HintText(LOCTEXT("PythonCodeHint", "生成的 Python 脚本将显示在此处..."))
					.BackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f))
                    //.TextStyle(...) // Deprecated, using default style for now to avoid errors
                    //.Font(...) // Might not be available on TextBox wrapper

				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 5)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("RunPythonButton", "运行 Python 脚本"))
				.OnClicked(this, &SAICopilotWindow::OnRunPythonClicked)
			]
		]
	];
}

FReply SAICopilotWindow::OnAddFilesClicked()
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        TArray<FString> OutFilenames;
        // FileTypes: "All Files (*.*)|*.*|C++ Files (*.h;*.cpp)|*.h;*.cpp|Blueprint (*.uasset)|*.uasset"
        const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
        bool bOpened = DesktopPlatform->OpenFileDialog(
            ParentWindowHandle,
            TEXT("Select Context Files"),
            FPaths::ProjectDir(),
            TEXT(""),
            TEXT("All Files (*.*)|*.*|C++ Files (*.h;*.cpp)|*.h;*.cpp|Blueprint Files (*.uasset)|*.uasset"),
            EFileDialogFlags::Multiple,
            OutFilenames
        );

        if (bOpened && OutFilenames.Num() > 0)
        {
            for (const FString& Path : OutFilenames)
            {
                SelectedContextPaths.Add(MakeShareable(new FString(Path)));
            }
            if(ContextListView.IsValid()) ContextListView->RequestListRefresh();
        }
    }
    return FReply::Handled();
}

FReply SAICopilotWindow::OnAddFolderClicked()
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        FString OutFolderName;
        const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
        bool bOpened = DesktopPlatform->OpenDirectoryDialog(
            ParentWindowHandle,
            TEXT("Select Context Folder"),
            FPaths::ProjectDir(),
            OutFolderName
        );

        if (bOpened && !OutFolderName.IsEmpty())
        {
            SelectedContextPaths.Add(MakeShareable(new FString(OutFolderName)));
            if(ContextListView.IsValid()) ContextListView->RequestListRefresh();
        }
    }
    return FReply::Handled();
}

// --- Control Buttons ---

bool SAICopilotWindow::CanStop() const
{
    return ActiveRequest.IsValid() && ActiveRequest->GetStatus() == EHttpRequestStatus::Processing;
}

FReply SAICopilotWindow::OnStopClicked()
{
    if (ActiveRequest.IsValid())
    {
        ActiveRequest->CancelRequest();
        ActiveRequest.Reset();
        AddMessage("System", "Request cancelled by user.");
    }
    
    // Stop Agent Mode Loop
    bAgentMode = false;
    
    return FReply::Handled();
}

bool SAICopilotWindow::CanRetry() const
{
    if (ActiveRequest.IsValid() && ActiveRequest->GetStatus() == EHttpRequestStatus::Processing)
        return false;
        
    return ChatHistory.Num() > 0;
}

FReply SAICopilotWindow::OnRetryClicked()
{
    if (ChatHistory.Num() == 0) return FReply::Handled();

    // Strategy: Remove the last response (AI/System) if present, then get the last user message,
    // put it back in the input box, and trigger Send.

    FAIChatMessage LastMsg = ChatHistory.Last();
    
    // 1. If last was AI, remove it first
    if (LastMsg.Role == "AI" || LastMsg.Role == "Assistant" || LastMsg.Role == "System")
    {
        ChatHistory.Pop();
        
        // Refresh UI
        if(ChatScrollBox.IsValid()) ChatScrollBox->ClearChildren();
        for(const auto& Msg : ChatHistory) CreateChatBubble(Msg.Role, Msg.Content);
    }
    
    // 2. Now check if we have a User message at the end
    if (ChatHistory.Num() > 0)
    {
        FAIChatMessage UserMsg = ChatHistory.Last();
        if (UserMsg.Role == "User")
        {
            // Found it. Extract content, remove from history, put to input, and click Send.
            FString Content = UserMsg.Content;
            ChatHistory.Pop(); 
            
            // Refresh UI again to remove the user message we are about to re-send
            if(ChatScrollBox.IsValid()) ChatScrollBox->ClearChildren();
            for(const auto& Msg : ChatHistory) CreateChatBubble(Msg.Role, Msg.Content);

            if (InputTextBox.IsValid())
            {
                InputTextBox->SetText(FText::FromString(Content));
                return OnSendClicked(); // Simulate Send Click
            }
        }
    }
    
    return FReply::Handled();
}

bool SAICopilotWindow::CanUndoChat() const
{
    return ChatHistory.Num() > 0 && (!ActiveRequest.IsValid() || ActiveRequest->GetStatus() != EHttpRequestStatus::Processing);
}

FReply SAICopilotWindow::OnUndoChatClicked()
{
    if (ChatHistory.Num() == 0) return FReply::Handled();

    // 1. Remove last message
    FAIChatMessage Last = ChatHistory.Pop();
    
    // 2. If it was AI, and there's a preceding User message, likely remove that too to restore state
    if (Last.Role == "AI" || Last.Role == "Assistant" || Last.Role == "System")
    {
        if (ChatHistory.Num() > 0 && ChatHistory.Last().Role == "User")
        {
            FAIChatMessage UserMsg = ChatHistory.Pop();
            // Restore text to input box for convenience
            if (InputTextBox.IsValid())
            {
                InputTextBox->SetText(FText::FromString(UserMsg.Content));
            }
        }
    }
    else if (Last.Role == "User")
    {
        // Restore text if we just undid a user message (e.g. before AI replied)
        if (InputTextBox.IsValid())
        {
             InputTextBox->SetText(FText::FromString(Last.Content));
        }
    }

    // 3. Refresh UI
    if(ChatScrollBox.IsValid())
    {
        ChatScrollBox->ClearChildren();
        for(const auto& Msg : ChatHistory) CreateChatBubble(Msg.Role, Msg.Content);
    }
    
    // DO NOT AddMessage("System"...) here, as it would re-clutter the history we just cleaned.

    return FReply::Handled();
}

// ------------------
FReply SAICopilotWindow::OnSendClicked()
{
	const UAICopilotSettings* Settings = GetDefault<UAICopilotSettings>();
	FString ApiEndpoint = Settings->ApiUrl;
	FString ApiKey = Settings->ApiKey;
	FString ModelName = Settings->ModelName;

	FString UserInput = InputTextBox->GetText().ToString();
	if (UserInput.IsEmpty()) return FReply::Handled();

	// Only require API Key if not using Ollama (Ollama often runs without auth locally)
	if (ApiKey.IsEmpty() && Settings->ModelProvider != EAICopilotModelProvider::Ollama)
	{
		AddMessage("System", "Please configure your API Key in Settings.");
		return FReply::Handled();
	}

	InputTextBox->SetText(FText::GetEmpty());

	// Construct HTTP Request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	
	Request->OnProcessRequestComplete().BindSP(this, &SAICopilotWindow::OnResponseReceived);
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");

    // Track active request
    ActiveRequest = Request;

	// Add the user message to history NOW
	AddMessage("User", UserInput);

	// Create JSON Payload
	TSharedPtr<FJsonObject> JsonRequest = MakeShareable(new FJsonObject);
	FString RequestBody;

    // --- CONTEXT GATHERING ---
    // 1. Text Box Context
    FString SystemContext;
    if (ContextTextBox.IsValid() && !ContextTextBox->GetText().IsEmpty())
    {
        SystemContext += TEXT("Text Context:\n") + ContextTextBox->GetText().ToString() + TEXT("\n\n");
    }
    
    // 2. File Context
    for(auto Item : SelectedContextPaths)
    {
        FString FilePath = *Item;
        if(FPaths::FileExists(FilePath))
        {
             // Check extension
             FString Ext = FPaths::GetExtension(FilePath).ToLower();
             if(Ext == "cpp" || Ext == "h" || Ext == "cs")
             {
                 FString Content;
                 if(FFileHelper::LoadFileToString(Content, *FilePath))
                 {
                     SystemContext += FString::Printf(TEXT("File Context (%s):\n```cpp\n%s\n```\n\n"), *FPaths::GetCleanFilename(FilePath), *Content);
                 }
             }
             else if (Ext == "uasset")
             {
                 // Convert file path to package path
                 FString PackageName;
                 if (FPackageName::TryConvertFilenameToLongPackageName(FilePath, PackageName))
                 {
                     // Check if it's a Blueprint and inspect nodes directly in C++
                     // This is much more reliable than Python introspection for node details.
                     UPackage* Package = LoadPackage(nullptr, *PackageName, LOAD_None);
                     if (Package)
                     {
                         UObject* Asset = Package->FindAssetInPackage();
                         if (Asset && Asset->IsA<UBlueprint>())
                         {
                             // Explicitly case to UBlueprint to access its graphs
                             UBlueprint* BP = Cast<UBlueprint>(Asset);
                             
                             // 1. Basic Info
                             TSharedPtr<FJsonObject> BPInfo = MakeShareable(new FJsonObject);
                             BPInfo->SetStringField("name", Asset->GetName());
                             BPInfo->SetStringField("path", PackageName);
                             BPInfo->SetStringField("parent_class", BP->ParentClass ? BP->ParentClass->GetName() : TEXT("None"));
                             
                             // 2. Graphs & Nodes (Limited Recursive Dump)
                             TArray<TSharedPtr<FJsonValue>> GraphsJson;
                             
                             TArray<UEdGraph*> Graphs;
                             // Helper to get all graphs including function graphs and macros
                             Graphs = BP->UbergraphPages;
                             Graphs.Append(BP->FunctionGraphs);
                             Graphs.Append(BP->MacroGraphs);

                             for (UEdGraph* Graph : Graphs)
                             {
                                 TSharedPtr<FJsonObject> GraphObj = MakeShareable(new FJsonObject);
                                 GraphObj->SetStringField("name", Graph->GetName());
                                 GraphObj->SetStringField("type", Graph->GetClass()->GetName());
                                 
                                 TArray<TSharedPtr<FJsonValue>> NodesJson;
                                 for (UEdGraphNode* Node : Graph->Nodes)
                                 {
                                     TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject);
                                     NodeObj->SetStringField("title", Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
                                     NodeObj->SetStringField("class", Node->GetClass()->GetName());
                                     
                                     // Pins logic
                                     TArray<TSharedPtr<FJsonValue>> PinsJson;
                                     for (UEdGraphPin* Pin : Node->Pins)
                                     {
                                         // Skip hidden pins to reduce noise
                                         if (Pin->bHidden) continue;

                                         TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
                                         PinObj->SetStringField("name", Pin->PinName.ToString());
                                         
                                         // For input pins, show default value
                                         if (Pin->Direction == EGPD_Input && !Pin->DefaultValue.IsEmpty())
                                         {
                                             PinObj->SetStringField("val", Pin->DefaultValue);
                                         }
                                         
                                         // For output pins, show connections (Logic Flow)
                                         if (Pin->Direction == EGPD_Output)
                                         {
                                             if (Pin->LinkedTo.Num() > 0)
                                             {
                                                 TArray<TSharedPtr<FJsonValue>> Links;
                                                 for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                                                 {
                                                     if (LinkedPin)
                                                     {
                                                         UEdGraphNode* ConnectedNode = LinkedPin->GetOwningNode();
                                                         FString TargetTitle = ConnectedNode->GetNodeTitle(ENodeTitleType::ListView).ToString();
                                                         
                                                         if (TargetTitle.IsEmpty()) TargetTitle = ConnectedNode->GetClass()->GetName();
                                                         
                                                         Links.Add(MakeShareable(new FJsonValueString(
                                                             FString::Printf(TEXT("%s -> [%s]"), *Pin->PinName.ToString(), *TargetTitle)
                                                         )));
                                                     }
                                                 }
                                                 PinObj->SetArrayField("links", Links);
                                                 PinsJson.Add(MakeShareable(new FJsonValueObject(PinObj)));
                                             }
                                             // If it's an Output EXEC pin that is NOT connected, include it so AI knows flow stops here
                                             else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
                                             {
                                                 PinsJson.Add(MakeShareable(new FJsonValueObject(PinObj)));
                                             }
                                         }
                                         else if (Pin->Direction == EGPD_Input)
                                         {
                                              if (!Pin->DefaultValue.IsEmpty())
                                              {
                                                  PinObj->SetStringField("val", Pin->DefaultValue);
                                                  PinsJson.Add(MakeShareable(new FJsonValueObject(PinObj))); 
                                              }
                                              // Exec Input pins are implied by being a node, but maybe useful for entry points
                                              else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
                                              {
                                                   // Empty body to skip
                                              }
                                         }
                                     }
                                     NodeObj->SetArrayField("pins", PinsJson);
                                     NodesJson.Add(MakeShareable(new FJsonValueObject(NodeObj)));
                                 }
                                 GraphObj->SetArrayField("nodes", NodesJson);
                                 GraphsJson.Add(MakeShareable(new FJsonValueObject(GraphObj)));
                             }
                             BPInfo->SetArrayField("graphs", GraphsJson);
                             
                             // Serialize to String
                             FString OutputString;
                             TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
                             FJsonSerializer::Serialize(BPInfo.ToSharedRef(), Writer);
                             
                             SystemContext += FString::Printf(TEXT("Asset Context (%s):\nBlueprint Structure (JSON):\n%s\n\n"), *FPaths::GetCleanFilename(FilePath), *OutputString);
                         }
                         else
                         {
                             // Fallback for non-Blueprints or if failed to load properly
                             SystemContext += FString::Printf(TEXT("Asset Context (%s):\nPath: %s\nType: %s\n(Non-Blueprint Asset)\n\n"), *FPaths::GetCleanFilename(FilePath), *PackageName, Asset ? *Asset->GetClass()->GetName() : TEXT("Unknown"));
                         }
                     }
                 }
             }
        }
        else if (FPaths::DirectoryExists(FilePath))
        {
             SystemContext += FString::Printf(TEXT("Folder Context (%s): User is asking about files in this folder. (Content reading skipped for performance)\n"), *FilePath);
        }
    }
    // -------------------------

	// Handle Google Gemini Native API
	if (Settings->ModelProvider == EAICopilotModelProvider::GeminiNative)
	{
        // ... (Keep existing Gemini Native code, ensuring SystemContext is prepended)
		// 1. URL with Key
		FString UrlWithKey = FString::Printf(TEXT("%s?key=%s"), *ApiEndpoint, *ApiKey);
		Request->SetURL(UrlWithKey);
		
		TArray<TSharedPtr<FJsonValue>> ContentsJson;

		if (ContextTextBox.IsValid() && !ContextTextBox->GetText().IsEmpty())
		{
            // Note: We already built SystemContext above, just need to use it
		}

		for (int32 i = 0; i < ChatHistory.Num(); i++)
		{
			const FAIChatMessage& Msg = ChatHistory[i];
			
			TSharedPtr<FJsonObject> TurnObj = MakeShareable(new FJsonObject);
			FString Role = (Msg.Role.ToLower() == "assistant" || Msg.Role.ToLower() == "ai") ? "model" : "user";
			TurnObj->SetStringField("role", Role);

			FString ContentStr = Msg.Content;
			// Prepend context to the very first user message 
			if (i == 0 && Role == "user" && !SystemContext.IsEmpty())
			{
				ContentStr = SystemContext + ContentStr;
				SystemContext.Empty(); 
			}

			TArray<TSharedPtr<FJsonValue>> Parts;
			TSharedPtr<FJsonObject> PartObj = MakeShareable(new FJsonObject);
			PartObj->SetStringField("text", ContentStr);
			Parts.Add(MakeShareable(new FJsonValueObject(PartObj)));

			TurnObj->SetArrayField("parts", Parts);
			ContentsJson.Add(MakeShareable(new FJsonValueObject(TurnObj)));
		}
		
		JsonRequest->SetArrayField("contents", ContentsJson);
	}
	else 
	{
		// Standard OpenAI Format
		Request->SetURL(ApiEndpoint);
		if (!ApiKey.IsEmpty())
		{
			Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *ApiKey));
		}

		JsonRequest->SetStringField("model", ModelName);
		
		TArray<TSharedPtr<FJsonValue>> MessagesJson;

		// System/Context
        // We merged all contexts into SystemContext variable
		if (!SystemContext.IsEmpty())
		{
			TSharedPtr<FJsonObject> SysMsg = MakeShareable(new FJsonObject);
			SysMsg->SetStringField("role", "system");
			SysMsg->SetStringField("content", SystemContext);
			MessagesJson.Add(MakeShareable(new FJsonValueObject(SysMsg)));
		}

        // History
		for (const FAIChatMessage& Msg : ChatHistory)
		{
			TSharedPtr<FJsonObject> MsgObj = MakeShareable(new FJsonObject);
			MsgObj->SetStringField("role", Msg.Role.ToLower());
			MsgObj->SetStringField("content", Msg.Content);
			MessagesJson.Add(MakeShareable(new FJsonValueObject(MsgObj)));
		}
		
		JsonRequest->SetArrayField("messages", MessagesJson);
	}

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonRequest.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();

	return FReply::Handled();
}

FReply SAICopilotWindow::OnSettingsClicked()
{
	TSharedRef<SWindow> SettingsWindow = SNew(SWindow)
		.Title(LOCTEXT("SettingsWindowTitle", "AI Copilot Settings"))
		.ClientSize(FVector2D(400, 200))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TSharedRef<SAICopilotSettings> SettingsWidget = SNew(SAICopilotSettings);
	SettingsWidget->SetParentWindow(SettingsWindow);

	SettingsWindow->SetContent(SettingsWidget);

	FSlateApplication::Get().AddWindow(SettingsWindow);

	return FReply::Handled();
}

void SAICopilotWindow::OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		OnSendClicked();
	}
}

void SAICopilotWindow::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    ActiveRequest.Reset();

	if (!bWasSuccessful || !Response.IsValid())
	{
		AddMessage("System", "Error: Request failed.");
		bAgentMode = false;
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		AddMessage("System", FString::Printf(TEXT("Error %d: %s"), Response->GetResponseCode(), *Response->GetContentAsString()));
		bAgentMode = false;
		return;
	}

	TSharedPtr<FJsonObject> JsonResponse;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonResponse))
	{
		FString Content;
		
		// Check for OpenAI / Gemini (OpenAI) format
		const TArray<TSharedPtr<FJsonValue>>* Choices;
		if (JsonResponse->TryGetArrayField("choices", Choices) && Choices->Num() > 0)
		{
			TSharedPtr<FJsonObject> Choice = (*Choices)[0]->AsObject();
			TSharedPtr<FJsonObject> Message = Choice->GetObjectField("message");
			Content = Message->GetStringField("content");
		}
		// Check for Gemini Native Format
        else 
        {
             const TArray<TSharedPtr<FJsonValue>>* Candidates;
             if (JsonResponse->TryGetArrayField("candidates", Candidates) && Candidates->Num() > 0)
             {
                 TSharedPtr<FJsonObject> Candidate = (*Candidates)[0]->AsObject();
                 TSharedPtr<FJsonObject> ContentObj = Candidate->GetObjectField("content");
                 const TArray<TSharedPtr<FJsonValue>>* Parts;
                 if (ContentObj->TryGetArrayField("parts", Parts) && Parts->Num() > 0)
                 {
                      TSharedPtr<FJsonObject> Part = (*Parts)[0]->AsObject();
                      Content = Part->GetStringField("text");
                 }
             }
        }
		
		if (!Content.IsEmpty())
		{
			AddMessage("AI", Content);

			if (bAgentMode)
			{
				// Extract Python Code from Markdown
				FString PythonCode;
				int32 StartIdx = Content.Find("```python");
				if (StartIdx == INDEX_NONE) StartIdx = Content.Find("```");
				
				if (StartIdx != INDEX_NONE)
				{
					// Find end of block (checking after the start tag)
					int32 BlockStart = StartIdx + (Content.Find("python", ESearchCase::IgnoreCase, ESearchDir::FromStart, StartIdx) != INDEX_NONE ? 9 : 3);
					// Skip newline if present
					if (BlockStart < Content.Len() && Content[BlockStart] == '\n') BlockStart++;

					int32 EndIdx = Content.Find("```", ESearchCase::IgnoreCase, ESearchDir::FromStart, BlockStart);
					
					if (EndIdx != INDEX_NONE)
					{
						PythonCode = Content.Mid(BlockStart, EndIdx - BlockStart);
					}
					else
					{
						PythonCode = Content.Mid(BlockStart); // Take rest if no closing tag found (handling truncation or streaming)
					}
				}
				
				if (!PythonCode.IsEmpty())
				{
					if (PythonCodeBox.IsValid())
					{
						PythonCodeBox->SetText(FText::FromString(PythonCode));
					}
					
					// Auto-Run the code
					OnRunPythonClicked();
				}
				else
				{
					// No code found, stop agent mode? Or let it hallucinate?
					// Usually if AI replies without code, it's asking a question or done.
					// Let's stop to be safe.
					// But sometimes it explains first.
					// We'll check if "Task Complete" or similar is in text?
					// For now, if no code, keep it running? No, that might loop forever if it just chats.
					// Let's stop if no code block.
					bAgentMode = false;
					AddMessage("System", "Agent specific: No code block found. Stopping Agent Mode.");
				}
			}
		}
	}
}

void SAICopilotWindow::OnBlueprintResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		AddMessage("System", "Error: BP Generation Request failed.");
		return;
	}

	if (Response->GetResponseCode() != 200)
	{
		AddMessage("System", FString::Printf(TEXT("Error %d: %s"), Response->GetResponseCode(), *Response->GetContentAsString()));
		return;
	}

	TSharedPtr<FJsonObject> JsonResponse;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonResponse))
	{
		const TArray<TSharedPtr<FJsonValue>>* Choices;
		if (JsonResponse->TryGetArrayField("choices", Choices) && Choices->Num() > 0)
		{
			TSharedPtr<FJsonObject> Choice = (*Choices)[0]->AsObject();
			TSharedPtr<FJsonObject> Message = Choice->GetObjectField("message");
			FString Content = Message->GetStringField("content");

			// Extract Python Code from Markdown
			FString PythonCode;
			int32 StartIdx = Content.Find("```python");
			if (StartIdx == INDEX_NONE) StartIdx = Content.Find("```");
			
			if (StartIdx != INDEX_NONE)
			{
				int32 EndIdx = Content.Find("```", ESearchCase::IgnoreCase, ESearchDir::FromStart, StartIdx + 8); // Skip ```python
				if (EndIdx != INDEX_NONE)
				{
					// Extract
					int32 CodeStart = StartIdx + (Content.Find("python", ESearchCase::IgnoreCase, ESearchDir::FromStart, StartIdx) != INDEX_NONE ? 9 : 3);
					// Skip newline if present
					if (CodeStart < Content.Len() && Content[CodeStart] == '\n') CodeStart++;
					
					PythonCode = Content.Mid(CodeStart, EndIdx - CodeStart);
				}
				else
				{
					PythonCode = Content; // Fallback
				}
			}
			else
			{
				PythonCode = Content;
			}
			
			AddMessage("AI", "Generated Python Script. Review code below and click 'Run'.");
			SetPythonCode(PythonCode);
		}
	}
}

FReply SAICopilotWindow::OnGenerateBlueprintClicked()
{
	FString UserPrompt = InputTextBox->GetText().ToString();
	if (UserPrompt.IsEmpty()) return FReply::Handled();

	const UAICopilotSettings* Settings = GetDefault<UAICopilotSettings>();
	if (Settings->ApiKey.IsEmpty() && Settings->ModelProvider != EAICopilotModelProvider::Ollama)
	{
		AddMessage("System", "Please set API Key in settings.");
		return FReply::Handled();
	}

	AddMessage("User", "Generating Blueprint for: " + UserPrompt);
	// Clear the input box after reading the prompt
	InputTextBox->SetText(FText::GetEmpty());

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindSP(this, &SAICopilotWindow::OnBlueprintResponseReceived);
	Request->SetURL(Settings->ApiUrl);
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");
	if (!Settings->ApiKey.IsEmpty())
	{
		Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *Settings->ApiKey));
	}

	TSharedPtr<FJsonObject> JsonRequest = MakeShareable(new FJsonObject);
	JsonRequest->SetStringField("model", Settings->ModelName);

	TArray<TSharedPtr<FJsonValue>> MessagesJson;

	// System Prompt
	TSharedPtr<FJsonObject> SysMsg = MakeShareable(new FJsonObject);
	SysMsg->SetStringField("role", "system");
	SysMsg->SetStringField("content", "You are an expert Unreal Engine 5 Technical Artist. Write a Python script using the `unreal` module to create the requested Blueprint/Asset. Return valid Python code inside ```python blocks. Ensure the code handles asset creation and saving.");
	MessagesJson.Add(MakeShareable(new FJsonValueObject(SysMsg)));

	// User Prompt
	TSharedPtr<FJsonObject> UserMsg = MakeShareable(new FJsonObject);
	UserMsg->SetStringField("role", "user");
	UserMsg->SetStringField("content", UserPrompt);
	MessagesJson.Add(MakeShareable(new FJsonValueObject(UserMsg)));

	JsonRequest->SetArrayField("messages", MessagesJson);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonRequest.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();

	return FReply::Handled();
}

void SAICopilotWindow::OnAgentModeChanged(ECheckBoxState NewState)
{
    bAgentMode = (NewState == ECheckBoxState::Checked);
}

ECheckBoxState SAICopilotWindow::IsAgentModeChecked() const
{
    return bAgentMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FReply SAICopilotWindow::OnRunPythonClicked()
{
	FString Code = PythonCodeBox->GetText().ToString();
	if (Code.IsEmpty()) return FReply::Handled();

	IPythonScriptPlugin* PythonPlugin = IPythonScriptPlugin::Get();
	if (PythonPlugin && PythonPlugin->IsPythonAvailable())
	{
		// 1. Prepare Code for Injection (Safe for Printf and Indentation)
        FString IndentedCode = Code;
        // Escape % first to avoid conflicts with Printf
        IndentedCode.ReplaceInline(TEXT("%"), TEXT("%%"));
        // Indent lines for try-except block
        IndentedCode.ReplaceInline(TEXT("\n"), TEXT("\n    ")); 
        IndentedCode = TEXT("    ") + IndentedCode;

        FString OutputLogPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AgentExecLog.txt"));
        OutputLogPath.ReplaceInline(TEXT("\\"), TEXT("/"));

        // Python wrapper to redirect stdout/stderr
        FString WrappedCode = FString::Printf(TEXT(
R"(
import sys
import io
import traceback
import unreal

# Setup Capture
capture_io = io.StringIO()
original_stdout = sys.stdout
sys.stdout = capture_io

# Execute User Code
try:
%s
except Exception:
    print("\nExecution Exception:")
    traceback.print_exc()

# Restore Stdout
sys.stdout = original_stdout

# Read Captured Output
captured_output = capture_io.getvalue()

# Write to Log File
with open('%s', 'w', encoding='utf-8') as f:
    f.write(captured_output)
)"), *IndentedCode, *OutputLogPath);

        PythonPlugin->ExecPythonCommand(*WrappedCode);

        // 2. Read Output Log
        FString ExecutionResult;
        if(FFileHelper::LoadFileToString(ExecutionResult, *OutputLogPath))
        {
             // Log to Chat History
             if (!ExecutionResult.IsEmpty())
             {
                 FString SystemResult = FString::Printf(TEXT("System [Execution Result]:\n%s"), *ExecutionResult);
                 AddMessage("System", SystemResult);
                 
                 // 3. Auto-Reply if Agent Mode
                 if(bAgentMode)
                 {
                     SendDirectMessage("System", SystemResult);
                 }
             }
             else
             {
                 AddMessage("System", "Executed successfully (No Output).");
                 if(bAgentMode)
                 {
                     SendDirectMessage("System", "Executed successfully (No Output).");
                 }
             }
        }
        else
        {
             AddMessage("System", "Executed, but failed to read output log.");
             bAgentMode = false;
        }
	}
	else
	{
		AddMessage("System", "Error: PythonScriptPlugin not available or Python not initialized.");
		bAgentMode = false;
	}
	
	return FReply::Handled();
}

void SAICopilotWindow::SetPythonCode(const FString& Code)
{
	if (PythonCodeBox.IsValid())
	{
		PythonCodeBox->SetText(FText::FromString(Code));
		PushCodeHistory(Code);
	}
}

void SAICopilotWindow::AppendContext(const FString& NewContext)
{
	if (ContextTextBox.IsValid())
	{
		FString Current = ContextTextBox->GetText().ToString();
		if (!Current.IsEmpty())
		{
			Current += TEXT("\n\n");
		}
		Current += NewContext;
		ContextTextBox->SetText(FText::FromString(Current));
	}
}

FReply SAICopilotWindow::OnAddSelectionContextClicked()
{
	FString SelectedContext;
	bool bFoundSelection = false;

	// 1. Get Selected Actors (Level Editor)
	if (GEditor)
	{
		USelection* SelectedActors = GEditor->GetSelectedActors();
		if (SelectedActors && SelectedActors->Num() > 0)
		{
			SelectedContext += TEXT("Selected Level Actors:\n");
			for (FSelectionIterator It(*SelectedActors); It; ++It)
			{
				if (AActor* Actor = Cast<AActor>(*It))
				{
					SelectedContext += FString::Printf(TEXT("- %s (Class: %s) Location: %s\n"), 
						*Actor->GetActorLabel(), 
						*Actor->GetClass()->GetName(),
						*Actor->GetActorLocation().ToString());
					bFoundSelection = true;
				}
			}
			SelectedContext += TEXT("\n");
		}
	
		// 2. Get Selected Objects (Content Browser items are often here)
		USelection* SelectedObjects = GEditor->GetSelectedObjects();
		if (SelectedObjects && SelectedObjects->Num() > 0)
		{
			// Filter out actors we already logged
			bool bHeaderAdded = false;
			for (FSelectionIterator It(*SelectedObjects); It; ++It)
			{
				UObject* Obj = *It;
				if (Obj && !Obj->IsA<AActor>()) // Skip actors
				{
					if (!bHeaderAdded)
					{
						SelectedContext += TEXT("Selected Objects/Assets:\n");
						bHeaderAdded = true;
					}
					
					SelectedContext += FString::Printf(TEXT("- %s (Class: %s) Path: %s\n"), 
						*Obj->GetName(), 
						*Obj->GetClass()->GetName(),
						*Obj->GetPathName());
					bFoundSelection = true;
				}
			}
			if (bHeaderAdded) SelectedContext += TEXT("\n");
		}
	}
	
	// 3. Try to get Graph Nodes via Copy Buffer
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
	if (ClipboardContent.Contains("Begin Object") && (ClipboardContent.Contains("Class=/Script/BlueprintGraph") || ClipboardContent.Contains("EdGraphNode")))
	{
		 SelectedContext += TEXT("Clipboard Content (Potential Nodes):\n");
		 SelectedContext += ClipboardContent.Left(2000); // Limit size
		 if (ClipboardContent.Len() > 2000) SelectedContext += TEXT("... (Truncated)");
		 SelectedContext += TEXT("\n");
		 bFoundSelection = true;
	}

	if (!bFoundSelection)
	{
		SelectedContext = TEXT("[No Selection found. Copy Blueprint Nodes (Ctrl+C) to context]");
	}

	AppendContext(SelectedContext);
	return FReply::Handled();
}

void SAICopilotWindow::AddMessage(const FString& Role, const FString& Content)
{
	// Ensure role for history is correct (User/AI -> user/assistant)
	FString ApiRole = (Role == "User") ? "user" : "assistant";
	
	// Only add to history if not already there (simple check, or just always add)
	// We call AddMessage for User inputs and AI responses.
	ChatHistory.Add({ApiRole, Content});

	// Use CreateChatBubble for visualization
	CreateChatBubble(Role, Content);
}

void SAICopilotWindow::CreateChatBubble(const FString& InRole, const FString& Content)
{
	bool bIsUser = (InRole == "User");
	
	// Chat Bubble Widget
	TSharedPtr<SWidget> ChatBubble;

    auto MessageTextBlock = SNew(SMultiLineEditableTextBox)
        .Text(FText::FromString(Content))
        .IsReadOnly(true)
        .AutoWrapText(true)
        .BackgroundColor(FLinearColor::Transparent)
        .TextStyle(FAppStyle::Get(), "NormalText");

    // Start with a generic border
    auto BubbleBorder = SNew(SBorder)
        .Padding(FMargin(10))
        .BorderImage(FAppStyle::GetBrush(bIsUser ? "ToolPanel.GroupBorder" : "Brushes.Panel"));
		
	if (bIsUser)
	{
		BubbleBorder->SetBorderBackgroundColor(FLinearColor(0.2f, 0.4f, 0.8f)); // User Blue
	}
	else
	{
		BubbleBorder->SetBorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f)); // AI Dark
	}

    BubbleBorder->SetContent(MessageTextBlock);

	ChatScrollBox->AddSlot()
	.Padding(5)
	.HAlign(bIsUser ? HAlign_Right : HAlign_Left) // Alignment based on role
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(InRole))
			.ColorAndOpacity(FLinearColor::Gray)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.Justification(bIsUser ? ETextJustify::Right : ETextJustify::Left)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 2)
		[
            SNew(SBox)
            .MaxDesiredWidth(600) // Limit width of bubble
            [
			    BubbleBorder
            ]
		]
	];
	
	ChatScrollBox->ScrollToEnd();
}

FReply SAICopilotWindow::OnClearChatClicked()
{
	ChatHistory.Empty();
	if(ChatScrollBox.IsValid())
	{
		ChatScrollBox->ClearChildren();
	}
	AddMessage("System", "Chat cleared.");
	return FReply::Handled();
}

FReply SAICopilotWindow::OnSaveChatHistoryClicked()
{
    FString FilePath = FPaths::ProjectSavedDir() / TEXT("AICopilot_ChatHistory.txt");
    FString OutputString;
    
    for(const auto& Msg : ChatHistory)
    {
        OutputString += FString::Printf(TEXT("[%s]: %s\n\n"), *Msg.Role, *Msg.Content);
    }
    
    if(FFileHelper::SaveStringToFile(OutputString, *FilePath))
    {
        AddMessage("System", FString::Printf(TEXT("Chat history saved to: %s"), *FilePath));
        FPlatformProcess::ExploreFolder(*FilePath);
    }
    else
    {
        AddMessage("System", "Failed to save chat history.");
    }

	return FReply::Handled();
}

void SAICopilotWindow::PushCodeHistory(const FString& NewCode)
{
	// If we are not at the end of the history (i.e. we undid some steps), remove the future steps
	if(CurrentCodeIndex < CodeHistory.Num() - 1)
	{
		CodeHistory.SetNum(CurrentCodeIndex + 1);
	}
    
    // push new code
    CodeHistory.Add(NewCode);
    CurrentCodeIndex++;
    
    UpdateUndoRedoState();
}

FReply SAICopilotWindow::OnUndoCodeClicked()
{
   if(CurrentCodeIndex > 0)
   {
       CurrentCodeIndex--;
       FString PreviousCode = CodeHistory[CurrentCodeIndex];
       
       // Update UI without pushing to history again
       if(PythonCodeBox.IsValid())
       {
           PythonCodeBox->SetText(FText::FromString(PreviousCode));
       }
       
       UpdateUndoRedoState();
   }
   return FReply::Handled();
}

FReply SAICopilotWindow::OnRedoCodeClicked()
{
    if(CurrentCodeIndex < CodeHistory.Num() - 1)
    {
        CurrentCodeIndex++;
        FString NextCode = CodeHistory[CurrentCodeIndex];
        
        if(PythonCodeBox.IsValid())
        {
             PythonCodeBox->SetText(FText::FromString(NextCode));
        }
        
        UpdateUndoRedoState();
    }
    return FReply::Handled();
}

void SAICopilotWindow::UpdateUndoRedoState()
{
    if(UndoButton.IsValid())
    {
        UndoButton->SetEnabled(CurrentCodeIndex > 0);
    }
    if(RedoButton.IsValid())
    {
        RedoButton->SetEnabled(CurrentCodeIndex < CodeHistory.Num() - 1);
    }
}

// Helper to send messages without user typing
void SAICopilotWindow::SendDirectMessage(const FString& InRole, const FString& InContent)
{
	const UAICopilotSettings* Settings = GetDefault<UAICopilotSettings>();
	FString ApiEndpoint = Settings->ApiUrl;
	FString ApiKey = Settings->ApiKey;
	FString ModelName = Settings->ModelName;

    // Use current settings
	if (ApiKey.IsEmpty() && Settings->ModelProvider != EAICopilotModelProvider::Ollama)
	{
		AddMessage("System", "Agent Stop: No API Key.");
		bAgentMode = false;
        if(ContextListView.IsValid()) ContextListView->RequestListRefresh(); // Refresh UI if bound
		return;
	}

	// Message is already added to History by OnRunPythonClicked -> AddMessage("System"...)
    // Wait, AddMessage adds to UI and ChatHistory.
    // If we call SendDirectMessage("System", ...), we might double add if not careful.
    // The previous code in OnRunPythonClicked calls AddMessage("System", Result).
    // So here we should NOT add to history again, just trigger the request.
    // BUT, SendDirectMessage implies "Send what is in history plus this new thing".
    
    // Actually, OnRunPythonClicked calls AddMessage("System", ...).
    // So ChatHistory is UPDATED.
    // We just need to trigger the API call.
    
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindSP(this, &SAICopilotWindow::OnResponseReceived);
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");

    // Track active request for Stop button
    ActiveRequest = Request;

	// Create JSON Payload
	TSharedPtr<FJsonObject> JsonRequest = MakeShareable(new FJsonObject);
	FString RequestBody;

    // --- CONTEXT GATHERING (Reuse from OnSendClicked - Should refactor this into GetContextString()) ---
    FString SystemContext;
    if (ContextTextBox.IsValid() && !ContextTextBox->GetText().IsEmpty())
    {
        SystemContext += TEXT("Text Context:\n") + ContextTextBox->GetText().ToString() + TEXT("\n\n");
    }
    // File Context (Simplified re-fetch or assume it's same session)
    // Refactoring this is risky for now, let's duplicate the context block for safety or just omit file re-reading if not needed.
    // Ideally Agent loop remembers context via history, but System Prompt is needed.
    // Let's copy the file reading block for consistency.
    for(auto Item : SelectedContextPaths)
    {
        FString FilePath = *Item;
        if(FPaths::FileExists(FilePath))
        {
             FString Ext = FPaths::GetExtension(FilePath).ToLower();
             if(Ext == "cpp" || Ext == "h" || Ext == "cs")
             {
                 FString Content;
                 if(FFileHelper::LoadFileToString(Content, *FilePath))
                     SystemContext += FString::Printf(TEXT("File Context (%s):\n```cpp\n%s\n```\n\n"), *FPaths::GetCleanFilename(FilePath), *Content);
             }
             // Note: Blueprint context is added when sending based on selection or file list
             // For simplicity, we skip re-parsing huge BPs in the loop to save tokens unless strictly necessary, 
             // OR we should maintain it. The user expectation is full context.
             // We will assume file context is static for the session.
        }
    }
    // -------------------------

	// Handle Google Gemini Native API
	if (Settings->ModelProvider == EAICopilotModelProvider::GeminiNative)
	{
		FString UrlWithKey = FString::Printf(TEXT("%s?key=%s"), *ApiEndpoint, *ApiKey);
		Request->SetURL(UrlWithKey);
		
		TArray<TSharedPtr<FJsonValue>> ContentsJson;

		for (int32 i = 0; i < ChatHistory.Num(); i++)
		{
			const FAIChatMessage& Msg = ChatHistory[i];
			
			TSharedPtr<FJsonObject> TurnObj = MakeShareable(new FJsonObject);
			FString Role = (Msg.Role.ToLower() == "assistant" || Msg.Role.ToLower() == "ai") ? "model" : "user";
            if (Msg.Role.ToLower() == "system") Role = "user"; // Map system msgs (results) to user for Gemini

			TurnObj->SetStringField("role", Role);

			FString ContentStr = Msg.Content;
			if (i == 0 && Role == "user" && !SystemContext.IsEmpty())
			{
				ContentStr = SystemContext + ContentStr;
			}

			TArray<TSharedPtr<FJsonValue>> Parts;
			TSharedPtr<FJsonObject> PartObj = MakeShareable(new FJsonObject);
			PartObj->SetStringField("text", ContentStr);
			Parts.Add(MakeShareable(new FJsonValueObject(PartObj)));

			TurnObj->SetArrayField("parts", Parts);
			ContentsJson.Add(MakeShareable(new FJsonValueObject(TurnObj)));
		}
		
		JsonRequest->SetArrayField("contents", ContentsJson);
	}
	else 
	{
		// Standard OpenAI Format
		Request->SetURL(ApiEndpoint);
		if (!ApiKey.IsEmpty())
		{
			Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *ApiKey));
		}

		JsonRequest->SetStringField("model", ModelName);
		
		TArray<TSharedPtr<FJsonValue>> MessagesJson;

		if (!SystemContext.IsEmpty())
		{
			TSharedPtr<FJsonObject> SysMsg = MakeShareable(new FJsonObject);
			SysMsg->SetStringField("role", "system");
			SysMsg->SetStringField("content", SystemContext);
			MessagesJson.Add(MakeShareable(new FJsonValueObject(SysMsg)));
		}

		for (const FAIChatMessage& Msg : ChatHistory)
		{
			TSharedPtr<FJsonObject> MsgObj = MakeShareable(new FJsonObject);
			MsgObj->SetStringField("role", Msg.Role.ToLower());
			MsgObj->SetStringField("content", Msg.Content);
			MessagesJson.Add(MakeShareable(new FJsonValueObject(MsgObj)));
		}
		
		JsonRequest->SetArrayField("messages", MessagesJson);
	}

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonRequest.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
}

#undef LOCTEXT_NAMESPACE
