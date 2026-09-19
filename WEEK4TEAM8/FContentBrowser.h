#pragma once

#include "ImGui/imgui.h"
#include "NativeFileDialog.h"
#include "AssetFileIOs.h"
#include <filesystem>

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