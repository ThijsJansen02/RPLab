

#include <LabCore/LabCoreAPI.h>
#include "views/DSPview.h"


namespace RPLab {
	PH::bool32 onRenderFilterCreate(Engine::AssetLibrary* assets);
}

using namespace PH::LabCore;


extern "C" {
	_declspec(dllexport) void init(const char* plugindir) {
		LabCore::addView(RPLab::createDSPviewDescription());

		LabCore::getAssetLibrary()->addAssetType(RPLab::FilterArray::createAssetDescription());
		LabCore::addOnRenderCallback(RPLab::onRenderFilterCreate);
	}
}