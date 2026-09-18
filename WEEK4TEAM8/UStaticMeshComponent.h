#pragma once
#include "PrimitiveComponent.h"
#include "Assets.h"
#include "RayCast.h"
#include "EngineMathLibrary.h"
#include "RenderInfo.h"
#include "enum.h"
#include "Actor.h"
#include "FAssetManager.h"

class UStaticMeshComponent : public UPrimitiveComponent
{
    REFLECT_CLASS(UStaticMeshComponent, UPrimitiveComponent)
public:
    UStaticMeshComponent();
    virtual ~UStaticMeshComponent() = default;

    void Initialize(
        FVector location = FVector(0.f, 0.f, 0.f),
        FRotator rotation = FRotator(0.f, 0.f, 0.f),
        FVector scale3D = FVector(1.f, 1.f, 1.f)
    );

    virtual void Render(FRenderCollector& RenderCollector) override;
    void SetStaticMesh(TSharedPtr<FStaticMeshAsset> InMeshAsset);

    virtual bool RayCastComponent(const FPickingRay& PickingRay, float& OutHitT) const override;
};