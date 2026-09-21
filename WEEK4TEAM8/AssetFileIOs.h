#pragma once

#include "Core.h"
#include "TArray.h"
#include "Stb/stb_image.h"
#include "FArchive.h"
#include "Serializers.h"
#include "Object.h"
#include "FStaticMeshBuilder.h"
#include "FObjImporter.h"
#include "FMaterialImporter.h"
#include "FLogManager.h"
#include <filesystem>


// Texture 전용 Payload, FileIO
struct FImagePayload
{
	int32 Width;
	int32 Height;
	int32 Channels;
	TArray<int8> ImageData;
};

class FImageFileIO
{
public:
	static bool Load(const std::filesystem::path& FilePath, FImagePayload& OutPayload)
	{
		stbi_uc* ImageDataPtr = stbi_load(FilePath.string().c_str(), &OutPayload.Width, &OutPayload.Height, &OutPayload.Channels, 4);
		if (!ImageDataPtr)
		{
			return false;
		}

		OutPayload.ImageData.SetNum(OutPayload.Width * OutPayload.Height * 4);
		std::memcpy(OutPayload.ImageData.Data(), ImageDataPtr, OutPayload.ImageData.Num());

		stbi_image_free(ImageDataPtr);

		return true;
	}

	static bool Load(FArchive& Ar, FImagePayload& OutPayload)
	{
		Ar << OutPayload.Width;
		Ar << OutPayload.Height;
		Ar << OutPayload.Channels;
		Ar << OutPayload.ImageData;

		return true;
	}

	static bool Save(FArchive& Ar, FImagePayload& InPayload)
	{
		Ar << InPayload.Width;
		Ar << InPayload.Height;
		Ar << InPayload.Channels;
		Ar << InPayload.ImageData;

		return true;
	}
};

struct FStaticMeshPayload
{
	FGuid AssetID;
	FName AssetName;
	FStaticMeshBuildData BuildData;
	TArray<FObjMaterialInfo> Materials;
};

// StaicMesh 전용 IO
class FStaticMeshFileIO
{
public:
	static bool Load(FArchive& Ar, FStaticMeshBuildData& OutData)
	{
		Ar << OutData.Vertices;
		Ar << OutData.Indices;
		Ar << OutData.Sections;
		return true;
	}

	static bool Load(const std::filesystem::path& FilePath,
		FStaticMeshPayload& OutPayload)
	{
		const std::filesystem::path Extension = FilePath.extension();

		if (Extension == ".obj")
		{
			FObjImporter Importer;
			FObjInfo ObjInfo;
			FMeshDescription MeshDescription;

			if (!Importer.ParseObj(FString(FilePath.string()), ObjInfo))
			{
				return false;
			}
			if (!Importer.ConvertToMeshDescription(ObjInfo, MeshDescription))
			{
				return false;
			}
			if (!FStaticMeshBuilder::Build(MeshDescription, OutPayload.BuildData))
			{
				return false;
			}

			const std::filesystem::path ObjDirectory = FilePath.parent_path();
			const std::string MeshName = FilePath.stem().string();

			for (const FObjMaterialInfo& Material : ObjInfo.Materials)
			{
				const std::filesystem::path MaterialPath = ObjDirectory / (MeshName + "_" + Material.Name.CStr() + ".uasset");

				FAssetFileHeader MaterialHeader;

				if (std::filesystem::exists(MaterialPath))
				{
					FWindowsBinReader Reader(MaterialPath);
					Reader << MaterialHeader;
					if (MaterialHeader.AssetType != EAssetType::Material)
					{
						UE_LOG_ERROR("Invalid material asset: %s", MaterialPath.string().c_str());
						return false;
					}
				}
				else if (!FMaterialImporter::Import(
					Material,
					ObjDirectory,
					MaterialPath,
					MaterialHeader))
				{
					UE_LOG_ERROR("Failed to import material: %s",
						Material.Name.CStr());
					return false;
				}

				for (FStaticMeshSection& Section : OutPayload.BuildData.Sections)
				{
					if (Section.MaterialName == Material.Name)
					{
						Section.MaterialAssetID = MaterialHeader.AssetID;
					}
				}
			}


			OutPayload.AssetID = FGuid::NewGuid();
			OutPayload.AssetName = FName(FilePath.string());
			OutPayload.Materials = ObjInfo.Materials;
			return true;
		}

		if (Extension == ".uasset")
		{
			FWindowsBinReader Reader(FilePath);
			FAssetFileHeader Header;
			Reader << Header;

			if (Header.AssetType != EAssetType::StaticMesh)
			{
				return false;
			}

			OutPayload.AssetID = Header.AssetID;
			OutPayload.AssetName = FName(FilePath.string());
			return Load(Reader, OutPayload.BuildData);
		}

		return false;
	}

	static bool Save(FArchive& Ar, FStaticMeshBuildData& InData)
	{
		Ar << InData.Vertices;
		Ar << InData.Indices;
		Ar << InData.Sections;
		return true;
	}
	
private:
};

// Material 전용 Payload, IO
struct  FMaterialPayload
{
	FVector AmbientColor;
	FVector DiffuseColor;
	FVector SpecularColor;
	FGuid DiffuseTexture;
	FGuid SpecularTexture;
	FGuid NormalTexture;
	float Opacity;
};

class FMaterialFileIO
{
public:

	static bool Load(FArchive& Ar, FMaterialPayload& OutPayload)
	{
		Ar << OutPayload.AmbientColor;
		Ar << OutPayload.DiffuseColor;
		Ar << OutPayload.SpecularColor;
		Ar << OutPayload.DiffuseTexture;
		Ar << OutPayload.SpecularTexture;
		Ar << OutPayload.NormalTexture;
		Ar << OutPayload.Opacity;
		
		return true;
	}
	
	// mtl -> uasset
	static bool Save(FArchive& Ar, FMaterialPayload& InPayload)
	{
		Ar << InPayload.AmbientColor;
		Ar << InPayload.DiffuseColor;
		Ar << InPayload.SpecularColor;
		Ar << InPayload.DiffuseTexture;
		Ar << InPayload.SpecularTexture;
		Ar << InPayload.NormalTexture;
		Ar << InPayload.Opacity;
		
		return true;
	}
	
	private:
};
