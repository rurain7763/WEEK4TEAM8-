#include "FObjManager.h"

#include "Assets.h"
#include "FAssetManager.h"
#include "FObjImporter.h"
#include "FStaticMeshBuilder.h"
#include "ObjectFactory.h"
#include "Renderer.h"
#include "FileManager.h"

#include <filesystem>

URenderer* FObjManager::Renderer = nullptr;
TMap<FString, UStaticMesh*> FObjManager::ObjStaticMeshMap;
FFileManager* FObjManager::FileManager = nullptr;

void FObjManager::Initialize(URenderer& InRenderer, FFileManager& InFileManager)
{
    Renderer = &InRenderer;
    FileManager = &InFileManager;
}

UStaticMesh* FObjManager::LoadObjStaticMesh(const FString& FilePath)
{
    UStaticMesh** FoundStaticMesh = ObjStaticMeshMap.Find(FilePath);

    if (FoundStaticMesh)
    {
        return *FoundStaticMesh;
    }

    if (!Renderer)
    {
        return nullptr;
    }

    FObjImporter Importer;
    FObjInfo ObjInfo;
    FMeshDescription MeshDescription;
    FStaticMeshBuildData BuildData;

    if (!Importer.ParseObj(FilePath, ObjInfo))
    {
        return nullptr;
    }
    if (!Importer.ConvertToMeshDescription(ObjInfo, MeshDescription))
    {
        return nullptr;
    }
    if (!FStaticMeshBuilder::Build(MeshDescription, BuildData))
    {
        return nullptr;
    }

    TMap<FString, TSharedPtr<FTexture2DAsset>> DiffuseTextures;
    TSharedPtr<FTexture2DAssetLoader> TextureLoader =  MakeShared<FTexture2DAssetLoader>(*Renderer);

    const std::filesystem::path ObjPath(FilePath.CStr());
    const std::filesystem::path ObjDirectory = ObjPath.parent_path();

    for (const FObjMaterialInfo& Material : ObjInfo.Materials)
    {
        if (Material.DiffuseTexturePath.Len() == 0)
        {
            continue;
        }

        const std::filesystem::path TexturePath = ObjDirectory / Material.DiffuseTexturePath.CStr();
        const std::filesystem::path AssetRelativeTexturePath = TexturePath.lexically_relative("Assets");
        const FString TexturePathString = FString(TexturePath.string());
        const FName TextureAssetName(TexturePathString);

        TSharedPtr<FTexture2DAsset> Texture =
            FAssetManager::Get().GetAssetAs<FTexture2DAsset>(
                TextureAssetName,
                true);

        if (!Texture)
        {
            TSharedPtr<FFileAssetSource> TextureSource =
                MakeShared<FFileAssetSource>(
                    *FileManager,
                    AssetRelativeTexturePath);

            FAssetManager::Get().RegisterAsset(
                TextureAssetName,
                TextureLoader,
                TextureSource);

            Texture =
                FAssetManager::Get().GetAssetAs<FTexture2DAsset>(
                    TextureAssetName,
                    true);
        }

        if (Texture)
        {
            DiffuseTextures.Add(Material.Name, Texture);
        }
    }

    const FName MeshAssetName(FilePath);
    TSharedPtr<FStaticMeshAsset> StaticMeshAsset =
        MakeShared<FStaticMeshAsset>(MeshAssetName, *Renderer, BuildData);

    FAssetManager::Get().RegisterAsset(StaticMeshAsset);

    UStaticMesh* StaticMesh =FObjectFactory::ConstructObject<UStaticMesh>();
    if (!StaticMesh)
    {
        return nullptr;
    }

    StaticMesh->SetStaticMeshAsset(StaticMeshAsset, FilePath, ObjInfo.Materials, DiffuseTextures);
    ObjStaticMeshMap.Add(FilePath, StaticMesh);

    return StaticMesh;
}