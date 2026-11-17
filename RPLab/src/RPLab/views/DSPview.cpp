
#include "DSPview.h"
#include <LabCore/LabCoreAPI.h>
#include <Engine/imgui/extensions.h>
#include <labcore-ssh/views/ConnectionView.h>

uint32 connectToSocket(const char* hostname, const char* message);
void sendMessageOverSocket(uint32 connect, const char* message);
void closeSocketimpl(uint32 sock);

namespace RPLab {

	typedef DSPview View;



	static PH::bool32 drawView(View* view) {
		view->redpitaya = ImGuiext::assetSelecter<LabCoreSSH::Connection>("red pitaya", view->redpitaya, "select connection", LabCore::getAssetLibrary());
		
		ImGui::SameLine();
		if (ImGui::Button("create")) {
			LabCoreSSH::openCreateConnectionPopup();
		}

		ImGuiext::Input("server message", &view->message);
		if (ImGui::Button("send message")) {
			sendMessageOverSocket(view->socket, view->message.getC_Str());
		}
		ImGui::SameLine();
		if (ImGui::Button("connect")) {
			closeSocketimpl(view->socket);
			view->socket = connectToSocket("rp-f06d2e.local", view->message.getC_Str());
		}
		
		return true;
	}

	static PH::bool32 updateView(View* view) {


		return true;
	}

	static PH::bool32 instantiateView(View* view) {
		view->redpitaya = nullptr;
		view->message = Engine::String::create("");
		return true;
	}

	static PH::bool32 destroyView(View* data) {
		return true;
	}

	static PH::bool32 ViewOnEvent(View* data, const PH::Platform::Event& e) {
		return true;
	}

	static PH::bool32 deserializeView(View* data, const YAML::Node& node) {
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