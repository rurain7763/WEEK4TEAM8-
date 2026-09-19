#pragma once

#include "Renderer.h"

class FEditorViewportClient;

enum class EViewportType
{
	Perspective,
	Top,
	Front,
	Side,
};

struct FPoint
{
	float X = 0.0f;
	float Y = 0.0f;
};

struct FRect
{
	float X = 0.0f;
	float Y = 0.0f;
	float Width = 0.0f;
	float Height = 0.0f;

	bool Contains(FPoint Point) const;
};


class SWindow
{
public:
	virtual ~SWindow() = default;

	FRect Rect;
	bool IsHover(FPoint Coord) const;
	virtual void Arrange(const FRect& InRect);
};

// leaf window만 하나씩 가지는 렌더링 상태
class FViewport
{
public:
	FRect Rect;
	EViewportType Type = EViewportType::Perspective;
	FEditorViewportClient* Client = nullptr;

	TSharedPtr<FRenderTarget2D> mSceneRenderTarget;
	TSharedPtr<FDepthStencil> mSceneDepthStencil;
};

class SViewportWindow final : public SWindow
{
public:
	explicit SViewportWindow(EViewportType InType);

	FViewport Viewport;
	void Arrange(const FRect& InRect) override;
};

class SSplitter : public SWindow
{
public:
	SWindow* SideLT = nullptr;
	SWindow* SideRB = nullptr;
	float SplitRatio = 0.5f;

	virtual bool IsHorizontal() const = 0;
	bool IsSplitterHover(FPoint Coord, float Thickness) const;
	void Arrange(const FRect& InRect) override;
};

class SSplitterH final : public SSplitter
{
public:
	bool IsHorizontal() const override { return true; }
};

class SSplitterV final : public SSplitter
{
public:
	bool IsHorizontal() const override { return false; }
};
