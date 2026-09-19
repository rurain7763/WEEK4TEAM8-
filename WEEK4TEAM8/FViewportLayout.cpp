#include "FViewportLayout.h"

bool FRect::Contains(FPoint Point) const
{
	return Point.X >= X && Point.X <= X + Width && Point.Y >= Y && Point.Y <= Y + Height;
}

bool SWindow::IsHover(FPoint Coord) const
{
	return Rect.Contains(Coord);
}

void SWindow::Arrange(const FRect& InRect)
{
	Rect = InRect;
}

SViewportWindow::SViewportWindow(EViewportType InType)
{
	Viewport.Type = InType;
}

void SViewportWindow::Arrange(const FRect& InRect)
{
	SWindow::Arrange(InRect);
	Viewport.Rect = InRect;
}

bool SSplitter::IsSplitterHover(FPoint Coord, float Thickness) const
{
	if (IsHorizontal())
	{
		const float SplitX = Rect.X + Rect.Width * SplitRatio;
		return Coord.X >= SplitX - Thickness * 0.5f && Coord.X <= SplitX + Thickness * 0.5f &&
			Coord.Y >= Rect.Y && Coord.Y <= Rect.Y + Rect.Height;
	}

	const float SplitY = Rect.Y + Rect.Height * SplitRatio;
	return Coord.X >= Rect.X && Coord.X <= Rect.X + Rect.Width &&
		Coord.Y >= SplitY - Thickness * 0.5f && Coord.Y <= SplitY + Thickness * 0.5f;
}

void SSplitter::Arrange(const FRect& InRect)
{
	SWindow::Arrange(InRect);
	if (!SideLT || !SideRB)
	{
		return;
	}

	if (IsHorizontal())
	{
		const float LeftWidth = Rect.Width * SplitRatio;
		SideLT->Arrange({ Rect.X, Rect.Y, LeftWidth, Rect.Height });
		SideRB->Arrange({ Rect.X + LeftWidth, Rect.Y, Rect.Width - LeftWidth, Rect.Height });
		return;
	}

	const float TopHeight = Rect.Height * SplitRatio;
	SideLT->Arrange({ Rect.X, Rect.Y, Rect.Width, TopHeight });
	SideRB->Arrange({ Rect.X, Rect.Y + TopHeight, Rect.Width, Rect.Height - TopHeight });
}