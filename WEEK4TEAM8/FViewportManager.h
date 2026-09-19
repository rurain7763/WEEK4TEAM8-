#pragma once
#include "SceneManager.h"
#include "GraphicsManager.h"
#include "FEditorViewportClient.h"
#include "Renderer.h"
#include "Actor.h"
#include "InputState.h"
#include "ImGui/imgui.h"

enum class EViewportLayoutMode
{
    Single, // 1개 화면 최대화
    Quad    // 4분할 화면
};

class FViewportManager
{
public:
    void Init();
    void Tick(float DeltaTime, FSceneManager* SceneManager, FEditorViewportClient* ViewportClient, FRenderCollector* RenderCollector, const FInputState& Input);
    void Render(FGraphicsManager* GraphicsManager, FSceneManager* SceneManager, FEditorViewportClient* ViewportClient);

    void SetLayoutMode(EViewportLayoutMode InMode, FSceneManager* SceneManager);
    void ToggleMaximize(FSceneManager* SceneManager);
    void ChangeViewportType(SViewportWindow* TargetVW, EViewportType NewType);

    void UpdateMouseCursor();

    EViewportLayoutMode GetLayoutMode() const { return CurrentLayoutMode; }
    SViewportWindow* GetActiveViewport() const { return mActiveViewportWindow; }

    float GetSplitRatioX() const { return mTopSplitter ? mTopSplitter->SplitRatio : 0.5f; }
    float GetSplitRatioY() const { return mRootSplitter ? mRootSplitter->SplitRatio : 0.5f; }
    ESplitterDragState GetDragState() const { return SplitterDragState; }
private:
    void RenderSingleViewport(SViewportWindow* VW, FGraphicsManager* GraphicsManager, FSceneManager* SceneManager, FEditorViewportClient* ViewportClient, bool bClear, bool bRecordGizmo);
    SViewportWindow* GetGizmoTargetViewport(FEditorViewportClient* ViewportClient) const;

    EViewportLayoutMode CurrentLayoutMode = EViewportLayoutMode::Quad;

    TArray<SViewportWindow*> mViewportWindows;
    SViewportWindow* mActiveViewportWindow = nullptr;
    SViewportWindow* mHoveredViewportWindow = nullptr;

    SSplitterV* mRootSplitter = nullptr;
    SSplitterH* mTopSplitter = nullptr;
    SSplitterH* mBottomSplitter = nullptr;

    ESplitterDragState SplitterDragState = ESplitterDragState::None;
    bool bIsDraggingSplitter = false;
};