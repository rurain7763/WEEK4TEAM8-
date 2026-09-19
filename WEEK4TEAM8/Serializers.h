#pragma once

#include "Core.h"
#include "FArchive.h"
#include "Vector.h"
#include "FName.h"
#include "FGuid.h"
#include "FAsset.h"
#include "FObjImporter.h"

template <>
struct FArchiveSerializer<FVector>
{
	static void Serialize(FArchive& Ar, FVector& Value)
	{
		Ar << Value.x;
		Ar << Value.y;
		Ar << Value.z;
	}
};

template <>
struct FArchiveSerializer<FVector2>
{
	static void Serialize(FArchive& Ar, FVector2& Value)
	{
		Ar << Value.X;
		Ar << Value.Y;
	}
};

template<>
struct FArchiveSerializer<FString>
{
	static void Serialize(FArchive& Ar, FString& Value)
	{
		uint64 Length = Ar.GetMode() == EArchiveMode::Write ? Value.Len() : 0;
		
		Ar << Length;

		if (Ar.GetMode() == EArchiveMode::Read)
		{
			Value.Resize(static_cast<int32>(Length));
		}

		if (Length > 0)
		{
			Ar.Serialize(Value.CStr(), Length);
		}
	}
};

template <>
struct FArchiveSerializer<FName>
{
	static void Serialize(FArchive& Ar, FName& Value)
	{
		FString NameString;

		if (Ar.GetMode() == EArchiveMode::Write)
		{
			NameString = Value.ToString();
		}

		Ar << NameString;

		if (Ar.GetMode() == EArchiveMode::Read)
		{
			Value = FName(NameString);
		}
	}
};

template <>
struct FArchiveSerializer<FGuid>
{
	static void Serialize(FArchive& Ar, FGuid& Value)
	{
		Ar << Value.A;
		Ar << Value.B;
		Ar << Value.C;
		Ar << Value.D;
	}
};

template <>
struct FArchiveSerializer<FAssetFileHeader>
{
	static void Serialize(FArchive& Ar, FAssetFileHeader& Value)
	{
		Ar << Value.Version;
		Ar << Value.AssetType;
		Ar << Value.AssetID;
	}
};

