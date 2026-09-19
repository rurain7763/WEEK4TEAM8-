#pragma once

#include <Windows.h>
#include "FrameTimer.h"
#include "FEditorViewportClient.h"
#include "Camera.h"
#include "SceneManager.h"
#include "FileManager.h"
#include "Renderer.h"
#include "World.h"
#include "FAssetManager.h"
#include "FFontManager.h"
#include "FComponentVisualizer.h"

#include <d3d11.h>

class Sphere;
class FGraphicsManager;

struct FEditorLayout
{
	bool bIsSplitView = false;

	TSharedPtr<SWindow> RootWindow;
	TSharedPtr<SSplitterH> HSplitter;
	TSharedPtr<SSplitterV> VSplitter[2];
	TSharedPtr<SWindow> ViewportWindows[4];

	void Initialize(const FRect& InRect)
	{
		// Build viewport layout tree
		TSharedPtr<SSplitterH> HRoot = MakeShared<SSplitterH>();
		TSharedPtr<SSplitterV> VSplitter0 = MakeShared<SSplitterV>();
		TSharedPtr<SSplitterV> VSplitter1 = MakeShared<SSplitterV>();

		for (int32 i = 0; i < 4; ++i)
		{
			ViewportWindows[i] = MakeShared<SWindow>();
		}

		VSplitter0->SideLT = ViewportWindows[0];
		VSplitter0->SideRB = ViewportWindows[1];

		VSplitter1->SideLT = ViewportWindows[2];
		VSplitter1->SideRB = ViewportWindows[3];

		HRoot->SideLT = VSplitter0;
		HRoot->SideRB = VSplitter1;

		HRoot->SetRect(InRect);

		RootWindow = HRoot;
		HSplitter = HRoot;
		VSplitter[0] = VSplitter0;
		VSplitter[1] = VSplitter1;
	}

	void Resize(const FRect& InRect)
	{
		RootWindow->SetRect(InRect);
	}

	void SetSplitRatios(float HorizontalRatio, float VerticalRatio)
	{
		HSplitter->SplitterRatio = HorizontalRatio;
		VSplitter[0]->SplitterRatio = VerticalRatio;
		VSplitter[1]->SplitterRatio = VerticalRatio;
	}

	void GetSplitRatios(float& HorizontalRatio, float& VerticalRatio)
	{
		HorizontalRatio = HSplitter->SplitterRatio;
		VerticalRatio = VSplitter[0]->SplitterRatio;
	}

	void GetViewportRects(FRect& LT, FRect& RT, FRect& LB, FRect& RB)
	{
		LT = ViewportWindows[0]->Rect;
		RT = ViewportWindows[1]->Rect;
		LB = ViewportWindows[2]->Rect;
		RB = ViewportWindows[3]->Rect;
	}
};

struct FEditorViewport
{
	TSharedPtr<SWindow> Window;
	TSharedPtr<FViewport> Viewport;
	TSharedPtr<FEditorViewportClient> Client;

	void Release()
	{
		Viewport.reset();
		Client.reset();
		Window.reset();
	}
};

class FEngineLoop
{
public:
	FEngineLoop() = default;
	~FEngineLoop() = default;

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

	FEditorLayout mEditorLayout;
	FEditorViewport mMainViewport;
	FEditorViewport mSplitViewports[4];

	FGraphicsManager* mGraphicsManager;
	FSceneManager* mSceneManager;
	FFileManager* mFileManager;
	FAssetManager* mAssetManager;
	FFontManager* mFontManager;

	FComponentVisualizerManager* mComponentVisualizerManager;
};

inline FEngineLoop GEngineLoop;
