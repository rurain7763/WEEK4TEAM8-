#pragma once

#include <Windows.h>
#include "FrameTimer.h"
#include "FEditorViewportClient.h"
#include "FEditorUIManager.h"
#include "Camera.h"
#include "SceneManager.h"
#include "FileManager.h"
#include "Renderer.h"
#include "World.h"
#include "FAssetManager.h"
#include "FFontManager.h"
#include "FComponentVisualizer.h"
#include "FEditorViewportManager.h"

#include <d3d11.h>

class Sphere;
class FGraphicsManager;
class FEngineLoop
{
public:
	FEngineLoop()
	{
	}
	~FEngineLoop() {};

	void Init(HINSTANCE hInstance, WNDPROC WndProc);
	void Tick(bool bPumpMessages);
	void End();

	FAssetManager* GetAssetManager() { return mAssetManager; }

private:
	void InitAssetManager();

private:
	// Todo: Make as pointer
	FFrameTimer* FrameTimer;
	bool GInTick = false;
	FEditorViewportClient* ViewportClient;

	FGraphicsManager* mGraphicsManager;
	FSceneManager* mSceneManager;
	FFileManager* mFileManager;
	FAssetManager* mAssetManager;
	FFontManager* mFontManager;
	FEditorUIManager* mEditorUIManager;
	FEditorViewportManager* mViewportManager;

	FComponentVisualizerManager* mComponentVisualizerManager;
};

inline FEngineLoop GEngineLoop;
