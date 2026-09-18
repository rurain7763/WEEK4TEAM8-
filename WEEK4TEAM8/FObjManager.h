#pragma once

#include "TMap.h"
#include "Assets.h"
#include "UStaticMesh.h"

class URenderer;
class FFileManager;

class FObjManager
{
public:
	static void Initialize(URenderer& InRenderer, FFileManager& InFileManager);
	static UStaticMesh* LoadObjStaticMesh(const FString& FilePath);

private:
	static URenderer* Renderer;
	static TMap<FString, UStaticMesh*> ObjStaticMeshMap;
	static FFileManager* FileManager;

};