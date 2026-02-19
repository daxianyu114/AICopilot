#include "AICopilot.h"
#include "AICopilotStyle.h"
#include "AICopilotCommands.h"
#include "SAICopilotWindow.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

static const FName AICopilotTabName("AICopilot");

#define LOCTEXT_NAMESPACE "FAICopilotModule"

void FAICopilotModule::StartupModule()
{
	FAICopilotStyle::Initialize();
	FAICopilotStyle::ReloadTextures();

	FAICopilotCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FAICopilotCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FAICopilotModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAICopilotModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AICopilotTabName, FOnSpawnTab::CreateRaw(this, &FAICopilotModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("AICopilotTabTitle", "AI Copilot"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FAICopilotModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FAICopilotStyle::Shutdown();

	FAICopilotCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AICopilotTabName);
}

TSharedRef<SDockTab> FAICopilotModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SAICopilotWindow)
		];
}

void FAICopilotModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(AICopilotTabName);
}

void FAICopilotModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FAICopilotCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FAICopilotCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAICopilotModule, AICopilot)
