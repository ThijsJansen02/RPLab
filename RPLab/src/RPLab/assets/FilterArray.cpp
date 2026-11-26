#include "FilterArray.h"
#include <LabCore/LabCoreAPI.h>
#include <Engine/imgui/extensions.h>
#include <labcore-plt/views/PlotView.h>

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

	//filter array scene component 
	bool32 serializeFAcomponent(YAML::Emitter& out, entt::entity entity, LabCore::Scene* scene) {
		return true;
	}


	bool32 deserializeFAcomponent(YAML::Node in, entt::entity entity, LabCore::Scene* scene) {
		return true;
	}

	bool32 addFAcomponent(entt::entity entity, LabCore::Scene* scene) {
		FilterArraySceneComponent comp;
		comp.filterarray = nullptr;
		scene->registry.emplace<FilterArraySceneComponent>(entity, comp);
		return true;
	}


	real32 linethickness = 0.7;

	bool32 drawFAcomponentUI(entt::entity entity, LabCore::Scene* scene) {
		auto* FAcomp = &scene->registry.get<FilterArraySceneComponent>(entity);
		FAcomp->filterarray = ImGuiext::assetSelecter<FilterArray>("Filter", FAcomp->filterarray, "select filter", LabCore::getAssetLibrary());

		ImGui::DragFloat("line thickness", &linethickness, linethickness * 0.01);

		return true;
	}

	bool32 hasFAcomponent(entt::entity entity, LabCore::Scene* scene) {
		return scene->registry.any_of<FilterArraySceneComponent>(entity);

		return true;
	}
	
	//returns the magnitude of the filter at omega
	inline real64 filterMagnitude(real64 omega, const IIRcoefficients_2nd_Order& coeffs) {

		Base::Complex<real64> s = omega * Base::Complex<real64>::i();
		Base::Complex<real64> f = (coeffs.b[0] * s * s + coeffs.b[1] * s + coeffs.b[2]) / (coeffs.a[0] * s * s + coeffs.a[1] * s + coeffs.a[2]);

		return f.modulus();

	}

	//returns the phase in radians at omega
	inline real64 filterPhase(real64 omega, const IIRcoefficients_2nd_Order& coeffs) {

		Base::Complex<real64> s = omega * Base::Complex<real64>::i();
		Base::Complex<real64> f = (coeffs.b[0] * s * s + coeffs.b[1] * s + coeffs.b[2]) / (coeffs.a[0] * s * s + coeffs.a[1] * s + coeffs.a[2]);

		return f.arg();
	}

	//transforms x to the given domain
	static real64 transformScreenSpaceToDomain(real32 x, real32 screensize, real64 domainleft, real64 domainright) {
		real64 domainsize = domainright - domainleft;
		return domainleft + ((x / screensize) * domainsize);
	}


	void drawFilterArray(const FilterArray& filterarray, LabCore::Scene::RenderProperties* renderproperties) {

		LabCorePLT::PlotView::Settings* settings = LabCorePLT::getCurrentPlotViewSettings();
		Engine::ArrayList<IIRcoefficients_2nd_Order> filtercoeffs = Engine::ArrayList<IIRcoefficients_2nd_Order>::create(filterarray.filters.getCount());

		for (auto& filter : filterarray.filters) {
			filtercoeffs.pushBack(RPLab::getIIRcoefficients(filter));
		}

		Engine::BatchRenderer2D* renderer = LabCore::getRenderer2D();

		glm::vec2 lastsample = { 0.0, 0.0 };

		uint32 xpixel = 0;
		while (xpixel < (uint32)renderproperties->viewportsize.x) {

			Engine::Box2D domain = renderproperties->camera.domain;
			real64 x = transformScreenSpaceToDomain(xpixel, renderproperties->viewportsize.x, domain.left, domain.right);

			//if the plot settings are log scale than interpret x as a log value
			real32 x_hz = x;
			if (settings->logx) {
				x_hz = Base::powReal64(10.0f, x_hz);
			}

			real64 out = 1.0;
			for (auto& c : filtercoeffs) {
				out *= filterMagnitude(x_hz, c);
			}

			glm::vec2 newsample = { x, 20 * log10f(out) };

			//Engine::INFO << "prev: " << lastsample.x << "new: " << newsample.x << "\n";

			renderer->drawLine(lastsample, newsample, 2.0f, linethickness, glm::mat2(renderproperties->camera.worldtoscreen), glm::vec4(1.0f));

			lastsample = newsample;

			xpixel++;
		}

		Engine::ArrayList<IIRcoefficients_2nd_Order>::destroy(&filtercoeffs);
	}

	bool32 drawFAcomponent(LabCore::Scene* scene, LabCore::Scene::RenderProperties* renderproperties) {
		for (auto& ent : scene->registry.view<FilterArraySceneComponent>()) {

			auto* FAcomp = &scene->registry.get<FilterArraySceneComponent>(ent);

			if (!FAcomp->filterarray) {
				continue;
			}

			drawFilterArray(*FAcomp->filterarray, renderproperties);
		}
		return true;
	}

	LabCore::SceneComponentDescription FilterArraySceneComponent::createDescription() {
		LabCore::SceneComponentDescription desc{};
		desc.hasComponent_ = hasFAcomponent;
		desc.deserializeComponent_ = deserializeFAcomponent;
		desc.serializeComponent_ = serializeFAcomponent;
		desc.addComponent = addFAcomponent;
		desc.drawUI_ = drawFAcomponentUI;
		desc.drawComponents_ = drawFAcomponent;
		desc.name = Engine::String::create(FilterArraySceneComponent::getComponentName());
		return desc;
	}
}