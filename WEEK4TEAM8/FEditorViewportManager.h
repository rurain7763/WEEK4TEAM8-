#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "FViewportLayout.h"
#include "TArray.h"
#include "FEditorViewportClient.h"

struct ImDrawList;

class URenderer;

// 모든 viewport UI 노드와 viewport의 소유권을 관리
class FEditorViewportManager
{
public:
	static constexpr float SplitterThickness = 4.0f;
	static constexpr float MinimumViewportSize = 400.0f;

	FEditorViewportManager(URenderer& Renderer);

	void UpdateViewportRenderTargets();

	void Arrange(const FRect& EditorRect);
	bool BeginSplitterDrag(FPoint Cursor);
	void UpdateSplitterDrag(FPoint Cursor);
	void EndSplitterDrag();

	void LoadLayout();
	void SaveLayout() const;

	void DrawSplitters(ImDrawList* DrawList) const;

	SViewportWindow& GetViewportWindow(EViewportType Type);
	EViewportType ActiveViewportType = EViewportType::Perspective;

	SViewportWindow& GetActiveViewportWindow()
	{
		return GetViewportWindow(ActiveViewportType);
	}

	FEditorViewportClient& GetActiveViewportClient()
	{
		return *GetActiveViewportWindow().Viewport.Client;
	}

	SViewportWindow* FindViewportAt(FPoint Cursor);

	bool bMultiViewport = true;

private:
	template <typename T, typename... TArgs>
	T* MakeWindow(TArgs&&... Args)
	{
		auto Window = std::make_unique<T>(std::forward<TArgs>(Args)...);
		T* Result = Window.get();
		Windows.emplace_back(std::move(Window));
		return Result;
	}

	void BuildDefaultLayout();
	void SetDraggedSplitterRatio(FPoint Cursor);

	URenderer& Renderer;
	TArray<std::unique_ptr<FEditorViewportClient>> ViewportClients;

	std::vector<std::unique_ptr<SWindow>> Windows;
	SWindow* RootWindow = nullptr;
	SSplitterV* RootSplitter = nullptr;
	SSplitterH* TopSplitter = nullptr;
	SSplitterH* BottomSplitter = nullptr;
	TArray<SViewportWindow*> ViewportWindows{};
	SSplitter* DraggedSplitter = nullptr;
};
