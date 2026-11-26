#pragma once
#include <Base/Base.h>
#include <Base/Math/Complex.h>

#include <yaml-cpp/yaml.h>

namespace RPLab {

	struct IIRcoefficients_2nd_Order {
		union {
			struct {
				PH::real64 b[3];
				PH::real64 a[3];
			};

			struct {
				PH::real64 b0, b1, b2;
				PH::real64 a0, a1, a2;
			};
		};
	};


	enum FilterType {
		LOWPASS,
		NOTCH,
		HIGHPASS,
		ALLPASS,
		RESONANCE_ANTI_RESONANCE,
		COMPLEX,
		COEFFICIENTS
	};

	static const char* FilterTypeStrings[] = {
		"Low-Pass",
		"Notch",
		"High-Pass",
		"All-Pass",
		"Resonance Anti-Resonance",
		"Complex",
		"Coefficients"
	};

	struct IIRFilter {

		FilterType type;

		struct CQG { //cutoff, Q-factor, Gain representation
			PH::real64 cutoff;
			PH::real64 Qfactor;
			PH::real64 Gain;
		};

		struct RAP { //resonance anti resonance pair
			PH::real64 cutoff;
			PH::real64 Qfactor;
			PH::real64 a_cutoff;
			PH::real64 a_Qfactor;
		};

		struct CPZ { //Complex poles and zeros
			PH::Base::Complex<PH::real64> pole;
			PH::bool32 usepole;

			PH::Base::Complex<PH::real64> zero;
			PH::bool32 usezero;
		};
		
		struct Params {
			union {
				CQG cqg;
				RAP rap;
				CPZ cpz;

				IIRcoefficients_2nd_Order coeffs;
			};
		} params;

	};

	//returns the bilinear transformed coefficients for a filter
	IIRcoefficients_2nd_Order calculateBiquadCoefficients(const IIRFilter& filter, PH::real64 f_sample);

	//returns the unwarped continuous coefficients
	IIRcoefficients_2nd_Order getIIRcoefficients(const IIRFilter& filter);
	
	//draws the ImGui ui for the given IIR filter, returns true if the filter is invalidated
	PH::bool32 drawIIRFilterUI(IIRFilter* filter);
	void serializeIIRFilter(const IIRFilter& filter, YAML::Emitter& out);
	IIRFilter deserializeIIRFilter(YAML::Node in);
	
	IIRFilter createAllPassFilter();
}