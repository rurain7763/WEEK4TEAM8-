#pragma once

#include "ImGui/imgui.h"
#include "NativeFileDialog.h"
#include "AssetFileIOs.h"
#include <filesystem>
#include "FTexture2DImporter.h"
#include "FStaticMeshImporter.h"
#include "FMaterialImporter.h"
#include "FLogManager.h"


struct FContentBrowserEventHandler
{
	virtual void OnNewAssetFile(const FAssetFileHeader& Header, const std::filesystem::path& FilePath) {}
	virtual void OnDeleteAssetFile(const std::filesystem::path& FilePath) {}
};

class FContentBrowser
{
public:
	void Initialize(const std::filesystem::path& InitDirectory)
	{
		RootDirectory = InitDirectory;
		CurrentDirectory = InitDirectory;
	}

	void SetEventHandler(FContentBrowserEventHandler* InEventHandler)
	{
		EventHandler = InEventHandler;
	}

	void Render()
	{
		struct FAssetFileEntry
		{
			FAssetFileHeader Header;
			std::filesystem::path FilePath;
		};

		TArray<FAssetFileEntry> NewAssetFiles;
		TArray<std::filesystem::path> DeletedAssetFiles;

		ImGui::Begin("Content Browser");
		
		if (ImGui::Button("Import Texture2D"))
		{
			std::filesystem::path TargetPath;
			FAssetFileHeader Header;
			
			if (FNativeFileDialog::OpenFileDialog(CurrentDirectory, { FFileFilter{L"Image Files", L"*.png;*.jpg;"} }, L"", TargetPath))
			{
				std::filesystem::path NewFilePath = TargetPath;
				NewFilePath.replace_extension(".uasset");   // CurrentDirectory 대신 원본 옆에 저장
				
				if(FTexture2DImporter::Import(TargetPath, NewFilePath, Header))
					NewAssetFiles.Emplace(FAssetFileEntry{ Header, NewFilePath });
			}

			#if 0
			if (FNativeFileDialog::OpenFileDialog(CurrentDirectory, { FFileFilter{L"Image Files", L"*.png;*.jpg;"} }, L"", TargetPath))
			{
				FImagePayload ImagePayload;
				if (FImageFileIO::Load(TargetPath, ImagePayload))
				{
					std::filesystem::path NewFilePath = CurrentDirectory / (TargetPath.stem().string() + ".uasset");
					// TODO: 파일 이름이 중복되는 경우 처리 필요
				
					FWindowsBinWriter FileWriter(NewFilePath);
				
					FAssetFileHeader Header;
					Header.Version = 1;
					Header.AssetType = EAssetType::Texture2D;
					Header.AssetID = FGuid::NewGuid();

					FileWriter << Header;
					if (FImageFileIO::Save(FileWriter, ImagePayload))
					{
						NewAssetFiles.Emplace(FAssetFileEntry{ Header, NewFilePath });
					}
				}
			}
			#endif
		}
		if (ImGui::Button("Import StaticMesh"))
		{
			std::filesystem::path TargetPath;
			FAssetFileHeader Header;
			
			if (FNativeFileDialog::OpenFileDialog(CurrentDirectory, { FFileFilter{L"Obj Files", L"*.obj;"} }, L"", TargetPath))
			{
				std::filesystem::path NewFilePath = TargetPath;
				NewFilePath.replace_extension(".uasset");   // CurrentDirectory 대신 원본 옆에 저장
				
				if(FStaticMeshImporter::Import(TargetPath, NewFilePath, Header))
					NewAssetFiles.Emplace(FAssetFileEntry{ Header, NewFilePath });
			}
			
		}
		if (ImGui::Button("Import Material"))
		{
			std::filesystem::path TargetPath;
			FAssetFileHeader Header;
			
			if (FNativeFileDialog::OpenFileDialog(CurrentDirectory, { FFileFilter{L"MTL Files", L"*.mtl;"} }, L"", TargetPath))
			{
				FObjInfo ObjInfo;
				FObjImporter Importer;
				Importer.ParseMtl(TargetPath.string(), ObjInfo);

				for (const FObjMaterialInfo& Material : ObjInfo.Materials)
				{
					std::filesystem::path MaterialPath = TargetPath.parent_path() / (TargetPath.stem().string() + "_" + Material.Name.CStr() + ".uasset");
					if(FMaterialImporter::Import(Material, TargetPath.parent_path(), MaterialPath, Header))
						NewAssetFiles.Emplace(FAssetFileEntry{ Header, MaterialPath });
				}
			}
		}

		ImGui::Separator();

		ImGui::Text("Current Directory: %s", CurrentDirectory.string().c_str());

		for (const auto& Entry : std::filesystem::directory_iterator(CurrentDirectory))
		{
			if (Entry.is_directory())
			{
				if (ImGui::Button(Entry.path().filename().string().c_str()))
				{
					CurrentDirectory = Entry.path();
				}
			}
			else
			{
				std::filesystem::path Extension = Entry.path().extension();
				if (Extension == ".uasset")
				{
					std::filesystem::path FileName = Entry.path().filename();
					ImGui::Text("%s", FileName.string().c_str());
				}
			}
		}

		if (ImGui::Button("<-"))
		{
			if (CurrentDirectory != RootDirectory)
			{
				CurrentDirectory = CurrentDirectory.parent_path();
			}
		}

		ImGui::End();

		// Dispatch events for new and deleted asset files
		if (EventHandler)
		{
			for (const auto& NewFile : NewAssetFiles)
			{
				EventHandler->OnNewAssetFile(NewFile.Header, NewFile.FilePath);
			}

			for (const auto& DeletedFile : DeletedAssetFiles)
			{
				EventHandler->OnDeleteAssetFile(DeletedFile);
			}
		}
	}

private:
	std::filesystem::path RootDirectory;
	std::filesystem::path CurrentDirectory;

	FContentBrowserEventHandler* EventHandler = nullptr;
};