#pragma once

#include "Core.h"
#include "FFontAtlas.h"
#include "enum.h"

class FTextBuilder
{
public:
	FTextBuilder(const TSharedPtr<FFontAtlas>& fontAtlas, float unitFactor = 1.0f)
		: mFontAtlas(fontAtlas)
		, mUnitFactor(unitFactor)
	{
	}

	inline void SetCoordinateSpace(ECoordinateSpace coordinateSpace)
	{
		mCoordinateSpace = coordinateSpace;
	}

	void CalculateSize(const std::wstring& Text, float& OutWidth, float& OutHeight)
	{
		const float LineHeight = mFontAtlas->LineHeight() * mUnitFactor;
		const float Ascender = mFontAtlas->Ascender() * mUnitFactor;
		const float Descender = mFontAtlas->Descender() * mUnitFactor;

		OutWidth = 0.0f;
		OutHeight = 0.0f;
		uint32 LineCount = 1;

		float CurrentLineWidth = 0.0f;
		for (wchar_t C : Text)
		{
			if (C == L'\n')
			{
				OutWidth = FPlatformMath::Max(OutWidth, CurrentLineWidth);
				LineCount++;
				CurrentLineWidth = 0.0f;
				continue;
			}

			if (!mFontAtlas->HasGlyph(C))
			{
				mFontAtlas->AddGlyph(C);
			}

			const FFontGlyph& Glyph = mFontAtlas->GetGlyph(C);

			CurrentLineWidth += Glyph.AdvanceX * mUnitFactor;
		}

		OutWidth = FPlatformMath::Max(OutWidth, CurrentLineWidth);
		OutHeight = (Ascender - Descender) + (LineCount - 1) * LineHeight;
	}

	template <typename Func>
	void Build(const std::wstring& Text, float Width, float Height, Func&& Callback)
	{
		const float LineHeight = mFontAtlas->LineHeight() * mUnitFactor;
		const float Ascender = mFontAtlas->Ascender() * mUnitFactor;
		const float Descender = mFontAtlas->Descender() * mUnitFactor;

		FVector2 TextLocation = FVector2(-Width * 0.5f, Height * 0.5f - Ascender);
		for (wchar_t C : Text)
		{
			if (C == L'\n')
			{
				TextLocation.X = -Width * 0.5f;
				TextLocation.Y -= LineHeight;
				continue;
			}

			if (!mFontAtlas->HasGlyph(C))
			{
				continue;
			}

			const FFontGlyph& Glyph = mFontAtlas->GetGlyph(C);

			float GlyphWidth = Glyph.Width * mUnitFactor;
			float GlyphHeight = Glyph.Height * mUnitFactor;
			float AdvanceX = Glyph.AdvanceX * mUnitFactor;
			float BearingX = Glyph.BearingX * mUnitFactor;
			float BearingY = Glyph.BearingY * mUnitFactor;

			FVector2 GlyphCenter(TextLocation.X + BearingX + GlyphWidth * 0.5f, TextLocation.Y + BearingY - GlyphHeight * 0.5f);

			FRect TextRect;
			TextRect.X = GlyphCenter.X;
			TextRect.Y = mCoordinateSpace == ECoordinateSpace::World ? GlyphCenter.Y : -GlyphCenter.Y;
			TextRect.Width = GlyphWidth;
			TextRect.Height = GlyphHeight;

			FRect SubUVRect;
			SubUVRect.X = Glyph.SubUV.x;
			SubUVRect.Y = Glyph.SubUV.y;
			SubUVRect.Width = Glyph.SubUV.z;
			SubUVRect.Height = Glyph.SubUV.w;

			Callback(TextRect, SubUVRect);

			TextLocation.X += AdvanceX;
		}
	}

private:
	ECoordinateSpace mCoordinateSpace = ECoordinateSpace::World;
	TSharedPtr<FFontAtlas> mFontAtlas;
	float mUnitFactor = 1.0f;
};