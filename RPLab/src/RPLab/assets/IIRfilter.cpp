#include "IIRfilter.h"
#include <imgui.h>
#include <Base/Datastructures/HashMap.h>
#include <Engine/Engine.h>

namespace RPLab {

	//using namespace PH::LabCore;
	using namespace PH;

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

	//inserted little hack, if f_sample == 0.0f do not prewarp
	inline real64 prewarp(real64 f_continuous, real64 f_sample) {
		if (f_sample == 0.0f) {
			return f_continuous;
		}

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

	IIRFilter::CQG prewarpCQG(const IIRFilter::CQG& params, real64 f_sample) {
		IIRFilter::CQG cqg = params;
		cqg.cutoff = prewarp(params.cutoff, f_sample);
		return cqg;
	}

	//returns the prewarped LTI coefficients for a notch filter
	IIRcoefficients_2nd_Order getNotchCoefficients(const IIRFilter::CQG& params, real64 f_sample) {
		IIRFilter::CQG cqg = prewarpCQG(params, f_sample);

		IIRcoefficients_2nd_Order coeffs;

		real64 cutoffFrequency = cqg.cutoff;
		real64 qFactor = cqg.Qfactor;
		real64 gain = cqg.Gain;

		coeffs.b[0] = 1;
		coeffs.b[1] = 2 * cutoffFrequency / qFactor;
		coeffs.b[2] = cutoffFrequency * cutoffFrequency;
		coeffs.a[0] = 1;
		coeffs.a[1] = 2 * pow(10.0, -gain / 20.0) * cutoffFrequency / qFactor;
		coeffs.a[2] = cutoffFrequency * cutoffFrequency;

		return coeffs;
	}

	//returns the prewarped LTI coefficients for a low pass filter
	IIRcoefficients_2nd_Order getLowPassCoefficients(const IIRFilter::CQG& params, real64 f_sample) {
		IIRFilter::CQG cqg = prewarpCQG(params, f_sample);

		IIRcoefficients_2nd_Order coeffs;

		real64 cutoffFrequency = cqg.cutoff;
		real64 qFactor = cqg.Qfactor;
		real64 gain = cqg.Gain;

		coeffs.b[0] = 0;
		coeffs.b[1] = 0;
		coeffs.b[2] = cutoffFrequency * cutoffFrequency;
		coeffs.a[0] = 1;
		coeffs.a[1] = cutoffFrequency / qFactor;
		coeffs.a[2] = cutoffFrequency * cutoffFrequency;

		return coeffs;
	}

	//returns the prewarped LTI coefficients for a high pass filter
	IIRcoefficients_2nd_Order gethighPassCoefficients(const IIRFilter::CQG& params, real64 f_sample) {
		IIRFilter::CQG cqg = prewarpCQG(params, f_sample);

		IIRcoefficients_2nd_Order coeffs;

		real64 cutoffFrequency = cqg.cutoff;
		real64 qFactor = cqg.Qfactor;
		real64 gain = cqg.Gain;

		coeffs.b[0] = 1.0f;
		coeffs.b[1] = 0.0f;
		coeffs.b[2] = 0.0f;
		coeffs.a[0] = 1.0f;
		coeffs.a[1] = cutoffFrequency / qFactor;
		coeffs.a[2] = cutoffFrequency * cutoffFrequency;

		return coeffs;
	}

	//returns the prewarped LTI coefficients for a high pass filter
	IIRcoefficients_2nd_Order getAllPassCoefficients() {

		IIRcoefficients_2nd_Order coeffs;

		coeffs.b[0] = 0.0f;
		coeffs.b[1] = 0.0f;
		coeffs.b[2] = 1.0f;
		coeffs.a[0] = 0.0f;
		coeffs.a[1] = 0.0f;
		coeffs.a[2] = 1.0f;

		return coeffs;
	}

	IIRFilter::RAP prewarpRAP(const IIRFilter::RAP& params, real64 f_sample) {
		IIRFilter::RAP rap = params;
		rap.a_cutoff = prewarp(params.a_cutoff, f_sample);
		rap.cutoff = prewarp(params.cutoff, f_sample);
		return rap;
	}

	//returns the IIR coefficients for a Resonance anti resonance pair
	IIRcoefficients_2nd_Order getRAPCoefficients(const IIRFilter::RAP& params, real64 f_sample) {
		IIRFilter::RAP rap = prewarpRAP(params, f_sample);

		IIRcoefficients_2nd_Order coeffs{};

		real64 cutoffFrequency = rap.cutoff;
		real64 qFactor = rap.Qfactor;
		real64 antiQFactor = rap.a_Qfactor;
		real64 antiCutoffFrequency = rap.a_cutoff;

		coeffs.b[0] = 1.0f;
		coeffs.b[1] = cutoffFrequency / qFactor;
		coeffs.b[2] = cutoffFrequency * cutoffFrequency;
		coeffs.a[0] = 1.0f;
		coeffs.a[1] = antiCutoffFrequency / antiQFactor;
		coeffs.a[2] = antiCutoffFrequency * antiCutoffFrequency;

		return coeffs;
	}

	//returns the LTI coefficients build from a pair of complex poles and zeros
	IIRcoefficients_2nd_Order getCPZCoefficients(const IIRFilter::CPZ& params) {
		IIRcoefficients_2nd_Order coeffs{};

		bool32 use_zero = true;
		bool32 use_pole = true;

		real64 zero_r = params.zero.real;
		real64 zero_i = params.zero.imag;

		real64 pole_r = params.pole.real;
		real64 pole_i = params.pole.imag;

		if (use_zero) {
			coeffs.b[0] = 1.0f;
			coeffs.b[1] = -2 * zero_r;
			coeffs.b[2] = zero_r * zero_r + zero_i * zero_i;
		}
		else {
			coeffs.b[0] = 0.0f;
			coeffs.b[1] = 0.0f;
			coeffs.b[2] = zero_r * zero_r + zero_i * zero_i;
		}
		if (use_pole)
		{
			coeffs.a[0] = 1.0f;
			coeffs.a[1] = -2 * pole_r;
			coeffs.a[2] = pole_r * pole_r + pole_i * pole_i;
		}
		else {
			coeffs.a[0] = 0.0f;
			coeffs.a[1] = 0.0f;
			coeffs.a[2] = pole_r * pole_r + pole_i * pole_i;
		}

		return coeffs;
	}

	//place holder, just returns the coefficients
	IIRcoefficients_2nd_Order getCoeffs(const IIRcoefficients_2nd_Order& coeffs) {
		return coeffs;
	}

	IIRcoefficients_2nd_Order getPrewarpedCoefficients(const IIRFilter& filter, real64 f_sample) {
		IIRcoefficients_2nd_Order ccoeffs{};

		switch (filter.type) {
		case FilterType::NOTCH: ccoeffs = getNotchCoefficients(filter.params.cqg, f_sample); break;
		case FilterType::ALLPASS: ccoeffs = getAllPassCoefficients(); break;
		case FilterType::COMPLEX: ccoeffs = getCPZCoefficients(filter.params.cpz); break;
		case FilterType::HIGHPASS: ccoeffs = gethighPassCoefficients(filter.params.cqg, f_sample); break;
		case FilterType::LOWPASS: ccoeffs = getLowPassCoefficients(filter.params.cqg, f_sample); break;
		case FilterType::RESONANCE_ANTI_RESONANCE: ccoeffs = getRAPCoefficients(filter.params.rap, f_sample); break;
		case FilterType::COEFFICIENTS: ccoeffs = getCoeffs(filter.params.coeffs); break;
		default: ccoeffs = getAllPassCoefficients(); break;
		};

		return ccoeffs;
	}

	IIRcoefficients_2nd_Order getIIRcoefficients(const IIRFilter& filter) {
		return getPrewarpedCoefficients(filter, 0.0f);
	}

	//returns the bilinear transformed coefficients for a filter
	IIRcoefficients_2nd_Order calculateBiquadCoefficients(const IIRFilter& filter, real64 f_sample) {
		
		IIRcoefficients_2nd_Order ccoeffs = getPrewarpedCoefficients(filter, f_sample);
		IIRcoefficients_2nd_Order dcoeffs{};

		bilinearTransform((real64*)&ccoeffs, (real64*)&dcoeffs, f_sample);

		return dcoeffs;
	}

	bool32 DragReal64(const char* label, real64* value, real64 speed) {
		return ImGui::DragScalar(label, ImGuiDataType_Double, value, speed);
	}

	real64 speed_mul = 0.0005f;

	bool32 drawCQG(IIRFilter::CQG* params) {
		bool32 r = false;

		r |= DragReal64("Cutoff (Hz)", &params->cutoff, speed_mul * params->cutoff);
		r |= DragReal64("Q factor", &params->Qfactor, speed_mul * params->Qfactor);
		r |= DragReal64("Gain (dB)", &params->Gain, speed_mul * params->Gain);

		return r;
	}

	bool32 drawCQ(IIRFilter::CQG* params) {
		bool32 r = false;

		r |= DragReal64("Cutoff (Hz)", &params->cutoff, speed_mul * params->cutoff);
		r |= DragReal64("Q factor", &params->Qfactor, speed_mul * params->Qfactor);

		return r;
	}

	bool32 drawRAP(IIRFilter::RAP* params) {
		bool32 r = false;

		r |= DragReal64("Cutoff (Hz)", &params->cutoff, speed_mul * params->cutoff);
		r |= DragReal64("Q factor", &params->Qfactor, speed_mul * params->Qfactor);

		r |= DragReal64("anti Cutoff (Hz)", &params->a_cutoff, speed_mul * params->a_cutoff);
		r |= DragReal64("anti Q factor", &params->a_Qfactor, speed_mul * params->a_Qfactor);

		return r;
	}

	bool32 DragComplexReal64(const char* label, PH::Base::Complex<real64>* complex) {
		bool32 r = false;

		uint64 hash = Base::stringHash(Base::SubString(label));
		ImGui::PushItemWidth(100.0f);
		ImGui::Text(label); 
		ImGui::PushID(hash);
		ImGui::SameLine(); 
		r |= DragReal64("Real", &complex->real, complex->real * speed_mul);
		ImGui::PopID();
		ImGui::PushID(hash + 1);
		ImGui::SameLine(); 
		r |= DragReal64("Imag", &complex->imag, complex->imag * speed_mul);
		ImGui::PopID();
		ImGui::PopItemWidth();

		return r;
	}

	bool32 drawCPZ(IIRFilter::CPZ* params) {
		bool32 r = false;

		r |= DragComplexReal64("Pole", &params->pole);
		r |= DragComplexReal64("Zero", &params->zero);

		return r;
	}

	bool32 draw3coeffs(const char* label, real64* coeffs) {
		uint64 hash = Base::stringHash(Base::SubString(label));
		ImGui::PushID(hash);
		ImGui::PushItemWidth(100);

		bool32 r = false;

		ImGui::Text(label); 
		ImGui::SameLine(); r |= DragReal64("0", &coeffs[0], coeffs[0] * speed_mul);
		ImGui::SameLine(); r |= DragReal64("1", &coeffs[1], coeffs[1] * speed_mul);
		ImGui::SameLine(); r |= DragReal64("2", &coeffs[2], coeffs[2] * speed_mul);

		ImGui::PopItemWidth();
		ImGui::PopID();

		return r;
	}

	bool32 drawCoeffs(IIRcoefficients_2nd_Order* coeffs) {
		bool32 r = false;
		r |= draw3coeffs("b", coeffs->b);
		r |= draw3coeffs("a", coeffs->a);

		return r;
	}

	bool32 drawIIRFilterUI(IIRFilter* filter) {

		if (ImGui::BeginCombo("Filter Type", FilterTypeStrings[filter->type])) {
			for (uint32 i = 0; i < ARRAY_LENGTH(FilterTypeStrings); i++) {
				if (ImGui::Selectable(FilterTypeStrings[i], i == filter->type)) {
					filter->type = (FilterType)i;
				}
			}
			ImGui::EndCombo();
		}

		bool32 r = false;

		if (ImGui::TreeNodeEx("Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
			switch (filter->type) {
			case FilterType::NOTCH:			r |= drawCQG(&filter->params.cqg); break;
			case FilterType::ALLPASS:		break;
			case FilterType::COMPLEX:		r |= drawCPZ(&filter->params.cpz); break;
			case FilterType::HIGHPASS:		r |= drawCQ(&filter->params.cqg); break;
			case FilterType::LOWPASS:		r |= drawCQ(&filter->params.cqg); break;
			case FilterType::RESONANCE_ANTI_RESONANCE: r |= drawRAP(&filter->params.rap); break;
			case FilterType::COEFFICIENTS:	r |= drawCoeffs(&filter->params.coeffs); break;
			default: break;
			};
			ImGui::TreePop();
		}

		return r;
	}

	using namespace PH;

	//for now I just serialize binary, in the future it is a good idea to serialize as YAML text
	void serializeIIRFilter(const IIRFilter& filter, YAML::Emitter& out) {
		out << YAML::Binary((unsigned char*)&filter, sizeof(IIRFilter));
	}

	IIRFilter deserializeIIRFilter(YAML::Node in) {
		IIRFilter r{};
		
		try {
			YAML::Binary b = in.as<YAML::Binary>();
			Base::copyMemory((void*)b.data(), (void*)&r, Base::minVal<PH::sizeptr>(sizeof(IIRFilter), b.size()));
		}
		catch(...) {
			Engine::INFO << "failed to load IIR filer!\n";
		}

		return r;
	}

	IIRFilter createAllPassFilter() {
		IIRFilter r{};
		r.type = FilterType::ALLPASS;
		return r;
	}

}