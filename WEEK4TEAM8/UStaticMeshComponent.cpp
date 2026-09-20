#include "UStaticMeshComponent.h"

#include "FAssetManager.h"
#include "RenderInfo.h"
#include "ShowFlags.h"
#include "Actor.h"
#include "JsonUtil.h"
#include "FObjManager.h"
#include "EngineMathLibrary.h"


void UStaticMeshComponent::Initialize(const FString& InAssetPathFileName, FVector Location,
    FRotator Rotation,  FVector Scale)
{
    USceneComponent::Initialize(Location, Rotation, Scale);
    //SetMeshAsset(InMeshAssetName);
    StaticMesh = FObjManager::LoadObjStaticMesh(InAssetPathFileName);
}

void UStaticMeshComponent::InitializeFromStaticMesh(UStaticMesh* InStaticMesh, FVector Location, FRotator Rotation, FVector Scale)
{
    USceneComponent::Initialize(Location, Rotation, Scale);
    StaticMesh = InStaticMesh;

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



    for (int32 SectionIndex = 0; SectionIndex < MeshAsset->GetSections().Num(); ++SectionIndex)
    {
        const FStaticMeshSection& Section =  MeshAsset->GetSections()[SectionIndex];
        TSharedPtr<FMaterialAsset> Material = FAssetManager::Get().GetAssetAs<FMaterialAsset>(Section.MaterialAssetID, true);

        const FVector4 MaterialColor = Material
            ? FVector4(
                Material->GetDiffuseColor().x,
                Material->GetDiffuseColor().y,
                Material->GetDiffuseColor().z,
                Material->GetOpacity())
            : FVector4(1, 1, 1, 1);

        TSharedPtr<FTexture2DAsset> SectionTexture = Material ? Material->GetDiffuseTexture() : nullptr;
        if (!SectionTexture)
        {
            SectionTexture = StaticMesh->GetDiffuseTexture(Section.MaterialName);
        }

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
            bUseVertexColor ? MaterialColor : Color,
            Section.FirstIndex,
            Section.IndexCount,
            bUseVertexColor
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
    for (int32 SectionIndex = 0; SectionIndex < MeshAsset->GetSections().Num(); ++SectionIndex)
    {
        const FStaticMeshSection& Section = MeshAsset->GetSections()[SectionIndex];
        TSharedPtr<FMaterialAsset> Material = FAssetManager::Get().GetAssetAs<FMaterialAsset>(Section.MaterialAssetID, true);

        const FVector4 MaterialColor = Material
            ? FVector4(
                Material->GetDiffuseColor().x,
                Material->GetDiffuseColor().y,
                Material->GetDiffuseColor().z,
                Material->GetOpacity())
            : FVector4(1, 1, 1, 1);

        TSharedPtr<FTexture2DAsset> SectionTexture = Material ? Material->GetDiffuseTexture() : nullptr;
        if (!SectionTexture)
        {
            SectionTexture = StaticMesh->GetDiffuseTexture(Section.MaterialName);
        }
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
            bUseVertexColor ? MaterialColor : Color,
            Section.FirstIndex,
            Section.IndexCount,
            bUseVertexColor
            });
    }
}

bool UStaticMeshComponent::RayCastComponent(const FPickingRay& PickingRay, float& OutHitT) const
{
    if (!StaticMesh)
    {
        return false;
    }

    const FMatrix WorldMatrix = GetTransformMatrix().MakeMatrix();

    const FStaticMeshAsset* meshAsset = StaticMesh->GetStaticMeshAsset().get();
    const auto& vertices = meshAsset->GetCpuVertices();
    const auto& indices = meshAsset->GetCpuIndices();

    const FAABB BoundingBox = meshAsset->GetLocalBoundingBox().ToWorld(WorldMatrix);
    if (!RayIntersectsAABB(PickingRay.ToRay(), PickingRay.Length, BoundingBox))
    {
        return false;
    }

    const FMatrix WorldToLocal = WorldMatrix.AffineInverse();
    if (WorldToLocal == FMatrix::Zero)
    {
        return false;
    }

    const FVector LocalNear = WorldToLocal.TransformPosition(PickingRay.Near);
    const FVector LocalFar = WorldToLocal.TransformPosition(PickingRay.Far);

    bool bHit = false;
    float NearestT = FLT_MAX;

    uint32 indexCount = indices.Num();

    for (int32 i = 0; i < indexCount; i += 3)
    {
        const FVector V0 = vertices[indices[i]].GetPosition();
        const FVector V1 = vertices[indices[i + 1]].GetPosition();
        const FVector V2 = vertices[indices[i + 2]].GetPosition();

        float OutT, OutU, OutV;
        if (RayIntersectsTriangle(LocalNear, LocalFar, V0, V1, V2, OutT, OutU, OutV) && OutT < NearestT)
        {
            NearestT = OutT;
            bHit = true;
        }
    }

    if (bHit)
    {
        OutHitT = NearestT;
    }

    return bHit;
}