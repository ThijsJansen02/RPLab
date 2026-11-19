#pragma once
#include <Engine/Engine.h>


namespace RPLab {

	struct PIDLockController {

		PH::real64 setpoint;
		PH::real64 Kp;
		PH::real64 Ti;
		PH::real64 Td;
		
		void ImGuiDraw();
		void sendToServer(PH::uint32 serversocket);
	};


}