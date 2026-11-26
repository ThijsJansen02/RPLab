

#include <LabCore/LabCoreAPI.h>
#include "views/DSPview.h"
#include <labcore-scene/assets/scene.h>


namespace RPLab {
	PH::bool32 onRenderFilterCreate(Engine::AssetLibrary* assets);
}

using namespace PH::LabCore;


extern "C" {
	_declspec(dllexport) void init(const char* plugindir) {
		LabCore::addView(RPLab::createDSPviewDescription());

		Scene::addComponentDescription(RPLab::FilterArraySceneComponent::createDescription());

		LabCore::getAssetLibrary()->addAssetType(RPLab::FilterArray::createAssetDescription());
		LabCore::addOnRenderCallback(RPLab::onRenderFilterCreate);
	}
}