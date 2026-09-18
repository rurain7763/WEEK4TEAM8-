#include "Assets.h"
#include "FileManager.h"
#include "Stb/stb_image.h"
#include "FLogManager.h"
#include "Renderer.h"
#include "FFontManager.h"
#include "MathUtility.h"
#include "FObjImporter.h"
#include "Serializers.h"
#include "FAssetManager.h"

TSharedPtr<FArchive> FFileAssetSource::CreateArchive()
{
	return MakeShared<FWindowsBinReader>(FilePath);
}

FStaticMeshAsset::FStaticMeshAsset(const FGuid& InAssetID, const FName& InAssetName, URenderer& InRenderer, const FVertexSimple* InVertices, uint32 InVertexCount)
	: FAsset(InAssetID, InAssetName, EAssetType::StaticMesh)
	, VertexCount(InVertexCount)
{
	VertexBuffer = InRenderer.CreateVertexBuffer(InVertices, InVertexCount);

	for (uint32 i = 0; i < InVertexCount; ++i)
	{
		const FVertexSimple& Vertex = InVertices[i];
		BoundingBox.ExpandToInclude(FVector(Vertex.x, Vertex.y, Vertex.z));
	}
}

FStaticMeshAsset::FStaticMeshAsset(const FGuid& InAssetID, const FName& InAssetName, URenderer& InRenderer, const FVertexSimple* InVertices, uint32 InVertexCount, const uint32* InIndices, uint32 InIndexCount)
	: FAsset(InAssetID, InAssetName, EAssetType::StaticMesh)
	, VertexCount(InVertexCount)
{
	VertexBuffer = InRenderer.CreateVertexBuffer(InVertices, InVertexCount);
	
	Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer = InRenderer.CreateIndexBuffer(InIndices, InIndexCount);
	SubMeshIndexBuffers.Add(IndexBuffer);
	SubMeshIndexCounts.Add(InIndexCount);

	for (uint32 i = 0; i < InIndexCount; ++i)
	{
		const FVertexSimple& Vertex = InVertices[InIndices[i]];
		BoundingBox.ExpandToInclude(FVector(Vertex.x, Vertex.y, Vertex.z));
	}
}

FStaticMeshAsset::FStaticMeshAsset(const FGuid& InAssetID, const FName& InAssetName, URenderer& InRenderer, const FStaticMesh& InStaticMesh)
	: FAsset(InAssetID, InAssetName, EAssetType::StaticMesh)
{
	TArray<FVertexSimple> Vertices;
	for (int32 i = 0; i < InStaticMesh.Positions.Num(); ++i)
	{
		FVertexSimple& Vertex = Vertices.Emplace();

		Vertex.x = InStaticMesh.Positions[i].x;
		Vertex.y = InStaticMesh.Positions[i].y;
		Vertex.z = InStaticMesh.Positions[i].z;
		Vertex.r = 1.0f;
		Vertex.g = 1.0f;
		Vertex.b = 1.0f;
		Vertex.a = 1.0f;
		Vertex.u = InStaticMesh.TexCoords[i].X;
		Vertex.v = 1.0f - InStaticMesh.TexCoords[i].Y; // 텍스처 좌표의 Y축을 뒤집음
	}

	VertexBuffer = InRenderer.CreateVertexBuffer(Vertices.Data(), Vertices.Num());
	VertexCount = Vertices.Num();

	for (const FMeshSection& Section : InStaticMesh.MeshSections)
	{
		Microsoft::WRL::ComPtr<ID3D11Buffer> IndexBuffer = InRenderer.CreateIndexBuffer(Section.Indices.Data(), Section.Indices.Num());
		SubMeshIndexBuffers.Add(IndexBuffer);
		SubMeshIndexCounts.Add(Section.Indices.Num());

		for (uint32 i = 0; i < Section.Indices.Num(); ++i)
		{
			const FVector& Position = InStaticMesh.Positions[Section.Indices[i]];
			BoundingBox.ExpandToInclude(Position);
		}
	}
}

TSharedPtr<FAsset> FStaticMeshAssetLoader::LoadAsset(const FGuid& AssetID, const FName& AssetName, FArchive& Ar)
{
	TArray<FVector> Positions;
	TArray<FVector> Normals;
	TArray<FVector2> TexCoords;
	TArray<FMeshSection> MeshSections;

	Ar << Positions;
	Ar << Normals;
	Ar << TexCoords;
	Ar << MeshSections;

	TArray<FVertexSimple> Vertices;
	for (int32 i = 0; i < Positions.Num(); ++i)
	{
		FVertexSimple& Vertex = Vertices.Emplace();

		Vertex.x = Positions[i].x;
		Vertex.y = Positions[i].y;
		Vertex.z = Positions[i].z;
		Vertex.r = 1.0f;
		Vertex.g = 1.0f;
		Vertex.b = 1.0f;
		Vertex.a = 1.0f;
		Vertex.u = TexCoords[i].X;
		Vertex.v = 1.0f - TexCoords[i].Y; // 텍스처 좌표의 Y축을 뒤집음
	}

	FStaticMesh StaticMesh;
	StaticMesh.Positions = Positions;
	StaticMesh.Normals = Normals;
	StaticMesh.TexCoords = TexCoords;
	StaticMesh.MeshSections = MeshSections;
	
	return MakeShared<FStaticMeshAsset>(AssetID, AssetName, Renderer, StaticMesh);
}

void FStaticMeshAssetLoader::UnloadAsset(TSharedPtr<FAsset> Asset)
{
	// NOTE: Nothing to do for now
}

TSharedPtr<FAsset> FTexture2DAssetLoader::LoadAsset(const FGuid& AssetID, const FName& AssetName, FArchive& Ar)
{
	int32 Width, Height, Channels;
	TArray<uint8> ImageData;

	Ar << Width;
	Ar << Height;
	Ar << Channels;
	Ar << ImageData;

	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = Width;
	TextureDesc.Height = Height;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = 0;
	TextureDesc.MipLevels = 1;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture = Renderer.CreateTexture2D(TextureDesc, ImageData.Data());
	
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = TextureDesc.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV = Renderer.CreateShaderResourceView(Texture, &SRVDesc);

	return MakeShared<FTexture2DAsset>(AssetID, AssetName, Texture, SRV);
}

void FTexture2DAssetLoader::UnloadAsset(TSharedPtr<FAsset> Asset)
{
	// NOTE: Nothing to do for now
}

TSharedPtr<FAsset> FFontAssetLoader::LoadAsset(const FGuid& AssetID, const FName& AssetName, FArchive& Ar)
{
	TArray<int8> FileData;
	if (!TryReadToBytes(Ar, FileData))
	{
		UE_LOG_ERROR("Failed to read font asset: %s", AssetName.ToString().CStr());
		return nullptr;
	}
	
	FT_Library Library = FontManager.GetLibrary();

	FT_Face Face;
	FT_Error Err = FT_New_Memory_Face(Library, reinterpret_cast<const FT_Byte*>(FileData.Data()), FileData.Num(), 0, &Face);
	if (Err)
	{
		UE_LOG_ERROR("Failed to load font asset: %s", AssetName.ToString().CStr());
		return nullptr;
	}

	const FT_UInt DefaultSize = 64; // 기본 폰트 크기 설정
	if (FT_Set_Pixel_Sizes(Face, 0, DefaultSize))
	{
		UE_LOG_ERROR("Failed to set font size for asset: %s", AssetName.ToString().CStr());
		FT_Done_Face(Face);
		return nullptr;
	}

	return MakeShared<FFontAsset>(AssetID, AssetName, Face, std::move(FileData));
}

void FFontAssetLoader::UnloadAsset(TSharedPtr<FAsset> Asset)
{
	// Nothing to do for now
}

void FFontAtlasAsset::UpdateRegion(uint32 Left, uint32 Top, uint32 Right, uint32 Bottom, const void* Data, uint32 RowPitch)
{
	if (!Texture || !Data)
	{
		return;
	}

	if (Right <= Left || Bottom <= Top)
	{
		return;
	}

	D3D11_BOX DestBox = {};
	DestBox.left = Left;
	DestBox.top = Top;
	DestBox.right = Right;
	DestBox.bottom = Bottom;
	DestBox.front = 0;
	DestBox.back = 1;

	Renderer.GetDeviceContext()->UpdateSubresource(Texture.Get(), 0, &DestBox, Data, RowPitch, 0);
}

FFontAtlasAsset::FFontAtlasAsset(const FGuid& InAssetID, const FName& InAssetName, URenderer& InRenderer, TSharedPtr<FFontAsset>& InFontAsset, uint32 InWidth, uint32 InHeight, uint32 InPaddingW, uint32 InPaddingH)
	: FTexture2DAsset(InAssetID, InAssetName, EAssetType::FontAtlas, nullptr, nullptr)
	, Renderer(InRenderer)
	, FontAsset(InFontAsset)
	, FontAtlas(MakeShared<FFontAtlas>(InFontAsset->GetFace(), InWidth, InHeight, InPaddingW, InPaddingH))
{
	D3D11_TEXTURE2D_DESC TextureDesc = {};
	TextureDesc.Width = InWidth;
	TextureDesc.Height = InHeight;
	TextureDesc.MipLevels = 1;
	TextureDesc.ArraySize = 1;
	TextureDesc.Format = DXGI_FORMAT_R8_UNORM;
	TextureDesc.SampleDesc.Count = 1;
	TextureDesc.Usage = D3D11_USAGE_DEFAULT;
	TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	TextureDesc.CPUAccessFlags = 0;
	TextureDesc.MiscFlags = 0;

	Texture = Renderer.CreateTexture2D(TextureDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = TextureDesc.Format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	SRVDesc.Texture2D.MipLevels = 1;

	SRV = Renderer.CreateShaderResourceView(Texture, &SRVDesc);

	Width = InWidth;
	Height = InHeight;
	Format = TextureDesc.Format;

	FontAtlas->SetAtlasHandler(*this);
}

bool FFontAtlasAsset::HandleAddGlyph(FFontAtlas& FontAtlas, const FFontGlyph& InGlyph, const FFontGlyphBitmap& InBitmap)
{
	UpdateRegion(InBitmap.Left, InBitmap.Top, InBitmap.Right, InBitmap.Bottom, InBitmap.Buffer, static_cast<uint32>(InBitmap.Pitch));

	return true;
}

FSpriteAtlasAsset::FSpriteAtlasAsset(const FGuid& InAssetID, const FName& InAssetName, URenderer& InRenderer, const TSharedPtr<FTexture2DAsset>& InSource, uint32 InCols, uint32 InRows, uint32 InFrameCount)
	: FTexture2DAsset(InAssetID, InAssetName, EAssetType::SpriteAtlas,
		InSource ? InSource->GetTexture() : Microsoft::WRL::ComPtr<ID3D11Texture2D>(),
		InSource ? InSource->GetSRV() : Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>())
	, Renderer(InRenderer)
{
	if (!InSource)
	{
		UE_LOG_ERROR("Sprite atlas '%s' has no source texture", InAssetName.ToString().CStr());
		return;
	}

	if (InCols == 0 || InRows == 0)
	{
		UE_LOG_ERROR("Sprite atlas '%s' has zero columns or rows", InAssetName.ToString().CStr());
		return;
	}

	const uint32 CellCount = InCols * InRows;
	const uint32 FrameCount = (InFrameCount == 0) ? CellCount : FPlatformMath::Min(InFrameCount, CellCount);

	const float FrameW = 1.0f / static_cast<float>(InCols);
	const float FrameH = 1.0f / static_cast<float>(InRows);

	FrameSubUVs.Reserve(FrameCount);
	for (uint32 i = 0; i < FrameCount; ++i)
	{
		const uint32 Col = i % InCols;
		const uint32 Row = i / InCols;

		FrameSubUVs.Add(FVector4(Col * FrameW, Row * FrameH, FrameW, FrameH));
	}
}

FSpriteAtlasAsset::FSpriteAtlasAsset(const FGuid& InAssetID, const FName& InAssetName, URenderer& InRenderer, const TSharedPtr<FTexture2DAsset>& InSource, const TArray<FVector4>& InFrameSubUVs)
	: FTexture2DAsset(InAssetID, InAssetName, EAssetType::SpriteAtlas,
		InSource ? InSource->GetTexture() : Microsoft::WRL::ComPtr<ID3D11Texture2D>(),
		InSource ? InSource->GetSRV() : Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>())
	, Renderer(InRenderer)
	, FrameSubUVs(InFrameSubUVs)
{
}

const FVector4& FSpriteAtlasAsset::GetFrameSubUV(int32 FrameIndex) const
{
	static const FVector4 WholeTexture(0.f, 0.f, 1.f, 1.f);

	if (FrameSubUVs.IsEmpty())
	{
		return WholeTexture;
	}

	if (FrameIndex < 0 || FrameIndex >= FrameSubUVs.Num())
	{
		return FrameSubUVs[0];
	}

	return FrameSubUVs[static_cast<uint32>(FrameIndex)];
}

TSharedPtr<FTexture2DAsset> FMaterialAsset::GetDiffuseTexture() const
{
	return FAssetManager::Get().GetAssetAs<FTexture2DAsset>(DiffuseTexture, true);
}

TSharedPtr<FTexture2DAsset> FMaterialAsset::GetSpecularTexture() const
{
	return FAssetManager::Get().GetAssetAs<FTexture2DAsset>(SpecularTexture, true);
}

TSharedPtr<FTexture2DAsset> FMaterialAsset::GetNormalTexture() const
{
	return FAssetManager::Get().GetAssetAs<FTexture2DAsset>(NormalTexture, true);
}

TSharedPtr<FAsset> FMaterialAssetLoader::LoadAsset(const FGuid& AssetID, const FName& AssetName, FArchive& Ar)
{
	FVector AmbientColor, DiffuseColor, SpecularColor;
	FGuid DiffuseTexture, SpecularTexture, NormalTexture;

	Ar << AmbientColor;
	Ar << DiffuseColor;
	Ar << SpecularColor;
	Ar << DiffuseTexture;
	Ar << SpecularTexture;
	Ar << NormalTexture;

	return MakeShared<FMaterialAsset>(AssetID, AssetName, AmbientColor, DiffuseColor, SpecularColor, DiffuseTexture, SpecularTexture, NormalTexture);
}

void FMaterialAssetLoader::UnloadAsset(TSharedPtr<FAsset> Asset)
{
	// NOTE: Nothing to do for now
}
