#include "FObjImporter.h"
#include "FileManager.h"
#include "FLogManager.h"
#include <sstream>

constexpr int32 INDEX_NONE = -1;

struct FFaceVertex
{
	int32 V = INDEX_NONE;
	int32 VT = INDEX_NONE;
	int32 VN = INDEX_NONE;
};

TSharedPtr<FStaticMeshAsset> FObjImporter::Import(const FName& InAssetName, URenderer& InRenderer, const FString& FilePath)
{
	FObjInfo RawData;
	if (!ParseOBJ(FilePath, RawData))
	{
		return nullptr;
	}
	return BuildStaticMesh(InAssetName, InRenderer, RawData, FilePath);
}

TSharedPtr<FStaticMeshAsset> FObjImporter::BuildStaticMesh(
	const FName& InAssetName,
	URenderer& InRenderer,
	const FObjInfo& InRawData,
	const FString& FilePath
)
{
	TArray<FVertexPNCT> Vertices;
	TArray<uint32> Indices;
	int32 TotalFaceIndices = InRawData.VertexIndices.Num();

	for (int i = 0;i < TotalFaceIndices;++i)
	{
		int32 VIdx = InRawData.VertexIndices[i];
		int32 VTIdx = InRawData.UVIndices[i];
		int32 VNIdx = InRawData.NormalIndices[i];

		FVector Position = InRawData.Vertices[VIdx];
		FVector2 UV;
		FVector Normal;
		if (VTIdx != INDEX_NONE)
			UV = InRawData.UV[VTIdx];
		else
			UV = FVector2(0.0f, 0.0f);
		if (VNIdx != INDEX_NONE)
			Normal = InRawData.Normal[VNIdx];
		else
			Normal = FVector(0.0f, 1.0f, 0.0f);

		FVertexPNCT Vertex({
			Position.x, Position.y, Position.z,
			UV.X, UV.Y,
			1.0f, 1.0f, 1.0f, 1.0f,
			Normal.x, Normal.y, Normal.z
			});

		Vertices.Add(Vertex);
		Indices.Add(static_cast<uint32>(i));
	}

	FString MtlFilePath = InRawData.MtlFileName;
	if (InRawData.MtlFileName.Len() > 0)
	{
		int32 LastSlash = -1;
		for (int32 i = FilePath.Len() - 1; i >= 0; --i)
		{
			if (FilePath.CStr()[i] == '/' || FilePath.CStr()[i] == '\\')
			{
				LastSlash = i;
				break;
			}
		}

		if (LastSlash != -1)
		{
			MtlFilePath = FilePath.Mid(0, LastSlash + 1);
			MtlFilePath += InRawData.MtlFileName;
		}
	}

	if (Vertices.Num() == 0 || Indices.Num() == 0)
	{
		UE_LOG("Failed to build static mesh: No valid geometry data.");
		return nullptr;
	}

	TArray<FObjMaterialInfo> ParsedMaterials;
	if (MtlFilePath.Len() > 0)
	{
		ParseMTL(MtlFilePath, ParsedMaterials);
	}

	// 다중 메테리얼 지원 기능 추가 해야 함, 현재는 단일 메테리얼만 지원
	TArray<FStaticMeshSection> Sections;
	if (Indices.Num() > 0)
	{
		FStaticMeshSection DefaultSection;
		DefaultSection.StartIndex = 0;
		DefaultSection.IndexCount = Indices.Num();
		DefaultSection.MaterialIndex = 0;
		if (ParsedMaterials.Num() > 0)
		{
			DefaultSection.MaterialName = ParsedMaterials[0].MaterialName;
		}
		Sections.Add(DefaultSection);
	}

	TSharedPtr<FStaticMeshAsset> MeshAsset = MakeShared<FStaticMeshAsset>(
		InAssetName,
		InRenderer,
		Vertices.Data(),
		static_cast<uint32>(Vertices.Num()),
		Indices.Data(),
		static_cast<uint32>(Indices.Num()),
		Sections,
		FilePath
	);

	return MeshAsset;
}

bool FObjImporter::ParseOBJ(const FString& FilePath, FObjInfo& OutRawData)
{
	FFileManager FileManager;
	FString FileContent;
	try
	{
		FileContent = FileManager.ReadFileToString(FilePath);
	}
	catch (const std::exception& e)
	{
		//로그 출력
		UE_LOG("Failed To Read OBJ: %s, Erro: %s", FilePath, e.what());
		return false;
	}

	if (FileContent.Len() == 0)
		return false;

	int32 TotalLen = FileContent.Len();
	int32 Start = 0;

	while (Start < TotalLen)
	{
		int32 End = FileContent.Find(std::string_view("\n"), Start);
		if (End == -1)
		{
			End = TotalLen;
		}

		int32 Count = End - Start;
		FString Line = FileContent.Mid(Start, Count);

		Line.RemoveFromEnd(std::string_view("\r"));
		if (Line.Len() > 0 && !Line.StartsWith(std::string_view("#")))
		{
			if (Line.StartsWith(std::string_view("v ")))
			{
				FVector Vertex;
				sscanf_s(Line.CStr() + 2, "%f %f %f", &Vertex.x, &Vertex.y, &Vertex.z);
				OutRawData.Vertices.Add(Vertex);
			}
			else if (Line.StartsWith(std::string_view("vn ")))
			{
				FVector Normal;
				sscanf_s(Line.CStr() + 3, "%f %f %f", &Normal.x, &Normal.y, &Normal.z);
				OutRawData.Normal.Add(Normal);
			}
			else if (Line.StartsWith(std::string_view("vt ")))
			{
				FVector2 UV;
				sscanf_s(Line.CStr() + 3, "%f %f", &UV.X, &UV.Y);
				UV.Y = 1.0f - UV.Y;
				OutRawData.UV.Add(UV);
			}
			else if (Line.StartsWith(std::string_view("f ")))
			{
				std::stringstream stream(Line.CStr() + 2);
				std::string Token;
				TArray<FFaceVertex> FaceVertices;

				while (stream >> Token)
				{
					FFaceVertex FaceVertex;
					const char* str = Token.c_str();

					int32 RawV = 0, RawVT = 0, RawVN = 0;

					if (sscanf_s(str, "%d/%d/%d", &RawV, &RawVT, &RawVN) == 3)
					{
						FaceVertex.V = RawV - 1;
						FaceVertex.VT = RawVT - 1;
						FaceVertex.VN = RawVN - 1;
					}
					else if (sscanf_s(str, "%d//%d", &RawV, &RawVN) == 2)
					{
						FaceVertex.V = RawV - 1;
						FaceVertex.VN = RawVN - 1;
					}
					else if (sscanf_s(str, "%d/%d", &RawV, &RawVT) == 2)
					{
						FaceVertex.V = RawV - 1;
						FaceVertex.VT = RawVT - 1;
					}
					else if (sscanf_s(str, "%d", &RawV) == 1)
					{
						FaceVertex.V = RawV - 1;
					}

					FaceVertices.Add(FaceVertex);
				}
				if (FaceVertices.Num() == 3)
				{
					for (int i = 0;i < 3;++i)
					{
						OutRawData.VertexIndices.Add(FaceVertices[i].V);
						OutRawData.UVIndices.Add(FaceVertices[i].VT);
						OutRawData.NormalIndices.Add(FaceVertices[i].VN);
					}
				}
				else if (FaceVertices.Num() == 4)
				{
					const int32 QuadIndices[6] = { 0, 1, 2, 0, 2, 3 };
					for (int i = 0; i < 6; ++i)
					{
						int32 idx = QuadIndices[i];
						OutRawData.VertexIndices.Add(FaceVertices[idx].V);
						OutRawData.UVIndices.Add(FaceVertices[idx].VT);
						OutRawData.NormalIndices.Add(FaceVertices[idx].VN);
					}
				}
			}
			else if (Line.StartsWith(std::string_view("mtllib ")))
			{
				FString MtlFileName(std::string_view(Line.CStr() + 7));
				OutRawData.MtlFileName = MtlFileName;
			}
			else if (Line.StartsWith(std::string_view("usemtl ")))
			{
				FString MaterialName(std::string_view(Line.CStr() + 7));
				OutRawData.Materials.Add(MaterialName);
			}
		}

		Start = End + 1;
	}

	return true;
}

bool FObjImporter::ParseMTL(const FString& MtlFilePath, TArray<FObjMaterialInfo>& OutMaterials)
{
	FFileManager FileManager;
	FString FileContent;
	try
	{
		FileContent = FileManager.ReadFileToString(MtlFilePath);
	}
	catch (const std::exception& e)
	{
		//로그 출력
		UE_LOG("Failed To Read MTL: %s, Erro: %s", MtlFilePath, e.what());
		return false;
	}

	if (FileContent.Len() == 0)
		return false;

	int32 TotalLen = FileContent.Len();
	int32 Start = 0;
	FObjMaterialInfo CurrentMaterial;

	while (Start < TotalLen)
	{
		int32 End = FileContent.Find(std::string_view("\n"), Start);
		if (End == -1)
		{
			End = TotalLen;
		}

		int32 Count = End - Start;
		FString Line = FileContent.Mid(Start, Count);

		Line.RemoveFromEnd(std::string_view("\r"));
		if (Line.Len() > 0 && !Line.StartsWith(std::string_view("#")))
		{
			if (Line.StartsWith(std::string_view("newmtl ")))
			{
				if (CurrentMaterial.MaterialName.Len() != 0)
				{
					OutMaterials.Add(CurrentMaterial);
					CurrentMaterial = FObjMaterialInfo();
				}
				FString CurrentMaterialName(std::string_view(Line.CStr() + 7));
				CurrentMaterial.MaterialName = CurrentMaterialName;
			}
			else if (Line.StartsWith(std::string_view("map_Kd ")))
			{
				FString CurrentMaterialPath(std::string_view(Line.CStr() + 7));
				int32 LastSlash = -1;
				for (int32 i = MtlFilePath.Len() - 1; i >= 0; --i)
				{
					if (MtlFilePath.CStr()[i] == '/' || MtlFilePath.CStr()[i] == '\\')
					{
						LastSlash = i;
						break;
					}
				}

				if (LastSlash != -1)
				{
					FString FinalPath = MtlFilePath.Mid(0, LastSlash + 1);
					FinalPath += CurrentMaterialPath;
					CurrentMaterial.MaterialTexturePath = FinalPath;
				}
				else
				{
					CurrentMaterial.MaterialTexturePath = CurrentMaterialPath;
				}
			}
			else if (Line.StartsWith(std::string_view("Ka ")))
			{
				FVector CurrentAmbient;
				sscanf_s(Line.CStr() + 3, "%f %f %f", &CurrentAmbient.x, &CurrentAmbient.y, &CurrentAmbient.z);
				CurrentMaterial.Ambient = CurrentAmbient;
			}
			else if (Line.StartsWith(std::string_view("Kd ")))
			{
				FVector CurrentDiffuse;
				sscanf_s(Line.CStr() + 3, "%f %f %f", &CurrentDiffuse.x, &CurrentDiffuse.y, &CurrentDiffuse.z);
				CurrentMaterial.Diffuse = CurrentDiffuse;
			}
			else if (Line.StartsWith(std::string_view("Ks ")))
			{
				FVector CurrentSpecular;
				sscanf_s(Line.CStr() + 3, "%f %f %f", &CurrentSpecular.x, &CurrentSpecular.y, &CurrentSpecular.z);
				CurrentMaterial.Specular = CurrentSpecular;
			}
			else if (Line.StartsWith(std::string_view("Ns ")))
			{
				float CurrentNs;
				sscanf_s(Line.CStr() + 3, "%f", &CurrentNs);
				CurrentMaterial.Ns = CurrentNs;
			}
			else if (Line.StartsWith(std::string_view("illum ")))
			{
				int32 Currentillum;
				sscanf_s(Line.CStr() + 6, "%d", &Currentillum);
				CurrentMaterial.illum = Currentillum;
			}
		}
		Start = End + 1;
	}

	if (CurrentMaterial.MaterialName.Len() != 0)
		OutMaterials.Add(CurrentMaterial);

	return true;
}