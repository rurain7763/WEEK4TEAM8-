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