#include "VortexEditor.h"
#include "VortexMechanism.h"
#include "MechanismComponent.h"
#include "VortexRuntime.h"
#include "Runtime/Projects/Public/Interfaces/IPluginManager.h"
#include "Runtime/SlateCore/Public/Styling/SlateStyleRegistry.h"
#include "ContentBrowserModule.h"
#include "ComponentAssetBroker.h"
#include "MechanismComponentAssetBroker.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "FVortexEditorModule"

namespace
{
    void CreateVortexEditorActionsMenu(FMenuBuilder& MenuBuilder, UVortexMechanism* mechanism)
    {
        MenuBuilder.AddMenuEntry(
            LOCTEXT("EditMenuLabel", "Edit with Vortex Studio"),
            LOCTEXT("EditMenuTooltip", "Edit this Mechanism with Vortex Studio"),
            FSlateIcon("VortexStyle", "ClassThumbnail.VortexMechanism"),
            FExecuteAction::CreateStatic([](UVortexMechanism* mechanism) {
                FPlatformProcess::CreateProc(*FPaths::Combine(*FVortexRuntimeModule::GetVortexStudioBinDir(), TEXT("VortexEditor.exe")),
                    *FString::Printf(TEXT("\"%s\""), *FPaths::Combine(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()), mechanism->MechanismFilepath.FilePath)),
                    true, false, false, nullptr, 0, nullptr, nullptr);
                }, mechanism),
            NAME_None,
                    EUserInterfaceActionType::Button);
    }
}

void FVortexEditorModule::StartupModule()
{
    //This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
    StyleSet = MakeShareable(new FSlateStyleSet("VortexStyle"));

    //Content path of this plugin
    FString BaseDir = IPluginManager::Get().FindPlugin("VortexPlugin")->GetBaseDir();

    //The image we wish to load is located inside the Resources folder inside the Base Directory
    //so let's set the content dir to the base dir and manually switch to the Resources folder:
    StyleSet->SetContentRoot(BaseDir);

    //Create a brush from the icon
    FSlateImageBrush* ThumbnailBrush = new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Resources/MechanismIcon"), TEXT(".png")), FVector2D(72.f, 72.f));

    if (ThumbnailBrush)
    {
        //In order to bind the thumbnail to our class we need to type ClassThumbnail.X where X is the name of the C++ class of the asset
        StyleSet->Set("ClassThumbnail.VortexMechanism", ThumbnailBrush);
        //Register the created style
        FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
    }

    //Edit in Studio menu
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    ContentBrowserModule.GetAllAssetViewContextMenuExtenders().Add(
        FContentBrowserMenuExtender_SelectedAssets::CreateStatic([](const TArray<FAssetData> & SelectedAssets)
        {
            TSharedRef<FExtender> Extender(new FExtender());
            if (SelectedAssets.Num() == 1 && SelectedAssets[0].GetClass() == UVortexMechanism::StaticClass())
            {
                Extender->AddMenuExtension(
                    "CommonAssetActions",
                    EExtensionHook::Before,
                    nullptr,
                    FMenuExtensionDelegate::CreateStatic(&CreateVortexEditorActionsMenu, Cast<UVortexMechanism>(SelectedAssets[0].GetAsset())));
            }
            return Extender;
        }));
    //Register Vortex Mechanism as an asset that can be used to create a Mechanism Component
    
    MechanismAssetBroker = MakeShared<FMechanismComponentAssetBroker>();
    FComponentAssetBrokerage::RegisterBroker(MechanismAssetBroker, UMechanismComponent::StaticClass(), true, true);
}

void FVortexEditorModule::ShutdownModule()
{

}

IMPLEMENT_MODULE(FVortexEditorModule, VortexEditor)