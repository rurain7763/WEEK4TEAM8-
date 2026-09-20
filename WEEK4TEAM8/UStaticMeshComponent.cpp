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

    for (const FStaticMeshSection& Section : mMeshAsset->GetSections())
    {
		// TODO: SubMesh별로 마테리얼을 통해 다른 텍스처를 적용할 수 있도록 수정 필요
		FRenderInfo RenderInfo;
        RenderInfo.VertexBuffer = mMeshAsset->GetVertexBuffer();
		RenderInfo.IndexBuffer = mMeshAsset->GetIndexBuffer();
        RenderInfo.StartIndex = Section.FirstIndex;
		RenderInfo.IndexCount = Section.IndexCount;
		RenderInfo.Texture = mTextureAsset;
		RenderInfo.UVOffset = mUVOffset;
		RenderInfo.ePrimitive = EPrimitive::EP_StaticMesh;
		RenderInfo.Model = GetTransformMatrix().MakeMatrix();
		RenderInfo.Color = FVector4(1, 1, 1, 1);
		RenderInfo.UseVertexColor = mTextureAsset == nullptr;

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
