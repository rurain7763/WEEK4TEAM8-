#include "FViewportManager.h"

void FViewportManager::Init()
{
	mRootSplitter = new SSplitterV();
	mTopSplitter = new SSplitterH();
	mBottomSplitter = new SSplitterH();

	SViewportWindow* TopView = new SViewportWindow();
	TopView->SetupView(EViewportType::Top);

	SViewportWindow* PerspView = new SViewportWindow();
	PerspView->SetupView(EViewportType::Perspective);

	SViewportWindow* FrontView = new SViewportWindow();
	FrontView->SetupView(EViewportType::Front);

	SViewportWindow* SideView = new SViewportWindow();
	SideView->SetupView(EViewportType::Side);

	mRootSplitter->SideLT = mTopSplitter;
	mRootSplitter->SideRB = mBottomSplitter;

	mTopSplitter->SideLT = TopView;
	mTopSplitter->SideRB = PerspView;

	mBottomSplitter->SideLT = FrontView;
	mBottomSplitter->SideRB = SideView;

	mViewportWindows.Add(TopView);
	mViewportWindows.Add(PerspView);
	mViewportWindows.Add(FrontView);
	mViewportWindows.Add(SideView);

	mActiveViewportWindow = PerspView;
}

void FViewportManager::Tick(float DeltaTime, FSceneManager* SceneManager, FEditorViewportClient* ViewportClient, FRenderCollector* RenderCollector, const FInputState& Input)
{
	const float TotalW = SceneManager->GetViewportWidth();
	const float TotalH = SceneManager->GetViewportHeight();

	const float SplitX = TotalW * mTopSplitter->SplitRatio;
	const float SplitY = TotalH * mRootSplitter->SplitRatio;

	const float HitThickness = 6.0f;

	FVector2 LocalMouse(
		static_cast<float>(Input.CursorX) - SceneManager->GetViewportX(),
		static_cast<float>(Input.CursorY) - SceneManager->GetViewportY()
	);

	if (CurrentLayoutMode == EViewportLayoutMode::Single)
	{
		bIsDraggingSplitter = false;
		SplitterDragState = ESplitterDragState::None;
		mHoveredViewportWindow = mActiveViewportWindow;

		if (mActiveViewportWindow && SceneManager->IsViewportHovered() && TotalW > 0.0f && TotalH > 0.0f)
		{
			mActiveViewportWindow->Rect = FRect(0.0f, 0.0f, TotalW, TotalH);

			float Ratio = mActiveViewportWindow->bIsOrthographic ? 0.0f : 1.0f;
			FMatrix GizmoVP = mActiveViewportWindow->Camera.GetViewMatrix() *
				mActiveViewportWindow->Camera.GetUnifiedProjectionMatrix(
					TotalW / TotalH,
					mActiveViewportWindow->Camera.mFovDegree,
					mActiveViewportWindow->Camera.mOrthoDistance, 0.1f, 1000.f, Ratio);

			ViewportClient->mGizmo.Update(SceneManager, GizmoVP, mActiveViewportWindow->Rect);

			if (!ViewportClient->mGizmo.IsDragging() && !ViewportClient->mGizmo.IsMouseOverHandle())
			{
				AActor* HitActor = ViewportClient->PerformMousePicking(
					mActiveViewportWindow->Camera,
					static_cast<int32>(LocalMouse.X),
					static_cast<int32>(LocalMouse.Y),
					TotalW,
					TotalH,
					Ratio,
					*RenderCollector
				);

				if (Input.WasPressed(VK_LBUTTON))
				{
					if (HitActor) SceneManager->SetSelectedActor(HitActor);
					else SceneManager->ResetSelectedActor();
				}
			}
		}
		return;
	}

	if (!bIsDraggingSplitter)
	{
		bool bNearV = FMath::Abs(LocalMouse.X - SplitX) <= HitThickness;
		bool bNearH = FMath::Abs(LocalMouse.Y - SplitY) <= HitThickness;

		if (bNearV && bNearH)
			SplitterDragState = ESplitterDragState::Cross;
		else if (bNearV)
			SplitterDragState = ESplitterDragState::Vertical;
		else if (bNearH)
			SplitterDragState = ESplitterDragState::Horizontal;
		else
			SplitterDragState = ESplitterDragState::None;
	}

	if (SceneManager->IsViewportHovered() && mRootSplitter)
	{
		SWindow* Node = mRootSplitter->FindHoveredWindow(LocalMouse);
		mHoveredViewportWindow = dynamic_cast<SViewportWindow*>(Node);
		if (mHoveredViewportWindow && (Input.WasPressed(VK_LBUTTON) || Input.WasPressed(VK_RBUTTON)))
		{
			mActiveViewportWindow = mHoveredViewportWindow;
		}
	}

	if (SplitterDragState != ESplitterDragState::None && Input.WasPressed(VK_LBUTTON))
	{
		bIsDraggingSplitter = true;
	}

	if (bIsDraggingSplitter)
	{
		if (Input.IsDown(VK_LBUTTON))
		{
			constexpr float MinRatio = 0.1f;
			constexpr float MaxRatio = 0.9f;

			if (SplitterDragState == ESplitterDragState::Vertical || SplitterDragState == ESplitterDragState::Cross)
			{
				float NewRatioX = FMath::Clamp(LocalMouse.X / TotalW, MinRatio, MaxRatio);
				mTopSplitter->SplitRatio = NewRatioX;
				mBottomSplitter->SplitRatio = NewRatioX;
			}

			if (SplitterDragState == ESplitterDragState::Horizontal || SplitterDragState == ESplitterDragState::Cross)
			{
				float NewRatioY = FMath::Clamp(LocalMouse.Y / TotalH, MinRatio, MaxRatio);
				mRootSplitter->SplitRatio = NewRatioY;
			}
		}
		else
		{
			bIsDraggingSplitter = false;
			SplitterDragState = ESplitterDragState::None;
		}
	}

	const bool bInteractingWithSplitter = (SplitterDragState != ESplitterDragState::None) || bIsDraggingSplitter;

	if (!bInteractingWithSplitter)
	{
		SViewportWindow* GizmoTargetVW = ViewportClient->mGizmo.IsDragging() ? mActiveViewportWindow : mHoveredViewportWindow;

		if (GizmoTargetVW)
		{
			float Ratio = GizmoTargetVW->bIsOrthographic ? 0.0f : 1.0f;
			FMatrix GizmoVP = GizmoTargetVW->Camera.GetViewMatrix() *
				GizmoTargetVW->Camera.GetUnifiedProjectionMatrix(
					GizmoTargetVW->Rect.Width / GizmoTargetVW->Rect.Height,
					GizmoTargetVW->Camera.mFovDegree,
					GizmoTargetVW->Camera.mOrthoDistance, 0.1f, 1000.f, Ratio);

			ViewportClient->mGizmo.Update(SceneManager, GizmoVP, GizmoTargetVW->Rect);

			if (ViewportClient->mGizmo.IsDragging() && mHoveredViewportWindow)
			{
				mActiveViewportWindow = mHoveredViewportWindow;
			}
		}

		if (mHoveredViewportWindow && !ViewportClient->mGizmo.IsDragging() && !ViewportClient->mGizmo.IsMouseOverHandle())
		{
			int32 SubMouseX = static_cast<int32>(LocalMouse.X - mHoveredViewportWindow->Rect.X);
			int32 SubMouseY = static_cast<int32>(LocalMouse.Y - mHoveredViewportWindow->Rect.Y);
			float Ratio = mHoveredViewportWindow->bIsOrthographic ? 0.0f : 1.0f;

			AActor* HitActor = ViewportClient->PerformMousePicking(
				mHoveredViewportWindow->Camera,
				SubMouseX, SubMouseY,
				mHoveredViewportWindow->Rect.Width, mHoveredViewportWindow->Rect.Height,
				Ratio,
				*RenderCollector
			);

			if (Input.WasPressed(VK_LBUTTON))
			{
				if (HitActor) SceneManager->SetSelectedActor(HitActor);
				else SceneManager->ResetSelectedActor();
			}
		}
	}
}

void FViewportManager::Render(FGraphicsManager* GraphicsManager, FSceneManager* SceneManager, FEditorViewportClient* ViewportClient)
{
	const float TotalW = SceneManager->GetViewportWidth();
	const float TotalH = SceneManager->GetViewportHeight();

	if (CurrentLayoutMode == EViewportLayoutMode::Single)
	{
		mActiveViewportWindow->Rect = FRect(0.0f, 0.0f, TotalW, TotalH);
		RenderSingleViewport(mActiveViewportWindow, GraphicsManager, SceneManager, ViewportClient, true, true);
	}
	else
	{
		mRootSplitter->UpdateLayout(FRect(0.0f, 0.0f, TotalW, TotalH));

		for (int i = 0; i < 4; ++i)
		{
			bool bClear = (i == 0);
			bool bRecordGizmo = (mViewportWindows[i] == GetGizmoTargetViewport(ViewportClient));
			RenderSingleViewport(mViewportWindows[i], GraphicsManager, SceneManager, ViewportClient, bClear, bRecordGizmo);
		}
	}
}

void FViewportManager::RenderSingleViewport(SViewportWindow* VW, FGraphicsManager* GraphicsManager, FSceneManager* SceneManager, FEditorViewportClient* ViewportClient, bool bClear, bool bRecordGizmo)
{
	if (!VW || VW->Rect.Width <= 0.0f || VW->Rect.Height <= 0.0f) return;

	float Ratio = VW->bIsOrthographic ? 0.0f : 1.0f;
	GraphicsManager->Prepare(&VW->Camera, VW->Rect.Width, VW->Rect.Height, Ratio, bClear);

	GraphicsManager->SetCurrentViewportType(VW->ViewType);

	D3D11_VIEWPORT D3DViewport = VW->GetD3DViewport();
	GraphicsManager->GetRenderer()->GetDeviceContext()->RSSetViewports(1, &D3DViewport);

	GraphicsManager->Render();

	if (SceneManager->GetSelectedActor())
	{
		FRenderInfo clickedRenderInfo;
		SceneManager->GetSelectedActor()->GetFirstRenderInfo(clickedRenderInfo);
		GraphicsManager->RenderHighLight(clickedRenderInfo);
	}

	ViewportClient->mGizmo.Render(SceneManager, VW->Camera.Transform.Location, GraphicsManager->GetViewProjectionMatrix(), bRecordGizmo);
}

void FViewportManager::SetLayoutMode(EViewportLayoutMode InMode, FSceneManager* SceneManager)
{
	if (CurrentLayoutMode == InMode) return;
	CurrentLayoutMode = InMode;

	if (CurrentLayoutMode == EViewportLayoutMode::Quad)
	{
		FVector FocusPoint(0.f, 0.f, 0.f);
		if (AActor* Selected = SceneManager->GetSelectedActor())
		{
			FocusPoint = Selected->GetTransform().Location;
		}
		else
		{
			FocusPoint = mActiveViewportWindow->Camera.Transform.Location + mActiveViewportWindow->Camera.GetForwardVector() * 10.0f;
		}

		for (int i = 0; i < 4; ++i)
		{
			if (mViewportWindows[i] != mActiveViewportWindow && mViewportWindows[i]->bIsOrthographic)
			{
				mViewportWindows[i]->SetupView(mViewportWindows[i]->ViewType, FocusPoint);
			}
		}
	}
}

SViewportWindow* FViewportManager::GetGizmoTargetViewport(FEditorViewportClient* ViewportClient) const
{
	SViewportWindow* Target = ViewportClient->mGizmo.IsDragging() ? mActiveViewportWindow : mHoveredViewportWindow;
	return Target ? Target : mActiveViewportWindow;
}

void FViewportManager::ToggleMaximize(FSceneManager* SceneManager)
{
	if (CurrentLayoutMode == EViewportLayoutMode::Single)
	{
		SetLayoutMode(EViewportLayoutMode::Quad, SceneManager);
	}
	else
	{
		SetLayoutMode(EViewportLayoutMode::Single, SceneManager);
	}
}

void FViewportManager::ChangeViewportType(SViewportWindow* TargetVW, EViewportType NewType)
{
	if (!TargetVW || TargetVW->ViewType == NewType) return;

	FVector CurrentFocus = TargetVW->Camera.Transform.Location + TargetVW->Camera.GetForwardVector() * 10.0f;
	TargetVW->SetupView(NewType, CurrentFocus);
}

void FViewportManager::UpdateMouseCursor()
{
	if (SplitterDragState == ESplitterDragState::Cross)
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
	else if (SplitterDragState == ESplitterDragState::Vertical)
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	else if (SplitterDragState == ESplitterDragState::Horizontal)
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
}