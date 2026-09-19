#include "SSplitter.h"

void SSplitterH::UpdateLayout(const FRect& InRect)
{
	Rect = InRect;

	float SplitWidth = Rect.Width * SplitRatio;

	if (SideLT)
	{
		FRect LeftRect = { Rect.X, Rect.Y, SplitWidth, Rect.Height };
		SideLT->UpdateLayout(LeftRect);
	}

	if (SideRB)
	{
		FRect RightRect = { Rect.X + SplitWidth, Rect.Y, Rect.Width - SplitWidth, Rect.Height };
		SideRB->UpdateLayout(RightRect);
	}
}

void SSplitterV::UpdateLayout(const FRect& InRect)
{
	Rect = InRect;

	float SplitHeight = Rect.Height * SplitRatio;

	if (SideLT)
	{
		FRect TopRect = { Rect.X, Rect.Y, Rect.Width, SplitHeight };
		SideLT->UpdateLayout(TopRect);
	}

	if (SideRB)
	{
		FRect BottomRect = { Rect.X, Rect.Y + SplitHeight, Rect.Width, Rect.Height - SplitHeight };
		SideRB->UpdateLayout(BottomRect);
	}
}

void SViewportWindow::SetupView(EViewportType InViewType, const FVector& FocusPoint, float Distance)
{
	ViewType = InViewType;
	bIsOrthographic = (InViewType != EViewportType::Perspective);
	Camera.mOrthoDistance = 10.0f;

	switch (InViewType)
	{
	case(EViewportType::Top):
		Camera.Transform.Location = FocusPoint + FVector(0.f, 0.f, Distance);
		Camera.LookAt(FocusPoint);
		break;
	case(EViewportType::Perspective):
		Camera.Transform.Location = FocusPoint + FVector(0.f, 0.f, 0.f);
		Camera.LookAt(FocusPoint);
		break;
	case(EViewportType::Front):
		Camera.Transform.Location = FocusPoint + FVector(Distance, 0.f, 0.f);
		Camera.LookAt(FocusPoint);
		break;
	case(EViewportType::Side):
		Camera.Transform.Location = FocusPoint + FVector(0.f, Distance, 0.f);
		Camera.LookAt(FocusPoint);
		break;
	}
	
}