#pragma once

#include <string_view>
#include <filesystem>
#include "TArray.h"
#include "RenderInfo.h"

inline constexpr std::string_view kSceneDataDir = "SceneData\\";
inline constexpr std::string_view kSceneDataSuffix = ".Scene";

class FFileManager;
class AActor;
class UWorld;

class FSceneManager
{
public:
	FSceneManager();
	~FSceneManager();

	void Tick(float deltaTime);
	void Update(float deltaTime, FRenderCollector& outCollector);

	const TArray<FRenderInfo> GetAxisRenderInfos();

	// Clear world
	void NewScene();
	void DeleteScene();

	UWorld* GetCurrentWorld() const { return mCurrentWorld; }
	AActor* GetSelectedActor() const { return mSelectedActor; }
	bool IsActorSelected() const { return mSelectedActor != nullptr; }
	void SetSelectedActor(AActor* actor);
	void ResetSelectedActor() { mSelectedActor = nullptr; }

	// 파일 탐색기용 오버로드
	void SaveScene(const std::filesystem::path& scenePath, const FFileManager& fileManager);
	void LoadScene(const std::filesystem::path& scenePath, const FFileManager& fileManager);

private:
	AActor* mSelectedActor = nullptr;
	UWorld* mCurrentWorld = nullptr;

};
