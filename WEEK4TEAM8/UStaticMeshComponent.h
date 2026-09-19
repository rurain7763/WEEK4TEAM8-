#pragma once

#include "PrimitiveComponent.h"
#include "UStaticMesh.h"

class UStaticMeshComponent : public UPrimitiveComponent
{
	REFLECT_CLASS(UStaticMeshComponent, UPrimitiveComponent)

public:
	UStaticMeshComponent() = default;
	using UPrimitiveComponent::Initialize;
    void Initialize(const FString& InAssetPathFileName, FVector Location,
        FRotator Rotation, FVector Scale);

	virtual ~UStaticMeshComponent() = default;

	virtual void SerializeClass(json::JSON& outJson) const override;
	virtual void DeserializeClass(const json::JSON& inJson) override;

    virtual void Render(FRenderCollector& RenderCollector) override;
    virtual void GetRenderInfos(TArray<FRenderInfo>* OutRenderInfos) const override;

    virtual bool RayCastComponent(const FPickingRay& PickingRay, float& OutHitT) const override;

    void SetStaticMesh(UStaticMesh* InStaticMesh);
    void SetTexture(const TSharedPtr<FTexture2DAsset>& InTexture) { TextureAsset = InTexture; }
    UStaticMesh* GetStaticMesh() { return StaticMesh; }

    UStaticMesh* StaticMesh = nullptr;
    TSharedPtr<FTexture2DAsset> TextureAsset;
};



