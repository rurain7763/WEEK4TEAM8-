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
		mViewportManager = new FViewportManager();
		mViewportManager->Init();
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

		if (SViewportWindow* ActiveVW = mViewportManager->GetActiveViewport())
		{
			float ActiveRatio = ActiveVW->bIsOrthographic ? 0.0f : 1.0f;
			ViewportClient->Update(deltaTime, ActiveVW->Camera, ActiveRatio, mSceneManager);
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
		mViewportManager->Tick(deltaTime, mSceneManager, ViewportClient, &RenderCollector, WindowApplication.Input);
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

		if (TotalW > 0.0f && TotalH > 0.0f)
		{
			mGraphicsManager->ResizeSceneRenderTarget(static_cast<UINT>(TotalW), static_cast<UINT>(TotalH));
		}

		mViewportManager->Render(mGraphicsManager, mSceneManager, ViewportClient);

		mGraphicsManager->GetRenderCollector().Clear();
		//ImGui
		{
			//ImGui Input
			mSceneManager->UpdateGUI({ *FrameTimer, mGraphicsManager, ViewportClient, mFileManager, mAssetManager, mViewportManager->GetSplitRatioX(), mViewportManager->GetSplitRatioY(), mViewportManager->GetDragState()});

			const EViewportLayoutMode TargetMode = mSceneManager->GetMaximizeWindow()
				? EViewportLayoutMode::Quad
				: EViewportLayoutMode::Single;
			mViewportManager->SetLayoutMode(TargetMode, mSceneManager);

			mViewportManager->UpdateMouseCursor();

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

	delete mViewportManager;
	delete mComponentVisualizerManager;
	delete ViewportClient;
	delete FrameTimer;
	delete mSceneManager;
	delete mFileManager;
	delete mAssetManager;
	delete mFontManager;

	delete mGraphicsManager;
}
