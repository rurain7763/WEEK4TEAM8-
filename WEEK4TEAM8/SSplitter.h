#pragma once
#include <d3d11.h>
#include "Vector.h"
#include "Camera.h"

struct FRect
{
    float X = 0.0f;
    float Y = 0.0f;
    float Width = 0.0f;
    float Height = 0.0f;

    FRect() = default;
    FRect(float InX, float InY, float InW, float InH)
        : X(InX), Y(InY), Width(InW), Height(InH) {
    }

    // 우측/하단 끝 좌표
    float Right() const { return X + Width; }
    float Bottom() const { return Y + Height; }

    // 마우스 좌표가 사각형 내부인지 검사
    bool Contains(const FVector2& Point) const
    {
        return (Point.X >= X && Point.X <= Right() &&
            Point.Y >= Y && Point.Y <= Bottom());
    }

    D3D11_VIEWPORT ToD3DViewport(float MinDepth = 0.0f, float MaxDepth = 1.0f) const
    {
        D3D11_VIEWPORT Viewport = {};
        Viewport.TopLeftX = X;
        Viewport.TopLeftY = Y;
        Viewport.Width = Width;
        Viewport.Height = Height;
        Viewport.MinDepth = MinDepth;
        Viewport.MaxDepth = MaxDepth;
        return Viewport;
    }
};

enum class EViewportType
{
    Top,
    Perspective,
    Front,
    Side
};
enum class ESplitterDragState
{
    None,
    Horizontal,
    Vertical,
    Cross
};

class SWindow
{
public:
    FRect Rect;

    virtual ~SWindow() = default;
    virtual void UpdateLayout(const FRect& InRect)
    {
        Rect = InRect;
    }

    virtual bool IsHover(const FVector2& Coord) const
    {
        return Rect.Contains(Coord);
    }

    virtual SWindow* FindHoveredWindow(const FVector2& Coord)
    {
        return IsHover(Coord) ? this : nullptr;
    }
};

class SSplitter : public SWindow
{
public:
    SWindow* SideLT = nullptr;
    SWindow* SideRB = nullptr;
    float SplitRatio = 0.5f;
    float MinRatio = 0.1f;
    float MaxRatio = 0.9f;

    virtual ~SSplitter() override
    {
        delete SideLT;
        delete SideRB;
    }

    virtual SWindow* FindHoveredWindow(const FVector2& Coord) override
    {
        if (!IsHover(Coord)) return nullptr;

        if (SideLT && SideLT->IsHover(Coord))
            return SideLT->FindHoveredWindow(Coord);

        if (SideRB && SideRB->IsHover(Coord))
            return SideRB->FindHoveredWindow(Coord);

        return this;
    }
};

class SSplitterH : public SSplitter
{
public:
    virtual void UpdateLayout(const FRect& InRect) override;
};

class SSplitterV : public SSplitter
{
public:
    virtual void UpdateLayout(const FRect& InRect) override;
};

class SViewportWindow : public SWindow
{
public:
    EViewportType ViewType;
    FCamera Camera;
    bool bIsOrthographic;

    D3D11_VIEWPORT GetD3DViewport() const { return Rect.ToD3DViewport(); }
    void SetupView(EViewportType InViewType, const FVector& FocusPoint = FVector(0.f, 0.f, 0.f), float Distance = 20.0f);
};