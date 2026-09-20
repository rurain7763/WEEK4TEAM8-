#include "FObjManager.h"

#include "Assets.h"
#include "FAssetManager.h"
#include "FObjImporter.h"
#include "FStaticMeshBuilder.h"
#include "ObjectFactory.h"
#include "Renderer.h"
#include "FileManager.h"
#include "FTexture2DImporter.h"
#include "FStaticMeshImporter.h"
#include "Serializers.h"


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
    // 이미 있으면 반환 (메모리 캐시, 프로그램 껐다 켜면 사라짐)
    UStaticMesh** FoundStaticMesh = ObjStaticMeshMap.Find(FilePath);
    if (FoundStaticMesh) return *FoundStaticMesh;

    if (!Renderer) return nullptr;

    FObjImporter Importer;
    FObjInfo ObjInfo;
    // FMeshDescription MeshDescription;
    // FStaticMeshBuildData BuildData;

    // .obj -> v/vt/vn/f/mtl 정보 담은 OBjInfo 반환
    if (!Importer.ParseObj(FilePath, ObjInfo))
    {
        return nullptr;
    }

    // 아래 캐시 로직이 있으므로 삭제
    #if 0
    // 위 정보로 MeshDescription 작성
    if (!Importer.ConvertToMeshDescription(ObjInfo, MeshDescription))
    {
        return nullptr;
    }
    
    // 최종 vertices, indices 
    if (!FStaticMeshBuilder::Build(MeshDescription, BuildData))
    {
        return nullptr;
    }
    #endif

    // <Material 이름, Texture> 을 담는 DiffuseTextures map
    TMap<FString, TSharedPtr<FTexture2DAsset>> DiffuseTextures;
    TSharedPtr<FTexture2DAssetLoader> TextureLoader =  MakeShared<FTexture2DAssetLoader>(*Renderer);

    const std::filesystem::path ObjPath(FilePath.CStr());
    const std::filesystem::path ObjDirectory = ObjPath.parent_path();

    // Material 마다 참조하는 텍스처 경로 계산
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

        // 이미 로드된 텍스처 있으면 재사용
        TSharedPtr<FTexture2DAsset> Texture =
            FAssetManager::Get().GetAssetAs<FTexture2DAsset>(TextureAssetName, true);

        // 없으면 GetorImport로 .uasset 생성 or 반환
        if (!Texture)
        {
            std::optional<std::filesystem::path> NewTexturePath = FTexture2DImporter::GetorImport(TexturePath);

            if (!NewTexturePath)
            {
                UE_LOG_ERROR("Failed to import texture: %s", TexturePath.string().c_str());
                continue;
            }
            
            FAssetFileHeader TextureHeader;
            FWindowsBinReader Reader(*NewTexturePath);
            Reader << TextureHeader;

            TSharedPtr<FFileAssetSource> TextureSource = MakeShared<FFileAssetSource>(*NewTexturePath);

            // AssetManager에 등록
            FAssetManager::Get().RegisterAsset(
                TextureAssetName,
                TextureLoader,
                TextureSource);

            Texture = FAssetManager::Get().GetAssetAs<FTexture2DAsset>(
                TextureHeader.AssetID,
                true);
        }

        // DiffuseTextures 맵에 저장
        if (Texture)
        {
            DiffuseTextures.Add(Material.Name, Texture);
        }
    }

    FName MeshAssetName(std::filesystem::weakly_canonical(ObjPath).string());
    
    // 이미 StaticMesh Asset에 있으면 가져오기
    TSharedPtr<FStaticMeshAsset> StaticMeshAsset =
        FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(MeshAssetName, true);

    // 없으면 생성 하거나 반환
    if (!StaticMeshAsset)
    {
        std::optional<std::filesystem::path> MeshUAssetPath =
            FStaticMeshImporter::GetorImport(ObjPath);

        if (MeshUAssetPath)
        {
            TSharedPtr<FStaticMeshAssetLoader> MeshLoader = MakeShared<FStaticMeshAssetLoader>(*Renderer);
            TSharedPtr<FFileAssetSource> MeshSource = MakeShared<FFileAssetSource>(*MeshUAssetPath);

            // 등록 후 가져오기
            FAssetManager::Get().RegisterAsset(MeshAssetName, MeshLoader, MeshSource);
            StaticMeshAsset = FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(MeshAssetName, true);
        }
    }

    // GetorImport 실패할 경우를 대비해 fallback
    if (!StaticMeshAsset)
    {
        FMeshDescription MeshDescription;
        FStaticMeshBuildData BuildData;

        if (!Importer.ConvertToMeshDescription(ObjInfo, MeshDescription)) return nullptr;
        if (!FStaticMeshBuilder::Build(MeshDescription, BuildData)) return nullptr;

        StaticMeshAsset = MakeShared<FStaticMeshAsset>(FGuid::NewGuid(), MeshAssetName, *Renderer, BuildData);
        FAssetManager::Get().RegisterAsset(StaticMeshAsset);
        
    }

    UStaticMesh* StaticMesh = FObjectFactory::ConstructObject<UStaticMesh>();
    if (!StaticMesh) return nullptr;

    StaticMesh->SetStaticMeshAsset(StaticMeshAsset, FilePath, ObjInfo.Materials, DiffuseTextures);
    ObjStaticMeshMap.Add(FilePath, StaticMesh);
    

    #if 0
    // BuildData(vertices, indices) -> GPU에 올라갈 Asset 만들고
    TSharedPtr<FStaticMeshAsset> StaticMeshAsset =
		MakeShared<FStaticMeshAsset>(FGuid::NewGuid(), MeshAssetName, *Renderer, BuildData);

    // Asset Manager에 등록
    FAssetManager::Get().RegisterAsset(StaticMeshAsset);
    #endif

    return StaticMesh;
}
