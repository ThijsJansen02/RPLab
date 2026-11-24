#include "FilterArray.h"

namespace RPLab {

	using namespace PH;

	static void serialize(YAML::Emitter& out, FilterArray* asset) {
		out << YAML::BeginMap;
		out << YAML::Key << "IIRFilters" << YAML::Value << YAML::BeginSeq;

		for (const auto& f : asset->filters) {
			out << YAML::Value;
			serializeIIRFilter(f, out);
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;
	}

	static void deserialize(YAML::Node node, FilterArray* asset) {
		
		asset->filters = Engine::ArrayList<IIRFilter>::create(1);
		if (node["IIRFilters"]) {
			for (auto n : node["IIRFilters"]) {
				asset->filters.pushBack(deserializeIIRFilter(n));
			}
		}
	}

	static void destroy(FilterArray* asset) {
		Engine::ArrayList<IIRFilter>::destroy(&asset->filters);
	}

	FilterArray createFilterArray() {
		FilterArray r;
		r.filters = Engine::ArrayList<IIRFilter>::create(1);

		IIRFilter f{};
		f.type = FilterType::ALLPASS;
		r.filters.pushBack(f);

		return r;
	}

	PH::Engine::AssetDescription FilterArray::createAssetDescription() {
		Engine::AssetDescription d{};
		d.destroy_ = nullptr;
		d.id = FilterArray::getAssetDescriptionID();
		d.extension = FilterArray::getExtension();
		d.localdatasize = sizeof(FilterArray);
		d.serialize_ = (Engine::SerializeAsset)RPLab::serialize;
		d.deserialize_ = (Engine::deserializeAsset)RPLab::deserialize;

		return d;
	}

	bool32 drawFilterArrayUI(FilterArray* filterarray) {
		uint32 i = 0;

		bool32 r = false;

		for (auto& f : filterarray->filters) {
			ImGui::PushID(i);

			char buffer[64];
			sprintf_s(buffer, 64, "filter %d", i);
			ImGui::SeparatorText(buffer);
			i++;
			r |= drawIIRFilterUI(&f);

			ImGui::PopID();
		}

		if (ImGui::Button("add filter")) {
			filterarray->filters.pushBack(createAllPassFilter());
		}

		return r;
	}

}