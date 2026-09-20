#pragma once

#include "Matrix.h"
#include "Enum.h"

#include "TArray.h"
#include "TMap.h"
#include "Renderer.h"
#include "Camera.h"
#include "RenderInfo.h"
#include "Vector.h"
#include "ShowFlags.h"

class FAssetManager;
struct FViewport;

struct FBuffer
{
	ID3D11Buffer* Buffer;
	uint32 SourceNum;
};

class FGraphicsManager
{
public:
	FGraphicsManager(HWND hWindow);
	~FGraphicsManager();

	//void Prepare(const Camera* mCamera);
	void Prepare(const FCamera* Camera,float viewportWidth, float viewportHeight, const FViewport& viewport);
	void GizmoPrepare();

	// 표시 옵션은 FShowFlags가 들고 있다. 여기서 중계하지 않는다.
#if 0
	static FVector GetPrimitiveCenter(EPrimitive type);
	static FVector GetPrimitiveHalfExtent(EPrimitive type);
	void RenderHighLight(const FRenderInfo& RI);
#else
	void RenderHighLight(const TArray<UPrimitiveComponent*>& Primitives);
#endif

	void Render();

	void Display();

	float GetAspect() const { return mAspect; }
	EViewModeIndex GetViewModeIndex() const { return mViewModeIndex; }
	void SetViewModeIndex(EViewModeIndex viewModeIndex) { mViewModeIndex = viewModeIndex; }

	bool IsPerspectiveProjection() const;
	void SetPerspectiveProjection(bool bPerspectiveProjection);

	float GetPerspectiveRatio() const { return mProjectionRatio; }
	const FMatrix& GetViewProjectionMatrix() const { return mViewUnifiedProjectionMatrix; }
	void SetPerspectiveRatio(float ratio) { mProjectionRatio = FMath::Clamp(ratio, 0.0f, 1.0f); }

	float GetCameraOrthoDistance() const { return mCameraOrthoDistance; }
	void SetCameraOrthoDistance(float distance) { mCameraOrthoDistance = distance; }

	// Todo: Change name
	URenderer* GetRenderer() const;
	void OnResize(UINT width, UINT height);

	//Highlight
	//Line batch
	// 호출 즉시 그리지 않고 배열에 쌓는다. FlushLines()에서 한 번에 그린다.
	void DrawLine(const FVector& start, const FVector& end, const FVector4& color);
	void FlushLines();

	// Projection ratio smoothing
	void StartProjectionTransition(bool orthographic);
	bool IsOrthographicTarget() const;
	void UpdateProjectionTransition(float deltaTime);

	inline FRenderCollector& GetRenderCollector() { return mRenderCollector; }
	inline TArray<FRenderInfo>& GetRenderInfos() { return mRenderCollector.RenderInfos; }

	inline int32 GetGridGap() { return GridGap; }
	void SetGridGap(int32 GridGap);

private:
	URenderer* mRenderer;
	FMatrix mViewMatrix;
	FMatrix mProjectionMatrix;
	FMatrix mViewProjectionMatrix;
	FMatrix mViewOrthogonalProjectionMatrix;
	FMatrix mViewUnifiedProjectionMatrix;

	// Prepare에서 갱신. 하이라이트 두께의 픽셀 → 월드 환산에 쓴다
	FVector mCameraLocation;
	FVector mCameraForward;
	float mCameraFovDegree = 60.0f;
	float mCameraOrthoDistance = 10.0f;

	// Graphics config
	// 이번 프레임에 쌓인 선분. 정점 2개가 선분 하나
	TArray<FVertexSimple> mLineVertices;

	EViewModeIndex mViewModeIndex = EViewModeIndex::VMI_Lit;
	bool mbPerspectiveProjection;
	float mAspect;
	float mProjectionRatio; // 0.0f ~ 1.0f, 0이면 직교, 1이면 원근, 그 사이면 혼합

	// Projection ratio smoothing
	float mProjectionStartRatio = 1.0f;
	float mProjectionTargetRatio = 1.0f;
	float mProjectionElapsed = 0.0f;
	float mProjectionDuration = 1.0f;
	bool mbProjectionTransitioning = false;

	TSharedPtr<FRenderPipeline> mMeshPipeline;

	TSharedPtr<FRenderPipeline> mHighlightMarkPipeline;
	TSharedPtr<FRenderPipeline> mHighlightDrawPipeline;
	TSharedPtr<FVertexBuffer> mHighlightVertexBuffer;
	TSharedPtr<FIndexBuffer> mHighlightIndexBuffer;

	FRenderCollector mRenderCollector;

	int32 GridGap = 1;
};
