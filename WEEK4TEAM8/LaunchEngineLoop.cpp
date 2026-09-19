#include "LaunchEngineLoop.h"

#include <windows.h>
#include "Renderer.h"
#include "WindowApplication.h"
#include "Console.h"
#include "GraphicsManager.h"
#include "CubeComponent.h"
#include "ObjectFactory.h"
#include "Cube.h"
#include "Sphere.h"
#include "Circle.h"
#include "Triangle.h"
#include "Plane.h"
#include "Object.h"
#include "GizmoArrow.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"
#include "Actor.h"
#include "World.h"
#include <FLogManager.h>
#include "Assets.h"
#include "FMeshDescription.h"
#include "FStaticMeshBuilder.h"
#include "FObjImporter.h"
#include "UStaticMeshComponent.h"
#include "FObjManager.h"

void FEngineLoop::Init(HINSTANCE hInstance, WNDPROC WndProc)
{
	// Initialize window infos
	WCHAR WindowClass[] = L"JungleWindowClass";
	WCHAR Title[] = L"Game Tech Lab";
	WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };
	RegisterClassW(&wndclass);

	HWND hWnd = CreateWindowExW(
		0,
		WindowClass,
		Title,
		WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 1024,
		nullptr, nullptr, hInstance, nullptr
	);

	// 창을 화면 크기에 맞게 최대화하여 표시
	ShowWindow(hWnd, SW_SHOWMAXIMIZED);
	UpdateWindow(hWnd);

	// 최대화된 후의 실제 클라이언트 크기를 구해 콘솔에 전달
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	int clientWidth = clientRect.right - clientRect.left;
	int clientHeight = clientRect.bottom - clientRect.top;

	RAWINPUTDEVICE rid = {};
	rid.usUsagePage = 0x01;		// Generic Desktop
	rid.usUsage = 0x02;			// Mouse
	rid.dwFlags = 0;		// 포커스 있을 때만 수신
	rid.hwndTarget = hWnd;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));

	mGraphicsManager = new FGraphicsManager(hWnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(mGraphicsManager->GetRenderer()->GetDevice(), mGraphicsManager->GetRenderer()->GetDeviceContext());
	auto& IO = ImGui::GetIO();
	IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	IO.Fonts->AddFontFromFileTTF(
		"C:/Windows/Fonts/malgun.ttf",
		18.0f,
		nullptr,
		IO.Fonts->GetGlyphRangesKorean()
	);

	/* Console Window */
	ConsoleWindow& console = ConsoleWindow::Get();
	console.Init(clientWidth);

	FrameTimer = new FFrameTimer(120);
	ViewportClient = new FEditorViewportClient(*mGraphicsManager->GetRenderer()); // Todo: cChange to class

	const FVector4 NearTint(1.0f, 0.65f, 0.15f, 0.85f); // 주황 = 가까운 쪽
	const FVector4 FarTint(0.25f, 0.55f, 1.0f, 0.85f); // 파랑 = 먼 쪽

	mSceneManager = new FSceneManager();
	mFileManager = new FFileManager();
	mFontManager = new FFontManager();
	InitAssetManager();

	mComponentVisualizerManager = new FComponentVisualizerManager();

	char Value[64] = {};
	GetPrivateProfileStringA("Grid", "Gap", "", Value, sizeof(Value), ".\\editor.ini");
	int32 GridGap = 1;
	sscanf_s(Value, "%d", &GridGap);
	mGraphicsManager->SetGridGap(GridGap);

	mSceneManager->NewScene();

	{
		//test code
		//{
		//	UCubeComponent* cubeComonent = FObjectFactory::ConstructObject<UCubeComponent>(FVector(0), FRotator(), FVector(1));
		//	AActor* cubeActor = FObjectFactory::ConstructObject<AActor>();
		//	cubeActor->AddComponent(cubeComonent);
		//	mSceneManager.GetCurrentWorld()->AddActor(cubeActor);
		//}

		AActor* ObjActor = FObjectFactory::ConstructObject<AActor>();
		UStaticMeshComponent* ObjComponent =
			FObjectFactory::ConstructObject<UStaticMeshComponent>(FString("Assets/Meshes/TestTriangle.obj"),
				FVector(0, 0, 0), FRotator(0, 0, 0), FVector(1, 1, 1));


		ObjActor->AddComponent(ObjComponent);
		mSceneManager->GetCurrentWorld()->AddActor(ObjActor);
	}

	{
		mRootSplitter = new SSplitterV();
		mTopSplitter = new SSplitterH();
		mBottomSplitter = new SSplitterH();

		SViewportWindow* TopView = new SViewportWindow();
		TopView->ViewType = EViewportType::Top;
		TopView->bIsOrthographic = true;
		TopView->Camera.Transform.Location = FVector(0.f, 0.f, 20.f);
		TopView->Camera.LookAt(FVector(0.f, 0.f, 0.f));
		TopView->Camera.mOrthoDistance = 10.0f;

		SViewportWindow* PerspView = new SViewportWindow();
		PerspView->ViewType = EViewportType::Perspective;
		PerspView->bIsOrthographic = false;
		PerspView->Camera.Transform.Location = FVector(-5.f, 5.f, 5.f);
		PerspView->Camera.LookAt(FVector(0.f, 0.f, 0.f));

		SViewportWindow* FrontView = new SViewportWindow();
		FrontView->ViewType = EViewportType::Front;
		FrontView->bIsOrthographic = true;
		FrontView->Camera.Transform.Location = FVector(-20.f, 0.f, 0.f);
		FrontView->Camera.LookAt(FVector(0.f, 0.f, 0.f));
		FrontView->Camera.mOrthoDistance = 10.0f;

		SViewportWindow* SideView = new SViewportWindow();
		SideView->ViewType = EViewportType::Side;
		SideView->bIsOrthographic = true;
		SideView->Camera.Transform.Location = FVector(0.f, -20.f, 0.f);
		SideView->Camera.LookAt(FVector(0.f, 0.f, 0.f));
		SideView->Camera.mOrthoDistance = 10.0f;

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
}

void FEngineLoop::InitAssetManager()
{
	mAssetManager = new FAssetManager();

	URenderer* renderer = mGraphicsManager->GetRenderer();

	FObjManager::Initialize(*renderer, *mFileManager);

	FObjManager::LoadObjStaticMesh("Assets/Meshes/TestCube.obj");
	FObjManager::LoadObjStaticMesh("Assets/Meshes/TestTriangle.obj");
	FObjManager::LoadObjStaticMesh("Assets/Meshes/TestHexagonalPrism.obj");

	// Register built-in asset types
	TSharedPtr<FStaticMeshAsset> cubeAsset = MakeShared<FStaticMeshAsset>(BuiltInAssetID::CubeMesh, FName("CubeMesh"), *renderer, Cube_vertices, sizeof(Cube_vertices) / sizeof(FVertexSimple), Cube_indices, sizeof(Cube_indices) / sizeof(uint32));
	mAssetManager->RegisterAsset(cubeAsset);

	TSharedPtr<FStaticMeshAsset> sphereAsset = MakeShared<FStaticMeshAsset>(BuiltInAssetID::CircleMesh, FName("SphereMesh"), *renderer, Sphere_vertices, sizeof(Sphere_vertices) / sizeof(FVertexSimple), Sphere_indices, sizeof(Sphere_indices) / sizeof(uint32));
	mAssetManager->RegisterAsset(sphereAsset);

	TSharedPtr<FStaticMeshAsset> circleAsset = MakeShared<FStaticMeshAsset>(BuiltInAssetID::CircleMesh, FName("CircleMesh"), *renderer, Circle_vertices, sizeof(Circle_vertices) / sizeof(FVertexSimple), Circle_indices, sizeof(Circle_indices) / sizeof(uint32));
	mAssetManager->RegisterAsset(circleAsset);

	TSharedPtr<FStaticMeshAsset> triangleAsset = MakeShared<FStaticMeshAsset>(BuiltInAssetID::TriangleMesh, FName("TriangleMesh"), *renderer, Triangle_vertices, sizeof(Triangle_vertices) / sizeof(FVertexSimple), Triangle_indices, sizeof(Triangle_indices) / sizeof(uint32));
	mAssetManager->RegisterAsset(triangleAsset);

	TSharedPtr<FStaticMeshAsset> gizmoArrowAsset = MakeShared<FStaticMeshAsset>(BuiltInAssetID::GizmoArrowMesh, FName("GizmoArrowMesh"), *renderer, GizmoArrow_vertices, sizeof(GizmoArrow_vertices) / sizeof(FVertexSimple), GizmoArrow_indices, sizeof(GizmoArrow_indices) / sizeof(uint32));
	mAssetManager->RegisterAsset(gizmoArrowAsset);

	TSharedPtr<FStaticMeshAsset> PlaneAsset = MakeShared<FStaticMeshAsset>(BuiltInAssetID::PlaneMesh, FName("PlaneMesh"), *renderer, Plane_vertices, sizeof(Plane_vertices) / sizeof(FVertexSimple), Plane_indices, sizeof(Plane_indices) / sizeof(uint32));
	mAssetManager->RegisterAsset(PlaneAsset);

	TSharedPtr<FTexture2DAssetLoader> TextureLoader = MakeShared<FTexture2DAssetLoader>(*renderer);
	TSharedPtr<FFontAssetLoader> FontLoader = MakeShared<FFontAssetLoader>(*mFontManager);

	TSharedPtr<FFileAssetSource> FileAssetSource = MakeShared<FFileAssetSource>("Assets/Textures/Test.jpg");
	mAssetManager->RegisterAsset(FGuid::NewGuid(), FName("TestTexture"), TextureLoader, FileAssetSource);

	TSharedPtr<FFileAssetSource> SpotLightIconAssetSource = MakeShared<FFileAssetSource>("Assets/Textures/Icon_SpotLight.png");
	mAssetManager->RegisterAsset(FGuid::NewGuid(), FName("SpotLightIcon"), TextureLoader, SpotLightIconAssetSource);

	TSharedPtr<FFileAssetSource> ExplosionTextureSource = MakeShared<FFileAssetSource>("Assets/Textures/ExplosionAtlas.png");
	mAssetManager->RegisterAsset(FGuid::NewGuid(), FName("ExplosionTexture"), TextureLoader, ExplosionTextureSource);

	TSharedPtr<FTexture2DAsset> ExplosionTexture2DAsset = mAssetManager->GetAssetAs<FTexture2DAsset>("ExplosionTexture", true);
	TSharedPtr<FSpriteAtlasAsset> ExplosionSpriteAtlasAsset = MakeShared<FSpriteAtlasAsset>(FGuid::NewGuid(), FName("ExplosionSpriteAtlas"), *renderer, ExplosionTexture2DAsset, 6, 6);
	mAssetManager->RegisterAsset(ExplosionSpriteAtlasAsset);

	TSharedPtr<FFileAssetSource> FontAssetSource = MakeShared<FFileAssetSource>("Assets/Fonts/BMKkubulimTTF.ttf");
	mAssetManager->RegisterAsset(FGuid::NewGuid(), FName("TestFont"), FontLoader, FontAssetSource);

	TSharedPtr<FFontAsset> TestFontAsset = mAssetManager->GetAssetAs<FFontAsset>(FName("TestFont"), true);
	TSharedPtr<FFontAtlasAsset> FontAtlasAsset = MakeShared<FFontAtlasAsset>(FGuid::NewGuid(), FName("TestFontAtlas"), *renderer, TestFontAsset, 512, 512, 2, 2);
	mAssetManager->RegisterAsset(FontAtlasAsset);
}

void FEngineLoop::Tick(bool bPumpMessages)
{
	if (GInTick) return;
	GInTick = true;

	FrameTimer->StartFrame();
	float deltaTime = FrameTimer->GetDeltaTime();
	ConsoleWindow& console = ConsoleWindow::Get();

	FRenderCollector& RenderCollector = mGraphicsManager->GetRenderCollector();
	RenderCollector.Camera = &ViewportClient->GetCamera();

	//Input Threads
	{
		WindowApplication.ProcessDeferredEvents();

		mGraphicsManager->UpdateProjectionTransition(deltaTime);

		if (mActiveViewportWindow)
		{
			float ActiveRatio = mActiveViewportWindow->bIsOrthographic ? 0.0f : 1.0f;
			ViewportClient->Update(deltaTime, mActiveViewportWindow->Camera, ActiveRatio, mSceneManager);
		}

		//ViewportClient->Update(deltaTime, mSceneManager, mGraphicsManager->GetPerspectiveRatio(), RenderCollector);
	}

	//Physics Threads
	{

	}

	//Game Threads
	{
		mSceneManager->Tick(deltaTime);
		mSceneManager->Update(deltaTime, RenderCollector);
	}

	//mouse picking
	{
		const float TotalW = mSceneManager->GetViewportWidth();
		const float TotalH = mSceneManager->GetViewportHeight();

		// 십자선의 현재 픽셀 위치
		const float SplitX = TotalW * mTopSplitter->SplitRatio;
		const float SplitY = TotalH * mRootSplitter->SplitRatio;

		const float HitThickness = 6.0f;

		const FInputState& Input = WindowApplication.Input;

		FVector2 LocalMouse(
			static_cast<float>(Input.CursorX) - mSceneManager->GetViewportX(),
			static_cast<float>(Input.CursorY) - mSceneManager->GetViewportY()
		);

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

		if (mSceneManager->IsViewportHovered() && mRootSplitter)
		{
			SWindow* Node = mRootSplitter->FindHoveredWindow(LocalMouse);
			HoveredVW = dynamic_cast<SViewportWindow*>(Node);
			if (HoveredVW && (Input.WasPressed(VK_LBUTTON) || Input.WasPressed(VK_RBUTTON)))
			{
				mActiveViewportWindow = HoveredVW;
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
			SViewportWindow* GizmoTargetVW = ViewportClient->mGizmo.IsDragging() ? mActiveViewportWindow : HoveredVW;

			if (GizmoTargetVW)
			{
				float Ratio = GizmoTargetVW->bIsOrthographic ? 0.0f : 1.0f;
				FMatrix GizmoVP = GizmoTargetVW->Camera.GetViewMatrix() *
					GizmoTargetVW->Camera.GetUnifiedProjectionMatrix(
						GizmoTargetVW->Rect.Width / GizmoTargetVW->Rect.Height,
						GizmoTargetVW->Camera.mFovDegree,
						GizmoTargetVW->Camera.mOrthoDistance, 0.1f, 1000.f, Ratio);

				ViewportClient->mGizmo.Update(mSceneManager, GizmoVP, GizmoTargetVW->Rect);

				if (ViewportClient->mGizmo.IsDragging() && HoveredVW)
				{
					mActiveViewportWindow = HoveredVW;
				}
			}

			if (HoveredVW && !ViewportClient->mGizmo.IsDragging() && !ViewportClient->mGizmo.IsMouseOverHandle())
			{
				int32 SubMouseX = static_cast<int32>(LocalMouse.X - HoveredVW->Rect.X);
				int32 SubMouseY = static_cast<int32>(LocalMouse.Y - HoveredVW->Rect.Y);
				float Ratio = HoveredVW->bIsOrthographic ? 0.0f : 1.0f;

				AActor* HitActor = ViewportClient->PerformMousePicking(
					HoveredVW->Camera,
					SubMouseX, SubMouseY,
					HoveredVW->Rect.Width, HoveredVW->Rect.Height,
					Ratio,
					RenderCollector
				);

				if (Input.WasPressed(VK_LBUTTON))
				{
					if (HitActor) mSceneManager->SetSelectedActor(HitActor);
					else mSceneManager->ResetSelectedActor();
				}
			}
		}
		// 4. 기즈모 업데이트는 현재 '활성화된 뷰포트'의 카메라 행렬 전달
		// 뷰포트가 ImGui 창이 되면서 그 위에서는 io.WantCaptureMouse 가 항상 true 다.
		// 그대로 두면 씬을 클릭해도 선택이 되지 않는다. 카메라/기즈모와 같은 기준을 쓴다.
	}

	//Render Threads
	{
		if (WindowApplication.bPendingResize)
		{
			mGraphicsManager->OnResize(WindowApplication.PendingWidth, WindowApplication.PendingHeight);
			WindowApplication.bPendingResize = false;
		}

		mGraphicsManager->Update(deltaTime);

		const float TotalW = mSceneManager->GetViewportWidth();
		const float TotalH = mSceneManager->GetViewportHeight();

		const float SplitX = TotalW * mTopSplitter->SplitRatio;
		const float SplitY = TotalH * mRootSplitter->SplitRatio;

		if (TotalW > 0.0f && TotalH > 0.0f)
		{
			mGraphicsManager->ResizeSceneRenderTarget(static_cast<UINT>(TotalW), static_cast<UINT>(TotalH));

			if (mRootSplitter)
			{
				mRootSplitter->UpdateLayout(FRect(0.0f, 0.0f, TotalW, TotalH));
			}
		}

		SViewportWindow* GizmoTargetVW = ViewportClient->mGizmo.IsDragging() ? mActiveViewportWindow : HoveredVW;
		if (!GizmoTargetVW)
		{
			GizmoTargetVW = mActiveViewportWindow;

		}
		for (int i = 0; i < mViewportWindows.Num(); ++i)
		{
			SViewportWindow* VW = mViewportWindows[i];
			if (VW == nullptr)
				continue;
			if (VW->Rect.Width <= 0.0f || VW->Rect.Height <= 0.0f) continue;

			bool bClear = (i == 0);
			float Ratio = VW->bIsOrthographic ? 0.0f : 1.0f;

			mGraphicsManager->Prepare(&VW->Camera, VW->Rect.Width, VW->Rect.Height, Ratio, bClear);

			D3D11_VIEWPORT D3DViewport = VW->GetD3DViewport();
			mGraphicsManager->GetRenderer()->GetDeviceContext()->RSSetViewports(1, &D3DViewport);

			mGraphicsManager->Render();

			if (mSceneManager->GetSelectedActor())
			{
				FRenderInfo clickedRenderInfo;
				mSceneManager->GetSelectedActor()->GetFirstRenderInfo(clickedRenderInfo);
				mGraphicsManager->RenderHighLight(clickedRenderInfo);
			}

			bool bRecord = (VW == GizmoTargetVW);
			ViewportClient->mGizmo.Render(mSceneManager, VW->Camera.Transform.Location, mGraphicsManager->GetViewProjectionMatrix(), bRecord);
		}

		mGraphicsManager->GetRenderCollector().Clear();

		//ImGui
		{
			//ImGui Input
			mSceneManager->UpdateGUI({ *FrameTimer, mGraphicsManager, ViewportClient, mFileManager, mAssetManager, mTopSplitter ? mTopSplitter->SplitRatio : 0.5f, mRootSplitter ? mRootSplitter->SplitRatio : 0.5f, SplitterDragState });

			if (SplitterDragState == ESplitterDragState::Cross)
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
			}
			else if (SplitterDragState == ESplitterDragState::Vertical)
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			}
			else if (SplitterDragState == ESplitterDragState::Horizontal)
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
			}

			mGraphicsManager->GetRenderer()->BindFrameBuffer();

			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		}

		mGraphicsManager->Display();
	}

	FrameTimer->EndFrame();

	GInTick = false;
}

void FEngineLoop::End()
{
	const std::string Value = std::format("{:.6f}", ViewportClient->GetCamera().Sensitivity);

	if (!WritePrivateProfileStringA("Camera", "Sensitivity", Value.c_str(), ".\\editor.ini"))
	{
		UE_LOG_ERROR("Failed to save camera sensitivity to editor.ini");
	}

	const std::string ValueGrid = std::format("{:6d}", mGraphicsManager->GetGridGap());

	if (!WritePrivateProfileStringA("Grid", "Gap", ValueGrid.c_str(), ".\\editor.ini"))
	{
		UE_LOG_ERROR("Failed to save grid gap to editor.ini");
	}
	mSceneManager->DeleteScene();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	delete mComponentVisualizerManager;
	delete ViewportClient;
	delete FrameTimer;
	delete mSceneManager;
	delete mFileManager;
	delete mAssetManager;
	delete mFontManager;

	delete mGraphicsManager;
}
