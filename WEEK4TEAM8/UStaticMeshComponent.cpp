#include "UStaticMeshComponent.h"

#include "FAssetManager.h"
#include "RenderInfo.h"
#include "ShowFlags.h"
#include "Actor.h"
#include "JsonUtil.h"
#include "FObjManager.h"
#include "EngineMathLibrary.h"
#include "FLogManager.h"


void UStaticMeshComponent::Initialize(const FString& InAssetPathFileName, FVector Location,
    FRotator Rotation,  FVector Scale)
{
    USceneComponent::Initialize(Location, Rotation, Scale);

    // OBJ
    StaticMesh = FObjManager::LoadObjStaticMesh(InAssetPathFileName);
    if (StaticMesh)
    {
        mMeshAsset = StaticMesh->GetStaticMeshAsset();
    }
}

void UStaticMeshComponent::SerializeClass(json::JSON& outJson) const
{
    USceneComponent::SerializeClass(outJson);

	FGuid AssetID = mMeshAsset ? mMeshAsset->GetAssetID() : FGuid();
	outJson["Properties"]["ObjStaticMeshAsset"] = FGuidToJson(AssetID);
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

    if (PropertiesJson.at("ObjStaticMeshAsset").JSONType() != json::JSON::Class::Object)
    {
        throw std::runtime_error("UStaticMeshComponent: ObjStaticMeshAsset property requires an object");
    }

	FGuid AssetID = FGuidFromJson(PropertiesJson.at("ObjStaticMeshAsset"));

	if (AssetID.IsValid())
	{
		mMeshAsset = FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(AssetID, true);
	}
}

//void UStaticMeshComponent::SetMeshAsset(const FName& InMeshAssetName)
//{
//    MeshAssetName = InMeshAssetName;
//    MeshAsset = FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(MeshAssetName, true);
//}

void UStaticMeshComponent::Render(FRenderCollector& RenderCollector)
{
    if (!mMeshAsset)
    {
        return;
    }
    
    if (!FShowFlags::Get().IsEnabled(EShowFlag::Primitive))
    {
        return;
    }

    for (int32 SectionIndex = 0; SectionIndex < mMeshAsset->GetSections().Num(); ++SectionIndex)
    {
        const FStaticMeshSection& Section = mMeshAsset->GetSections()[SectionIndex];
        TSharedPtr<FMaterialAsset> Material = FAssetManager::Get().GetAssetAs<FMaterialAsset>(Section.MaterialAssetID, true);

        const FVector4 MaterialColor = Material
            ? FVector4(
                Material->GetDiffuseColor().x,
                Material->GetDiffuseColor().y,
                Material->GetDiffuseColor().z,
                Material->GetOpacity())
            : FVector4(1, 1, 1, 1);

        TSharedPtr<FTexture2DAsset> SectionTexture = Material ? Material->GetDiffuseTexture() : nullptr;

        if (!SectionTexture && StaticMesh)
        {
            SectionTexture = StaticMesh->GetDiffuseTexture(Section.MaterialName);
        }
        if (!SectionTexture)
        {
            SectionTexture = mTextureAsset;
        }

        FRenderInfo RenderInfo;
        RenderInfo.VertexBuffer = mMeshAsset->GetVertexBuffer();
        RenderInfo.IndexBuffer = mMeshAsset->GetIndexBuffer();
        RenderInfo.StartIndex = Section.FirstIndex;
        RenderInfo.IndexCount = Section.IndexCount;
        RenderInfo.Texture = SectionTexture;
        RenderInfo.UVOffset = mUVOffset;
        RenderInfo.ePrimitive = EPrimitive::EP_StaticMesh;
        RenderInfo.Model = GetTransformMatrix().MakeMatrix();
        RenderInfo.Color = bUseVertexColor ? MaterialColor : Color;
        RenderInfo.UseVertexColor = bUseVertexColor;
        RenderInfo.ObjectInternalIndex = mOwner->InternalIndex;

        RenderCollector.RenderInfos.Add(RenderInfo);
    }
}

FAABB UStaticMeshComponent::GetBoundingBox() const
{
	if (!mMeshAsset)
	{
		return FAABB();
	}

	return mMeshAsset->GetLocalBoundingBox().ToWorld(GetTransformMatrix().MakeMatrix());
}
