#include "FMaterialImporter.h"
#include "AssetFileIOs.h"
#include "FArchive.h"
#include "FLogManager.h"
#include "FGuid.h"
#include "Serializers.h"
#include "FTexture2DImporter.h"

bool FMaterialImporter::Import(const FObjMaterialInfo& MaterialInfo, const std::filesystem::path& InPath, const std::filesystem::path& OutPath, FAssetFileHeader& OutHead)
{
    OutHead.Version = 1;
    OutHead.AssetType = EAssetType::Material;
    OutHead.AssetID = FGuid::NewGuid();

    try
    {
        FGuid DiffuseTextureID;

        if (MaterialInfo.DiffuseTexturePath.Len() != 0)
        {
            // Texture Guid 가져오기
            std::filesystem::path TexturePath = InPath / MaterialInfo.DiffuseTexturePath.CStr();
            std::optional<std::filesystem::path> TextureUAssetPath = FTexture2DImporter::GetorImport(TexturePath);
            if (!TextureUAssetPath)
            {
                UE_LOG_ERROR("Failed to find Diffuse Texture: %s", TexturePath.string().c_str());
                return false;
            }
            
            FWindowsBinReader Reader(*TextureUAssetPath);
            FAssetFileHeader TextureHeader;
            Reader << TextureHeader;
            DiffuseTextureID = TextureHeader.AssetID;
            
        }   
        
        FVector ZeroVector(0.0f);
        FGuid EmptyGuid;
        FWindowsBinWriter FileWriter(OutPath);

        FVector DiffuseColor = MaterialInfo.DiffuseColor;  // const 떼어내기
        float Opacity = MaterialInfo.Opacity;

        FileWriter << OutHead;
        FileWriter << DiffuseColor;              // Ambient Color ( 임시로 diffuse 사용 )
        FileWriter << DiffuseColor;              // Diffuse Color
        FileWriter << Opacity;
        FileWriter << ZeroVector;                // Specular Color ( 임시로 black 사용 )
        FileWriter << DiffuseTextureID;          // Diffuse Texture
        FileWriter << EmptyGuid;                 // Specular Texture ( FGuid-0000 '없음' )
        FileWriter << EmptyGuid;                 // Normal Texture
    }
    catch(const std::exception& e)
    {
        UE_LOG_ERROR("Failed to write to %s", OutPath.string().c_str());
        return false;
    }

    return true;
}