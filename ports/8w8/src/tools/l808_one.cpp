/* l808_one OUT.raw [beat|VOICE[-acc]] — one case from l808_ab, rendered with the process's TR808_* settings: float32 mono
 * at 48 kHz to OUT.raw, CPU % on stdout (best of 2). Settings the solver reads once per process (TR808_TOL, TR808_LINTOL,
 * TR808_LINWINDOW, TR808_PLAIN ...) need a process each, so the comparison is tools/l808_cmp.py's. AGPL-3.0. */
#include "tr808/core/voiceboard.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using namespace tr808;
int main(int argc, char **argv)
{
	const int sr = 48000;
	const std::string which = argc > 2 ? argv[2] : "beat";
	std::vector<std::pair<double, uint16_t>> hits;
	double secs = 8;
	if (which == "beat") {
		for (int s = 0; s < 64; ++s) {
			uint16_t d = (s % 8 == 7) ? kBitOH : kBitCH;
			if (s % 4 == 0) d |= kBitBD;
			if (s % 8 == 4) d |= kBitSD | kBitCP;
			if (s % 16 == 0) d |= kBitAC;
			if (s % 16 == 14) d |= kBitLT;
			if (s % 32 == 31) d |= kBitCY;
			hits.push_back({0.05 + s * 0.125, d});
		}
	} else {
		const char *n[] = {"bd", "sd", "lt", "mt", "ht", "rs", "cp", "cb", "cy", "oh", "ch"};
		const bool acc = which.size() > 4 && which.substr(which.size() - 4) == "-acc";
		const std::string v = acc ? which.substr(0, which.size() - 4) : which;
		for (int i = 0; i < 11; ++i)
			if (v == n[i]) hits.push_back({0.05, uint16_t((1u << i) | (acc ? kBitAC : 0))});
		secs = 2;
	}
	std::vector<float> out;
	double best = 1e9;
	for (int run = 0; run < 2; ++run) {
		VoiceBoard b;
		b.setCalibration(unitCalibration());
		b.reset();
		const int n = int(secs * sr), blk = 256;
		out.assign(size_t(n), 0.0f);
		std::vector<float> mix(blk);
		static float jk[kVoices][256];
		float *jp[kVoices];
		for (int v = 0; v < kVoices; ++v) jp[v] = jk[v];
		size_t h = 0;
		const auto t0 = std::chrono::steady_clock::now();
		for (int i = 0; i < n; i += blk) {
			const int k = std::min(blk, n - i);
			uint16_t data = 0;
			bool pulse = false;
			while (h < hits.size() && hits[h].first * sr < i + k) { data |= hits[h].second; pulse = true; ++h; }
			if (pulse) b.setLines(true, data);
			b.render(mix.data(), k, jp);
			if (pulse) b.setLines(false, data);
			for (int j = 0; j < k; ++j) { float s = 0; for (int v = 0; v < kVoices; ++v) s += jk[v][j]; out[size_t(i + j)] = s; }
		}
		best = std::min(best, std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
	}
	std::printf("%.1f\n", 100 * best / secs);
	FILE *f = std::fopen(argv[1], "wb");
	std::fwrite(out.data(), 4, out.size(), f);
	std::fclose(f);
}
