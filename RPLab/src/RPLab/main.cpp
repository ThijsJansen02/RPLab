

#include <LabCore/LabCoreAPI.h>
#include "views/DSPview.h"


namespace PH::LabCoreSSH {



}

using namespace PH::LabCore;

extern "C" {
	_declspec(dllexport) void init(const char* plugindir) {
		LabCore::addView(RPLab::createDSPviewDescription());
	}
}