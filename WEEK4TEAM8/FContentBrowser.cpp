#include "FContentBrowser.h"
#include "ImGui/imgui_internal.h"

void FContentBrowser::Initialize(const std::filesystem::path& InitDirectory)
{
	RootDirectory = InitDirectory;
	CurrentDirectory = InitDirectory;
}

void FContentBrowser::SetEventHandler(FContentBrowserEventHandler* InEventHandler)
{
	EventHandler = InEventHandler;
}

/*void FContentBrowser::Render()
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
		if (FNativeFileDialog::OpenFileDialog(CurrentDirectory, { FFileFilter{ L"Image Files", L"*.png;*.jpg;" } }, L"", TargetPath))
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
}*/

void FContentBrowser::Render()
{
	if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_Space, false))
	{
		ToggleDrawer();
	}

	RenderBottomBar();

	if (bIsDrawerOpen)
	{
		RenderDrawer();
	}
}

void FContentBrowser::RenderBottomBar()
{
	const ImGuiViewport* Viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(ImVec2(Viewport->WorkPos.x, Viewport->WorkPos.y + Viewport->WorkSize.y - BottomBarHeight));
	ImGui::SetNextWindowSize(ImVec2(Viewport->WorkSize.x, BottomBarHeight));
	ImGui::SetNextWindowViewport(Viewport->ID);

	const ImGuiWindowFlags BottomBarFlags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoScrollbar;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f,3.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(24, 24, 24, 255));

	if (ImGui::Begin("##EditorBottomBar", nullptr, BottomBarFlags))
	{
		if (bIsDrawerOpen)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(230, 120, 0, 255));
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(45, 45, 48, 255));
		}

		if (ImGui::Button("[ Content Drawer] (Ctrl+Space)"))
		{
			ToggleDrawer();
		}

		ImGui::PopStyleColor();

		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 250.0f);
		ImGui::TextDisabled("Path: %s", CurrentDirectory.filename().string().c_str());
	}

	ImGui::End();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}

void FContentBrowser::RenderDrawer()
{
	const ImGuiViewport* Viewport = ImGui::GetMainViewport();


	const ImVec2 DrawerPos = { Viewport->WorkPos.x, Viewport->WorkPos.y + Viewport->WorkSize.y - BottomBarHeight - DrawerHeight };
	const ImVec2 DrawerSize = { Viewport->WorkSize.x, DrawerHeight };

	ImGui::SetNextWindowPos(DrawerPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(DrawerSize, ImGuiCond_Always);
	ImGui::SetNextWindowViewport(Viewport->ID);

	const ImGuiWindowFlags DrawerFlags =
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoDocking;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(32, 32, 32, 245));

	if (ImGui::Begin("Content Drawer", &bIsDrawerOpen, DrawerFlags))
	{
		if (ImGui::Button("Import Texture2D"))
		{
			std::filesystem::path TargetPath;
			if (FNativeFileDialog::OpenFileDialog(CurrentDirectory, { FFileFilter{ L"Image Files", L"*.png;*.jpg;" } }, L"", TargetPath))
			{
				FImagePayload ImagePayload;
				if (FImageFileIO::Load(TargetPath, ImagePayload))
				{
					std::filesystem::path NewFilePath = CurrentDirectory;
					FWindowsBinWriter FileWriter(NewFilePath);

					FAssetFileHeader Header;
					Header.Version = 1;
					Header.AssetType = EAssetType::Texture2D;
					Header.AssetID = FGuid::NewGuid();

					FileWriter << Header;
					if (FImageFileIO::Save(FileWriter, ImagePayload))
					{
						if (EventHandler) EventHandler->OnNewAssetFile(Header, NewFilePath);
					}
				}
			}
		}

		ImGui::SameLine();
		ImGui::Text(" | Current: %s", CurrentDirectory.string().c_str());

		ImGui::SameLine(ImGui::GetWindowWidth() - 70.0f);
		if (ImGui::Button("Close"))
		{
			bIsDrawerOpen = false;
		}

		ImGui::Separator();

		if (ImGui::BeginChild("FolderTreePanel", ImVec2(220.0f, 0.0f), true))
		{
			ImGui::TextDisabled("FOLDERS");
			ImGui::Separator();

			RenderFolderNode(RootDirectory);
		}
		ImGui::EndChild();

		ImGui::SameLine();

		if (ImGui::BeginChild("AssetContentPanel", ImVec2(0.0f, 0.0f), true))
		{
			ImGui::Text("Current Path: %s", CurrentDirectory.string().c_str());
			ImGui::Separator();

			for (const auto& Entry : std::filesystem::directory_iterator(CurrentDirectory))
			{
				if (Entry.is_directory())
				{
					ImGui::BulletText("[Folder] %s", Entry.path().filename().string().c_str());
				}
				else
				{
					ImGui::BulletText("[File] %s", Entry.path().filename().string().c_str());
				}
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();

	ImGui::PopStyleColor();
}

void FContentBrowser::RenderFolderNode(const std::filesystem::path& DirectoryPath)
{
	bool bHasSubDirectories = false;

	try
	{
		for (const auto& Entry : std::filesystem::directory_iterator(DirectoryPath))
		{
			if (Entry.is_directory())
			{
				bHasSubDirectories = true;
				break;
			}
		}
	}
	catch (...) 
	{
		//UELOG 추가
	}

	ImGuiTreeNodeFlags NodeFlags =
		ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_OpenOnDoubleClick |
		ImGuiTreeNodeFlags_SpanAvailWidth;

	if (CurrentDirectory == DirectoryPath)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	if (CurrentDirectory == DirectoryPath)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	if (!bHasSubDirectories)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	FString FolderName = DirectoryPath.filename().string();
	if (FolderName.IsEmpty())
	{
		FolderName = DirectoryPath.string();
	}

	FString PathString = DirectoryPath.string();

	bool bNodeOpen = ImGui::TreeNodeEx(PathString.c_str(), NodeFlags, "%s", FolderName.c_str());

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		CurrentDirectory = DirectoryPath;
	}

	if (bNodeOpen && bHasSubDirectories)
	{
		try
		{
			for (const auto& Entry : std::filesystem::directory_iterator(DirectoryPath))
			{
				if (Entry.is_directory())
				{
					RenderFolderNode(Entry.path());
				}
			}
		}
		catch (...) {}

		ImGui::TreePop();
	}
}