#include "UStaticMeshComponent.h"

#include "FAssetManager.h"
#include "RenderInfo.h"
#include "ShowFlags.h"
#include "Actor.h"
#include "JsonUtil.h"
#include "FObjManager.h"

void UStaticMeshComponent::Initialize(const FString& InAssetPathFileName, FVector Location,
    FRotator Rotation,  FVector Scale)
{
    USceneComponent::Initialize(Location, Rotation, Scale);
    //SetMeshAsset(InMeshAssetName);
    StaticMesh = FObjManager::LoadObjStaticMesh(InAssetPathFileName);
}

void UStaticMeshComponent::SerializeClass(json::JSON& outJson) const
{
    USceneComponent::SerializeClass(outJson);

    //outJson["Properties"]["MeshAssetName"] = MeshAssetName.ToString().CStr();
    outJson["Properties"]["ObjStaticMeshAsset"] = StaticMesh ? StaticMesh->GetAssetPathFileName().CStr() : "";
}

void UStaticMeshComponent::DeserializeClass(const json::JSON& inJson)
{
    USceneComponent::DeserializeClass(inJson);

    const json::JSON& PropertiesJson = inJson.at("Properties");

    /*if (!PropertiesJson.hasKey("MeshAssetName") || PropertiesJson.at("MeshAssetName").JSONType() != json::JSON::Class::String)
    {
        throw std::runtime_error("UStaticMeshComponent: MeshAssetName property requires a string");
    }*/
    /*MeshAssetName = FName(PropertiesJson.at("MeshAssetName").ToString());
    SetMeshAsset(MeshAssetName);*/

    if (!PropertiesJson.hasKey("ObjStaticMeshAsset"))
    {
        throw std::runtime_error("UStaticMeshComponent: ObjStaticMeshAsset property is required");
    }
    if (PropertiesJson.at("ObjStaticMeshAsset").JSONType() != json::JSON::Class::String)
    {
        throw std::runtime_error("UStaticMeshComponent: ObjStaticMeshAsset property requires a string");
    }

    const FString AssetPathFileName = PropertiesJson.at("ObjStaticMeshAsset").ToString();
    SetStaticMesh(FObjManager::LoadObjStaticMesh(AssetPathFileName));
}

void UStaticMeshComponent::SetStaticMesh(UStaticMesh* InStaticMesh)
{
    StaticMesh = InStaticMesh;
}

//void UStaticMeshComponent::SetMeshAsset(const FName& InMeshAssetName)
//{
//    MeshAssetName = InMeshAssetName;
//    MeshAsset = FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(MeshAssetName, true);
//}

void UStaticMeshComponent::Render(FRenderCollector& RenderCollector)
{
    if (!StaticMesh)
    {
        return;
    }
    
    const TSharedPtr<FStaticMeshAsset>& MeshAsset = StaticMesh->GetStaticMeshAsset();

    if (!FShowFlags::Get().IsEnabled(EShowFlag::Primitive))
    {
        return;
    }

    for (const FStaticMeshSection& Section : MeshAsset->GetSections())
    {
        const FObjMaterialInfo* Material =  StaticMesh->FindMaterial(static_cast<std::string>(Section.MaterialName));

        const FVector4 MaterialColor = Material
            ? FVector4(
                Material->DiffuseColor.x,
                Material->DiffuseColor.y,
                Material->DiffuseColor.z,
                Material->Opacity)
            : FVector4(1, 1, 1, 1);

        TSharedPtr<FTexture2DAsset> SectionTexture = StaticMesh->GetDiffuseTexture(Section.MaterialName);
        if (!SectionTexture)
        {
            SectionTexture = TextureAsset;
        }

        RenderCollector.RenderInfos.Add({
            MeshAsset,
            SectionTexture,
            EPrimitive::EP_Cube,
            GetTransformMatrix().MakeMatrix(),
            { mOwner->UUID, mOwner->InternalIndex },
            MaterialColor,
            Section.FirstIndex,
            Section.IndexCount,
            false
            });
    }
}

void UStaticMeshComponent::GetRenderInfos(TArray<FRenderInfo>* OutRenderInfos) const
{
    if (!OutRenderInfos || !StaticMesh)
    {
        return;
    }

    const TSharedPtr<FStaticMeshAsset>& MeshAsset = StaticMesh->GetStaticMeshAsset();
    for (const FStaticMeshSection& Section : MeshAsset->GetSections())
    {
        const FObjMaterialInfo* Material =  StaticMesh->FindMaterial(static_cast<std::string>(Section.MaterialName));

        const FVector4 MaterialColor = Material
            ? FVector4(
                Material->DiffuseColor.x,
                Material->DiffuseColor.y,
                Material->DiffuseColor.z,
                Material->Opacity)
            : FVector4(1, 1, 1, 1);

        TSharedPtr<FTexture2DAsset> SectionTexture = StaticMesh->GetDiffuseTexture(Section.MaterialName);
        if (!SectionTexture)
        {
            SectionTexture = TextureAsset;
        }

        OutRenderInfos->Add({
            MeshAsset,
            SectionTexture,
            EPrimitive::EP_Cube,
            GetTransformMatrix().MakeMatrix(),
            { mOwner->UUID, mOwner->InternalIndex },
            MaterialColor,
            Section.FirstIndex,
            Section.IndexCount,
            false
            });
    }
}