#include "FStaticMeshImporter.h"
#include "FObjImporter.h"
#include "FArchive.h"
#include "FLogManager.h"
#include "FGuid.h"
#include "Serializers.h"
#include "FStaticMeshBuilder.h"
#include "FMaterialImporter.h"


// .obj -> .uasset 변환
bool FStaticMeshImporter::Import(const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead)
{
    FObjInfo ObjInfo;
    FObjImporter Importer;
    FMeshDescription MeshDescription;
    FStaticMeshBuildData BuildData;

    if (!Importer.ParseObj(FString(InPath.string()), ObjInfo))
    {
        UE_LOG_ERROR("Failed to Parse: %s", InPath.string().c_str());
        return false;
    }
    
    if (!Importer.ConvertToMeshDescription(ObjInfo, MeshDescription))
    {
        UE_LOG_ERROR("Failed to Convert into Mesh :: %s", InPath.string().c_str());
        return false;
    }
    
    if (!FStaticMeshBuilder::Build(MeshDescription, BuildData))
    {
        UE_LOG_ERROR("Failed to Build Cooked data :: %s", InPath.string().c_str());
        return false;
    }
    
    OutHead.Version = 1;
    OutHead.AssetType = EAssetType::StaticMesh;
    OutHead.AssetID = FGuid::NewGuid();

    
    try
    {
        FWindowsBinWriter FileWriter(OutPath);
        
        // FMeshDescription MeshDescription;
        for (FObjMaterialInfo Material : ObjInfo.Materials)
        {
            std::filesystem::path MaterialPath = OutPath.parent_path() / (OutPath.stem().string() + "_" + Material.Name.CStr() + ".uasset");
            FAssetFileHeader MaterialHeader;
            FMaterialImporter::Import(Material, InPath.parent_path(), MaterialPath, MaterialHeader);
            
            for (FStaticMeshSection& Section : BuildData.Sections)
            {
                if (Section.MaterialName == Material.Name)
                {
                    Section.MaterialAssetID = MaterialHeader.AssetID;
                }
            }
        }

        FileWriter << OutHead;
        FileWriter << BuildData.Vertices;
        FileWriter << BuildData.Indices;
        FileWriter << BuildData.Sections;
    }
    catch(const std::exception& e)
    {
        UE_LOG_ERROR("Failed to write to %s", OutPath.string().c_str());
        return false;
    }
    
    return true;
}   

// .uasset 반환 or 변환 후 반환
std::optional<std::filesystem::path> FStaticMeshImporter::GetorImport(const std::filesystem::path& InPath)
{
    std::filesystem::path CheckPath = InPath;
    CheckPath.replace_extension(".uasset");
    
    // .uasset 파일 있으면 그 경로 반환
    if(std::filesystem::exists(CheckPath))
        return CheckPath;

    // 없으면 Import 함수로 생성
    FAssetFileHeader OutHeader;
    if (Import(InPath, CheckPath, OutHeader))
        return CheckPath;
    else
        return std::nullopt;
}