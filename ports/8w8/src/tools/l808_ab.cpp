/*
 * l808_ab — Lemus's TR-808 voicing board at its own rates (the reference: the board at 2x the host
 * rate, the metal group at 4x) against cheaper rate settings, voice by voice and on a beat.
 *
 *   l808_ab OUTDIR [RATE]
 *
 * Each case is rendered in both settings: its WAVs (OUTDIR/<case>-ref.wav, -<cfg>.wav), the CPU
 * time, and against the reference the level in 1/3-octave bands (the largest difference over bands
 * within 50 dB of the loudest, which ignores the free-running squares' and noise's phase) and the
 * envelope (10 ms frames within 50 dB of the peak). Settings are the board's own environment
 * switches, read at prepare(): TR808_FACTOR (board oversampling) and TR808_GROUP (the metal
 * group's).
 *
 * AGPL-3.0.
 */
#include "tr808/core/voiceboard.h"

#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace tr808;

namespace {

struct Case {
	const char *name;
	double seconds;
	std::vector<std::pair<double, uint16_t>> hits; // time, data
};

struct Cfg {
	const char *name;
	const char *factor, *group; // nullptr: the board's default
};

void wav(const std::string &path, const std::vector<float> &d, int sr)
{
	FILE *f = std::fopen(path.c_str(), "wb");
	if (!f)
		return;
	auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
	auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
	const uint32_t b = uint32_t(d.size() * 2);
	std::fwrite("RIFF", 1, 4, f); u32(36 + b); std::fwrite("WAVEfmt ", 1, 8, f); u32(16); u16(1); u16(1);
	u32(uint32_t(sr)); u32(uint32_t(sr) * 2); u16(2); u16(16); std::fwrite("data", 1, 4, f); u32(b);
	for (float x : d) {
		const double v = std::max(-1.0, std::min(1.0, double(x) * 4.0)); // +12 dB: the jack is volts / 10
		const int16_t s = int16_t(std::lround(v * 32767));
		std::fwrite(&s, 2, 1, f);
	}
	std::fclose(f);
}

void setenv2(const char *k, const char *v)
{
	if (v)
		setenv(k, v, 1);
	else
		unsetenv(k);
}

std::vector<float> render(const Case &c, const Cfg &g, int sr, double &cpu)
{
	setenv2("TR808_FACTOR", g.factor);
	setenv2("TR808_GROUP", g.group);
	VoiceBoard b;
	if (sr != 48000)
		b.prepare(sr);
	b.setCalibration(unitCalibration());
	b.reset();
	const int n = int(c.seconds * sr), blk = 256;
	std::vector<float> out(static_cast<size_t>(n)), mix(blk);
	std::vector<std::vector<float>> jk(kVoices, std::vector<float>(blk));
	float *jp[kVoices];
	for (int v = 0; v < kVoices; ++v)
		jp[v] = jk[size_t(v)].data();
	size_t h = 0;
	const auto t0 = std::chrono::steady_clock::now();
	for (int i = 0; i < n; i += blk) {
		const int k = std::min(blk, n - i);
		uint16_t data = 0;
		bool pulse = false;
		while (h < c.hits.size() && c.hits[h].first * sr < i + k) {
			data |= c.hits[h].second;
			pulse = true;
			++h;
		}
		if (pulse)
			b.setLines(true, data);
		b.render(mix.data(), k, jp);
		if (pulse)
			b.setLines(false, data);
		for (int j = 0; j < k; ++j) {
			float s = 0;
			for (int v = 0; v < kVoices; ++v)
				s += jk[size_t(v)][size_t(j)];
			out[size_t(i + j)] = s;
		}
	}
	cpu = 100.0 * std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() / c.seconds;
	return out;
}

// 1/3-octave band powers (dB) from 25 Hz up to the Nyquist, by a plain DFT over each band's bins of
// the whole signal's spectrum (zero-padded FFT).
std::vector<double> bands(const std::vector<float> &x, int sr, std::vector<double> &centres)
{
	size_t n = 1;
	while (n < x.size())
		n <<= 1;
	std::vector<std::complex<double>> a(n);
	for (size_t i = 0; i < x.size(); ++i)
		a[i] = x[i];
	for (size_t i = 1, j = 0; i < n; ++i) { // FFT
		size_t bit = n >> 1;
		for (; j & bit; bit >>= 1)
			j ^= bit;
		j ^= bit;
		if (i < j)
			std::swap(a[i], a[j]);
	}
	for (size_t len = 2; len <= n; len <<= 1) {
		const std::complex<double> w = std::polar(1.0, -2 * M_PI / double(len));
		for (size_t i = 0; i < n; i += len) {
			std::complex<double> wn = 1;
			for (size_t j = 0; j < len / 2; ++j) {
				const auto u = a[i + j], v = a[i + j + len / 2] * wn;
				a[i + j] = u + v;
				a[i + j + len / 2] = u - v;
				wn *= w;
			}
		}
	}
	std::vector<double> out;
	centres.clear();
	for (double fc = 25; fc * std::pow(2.0, 1.0 / 6) < sr / 2.0; fc *= std::pow(2.0, 1.0 / 3)) {
		const double lo = fc / std::pow(2.0, 1.0 / 6), hi = fc * std::pow(2.0, 1.0 / 6);
		double p = 1e-30;
		for (size_t k = size_t(lo * n / sr); k <= size_t(hi * n / sr) && k < n / 2; ++k)
			p += std::norm(a[k]);
		out.push_back(10 * std::log10(p));
		centres.push_back(fc);
	}
	return out;
}

std::vector<double> envelope(const std::vector<float> &x, int sr)
{
	const int fr = sr / 100;
	std::vector<double> e;
	for (size_t i = 0; i + size_t(fr) <= x.size(); i += size_t(fr)) {
		double s = 1e-30;
		for (int j = 0; j < fr; ++j)
			s += double(x[i + size_t(j)]) * x[i + size_t(j)];
		e.push_back(10 * std::log10(s / fr));
	}
	return e;
}

} // namespace

int main(int argc, char **argv)
{
	if (argc < 2) {
		std::fprintf(stderr, "usage: l808_ab OUTDIR [RATE]\n");
		return 1;
	}
	const std::string dir = argv[1];
	const int sr = argc > 2 ? std::atoi(argv[2]) : 48000;

	std::vector<Case> cases;
	const struct { const char *n; uint16_t bit; double s; } voices[] = {
		{"bd", kBitBD, 2.0}, {"sd", kBitSD, 1.0}, {"lt", kBitLT, 1.5}, {"mt", kBitMT, 1.0}, {"ht", kBitHT, 1.0},
		{"rs", kBitRS, 0.5}, {"cp", kBitCP, 1.5}, {"cb", kBitCB, 1.0}, {"cy", kBitCY, 3.0}, {"oh", kBitOH, 1.5},
		{"ch", kBitCH, 0.5},
	};
	for (const auto &v : voices) {
		cases.push_back({nullptr, v.s, {{0.05, v.bit}}});
		cases.back().name = strdup((std::string(v.n)).c_str());
		cases.push_back({nullptr, v.s, {{0.05, uint16_t(v.bit | kBitAC)}}});
		cases.back().name = strdup((std::string(v.n) + "-acc").c_str());
	}
	{
		Case beat{"beat", 8.0, {}};
		const double step = 60.0 / 120 / 4; // 16ths at 120 BPM
		for (int s = 0; s < 64; ++s) {
			uint16_t d = (s % 8 == 7) ? kBitOH : kBitCH;
			if (s % 4 == 0) d |= kBitBD;
			if (s % 8 == 4) d |= kBitSD | kBitCP;
			if (s % 16 == 0) d |= kBitAC;
			if (s % 16 == 14) d |= kBitLT;
			if (s % 32 == 31) d |= kBitCY;
			beat.hits.push_back({0.05 + s * step, d});
		}
		cases.push_back(beat);
	}

	const Cfg ref{"ref", nullptr, nullptr};
	const Cfg cfgs[] = {
		{"g1", nullptr, "1"},   // the metal group at the board's rate
		{"f1", "1", nullptr},   // the board at the host rate, the metal group at 2x
		{"f1g1", "1", "1"},     // everything at the host rate
	};

	std::printf("%-8s %7s", "case", "ref%");
	for (const auto &g : cfgs)
		std::printf(" | %-4s cpu%%  band dB (Hz)      env dB", g.name);
	std::printf("\n");
	for (const auto &c : cases) {
		double cref;
		const auto r = render(c, ref, sr, cref);
		wav(dir + "/" + c.name + "-ref.wav", r, sr);
		std::vector<double> fc;
		const auto br = bands(r, sr, fc);
		const auto er = envelope(r, sr);
		double bmax = -1e9, emax = -1e9;
		for (double x : br) bmax = std::max(bmax, x);
		for (double x : er) emax = std::max(emax, x);
		std::printf("%-8s %6.1f%%", c.name, cref);
		for (const auto &g : cfgs) {
			double cpu;
			const auto y = render(c, g, sr, cpu);
			wav(dir + "/" + c.name + "-" + g.name + ".wav", y, sr);
			const auto by = bands(y, sr, fc);
			const auto ey = envelope(y, sr);
			double bd = 0, bf = 0;
			for (size_t i = 0; i < br.size(); ++i)
				if (br[i] > bmax - 50 && std::fabs(by[i] - br[i]) > std::fabs(bd)) { bd = by[i] - br[i]; bf = fc[i]; }
			double ed = 0;
			for (size_t i = 0; i < er.size() && i < ey.size(); ++i)
				if (er[i] > emax - 50 && std::fabs(ey[i] - er[i]) > std::fabs(ed)) ed = ey[i] - er[i];
			std::printf(" | %-4s %5.1f%% %+6.2f (%5.0f) %+6.2f", g.name, cpu, bd, bf, ed);
		}
		std::printf("\n");
		std::fflush(stdout);
	}
}
