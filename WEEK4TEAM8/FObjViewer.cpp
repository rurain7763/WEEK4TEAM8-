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
#include "FAssetManager.h"
#include "FArchive.h"
#include "UStaticMesh.h"
#include "FObjManager.h"

void FObjViewer::Initialize(FSceneManager& InSceneManager, URenderer& Renderer, FFileManager& InFileManager)
{
	FObjManager::Initialize(Renderer, InFileManager);
	mSceneManager = &InSceneManager;
	FShowFlags::Get().SetEnabled(EShowFlag::UUIDText, false); 
}

void FObjViewer::UpdateObjGUI(FGraphicsManager& InGraphicsManager)
{
	ImGui::Begin("OBJ Viewer");

	if (mViewerComponent && mViewerComponent->GetMesh())
	{
		const auto& meshAsset = mViewerComponent->GetMesh();

		ImGui::Text("Vertices: %u", meshAsset->GetVertices().Num());
		//ImGui::Text("Indices: %u", meshAsset->GetCpuIndices().Num());
		ImGui::Text("Triangles: %u", meshAsset->GetIndices().Num() / 3);
	}

	ImGui::SeparatorText("Transform");
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

	ImGui::SeparatorText("Appearance");
	if (mViewerComponent)
	{
		bool bUseVertexColor = mViewerComponent->GetUseVertexColor();
		if (ImGui::Checkbox("Use Vertex Color", &bUseVertexColor))
		{
			mViewerComponent->SetUseVertexColor(bUseVertexColor);
		}

		FVector4 Color = mViewerComponent->GetColor();

		if (ImGui::ColorEdit4("Color", &Color.x))
		{
			mViewerComponent->SetColor(Color);
		}

	}

	ImGui::SeparatorText("Materials");

	if (mViewerComponent && mViewerComponent->GetMesh())
	{
		const TSharedPtr<FStaticMeshAsset>& MeshAsset = mViewerComponent->GetMesh();

		for (int32 SectionIndex = 0; SectionIndex < MeshAsset->GetSections().Num(); ++SectionIndex)
		{
			const FStaticMeshSection& Section = MeshAsset->GetSections()[SectionIndex];
			TSharedPtr<FMaterialAsset> Material = FAssetManager::Get().GetAssetAs<FMaterialAsset>(Section.MaterialAssetID, true);

			ImGui::Text("Section %d", SectionIndex);

			if (!Material)
			{
				ImGui::TextDisabled("Material: None");
				continue;
			}

			ImGui::Text("Material: %s", Material->GetAssetName().ToString().CStr());

			TSharedPtr<FTexture2DAsset> Texture = Material->GetDiffuseTexture();
			if (!Texture)
			{
				ImGui::TextDisabled("Diffuse Texture: None");
				continue;
			}

			ImGui::Text("Diffuse Texture: %s", Texture->GetAssetName().ToString().CStr());

			ImGui::Image( reinterpret_cast<ImTextureID>(Texture->GetSRV().Get()),ImVec2(96.0f, 96.0f));
		}
	}

	ImGui::SeparatorText("File");
	if (ImGui::Button("Open OBJ"))
	{
		std::filesystem::path targetPath;

		try
		{
			if (FNativeFileDialog::OpenFileDialog(kDefaultOBJPath,
				{ FFileFilter{ L"OBJ Files", L"*.obj" } }, L"obj", targetPath))
			{
				OpenObj(targetPath);
				UE_LOG("Successed to load: %s", targetPath.string().c_str());
			}
		}
		catch (const std::exception& e)
		{
			UE_LOG_ERROR("Failed to load: %s", e.what());
		}
	}

	if (ImGui::Button("Open UAsset"))
	{
		std::filesystem::path targetPath;

		try
		{
			if (FNativeFileDialog::OpenFileDialog(kDefaultOBJPath,
				{ FFileFilter{ L"UAsset Files", L"*.uasset" } }, L"obj", targetPath))
			{
				OpenStaticMeshAsset(targetPath);
				UE_LOG("Successed to load: %s", targetPath.string().c_str());
			}
		}
		catch (const std::exception& e)
		{
			UE_LOG_ERROR("Failed to load: %s", e.what());
		}
	}

	ImGui::SeparatorText("View Mode");
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

void FObjViewer::OpenObj(const std::filesystem::path& FilePath)
{
	const FString AssetPath(FilePath.string());

	if (!mSceneManager)
	{
		return;
	}

	if (mViewerActor)
	{
		mSceneManager->ResetSelectedActor();

		mSceneManager->GetCurrentWorld()->RemoveActor(mViewerActor->UUID);
		mViewerActor = nullptr;
	}

	mViewerActor = FObjectFactory::ConstructObject<AActor>();

	UStaticMeshComponent* objComponent =
		FObjectFactory::ConstructObject<UStaticMeshComponent>(
			AssetPath,
			FVector(0, 0, 0),
			FRotator(0, 0, 0),
			FVector(1, 1, 1));


	/*FStaticMeshFileIO::Load(FilePath, Payload)

	objComponent->SetMesh()*/
	mViewerComponent = objComponent;
	mViewerActor->AddRootSceneComponent(objComponent);
	mSceneManager->GetCurrentWorld()->AddActor(mViewerActor);
	//mSceneManager->SetSelectedActor(mViewerActor);

	mLoadedFilePath = AssetPath;
}

void FObjViewer::OpenStaticMeshAsset(const std::filesystem::path& FilePath)
{
	FAssetFileHeader Header;

	try
	{
		FWindowsBinReader Reader(FilePath);
		Reader << Header;
	}
	catch (const std::exception& e)
	{
		UE_LOG_ERROR("Failed to read asset: %s", FilePath.string().c_str());
		return;
	}

	if (Header.AssetType != EAssetType::StaticMesh)
	{
		UE_LOG_ERROR("Selected file is not a StaticMesh: %s", FilePath.string().c_str());
		return;
	}

	TSharedPtr<FStaticMeshAsset> MeshAsset = 
		FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(Header.AssetID, true);

	if (!MeshAsset)
	{
		UE_LOG_ERROR("Failed to load StaticMesh asset: %s", FilePath.string().c_str());
		return;
	}

	if (mViewerActor)
	{
		mSceneManager->ResetSelectedActor();
		mSceneManager->GetCurrentWorld()->RemoveActor(mViewerActor->UUID);
		mViewerActor = nullptr;
	}

	UStaticMesh* StaticMesh = FObjectFactory::ConstructUnInitializedObject<UStaticMesh>();

	StaticMesh->SetCookedStaticMeshAsset(MeshAsset, FString(FilePath.string()), {});

	UStaticMeshComponent* Component = FObjectFactory::ConstructObject<UStaticMeshComponent>(FVector(0, 0, 0), FRotator(0, 0, 0), FVector(1, 1, 1));
	Component->SetMesh(MeshAsset);

	mViewerActor = FObjectFactory::ConstructObject<AActor>();
	mViewerActor->AddRootSceneComponent(Component);
	mSceneManager->GetCurrentWorld()->AddActor(mViewerActor);

	mViewerComponent = Component;
	mLoadedFilePath = FString(FilePath.string());
}