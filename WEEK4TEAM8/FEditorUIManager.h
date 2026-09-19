#pragma once

#include "Core.h"
#include "TArray.h"
#include "enum.h"

class FFileManager;
class FEditorViewportClient;
class FSceneManager;
class FGraphicsManager;
class FAssetManager;
class UObject;
class ImGuiIO;
class FFrameTimer;

struct FGuiReference
{
	const FFrameTimer& FrameTimer;
	FGraphicsManager* GraphicsManager;
	FEditorViewportClient* ViewportClient;
	FSceneManager* SceneManager;
	const FFileManager* FileManager;
	FAssetManager* AssetManager;
};

struct FGuiInputField
{
	/* Spawn Actor */
	EPrimitive PrimitiveType = EPrimitive::EP_Cube;
	int32 SpawnCount = 1;

	/* Object Lists */
	TArray<UObject*> SortedObjectLists;
	uint64 LastGUObjectRevision = -1;
};

class FEditorUIManager
{
public:
	FEditorUIManager(const ImGuiIO& InIO);
	void UpdateGUI(const FGuiReference& guiReference);


	float GetPanelWidth() const { return mPanelWidth; }
	float GetViewportX() const { return mViewportX; }
	float GetViewportY() const { return mViewportY; }
	float GetViewportWidth() const { return mViewportWidth; }
	float GetViewportHeight() const { return mViewportHeight; }
	bool IsViewportHovered() const { return mbViewportHovered; }

private:
	FGuiInputField mGuiInputField;

	float mPanelWidth;
	float mViewportX;
	float mViewportY;
	float mViewportWidth;
	float mViewportHeight;
	bool mbViewportHovered = false;


	static constexpr float MIN_WIDTH_RATIO = 0.2f;
	static constexpr float CONTROL_PANEL_HEIGHT_RATIO = 0.4f;
	static constexpr float WINDOW_PROPERTY_HEIGHT_RATIO = 0.3f;

	void updateControlPanelGUI(const FGuiReference& guiReference);
	void updatePropertyWindowGUI(const FGuiReference& guiReference);
	void updateObjectListPanelGUI(const FGuiReference& guiReference);
};
