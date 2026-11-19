
#include "DSPview.h"
#include <LabCore/LabCoreAPI.h>
#include <Engine/imgui/extensions.h>
#include <labcore-ssh/views/ConnectionView.h>

#include "RpServerConnection.h"


namespace RPLab {

	typedef DSPview View;

	static PH::bool32 drawView(View* view) {
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
					sendRunBitFileRequest("PIDlock.bit.bin", view->server);
				}
			}
			if (ImGui::BeginCombo("Server", "server settings")) {

				if (ImGui::Button("build server")) {
					buildServerOnRedPitaya("assets/RPserver/", view->redpitaya);
				}

				if (ImGui::Button("Run server")) {
					runServer(view->redpitaya);
				}

				if (ImGui::Button("send Server Close")) {
					sendServerClose(view->server);
				}
				ImGui::EndCombo();
			}

			view->pid.ImGuiDraw();
			if (ImGui::Button("update pid")) {
				view->pid.sendToServer(view->server);
			}
		}
		
		return true;
	}

	static PH::bool32 updateView(View* view) {


		return true;
	}

	static PH::bool32 instantiateView(View* view) {
		view->redpitaya = nullptr;
		view->server = 0;
		view->pid = {};
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
		closeServerConnection(data->server);
		return true;
	}

	static PH::bool32 serializeView(View* data, YAML::Emitter& out) {
		return true;
		out << YAML::Value << "DSP";
	}

	LabCore::ViewDescription createDSPviewDescription() {
		LabCore::ViewDescription d{};
		d.draw_ = (LabCore::ViewDrawFunction)drawView;
		d.onUpdate_ = (LabCore::ViewOnUpdateFunction)updateView;
		d.onEvent_ = (LabCore::ViewOnEventFunction)ViewOnEvent;

		d.destroy_ = (LabCore::ViewDestroyFunction)destroyView;
		d.instantiate_ = (LabCore::ViewInstantiateFunction)instantiateView;

		d.windowpadding = { 5,5 };
		d.windowflags = 0;

		//d.deserialize_ = (LabCore::ViewDeserializeFunction)deserializeView;
		//d.serialize_ = (LabCore::ViewSerializeFunction)serializeView;

		d.name = Engine::String::create("Red Pitaya DSP");
		d.instancedatasize = sizeof(View);

		return d;
	}
}