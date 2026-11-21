# Lemus TR-909 core (AGPL-3.0)

From github.com/lemus-dev/lemus, commit 48f10291bd3aeb7b1b17193ebe68696549f2a114 (2026-11-17):
machines/tr909/core (the voicing board only: circuit, voiceboard, noise, hihat, cymbals,
oversampler, parallel, revisions.h) and its netlists, without the test_*.cir circuits. The CPU board
(machine, factory, upd7810) is left out: 9X9's own sequencer writes the board's latches.

generated/tr909/netlists.h comes from tools/netlists.cmake (Lemus's tools/cmake/netlists.cmake).

The ROMs aren't shipped: the user supplies hn61256p__c43.ic69 (hi-hat),
__c42.ic62 (crash) and __c44.ic54 (ride), matched by CRC.

## Local changes

- tr909/core/behave.h (new, 9X9): behavioural stand-ins for the VCO partitions of the toms, snare and
  kick, fitted to Lemus's own circuits; voiceboard.cpp steps them in their circuits' place when
  LEMUS_BEH_VCO=1 (or TR909_BEHVCO=1). Off by default; 9X9 builds with it on. Against stock Lemus at its own rate: toms
  within 1 dB per band (envelopes 0.1-0.3 dB rms), snare 1.8 dB (0.7 dB rms), kick 1.5 dB (0.05 dB
  rms). The rest of each voice is Lemus's circuit, unchanged.
- The snare's model (behave.h): Q44's reverse-active current at rest and a constant 7.5 mV inverter
  drop; a flip whose threshold moved past the ramp fires at once (it got stuck). Over Tune 0-1, Tone,
  accents and retriggers the snare is within 0.4 dB per band and 2 cents of Lemus's.
- voiceboard.cpp: the BD and toms count as quiet within 0.1 mV (about -100 dB from their peaks;
  Lemus: 10 uV, which kept them awake 4-8 s after a hit).
- netlists lt/mt/ht/bd/sd.cir: finer runtime partitions at stiff nodes (the toms' trigger partition
  in four, the BD's ENV1 apart, the snare's accent block in three): within 0.3 dB, cheaper edges.
  generated/tr909/netlists.h regenerated.
- Compile-time defaults for the solver's tools variables: LEMUS_FACTOR (oversampling below 80 kHz,
  Lemus 2), LEMUS_TOL (Newton, Lemus 1e-5), LEMUS_LINTOL (linear mode, Lemus 1e-4). 9X9 builds with
  1, 1e-3, 1e-3 (vst.json): FACTOR 1 costs about 4 dB at 12.8 kHz; the tolerances move the voices
  well under 1 dB. The TR909_* environment variables still override them.
