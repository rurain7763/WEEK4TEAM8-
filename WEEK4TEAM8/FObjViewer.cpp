#include "FObjViewer.h"

#include "ObjectFactory.h"
#include "Actor.h"
#include "SceneManager.h"
#include "World.h"
#include "UStaticMeshComponent.h"
#include "NativeFileDialog.h"
#include "FLogManager.h"
#include "GraphicsManager.h"
#include "ShowFlags.h"

void FObjViewer::Initialize(FSceneManager& InSceneManager)
{
	mSceneManager = &InSceneManager;
	OpenObj(FString("Assets/Meshes/TestTriangle.obj"));
	FShowFlags::Get().SetEnabled(EShowFlag::UUIDText, false);  //
}

void FObjViewer::UpdateObjGUI(FGraphicsManager& InGraphicsManager)
{
	ImGui::Begin("OBJ Viewer");

	if (mViewerComponent && mViewerComponent->GetStaticMesh())
	{
		const auto& meshAsset = mViewerComponent->GetStaticMesh()->GetStaticMeshAsset();

		ImGui::Text("Vertices: %u", meshAsset->GetCpuVertices().Num());
		ImGui::Text("Indices: %u", meshAsset->GetCpuIndices().Num());
		ImGui::Text("Triangles: %u", meshAsset->GetCpuIndices().Num() / 3);
	}

	if (mViewerActor)
	{
		const FTransform& originalTransform = mViewerActor->GetTransform();

		FVector translationInput = originalTransform.Location;
		FVector rotationInput = {
			originalTransform.Rotation.Roll,
			originalTransform.Rotation.Pitch,
			originalTransform.Rotation.Yaw
		};
		FVector scaleInput = originalTransform.Scale;

		if (ImGui::DragFloat3("Translation", &translationInput.x, 0.1f))
		{
			mViewerActor->SetLocation(translationInput);
		}
		if (ImGui::DragFloat3("Rotation", &rotationInput.x, 0.1f))
		{
			mViewerActor->SetRotation({rotationInput.y,  rotationInput.z,  rotationInput.x });
		}
		if (ImGui::DragFloat3("Scale", &scaleInput.x, 0.1f, MIN_SCALE, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp))
		{
			mViewerActor->SetScale(scaleInput);
		}
	}

	if (ImGui::Button("Reset Transform") && mViewerActor)
	{
		mViewerActor->SetLocation(FVector(0.0f, 0.0f, 0.0f));
		mViewerActor->SetRotation(FRotator(0.0f, 0.0f, 0.0f));
		mViewerActor->SetScale(FVector(1.0f, 1.0f, 1.0f));
	}

	if (ImGui::Button("Open OBJ"))
	{
		std::filesystem::path targetPath;

		try
		{
			if (FNativeFileDialog::OpenFileDialog(kDefaultOBJPath,
				{ FFileFilter{ L"OBJ Files", L"*.obj" } }, L"obj", targetPath))
			{
				OpenObj(FString(targetPath.string()));
			}
		}
		catch (const std::exception& e)
		{
			UE_LOG_ERROR("Failed to load: %s", e.what());
		}
	}

	bool bGrid = FShowFlags::Get().IsEnabled(EShowFlag::Grid);
	if (ImGui::Checkbox("Grid", &bGrid))
	{
		FShowFlags::Get().SetEnabled(EShowFlag::Grid, bGrid);
	}

	const char* viewModes[] = { "Lit", "Unlit", "Wireframe" };
	int viewMode = static_cast<int>(InGraphicsManager.GetViewModeIndex());

	if (ImGui::Combo("View Mode", &viewMode, viewModes, IM_ARRAYSIZE(viewModes)))
	{
		InGraphicsManager.SetViewModeIndex(static_cast<EViewModeIndex>(viewMode));
	}

	ImGui::End();

}

void FObjViewer::OpenObj(const FString& filePath)
{
	if (!mSceneManager)
	{
		return;
	}

	if (mViewerActor)
	{
		mSceneManager->GetCurrentWorld()->RemoveActor(mViewerActor->UUID);
		mViewerActor = nullptr;
	}

	mViewerActor = FObjectFactory::ConstructObject<AActor>();

	UStaticMeshComponent* objComponent =
		FObjectFactory::ConstructObject<UStaticMeshComponent>(
			filePath,
			FVector(0, 0, 0),
			FRotator(0, 0, 0),
			FVector(1, 1, 1));

	mViewerComponent = objComponent;
	mViewerActor->AddRootSceneComponent(objComponent);
	mSceneManager->GetCurrentWorld()->AddActor(mViewerActor);

	mLoadedFilePath = filePath;
}