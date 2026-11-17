/* l808_cmp REF.raw TEST.raw — the largest 1/3-octave band difference (bands within 50 dB of the loudest) and
 * envelope difference (10 ms frames within 50 dB of the peak), as l808_ab, between two l808_one renders. AGPL-3.0. */
#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>
namespace {
std::vector<float> load(const char *p){ std::vector<float> d; FILE*f=std::fopen(p,"rb"); float x; while(f && std::fread(&x,4,1,f)==1) d.push_back(x); if(f) std::fclose(f); return d; }
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

}
int main(int, char **argv)
{
	const int sr = 48000;
	const auto r = load(argv[1]), y = load(argv[2]);
	std::vector<double> fc;
	const auto br = bands(r, sr, fc), by = bands(y, sr, fc);
	const auto er = envelope(r, sr), ey = envelope(y, sr);
	double bmax = -1e9, emax = -1e9;
	for (double x : br) bmax = std::max(bmax, x);
	for (double x : er) emax = std::max(emax, x);
	double bd = 0, bf = 0, ed = 0, es = 0; int en = 0;
	for (size_t i = 0; i < br.size(); ++i)
		if (br[i] > bmax - 50 && std::fabs(by[i] - br[i]) > std::fabs(bd)) { bd = by[i] - br[i]; bf = fc[i]; }
	for (size_t i = 0; i < er.size() && i < ey.size(); ++i)
		if (er[i] > emax - 50) { const double d = ey[i] - er[i]; if (std::fabs(d) > std::fabs(ed)) ed = d; es += d * d; ++en; }
	std::printf("band %+6.2f dB (%5.0f Hz)  env max %+6.2f dB rms %5.2f dB\n", bd, bf, ed, std::sqrt(es / std::max(1, en)));
}
