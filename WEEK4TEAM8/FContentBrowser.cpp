#include "FContentBrowser.h"
#include "ImGui/imgui_internal.h"
#include "Assets.h"

void FContentBrowser::Initialize(const std::filesystem::path& InitDirectory)
{
	RootDirectory = InitDirectory;
	CurrentDirectory = InitDirectory;
}

void FContentBrowser::SetEventHandler(FContentBrowserEventHandler* InEventHandler)
{
	EventHandler = InEventHandler;
}

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
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 3.0f));
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

			const float TileWidth = 80.0f;
			const float TileHeight = 80.0f;

			std::error_code ec;
			bool bIsRoot = std::filesystem::equivalent(CurrentDirectory, RootDirectory, ec);

			if (!bIsRoot && !ec)
			{
				ImGui::BeginGroup();
				{
					ImGui::InvisibleButton("##UpFolderBtn", ImVec2(TileWidth, TileHeight));

					bool bHovered = ImGui::IsItemHovered();
					bool bActive = ImGui::IsItemActive();
					ImVec2 PMin = ImGui::GetItemRectMin();
					ImVec2 PMax = ImGui::GetItemRectMax();
					ImDrawList* DrawList = ImGui::GetWindowDrawList();

					if (ImGui::IsItemClicked())
					{
						std::error_code clickEc;
						if (!std::filesystem::equivalent(CurrentDirectory, RootDirectory, clickEc))
						{
							CurrentDirectory = CurrentDirectory.parent_path();
						}
					}

					if (bHovered || bActive)
					{
						ImU32 BgColor = bActive ? IM_COL32(255, 255, 255, 40) : IM_COL32(255, 255, 255, 20);
						DrawList->AddRectFilled(PMin, PMax, BgColor, 4.0f);
					}

					ImU32 TabColor = bActive ? 0xFF80C0E0 : (bHovered ? 0xFF99D0F0 : 0xFF70B0D0);
					ImU32 BodyColor = bActive ? 0xFF90D0F8 : (bHovered ? 0xFFAAE0FF : 0xFF80C0E8);

					ImVec2 TabMin = ImVec2(PMin.x + 8.0f, PMin.y + 12.0f);
					ImVec2 TabMax = ImVec2(PMin.x + 36.0f, PMin.y + 24.0f);
					DrawList->AddRectFilled(TabMin, TabMax, TabColor, 4.0f);

					ImVec2 BodyMin = ImVec2(PMin.x + 8.0f, PMin.y + 20.0f);
					ImVec2 BodyMax = ImVec2(PMax.x - 8.0f, PMax.y - 12.0f);
					DrawList->AddRectFilled(BodyMin, BodyMax, BodyColor, 6.0f);
					DrawList->AddRect(BodyMin, BodyMax, 0x55000000, 6.0f, 0, 1.5f);

					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (TileWidth - ImGui::CalcTextSize("..").x) * 0.5f);
					ImGui::TextUnformatted("..");
				}
				ImGui::EndGroup();

				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Go to parent directory (%s)", CurrentDirectory.parent_path().filename().string().c_str());
				}

				ImGui::SameLine();
			}

			const ImGuiStyle& Style = ImGui::GetStyle();

			const float WindowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

			int ItemIndex = 0;

			for (const auto& Entry : std::filesystem::directory_iterator(CurrentDirectory))
			{
				const std::filesystem::path& Path = Entry.path();
				FString FileName = Path.filename().string();
				bool bIsDirectory = Entry.is_directory();
				std::string Extension = Path.extension().string();

				ImGui::PushID(ItemIndex++);

				ImGui::BeginGroup();
				{
					ImGui::InvisibleButton("##TileBtn", ImVec2(TileWidth, TileHeight));

					bool bHovered = ImGui::IsItemHovered();
					bool bActive = ImGui::IsItemActive();
					ImVec2 PMin = ImGui::GetItemRectMin();
					ImVec2 PMax = ImGui::GetItemRectMax();
					ImDrawList* DrawList = ImGui::GetWindowDrawList();

					if (bHovered || bActive)
					{
						ImU32 BgColor = bActive ? IM_COL32(255, 255, 255, 40) : IM_COL32(255, 255, 255, 20);
						DrawList->AddRectFilled(PMin, PMax, BgColor, 4.0f);
					}

					if (bIsDirectory)
					{
						if (ImGui::IsItemClicked())
						{
							CurrentDirectory = Path;
							ImGui::EndGroup();
							ImGui::PopID();
							break;
						}
						ImU32 TabColor = bActive ? 0xFF80C0E0 : (bHovered ? 0xFF99D0F0 : 0xFF70B0D0);
						ImU32 BodyColor = bActive ? 0xFF90D0F8 : (bHovered ? 0xFFAAE0FF : 0xFF80C0E8);

						ImVec2 TabMin = ImVec2(PMin.x + 8.0f, PMin.y + 12.0f);
						ImVec2 TabMax = ImVec2(PMin.x + 36.0f, PMin.y + 24.0f);
						DrawList->AddRectFilled(TabMin, TabMax, TabColor, 4.0f);

						ImVec2 BodyMin = ImVec2(PMin.x + 8.0f, PMin.y + 20.0f);
						ImVec2 BodyMax = ImVec2(PMax.x - 8.0f, PMax.y - 12.0f);
						DrawList->AddRectFilled(BodyMin, BodyMax, BodyColor, 6.0f);
						DrawList->AddRect(BodyMin, BodyMax, 0x55000000, 6.0f, 0, 1.5f);
					}
					else if (Extension == ".obj")
					{
						ImVec2 Center = ImVec2((PMin.x + PMax.x) * 0.5f, (PMin.y + PMax.y) * 0.5f - 4.0f);
						const float HalfSize = 16.0f;
						const float Offset = 8.0f;

						ImVec2 F_TL = ImVec2(Center.x - HalfSize, Center.y - HalfSize + Offset * 0.5f);
						ImVec2 F_BR = ImVec2(Center.x + HalfSize - Offset, Center.y + HalfSize);

						ImVec2 B_TL = ImVec2(F_TL.x + Offset, F_TL.y - Offset);
						ImVec2 B_BR = ImVec2(F_BR.x + Offset, F_BR.y - Offset);

						ImU32 CubeColor = bActive ? 0xFF00FFFF : (bHovered ? 0xFF66FFFF : 0xFF00D2D2);
						ImU32 FillColor = (CubeColor & 0x00FFFFFF) | 0x22000000;

						DrawList->AddRectFilled(F_TL, F_BR, FillColor);
						DrawList->AddRect(F_TL, F_BR, CubeColor, 0.0f, 0, 1.5f);
						DrawList->AddRect(B_TL, B_BR, CubeColor, 0.0f, 0, 1.5f);

						DrawList->AddLine(F_TL, B_TL, CubeColor, 1.5f);
						DrawList->AddLine(ImVec2(F_BR.x, F_TL.y), ImVec2(B_BR.x, B_TL.y), CubeColor, 1.5f);
						DrawList->AddLine(ImVec2(F_TL.x, F_BR.y), ImVec2(B_TL.x, B_BR.y), CubeColor, 1.5f);
						DrawList->AddLine(F_BR, B_BR, CubeColor, 1.5f);
					}
					else if (Extension == ".jpg" || Extension == ".png")
					{
						if (AssetManager)
						{
							TSharedPtr<FTexture2DAsset> TextureAsset = AssetManager->GetAssetAs<FTexture2DAsset>(Path.stem().string().c_str(), true);
							if (TextureAsset && TextureAsset->GetSRV())
							{
								ImTextureID TexID = (ImTextureID)TextureAsset->GetSRV().Get();
								ImVec2 ImgMin = ImVec2(PMin.x + 8.0f, PMin.y + 8.0f);
								ImVec2 ImgMax = ImVec2(PMax.x - 8.0f, PMax.y - 8.0f);
								DrawList->AddImage(TexID, ImgMin, ImgMax);
								DrawList->AddRect(ImgMin, ImgMax, 0x44FFFFFF, 2.0f);
							}
						}
					}
					else
					{
						ImVec2 Center = ImVec2((PMin.x + PMax.x) * 0.5f, (PMin.y + PMax.y) * 0.5f - 4.0f);
						const float HalfSize = 16.0f;
						const float Offset = 8.0f;

						ImVec2 F_TL = ImVec2(Center.x - HalfSize, Center.y - HalfSize + Offset * 0.5f);
						ImVec2 F_BR = ImVec2(Center.x + HalfSize - Offset, Center.y + HalfSize);

						ImVec2 B_TL = ImVec2(F_TL.x + Offset, F_TL.y - Offset);
						ImVec2 B_BR = ImVec2(F_BR.x + Offset, F_BR.y - Offset);

						ImU32 CubeColor = bActive ? 0xFF00FFFF : (bHovered ? 0xFF66FFFF : 0xFF00D2D2);
						ImU32 FillColor = (CubeColor & 0x00FFFFFF) | 0x22000000;

						DrawList->AddRectFilled(F_TL, F_BR, FillColor);
						DrawList->AddRect(F_TL, F_BR, CubeColor, 0.0f, 0, 1.5f);
						DrawList->AddRect(B_TL, B_BR, CubeColor, 0.0f, 0, 1.5f);

						DrawList->AddLine(F_TL, B_TL, CubeColor, 1.5f);
						DrawList->AddLine(ImVec2(F_BR.x, F_TL.y), ImVec2(B_BR.x, B_TL.y), CubeColor, 1.5f);
						DrawList->AddLine(ImVec2(F_TL.x, F_BR.y), ImVec2(B_TL.x, B_BR.y), CubeColor, 1.5f);
						DrawList->AddLine(F_BR, B_BR, CubeColor, 1.5f);
					}

					if (!bIsDirectory && ImGui::BeginDragDropSource())
					{
						if (Extension == ".obj")
						{
							std::string FullPath = Path.string();
							ImGui::SetDragDropPayload(AssetPayloadTags::StaticMesh, FullPath.c_str(), (FullPath.length() + 1) * sizeof(char));
							ImGui::Text("Mesh: %s", FileName.c_str());
						}
						else if (Extension == ".jpg" || Extension == ".png")
						{
							std::string FullPath = Path.string();
							ImGui::SetDragDropPayload(AssetPayloadTags::Texture2D, FullPath.c_str(), (FullPath.length() + 1) * sizeof(char));
							ImGui::Text("Texture2D: %s", FileName.c_str());
						}
						ImGui::EndDragDropSource();
					}

					std::string TruncatedName = FileName;
					if (TruncatedName.length() > 9)
					{
						TruncatedName = TruncatedName.substr(0, 7) + "..";
					}
					ImGui::TextWrapped("%s", TruncatedName.c_str());
				}
				ImGui::EndGroup();

				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s", FileName.c_str());
				}

				float LastItemX2 = ImGui::GetItemRectMax().x;
				float NextItemX2 = LastItemX2 + Style.ItemSpacing.x + TileWidth;

				if (NextItemX2 < WindowVisibleX2)
				{
					ImGui::SameLine();
				}

				ImGui::PopID();
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

	std::string NodeHiddenID = FString("##").Append(PathString);
	bool bNodeOpen = ImGui::TreeNodeEx(NodeHiddenID.c_str(), NodeFlags);

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		CurrentDirectory = DirectoryPath;
	}

	ImGui::SameLine();

	const float IconSize = 14.0f;
	ImGui::Dummy(ImVec2(IconSize, IconSize));

	ImVec2 IMin = ImGui::GetItemRectMin();
	ImVec2 IMax = ImGui::GetItemRectMax();
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	ImU32 TabColor = 0xFF80C0E0;
	ImU32 BodyColor = 0xFF90D0F8;

	DrawList->AddRectFilled(
		ImVec2(IMin.x + 1.0f, IMin.y + 1.0f),
		ImVec2(IMin.x + 7.0f, IMin.y + 4.0f),
		TabColor, 1.0f
	);

	DrawList->AddRectFilled(
		ImVec2(IMin.x + 1.0f, IMin.y + 3.5f),
		ImVec2(IMax.x - 1.0f, IMax.y - 1.0f),
		BodyColor, 2.0f
	);
	DrawList->AddRect(
		ImVec2(IMin.x + 1.0f, IMin.y + 3.5f),
		ImVec2(IMax.x - 1.0f, IMax.y - 1.0f),
		0x44000000, 2.0f, 0, 1.0f
	);

	ImGui::SameLine();
	ImGui::TextUnformatted(FolderName.c_str());

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