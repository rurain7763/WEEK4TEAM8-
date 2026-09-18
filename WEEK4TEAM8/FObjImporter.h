#pragma once

#include "Core.h"
#include "Assets.h"

// Raw Data

struct FObjInfo
{
    TArray<FVector> Vertices;
    TArray<FVector2> UV;
    TArray<FVector> Normal;
    TArray<int32> VertexIndices;
    TArray<int32> UVIndices;
    TArray<int32> NormalIndices;
    TArray<FString> Materials;
    FString MtlFileName;
};

struct FObjMaterialInfo
{
    FString MaterialName;
    FVector Ambient = { 1.0f,1.0f, 1.0f };
    FVector Diffuse = { 1.0f,1.0f, 1.0f };
    FVector Specular = { 1.0f,1.0f, 1.0f };
    float Ns = 0.0f;
    int32 illum = 0;
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

