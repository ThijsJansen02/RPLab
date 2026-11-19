#pragma once
#include <Engine/Engine.h>
#include <labcore-ssh/assets/Connection.h>

namespace RPLab {

	//creates a connection socket to a red pitaya with a running server
	PH::uint32 connectToServer(const char* hostname, const char* message);

	//builds the server on the red pitaya
	void buildServerOnRedPitaya(const char* localsourcepath, PH::LabCoreSSH::Connection* redpitaya);

	//runs the RPLab server on the red pitaya, consumes the connection
	PH::bool32 runServer(PH::LabCoreSSH::Connection* redpitaya);
	void sendServerClose(PH::uint32 serversocket);
	void closeServerConnection(PH::uint32 serversocket);
	void sendRunBitFileRequest(const char* bitfilepath, PH::uint32 server);

#pragma pack(push, 1)
	struct CoefficientWritePair {
		PH::uint32 address;
		PH::uint32 value;
	};
#pragma pack(pop)

	void sendCoefficientsToServer(PH::uint32 serversocket, PH::Base::Array<CoefficientWritePair> coefficients);
}