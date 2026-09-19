#pragma once

#include "Object.h"
#include <filesystem>

class FSceneManager;
class FGraphicsManager;
class AActor;
class UStaticMeshComponent;

struct FRect;

inline constexpr std::string_view kDefaultOBJPath = ".\\Assets\\Meshes\\";

class FObjViewer
{
public:
    void Initialize(FSceneManager& InSceneManager);
    void UpdateObjGUI(FGraphicsManager& InGraphicsManager);
    void OpenObj(const FString& filePath);

private:
    AActor* mViewerActor = nullptr;
    FSceneManager* mSceneManager = nullptr;
    FString mLoadedFilePath;
    FRect mViewportRcet;
    UStaticMeshComponent* mViewerComponent = nullptr;
};
