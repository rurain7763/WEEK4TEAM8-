#pragma once

#include "ImGui/imgui.h"
#include "NativeFileDialog.h"
#include "AssetFileIOs.h"
#include <filesystem>

struct FContentBrowserEventHandler
{
	virtual void OnNewAssetFile(const FAssetFileHeader& Header, const std::filesystem::path& FilePath) {}
	virtual void OnDeleteAssetFile(const std::filesystem::path& FilePath) {}
};

class FContentBrowser
{
public:
	void Initialize(const std::filesystem::path& InitDirectory);
	void SetEventHandler(FContentBrowserEventHandler* InEventHandler);

	void Render();

	void ToggleDrawer() { bIsDrawerOpen = !bIsDrawerOpen; }
	bool IsDrawerOpen() const { return bIsDrawerOpen; }

private:
	void RenderBottomBar();
	void RenderDrawer();

private:
	std::filesystem::path RootDirectory;
	std::filesystem::path CurrentDirectory;
	FContentBrowserEventHandler* EventHandler = nullptr;

	bool bIsDrawerOpen = false;
	float DrawerHeight = 350.0f;
	const float BottomBarHeight = 28.0f;
};