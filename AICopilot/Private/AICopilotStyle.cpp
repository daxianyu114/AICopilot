#include "AICopilotStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Brushes/SlateImageBrush.h"

TSharedPtr< FSlateStyleSet > FAICopilotStyle::StyleInstance = NULL;

void FAICopilotStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FAICopilotStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FAICopilotStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("AICopilotStyle"));
	return StyleSetName;
}

const ISlateStyle& FAICopilotStyle::Get()
{
	return *StyleInstance;
}

void FAICopilotStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )

TSharedRef< FSlateStyleSet > FAICopilotStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("AICopilotStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("AICopilot")->GetBaseDir() / TEXT("Resources"));

	Style->Set("AICopilot.OpenPluginWindow", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon20x20));

	return Style;
}
#undef IMAGE_BRUSH
