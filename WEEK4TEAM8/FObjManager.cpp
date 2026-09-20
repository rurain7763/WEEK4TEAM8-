#include "FObjManager.h"

#include "Assets.h"
#include "FAssetManager.h"
#include "FObjImporter.h"
#include "FStaticMeshBuilder.h"
#include "ObjectFactory.h"
#include "Renderer.h"
#include "FileManager.h"
#include "FTexture2DImporter.h"

#include <filesystem>
#include "FLogManager.h"

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

        // weakly_canonica : 절대경로 만들고 /.. 같은 거 정리해줌 (canonical과 달리 파일 없어도 동작함)
        const std::filesystem::path TexturePath = std::filesystem::weakly_canonical(ObjDirectory / Material.DiffuseTexturePath.CStr());
		std::filesystem::path ExpectedUAssetPath = TexturePath;
        ExpectedUAssetPath.replace_extension(".uasset");

        // 캐시 체크용 이름, 미리 계산
        const FName TextureAssetName(ExpectedUAssetPath.string());
        
        const FString TexturePathString = FString(TexturePath.string());

        TSharedPtr<FTexture2DAsset> Texture =
            FAssetManager::Get().GetAssetAs<FTexture2DAsset>(TextureAssetName, true);

        if (!Texture)
        {
            std::optional<std::filesystem::path> NewTexturePath =
                    FTexture2DImporter::GetorImport(TexturePath);
            
            if (!NewTexturePath)
            {
                UE_LOG_ERROR("Failed to import texture: %s", TexturePath.string().c_str());
                continue;
            }

            TSharedPtr<FFileAssetSource> TextureSource =
				MakeShared<FFileAssetSource>(*NewTexturePath);

            FAssetManager::Get().RegisterAsset(
                FGuid::NewGuid(),
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

    const FName MeshAssetName(ObjPath.stem().string().c_str());
    TSharedPtr<FStaticMeshAsset> StaticMeshAsset =
		MakeShared<FStaticMeshAsset>(FGuid::NewGuid(), MeshAssetName, *Renderer, BuildData);

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
