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
#include "Serializers.h"

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

	mEditorLayout.Initialize(FRect(0, 0, (float)clientWidth, (float)clientHeight));
	
	mMainViewport.Window = mEditorLayout.RootWindow;
	mMainViewport.Viewport = MakeShared<FViewport>();
	mMainViewport.Viewport->Resize(*mGraphicsManager->GetRenderer(), clientWidth, clientHeight);
	mMainViewport.Client = MakeShared<FEditorViewportClient>(*mGraphicsManager->GetRenderer());

	for (int32 i = 0; i < 4; ++i)
	{
		mSplitViewports[i].Window = mEditorLayout.ViewportWindows[i];
		mSplitViewports[i].Viewport = MakeShared<FViewport>();
		mSplitViewports[i].Viewport->Resize(*mGraphicsManager->GetRenderer(), mEditorLayout.ViewportWindows[i]->Rect.Width, mEditorLayout.ViewportWindows[i]->Rect.Height);
		mSplitViewports[i].Client = MakeShared<FEditorViewportClient>(*mGraphicsManager->GetRenderer());
	}

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
	sscanf_s(Value, "%d", &	GridGap);
	mGraphicsManager->SetGridGap(GridGap);

	mSceneManager->NewScene();

	{
		AActor* ObjActor = FObjectFactory::ConstructObject<AActor>();
		UStaticMeshComponent* ObjComponent = 
			FObjectFactory::ConstructObject<UStaticMeshComponent>(FString("Assets/Meshes/TestTriangle.obj"),
			FVector(0, 0, 0), FRotator(0, 0, 0), FVector(1, 1, 1));


		ObjActor->AddComponent(ObjComponent);
		mSceneManager->GetCurrentWorld()->AddActor(ObjActor);
	}
}

void FEngineLoop::InitAssetManager()
{
	mAssetManager = new FAssetManager();

	URenderer* renderer = mGraphicsManager->GetRenderer();

	FObjManager::Initialize(*renderer, *mFileManager);
	FAssetManager::Get().ScanDirectory("Assets", *renderer);

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

	// ScanDirectory로 파일 자동 스캔하여 uasset 등록하므로 아래 줄과 중복되어 삭제해도 되나,
	// 참고하고 있는 곳이 있어서 ScanDirectory와 동일한 파일명 규칙으로 수정해 둠.

	#if 0
	// TSharedPtr<FFileAssetSource> FileAssetSource = MakeShared<FFileAssetSource>("Assets/Textures/Test.jpg");
	// mAssetManager->RegisterAsset(FGuid::NewGuid(), FName("TestTexture"), TextureLoader, FileAssetSource);

	// TSharedPtr<FFileAssetSource> SpotLightIconAssetSource = MakeShared<FFileAssetSource>("Assets/Textures/Icon_SpotLight.png");
	// mAssetManager->RegisterAsset(FGuid::NewGuid(), FName("SpotLightIcon"), TextureLoader, SpotLightIconAssetSource);

	// TSharedPtr<FFileAssetSource> ExplosionTextureSource = MakeShared<FFileAssetSource>("Assets/Textures/ExplosionAtlas.png");
	// mAssetManager->RegisterAsset(FGuid::NewGuid(), FName("ExplosionTexture"), TextureLoader, ExplosionTextureSource);
	#endif

	FName ExplosionTextureName(std::filesystem::weakly_canonical("Assets/Textures/ExplosionAtlas.uasset").string());
	TSharedPtr<FTexture2DAsset> ExplosionTexture2DAsset = mAssetManager->GetAssetAs<FTexture2DAsset>(ExplosionTextureName, true);

	TSharedPtr<FSpriteAtlasAsset> ExplosionSpriteAtlasAsset = MakeShared<FSpriteAtlasAsset>(FGuid::NewGuid(), FName("ExplosionSpriteAtlas"), *renderer, ExplosionTexture2DAsset, 6, 6);
	mAssetManager->RegisterAsset(ExplosionSpriteAtlasAsset);

	TSharedPtr<FFileAssetSource> FontAssetSource = MakeShared<FFileAssetSource>("Assets/Fonts/BMKkubulimTTF.ttf");
	mAssetManager->RegisterAsset(FGuid::NewGuid(), FName("TestFont"), FontLoader, FontAssetSource);
	
	TSharedPtr<FFontAsset> TestFontAsset = mAssetManager->GetAssetAs<FFontAsset>(FName("TestFont"), true);
	TSharedPtr<FFontAtlasAsset> FontAtlasAsset = MakeShared<FFontAtlasAsset>(FGuid::NewGuid(), FName("TestFontAtlas"), *renderer, TestFontAsset, 512, 512, 2, 2);
	mAssetManager->RegisterAsset(FontAtlasAsset);

	// Register asset files
	for (const auto& entry : std::filesystem::recursive_directory_iterator(kDefaultAssetsPath))
	{
		if (entry.is_regular_file())
		{
			std::filesystem::path FilePath = entry.path();
			std::filesystem::path Extension = FilePath.extension();
			
			if (Extension != ".uasset")
			{
				continue;
			}

			FWindowsBinReader Reader(FilePath);
			
			FAssetFileHeader Header;
			Reader << Header;

			switch (Header.AssetType)
			{
				case EAssetType::Texture2D:
				{
					TSharedPtr<FFileAssetSource> TextureSource = MakeShared<FFileAssetSource>(FilePath);
					mAssetManager->RegisterAsset(FName(FilePath.stem().string()), TextureLoader, TextureSource);
				}
				break;
			}
		}
	}
}

void FEngineLoop::Tick(bool bPumpMessages)
{
	if (GInTick) return;
	GInTick = true;

	FrameTimer->StartFrame();
	float deltaTime = FrameTimer->GetDeltaTime();

	FRenderCollector& RenderCollector = mGraphicsManager->GetRenderCollector();

	WindowApplication.ProcessDeferredEvents();

	if (WindowApplication.bPendingResize)
	{
		mGraphicsManager->OnResize(WindowApplication.PendingWidth, WindowApplication.PendingHeight);
		WindowApplication.bPendingResize = false;
	}

	mSceneManager->Tick(deltaTime);

	mGraphicsManager->UpdateProjectionTransition(deltaTime);

	const float NearZ = 0.1f;
	const float FarZ = 2000.0f;
	const int32 ViewportCount = mEditorLayout.bIsSplitView ? 4 : 1;
	
	for (int32 i = 0; i < ViewportCount; ++i)
	{
		FEditorViewport* CurrentViewport = !mEditorLayout.bIsSplitView ? &mMainViewport : &mSplitViewports[i];

		const FRect& ViewportRect = CurrentViewport->Window->Rect;

		FCamera& Camera = CurrentViewport->Client->GetCamera();
		Camera.mAspect = ViewportRect.Width / ViewportRect.Height;
		Camera.mNear = NearZ;
		Camera.mFar = FarZ;

		RenderCollector.Clear();
		RenderCollector.Camera = &Camera;

		CurrentViewport->Client->Update(deltaTime, mGraphicsManager->GetPerspectiveRatio(), RenderCollector);

		FMatrix ViewProjection = Camera.GetViewMatrix() * Camera.GetProjectionMatrix();

		mSceneManager->Render(deltaTime, RenderCollector);

		// 마우스 피킹 처리
		// 뷰포트가 ImGui 창이 되면서 그 위에서는 io.WantCaptureMouse 가 항상 true 다.
		// 그대로 두면 씬을 클릭해도 선택이 되지 않는다. 카메라/기즈모와 같은 기준을 쓴다.
		const FInputState& Input = WindowApplication.Input;
		if (CurrentViewport->Client->IsActive() && Input.WasPressed(VK_LBUTTON) && !CurrentViewport->Client->mGizmo.IsDragging() && !CurrentViewport->Client->mGizmo.IsMouseOverHandle())
		{
			AActor* HitActor = CurrentViewport->Client->PerformMousePicking(CurrentViewport->Window->Rect, mGraphicsManager->GetPerspectiveRatio(), RenderCollector);
			if (HitActor)
			{
				mSceneManager->SetSelectedActor(HitActor);
			}
			else
			{
				mSceneManager->ResetSelectedActor();
			}
		}

		// 선택된 액터 처리
		AActor* SelectedActor = mSceneManager->GetSelectedActor();
		if (SelectedActor)
		{
			FTransform Transform = SelectedActor->GetTransform();

			for (UActorComponent* Component : SelectedActor->GetComponents())
			{
				UPrimitiveComponent* PrimitiveComponent = Component->Cast<UPrimitiveComponent>();
				if (PrimitiveComponent)
				{
					// 선택된 액터의 AABB를 화면에 표시
					FMatrix WorldMatrix = Transform.MakeMatrix();

					TSharedPtr<FStaticMeshAsset> MeshAsset = PrimitiveComponent->GetMesh();
					if (!MeshAsset.get()) continue;

					const FAABB& AABB = MeshAsset->GetLocalBoundingBox().ToWorld(WorldMatrix);

					AABB.ForEachCornerLines([&RenderCollector](const FVector& Start, const FVector& End)
						{
							FVector4 WorldStart = FVector4(Start, 1.f);
							FVector4 WorldEnd = FVector4(End, 1.f);

							FRenderLineInfo LineInfo;
							LineInfo.Start = WorldStart.ToVec3();
							LineInfo.End = WorldEnd.ToVec3();
							LineInfo.Color = FVector4(1.f, 0.f, 0.f, 1.f); // 빨간색
							LineInfo.Thickness = 5.0f;

							RenderCollector.LineInfos.Add(LineInfo);
						});
				}

				// 선택된 액터의 컴포넌트 시각화
				FComponentVisualizer* Visualizer = mComponentVisualizerManager->FindVisualizer(Component->GetRuntimeClass());
				if (Visualizer)
				{
					Visualizer->VisualizeComponent(Component, RenderCollector);
				}
			}

			CurrentViewport->Client->mGizmo.Update(SelectedActor, CurrentViewport->Window->Rect, CurrentViewport->Client->IsActive(), ViewProjection);
		}

		//Render Threads
		{
			CurrentViewport->Viewport->Resize(*mGraphicsManager->GetRenderer(), ViewportRect.Width, ViewportRect.Height);
			mGraphicsManager->Prepare(&CurrentViewport->Client->mCamera, ViewportRect.Width, ViewportRect.Height, *CurrentViewport->Viewport);
			mGraphicsManager->Render();

			//강조
			if (mSceneManager->GetSelectedActor())
			{
				FRenderInfo clickedRenderInfo;
				mSceneManager->GetSelectedActor()->GetFirstRenderInfo(clickedRenderInfo);
				mGraphicsManager->RenderHighLight(clickedRenderInfo);
			}

			CurrentViewport->Client->mGizmo.Render(SelectedActor, CurrentViewport->Client->mCamera.Transform.Location, ViewProjection);
		}
	}

	FGuiReference GuiReference;
	GuiReference.FrameTimer = FrameTimer;
	GuiReference.GraphicsManager = mGraphicsManager;
	GuiReference.ViewportClient = mMainViewport.Client.get();
	GuiReference.FileManager = mFileManager;
	GuiReference.AssetManager = mAssetManager;
	GuiReference.EditorLayout = &mEditorLayout;
	
	if (mEditorLayout.bIsSplitView)
	{
		GuiReference.Viewports = mSplitViewports;
		GuiReference.ViewportCount = 4;
	}
	else
	{
		GuiReference.Viewports = &mMainViewport;
		GuiReference.ViewportCount = 1;
	}

	mSceneManager->UpdateGUI(GuiReference);

	FRect ViewportRect;
	ViewportRect.X = mSceneManager->GetViewportX();
	ViewportRect.Y = mSceneManager->GetViewportY();
	ViewportRect.Width = mSceneManager->GetViewportWidth();
	ViewportRect.Height = mSceneManager->GetViewportHeight();
	mEditorLayout.Resize(ViewportRect);

	mGraphicsManager->GetRenderer()->BindFrameBuffer();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	mGraphicsManager->Display();
	
	FrameTimer->EndFrame();

	GInTick = false;
}

void FEngineLoop::End()
{
	const std::string Value = std::format("{:.6f}", mMainViewport.Client->GetCamera().Sensitivity);

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

	mMainViewport.Release();
	for (FEditorViewport& Viewport : mSplitViewports)
	{
		Viewport.Release();
	}

	delete mComponentVisualizerManager;
	delete FrameTimer;
	delete mSceneManager;
	delete mFileManager;
	delete mAssetManager;
	delete mFontManager;

	delete mGraphicsManager;
}
