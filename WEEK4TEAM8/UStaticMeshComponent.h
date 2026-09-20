#pragma once

#include "PrimitiveComponent.h"
#include "UStaticMesh.h"

class UStaticMeshComponent : public UPrimitiveComponent
{
	REFLECT_CLASS(UStaticMeshComponent, UPrimitiveComponent)

public:
	UStaticMeshComponent() = default;

	using UPrimitiveComponent::Initialize;
    void Initialize(const FString& InAssetPathFileName, FVector Location, FRotator Rotation, FVector Scale);

	virtual ~UStaticMeshComponent() = default;

	virtual void SerializeClass(json::JSON& outJson) const override;
	virtual void DeserializeClass(const json::JSON& inJson) override;

    virtual void Render(FRenderCollector& RenderCollector) override;

	FAABB GetBoundingBox() const override;

	const TArray<FVertex>& GetMeshVertices() const override
	{
		if (mMeshAsset)
		{
			return mMeshAsset->GetVertices();
		}

		return UPrimitiveComponent::GetMeshVertices();
	}

	const TArray<uint32>& GetMeshIndices() const override
	{
		if (mMeshAsset)
		{
			return mMeshAsset->GetIndices();
		}

		return UPrimitiveComponent::GetMeshIndices();
	}

    void SetStaticMesh(UStaticMesh* InStaticMesh);
    // void SetTexture(const TSharedPtr<FTexture2DAsset>& InTexture) { TextureAsset = InTexture; }
    UStaticMesh* GetStaticMesh() { return StaticMesh; }

    // 어떤 Material을 쓰게 할 것인지 Setter
    void SetMaterial(const TSharedPtr<FMaterialAsset>& InMaterial) { MaterialAsset = InMaterial; } 
    
    // 어떤 Material을 쓰고 있는지 Getter
    const TSharedPtr<FMaterialAsset>& GetMaterial() const { return MaterialAsset; }

	void SetUseVertexColor(bool bInUseVertexColor) { bUseVertexColor = bInUseVertexColor; }
	bool GetUseVertexColor() const { return bUseVertexColor; }
	void SetColor(const FVector4& InColor) { Color = InColor; }
	const FVector4& GetColor() const { return Color; }

	void SetMesh(const TSharedPtr<FStaticMeshAsset>& InMesh) { mMeshAsset = InMesh; }
    inline TSharedPtr<FStaticMeshAsset> GetMesh() { return mMeshAsset; }
    
	void SetTexture(const TSharedPtr<FTexture2DAsset>& InTexture) { mTextureAsset = InTexture; }
	inline TSharedPtr<FTexture2DAsset> GetTexture() { return mTextureAsset; }

	FVector2 GetUVOffset() const { return mUVOffset; }
	void SetUVOffset(const FVector2& InUVOffset) { mUVOffset = InUVOffset; }
    UStaticMesh* StaticMesh = nullptr;
    // TSharedPtr<FTexture2DAsset> TextureAsset;
    TSharedPtr<FMaterialAsset> MaterialAsset;

private:
	bool bUseVertexColor = true;
	FVector4 Color = FVector4(1.f, 1.f, 1.f, 1.f);
    TSharedPtr<FStaticMeshAsset> mMeshAsset;
    TSharedPtr<FTexture2DAsset> mTextureAsset;
	FVector2 mUVOffset = FVector2(0.f, 0.f);
};



