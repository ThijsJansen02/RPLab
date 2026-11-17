#pragma once
#include <LabCore/LabCoreAPI.h>
#include <labcore-ssh/assets/Connection.h>

using namespace PH;
using namespace PH::LabCore;

namespace RPLab {

	struct DSPview {
		LabCoreSSH::Connection* redpitaya;

		Engine::String message;

		uint32 socket;
	};

	LabCore::ViewDescription createDSPviewDescription();
}