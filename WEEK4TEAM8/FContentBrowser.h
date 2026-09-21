#pragma once

#include "ImGui/imgui.h"
#include "NativeFileDialog.h"
#include "AssetFileIOs.h"
#include "FAssetManager.h"
#include <filesystem>
#include "FTexture2DImporter.h"
#include "FStaticMeshImporter.h"
#include "FMaterialImporter.h"
#include "FLogManager.h"
#include "FEditorIconUtils.hpp"


struct FContentBrowserEventHandler
{
	virtual void OnNewAssetFile(const FAssetFileHeader& Header, const std::filesystem::path& FilePath) {}
	virtual void OnDeleteAssetFile(const std::filesystem::path& FilePath) {}
};

struct FContentItem
{
	std::filesystem::path Path;
	FString DisplayName;
	bool bIsDirectory = false;
	EAssetType AssetType = EAssetType::None;
};

namespace AssetPayloadTags
{
	inline constexpr const char* StaticMesh = "DND_ASSET_STATICMESH";
	inline constexpr const char* Texture2D = "DND_ASSET_TEXTURE2D";
	inline constexpr const char* Material = "DND_ASSET_MATERIAL";
}

class FContentBrowser
{
public:
	void Initialize(const std::filesystem::path& InitDirectory);
	void SetEventHandler(FContentBrowserEventHandler* InEventHandler);

	void Render();

	void ToggleDrawer()
	{
		bIsDrawerOpen = !bIsDrawerOpen;
		if (bIsDrawerOpen)
		{
			RefreshCache();
		}
	}
	bool IsDrawerOpen() const { return bIsDrawerOpen; }
	void SetAssetManager(FAssetManager* InAssetManager) { AssetManager = InAssetManager; }
private:
	void RenderBottomBar();
	void RenderDrawer();
	void RenderFolderNode(const std::filesystem::path& DirectoryPath);
private:
	std::filesystem::path RootDirectory;
	std::filesystem::path CurrentDirectory;
	FContentBrowserEventHandler* EventHandler = nullptr;

	FAssetManager* AssetManager = nullptr;

	bool bIsDrawerOpen = false;
	float DrawerHeight = 350.0f;
	const float BottomBarHeight = 28.0f;

	TArray<FContentItem> CachedItems;
	void RefreshCache();
};