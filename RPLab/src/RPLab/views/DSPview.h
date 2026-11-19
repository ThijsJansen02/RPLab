#pragma once
#include <LabCore/LabCoreAPI.h>
#include <labcore-ssh/assets/Connection.h>
#include "PIDLockController.h"

using namespace PH;
using namespace PH::LabCore;

namespace RPLab {

	struct DSPview {
		LabCoreSSH::Connection* redpitaya;
		
		uint32 server;
		PIDLockController pid;
		
	};

	LabCore::ViewDescription createDSPviewDescription();
}