
#include "DSPview.h"
#include <LabCore/LabCoreAPI.h>
#include <Engine/imgui/extensions.h>
#include <labcore-ssh/views/ConnectionView.h>

#include "RpServerConnection.h"


namespace RPLab {

	typedef DSPview View;

	bool32 openFilterCreatePopup = false;

	const char* Biquad4xFilename = "Biquad24bit4x.bit.bin";

	void drawServerConnectionUI(View* view) {
		view->redpitaya = ImGuiext::assetSelecter<LabCoreSSH::Connection>("red pitaya", view->redpitaya, "select connection", LabCore::getAssetLibrary());
		ImGui::SameLine();

		if (ImGui::Button("create")) {
			LabCoreSSH::openCreateConnectionPopup();
		}

		if (view->redpitaya) {
			if (ImGui::Button("connect to server")) {
				closeServerConnection(view->server);
				view->server = connectToServer(view->redpitaya->remoteip.getC_Str(), "");
				if (view->server != 1) {
					sendRunBitFileRequest(Biquad4xFilename, view->server);
				}
			}
			if (ImGui::TreeNodeEx("Server options")) {

				if (ImGui::Button("build server")) {
					buildServerOnRedPitaya("assets/RPserver/", view->redpitaya);
				}

				if (ImGui::Button("Run server")) {
					runServer(view->redpitaya);
				}

				if (ImGui::Button("send Server Close")) {
					sendServerClose(view->server);
				}
				ImGui::TreePop();
			}
		}
	}

#define FPGA_SAMPLESPEED 125000000

	static PH::bool32 drawView(View* view) {
		drawServerConnectionUI(view);
		
		ImGui::SeparatorText("Filter");

		view->filterarry = ImGuiext::assetSelecter("Filter", view->filterarry, "select filter", LabCore::getAssetLibrary());
		ImGui::SameLine();
		if (ImGui::Button("Create new filter")) {
			openFilterCreatePopup = true;
		}

		if (ImGui::TreeNodeEx("Filer Settings", ImGuiTreeNodeFlags_DefaultOpen)) {

			ImGui::InputInt("Decimation", &view->decimation);
			ImGui::Checkbox("Auto update", &view->autoupdate);
			ImGui::SameLine();
			if (ImGui::Button("Update Filter")) {
				sendFilterArrayToServer24Bit(view->server, *view->filterarry, FPGA_SAMPLESPEED / view->decimation);
			}

			ImGui::TreePop();
		}

		bool32 invalid = false;

		if (view->filterarry) {
			invalid |= drawFilterArrayUI(view->filterarry);
		}

		if (invalid && view->autoupdate) {
			sendFilterArrayToServer24Bit(view->server, *view->filterarry, FPGA_SAMPLESPEED / view->decimation);
		}

		return true;
	}


	PH::bool32 onRenderFilterCreate(Engine::AssetLibrary* assets) {

		if (openFilterCreatePopup) {
			ImGui::OpenPopup("Create filter");
			openFilterCreatePopup = false;
		}


		Platform::setCurrentDirectory(PH::LabCore::getCurrentProjectDir());
		ImGuiIO& io = ImGui::GetIO();

		ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(
			ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
			ImGuiCond_Appearing,
			ImVec2(0.5f, 0.5f)
		);

		if (ImGui::BeginPopupModal("Create filter", nullptr, ImGuiWindowFlags_NoResize)) {
			static Engine::String directory = Engine::String::create("assets/IIRFilters");
			static Engine::String name = Engine::String::create("filter");
			
			ImGuiext::FilePath("Directory", &directory, "", LabCore::getCurrentProjectDir().getC_Str(), PH_DIALOG_PATHMUSTEXIST);
			ImGuiext::Input("Filename", &name);


			if (ImGui::Button("create")) {
				directory.replace('\\', '/');
				Engine::String filepath = Engine::String::create(directory.getC_Str()).append("/").append(name.getC_Str());
				Base::setExtension(&filepath, FilterArray::getExtension());

				FilterArray* f = Engine::Allocator::push(createFilterArray());

				LabCore::getAssetLibrary()->addAsset(f, filepath.getC_Str(), name.getC_Str(), Engine::AssetFlagsBits::PH_ASSET_SERIALIZABLE);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		return true;
	}

	static PH::bool32 updateView(View* view) {


		return true;
	}

	static PH::bool32 instantiateView(View* view) {
		*view = View{};
		
		view->redpitaya = nullptr;
		view->server = 0;
		view->filterarry = nullptr;
		view->decimation = 1;
		return true;
	}

	static PH::bool32 destroyView(View* data) {
		closeServerConnection(data->server);
		return true;
	}

	static PH::bool32 ViewOnEvent(View* data, const PH::Platform::Event& e) {
		return true;
	}

	static PH::bool32 deserializeView(View* data, const YAML::Node& node) {
		
		instantiateView(data);

		if (node["SSHconnection"]) {
			data->redpitaya = LabCore::getAssetLibrary()->getLoadedAsset<LabCoreSSH::Connection>(node["SSHconnection"].as<Engine::AssetID>());
		}

		if (node["FilterArray"]) {
			data->filterarry = LabCore::getAssetLibrary()->getLoadedAsset<FilterArray>(node["FilterArray"].as<Engine::AssetID>());
		}

		if (node["decimation"]) {
			data->decimation = node["decimation"].as<int32>();
		}

		if (node["autoupdate"]) {
			data->autoupdate = node["autoupdate"].as<bool>();
		}
		return true;
	}

	static PH::bool32 serializeView(View* view, YAML::Emitter& out) {
		out << YAML::BeginMap;
		if (view->redpitaya) {
			out << YAML::Key << "SSHconnection" << YAML::Value << view->redpitaya->id;
		}

		if (view->filterarry) {
			out << YAML::Key << "FilterArray" << YAML::Value << view->filterarry->id;
		}
		out << YAML::Key << "decimation" << YAML::Value << view->decimation;
		out << YAML::Key << "autoupdate" << YAML::Value << view->autoupdate;
		out << YAML::EndMap;
		return true;
	}

	LabCore::ViewDescription createDSPviewDescription() {
		LabCore::ViewDescription d{};
		d.draw_ = (LabCore::ViewDrawFunction)drawView;
		d.onUpdate_ = (LabCore::ViewOnUpdateFunction)updateView;
		d.onEvent_ = (LabCore::ViewOnEventFunction)ViewOnEvent;

		d.destroy_ = (LabCore::ViewDestroyFunction)destroyView;
		d.instantiate_ = (LabCore::ViewInstantiateFunction)instantiateView;

		d.serialize_ = (LabCore::ViewSerializeFunction)serializeView;
		d.deserialize_ = (LabCore::ViewDeserializeFunction)deserializeView;

		d.windowpadding = { 5,5 };
		d.windowflags = 0;

		//d.deserialize_ = (LabCore::ViewDeserializeFunction)deserializeView;
		//d.serialize_ = (LabCore::ViewSerializeFunction)serializeView;

		d.name = Engine::String::create("Red Pitaya DSP");
		d.instancedatasize = sizeof(View);

		return d;
	}
}