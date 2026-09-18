#include "UStaticMeshComponent.h"


UStaticMeshComponent::UStaticMeshComponent()
{
    mePrimitive = EPrimitive::EP_Obj;
}

void UStaticMeshComponent::Initialize(FVector location, FRotator rotation, FVector scale3D)
{
    USceneComponent::Initialize(location, rotation, scale3D);
}

/*void UStaticMeshComponent::Render(FRenderCollector& RenderCollector)
{
    if (!mMeshAsset) return;

    if (FShowFlags::Get().IsEnabled(EShowFlag::Primitive))
    {
        FRenderInfo Info{};
        Info.StaticMesh = mMeshAsset;
        Info.WorldTransformMatrix = GetTransformMatrix().MakeMatrix();
        Info.ePrimitive = EPrimitive::EP_Obj;
        Info.Texture = GetTexture();
        if (mTextureAssets.IsEmpty() && mTextureAsset)
        {
            mTextureAssets.Add(mTextureAsset);
        }
        Info.Textures = mTextureAssets;
        Info.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

        RenderCollector.RenderInfos.Add(Info);
    }
}*/

bool UStaticMeshComponent::RayCastComponent(const FPickingRay& PickingRay, float& OutHitT) const
{
    if (!mMeshAsset) return false;

    const FMatrix WorldMatrix = GetTransformMatrix().MakeMatrix();
    const FAABB BoundingBox = mMeshAsset->GetLocalBoundingBox().ToWorld(WorldMatrix);

    return RayIntersectsAABB(PickingRay.ToRay(), PickingRay.Length, BoundingBox);
}

void UStaticMeshComponent::Render(FRenderCollector& RenderCollector)
{
    if (!mMeshAsset) return;

    if (FShowFlags::Get().IsEnabled(EShowFlag::Primitive))
    {
        FRenderInfo Info{};
        Info.StaticMesh = mMeshAsset;
        Info.WorldTransformMatrix = GetTransformMatrix().MakeMatrix();
        Info.ePrimitive = EPrimitive::EP_Obj;

        TSharedPtr<FTexture2DAsset> PrimaryTex = GetTexture();
        if (!PrimaryTex && mTextureAsset)
        {
            PrimaryTex = mTextureAsset;
        }
        Info.Texture = PrimaryTex;

        if (mTextureAssets.IsEmpty() && PrimaryTex)
        {
            mTextureAssets.Add(PrimaryTex);
        }
        else if (!mTextureAssets.IsEmpty() && !mTextureAssets[0] && PrimaryTex)
        {
            mTextureAssets[0] = PrimaryTex;
        }

        Info.Textures = mTextureAssets;
        Info.Color = FVector4(1.0f, 1.0f, 1.0f, 1.0f);

        RenderCollector.RenderInfos.Add(Info);
    }
}

void UStaticMeshComponent::SetStaticMesh(TSharedPtr<FStaticMeshAsset> InMeshAsset)
{
    mMeshAsset = InMeshAsset;
    if (!mMeshAsset) return;

    const auto& MaterialInfos = mMeshAsset->GetMaterialInfos();
    if (MaterialInfos.Num() > 0)
    {
        TArray<TSharedPtr<FTexture2DAsset>> LoadedTextures;
        bool bFoundAnyTexture = false;

        for (int32 i = 0; i < MaterialInfos.Num(); ++i)
        {
            const FString& TexPath = MaterialInfos[i].MaterialTexturePath;
            TSharedPtr<FTexture2DAsset> Tex = nullptr;
            if (TexPath.Len() > 0)
            {
                Tex = FAssetManager::Get().GetAssetAs<FTexture2DAsset>(FName(TexPath), true);
            }
            if (Tex)
            {
                bFoundAnyTexture = true;
            }
            LoadedTextures.Add(Tex);
        }

        if (bFoundAnyTexture)
        {
            mTextureAssets = LoadedTextures;
            if (!mTextureAssets.IsEmpty() && mTextureAssets[0])
            {
                mTextureAsset = mTextureAssets[0];
            }
        }
    }
}