#include "FAssetManager.h"
#include "LaunchEngineLoop.h"
#include "Serializers.h"
#include "Assets.h"
#include "FLogManager.h"

FAssetManager& FAssetManager::Get()
{
	return *GEngineLoop.GetAssetManager();
}

void FAssetManager::RegisterAsset(const FName& AssetName, const TSharedPtr<FAssetLoader>& AssetLoader, const TSharedPtr<FAssetSource>& AssetSource)
{
	TSharedPtr<FArchive> Archive = AssetSource->CreateArchive();

	FAssetFileHeader Header;
	*Archive << Header;

	if (AssetMetaInfos.Contains(Header.AssetID))
	{
		return;
	}

	FAssetMetaInfo MetaInfo;
	MetaInfo.AssetID = Header.AssetID;
	MetaInfo.AssetType = AssetLoader->GetAssetType();
	MetaInfo.AssetName = AssetName;
	MetaInfo.PayloadOffset = Archive->Tell();
	MetaInfo.AssetLoader = AssetLoader;
	MetaInfo.AssetSource = AssetSource;

	AssetMetaInfos.Add(MetaInfo.AssetID, MetaInfo);
	NameToAssetID.Add(MetaInfo.AssetName, MetaInfo.AssetID);
}

void FAssetManager::RegisterAsset(const FGuid& AssetID, const FName& AssetName, const TSharedPtr<FAssetLoader>& AssetLoader, const TSharedPtr<FAssetSource>& AssetSource)
{
	FString NormalizedPath = NormalizeAssetPath(std::filesystem::path(AssetName.ToString().CStr()));
	FName NormalizedKey(NormalizedPath);

	if (NameToAssetID.Contains(AssetName))
	{
		return;
	}

	FAssetMetaInfo MetaInfo;
	MetaInfo.AssetID = AssetID;
	MetaInfo.AssetType = AssetLoader->GetAssetType();
	MetaInfo.AssetName = AssetName;
	MetaInfo.PayloadOffset = 0;
	MetaInfo.AssetLoader = AssetLoader;
	MetaInfo.AssetSource = AssetSource;

	AssetMetaInfos.Add(MetaInfo.AssetID, MetaInfo);
	NameToAssetID.Add(AssetName, MetaInfo.AssetID);
}

void FAssetManager::RegisterAsset(const TSharedPtr<FAsset>& Asset)
{
	const FName& AssetName = Asset->GetAssetName();
	const FGuid& AssetID = Asset->GetAssetID();

	if (NameToAssetID.Contains(AssetName) || AssetMetaInfos.Contains(AssetID))
	{
		return;
	}

	FAssetMetaInfo MetaInfo;
	MetaInfo.AssetID = AssetID;
	MetaInfo.AssetType = Asset->GetAssetType();
	MetaInfo.AssetName = AssetName;
	MetaInfo.PayloadOffset = 0;
	MetaInfo.AssetLoader = nullptr;
	MetaInfo.AssetSource = nullptr;

	AssetMetaInfos.Add(AssetID, MetaInfo);
	NameToAssetID.Add(AssetName, AssetID);
	LoadedAssets.Add(AssetID, Asset);
}

void FAssetManager::UnregisterAsset(const FName& AssetName)
{
	FGuid* AssetIDPtr = NameToAssetID.Find(AssetName);
	if (!AssetIDPtr)
	{
		return;
	}

	const FGuid AssetID = *AssetIDPtr;
	UnloadAsset(AssetID);
	AssetMetaInfos.Remove(AssetID);
	NameToAssetID.Remove(AssetName);
}

void FAssetManager::UnloadAsset(const FName& AssetName)
{
	FGuid* AssetIDPtr = NameToAssetID.Find(AssetName);
	if (AssetIDPtr)
	{
		UnloadAsset(*AssetIDPtr);
	}
}

void FAssetManager::UnloadAsset(const FGuid& AssetID)
{
	TSharedPtr<FAsset> asset = GetAsset(AssetID);
	if (asset)
	{
		FAssetMetaInfo& MetaInfo = AssetMetaInfos[AssetID];
		TSharedPtr<FAssetLoader> assetLoader = MetaInfo.AssetLoader;
		if (assetLoader)
		{
			assetLoader->UnloadAsset(asset);
		}
		LoadedAssets.Remove(AssetID);
	}
}

// 프로그램 시작 시에 호출하여 Directory 스캔하는 함수
void FAssetManager::ScanDirectory(const std::filesystem::path& RootDir, URenderer& Renderer)
{
	for (const auto& Entry : std::filesystem::recursive_directory_iterator(RootDir))
	{
		// 폴더이거나 .uasset 파일이 아니면 pass
		if (Entry.is_directory() || Entry.path().extension() != ".uasset") continue;

		// 이름 중복 해결을 위해 전체 경로
		FName AssetName(NormalizeAssetPath(Entry.path()));
		FAssetFileHeader Header;
		
		try
		{
			FWindowsBinReader Reader(Entry.path());
			Reader << Header;

		}
		catch(const std::exception& e)
		{
			// Header 읽기 실패
			UE_LOG_ERROR("Failed to read header: %s", Entry.path().string().c_str());
			continue;
		}

		TSharedPtr<FAssetLoader> Loader;

		switch (Header.AssetType)
		{
			case EAssetType::Texture2D:
			{
				Loader = MakeShared<FTexture2DAssetLoader>(Renderer);
				break;
			}
			case EAssetType::StaticMesh:
			{
				Loader = MakeShared<FStaticMeshAssetLoader>(Renderer);
				break;
			}
			case EAssetType::Material:
			{
				Loader = MakeShared<FMaterialAssetLoader>();
				break;
			}
			default:
				continue;
	}

	// Asset 등록
	RegisterAsset(Header.AssetID, AssetName, Loader, MakeShared<FFileAssetSource>(Entry.path()));
	
	}
}

TSharedPtr<FAsset> FAssetManager::LoadAsset(const FName& AssetName)
{
	FGuid* AssetIDPtr = NameToAssetID.Find(AssetName);
	return AssetIDPtr ? LoadAsset(*AssetIDPtr) : nullptr;
}

TSharedPtr<FAsset> FAssetManager::LoadAsset(const FGuid& AssetID)
{
	TSharedPtr<FAsset>* LoadedAssetPtr = LoadedAssets.Find(AssetID);
	if (LoadedAssetPtr)
	{
		return *LoadedAssetPtr;
	}

	FAssetMetaInfo* MetaInfoPtr = AssetMetaInfos.Find(AssetID);
	if (!MetaInfoPtr || !MetaInfoPtr->AssetLoader || !MetaInfoPtr->AssetSource)
	{
		return nullptr;
	}

	TSharedPtr<FArchive> Archive = MetaInfoPtr->AssetSource->CreateArchive();
	Archive->Seek(MetaInfoPtr->PayloadOffset);

	TSharedPtr<FAsset> asset = MetaInfoPtr->AssetLoader->LoadAsset(MetaInfoPtr->AssetID, MetaInfoPtr->AssetName, *Archive);
	if (asset)
	{
		LoadedAssets.Add(MetaInfoPtr->AssetID, asset);
	}

	return asset;
}

TSharedPtr<FAsset> FAssetManager::GetAsset(const FName& AssetName, bool loadIfNotLoaded)
{
	FGuid* AssetIDPtr = NameToAssetID.Find(AssetName);
	return AssetIDPtr ? GetAsset(*AssetIDPtr, loadIfNotLoaded) : nullptr;
}

TSharedPtr<FAsset> FAssetManager::GetAsset(const FGuid& AssetID, bool loadIfNotLoaded)
{
	TSharedPtr<FAsset>* LoadedAssetPtr = LoadedAssets.Find(AssetID);
	if (LoadedAssetPtr)
	{
		return *LoadedAssetPtr;
	}

	if (loadIfNotLoaded)
	{
		return LoadAsset(AssetID);
	}

	return nullptr;
}
