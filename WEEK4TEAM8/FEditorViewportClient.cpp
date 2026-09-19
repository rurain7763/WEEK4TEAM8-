#include "FEditorViewportClient.h"

#include "Cube.h"
#include "Sphere.h"
#include "Triangle.h"
#include "GizmoArrow.h"
#include "Circle.h"
#include "Plane.h"
#include "WindowApplication.h"
#include "ImGui/imgui.h"
#include "Console.h"
#include "SceneManager.h"
#include "MathUtility.h"
#include "GraphicsManager.h"
#include "Renderer.h"
#include <cstdio>
#include "UTextComponent.h"
#include "EngineMathLibrary.h"
#include "PrimitiveComponent.h"
#include "RayCast.h"

FEditorViewportClient::FEditorViewportClient(URenderer& InRenderer)
	: mCamera(FTransform({ -2.0f, 1.0f, 1.0f }, { 0, 30, 0 }, { 1, 1, 1 }))
	, mGizmo(InRenderer)
{
	char Value[64] = {};
	GetPrivateProfileStringA("Camera", "Sensitivity", "", Value, sizeof(Value), ".\\editor.ini");
	float Sensitivity = 0.1f;
	if (sscanf_s(Value, "%f", &Sensitivity) == 1 && Sensitivity >= 0.01f && Sensitivity <= 1.0f)
	{
		mCamera.SetSensitivity(Sensitivity);
	}
}

AActor* FEditorViewportClient::PerformMousePicking(FCamera& InCamera, int32 MouseX, int32 MouseY, float ScreenW, float ScreenH, float perspectiveRatio, const FRenderCollector& RenderCollector)
{
	bMouseHit = false;
	if (ScreenW <= 0.f || ScreenH <= 0.f) return nullptr;

	const float ndcX = (2.0f * (MouseX + 0.5f) / ScreenW) - 1.0f;
	const float ndcY = 1.0f - (2.0f * (MouseY + 0.5f) / ScreenH);

	const FMatrix invProjection = InCamera.GetInverseUnifiedProjectionMatrix( ScreenW / ScreenH, InCamera.mFovDegree, InCamera.mOrthoDistance, 0.1f, 100.f, perspectiveRatio);
	const FMatrix invViewProj = invProjection * InCamera.GetViewMatrix().AffineInverse();

	const auto Unproject = [&](float ndcZ) -> FVector
		{
			const FVector xyz = invViewProj.TransformPosition(FVector(ndcX, ndcY, ndcZ));
			const float w = ndcX * invViewProj.M[0][3] + ndcY * invViewProj.M[1][3] + ndcZ * invViewProj.M[2][3] + invViewProj.M[3][3];
			return xyz * (1.0f / w);
		};

	mRayNear = Unproject(0.0f);
	mRayFar = Unproject(1.0f);

	float NearestT = FLT_MAX;
	AActor* NearestActor = nullptr;
	const FPickingRay PickingRay(mRayNear, mRayFar);

	for (UPrimitiveComponent* PickTarget : RenderCollector.PickTargets)
	{
		float HitT = FLT_MAX;
		if (!PickTarget->RayCastComponent(PickingRay, HitT)) continue;

		if (HitT < NearestT)
		{
			NearestT = HitT;
			bMouseHit = true;
			NearestActor = PickTarget->GetOwner();
		}
	}

	return NearestActor;
}


void FEditorViewportClient::Update(float deltaTime, FCamera& InCamera, float perspectiveRatio, FSceneManager* sceneManager)
{
	const FInputState& Input = WindowApplication.Input;

	if (perspectiveRatio < 0.5f)
	{
		if (Input.IsDown(VK_RBUTTON))
		{
			float DeltaX = static_cast<float>(Input.MouseDX);
			float DeltaY = static_cast<float>(Input.MouseDY);

			float PanSpeed = InCamera.mOrthoDistance * 0.0015f;

			InCamera.Transform.Location += -InCamera.GetRightVector() * (DeltaX * PanSpeed);
			InCamera.Transform.Location += InCamera.GetUpVector() * (DeltaY * PanSpeed);
		}

		if (Input.MouseWheelDelta != 0)
		{
			float ZoomFactor = (Input.MouseWheelDelta > 0) ? 0.85f : 1.15f;
			InCamera.mOrthoDistance = FMath::Clamp(InCamera.mOrthoDistance * ZoomFactor, 0.5f, 500.0f);
		}

		return;
	}

	bool bAllowMouse = sceneManager->IsViewportHovered();
	bool bAllowKeyboardInput = bAllowMouse && !ImGui::GetIO().WantCaptureKeyboard;

	const bool bIsOrtho = (perspectiveRatio < 1.0f);

	if (bAllowMouse && Input.IsDown(VK_RBUTTON) && !bIsOrtho)
	{
		InCamera.Rotate(Input.MouseDX, Input.MouseDY);
	}

	FVector MoveDir(0.f, 0.f, 0.f);
	if (bAllowKeyboardInput)
	{
		if (!bIsOrtho)
		{
			const FMatrix R = FMatrix::Rotate(InCamera.Transform.Rotation);
			const FVector Forward = R.GetUnitAxis(EAxis::X);
			const FVector Right = R.GetUnitAxis(EAxis::Y);

			if (Input.IsDown('W')) MoveDir += Forward;
			if (Input.IsDown('S')) MoveDir -= Forward;
			if (Input.IsDown('D')) MoveDir += Right;
			if (Input.IsDown('A')) MoveDir -= Right;
			if (Input.IsDown('E')) MoveDir += FVector(0.f, 0.f, 1.f);
			if (Input.IsDown('Q')) MoveDir -= FVector(0.f, 0.f, 1.f);
		}
		else
		{
			const FVector Up = InCamera.GetUpVector();
			const FVector Right = InCamera.GetRightVector();

			if (Input.IsDown('W')) MoveDir += Up;
			if (Input.IsDown('S')) MoveDir -= Up;
			if (Input.IsDown('D')) MoveDir += Right;
			if (Input.IsDown('A')) MoveDir -= Right;
		}
	}

	const bool bMoveKeyDown = !MoveDir.IsNearlyZero();
	if (bMoveKeyDown)
	{
		MoveDir.Normalize();
	}

	if (bAllowMouse && Input.MouseWheelDelta != 0.0f)
	{
		if (!bMoveKeyDown)
		{
			if (bIsOrtho)
			{
				InCamera.mOrthoDistance *= FMath::Pow(1.2f, -Input.MouseWheelDelta);
				InCamera.mOrthoDistance = FMath::Clamp(InCamera.mOrthoDistance, 0.1f, 100.0f);
			}
			else
			{
				InCamera.Transform.Location += InCamera.GetForwardVector() * 1.5f * Input.MouseWheelDelta;
			}
		}
		else
		{
			InCamera.Speed *= FMath::Pow(1.2f, Input.MouseWheelDelta);
			InCamera.Speed = FMath::Clamp(InCamera.Speed, 0.1f, 100.0f);
		}
	}

	const FVector TargetVelocity = MoveDir * InCamera.Speed;
	const float Alpha = FMath::Exp(-InCamera.Damping * deltaTime);
	InCamera.Velocity = TargetVelocity + (InCamera.Velocity - TargetVelocity) * Alpha;
	if (InCamera.Velocity.IsNearlyZero())
	{
		InCamera.Velocity = FVector(0.f);
	}

	InCamera.Transform.Location += InCamera.Velocity * deltaTime;

	if (bAllowKeyboardInput)
	{
		if (Input.WasPressed(VK_SPACE))
		{
			mGizmo.SetOperation(static_cast<EGIZMO_TYPE>((static_cast<int32>(mGizmo.GetOperation()) + 1) % 3));
		}

		if (Input.WasPressed('V'))
		{
			mGizmo.SetWorldMode(true);
		}
		else if (Input.WasPressed('B'))
		{
			mGizmo.SetWorldMode(false);
		}
	}
}

void FEditorViewportClient::DeprojectScreenToWorld(int32 MouseX, int32 MouseY, float ScreenW, float ScreenH, float NearZ, float FarZ, FVector& OutNearPoint, FVector& OutFarPoint)
{
	// 1) 픽셀 -> NDC. 화면 Y 는 아래로 +, NDC Y 는 위로 + 라서 뒤집는다
	const float ndcX = (2.0f * (MouseX + 0.5f) / ScreenW) - 1.0f;
	const float ndcY = 1.0f - (2.0f * (MouseY + 0.5f) / ScreenH);

	// 2) 투영 스케일 항 — GetProjectionMatrix 와 반드시 같은 식이어야 한다
	const float Aspect = ScreenW / ScreenH;
	const float yScale = 1.0f / tanf(mCamera.mFovDegree * 0.5f * PI / 180.f);
	const float xScale = yScale / Aspect;

	// 3) 카메라 기저로 월드 방향 합성. 전방 성분이 1 이므로 정규화하면 안 된다
	const FMatrix R = FMatrix::Rotate(mCamera.Transform.Rotation);
	FVector V = R.GetUnitAxis(EAxis::X);                    // 전방 (성분 1)
	V += R.GetUnitAxis(EAxis::Y) * (ndcX / xScale);         // 우측
	V += R.GetUnitAxis(EAxis::Z) * (ndcY / yScale);         // 상방

	// 4) 곱하면 그대로 각 평면 위의 점
	OutNearPoint = mCamera.Transform.Location + V * NearZ;
	OutFarPoint = mCamera.Transform.Location + V * FarZ;
}

void FEditorViewportClient::DeprojectScreenToWorldForOrtho(int32 MouseX, int32 MouseY, float ScreenW, float ScreenH, float NearZ, float FarZ, FVector& OutNearPoint, FVector& OutFarPoint)
{
	// 1) 픽셀 -> NDC. 화면 Y 는 아래로 +, NDC Y 는 위로 + 라서 뒤집는다
	const float ndcX = (2.0f * (MouseX + 0.5f) / ScreenW) - 1.0f;
	const float ndcY = 1.0f - (2.0f * (MouseY + 0.5f) / ScreenH);

	// 2) 화면이 담는 월드 크기 — GetOrthographicMatrix 에 넘기는 값과 반드시 같아야 한다.
	//    직교 행렬은 2/width, 2/height 로 나누므로 되돌리려면 절반을 곱한다
	const float Aspect = ScreenW / ScreenH;
	const float orthoHeight = mCamera.mOrthoHeight;
	const float orthoWidth = orthoHeight * Aspect;

	const FMatrix R = FMatrix::Rotate(mCamera.Transform.Rotation);
	const FVector Forward = R.GetUnitAxis(EAxis::X);
	const FVector Right = R.GetUnitAxis(EAxis::Y);
	const FVector Up = R.GetUnitAxis(EAxis::Z);

	// 3) 원근과 결정적으로 다른 점: 방향이 아니라 시작점이 픽셀마다 달라진다.
	//    모든 광선이 전방과 나란하고, 카메라 평면 위에서 평행이동한 자리에서 출발한다
	const FVector RayOrigin = mCamera.Transform.Location
		+ Right * (ndcX * orthoWidth * 0.5f)
		+ Up * (ndcY * orthoHeight * 0.5f);

	OutNearPoint = RayOrigin + Forward * NearZ;
	OutFarPoint = RayOrigin + Forward * FarZ;
}

void FEditorViewportClient::DeprojectScreenToWorldForUnified(
	int32 MouseX, int32 MouseY,
	float ScreenW, float ScreenH, float NearZ, float FarZ,
	float orthoDistance, float perspectiveRatio,
	FVector& OutNearPoint, FVector& OutFarPoint
)
{
	const float ndcX = (2.0f * (MouseX + 0.5f) / ScreenW) - 1.0f;
	const float ndcY = 1.0f - (2.0f * (MouseY + 0.5f) / ScreenH);

	const FMatrix invProjection = mCamera.GetInverseUnifiedProjectionMatrix(
		ScreenW / ScreenH, mCamera.mFovDegree, orthoDistance, NearZ, FarZ, perspectiveRatio
	);

	const FMatrix invViewProj = invProjection * mCamera.GetViewMatrix().AffineInverse();

	const auto Unproject = [&](float ndcZ) -> FVector
		{
			const FVector xyz = invViewProj.TransformPosition(FVector(ndcX, ndcY, ndcZ));

			const float w =
				ndcX * invViewProj.M[0][3] +
				ndcY * invViewProj.M[1][3] +
				ndcZ * invViewProj.M[2][3] +
				invViewProj.M[3][3];

			return xyz * (1.0f / w);
		};

	OutNearPoint = Unproject(0.0f);
	OutFarPoint = Unproject(1.0f);
}

void FEditorViewportClient::Reset()
{
	bMouseHit = false;
	mGizmo.Reset();
}
