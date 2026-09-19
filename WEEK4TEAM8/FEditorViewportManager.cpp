#include "Core.h"
#include "FEditorViewportManager.h"
#include "ImGui/imgui.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <windows.h>
#include <assert.h>

FEditorViewportManager::FEditorViewportManager(URenderer& InRenderer) : Renderer(InRenderer)
{
	BuildDefaultLayout();
	LoadLayout();
}

void FEditorViewportManager::BuildDefaultLayout()
{
	for (int i = 0; i < 4; ++i)
	{
		ViewportClients.Emplace(std::make_unique<FEditorViewportClient>(Renderer));
	}

	ViewportWindows.SetNum(4);

	RootWindow = MakeWindow<SWindow>();
	RootSplitter = MakeWindow<SSplitterV>();
	TopSplitter = MakeWindow<SSplitterH>();
	BottomSplitter = MakeWindow<SSplitterH>();

	ViewportWindows[static_cast<size_t>(EViewportType::Perspective)] = MakeWindow<SViewportWindow>(EViewportType::Perspective);
	ViewportWindows[static_cast<size_t>(EViewportType::Top)] = MakeWindow<SViewportWindow>(EViewportType::Top);
	ViewportWindows[static_cast<size_t>(EViewportType::Front)] = MakeWindow<SViewportWindow>(EViewportType::Front);
	ViewportWindows[static_cast<size_t>(EViewportType::Side)] = MakeWindow<SViewportWindow>(EViewportType::Side);

	ViewportWindows[static_cast<size_t>(EViewportType::Perspective)]->Viewport.Client = ViewportClients[0].get();
	ViewportWindows[static_cast<size_t>(EViewportType::Top)]->Viewport.Client = ViewportClients[1].get();
	ViewportWindows[static_cast<size_t>(EViewportType::Front)]->Viewport.Client = ViewportClients[2].get();
	ViewportWindows[static_cast<size_t>(EViewportType::Side)]->Viewport.Client = ViewportClients[3].get();

	RootSplitter->SideLT = TopSplitter;
	RootSplitter->SideRB = BottomSplitter;
	TopSplitter->SideLT = ViewportWindows[static_cast<size_t>(EViewportType::Perspective)];
	TopSplitter->SideRB = ViewportWindows[static_cast<size_t>(EViewportType::Top)];
	BottomSplitter->SideLT = ViewportWindows[static_cast<size_t>(EViewportType::Front)];
	BottomSplitter->SideRB = ViewportWindows[static_cast<size_t>(EViewportType::Side)];

	FEditorViewportClient* PerspectiveClient = ViewportClients[0].get();
	FEditorViewportClient* TopClient = ViewportClients[1].get();
	FEditorViewportClient* FrontClient = ViewportClients[2].get();
	FEditorViewportClient* SideClient = ViewportClients[3].get();

	TopClient->GetCamera().Transform.Location = FVector(0.0f, 0.0f, 10.0f);
	TopClient->GetCamera().LookAt(FVector(0.0f, 0.0f, 0.0f));
	TopClient->GetCamera().mOrthoHeight = 10.0f;

	FrontClient->GetCamera().Transform.Location = FVector(0.0f, 10.0f, 1.0f);
	FrontClient->GetCamera().LookAt(FVector(0.0f, 0.0f, 0.0f));
	FrontClient->GetCamera().mOrthoHeight = 10.0f;

	SideClient->GetCamera().Transform.Location = FVector(10.0f, 0.0f, 1.0f);
	SideClient->GetCamera().LookAt(FVector(0.0f, 0.0f, 0.0f));
	SideClient->GetCamera().mOrthoHeight = 10.0f;

}

void FEditorViewportManager::UpdateViewportRenderTargets()
{
	for (SViewportWindow* Window : ViewportWindows)
	{
		assert(Window);

		FViewport& Viewport = Window->Viewport;

		const UINT Width = (std::max)(1u, static_cast<UINT>(Viewport.Rect.Width));
		const UINT Height = (std::max)(1u, static_cast<UINT>(Viewport.Rect.Height));

		const bool bNeedsCreate =
			!Viewport.mSceneRenderTarget ||
			!Viewport.mSceneDepthStencil ||
			!Viewport.mSceneRenderTarget->RTV ||
			!Viewport.mSceneRenderTarget->SRV ||
			!Viewport.mSceneDepthStencil->DSV ||
			Viewport.mSceneRenderTarget->Width != Width ||
			Viewport.mSceneRenderTarget->Height != Height;

		if (!bNeedsCreate)
		{
			continue;
		}
		
		Viewport.mSceneRenderTarget = Renderer.CreateRenderTarget2D(Width, Height, DXGI_FORMAT_R8G8B8A8_UNORM);
		Viewport.mSceneDepthStencil = Renderer.CreateDepthStencil(Width, Height);
	}
}

void FEditorViewportManager::Arrange(const FRect& EditorRect)
{
	if (!bMultiViewport)
	{
		GetViewportWindow(EViewportType::Perspective).Arrange(EditorRect);
		UpdateViewportRenderTargets();
		return;
	}

	assert(RootWindow);
	assert(RootSplitter);
	RootWindow->Arrange(EditorRect);
	RootSplitter->Arrange(EditorRect);

	UpdateViewportRenderTargets();
}


bool FEditorViewportManager::BeginSplitterDrag(FPoint Cursor)
{
	for (SSplitter* Splitter 
		: { static_cast<SSplitter*>(RootSplitter), static_cast<SSplitter*>(TopSplitter), static_cast<SSplitter*>(BottomSplitter) })
	{
		if (Splitter->IsSplitterHover(Cursor, SplitterThickness))
		{
			DraggedSplitter = Splitter;
			return true;
		}
	}
	return false;
}

void FEditorViewportManager::UpdateSplitterDrag(FPoint Cursor)
{
	if (!DraggedSplitter)
	{
		return;
	}
	SetDraggedSplitterRatio(Cursor);
	Arrange(RootWindow->Rect);
}

void FEditorViewportManager::EndSplitterDrag()
{
	DraggedSplitter = nullptr;
}

void FEditorViewportManager::SetDraggedSplitterRatio(FPoint Cursor)
{
	const float Length = DraggedSplitter->IsHorizontal() 
		? DraggedSplitter->Rect.Width : DraggedSplitter->Rect.Height;
	const float CursorOffset = DraggedSplitter->IsHorizontal() 
		? Cursor.X - DraggedSplitter->Rect.X : Cursor.Y - DraggedSplitter->Rect.Y;
	if (Length <= MinimumViewportSize * 2.0f)
	{
		return;
	}

	const float MinRatio = MinimumViewportSize / Length;
	const float NewRatio = std::clamp(CursorOffset / Length, MinRatio, 1.0f - MinRatio);

	if (DraggedSplitter == TopSplitter || DraggedSplitter == BottomSplitter)
	{
		TopSplitter->SplitRatio = NewRatio;
		BottomSplitter->SplitRatio = NewRatio;
	}
	else
	{
		RootSplitter->SplitRatio = NewRatio;
	}
}

void FEditorViewportManager::DrawSplitters(ImDrawList* DrawList) const
{
	const ImU32 Color = IM_COL32(120, 120, 120, 255);

	const float RootY =RootSplitter->Rect.Y + RootSplitter->Rect.Height * RootSplitter->SplitRatio;
	DrawList->AddLine(
		ImVec2(RootSplitter->Rect.X, RootY),
		ImVec2(RootSplitter->Rect.X + RootSplitter->Rect.Width, RootY),
		Color,
		SplitterThickness
	);

	const float TopX = TopSplitter->Rect.X + TopSplitter->Rect.Width * TopSplitter->SplitRatio;
	DrawList->AddLine(
		ImVec2(TopX, TopSplitter->Rect.Y),
		ImVec2(TopX, TopSplitter->Rect.Y + TopSplitter->Rect.Height),
		Color,
		SplitterThickness
	);

	const float BottomX = BottomSplitter->Rect.X + BottomSplitter->Rect.Width * BottomSplitter->SplitRatio;
	DrawList->AddLine(
		ImVec2(BottomX, BottomSplitter->Rect.Y),
		ImVec2(BottomX, BottomSplitter->Rect.Y + BottomSplitter->Rect.Height),
		Color,
		SplitterThickness
	);
}

void FEditorViewportManager::LoadLayout()
{
	auto ReadRatio = [](const char* Key, float& OutRatio)
	{
		char Value[64]{};
		GetPrivateProfileStringA("ViewportLayout", Key, "", Value, sizeof(Value), ".\\editor.ini");
		float Parsed = 0.5f;
		if (sscanf_s(Value, "%f", &Parsed) == 1)
		{
			OutRatio = std::clamp(Parsed, 0.05f, 0.95f);
		}
	};

	ReadRatio("RootSplit", RootSplitter->SplitRatio);
	ReadRatio("TopSplit", TopSplitter->SplitRatio);
	ReadRatio("BottomSplit", BottomSplitter->SplitRatio);
}

void FEditorViewportManager::SaveLayout() const
{
	auto WriteRatio = [](const char* Key, float Ratio)
	{
		const FString Value = std::to_string(Ratio);
		WritePrivateProfileStringA("ViewportLayout", Key, Value.CStr(), ".\\editor.ini");
	};

	WriteRatio("RootSplit", RootSplitter->SplitRatio);
	WriteRatio("TopSplit", TopSplitter->SplitRatio);
	WriteRatio("BottomSplit", BottomSplitter->SplitRatio);
}

SViewportWindow& FEditorViewportManager::GetViewportWindow(EViewportType Type)
{
	return *ViewportWindows[static_cast<size_t>(Type)];
}

SViewportWindow* FEditorViewportManager::FindViewportAt(FPoint Cursor)
{
	for (SViewportWindow* Window : ViewportWindows)
	{
		if (Window->Rect.Contains(Cursor))
		{
			return Window;
		}
	}

	return nullptr;
}