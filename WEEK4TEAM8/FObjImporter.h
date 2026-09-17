#pragma once

#include "Core.h"
#include "Assets.h"

// Raw Data

struct FObjInfo
{
    TArray<FVector> Vertices;
    TArray<FVector2> UV;
    TArray<FVector> Normal;
    TArray<uint32> VertexIndices;
    TArray<uint32> UVIndices;
    TArray<uint32> NormalIndices;
    TArray<FString> Materials;
    TArray<FString> Textures;
};

struct FObjMaterialInfo
{
    FString MaterialName;
    FVector4 DiffuseColor = { 1.0f,1.0f, 1.0f, 1.0f };
    FString MaterialTexturePath;
};

struct FObjImporter
{
public:
    static TSharedPtr<FStaticMeshAsset> Import(const FName& InAssetName, URenderer& InRenderer, const FString& FilePath);

private:
    static bool ParseOBJ(const FString& FilePath, FObjInfo& OutRawData);

    static bool ParseMTL(const FString& MtlFilePath, TArray<FObjMaterialInfo>& OutMaterials);

    static TSharedPtr<FStaticMeshAsset> BuildStaticMesh(
        const FName& InAssetName,
        URenderer& InRenderer,
        const FObjInfo& InRawData,
        const FString& FilePath
    );
};

