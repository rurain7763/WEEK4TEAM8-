#pragma once

#include "Core.h"
#include "FAsset.h"
#include "TMap.h"
#include "TArray.h"
#include "FGuid.h"

struct FAssetMetaInfo
{
	FGuid AssetID;
	EAssetType AssetType;
	FName AssetName;
	uint64 PayloadOffset;
	TSharedPtr<FAssetLoader> AssetLoader;
	TSharedPtr<FAssetSource> AssetSource;
};

class FAssetManager
{
public:
	static FAssetManager& Get();

	void RegisterAsset(const TSharedPtr<FAssetLoader>& AssetLoader, const TSharedPtr<FAssetSource>& AssetSource);
	void RegisterAsset(const FGuid& AssetID, const FName& AssetName, const TSharedPtr<FAssetLoader>& AssetLoader, const TSharedPtr<FAssetSource>& AssetSource);
	void RegisterAsset(const TSharedPtr<FAsset>& Asset);
	void UnregisterAsset(const FName& AssetName);

	TSharedPtr<FAsset> LoadAsset(const FName& AssetName);
	TSharedPtr<FAsset> LoadAsset(const FGuid& AssetID);

	TSharedPtr<FAsset> GetAsset(const FName& AssetName, bool loadIfNotLoaded = false);
	TSharedPtr<FAsset> GetAsset(const FGuid& AssetID, bool loadIfNotLoaded = false);

	template <typename T>
	TSharedPtr<T> GetAssetAs(const FName& AssetName, bool loadIfNotLoaded = false)
	{
		TSharedPtr<FAsset> asset = GetAsset(AssetName, loadIfNotLoaded);
		if (asset)
		{
			return std::static_pointer_cast<T>(asset);
		}

		return nullptr;
	}

	template <typename T>
	TSharedPtr<T> GetAssetAs(const FGuid& AssetID, bool loadIfNotLoaded = false)
	{
		TSharedPtr<FAsset> asset = GetAsset(AssetID, loadIfNotLoaded);
		if (asset)
		{
			return std::static_pointer_cast<T>(asset);
		}
		return nullptr;
	}

	void UnloadAsset(const FName& AssetName);
	void UnloadAsset(const FGuid& AssetID);

	template <typename Func>
	void ForEachMetaInfo(Func&& func)
	{
		for (auto& pair : AssetMetaInfos)
		{
			func(pair.second);
		}
	}

private:
	TMap<FName, FGuid> NameToAssetID;
	TMap<FGuid, FAssetMetaInfo> AssetMetaInfos;
	TMap<FGuid, TSharedPtr<FAsset>> LoadedAssets;
};