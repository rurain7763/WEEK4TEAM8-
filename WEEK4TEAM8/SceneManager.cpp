#include "SceneManager.h"

#include <format>

#include "FileManager.h"
#include "EngineStatics.h"
#include "JsonUtil.h"
#include "ObjectFactory.h"
#include "World.h"
#include "FLogManager.h"

FSceneManager::FSceneManager()
{
}

FSceneManager::~FSceneManager()
{
	delete mCurrentWorld;
}

void FSceneManager::Tick(float deltaTime)
{
	mCurrentWorld->Tick(deltaTime);
}

void FSceneManager::Update(float deltaTime, FRenderCollector& outCollector)
{
	// Todo: Save / Load
	{

	}

	mCurrentWorld->Update(deltaTime, outCollector);
}

void FSceneManager::NewScene()
{
	if (mCurrentWorld != nullptr)
	{
		delete mCurrentWorld;
	}

	//UEngineStatics::SetNextUUID(0);
	ResetSelectedActor();
	mCurrentWorld = FObjectFactory::ConstructObject<UWorld>();
}

void FSceneManager::DeleteScene()
{
	if (mCurrentWorld != nullptr)
	{
		delete mCurrentWorld;
		mCurrentWorld = nullptr;
	}
	ResetSelectedActor();
}

void FSceneManager::SaveScene(
	const std::filesystem::path& scenePath,
	const FFileManager& fileManager)
{
	if (mCurrentWorld == nullptr)
	{
		throw std::runtime_error(
			"Cannot save scene because current world is null.");
	}

	uint32 version = 0;

	// 기존 파일이 있으면 Version을 유지한다.
	try
	{
		const FString previousSceneString =
			fileManager.ReadFileToString(scenePath);

		const json::JSON previousSceneJson =
			json::JSON::Load(previousSceneString);

		if (previousSceneJson.hasKey("Version") &&
			previousSceneJson.at("Version").JSONType() ==
			json::JSON::Class::Integral)
		{
			version =
				previousSceneJson.at("Version").ToInt();
		}
	}
	catch (const std::exception&)
	{
		// 새로 저장하는 파일이면 Version 0부터 시작한다.
		version = 0;
	}

	json::JSON sceneJson =
		json::JSON::Make(json::JSON::Class::Object);

	json::JSON worldJson =
		json::JSON::Make(json::JSON::Class::Object);

	mCurrentWorld->SerializeClass(worldJson);

	sceneJson["Version"] = version;
	sceneJson["NextUUID"] = UEngineStatics::GetNextUUID();
	sceneJson["World"] = worldJson;

	const FString jsonString(
		sceneJson.dump(1, "  "));

	fileManager.WriteStringToFile(
		scenePath,
		jsonString);
}

void FSceneManager::LoadScene(
	const std::filesystem::path& scenePath,
	const FFileManager& fileManager)
{
	const FString jsonString =
		fileManager.ReadFileToString(scenePath);

	const json::JSON sceneJson =
		json::JSON::Load(jsonString);

	if (!sceneJson.hasKey("NextUUID") ||
		sceneJson.at("NextUUID").JSONType() !=
		json::JSON::Class::Integral)
	{
		throw std::runtime_error(
			std::format(
				"Scene file '{}' does not contain valid NextUUID data.",
				scenePath.string()));
	}

	if (!sceneJson.hasKey("World") ||
		sceneJson.at("World").JSONType() !=
		json::JSON::Class::Object)
	{
		throw std::runtime_error(
			std::format(
				"Scene file '{}' does not contain valid World data.",
				scenePath.string()));
	}

	const uint32 nextUUID =
		sceneJson.at("NextUUID").ToInt();

	const json::JSON worldJson =
		sceneJson.at("World");

	UWorld* newWorld =
		FObjectFactory::LoadObject<UWorld>(worldJson);

	if (newWorld == nullptr)
	{
		throw std::runtime_error(
			std::format(
				"Failed to deserialize world from '{}'.",
				scenePath.string()));
	}

	// 새 월드 생성이 성공한 경우에만 기존 월드를 교체한다.
	delete mCurrentWorld;
	mCurrentWorld = newWorld;

	UEngineStatics::SetNextUUID(nextUUID);
	ResetSelectedActor();
}

void  FSceneManager::SetSelectedActor(AActor* actor)
{
	if (actor == nullptr)
	{
		UE_LOG_WARN("SetSelectedActor: Attempted to set selected actor to nullptr.");
		return;
	}

	if (actor == mSelectedActor)
	{
		UE_LOG_WARN("SetSelectedActor: Actor with UUID %d is already selected.", actor->UUID);
		return; // No change
	}

	UE_LOG_WARN("SetSelectedActor: Actor with UUID %d is now selected.", actor->UUID);
	mSelectedActor = actor;
}

const TArray<FRenderInfo> FSceneManager::GetAxisRenderInfos()
{
	// TODO: Implement axis render info retrieval logic
	return TArray<FRenderInfo>();
}


//
//FSceneData FSceneManager::ReadSceneData(
//	std::string_view sceneName,
//	const FFileManager& fileManager)
//{
//	FString fileName = sceneName;
//	fileName += kSceneDataSuffix;
//
//	json::JSON jsonData = json::JSON::Load(fileManager.ReadFileToString(fileName));
//	FSceneData sceneData = FSceneData(jsonData);
//	return sceneData;
//}
//
//UWorld* FSceneManager::BuildWorldFromSceneData(const FSceneData& sceneData)
//{
//	//UWorld* newWorld = FObjectFactory::ConstructObject<UWorld>();
//
//	//for (const auto& [UUID, primitiveData] : sceneData.Primitives) 
//	//{
//	//	// TODO: Replace AActor creation logic later
//	//	AActor* newActor = FObjectFactory::ConstructObject<AActor>();
//	//	UPrimitiveComponent* newPrimitiveComponent =
//	//		FObjectFactory::ConstructObject<UPrimitiveComponent>(
//	//			);
//	//}
//
//	//UEngineStatics::SetNextUUID(sceneData.NextUUID);
//	throw std::logic_error("BuildWorldFromSceneData is not implemented yet.");
//	return nullptr;
//}
