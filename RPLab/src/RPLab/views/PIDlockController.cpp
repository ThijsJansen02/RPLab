#include <Engine/Engine.h>

#include "PIDLockController.h"
#include <Base/Datastructures/Stream.h>
#include "RpServerConnection.h"

using namespace PH;

namespace RPLab {

	struct BiQuadCoefficients {
		union {
			struct {
				real64 b[3];
				real64 a[3];
			};

			struct {
				real64 b0, b1, b2;
				real64 a0, a1, a2;
			};
		};
	};

	inline real64 prewarp(real64 f_continuous, real64 f_sample) {
		return 2 * f_sample * atan(f_continuous / (2 * f_sample));
	}

	inline void bilinearTransform(real64 acoefs[], real64 dcoefs[], real64 f_sample)
	{
		double b0, b1, b2, a0, a1, a2;
		double bz0, bz1, bz2, az0, az1, az2;

		b0 = acoefs[0]; b1 = acoefs[1]; b2 = acoefs[2];
		a0 = acoefs[3]; a1 = acoefs[4]; a2 = acoefs[5];

		real64 T = 1 / f_sample;
		real64 K = 2 / T;
		real64 Ks = K * K;

		az0 = a0 * Ks + a1 * K + a2;

		bz0 = ((b0 * Ks) + (b1 * K) + b2) / az0;
		bz1 = (2 * b2 - (2 * b0 * Ks)) / az0;
		bz2 = ((b0 * Ks) - (b1 * K) + b2) / az0;

		az1 = ((2 * a2) - (2 * a0 * Ks)) / az0;
		az2 = ((a0 * Ks) - (a1 * K) + a2) / az0;

		az0 = 1.0;

		dcoefs[0] = bz0; dcoefs[1] = bz1; dcoefs[2] = bz2;
		dcoefs[3] = az0; dcoefs[4] = az1; dcoefs[5] = az2;
	}

	inline PH::int16 convertToFixedPoint(real64 in) {
		uint64 mult = 1 << 14;
		real64 result = in * mult;
		return (PH::int16)(result > 0.0f ? result + 0.5 : result - 0.5);
	}

	inline PH::int32 convertToFixedPoint32(real64 in) {
		uint64 mult = 1 << 30;
		real64 result = in * mult;
		return (PH::int32)(result > 0.0f ? result + 0.5 : result - 0.5);
	}

	inline PH::int32 convertToFixedPoint24(real64 in) {
		uint64 mult = 1 << 22;
		real64 result = in * mult;
		return (PH::int32)(result > 0.0f ? result + 0.5 : result - 0.5);
	}

	void PIDLockController::ImGuiDraw() {
		ImGui::DragScalar("Setpoint (V)", ImGuiDataType_Double, &setpoint, 0.001);
		ImGui::DragScalar("P", ImGuiDataType_Double, &Kp, 0.001);
		ImGui::DragScalar("Ti (ms)", ImGuiDataType_Double, &Ti, 0.001);
		ImGui::DragScalar("Td (ms)", ImGuiDataType_Double, &Td, 0.001);
	}

	void PIDLockController::sendToServer(PH::uint32 serversocket) {
		BiQuadCoefficients coeffs{};
		coeffs.b0 = Td;
		coeffs.b1 = Kp;
		coeffs.b2 = 1.0f/Ti;

		coeffs.a0 = 0.0f;
		coeffs.a1 = 1.0f;
		coeffs.a2 = 0.0f;

		BiQuadCoefficients dcoeffs{};

		bilinearTransform((real64*)&coeffs, (real64*)&dcoeffs, 125000000);

		real64 setpoint = this->setpoint;
		uint64 mult = 1 << 13;
		setpoint *= mult;
		int32 sp = (int32)setpoint;


		Engine::ArrayList<CoefficientWritePair> writepairs = Engine::ArrayList<CoefficientWritePair>::create(1);
		writepairs.pushBack({ (uint32)0, (uint32)convertToFixedPoint32(dcoeffs.b0) });
		writepairs.pushBack({ 1, (uint32)convertToFixedPoint32(dcoeffs.b1) });
		writepairs.pushBack({ 2, (uint32)convertToFixedPoint32(dcoeffs.b2) });
		writepairs.pushBack({ 3, (uint32)convertToFixedPoint32(dcoeffs.a1) });
		writepairs.pushBack({ 4, (uint32)convertToFixedPoint32(dcoeffs.a2) });
		writepairs.pushBack({ 5, (uint32)sp });

		sendCoefficientsToServer(serversocket, writepairs.getArray());

		Engine::ArrayList<CoefficientWritePair>::destroy(&writepairs);
	}

}