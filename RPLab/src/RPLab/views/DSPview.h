#pragma once
#include <LabCore/LabCoreAPI.h>
#include <labcore-ssh/assets/Connection.h>
#include "../assets/FilterArray.h"

using namespace PH;
using namespace PH::LabCore;

namespace RPLab {

	struct DSPview {
		LabCoreSSH::Connection* redpitaya;
		
		uint32 server;
		
		FilterArray* filterarry;

		bool autoupdate;
		int32 decimation;
	};

	LabCore::ViewDescription createDSPviewDescription();
}