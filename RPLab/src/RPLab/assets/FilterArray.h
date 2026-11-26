#pragma once

#include <Engine/Engine.h>
#include <Engine/AssetLibrary.h>
#include "IIRfilter.h"
#include <labcore-scene/assets/scene.h>

#define RP_ASSET_FILTERARRAY 29865986986789675

namespace RPLab {

	struct FilterArray : public PH::Engine::AssetBase {
		static PH::Engine::AssetDescriptionID getAssetDescriptionID() {
			return RP_ASSET_FILTERARRAY;
		}

		static const char* getExtension() {
			return ".rpfilterarray";
		}

		PH::Engine::ArrayList<IIRFilter> filters;

		static PH::Engine::AssetDescription createAssetDescription();
	};

	FilterArray createFilterArray();

	//draws the ImGui ui for the filter array, returns true if the filter is invalidated
	PH::bool32 drawFilterArrayUI(FilterArray* filterarray);

	//scene component that draws a representation of the filter
	struct FilterArraySceneComponent {
		static const char* getComponentName() {
			return "IIR filter array";
		}

		static PH::LabCore::SceneComponentDescription createDescription();

		FilterArray* filterarray;
	};
}