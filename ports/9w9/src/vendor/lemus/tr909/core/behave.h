// behave.h — 9X9: behavioural stand-ins for a voice's partitions (LEMUS_BEH_VCO), stepped in place
// of their circuits with the same inputs and outputs. AGPL-3.0 (with Lemus).
#pragma once

#include "circuit.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace tr909 {

// The VCO's timing capacitor: the partition's one part capacitor (an op-amp model's internal pole
// capacitor is cx<name>_pole)
inline double timingCap(const circuit::Netlist &net)
{
	for (const auto &e : net.elements)
		if (e.kind == 'C' && e.name.rfind("cx", 0) != 0)
			return e.value;
	return 0;
}

struct Behaviour {
	virtual ~Behaviour() = default;
	// in: the partition's inputs in its netlist's order; out: its outputs, the same
	virtual void step(const double *in, double *out, double dt) = 0;
};

// The toms' triangle VCO (comparator IC a, 4069 inverters, integrator IC b; ht.cir's VCO1): the
// integrator ramps at -y / (R C), y the inverter's output (kSwing of VDD or VSS, the rail the
// comparator's state picks); the comparator (+-14 V, hysteresis 100k/100k) turns at +-7 V, kDelay
// after the ramp crosses (its slew to the inverters' threshold), placed within the sample. Line A
// high (the trigger): the comparator held low (D73), the integrator shorted through Q53 and 10k.
// Fitted against Lemus's VCOs: within about 2 cents over a hit (docs: 9w9 README).
struct TomVco final : Behaviour {
	static constexpr double kR = 47e3, kRreset = 10e3, kSwing = 0.996, kDelay = 10e-6, kThreshold = 7.0;
	double C = 15e-9;
	int ia = -1, ivdd = -1, ivss = -1;
	double o = 0, tsw = -1; // the ramp; the switch's pending time (-1: none)
	bool hi = false;        // the comparator's output high (the inverter at VSS)

	TomVco(const circuit::Netlist &net)
	{
		C = timingCap(net);
		for (size_t i = 0; i < net.inputs.size(); ++i) {
			const std::string &n = net.inputs[i];
			if (n == "@a") ia = int(i);
			if (n == "@vdd") ivdd = int(i);
			if (n == "@vss") ivss = int(i);
		}
	}
	bool ok() const { return C > 0 && ia >= 0 && ivdd >= 0 && ivss >= 0; }
	// Line A (the trigger) through D73/R340 into the comparator's - input (100k from the ramp): it
	// holds the comparator low. Through R363 (1M) into Q53's base: Q53, its collector below its
	// emitter (reverse, gain ~6.6), pulls the ramp toward the integrator's input through R361
	// (10k), to within its Vce(sat) 0.17 V, at most 6.6 Ib.
	static constexpr double kVd = 0.6, kBetaR = 6.6, kVcesat = 0.171, kRbase = 1e6;
	void step(const double *in, double *out, double dt) override
	{
		const double a = in[ia], vdd = in[ivdd], vss = in[ivss];
		const double imax = kBetaR * std::max(0.0, a - kVd) / kRbase;
		const bool held = a - kVd > o; // D73 conducts: the - input pulled up past +7 V with line A high
		if (held) {
			hi = false;
			tsw = -1;
		}
		auto rate = [&](double v, bool h) { // the ramp's slope at v, the comparator in state h
			const double y = kSwing * (h ? vss : vdd);
			double iq = 0;
			if (imax > 0) {
				iq = (v > 0 ? std::max(0.0, v - kVcesat) : std::min(0.0, v + kVcesat)) / kRreset;
				iq = std::max(-imax, std::min(imax, iq));
			}
			return -(y / kR + iq) / C;
		};
		if (imax > 0) { // Q53 on: two Euler halves (its 10k with C is ~150 us)
			const double h = dt / 2;
			for (int q = 0; q < 2; ++q)
				o += rate(o, hi) * h;
			out[0] = o;
			return;
		}
		const double sl = rate(o, hi);
		double on = o + sl * dt;
		if (tsw < 0) {
			const double thr = hi ? kThreshold : -kThreshold;
			if ((hi && on > thr) || (!hi && on < thr))
				tsw = std::max(0.0, (thr - o) / sl) + kDelay; // from this sample's start (0: the threshold moved past the ramp)
		}
		if (tsw >= 0 && tsw < dt) {
			hi = !hi;
			on = o + sl * tsw + rate(o, hi) * (dt - tsw);
			tsw = -1;
		} else if (tsw >= 0) {
			tsw -= dt;
		}
		o = on;
		out[0] = o;
	}
};

// The snare's VCOs (sd.cir VCO-1/-2): the toms' core with the comparator the other way up (the
// ramp into its - input, the hysteresis node P on its +), and the PULSE line on P through D62 and
// 10k: high, it forces the comparator high; low (at rest about -2 V) it holds P's lower threshold
// up, from -7 V to about -4.7 V, and loads the line (the trigger partition takes P back: the two
// VCOs' coupling). P is solved each sample with the 1SS133's own equation. Q44 (its base from PULSE
// through 1M, forward here: the ramp above its emitter) holds the ramp near +0.75 V for the trigger;
// at rest, with the line below 0, its base-collector junction turns on near the ramp's bottom and
// it runs reverse (BR 6), drawing BR times its base current out of the integrator's input: about
// 1.4 uA on average over the rising ramp, +45 cents.
struct SnareVco final : Behaviour {
	static constexpr double kR = 47e3, kRreset = 10e3, kDrop = 0.0075 /* the inverters' output under 47k, as Lemus's 4069 */, kDelay = 10e-6, kVsat = 14.0;
	static constexpr double kRhyst = 100e3, kRd = 10e3 + 1.44, kIs = 1.45e-9, kNVt = 1.71 * 0.025852;
	static constexpr double kBetaF = 350, kBetaR = 6, kVcesatF = 0.124, kVcesatR = 0.171, kRbase = 1e6, kQis = 1.1e-14, kVt = 0.025852;
	double C = 10e-9;
	int ipulse = -1, ivdd = -1, ivss = -1;
	std::vector<int> oTri, oP, oQ;
	double o = 0, tsw = -1, iD = 0;
	bool hi = false;

	SnareVco(const circuit::Netlist &net)
	{
		C = timingCap(net);
		for (size_t i = 0; i < net.inputs.size(); ++i) {
			const std::string &n = net.inputs[i];
			if (n == "@pulse") ipulse = int(i);
			if (n == "@vdd") ivdd = int(i);
			if (n == "@vss") ivss = int(i);
		}
		for (size_t k = 0; k < net.outputs.size(); ++k) {
			const std::string &n = net.outputs[k].name;
			(n.rfind("@p", 0) == 0 ? oP : n.rfind("@q", 0) == 0 ? oQ : oTri).push_back(int(k));
		}
	}
	bool ok() const { return C > 0 && ipulse >= 0 && ivdd >= 0 && ivss >= 0 && !oTri.empty() && oP.size() == 1 && oQ.size() == 1; }
	// P with the comparator at cmp: (cmp - P)/100k - P/100k + i_D = 0, i_D D62's current from the line
	// through 10k (none while the line is below P). One Newton on the diode's voltage vd (P = pulse -
	// vd - i_D 10k; the residual rises with vd), from the last solution vd0.
	double node(double cmp, double pulse, double &vd0) const
	{
		const double span = pulse - cmp / 2; // vd's most: the diode carrying all of P's pull
		if (span <= 0)
			return cmp / 2;
		double vd = std::clamp(vd0, 0.0, std::min(span, 1.0));
		for (int it = 0; it < 40; ++it) {
			const double e = kIs * std::exp(vd / kNVt), i = e - kIs, di = e / kNVt;
			const double f = (cmp - 2 * (pulse - vd - i * kRd)) / kRhyst + i, df = (2 + 2 * kRd * di) / kRhyst + di;
			const double next = std::clamp(vd - f / df, 0.0, std::min(span, 1.0));
			const double st = next - vd;
			vd = std::abs(st) > 0.05 ? vd + std::copysign(0.05, st) : next;
			if (std::abs(st) < 1e-10)
				break;
		}
		vd0 = vd;
		return pulse - vd - (kIs * std::expm1(vd / kNVt)) * kRd;
	}
	// Q44's base current at rest, reverse: the base-collector junction (IS/BR) in series with 1M
	// to the line and (1 + BR) 10k to the ramp (Newton on the junction's voltage, from the last)
	double xr = 0.5;
	double reverseBase(double pulse, double v)
	{
		const double vmax = pulse - v, rs = kRbase + (1 + kBetaR) * kRreset;
		if (vmax < 0.3)
			return 0;
		double x = std::min(xr, vmax);
		for (int it = 0; it < 30; ++it) {
			const double e = kQis / kBetaR * std::exp(x / kVt), f = e - (vmax - x) / rs, df = e / kVt + 1 / rs;
			const double st = f / df;
			x -= std::clamp(st, -0.05, 0.05);
			if (std::abs(st) < 1e-9)
				break;
		}
		xr = x;
		return std::max(0.0, (vmax - x) / rs);
	}
	double vdHi = 0.4, vdLo = 0.4;
	double pHi = 0, pLo = 0, lastPulse = 1e9;
	void step(const double *in, double *out, double dt) override
	{
		const double pulse = in[ipulse], vdd = in[ivdd], vss = in[ivss];
		if (std::abs(pulse - lastPulse) > 1e-6) { // both thresholds at this PULSE
			pHi = node(kVsat, pulse, vdHi);
			pLo = node(-kVsat, pulse, vdLo);
			lastPulse = pulse;
		}
		const double ib = std::max(0.0, pulse - 0.6) / kRbase;
		auto rate = [&](double v, bool h) {
			const double y = h ? vss + kDrop : vdd - kDrop;
			double iq = 0;
			if (ib > 0) {
				iq = v > 0 ? std::min(kBetaF * ib, std::max(0.0, v - kVcesatF) / kRreset)
				           : std::max(-kBetaR * ib, std::min(0.0, v + kVcesatR) / kRreset);
			} else {
				iq = -kBetaR * reverseBase(pulse, v);
			}
			return -(y / kR + iq) / C;
		};
		// the comparator: high while P (at its state) is above the ramp
		const double thr = hi ? pHi : pLo;
		if (ib > 0) {
			if (!hi && o < thr) { hi = true; tsw = -1; } // PULSE forces it high
			const double h = dt / 2;
			for (int q = 0; q < 2; ++q)
				o += rate(o, hi) * h;
		} else {
			const double sl = rate(o, hi);
			double on = o + sl * dt;
			if (tsw < 0 && ((hi && on > thr) || (!hi && on < thr)))
				tsw = std::max(0.0, (thr - o) / sl) + kDelay; // 0: the threshold moved past the ramp
			if (tsw >= 0 && tsw < dt) {
				hi = !hi;
				on = o + sl * tsw + rate(o, hi) * (dt - tsw);
				tsw = -1;
			} else if (tsw >= 0) {
				tsw -= dt;
			}
			o = on;
		}
		for (int k : oTri) out[k] = o;
		out[oP[0]] = hi ? pHi : pLo;
		out[oQ[0]] = pulse > 0.3 ? std::min(pulse, kVt * std::log1p(ib / kQis * kBetaF / (kBetaF + 1))) : pulse - kRbase * reverseBase(pulse, o);
	}
};

// The kick's VCO (bd.cir): IC12a turns ENV3 (C9) into va = -(C9 R28/R26 + 15 R28/R27); IC12b
// passes +va or -0.993 va (Q10 shorting its + input while the comparator is low) into the
// integrator IC13a (R59, C14); the comparator IC13b (+-14 V, R49/R50: +-4.48 V) turns it, kDelay
// late. The reset: C1 pulls Q7's base (R12 from p1, R13 to +15 V) down; Q7 drives Q11 (330k) across
// C14 through R51 (2.2k) and lifts IC13b's + input through R48/D12.
struct KickVco final : Behaviour {
	static constexpr double kR26 = 330e3, kR28 = 150e3, kR59 = 47e3, kR51 = 2.2e3, kLoGain = 0.9931;
	static constexpr double kVsat = 14.0, kR49 = 100e3, kR50 = 47e3, kR48 = 22e3, kR47 = 330e3;
	static constexpr double kR12 = 100e3, kR13 = 47e3, kDelay = 10e-6, kVd = 0.6, kVt = 0.025852;
	static constexpr double kQ11Vce = 0.0125, kQ11Vbe = 0.04 /* over kVd: its release, fitted to Lemus's */, kQ11Beta = 350, kQ7Beta = 370, kQ7Is = 1.2e-14, kQ7Ise = 2e-14, kQ7Ne = 1.5, kRload = 22e3 * 330e3 / (22e3 + 330e3);
	double C = 0.033e-6, R27 = 1.5e6;
	int ic9 = -1, ip1 = -1;
	std::vector<int> oTri, oQ7b, oI12an;
	double o = 0, tsw = -1;
	bool hi = true;

	KickVco(const circuit::Netlist &net)
	{
		C = timingCap(net);
		for (const auto &e : net.elements)
			if (e.kind == 'R' && (e.name == "r27" || e.name == "R27"))
				R27 = e.value;
		for (size_t i = 0; i < net.inputs.size(); ++i) {
			if (net.inputs[i] == "@c9") ic9 = int(i);
			if (net.inputs[i] == "@p1") ip1 = int(i);
		}
		for (size_t k = 0; k < net.outputs.size(); ++k) {
			const std::string &n = net.outputs[k].name;
			(n == "@q7b" ? oQ7b : n == "@i12an" ? oI12an : oTri).push_back(int(k));
		}
	}
	bool ok() const { return C > 0 && ic9 >= 0 && ip1 >= 0 && oQ7b.size() == 1 && oI12an.size() == 1 && !oTri.empty(); }
	void step(const double *in, double *out, double dt) override
	{
		const double va = -(in[ic9] * kR28 / kR26 + 15.0 * kR28 / R27);
		// Q7: its base from the divider, clamped a Vbe under +15 V while it conducts
		const double rth = kR12 * kR13 / (kR12 + kR13), div = (in[ip1] * kR13 + 15.0 * kR12) / (kR12 + kR13);
		// Q7's base current: Ib rth = (15 - div) - Vbe (Newton on Vbe)
		double ib7 = 0, q7b = div;
		if (15.0 - div > 0.3) {
			double vbe = std::min(0.65, 15.0 - div);
			for (int it = 0; it < 60; ++it) { // Ib = Is/BF (e^(Vbe/Vt) - 1) + ISE (e^(Vbe/(NE Vt)) - 1)
				const double e = std::exp(vbe / kVt), e2 = std::exp(vbe / (kQ7Ne * kVt));
				const double f = kQ7Is / kQ7Beta * (e - 1) + kQ7Ise * (e2 - 1) - (15.0 - div - vbe) / rth;
				const double step = f / (kQ7Is / kQ7Beta * e / kVt + kQ7Ise * e2 / (kQ7Ne * kVt) + 1 / rth);
				vbe -= step;
				if (std::abs(step) < 1e-9) break;
			}
			ib7 = kQ7Is / kQ7Beta * std::expm1(vbe / kVt); // the part of it that makes collector current
			q7b = 15.0 - vbe;
		}
		// Q7's collector current (saturating at 14.95 V) into R47 (Q11's base, a Vd up) and, once
		// it is a Vd over IC13b's + input, R48/D12 into it (with R49 from IC13b's output, R50 to
		// ground): u = q7c - Vd and P worked out together
		const double ic7 = kQ7Beta * ib7, G = 1 / kR49 + 1 / kR50, uMax = 14.95 - kVd;
		auto solve = [&](bool h, double &u, double &p) {
			const double cmp = h ? kVsat : -kVsat, p0 = cmp / kR49 / G, rs = kR48 + 1 / G;
			p = p0;
			if (ic7 <= 0) { // Q7 off: its collector floats (D12, Q11's base off)
				u = std::min(0.0, p0);
				return;
			}
			u = std::min(uMax, ic7 * kR47); // D12 off
			if (u <= p0)
				return;
			// D12 on: ic7 = max(u, 0)/R47 + (u - p0)/rs, P = p0 + i/G
			u = (ic7 + p0 / rs) / (1 / kR47 + 1 / rs);
			if (u < 0)
				u = p0 + ic7 * rs;
			u = std::min(u, uMax);
			p = p0 + (u - p0) / rs / G;
		};
		double u = 0, pDummy = 0;
		solve(hi, u, pDummy);
		const double ib11 = std::max(0.0, u - kQ11Vbe) / kR47, imax11 = kQ11Beta * ib11;
		auto plus = [&](bool h) {
			double uu, p;
			solve(h, uu, p);
			return p;
		};
		auto rate = [&](double v, bool h) {
			const double vb = h ? va : -kLoGain * va;
			double iq = 0;
			if (imax11 > 0) // Q11's collector current through R51, and its base current, out of its emitter
				iq = ib11 + std::max(-imax11, std::min(imax11, (v > 0 ? std::max(0.0, v - kQ11Vce) : std::min(0.0, v + kQ11Vce)) / kR51));
			return -(vb / kR59 + iq) / C;
		};
		if (imax11 > 0) { // Q11 across C14 (2.2k x C14 ~ 75 us): Euler in quarters
			const double h = dt / 4;
			for (int q = 0; q < 4; ++q)
				o += rate(o, hi) * h;
			if (!hi && plus(hi) > o) { hi = true; tsw = -1; }
			else if (hi && plus(hi) < o) { hi = false; tsw = -1; }
		} else {
			const double thr = plus(hi), sl = rate(o, hi);
			double on = o + sl * dt;
			if (tsw < 0 && ((hi && on > thr) || (!hi && on < thr)))
				tsw = std::max(0.0, (thr - o) / sl) + kDelay; // 0: the threshold moved past the ramp
			if (tsw >= 0 && tsw < dt) {
				hi = !hi;
				on = o + sl * tsw + rate(o, hi) * (dt - tsw);
				tsw = -1;
			} else if (tsw >= 0) {
				tsw -= dt;
			}
			o = on;
		}
		for (int k : oTri) out[k] = o;
		out[oQ7b[0]] = q7b;
		out[oI12an[0]] = -va / 1e5;
	}
};

} // namespace tr909
