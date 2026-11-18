// Generated from machines/tr909/core/netlists/*.cir by tools/cmake/netlists.cmake.
#pragma once

#include <cstring>

namespace tr909::netlists {

inline constexpr const char *bd = R"cir(* TR-909 BASS DRUM, voicing board 73133090 (service notes p.11, grid K-N 6-13; crop
* p11-bd-bass-drum.png; theory p.6). Transcription notes: docs/circuit/bd.md.
* Inputs: trig, the BD output of the TRIG latch IC1 (pin 10; 0/5 V for one 2 ms panel scan);
* acc, the BD accent DAC (IC2 into RA1, RM0621) into IC11b pin 5, its open-circuit voltage
* 5 V * code / 63; noise, the noise bus (IC33 pin 9, 0/15 V) into R10.
* Output: out, IC11a's output at the connector to the multi-jack board (CN-5 pin 33, R500).
* Controls: tune (VR2 100k A, a rheostat in ENV3's discharge), level (VR4 100k B, a rheostat in
* IC11a's feedback), attack (VR3 500 B, a rheostat into the attack VCA), decay (VR5 1M A, a
* rheostat in ENV1's discharge).
* A triangle VCO (integrator IC13a, comparator IC13b) whose rate IC12a/IC12b set from ENV3 (C9),
* reset by Q7/Q11 for the trigger, rounded by D10/D11 into the body VCA Q12 (ENV1: C8, held for
* about 50 ms by the C7/Q3/Q4 timer); the attack: Q8's 2 ms pulse through the Q9 Sallen-Key and
* the low-passed noise into the VCA Q6 (ENV2: C12). IC11a sums both.
* Board: the late one. Early units by .param: c9=0.22u before SN 381500 (SN p.7, "for expanding
* the TUNE range"); r495=150k up to SN 371499 (p.11, "SN UP to 371499 SURFACE MOUNTED"); the
* jack board's C500 is from SN 415300 (c500=1p before: absent; the solver takes no zero).
.param rlatch=600 rdac=2.54k c9=0.33u r495=1e12 c500=0.01u
* Parts a unit's calibration sets (docs/circuit/bd.md, "Calibration"; the printed values here)
.param c14=0.033u r27=1.5meg c8=0.33u r44=22k r15=100k r52=10k r53=2.2k r43=47
* dat: C8's and C9's (tantalum) dielectric absorption (circuit.h: da=): 2 %. Modern MnO2
* tantalum is about 0.5 % (Vishay's dielectric comparison); none is published for the 909's parts.
* The same as the toms' (docs/circuit/toms.md, "Calibration"); here it moves the decays by under 2 %.
.param dat=0.02
VB15 b15 0 15
VNB15 nb15 0 -15
VTRIG trig 0 input trig
VACC acc 0 input acc
VNOISE noise 0 input noise
* IC1's output resistance (HD14174 at +5 V, a stand-in), and the RM0621's 2.54k (5k || ... || 160k)
RIC1 trig t1 {rlatch}
RDAC acc i11p {rdac}
* --- trigger: Q1 pulls n1 down; Q2 (PNP) then lifts n2 to +15 V; C1 kicks Q7 on (the VCO reset)
R1 t1 q1b 22k
R2 q1b 0 10k
Q1 n1 q1b 0 2SC2603
D4 x4 n1 1SS133
R3 q2b x4 100k
R4 b15 q2b 47k
R11 x4 c6a 4.7k
C6 c6a 0 0.0022u
Q2 n2 q2b b15 2SA1115
R19 n2 0 2.2k
C1 n1 p1 0.068u
R12 p1 q7b 100k
R13 b15 q7b 47k
Q7 q7c q7b b15 2SA1115
* --- accent: IC11b x3.13; Q5 passes it to ENV2 while n1 is low, Q4 to ENV1 while Q3 is on
R38 i11n 0 4.7k
R37 i11n ac 10k
X11B i11p i11n ac opamp {gbw} 1e5 {opsat} {sr}
R39 ac q5b 1meg
R34 q5b d5a 100k
D5 d5a n1 1SS133
Q5 e2 q5b ac 2SA1115
* ENV1's hold timer: n2 charges C7 through D6/R20; C7 holds Q3 on through R18
D6 n2 d6k 1SS133
R20 d6k c7 6.8k
C7 c7 0 0.033u
R18 c7 q3b 470k
R17 q3b 0 470k
Q3 q3c q3b 0 2SC2603
R32 q4b q3c 47k
R33 ac q4b 1meg
Q4 e1 q4b ac 2SA1115
* ENV1: R35/D9 charge C8; D8 into the R36/R22 divider limits it; DECAY (VR5 + R58) discharges it
R35 e1 j 1.8k
D8 j d8k 1SS133
R36 b15 d8k 47k
R22 d8k 0 10k
D9 j c8 1SS133
C8 c8 0 {c8} da={dat}
P5 c8 c8 vr5b 1meg decay a0.15
R58 vr5b 0 47k
R54 c8 q12b 470k
* ENV2: Q5 charges C12 through R41; it discharges only through R42 into Q6's base (and R495)
R41 e2 c12 22k
C12 c12 0 0.033u
R42 c12 q6b 470k
R495 q6b 0 {r495}
* --- ENV3, the pitch envelope: n2 charges C9 through D7/R21; TUNE (R57 + VR2) || R26 discharges it
D7 n2 d7k 1SS133
R21 d7k c9 2.2k
C9 c9 0 {c9} da={dat}
R57 c9 r57b 22k
P2 r57b r57b 0 100k tune a0.15
* --- CV: IC12a, va = -(R28/R26 vc9 + R28/R27 15 V); IC12b, x(+1) or x(-1) as Q10 grounds its + input
R26 c9 i12an 330k
R27 b15 i12an {r27}
R28 i12an va 150k
R25 i12ap 0 100k
X12A i12ap i12an va opamp {gbw} 1e5
R29 va i12bn 100k
R30 i12bn vb 100k
R31 va i12bp 100k
Q10 i12bp q10b 0 2SA1115
R23 cmp q10b 100k
R24 q10b 0 22k
X12B i12bp i12bn vb opamp {gbw} 1e5 {opsat} {sr}
* --- VCO: IC13a integrator, IC13b comparator with hysteresis (R49/R50; lightly loaded, about 67k,
* it swings +-14 V: opsatlight); the reset: Q7 turns Q11 on across C14 (through R51) and holds
* IC13b's + input high through R48/D12
R59 vb i13n 47k
C14 i13n tri {c14}
X13A 0 i13n tri opamp {gbw} 1e5
R47 q7c q11b 330k
Q11 q11c q11b i13n 2SC2603
R51 q11c tri 2.2k
R48 q7c d12a 22k
D12 d12a i13p 1SS133
R49 i13p cmp 100k
R50 i13p 0 47k
X13B i13p tri cmp opamp {gbw} 1e5 {opsatlight} {sr}
* --- waveshaper and the body VCA Q12 (collector on the 47 ohm node, emitter into IC11a's sum)
R52 tri sh {r52}
D10 0 sh 1SS133
D11 sh 0 1SS133
R53 sh q12c {r53}
R43 q12c 0 {r43}
Q12 q12c q12b sj 2SC2603S
* --- attack pulse: Q8 (PNP, on at rest) lets go of its collector while n2 is high; C2/R7 into the
* Q9 Sallen-Key low-pass (R16, C4; C3 from its output), C11/R44 into the attack sum
R5 n2 q8b 100k
R6 b15 q8b 22k
Q8 q8c q8b b15 2SA1115
R14 q8c 0 22k
C5 q8c 0 0.0068u
C2 q8c c2b 0.0033u
R7 c2b ska 100k
R15 ska 0 {r15}
R16 ska q9b 39k
C4 q9b 0 100p
Q9 b15 q9b q9e 2SC2603
R8 q9e nb15 10k
C3 ska q9e 0.0047u
C11 c11p q9e 0.47u
R44 c11p sum {r44}
* --- noise: R10/R9 divider, C15, the R46/C13 low-pass, R45 into the attack sum
R10 noise nzd 12k
R9 nzd 0 4.7k
C15 nzd c15n 0.47u
R46 c15n c13 4.7k
C13 c13 0 0.1u
R45 c13 sum 4.7k
* --- the attack VCA Q6: ATTACK (VR3) into R55, ENV2 into its base
R56 sum 0 330
P3 sum q6c q6c 500 attack
R55 q6c 0 47
Q6 q6c q6b sj 2SC2603S
* --- IC11a sums the VCAs' currents; LEVEL (VR4) is its feedback
P4 out out sj 100k level
X11A 0 sj out opamp {gbw} 1e5 {opsat} {sr}
* the multi-jack board's load (p.12; mix.cir runs it): R500 1k, C500 0.01u (SN 415300 up), and
* R501, R502 (12k each) into the mix buses' virtual grounds while no plug is in the BD jack
R500 out jk 1k
C500 jk 0 {c500}
R501 jk 0 12k
R502 jk 0 12k
.control tune 0.5
.control level 0.5
.control attack 0.5
.control decay 0.5
.output out out
.output tri tri
.output env3 c9
.output env1 c8
.output env2 c12
.output atk sum
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
* The VCO's loop (IC12b, Q10, the comparator) stays whole: a cut in it would delay every flip.
.part trig in=q7b,q8b,q12b,q6b,i12an RIC1 RDAC R1 R2 Q1 D4 R3 R4 R11 C6 Q2 R19 C1 R12
+ R38 R37 X11B R39 R34 D5 Q5 D6 R20 C7 R18 R17 Q3 R32 R33 Q4 R35 D8 R36 R22 D9 C8 P5 R58 R54
+ R41 C12 R42 D7 R21 C9 R57 P2 R26 R5
.part vco in=c9,p1,sh R26 R27 R28 R25 X12A R29 R30 R31 Q10 R23 R24 X12B R59 C14 X13A R12 R13 Q7 R47
+ Q11 R51 R48 D12 R49 R50 X13B R52
.part shaper in=tri,q12c R52 D10 D11 R53
.part click in=n2,sum R5 R6 Q8 R14 C5 C2 R7 R15 R16 C4 Q9 R8 C3 C11 R44
.part noise in=sum R10 R9 C15 R46 C13 R45
.part vca in=sh,c8,c12,c11p,c13 R53 R43 Q12 R54 R44 R45 R56 P3 R55 Q6 R42 R495 P4 X11A R500 C500
+ R501 R502
)cir";

inline constexpr const char *cr = R"cir(* TR-909 CRASH, voicing board 73133090, analog half (service notes p.11, grid B-F 11-20, crop
* p11-cy-crash.png; p.5 "DIGITAL VOICE GENERATORS"; docs/circuit/cymbals.md). The clock (IC59b/c,
* VR25 TUNE; C50/R189) is crclk.cir; the logic between it and the D/As (timing flip-flops IC60,
* counters IC61 + IC58a, ROM IC62, latch IC63, NANDs IC59a/d) is core/cymbals.* (Cymbal), which
* gives this netlist the latch's and counter's outputs and IC59d's.
* Inputs: d0..d5, IC63's outputs (4174 on +5 V, 0/5 V, averaged over each step by Cymbal) into RA10's
* 32R .. R, i.e. the ROM byte's bits 2..7 (the public images are in D/A order: bit 7 is the MSB);
* e0..e5, counter bits 9..14 (IC61 Q10-Q12, IC58a Q1-Q3) into RA11's 32R .. R; run, IC59d's output
* (NOT counter bit 15: 5 V while the counter runs); rd.run, the ride's IC57a output (its load on the
* shared accent DAC moves with it); acc, the open-circuit voltage of accent DAC IC9 (RA8, address
* 4008, shared with the ride; 5 V code/63).
* Outputs: out, IC65a at the voicing board's connector W5 (CN-5 pin 41) to the multi-jack board;
* jack, the MULTI OUT jack's contact behind R524/C509 with nothing plugged in (the mix resistors on it).
* Controls: level (VR24 50k B, a rheostat in IC65a's feedback).
* RA10 (RM0621, R = 5k binary weighted) through C127 drives the 22 ohm R431: a current into the VCA
* Q71, a saturated NPN between that node and IC51b's virtual ground whose conductance follows its
* base current. That current is the anti-log pair Q70's: IC52b forces Q70's reference collector to
* the envelope current (5 V - RA11) / R432 (RA11 counts up, so the current falls in 64 steps of 512
* samples to none), and the accent (R426), RUN (R427, the mute when stopped) and R428 set the
* reference base against R429's, so the gain is exponential in the accent. Then two unity-gain
* Sallen-Key low-passes with emitter followers Q75, Q76 (about 4th-order Butterworth at 27 kHz),
* C132 and the LEVEL stage IC65a.
* rlatch: IC63's and the counters' output resistance (HD14174B at 5 V, ~0.9 mA at 0.4 V drop), also
* taken for IC59d and the ride's IC57a (4011UB); rdac: IC9's RM0621 Thevenin resistance (5k || 10k
* || ... || 160k).
.param rlatch=450 rdac=2.54k
VB15 b15 0 15
VNB15 nb15 0 -15
VV5 v5 0 5
VACC acc 0 input acc
VRUN run 0 input run
VRDRUN rdrun 0 input rd.run
VD0 d0 0 input d0
VD1 d1 0 input d1
VD2 d2 0 input d2
VD3 d3 0 input d3
VD4 d4 0 input d4
VD5 d5 0 input d5
VE0 e0 0 input e0
VE1 e1 0 input e1
VE2 e2 0 input e2
VE3 e3 0 input e3
VE4 e4 0 input e4
VE5 e5 0 input e5
* --- the envelope: counter bits 9-14 through RA11 and R432 into IC52b's virtual node at +5 V ------
RLE0 e0 l6 {rlatch}
RLE1 e1 l7 {rlatch}
RLE2 e2 l8 {rlatch}
RLE3 e3 l9 {rlatch}
RLE4 e4 l10 {rlatch}
RLE5 e5 l11 {rlatch}
RA11F l6 ea 160k
RA11E l7 ea 80k
RA11D l8 ea 40k
RA11C l9 ea 20k
RA11B l10 ea 10k
RA11A l11 ea 5k
R432 ea n52b 270k
* the anti-log pair Q70 (2SA798): IC52b holds Q70A's collector at its - input (+5 V), so Q70A carries
* the envelope current; Q70B's collector feeds Q71's base
X52B v5 n52b o52b opamp {gbw}
C126 n52b o52b 1000p
R430 o52b q70e 2.2k
Q70A n52b q70b1 q70e 2SA798
* the reference base: R428 to +5 V, R426 from the accent DAC, R427 from IC59d (RUN)
RRUN run rn {rlatch}
R428 v5 q70b1 220
R426 ac q70b1 4.7k
R427 rn q70b1 2.2k
* accent DAC IC9 (RA8), shared with the ride: its source resistance, and the ride's base network
* on it (R435 to its reference base, R433 to +5 V, R437 to its IC57a); the ride's base current
* (tens of nA through 200 ohms) is left out
RDAC acc ac {rdac}
R435 ac rdb 4.7k
R433 v5 rdb 220
RRD rdrun rdr {rlatch}
R437 rdr rdb 2.2k
* --- the D/A and the VCA ------------------------------------------------------------------------
R429 v5 q70b2 100
Q70B q71b q70b2 q70e 2SA798
* IC63's outputs through RA10 (R = 5k: d5 into R, d0 into 32R) to C127
RL0 d0 l0 {rlatch}
RL1 d1 l1 {rlatch}
RL2 d2 l2 {rlatch}
RL3 d3 l3 {rlatch}
RL4 d4 l4 {rlatch}
RL5 d5 l5 {rlatch}
RA10F l0 da 160k
RA10E l1 da 80k
RA10D l2 da 40k
RA10C l3 da 20k
RA10B l4 da 10k
RA10A l5 da 5k
C127 da vn 10u
R431 vn 0 22
* Q71 (S): collector on the 22 ohm node, emitter at IC51b's virtual ground, R425 the I-V
Q71 vn q71b n51b 2SC2603S
X51B 0 n51b o51b opamp {gbw}
R425 n51b o51b 47k
* --- the filter: two unity-gain Sallen-Key low-passes, emitter followers Q75, Q76 ----------------
R456 o51b s1 5.6k
C136 s1 q75e 1200p
R457 s1 q75b 5.6k
C137 q75b 0 1000p
Q75 b15 q75b q75e 2SC2603
R458 q75e nb15 10k
R459 q75e s2 5.6k
C138 s2 q76e 2700p
R461 s2 q76b 5.6k
C139 q76b 0 390p
Q76 b15 q76b q76e 2SC2603
R460 q76e nb15 10k
* LEVEL: C132, R445 into IC65a with VR24 in its feedback
C132 q76e c132 10u
R445 c132 n65a 22k
X65A 0 n65a out opamp {gbw} 1e5 {opsat}
P24 out out n65a 50k level
* the multi-jack board's load on W5 (JK-1, SN p.12): R524, C509 (from SN 415300) and the mix
* resistors R525 12k, R526 10k into the mix amplifiers' virtual grounds (mix.cir has them)
R524 out jack 1k
C509 jack 0 0.01u
R525 jack 0 12k
R526 jack 0 10k
.control level 0.5
.output out out
.output jack jack
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
* The envelope and the anti-log pair's reference side (cut at Q70's emitters: IC52b holds them where
* Q70A carries the envelope current, whatever Q70B draws), then the D/A and VCA to IC51b, then the
* filter and LEVEL, which sleep on their own while the VCA is shut.
.part env RLE0 RLE1 RLE2 RLE3 RLE4 RLE5 RA11F RA11E RA11D RA11C RA11B RA11A R432 X52B C126 R430 Q70A
+ RRUN R428 R426 R427 RDAC R435 R433 RRD R437
.part vca in=q70e sleep R429 Q70B RL0 RL1 RL2 RL3 RL4 RL5 RA10F RA10E RA10D RA10C RA10B RA10A C127
+ R431 Q71 X51B R425
.part out in=o51b sleep R456 C136 R457 C137 Q75 R458 R459 C138 R461 C139 Q76 R460 C132 R445 X65A P24
+ R524 C509 R525 R526
)cir";

inline constexpr const char *crclk = R"cir(* TR-909 CRASH clock: the IC59b/c astable with TUNE, and the counters' reset differentiator, voicing
* board 73133090 (service notes p.11, grid E-F 11-13, crop p11-cy-crash.png; docs/circuit/cymbals.md,
* "Clock"). Cymbal (core/cymbals.*) runs it at its own rate to time the edges that clock IC60a/b
* (oscb, osca) and the end of the counters' reset (rst); the board doesn't run it (its edges take
* nanoseconds, its period needs 100 MHz steps to be right to 0.05 %: docs/circuit/cymbals.md, "Rates").
* Inputs: trig, the crash output of TRIG latch IC10 (4174 on +5 V, 0/5 V); run, IC59d's output (NOT
* counter bit 15: 5 V while the counter runs), the enable of IC59b.
* Outputs: osca, IC59c pin 10 (clocks IC60b); oscb, IC59b pin 4 (clocks IC60a); rst, the reset line
* of IC60a/b, IC61 and IC58a.
* Controls: tune (VR25 10k B, a rheostat in the timing resistance, clockwise faster).
* IC59c (inputs 8, 9 tied, from R442) and IC59b (pin 5 the run line, pin 6 osca) make the two-gate
* astable: R443 + VR25 from osca and C131 from oscb meet at x, and the gate's input diodes clamp x's
* swing past the rails through R442. With run low IC59b's output stays high and IC59c settles,
* self-biased through R442/R443/VR25, at its threshold: the clock stops.
* Defaults are the late board: R442 10k from SN 392600 (33k before; annotated on p.11).
* rtrig: IC10's output resistance (HD14174B at 5 V, ~0.9 mA at 0.4 V drop).
.param rtrig=450 r442=10k vr25=10k c131=470p r443=6.8k cg=5p ca=15p cb=15p cgd=1f
VV5 v5 0 5
VTRIG trig 0 input trig
VRUN run 0 input run
* TRIG's edge through C50 into R189: the reset pulse (about 7 us above the inputs' threshold); DRST
* stands for those inputs' protection diodes, which clamp TRIG's falling edge
RTRIG trig t {rtrig}
C50 t rst 100p
R189 rst 0 100k
DRST 0 rst DCMOS
* IC59c: P pair from +5 V, N pair in series, both inputs on g
M59CP1 osca g v5 4011UB_P
M59CP2 osca g v5 4011UB_P
M59CN1 osca g c59c 4011UB_N
M59CN2 c59c g 0 4011UB_N
DG1 g v5 DCMOS
DG2 0 g DCMOS
* IC59b: pin 5 (run) and pin 6 (osca)
M59BP1 oscb run v5 4011UB_P
M59BP2 oscb osca v5 4011UB_P
M59BN1 oscb osca c59b 4011UB_N
M59BN2 c59b run 0 4011UB_N
* the pins' capacitance: IC59c's input (C59G); IC59c's output with IC59b pin 6 and IC60b's clock
* (C59A); IC59b's output with IC60a's clock (C59B): about 5 pF a pin
C59G g 0 {cg}
C59A osca 0 {ca}
C59B oscb 0 {cb}
* the gates' gate-drain (Miller) capacitance, IC59c's and IC59b's from osca: not on any sheet
* (C_IN 5 pF typ is with the output at a rail); none by default, fitted with the gates' drive
* (w4011n, w4011p) in the calibration (docs/circuit/cymbals.md, "Calibration")
C59M g osca {cgd}
C59N osca oscb {cgd}
* the timing parts
P25 x1 osca osca {vr25} tune
R443 x1 x {r443}
C131 x oscb {c131}
R442 x g {r442}
.control tune 0.5
.output osca osca
.output oscb oscb
.output rst rst
)cir";

inline constexpr const char *hc = R"cir(* TR-909 HAND CLAP, voicing board 73133090 (service notes p.11, grid G-J 11-16; docs/circuit/rshc.md).
* Inputs: trig, the HC output of TRIG latch IC10 (4174; TRIG2 bit 6; 0/5 V for one 2 ms scan tick);
* acc, the open-circuit voltage of accent DAC IC7 (RA6, RM0621; 5 V * code / 63), which the rim shot
* shares; noise, the noise generator's output (IC33 pin 9, 0-15 V) into R208.
* Output: out, IC28a at the voicing board's connector W5 pin 39 to the multi-jack board.
* Controls: level (VR20 50k B, a rheostat in IC28a's feedback); tm (TM 10k B, the service trim,
* default the .param tm: set per the ADJUSTMENT note, LEVEL at centre, 2 Vp-p at MULTI OUT;
* docs/circuit/rshc.md, "TM").
* The noise is band-passed by IC26b (MFB, ~960 Hz) and goes two ways into IC28a: through the OTA
* IC30 (BA662A), whose I_ABC the dual PNP Q38 takes from R235 by the sawtooth envelope ENV1 (C65,
* slammed down four times by Q86 under the AN6912 IC29) against TM, the clap's four pulses; and
* through C55/R204/C57 (~80-440 Hz) into the transistor attenuator Q37, whose base C59/C61 feed
* from the trigger through D52, the "reverb" tail. IC26a buffers ACC (x 3.23, +1.5 V from R496)
* for Q36, which passes it to C59 during the trigger, and for R235.
* Q40 in this circuit is the one later boards call Q86 (SN p.10); the drawing joins its base to
* R222 (shorting R221): the netlist follows the 808's topology and the Nava copy of the board
* (R221 in series with the base, R222 at comparator d's output).
* rlatch: IC10's output resistance (as rs.cir); rdac: IC7's Thevenin resistance. The DAC node's
* other load, the rim shot's IC48b (+), is an op-amp input and takes no current. c65: C65 0.47u/50,
* an electrolytic (the units' tooth spacings: docs/circuit/rshc.md). c507: the jack board's C507,
* from SN 415300 (c507=1p before).
* tm: TM's wiper (0 at the -15 V end, 1 at ground), a .param so the board's Calibration can set it;
* 0.255 is the service notes' adjustment done on the model. c42, c43, c57, c59, c61, c62, c64, r226:
* printed values, for the Calibration (docs/circuit/rshc.md, "Calibration").
.param rlatch=600 rdac=2.54k cmpvg=0.5m c65=0.47u c507=0.01u tm=0.255 c62=0.027u c64=0.001u r226=68k
+ c59=0.1u c61=0.1u c42=0.0047u c43=0.0047u c57=0.47u
VB15 b15 0 15
VNB15 nb15 0 -15
VTRIG trig 0 input trig
VACC acc 0 input acc
VNOISE noise 0 input noise
RLATCH trig t {rlatch}
RDAC acc a {rdac}
* accent buffer IC26a: ACC x (1 + R212/R206 + R212/R496) + 15 V x R212/R496
X26A a n26a o26a opamp {gbw} 1e5 {opsat} {sr}
R206 n26a 0 4.7k
R212 n26a o26a 10k
R496 n26a nb15 100k
* trigger gate: Q35 pulls Q36's base down; Q36 passes IC26a's output to R213/R218 (comparator a)
* and through D52 to C59
R215 t q35b 22k
R216 q35b 0 10k
Q35 q35c q35b 0 2SC2603
R210 q35c q36b 22k
R211 q36b o26a 10k
Q36 q36c q36b o26a 2SA1115
R213 q36c n4 22k
R218 n4 nb15 22k
* IC29 AN6912 on -15 V only (pin 3 ground, pin 12 -15 V), open collectors to -15 V; the ladder
* R219/R217/R214 sets -5 V (m5) and -10 V (m10)
R219 0 m5 4.7k
R217 m5 m10 4.7k
R214 m10 nb15 4.7k
* a (4-, 5+, out 2): sinks while the trigger lifts pin 4 above -10 V; C62/R229 hold it ~30 ms
BC29A m10 n4 oa nb15 comparator vg={cmpvg} vcm=-1.5 vos=-2m
R229 oa 0 1meg
C62 oa 0 {c62}
* b (8-, 9+, out 14): the window, low while C62 is under -5 V; R227/R228 set the sawtooth's
* threshold p (-10.1 V in the window, 0 V after)
BC29B oa m5 ob nb15 comparator vg={cmpvg} vcm=-1.5 vos=-2m
R227 ob p 2.7k
R228 p 0 5.6k
* c (6-, 7+, out 1): released in the window, so R220/D54 pull ENV1 up
BC29C m5 ob oc nb15 comparator vg={cmpvg} vcm=-1.5 vos=-2m
R220 oc 0 82k
D54 oc env1 1SS133
C65 0 env1 {c65}
* d (10-, 11+, out 13): released once ENV1 (through R225, C63 its hold) rises past p; R222 then
* lifts Q86's base through R221, and Q86 slams C65 to -15 V through R223. At the window's end
* C64/R226 kick Q86 once more (the fourth tooth)
R225 env1 dp 1meg
BC29D dp p od nb15 comparator vg={cmpvg} vcm=-1.5 vos=-2m
C63 od dp 220p
R222 0 od 10k
R221 od q86b 18k
C64 ob x64 {c64}
D55 nb15 x64 1SS133
R226 x64 q86b {r226}
Q86 q86c q86b nb15 2SC2603
R223 q86c env1 330
* Q38 (2SA798 pair): R235's current from IC26a split by ENV1 (R224/R238) against TM (R494/R230);
* the left collector is IC30's I_ABC
R224 q86c q38l 470k
R238 q38l 0 15k
R235 o26a e38 15k
Q38A abc q38l e38 2SA798
Q38B nb15 q38r e38 2SA798
R230 q38r 0 2.2k
R494 tmw q38r 47k
PTM 0 tmw nb15 10k tm
* the noise band-pass IC26b (multiple feedback): R208, R209, C42, C43, R207
R208 noise nbp 47k
R209 nbp 0 10k
C42 nbp n26b {c42}
C43 nbp o26b {c43}
R207 n26b o26b 150k
X26B 0 n26b o26b opamp {gbw}
* IC30 BA662A: (-) pin 2 through C56/R202/R201, (+) pin 3 R236 to ground; pin 6 into R237 and,
* through C58, IC28a's virtual ground (pins 4 and 7 grounded, pin 8 open: the buffer unused)
C56 o26b c56 0.022u
R202 c56 ota_n 39k
R201 ota_n 0 1k
R236 ota_p 0 1k
B30 abc nb15 ota_p ota_n o30 ota rs=450
R237 o30 0 82k
C58 o30 x28 0.001u
* the reverb path: C55, R204, C57, then R205/R203 into the collector of Q37 (marked S), whose
* emitter is at IC28a's virtual ground
C55 o26b c55 0.47u
R204 c55 c57 1k
C57 c57 0 {c57}
R205 c57 q37c 3.3k
R203 q37c 0 47
Q37 q37c q37b x28 2SC2603S
* its envelope: D52 charges C59 during the trigger; R233 into C61 (D53 its clamp), R234 to the base
D52 q36c c59 1SS133
C59 c59 0 {c59}
R233 c59 c61 470k
C61 c61 0 {c61}
D53 0 c61 1SS133
R234 c61 q37b 470k
* IC28a with LEVEL (VR20) in its feedback
X28A 0 x28 out opamp {gbw}
P20 out out x28 50k level
* the multi-jack board's load (p.12; mix.cir runs it): R518 and C507 (SN 415300 up) to the jack,
* whose normal contact takes R519/R520 to the mix buses (AC ground)
R518 out jk 1k
C507 jk 0 {c507}
R519 jk 0 12k
R520 jk 0 12k
.control level 0.5
.control tm {tm}
.output out out
* Runtime partitions (tr808/docs/model.md, "Partitions"): run in this order; in= nodes come from the
* others. C58 and R224, R234 are across cuts, listed on both sides.
* The noise band-pass runs at twice the voice's rate (its top octave, as the snare's filter).
.part noise over=2 R208 R209 C42 C43 R207 X26B
.part trig in=q37b RLATCH RDAC X26A R206 R212 R496 R215 R216 Q35 R210 R211 Q36 R213 R218 D52 C59
+ R233 C61 D53 R234
.part env in=n4,q38l R219 R217 R214 BC29A R229 C62 BC29B R227 R228 BC29C R220 D54 C65 R225 BC29D C63
+ R222 R221 C64 D55 R226 Q86 R223 R224
.part ota in=q86c,o26a,o26b,x28 sleep R224 R238 R235 Q38A Q38B R230 R494 PTM C56 R202 R201 R236 B30
+ R237 C58
.part out in=o26b,c61,o30 sleep C55 R204 C57 R205 R203 Q37 R234 C58 X28A P20 R518 C507 R519 R520
)cir";

inline constexpr const char *hh = R"cir(* TR-909 HI-HAT (closed and open), voicing board 73133090, analog half (service notes p.11, grid
* B-D 5-12; p.5 "DIGITAL VOICE GENERATORS"; docs/circuit/hh.md). The digital half (oscillator,
* counters IC70/IC71, address logic D196-D199, ROM IC69, latch IC68) is core/hihat.*: it gives this
* netlist IC68's six outputs and IC72a's RUN/STOP.
* Inputs: d0..d5, IC68's outputs (4174 on +5 V, 0/5 V, each step band-limited by HiHat) into RA9's
* 32R .. R, i.e. the ROM byte's bits 2..7 (the public images are descrambled: bit 7 is the MSB);
* run, IC72a's output (RUN/STOP, 5 V while the counter runs); trig, the HH output of TRIG latch IC10
* (Q3, pin 7); ohsel, IC10's CLOSED/OPEN (Q1, pin 2, 5 V for a closed hat); acc, the open-circuit
* voltage of accent DAC IC8 (RA7, address 4007; 5 V code/63).
* Outputs: out, IC65b at the voicing board's connector W5 to the multi-jack board; jack, the
* MULTI OUT jack's contact behind R521/C508 with nothing plugged in (the mix resistors on it).
* Controls: level (VR22 50k B, a rheostat in IC65b's feedback), chdecay (VR21 100k B) and ohdecay
* (VR23 1M A), rheostats charging C135.
* RA9 (RM0621, R = 5k binary weighted) through C157 drives the 22 ohm R482: a current into the
* VCA Q85, a saturated NPN between that node and IC67b's virtual ground whose conductance follows
* its base current. That current is the anti-log pair Q84's: IC67a forces Q84's reference
* collector to the envelope current (5 V - C135)/R453, and the accent (R481), RUN/STOP (R488) and
* R489 set the reference base against R490's, so the gain is exponential in the accent. Q74
* empties C135 while TRIG is high; it charges through R452 + OH DECAY, and through R451 + CH DECAY
* as well while Q73 (Q72, CLOSED) is on. Then two unity-gain Sallen-Key low-passes with emitter
* followers Q80, Q81 (about 4th-order Butterworth at 27 kHz), C134 and the LEVEL stage IC65b.
* Defaults are the late board: C134 0.01u from SN 415300 (10u before, SN p.7).
* Rate: twice the board's (176.4 / 192 kHz), for the D/A's first image band, which the filters
* pass, and their corners near 27 kHz (docs/circuit/hh.md, "Rates and the D/A's timing").
* rtrig, rlatch: IC10's and IC68's output resistance (HD14174B at 5 V, ~0.9 mA at 0.4 V drop), also
* taken for IC72a (4011UB); rdac: IC8's RM0621 Thevenin resistance (5k || 10k || ... || 160k).
.param rtrig=450 rlatch=450 rdac=2.54k c134=0.01u c135=1u
VB15 b15 0 15
VNB15 nb15 0 -15
VV5 v5 0 5
VTRIG trig 0 input trig
VOHSEL ohsel 0 input ohsel
VACC acc 0 input acc
VRUN run 0 input run
VD0 d0 0 input d0
VD1 d1 0 input d1
VD2 d2 0 input d2
VD3 d3 0 input d3
VD4 d4 0 input d4
VD5 d5 0 input d5
RTRIG trig t {rtrig}
ROHSEL ohsel cl {rtrig}
RDAC acc a {rdac}
RRUN run rn {rlatch}
* the D/A: IC68's outputs through RA9 (R = 5k: d5 into R, d0 into 32R) to C157
RL0 d0 l0 {rlatch}
RL1 d1 l1 {rlatch}
RL2 d2 l2 {rlatch}
RL3 d3 l3 {rlatch}
RL4 d4 l4 {rlatch}
RL5 d5 l5 {rlatch}
RA9F l0 da 160k
RA9E l1 da 80k
RA9D l2 da 40k
RA9C l3 da 20k
RA9B l4 da 10k
RA9A l5 da 5k
C157 da vn 10u
R482 vn 0 22
* the VCA: Q85 (S) from the 22 ohm node into IC67b's virtual ground, R483 the I-V
Q85 vn q85b n67b 2SC2603S
X67B 0 n67b o67b opamp {gbw}
R483 n67b o67b 47k
* the anti-log pair Q84 (2SA798): IC67a holds Q84's reference collector at its - input (+5 V) and
* so the reference current at R453's, (5 V - C135) / 220k; the other collector feeds Q85's base
R453 c135 n67a 220k
X67A v5 n67a o67a opamp {gbw}
C156 n67a o67a 1000p
R484 o67a q84e 2.2k
Q84A n67a q84b2 q84e 2SA798
Q84B q84c q84b1 q84e 2SA798
R485 q84c q85b 10k
R490 v5 q84b1 100
R489 v5 q84b2 150
R481 a q84b2 4.7k
R488 rn q84b2 2.2k
* the envelope: Q74 (TRIG) empties C135; R452 + VR23 (OH DECAY) charge it always, R451 + VR21
* (CH DECAY) through Q73 while Q72 (CLOSED) is on
R454 t q74b 22k
R455 q74b 0 10k
Q74 c135 q74b 0 2SC2603
C135 c135 0 {c135}
R452 v5 r452 100k
P23 c135 c135 r452 1meg ohdecay a0.15
R447 cl q72b 22k
R448 q72b 0 10k
Q72 q72c q72b 0 2SC2603
R449 q72c q73b 22k
R450 v5 q73b 10k
Q73 q73c q73b v5 2SA1115
R451 q73c r451 10k
P21 c135 c135 r451 100k chdecay
* the filter: two unity-gain Sallen-Key low-passes, emitter followers Q80, Q81
R480 o67b s1 5.6k
C148 s1 q80e 1200p
R475 s1 q80b 5.6k
C149 q80b 0 1000p
Q80 b15 q80b q80e 2SC2603
R476 q80e nb15 10k
R477 q80e s2 5.6k
C150 s2 q81e 2700p
R479 s2 q81b 5.6k
C151 q81b 0 390p
Q81 b15 q81b q81e 2SC2603
R478 q81e nb15 10k
* LEVEL: C134, R444 into IC65b with VR22 in its feedback
C134 q81e c134 {c134}
R444 c134 n65b 22k
X65B 0 n65b out opamp {gbw} 1e5 {opsat}
P22 out out n65b 50k level
* the multi-jack board's load on W5 (JK-1, SN p.12): R521, C508 (from SN 415300) and the mix
* resistors R522 15k, R523 8.2k into the mix amplifiers' virtual grounds (mix.cir has them)
R521 out jack 1k
C508 jack 0 0.01u
R522 jack 0 15k
R523 jack 0 8.2k
.control level 0.5
.control chdecay 0.5
.control ohdecay 0.5
.output out out
.output jack jack
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
* The envelope (cut at IC67a's virtual node, R453 in both), the anti-log pair with the D/A and the
* VCA to IC67b (Q85's base moves with the signal, so Q84 stays with it), then the filter and LEVEL;
* the last two sleep on their own while the VCA is shut.
.part env in=n67a RTRIG ROHSEL R454 R455 Q74 C135 R452 P23 R447 R448 Q72 R449 R450 Q73 R451 P21 R453
.part vca in=c135 sleep RDAC RRUN R453 X67A C156 R484 Q84A Q84B R485 R490 R489 R481 R488 RL0 RL1 RL2
+ RL3 RL4 RL5 RA9F RA9E RA9D RA9C RA9B RA9A C157 R482 Q85 X67B R483
.part out in=o67b sleep R480 C148 R475 C149 Q80 R476 R477 C150 R479 C151 Q81 R478 C134 R444 X65B P22
+ R521 C508 R522 R523
)cir";

inline constexpr const char *ht = R"cir(* TR-909 HI TOM, voicing board (service notes p.11, grid G-J 18-26; crop p11-ht-hi-tom.png).
* Inputs: trig (HT TRIG from latch IC1), acc (the HT accent DAC, IC6 into RA5), tomnoise.tn (the
* TOM NOISE bus: IC27a's output, tomnoise.cir's output tn, into R387).
* Outputs: out (IC45a's output, connector pin 37 to the multi-jack board), g (Q62's collector, the
* gated accent line, which tomnoise.cir takes as ht.g through D45).
* Controls: tune (VR16 10k B), decay (VR17 500k B, a rheostat), level (VR18 50k B, a rheostat in
* IC45a's feedback).
* Three triangle VCOs (comparator, two 4069UB inverters in parallel run from VDD/VSS, integrator),
* reset by line A for the trigger's 2 ms; VCO1 (C97) and VCO3 (C103) through diode shapers into the
* VCAs Q57 and Q59, VCO2 (C102) into Q58; the VCAs' emitters into IC45a's virtual ground.
.param rlatch=600 rdac=2.54k
* dat: the tantalum capacitors' dielectric absorption (circuit.h: da=): 2 %. Modern MnO2
* tantalum is about 0.5 % (Vishay's dielectric comparison); none is published for the 909's parts.
* It lengthens the toms' far tails a little (docs/circuit/toms.md, "Calibration")
.param dat=0.02
* parts a unit's calibration may set (docs/circuit/toms.md, "Calibration"); the printed values
.param c97=0.015u c102=0.022u c103=0.0082u vr16=10k vr17=500k c98=0.47u c100=0.47u c99=0.15u c106=0.047u
VB15 b15 0 15
VNB15 nb15 0 -15
VTRIG trig 0 input trig
VACC acc 0 input acc
VTN tn 0 input tomnoise.tn
* the TRIG latch's output resistance (HD14174BP on +5 V), and the DAC's source resistance (RA5)
RLATCH trig t {rlatch}
RA5 acc acc2 {rdac}
* trigger: Q61 pulls Q60's base (line A, +15 V for the trigger) and, through D81, Q62's (gated accent)
R345 t q61b 22k
R346 q61b 0 10k
Q61 q61c q61b 0 2SC2603
R344 q60b q61c 22k
R342 b15 q60b 10k
R343 b15 q60e 100
Q60 a q60b q60e 2SA1115
* accent: IC45b, gain 1 + R382/R383 + R382/R381, R381 to -15 V on the - input (+0.68 V offset)
X45B acc2 n45b o45b opamp {gbw} 1e5 {opsat} {sr}
R383 n45b 0 4.7k
R382 n45b o45b 10k
R381 n45b nb15 220k
R347 o45b d81a 100k
D81 d81a q61c 1SS133
R349 d81a q62b 100k
R348 o45b q62e 100
Q62 g q62b q62e 2SA1115
* Between hits the gated accent line floats: in the unit D45 into C54 (tomnoise.cir) takes the
* reverse-biased envelope diodes' leakage and holds it just above C54. tomnoise.cir takes g as a
* stiff source, so g is tied down here through 10 meg (not a part: microamps at most, nothing
* against Q62's drive), which keeps it under C54 at rest.
RGF g 0 10meg
* TUNE: VR16 between R386 (+15 V) and R384; IC42a follows the wiper as VDD (R333 inside its
* loop)
R386 b15 tt 68k
P16 tt tw tb {vr16} tune
R384 tb 0 10k
X42A tw vdd o42a opamp {gbw} 1e5 {opsat} {sr}
R333 o42a vdd 1k
* VSS: IC42b, -(VDD + V(C95) R337/R331 + V(C96) R337/R332), R338 inside its loop. Line A charges
* C95/C96 (the pitch envelope, 220 ms and 22 ms). D72 (RD18JB2, 18 V across VDD-VSS) is left out:
* VDD-VSS never exceeds 11 V.
R335 vdd n42b 100k
R331 c95 n42b 2.2meg
R332 c96 n42b 470k
R337 n42b vss 100k
X42B 0 n42b o42b opamp {gbw} 1e5 {opsat} {sr}
R338 o42b vss 1k
D83 a c95 1SS133
C95 c95 0 0.1u
D82 a c96 1SS133
C96 c96 0 0.047u
* VCO1: comparator IC43a (lightly loaded, it swings +-14 V: opsatlight), IC44a/f
* (pins 1-2, 13-12, TC4069UBP; in parallel, as one N and one P of twice a gate's width with their
* input diodes: models.cir 4069UB_N2/_P2, DCMOS2), integrator IC43b with C97; line A forces the
* comparator low through D73 and turns on Q53 across C97
D73 a d73k 1SS133
R340 d73k n43a 10k
R339 o43b n43a 100k
X43A p43a n43a o43a opamp {gbw} 1e5 {opsatlight} {sr}
R334 o43a p43a 100k
R336 p43a 0 100k
R362 o43a i1 100k
DIC44AH i1 vdd DCMOS2
DIC44AL vss i1 DCMOS2
M44AP y1 i1 vdd 4069UB_P2
M44AN y1 i1 vss 4069UB_N2
R364 y1 n43b 47k
X43B 0 n43b o43b opamp {gbw} 1e5 {opsat} {sr}
C97 n43b o43b {c97}
R363 a q53b 1meg
Q53 q53c q53b n43b 2SC2603
R361 o43b q53c 10k
* VCO1's shaper: R358 into D77/D78, with Q56 from the triangle (R359) across it while C101 (charged by
* line A through D76) holds its base: hard clipping at first, then soft; R357 into R351 and VCA Q57
D76 a c101 1SS133
C101 c101 0 0.0056u
R360 c101 q56b 1meg
R359 o43b q56c 1k
Q56 q56c q56b s1 2SC2603
R358 o43b s1 10k
D77 s1 0 1SS133
D78 0 s1 1SS133
R357 s1 x1 2.2k
R351 x1 0 47
Q57 x1 q57b sum 2SC2603S
* ENV1 (C99 through R354) and ENV2 (C100 through R353) into Q57's base
D74 g c99 1SS133
C99 c99 0 {c99} da={dat}
R354 c99 q57b 470k
R353 c100 q57b 220k
* VCO3: IC47a, IC44b/c (pins 3-4, 5-6), IC47b with C103, Q55
D87 a d87k 1SS133
R390 d87k n47a 10k
R389 o47b n47a 100k
X47A p47a n47a o47a opamp {gbw} 1e5 {opsatlight} {sr}
R372 o47a p47a 100k
R375 p47a 0 100k
R374 o47a i3 100k
DIC44BH i3 vdd DCMOS2
DIC44BL vss i3 DCMOS2
M44BP y3 i3 vdd 4069UB_P2
M44BN y3 i3 vss 4069UB_N2
R366 y3 n47b 47k
X47B 0 n47b o47b opamp {gbw} 1e5 {opsat} {sr}
C103 n47b o47b {c103}
R377 a q55b 1meg
Q55 q55c q55b n47b 2SC2603
R376 o47b q55c 10k
* VCO3's shaper and VCA Q59, with the tom noise through R387; ENV3 (C106 through R378)
R388 o47b s3 10k
D86 s3 0 1SS133
D85 0 s3 1SS133
R380 s3 x3 2.2k
R379 x3 0 47
R387 tn x3 22k
Q59 x3 q59b sum 2SC2603S
D84 g c106 1SS133
C106 c106 0 {c106}
R378 c106 q59b 470k
* VCO2: IC46a, IC44d/e (pins 9-8, 11-10), IC46b with C102, Q54 (its 1M base resistor has no printed
* designator: R365, the one number of the HT's range not used elsewhere; NAVA calls it R329)
D88 a d88k 1SS133
R391 d88k n46a 10k
R373 o46b n46a 100k
X46A p46a n46a o46a opamp {gbw} 1e5 {opsatlight} {sr}
R371 o46a p46a 100k
R369 p46a 0 100k
R370 o46a i2 100k
DIC44DH i2 vdd DCMOS2
DIC44DL vss i2 DCMOS2
M44DP y2 i2 vdd 4069UB_P2
M44DN y2 i2 vss 4069UB_N2
R368 y2 n46b 47k
X46B 0 n46b o46b opamp {gbw} 1e5 {opsat} {sr}
C102 n46b o46b {c102}
R365 a q54b 1meg
Q54 q54c q54b n46b 2SC2603
R367 o46b q54c 10k
* VCO2's shaper and VCA Q58
R356 o46b s2 10k
D79 s2 0 1SS133
D80 0 s2 1SS133
R355 s2 x2 2.2k
R350 x2 0 47
Q58 x2 q58b sum 2SC2603S
* ENV2: D75 charges C98, which DECAY (VR17 + R385) runs down and which fills C100 through R341;
* C100 feeds Q58 (R352) and Q57 (R353)
D75 g c98 1SS133
C98 c98 0 {c98} da={dat}
P17 r385 r385 c98 {vr17} decay
R385 r385 0 56k
R341 c98 c100 100k
C100 c100 0 {c100} da={dat}
R352 c100 q58b 220k
* IC45a sums the three VCAs; LEVEL (VR18) is its feedback
X45A 0 sum o45a opamp {gbw} 1e5 {opsat} {sr}
P18 o45a o45a sum 50k level
* the multi-jack board's load (p.12; mix.cir runs it): R512 and C505 (SN 415300 up) to the jack,
* whose normal contact takes R513/R514 to the mix buses (AC ground)
R512 o45a j 1k
C505 j 0 0.01u
R513 j 0 15k
R514 j 0 8.2k
.control tune 0.5
.control decay 0.5
.control level 0.5
.output out o45a
.output g g
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
* Line A, VDD and VSS are cut where they are stiff (Q60 for the trigger, IC42's outputs); each VCO's
* reset diode and comparator input (to the triangle) are in the trigger partition too, so line A
* floats between hits and falls at the trigger's end as it does whole. The shapers are cut from the
* VCOs at their resistors from the triangles (op-amp outputs), and from the VCAs at their 2.2k into
* the 47 ohm nodes (R357, R380, R355): each shaper a partition of its own, the VCAs and the output
* op-amp another. On a trigger edge every partition is to be substepped (docs/circuit/toms.md, §8).
.part trig in=o43b,o47b,o46b,q57b,q58b,q59b RLATCH R345 R346 Q61 R344 R342 R343 Q60 RA5 X45B R383 R382 R381 R347 D81 R349 R348 Q62 RGF R386 P16 R384 X42A R333 R335 R331 R332 R337 X42B R338 D83 C95 D82 C96 D73 R340 R339 D87 R390 R389 D88 R391 R373 D74 C99 D84 C106 D75 C98 P17 R385 R341 C100 R354 R353 R378 R352
.part vco1 in=a,vdd,vss,s1,q56c D73 R340 R339 X43A R334 R336 R362 DIC44AH DIC44AL M44AP M44AN R364 X43B C97 R363 Q53 R361 R359 R358
.part vco3 in=a,vdd,vss,s3 D87 R390 R389 X47A R372 R375 R374 DIC44BH DIC44BL M44BP M44BN R366 X47B C103 R377 Q55 R376 R388
.part vco2 in=a,vdd,vss,s2 D88 R391 R373 X46A R371 R369 R370 DIC44DH DIC44DL M44DP M44DN R368 X46B C102 R365 Q54 R367 R356
.part shape1 in=o43b,a,x1 D76 C101 R360 R359 Q56 R358 D77 D78 R357
.part shape3 in=o47b,x3 R388 D86 D85 R380
.part shape2 in=o46b,x2 R356 D79 D80 R355
.part vca in=s1,s3,s2,c99,c100,c106 R357 R351 Q57 R354 R353 R380 R379 R387 Q59 R378 R355 R350 Q58 R352 X45A P18 R512 C505 R513 R514
)cir";

inline constexpr const char *lt = R"cir(* TR-909 LOW TOM, voicing board (service notes p.11, grid O-R 18-26; crop p11-lt-low-tom.png).
* Inputs: trig (LT TRIG from latch IC1), acc (the LT accent DAC, IC4 into RA3), tomnoise.tn (the
* TOM NOISE bus: IC27a's output, tomnoise.cir's output tn, into R114).
* Outputs: out (IC18a's output, connector pin 35 to the multi-jack board), g (Q17's collector, the
* gated accent line, which tomnoise.cir takes as lt.g through D26).
* Controls: tune (VR10 10k B), decay (VR11 500k B, a rheostat), level (VR12 50k B, a rheostat in
* IC18a's feedback).
* Three triangle VCOs (comparator, two 4069UB inverters in parallel run from VDD/VSS, integrator),
* reset by line A for the trigger's 2 ms; VCO1 (C18) and VCO3 (C20) through diode shapers into the
* VCAs Q19 and Q21, VCO2 (C19) into Q20; the VCAs' emitters into IC18a's virtual ground.
.param rlatch=600 rdac=2.54k
* dat: the tantalum capacitors' dielectric absorption (circuit.h: da=): 2 %. Modern MnO2
* tantalum is about 0.5 % (Vishay's dielectric comparison); none is published for the 909's parts.
* It lengthens the toms' far tails a little (docs/circuit/toms.md, "Calibration")
.param dat=0.02
* parts a unit's calibration may set (docs/circuit/toms.md, "Calibration"); the printed values
.param c18=0.022u c19=0.033u c20=0.012u vr10=10k vr11=500k c21=0.68u c23=0.68u c22=0.22u c25=0.056u
VB15 b15 0 15
VNB15 nb15 0 -15
VTRIG trig 0 input trig
VACC acc 0 input acc
VTN tn 0 input tomnoise.tn
* the TRIG latch's output resistance (HD14174BP on +5 V), and the DAC's source resistance (RA3)
RLATCH trig t {rlatch}
RA3 acc acc2 {rdac}
* trigger: Q16 pulls Q15's base (line A, +15 V for the trigger) and, through D18, Q17's (gated accent)
R93 t q16b 22k
R94 q16b 0 10k
Q16 q16c q16b 0 2SC2603
R92 q15b q16c 22k
R90 b15 q15b 10k
R91 b15 q15e 100
Q15 a q15b q15e 2SA1115
* accent: IC18b, gain 1 + R119/R120 + R119/R118, R118 to -15 V on the - input (+0.68 V offset)
X18B acc2 n18b o18b opamp {gbw} 1e5 {opsat} {sr}
R120 n18b 0 4.7k
R119 n18b o18b 10k
R118 n18b nb15 220k
R95 o18b d18a 100k
D18 d18a q16c 1SS133
R97 d18a q17b 100k
R96 o18b q17e 100
Q17 g q17b q17e 2SA1115
* Between hits the gated accent line floats: in the unit D26 into C54 (tomnoise.cir) takes the
* reverse-biased envelope diodes' leakage and holds it just above C54. tomnoise.cir takes g as a
* stiff source, so g is tied down here through 10 meg (not a part: microamps at most, nothing
* against Q17's drive), which keeps it under C54 at rest.
RGF g 0 10meg
* TUNE: VR10 between R113 (+15 V) and R112; IC14a follows the wiper as VDD (R68 inside its
* loop)
R113 b15 tt 68k
P10 tt tw tb {vr10} tune
R112 tb 0 10k
X14A tw vdd o14a opamp {gbw} 1e5 {opsat} {sr}
R68 o14a vdd 1k
* VSS: IC14b, -(VDD + V(C16) R64/R60 + V(C17) R64/R61), R65 inside its loop. Line A charges
* C16/C17 (the pitch envelope, 220 ms and 22 ms). D16 (RD18JB2, 18 V across VDD-VSS) is left out:
* VDD-VSS never exceeds 11 V.
R66 vdd n14b 100k
R60 c16 n14b 2.2meg
R61 c17 n14b 470k
R64 n14b vss 100k
X14B 0 n14b o14b opamp {gbw} 1e5 {opsat} {sr}
R65 o14b vss 1k
D15 a c16 1SS133
C16 c16 0 0.1u
D14 a c17 1SS133
C17 c17 0 0.047u
* VCO1: comparator IC15a (lightly loaded, it swings +-14 V: opsatlight), IC16a/f
* (pins 1-2, 13-12, TC4069UBP; in parallel, as one N and one P of twice a gate's width with their
* input diodes: models.cir 4069UB_N2/_P2, DCMOS2), integrator IC15b with C18; line A forces the
* comparator low through D13 and turns on Q13 across C18
D13 a d13k 1SS133
R62 d13k n15a 10k
R63 o15b n15a 100k
X15A p15a n15a o15a opamp {gbw} 1e5 {opsatlight} {sr}
R69 o15a p15a 100k
R67 p15a 0 100k
R70 o15a i1 100k
DIC16AH i1 vdd DCMOS2
DIC16AL vss i1 DCMOS2
M16AP y1 i1 vdd 4069UB_P2
M16AN y1 i1 vss 4069UB_N2
R73 y1 n15b 47k
X15B 0 n15b o15b opamp {gbw} 1e5 {opsat} {sr}
C18 n15b o15b {c18}
R72 a q13b 1meg
Q13 q13c q13b n15b 2SC2603
R71 o15b q13c 10k
* VCO1's shaper: R103 into D20/D21, with Q18 from the triangle (R102) across it while C24 (charged by
* line A through D17) holds its base: hard clipping at first, then soft; R104 into R99 and VCA Q19
D17 a c24 1SS133
C24 c24 0 0.01u
R101 c24 q18b 1meg
R102 o15b q18c 1k
Q18 q18c q18b s1 2SC2603
R103 o15b s1 10k
D20 s1 0 1SS133
D21 0 s1 1SS133
R104 s1 x1 2.2k
R99 x1 0 47
Q19 x1 q19b sum 2SC2603S
* ENV1 (C22 through R107) and ENV2 (C23 through R108) into Q19's base
D24 g c22 1SS133
C22 c22 0 {c22} da={dat}
R107 c22 q19b 470k
R108 c23 q19b 220k
* VCO3: IC19a, IC16b/c (pins 3-4, 5-6), IC19b with C20, Q22
D32 a d32k 1SS133
R88 d32k n19a 10k
R87 o19b n19a 100k
X19A p19a n19a o19a opamp {gbw} 1e5 {opsatlight} {sr}
R86 o19a p19a 100k
R85 p19a 0 100k
R83 o19a i3 100k
DIC16BH i3 vdd DCMOS2
DIC16BL vss i3 DCMOS2
M16BP y3 i3 vdd 4069UB_P2
M16BN y3 i3 vss 4069UB_N2
R75 y3 n19b 47k
X19B 0 n19b o19b opamp {gbw} 1e5 {opsat} {sr}
C20 n19b o19b {c20}
R76 a q22b 1meg
Q22 q22c q22b n19b 2SC2603
R84 o19b q22c 10k
* VCO3's shaper and VCA Q21, with the tom noise through R114; ENV3 (C25 through R110)
R117 o19b s3 10k
D28 s3 0 1SS133
D27 0 s3 1SS133
R116 s3 x3 2.2k
R115 x3 0 47
R114 tn x3 22k
Q21 x3 q21b sum 2SC2603S
D25 g c25 1SS133
C25 c25 0 {c25}
R110 c25 q21b 470k
* VCO2: IC17a, IC16d/e (pins 9-8, 11-10), IC17b with C19, Q14
D33 a d33k 1SS133
R89 d33k n17a 10k
R82 o17b n17a 100k
X17A p17a n17a o17a opamp {gbw} 1e5 {opsatlight} {sr}
R81 o17a p17a 100k
R79 p17a 0 100k
R80 o17a i2 100k
DIC16DH i2 vdd DCMOS2
DIC16DL vss i2 DCMOS2
M16DP y2 i2 vdd 4069UB_P2
M16DN y2 i2 vss 4069UB_N2
R78 y2 n17b 47k
X17B 0 n17b o17b opamp {gbw} 1e5 {opsat} {sr}
C19 n17b o17b {c19}
R74 a q14b 1meg
Q14 q14c q14b n17b 2SC2603
R77 o17b q14c 10k
* VCO2's shaper and VCA Q20
R105 o17b s2 10k
D22 s2 0 1SS133
D23 0 s2 1SS133
R106 s2 x2 2.2k
R98 x2 0 47
Q20 x2 q20b sum 2SC2603S
* ENV2: D19 charges C21, which DECAY (VR11 + R111) runs down and which fills C23 through R100;
* C23 feeds Q20 (R109) and Q19 (R108)
D19 g c21 1SS133
C21 c21 0 {c21} da={dat}
P11 r111 r111 c21 {vr11} decay
R111 r111 0 56k
R100 c21 c23 100k
C23 c23 0 {c23} da={dat}
R109 c23 q20b 220k
* IC18a sums the three VCAs; LEVEL (VR12) is its feedback
X18A 0 sum o18a opamp {gbw} 1e5 {opsat} {sr}
P12 o18a o18a sum 50k level
* the multi-jack board's load (p.12; mix.cir runs it): R506 and C503 (SN 415300 up) to the jack,
* whose normal contact takes R507/R508 to the mix buses (AC ground)
R506 o18a j 1k
C503 j 0 0.01u
R507 j 0 8.2k
R508 j 0 15k
.control tune 0.5
.control decay 0.5
.control level 0.5
.output out o18a
.output g g
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
* Line A, VDD and VSS are cut where they are stiff (Q15 for the trigger, IC14's outputs); each VCO's
* reset diode and comparator input (to the triangle) are in the trigger partition too, so line A
* floats between hits and falls at the trigger's end as it does whole. The shapers are cut from the
* VCOs at their resistors from the triangles (op-amp outputs), and from the VCAs at their 2.2k into
* the 47 ohm nodes (R104, R116, R106): each shaper a partition of its own, the VCAs and the output
* op-amp another. On a trigger edge every partition is to be substepped (docs/circuit/toms.md, §8).
.part trig in=o15b,o19b,o17b,q19b,q20b,q21b RLATCH R93 R94 Q16 R92 R90 R91 Q15 RA3 X18B R120 R119 R118 R95 D18 R97 R96 Q17 RGF R113 P10 R112 X14A R68 R66 R60 R61 R64 X14B R65 D15 C16 D14 C17 D13 R62 R63 D32 R88 R87 D33 R89 R82 D24 C22 D25 C25 D19 C21 P11 R111 R100 C23 R107 R108 R110 R109
.part vco1 in=a,vdd,vss,s1,q18c D13 R62 R63 X15A R69 R67 R70 DIC16AH DIC16AL M16AP M16AN R73 X15B C18 R72 Q13 R71 R102 R103
.part vco3 in=a,vdd,vss,s3 D32 R88 R87 X19A R86 R85 R83 DIC16BH DIC16BL M16BP M16BN R75 X19B C20 R76 Q22 R84 R117
.part vco2 in=a,vdd,vss,s2 D33 R89 R82 X17A R81 R79 R80 DIC16DH DIC16DL M16DP M16DN R78 X17B C19 R74 Q14 R77 R105
.part shape1 in=o15b,a,x1 D17 C24 R101 R102 Q18 R103 D20 D21 R104
.part shape3 in=o19b,x3 R117 D28 D27 R116
.part shape2 in=o17b,x2 R105 D22 D23 R106
.part vca in=s1,s3,s2,c22,c23,c25 R104 R99 Q19 R107 R108 R116 R115 R114 Q21 R110 R106 R98 Q20 R109 X18A P12 R506 C503 R507 R508
)cir";

inline constexpr const char *mix = R"cir(* TR-909 mixer and outputs: the multi-jack board JK-1 73133100 (service notes p.12, crop
* p12-multi-jack-board-schematic.png) and the master section of the voicing board (p.11 grid A-B
* 22-26, crop p11-master-volume-mix-out.png). Transcription notes: docs/circuit/mix.md.
* Inputs: each voice's output at the voicing board's CN-5 (pins 33-42, W5), the voice netlists'
* out: bd sd lt mt ht rs hc hh cr rd; pon, the +15 V rail at C143's positive end (+15 V while
* running; see "power-on mute" below).
* Each voice: R5xx 1k into its MULTI OUT jack (C5xx 0.01u to ground from SN 415300); the jack's
* switching contact, closed while nothing is plugged in, feeds the voice's two mix resistors.
* Plugging a jack takes the voice out of the mix and loads it with the cable's input (RX, rmulti).
* The mix buses (pins 45 and 43) go through C153 / C154 into IC66b / IC66a, inverting summers with
* MASTER VOLUME VR28 (50k B dual, each half a rheostat) as feedback; then R474 / R473 1k and C155 /
* C152 10u to pins 47 / 48, and on JK-1 R530 / R532 2.2k, C511 / C512 0.01u, R531 / R533 10k to the
* Left and Right/MONO jacks. Pin 45 is the voicing board's "MIX IN (L)" (JK-1 prints "RMIX") and
* reaches the Left jack through IC66b: the upper resistor of each pair (R501, R504, ...) is the
* left one. The Left jack's switching contact joins the two outputs while it is empty (mono at
* Right/MONO).
* Outputs: l, r (the Left and Right/MONO jacks, each loaded by rload); mbd ... mrd (each MULTI OUT
* jack); busl, busr (the mix buses, probes).
* Controls: volume (VR28, 50k B; 0 = no feedback, no output); plug_bd ... plug_rd (1 = a cable in
* that MULTI OUT jack; default 0); plugl (1 = a cable in the Left jack: stereo; default 1).
* Power-on mute: Q82 / Q83 (2SC2878) short pins 48 / 47 to ground through their bases' drive from
* C143: while C143 is discharged (power-on) its negative end sits near +15 V and D194, R469 / R470
* saturate both; R472 470k and the bases' current charge it (muted for about 1.8 s; afterwards the
* reverse-conducting 2SC2878s hold the outputs' negative peaks above C143's negative end - 1.2 V
* until it has charged: docs/circuit/mix.md). To start muted, solve the DC point with pon = -15 V
* (C143 then holds 0 V) and step it to +15 V. Q79, C142, D193 and R468 (the mute at power-off, when
* the +15 V rail falls) are not here.
* Board: the late one (C500, C502-C512 from SN 415300); early units by .param cjack=1p.
.param cjack=0.01u rmulti=100k rload=100k
VB15 b15 0 15
VNB15 nb15 0 -15
VPON pon 0 input pon
VBD bd 0 input bd
VSD sd 0 input sd
VLT lt 0 input lt
VMT mt 0 input mt
VHT ht 0 input ht
VRS rs 0 input rs
VHC hc 0 input hc
VHH hh 0 input hh
VCR cr 0 input cr
VRD rd 0 input rd
* --- JK-1: per voice the series 1k, the 0.01u, the jack (S: its switching contact; SX and RX: a
* cable's input while plugged), and the two mix resistors (left bus first)
R500 bd mbd 1k
C500 mbd 0 {cjack}
SJ1 mbd nbd !plug_bd
SX1 mbd xbd plug_bd
RX1 xbd 0 {rmulti}
R501 nbd busl 12k
R502 nbd busr 12k
R503 sd msd 1k
C502 msd 0 {cjack}
SJ2 msd nsd !plug_sd
SX2 msd xsd plug_sd
RX2 xsd 0 {rmulti}
R504 nsd busl 12k
R505 nsd busr 12k
R506 lt mlt 1k
C503 mlt 0 {cjack}
SJ3 mlt nlt !plug_lt
SX3 mlt xlt plug_lt
RX3 xlt 0 {rmulti}
R507 nlt busl 8.2k
R508 nlt busr 15k
R509 mt mmt 1k
C504 mmt 0 {cjack}
SJ4 mmt nmt !plug_mt
SX4 mmt xmt plug_mt
RX4 xmt 0 {rmulti}
R510 nmt busl 12k
R511 nmt busr 12k
R512 ht mht 1k
C505 mht 0 {cjack}
SJ5 mht nht !plug_ht
SX5 mht xht plug_ht
RX5 xht 0 {rmulti}
R513 nht busl 15k
R514 nht busr 8.2k
R515 rs mrs 1k
C506 mrs 0 {cjack}
SJ6 mrs nrs !plug_rs
SX6 mrs xrs plug_rs
RX6 xrs 0 {rmulti}
R516 nrs busl 12k
R517 nrs busr 12k
R518 hc mhc 1k
C507 mhc 0 {cjack}
SJ7 mhc nhc !plug_hc
SX7 mhc xhc plug_hc
RX7 xhc 0 {rmulti}
R519 nhc busl 12k
R520 nhc busr 12k
R521 hh mhh 1k
C508 mhh 0 {cjack}
SJ8 mhh nhh !plug_hh
SX8 mhh xhh plug_hh
RX8 xhh 0 {rmulti}
R522 nhh busl 15k
R523 nhh busr 8.2k
R524 cr mcr 1k
C509 mcr 0 {cjack}
SJ9 mcr ncr !plug_cr
SX9 mcr xcr plug_cr
RX9 xcr 0 {rmulti}
R525 ncr busl 12k
R526 ncr busr 10k
R527 rd mrd 1k
C510 mrd 0 {cjack}
SJ10 mrd nrd !plug_rd
SX10 mrd xrd plug_rd
RX10 xrd 0 {rmulti}
R528 nrd busl 8.2k
R529 nrd busr 15k
* --- master section, voicing board: IC66b left (pin 45 in, pin 47 out), IC66a right (43, 48).
* The summers clip at opsat but don't slew: a full-swing 20 kHz output needs 1.6 V/us, under the
* M5218's 2.2, and the slewing input stage would keep the solver out of its linear mode.
C153 busl n66b 1u
X66B 0 n66b o66b opamp {gbw} 1e5 {opsat}
P28B o66b o66b n66b 50k volume
R474 o66b c155a 1k
C155 c155a lout 10u
C154 busr n66a 1u
X66A 0 n66a o66a opamp {gbw} 1e5 {opsat}
P28A o66a o66a n66a 50k volume
R473 o66a c152a 1k
C152 c152a rout 10u
* --- power-on mute: C143 from the +15 V rail, R472 to -15 V, D195 and R471 (C143's discharge at
* power-off), D194 into R469 / R470 and the bases of Q82 (pin 48) and Q83 (pin 47)
C143 pon m1 47u
R472 m1 nb15 470k
D195 d195a m1 1SS133
R471 d195a nb15 4.7k
D194 m1 m2 1SS133
* the leakage of D194 and Q79 (not on the schematic: the bases' only DC path while the mute is off,
* which holds them near -15 V)
RLK m2 m1 100meg
R469 m2 q82b 22k
R470 m2 q83b 22k
Q82 rout q82b 0 2SC2878
Q83 lout q83b 0 2SC2878
* --- JK-1 master outputs; the Left jack's switching contact joins l to r while it is empty
R530 lout l 2.2k
C511 l 0 {cjack}
R531 l 0 10k
R532 rout r 2.2k
C512 r 0 {cjack}
R533 r 0 10k
SJL l r !plugl
* the inputs the master outputs go to (not on the schematic)
RLL l 0 {rload}
RLR r 0 {rload}
.control volume 0.5
.control plug_bd 0
.control plug_sd 0
.control plug_lt 0
.control plug_mt 0
.control plug_ht 0
.control plug_rs 0
.control plug_hc 0
.control plug_hh 0
.control plug_cr 0
.control plug_rd 0
.control plugl 1
.output l l
.output r r
.output mbd mbd
.output msd msd
.output mlt mlt
.output mmt mmt
.output mht mht
.output mrs mrs
.output mhc mhc
.output mhh mhh
.output mcr mcr
.output mrd mrd
.output busl busl
.output busr busr
)cir";

inline constexpr const char *models = R"cir(* Semiconductor models for the TR-909 netlists (DC equations only: the solver has no charge
* storage). Parts from the voicing board schematic (service notes p.11: "All NPN 2SC2603-F, All PNP
* 2SA1115-F, All DIODE 1SS133", Q82/Q83 2SC2878, Q47/Q48 2SD1469, Q68/Q70/Q84 2SA798) and the
* parts list (p.2-3: 2SC2603-28-F, 2SA1115-28-F, 2SC2878-A or B, 2SD1469-R, 2SA798-G). Each from
* its data sheet (private/docs/datasheets/); the reasoning: docs/circuit/semiconductors.md.
* 2SC2603 (Mitsubishi; Micro Electronics' second-source sheet): rank F h_FE 250-500 at 1 mA, 6 V;
* BF 350 gives 345 there (VAF 110, ISE), the rank's geometric middle. No V_BE, BR or Early figure on
* the sheet: IS 1.1e-14 (0.65 V at 1 mA), BR 6, VAF 110 are typical small-signal values; ISE puts
* h_FE at 0.1 mA at 0.93 of that at 1 mA (the sheet: min 50 at 0.1 mA, min 90 at 1 mA).
.model 2SC2603 NPN(IS=1.1e-14 BF=350 BR=6 NF=1 NR=1 VAF=110 ISE=1e-14 NE=1.5)
* 2SC2603S: the "(S)" (selected) 2SC2603s of p.11, every single-transistor VCA (Q6, Q12, Q19-Q21,
* Q26-Q28, Q33, Q37, Q50, Q51, Q57-Q59, Q65, Q69, Q71, Q85): saturated, their gain about
* BR I_B / V_T, so BR is the one parameter that matters (Roland's MKS-30 sheet bins the same VCA's
* 2SC945s by this gain, "Gm selected", one bin a board). BR 6.5 fitted jointly to the AudioRealism
* unit: BD, toms, hats, cymbals within 1.1 dB of its levels against the snare's snappy (2SD1469s)
* and its clap (whose tail fits with TM at about 0.31); flat from 2 to 18 uA (the cymbals' VCA law:
* no ISC). No sheet gives the 2SC2603's reverse h_FE. docs/circuit/s-transistors.md.
.model 2SC2603S NPN(IS=1.1e-14 BF=350 BR=6.5 NF=1 NR=1 VAF=110 ISE=1e-14 NE=1.5)
* 2SA1115 (Mitsubishi): rank F h_FE 250-500 at -1 mA, -6 V: BF 370 gives 348 (VAF 90, ISE). As the
* 2SC2603, the sheet has no V_BE, BR or Early figure.
.model 2SA1115 PNP(IS=1.2e-14 BF=370 BR=6 NF=1 NR=1 VAF=90 ISE=2e-14 NE=1.5)
* 2SC2878 (Toshiba, for muting): V_BE 0.61 V typ at 4 mA, 2 V gives IS 2.2e-13; h_FE A 200-700,
* B 350-1200 (the parts list allows either) at 2 V, 4 mA: BF 480; reverse h_FE 150 typ (BR); V_EBO
* 25 V min (no breakdown in the model, none needed).
.model 2SC2878 NPN(IS=2.2e-13 BF=480 BR=150 NF=1 NR=1 VAF=100 ISE=1e-14 NE=1.5)
* 2SD1469 (Rohm, medium power, 1 A): rank R h_FE 180-390 at 3 V, 100 mA: BF 260; V_CE(sat) about
* 6 mV at 1 mA / 0.1 mA (the sheet's features) gives BR 50. No V_BE on the sheet: IS 5e-14, between
* the small-signal parts and the 2SC2878, an estimate.
.model 2SD1469 NPN(IS=5e-14 BF=260 BR=50 NF=1 NR=1 VAF=100 ISE=1e-14 NE=1.5)
* 2SA798 (Mitsubishi dual PNP): V_BE -0.63 V at -1 mA, -6 V (the I_C-V_BE curve, p.2) gives IS
* 2.2e-14; rank G h_FE 400-800 at -1 mA, -6 V: BF 480 with VAF 35 (h_oe 30 uS at 1 mA, 6 V and the
* I_C-V_CE curves: 27-50 V); h_FE flat from -10 uA to -3 mA (p.3): no ISE. BR not on the sheet.
.model 2SA798 PNP(IS=2.2e-14 BF=480 BR=6 NF=1 NR=1 VAF=35)
* 1SS133 (Rohm): fitted to the 25 C forward curve (Fig. 1, 0.2-95 mA, within 12 mV): 0.60 V at
* 1 mA, 0.71 V at 10 mA, 0.94 V at 100 mA (the sheet: 1.2 V max at 100 mA).
.model 1SS133 D(IS=1.45n N=1.71 RS=1.44)
* CMOS input protection diodes (DCMOS: one diode to each rail, at an input; the 4011UB's and
* 4069UB's).
.model DCMOS D(IS=1e-14 N=1 RS=10)
* CMOS (docs/circuit/solver-mosfet.md, section 5): SPICE level-1 MOSFETs, one inverter an N and a
* P, without charge storage. 4069UB (HD14069UBP, IC16/IC22/IC36/IC44): fitted to the HD14069UB
* typical table (25 C; private/docs/datasheets/hitachi-hd14069ub.pdf p.2, the same numbers as
* Motorola's MC14069UB), with the threshold and the curves' shape from TI's CD4069UB typical
* curves (ti-cd4069ub.pdf p.8 Fig. 3, 4, 6). 4069UB_N/_P for supplies of 3-10 V, as the VCO
* switches' +-V_cv rails: I_OL 0.81 mA at 5 V/0.4 V (sheet 0.88), 2.40 mA at 10 V/0.5 V (2.25);
* I_OH 4.03 mA at 5 V/2.5 V (4.2), 0.84 mA at 5 V/4.6 V (0.88), 2.44 mA at 10 V/9.5 V (2.25);
* into 50 pF at 5 V t_PHL 34 ns (sheet 65, of which ~20 ns is the gate's own: MC's formula
* 0.90 ns/pF CL + 20 ns), t_THL 67 ns (100); switching point 2.56 V; peak supply current 4.8 mA at
* 10 V (TI Fig. 3: 4.6 mA). 4069UB15_N/_P for 15 V: I_OL 8.76 mA at 15 V/1.5 V (8.8), I_OH
* 8.83 mA at 15 V/13.5 V (8.8), transfer gain 5 at 15 V as TI's soft 15 V curve; 35 % weak at 5 V.
* W and L are only a plausible split of beta = KP/2 W/L (metal gate, L 6 um).
.model 4069UB_N NMOS(VTO=1.15 KP=20u LAMBDA=0.03 W=165u L=6u)
.model 4069UB_P PMOS(VTO=-1.03 KP=8u LAMBDA=0.03 W=413u L=6u)
.model 4069UB15_N NMOS(VTO=1.32 KP=20u LAMBDA=0.106 W=117u L=6u)
.model 4069UB15_P PMOS(VTO=-1.15 KP=8u LAMBDA=0.106 W=291u L=6u)
* Three 4069UB inverters in parallel (the snare's IC36 runs three per VCO, inputs and outputs
* tied): 4069UB_N/_P with W x3, and their input diodes, three DCMOS each way, as one (IS x3, RS /3).
.model 4069UB_N3 NMOS(VTO=1.15 KP=20u LAMBDA=0.03 W=495u L=6u)
.model 4069UB_P3 PMOS(VTO=-1.03 KP=8u LAMBDA=0.03 W=1239u L=6u)
.model DCMOS3 D(IS=3e-14 N=1 RS=3.33)
* Two in parallel (the toms' IC16/IC22/IC44 run two per VCO): W x2, and DCMOS2 (IS x2, RS /2).
.model 4069UB_N2 NMOS(VTO=1.15 KP=20u LAMBDA=0.03 W=330u L=6u)
.model 4069UB_P2 PMOS(VTO=-1.03 KP=8u LAMBDA=0.03 W=826u L=6u)
.model DCMOS2 D(IS=2e-14 N=1 RS=5)
* 4011UB (TC4011UBP: IC57, IC59, the crash's and ride's clock oscillators, on +5 V; IC72): a gate
* is two of each, the P in parallel, the N in series. The TC4011UBP sheet (Toshiba 1985) gives
* only minimum drive, the MC14011UB's (Motorola 1988) and the CD4011UB's (Harris/TI) typicals differ
* threefold, and Motorola's stops the cymbals' clock from TUNE 0.9 up. Fitted, LAMBDA 0.05, to the
* CD4011UB's typical 5 V drive: source -1 mA at 4.6 V, -3.2 mA at 2.5 V (the P pair; the fit:
* -0.84, -3.6), sink 1 mA at 0.4 V (the N pair in series), with the threshold at 2.5 V for both
* inputs together and 2.35 V for one (the other high): beta 0.90 / 0.187 mA/V2. Checked on the
* hi-hat's fixed astable (R492 12k, R493 33k, C160 470p): 31.8 kHz against the 31.57 measured.
* private/docs/datasheets/; docs/circuit/cymbals.md, "The clock". w4011n / w4011p: the channels'
* width, a voice's .param (its gates' drive: docs/circuit/cymbals.md, "Calibration").
.param w4011n=540u w4011p=280.5u
.model 4011UB_N NMOS(VTO=2.06 KP=20u LAMBDA=0.05 W={w4011n} L=6u)
.model 4011UB_P PMOS(VTO=-2.04 KP=8u LAMBDA=0.05 W={w4011p} L=6u)
* Op-amps: M5218L, single pole. Mitsubishi 1984 data book (M5218/M5R4558P, Vcc +-15 V): gain-
* bandwidth 7 MHz typ, slew rate 2.2 V/us typ (the later M5218A: 3.0), maximum output +-14 V typ
* into 10k, +-13 V into 2k (+-12 / +-10 min).
.param gbw=7meg
* Op-amp outputs clip at +-13.5 V.
.param opsat=13.5
* An op-amp loaded by 10k or more (the toms' VCO comparators: about 50k) swings +-14 V (the data
* book's typical into 10k): their thresholds, and the toms' pitch, hang on it (docs/circuit/toms.md).
.param opsatlight=14
* Slew rate, V/us.
.param sr=2.2
)cir";

inline constexpr const char *mt = R"cir(* TR-909 MID TOM, voicing board (service notes p.11, grid K-N 18-26; crop p11-mt-mid-tom.png).
* Inputs: trig (MT TRIG from latch IC1), acc (the MT accent DAC, IC5 into RA4), tomnoise.tn (the
* TOM NOISE bus: IC27a's output, tomnoise.cir's output tn, into R177).
* Outputs: out (IC23a's output, connector pin 36 to the multi-jack board), g (Q29's collector, the
* gated accent line, which tomnoise.cir takes as mt.g through D44).
* Controls: tune (VR13 10k B), decay (VR14 500k B, a rheostat), level (VR15 50k B, a rheostat in
* IC23a's feedback).
* Three triangle VCOs (comparator, two 4069UB inverters in parallel run from VDD/VSS, integrator),
* reset by line A for the trigger's 2 ms; VCO1 (C32) and VCO3 (C34) through diode shapers into the
* VCAs Q26 and Q28, VCO2 (C33) into Q27; the VCAs' emitters into IC23a's virtual ground.
.param rlatch=600 rdac=2.54k
* dat: the tantalum capacitors' dielectric absorption (circuit.h: da=): 2 %. Modern MnO2
* tantalum is about 0.5 % (Vishay's dielectric comparison); none is published for the 909's parts.
* It lengthens the toms' far tails a little (docs/circuit/toms.md, "Calibration")
.param dat=0.02
* parts a unit's calibration may set (docs/circuit/toms.md, "Calibration"); the printed values
.param c32=0.018u c33=0.027u c34=0.01u vr13=10k vr14=500k c35=0.47u c37=0.47u c36=0.15u c39=0.056u
VB15 b15 0 15
VNB15 nb15 0 -15
VTRIG trig 0 input trig
VACC acc 0 input acc
VTN tn 0 input tomnoise.tn
* the TRIG latch's output resistance (HD14174BP on +5 V), and the DAC's source resistance (RA4)
RLATCH trig t {rlatch}
RA4 acc acc2 {rdac}
* trigger: Q25 pulls Q24's base (line A, +15 V for the trigger) and, through D42, Q29's (gated accent)
R154 t q25b 22k
R155 q25b 0 10k
Q25 q25c q25b 0 2SC2603
R153 q24b q25c 22k
R151 b15 q24b 10k
R152 b15 q24e 100
Q24 a q24b q24e 2SA1115
* accent: IC23b, gain 1 + R179/R180 + R179/R178, R178 to -15 V on the - input (+0.68 V offset)
X23B acc2 n23b o23b opamp {gbw} 1e5 {opsat} {sr}
R180 n23b 0 4.7k
R179 n23b o23b 10k
R178 n23b nb15 220k
R156 o23b d42a 100k
D42 d42a q25c 1SS133
R158 d42a q29b 100k
R157 o23b q29e 100
Q29 g q29b q29e 2SA1115
* Between hits the gated accent line floats: in the unit D44 into C54 (tomnoise.cir) takes the
* reverse-biased envelope diodes' leakage and holds it just above C54. tomnoise.cir takes g as a
* stiff source, so g is tied down here through 10 meg (not a part: microamps at most, nothing
* against Q29's drive), which keeps it under C54 at rest.
RGF g 0 10meg
* TUNE: VR13 between R172 (+15 V) and R173; IC20a follows the wiper as VDD (R129 inside its loop;
* printed "R12a")
R172 b15 tt 68k
P13 tt tw tb {vr13} tune
R173 tb 0 10k
X20A tw vdd o20a opamp {gbw} 1e5 {opsat} {sr}
R129 o20a vdd 1k
* VSS: IC20b, -(VDD + V(C30) R125/R121 + V(C31) R125/R122), R126 inside its loop. Line A charges
* C30/C31 (the pitch envelope, 220 ms and 22 ms). D34 (RD18JB2, 18 V across VDD-VSS) is left out:
* VDD-VSS never exceeds 11 V.
R127 vdd n20b 100k
R121 c30 n20b 2.2meg
R122 c31 n20b 470k
R125 n20b vss 100k
X20B 0 n20b o20b opamp {gbw} 1e5 {opsat} {sr}
R126 o20b vss 1k
D37 a c30 1SS133
C30 c30 0 0.1u
D36 a c31 1SS133
C31 c31 0 0.047u
* VCO1: comparator IC21a (R130 printed "R80"; lightly loaded, it swings +-14 V: opsatlight), IC22a/f
* (pins 1-2, 13-12, TC4069UBP; in parallel, as one N and one P of twice a gate's width with their
* input diodes: models.cir 4069UB_N2/_P2, DCMOS2), integrator IC21b with C32; line A forces the
* comparator low through D35 and turns on Q30 across C32
D35 a d35k 1SS133
R123 d35k n21a 10k
R124 o21b n21a 100k
X21A p21a n21a o21a opamp {gbw} 1e5 {opsatlight} {sr}
R130 o21a p21a 100k
R128 p21a 0 100k
R131 o21a i1 100k
DIC22AH i1 vdd DCMOS2
DIC22AL vss i1 DCMOS2
M22AP y1 i1 vdd 4069UB_P2
M22AN y1 i1 vss 4069UB_N2
R134 y1 n21b 47k
X21B 0 n21b o21b opamp {gbw} 1e5 {opsat} {sr}
C32 n21b o21b {c32}
R133 a q30b 1meg
Q30 q30c q30b n21b 2SC2603
R132 o21b q30c 10k
* VCO1's shaper: R164 into D38/D39, with Q23 from the triangle (R163) across it while C38 (charged by
* line A through D31) holds its base: hard clipping at first, then soft; R165 into R159 and VCA Q26
D31 a c38 1SS133
C38 c38 0 0.0068u
R162 c38 q23b 1meg
R163 o21b q23c 1k
Q23 q23c q23b s1 2SC2603
R164 o21b s1 10k
D38 s1 0 1SS133
D39 0 s1 1SS133
R165 s1 x1 2.2k
R159 x1 0 47
Q26 x1 q26b sum 2SC2603S
* ENV1 (C36 through R168) and ENV2 (C37 through R169) into Q26's base
D29 g c36 1SS133
C36 c36 0 {c36} da={dat}
R168 c36 q26b 470k
R169 c37 q26b 220k
* VCO3: IC25a, IC22b/c (pins 3-4, 5-6), IC25b with C34, Q32
D50 a d50k 1SS133
R149 d50k n25a 10k
R148 o25b n25a 100k
X25A p25a n25a o25a opamp {gbw} 1e5 {opsatlight} {sr}
R147 o25a p25a 100k
R146 p25a 0 100k
R144 o25a i3 100k
DIC22BH i3 vdd DCMOS2
DIC22BL vss i3 DCMOS2
M22BP y3 i3 vdd 4069UB_P2
M22BN y3 i3 vss 4069UB_N2
R136 y3 n25b 47k
X25B 0 n25b o25b opamp {gbw} 1e5 {opsat} {sr}
C34 n25b o25b {c34}
R137 a q32b 1meg
Q32 q32c q32b n25b 2SC2603
R145 o25b q32c 10k
* VCO3's shaper and VCA Q28, with the tom noise through R177; ENV3 (C39 through R174;
* NAVA calls it R129)
R181 o25b s3 10k
D47 s3 0 1SS133
D46 0 s3 1SS133
R176 s3 x3 2.2k
R175 x3 0 47
R177 tn x3 22k
Q28 x3 q28b sum 2SC2603S
D43 g c39 1SS133
C39 c39 0 {c39}
R174 c39 q28b 470k
* VCO2: IC24a, IC22d/e (pins 9-8, 11-10), IC24b with C33, Q31
D51 a d51k 1SS133
R150 d51k n24a 10k
R143 o24b n24a 100k
X24A p24a n24a o24a opamp {gbw} 1e5 {opsatlight} {sr}
R142 o24a p24a 100k
R140 p24a 0 100k
R141 o24a i2 100k
DIC22DH i2 vdd DCMOS2
DIC22DL vss i2 DCMOS2
M22DP y2 i2 vdd 4069UB_P2
M22DN y2 i2 vss 4069UB_N2
R139 y2 n24b 47k
X24B 0 n24b o24b opamp {gbw} 1e5 {opsat} {sr}
C33 n24b o24b {c33}
R135 a q31b 1meg
Q31 q31c q31b n24b 2SC2603
R138 o24b q31c 10k
* VCO2's shaper and VCA Q27
R166 o24b s2 10k
D40 s2 0 1SS133
D41 0 s2 1SS133
R167 s2 x2 2.2k
R160 x2 0 47
Q27 x2 q27b sum 2SC2603S
* ENV2: D30 charges C35, which DECAY (VR14 + R171) runs down and which fills C37 through R161;
* C37 feeds Q27 (R170) and Q26 (R169)
D30 g c35 1SS133
C35 c35 0 {c35} da={dat}
P14 r171 r171 c35 {vr14} decay
R171 r171 0 56k
R161 c35 c37 100k
C37 c37 0 {c37} da={dat}
R170 c37 q27b 220k
* IC23a sums the three VCAs; LEVEL (VR15) is its feedback
X23A 0 sum o23a opamp {gbw} 1e5 {opsat} {sr}
P15 o23a o23a sum 50k level
* the multi-jack board's load (p.12; mix.cir runs it): R509 and C504 (SN 415300 up) to the jack,
* whose normal contact takes R510/R511 to the mix buses (AC ground)
R509 o23a j 1k
C504 j 0 0.01u
R510 j 0 12k
R511 j 0 12k
.control tune 0.5
.control decay 0.5
.control level 0.5
.output out o23a
.output g g
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
* Line A, VDD and VSS are cut where they are stiff (Q24 for the trigger, IC20's outputs); each VCO's
* reset diode and comparator input (to the triangle) are in the trigger partition too, so line A
* floats between hits and falls at the trigger's end as it does whole. The shapers are cut from the
* VCOs at their resistors from the triangles (op-amp outputs), and from the VCAs at their 2.2k into
* the 47 ohm nodes (R165, R176, R167): each shaper a partition of its own, the VCAs and the output
* op-amp another. On a trigger edge every partition is to be substepped (docs/circuit/toms.md, §8).
.part trig in=o21b,o25b,o24b,q26b,q27b,q28b RLATCH R154 R155 Q25 R153 R151 R152 Q24 RA4 X23B R180 R179 R178 R156 D42 R158 R157 Q29 RGF R172 P13 R173 X20A R129 R127 R121 R122 R125 X20B R126 D37 C30 D36 C31 D35 R123 R124 D50 R149 R148 D51 R150 R143 D29 C36 D43 C39 D30 C35 P14 R171 R161 C37 R168 R169 R174 R170
.part vco1 in=a,vdd,vss,s1,q23c D35 R123 R124 X21A R130 R128 R131 DIC22AH DIC22AL M22AP M22AN R134 X21B C32 R133 Q30 R132 R163 R164
.part vco3 in=a,vdd,vss,s3 D50 R149 R148 X25A R147 R146 R144 DIC22BH DIC22BL M22BP M22BN R136 X25B C34 R137 Q32 R145 R181
.part vco2 in=a,vdd,vss,s2 D51 R150 R143 X24A R142 R140 R141 DIC22DH DIC22DL M22DP M22DN R139 X24B C33 R135 Q31 R138 R166
.part shape1 in=o21b,a,x1 D31 C38 R162 R163 Q23 R164 D38 D39 R165
.part shape3 in=o25b,x3 R181 D47 D46 R176
.part shape2 in=o24b,x2 R166 D40 D41 R167
.part vca in=s1,s3,s2,c36,c37,c39 R165 R159 Q26 R168 R169 R176 R175 R177 Q28 R174 R167 R160 Q27 R170 X23A P15 R509 C504 R510 R511
)cir";

inline constexpr const char *rd = R"cir(* TR-909 RIDE, voicing board 73133090, analog half (service notes p.11, grid B-F 18-27, crop
* p11-cy-ride.png; p.5 "DIGITAL VOICE GENERATORS"; docs/circuit/cymbals.md). The clock (IC57b/c,
* VR27 TUNE; C40/R188) is rdclk.cir; the logic between it and the D/As (timing flip-flops IC56,
* counters IC58b + IC55, ROM IC54, latch IC53, NANDs IC57a/d) is core/cymbals.* (Cymbal), which
* gives this netlist the latch's and counter's outputs and IC57a's. The same circuit as the crash's
* (cr.cir) but for R439 470k (the crash's R432 is 270k) and the jack board's mix resistors.
* Inputs: d0..d5, IC53's outputs (4174 on +5 V, 0/5 V, averaged over each step by Cymbal) into RA12's
* 32R .. R, i.e. the ROM byte's bits 2..7 (the public images are in D/A order: bit 7 is the MSB);
* e0..e5, counter bits 9..14 (IC55 Q6-Q11) into RA13's 32R .. R; run, IC57a's output (NOT counter
* bit 15: 5 V while the counter runs); cr.run, the crash's IC59d output (its load on the shared accent
* DAC moves with it); acc, the open-circuit voltage of accent DAC IC9 (RA8, address 4008, shared
* with the crash; 5 V code/63).
* Outputs: out, IC64a at the voicing board's connector W5 (CN-5 pin 42) to the multi-jack board;
* jack, the MULTI OUT jack's contact behind R527/C510 with nothing plugged in (the mix resistors on it).
* Controls: level (VR26 50k B, a rheostat in IC64a's feedback).
* RA12 (RM0621, R = 5k binary weighted) through C128 drives the 22 ohm R438: a current into the VCA
* Q69, a saturated NPN between that node and IC51a's virtual ground whose conductance follows its
* base current. That current is the anti-log pair Q68's: IC52a forces Q68's reference collector to
* the envelope current (5 V - RA13) / R439 (RA13 counts up, so the current falls in 64 steps of 512
* samples to none), and the accent (R435), RUN (R437, the mute when stopped) and R433 set the
* reference base against R434's, so the gain is exponential in the accent. Then two unity-gain
* Sallen-Key low-passes with emitter followers Q77, Q78 (about 4th-order Butterworth at 27 kHz),
* C133 and the LEVEL stage IC64a.
* rlatch: IC53's and the counters' output resistance (HD14174B at 5 V, ~0.9 mA at 0.4 V drop), also
* taken for IC57a and the crash's IC59d (4011UB); rdac: IC9's RM0621 Thevenin resistance (5k || 10k
* || ... || 160k).
.param rlatch=450 rdac=2.54k
VB15 b15 0 15
VNB15 nb15 0 -15
VV5 v5 0 5
VACC acc 0 input acc
VRUN run 0 input run
VCRRUN crrun 0 input cr.run
VD0 d0 0 input d0
VD1 d1 0 input d1
VD2 d2 0 input d2
VD3 d3 0 input d3
VD4 d4 0 input d4
VD5 d5 0 input d5
VE0 e0 0 input e0
VE1 e1 0 input e1
VE2 e2 0 input e2
VE3 e3 0 input e3
VE4 e4 0 input e4
VE5 e5 0 input e5
* --- the envelope: counter bits 9-14 through RA13 and R439 into IC52a's virtual node at +5 V ------
RLE0 e0 l6 {rlatch}
RLE1 e1 l7 {rlatch}
RLE2 e2 l8 {rlatch}
RLE3 e3 l9 {rlatch}
RLE4 e4 l10 {rlatch}
RLE5 e5 l11 {rlatch}
RA13F l6 ea 160k
RA13E l7 ea 80k
RA13D l8 ea 40k
RA13C l9 ea 20k
RA13B l10 ea 10k
RA13A l11 ea 5k
R439 ea n52a 470k
* the anti-log pair Q68 (2SA798): IC52a holds Q68A's collector at its - input (+5 V), so Q68A carries
* the envelope current; Q68B's collector feeds Q69's base
X52A v5 n52a o52a opamp {gbw}
C125 n52a o52a 1000p
R436 o52a q68e 2.2k
Q68A n52a q68b1 q68e 2SA798
* the reference base: R433 to +5 V, R435 from the accent DAC, R437 from IC57a (RUN)
RRUN run rn {rlatch}
R433 v5 q68b1 220
R435 ac q68b1 4.7k
R437 rn q68b1 2.2k
* accent DAC IC9 (RA8), shared with the crash: its source resistance, and the crash's base network
* on it (R426 to its reference base, R428 to +5 V, R427 to its IC59d); the crash's base current
* (tens of nA through 200 ohms) is left out
RDAC acc ac {rdac}
R426 ac crb 4.7k
R428 v5 crb 220
RCR crrun crr {rlatch}
R427 crr crb 2.2k
* --- the D/A and the VCA ------------------------------------------------------------------------
R434 v5 q68b2 100
Q68B q69b q68b2 q68e 2SA798
* IC53's outputs through RA12 (R = 5k: d5 into R, d0 into 32R) to C128
RL0 d0 l0 {rlatch}
RL1 d1 l1 {rlatch}
RL2 d2 l2 {rlatch}
RL3 d3 l3 {rlatch}
RL4 d4 l4 {rlatch}
RL5 d5 l5 {rlatch}
RA12F l0 da 160k
RA12E l1 da 80k
RA12D l2 da 40k
RA12C l3 da 20k
RA12B l4 da 10k
RA12A l5 da 5k
C128 da vn 10u
R438 vn 0 22
* Q69 (S): collector on the 22 ohm node, emitter at IC51a's virtual ground, R424 the I-V
Q69 vn q69b n51a 2SC2603S
X51A 0 n51a o51a opamp {gbw}
R424 n51a o51a 47k
* --- the filter: two unity-gain Sallen-Key low-passes, emitter followers Q77, Q78 ----------------
R462 o51a s1 5.6k
C144 s1 q77e 1200p
R463 s1 q77b 5.6k
C145 q77b 0 1000p
Q77 b15 q77b q77e 2SC2603
R464 q77e nb15 10k
R465 q77e s2 5.6k
C146 s2 q78e 2700p
R467 s2 q78b 5.6k
C147 q78b 0 390p
Q78 b15 q78b q78e 2SC2603
R466 q78e nb15 10k
* LEVEL: C133, R446 into IC64a with VR26 in its feedback
C133 q78e c133 10u
R446 c133 n64a 22k
X64A 0 n64a out opamp {gbw} 1e5 {opsat}
P26 out out n64a 50k level
* the multi-jack board's load on W5 (JK-1, SN p.12): R527, C510 (from SN 415300) and the mix
* resistors R528 8.2k, R529 15k into the mix amplifiers' virtual grounds (mix.cir has them)
R527 out jack 1k
C510 jack 0 0.01u
R528 jack 0 8.2k
R529 jack 0 15k
.control level 0.5
.output out out
.output jack jack
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
* The envelope and the anti-log pair's reference side (cut at Q68's emitters: IC52a holds them where
* Q68A carries the envelope current, whatever Q68B draws), then the D/A and VCA to IC51a, then the
* filter and LEVEL, which sleep on their own while the VCA is shut.
.part env RLE0 RLE1 RLE2 RLE3 RLE4 RLE5 RA13F RA13E RA13D RA13C RA13B RA13A R439 X52A C125 R436 Q68A
+ RRUN R433 R435 R437 RDAC R426 R428 RCR R427
.part vca in=q68e sleep R434 Q68B RL0 RL1 RL2 RL3 RL4 RL5 RA12F RA12E RA12D RA12C RA12B RA12A C128
+ R438 Q69 X51A R424
.part out in=o51a sleep R462 C144 R463 C145 Q77 R464 R465 C146 R467 C147 Q78 R466 C133 R446 X64A P26
+ R527 C510 R528 R529
)cir";

inline constexpr const char *rdclk = R"cir(* TR-909 RIDE clock: the IC57b/c astable with TUNE, and the counters' reset differentiator, voicing
* board 73133090 (service notes p.11, grid E-F 18-20, crop p11-cy-ride.png; docs/circuit/cymbals.md,
* "Clock"). Cymbal (core/cymbals.*) runs it at its own rate to time the edges that clock IC56a/b
* (oscb, osca) and the end of the counters' reset (rst); the board doesn't run it (its edges take
* nanoseconds, its period needs 100 MHz steps to be right to 0.05 %: docs/circuit/cymbals.md, "Rates").
* Inputs: trig, the ride output of TRIG latch IC10 (4174 on +5 V, 0/5 V); run, IC57a's output (NOT
* counter bit 15: 5 V while the counter runs), the enable of IC57c.
* Outputs: osca, IC57b pin 4 (clocks IC56b); oscb, IC57c pin 10 (clocks IC56a); rst, the reset line
* of IC56a/b, IC58b and IC55.
* Controls: tune (VR27 10k B, a rheostat in the timing resistance, clockwise faster).
* IC57b (inputs 5, 6 tied, from R440) and IC57c (pin 9 the run line, pin 8 osca) make the two-gate
* astable: R441 + VR27 from osca and C130 from oscb meet at x, and the gate's input diodes clamp x's
* swing past the rails through R440. With run low IC57c's output stays high and IC57b settles,
* self-biased through R440/R441/VR27, at its threshold: the clock stops. The same circuit as the
* crash's (crclk.cir), part for part.
* Defaults are the late board: R440 10k from SN 392600 (33k before; annotated on p.11).
* rtrig: IC10's output resistance (HD14174B at 5 V, ~0.9 mA at 0.4 V drop).
.param rtrig=450 r440=10k vr27=10k c130=470p r441=6.8k cg=5p ca=15p cb=15p cgd=1f
VV5 v5 0 5
VTRIG trig 0 input trig
VRUN run 0 input run
* TRIG's edge through C40 into R188: the reset pulse (about 7 us above the inputs' threshold); DRST
* stands for those inputs' protection diodes, which clamp TRIG's falling edge
RTRIG trig t {rtrig}
C40 t rst 100p
R188 rst 0 100k
DRST 0 rst DCMOS
* IC57b: P pair from +5 V, N pair in series, both inputs on g
M57BP1 osca g v5 4011UB_P
M57BP2 osca g v5 4011UB_P
M57BN1 osca g c57b 4011UB_N
M57BN2 c57b g 0 4011UB_N
DG1 g v5 DCMOS
DG2 0 g DCMOS
* IC57c: pin 9 (run) and pin 8 (osca)
M57CP1 oscb run v5 4011UB_P
M57CP2 oscb osca v5 4011UB_P
M57CN1 oscb osca c57c 4011UB_N
M57CN2 c57c run 0 4011UB_N
* the pins' capacitance: IC57b's input (C57G); IC57b's output with IC57c pin 8 and IC56b's clock
* (C57A); IC57c's output with IC56a's clock (C57B): about 5 pF a pin
C57G g 0 {cg}
C57A osca 0 {ca}
C57B oscb 0 {cb}
* the gates' gate-drain (Miller) capacitance, IC57b's and IC57c's from osca: not on any sheet
* (C_IN 5 pF typ is with the output at a rail); none by default, fitted with the gates' drive
* (w4011n, w4011p) in the calibration (docs/circuit/cymbals.md, "Calibration")
C57M g osca {cgd}
C57N osca oscb {cgd}
* the timing parts
P27 x1 osca osca {vr27} tune
R441 x1 x {r441}
C130 x oscb {c130}
R440 x g {r440}
.control tune 0.5
.output osca osca
.output oscb oscb
.output rst rst
)cir";

inline constexpr const char *rs = R"cir(* TR-909 RIM SHOT, voicing board 73133090 (service notes p.11, grid H-J 6-10; docs/circuit/rshc.md).
* Inputs: trig, the RS output of TRIG latch IC10 (4174; TRIG2 bit 1; 0/5 V for one 2 ms scan tick);
* acc, the open-circuit voltage of accent DAC IC7 (RA6, RM0621; 5 V * code / 63), which the hand
* clap shares.
* Outputs: out, IC50b at the voicing board's connector W5 pin 38 to the multi-jack board;
* trigout, the TRIG OUT (RIM SHOT) jack on the DIN jack board JK-2 (Q67 through R413 and CN-4
* pin 30, with C600 at the jack).
* Control: level (VR19 100k B, a rheostat in IC50a's feedback).
* Three bridged-T resonators IC48a (F1, ~500 Hz), IC49a (F2, ~220 Hz) and IC49b (F3, ~1 kHz),
* rung by one positive pulse from C111/D89, rail-clipping in their own loops, are summed by
* R408/R415/R417 into the D91/D92 clipper; Q65, its base fed from C119 through R403, gates the
* clipped sum into IC50a; IC50b is a Sallen-Key high-pass with gain 2. Q63/Q64 charge C119 to the
* accent buffer IC48b's output (ACC x 3.13) while the trigger's edge lasts.
* Defaults are the late board (SN 415300 up, SN p.7); earlier units by .param: r417=12k, r392=22k,
* r393=10k, r495=0.1 (a jumper), c600=1p (no C600 at the TRIG OUT jack) and c506=1p (the jack
* board's C506, from SN 415300 too). c121/c122: C121/C122 as printed, 0.01u; every recording fits
* 0.047u better (docs/circuit/rshc.md, "Against the real unit").
* c112-c118, r418, r419, r421, r423: printed values, for the board's Calibration (the unit Lemus
* plays has IC50a's output at the jack: docs/circuit/rshc.md, "Calibration").
* rlatch: IC10's output resistance (MC14174B at 5 V: 0.88 mA typ, 0.51 min, at a 0.4 V drop; 600 as
* the other netlists); rdac: IC7's Thevenin resistance (5k || 10k || ... || 160k). The DAC node's
* other load, the hand clap's IC26a (+), is an op-amp input and takes no current.
.param rlatch=600 rdac=2.54k r417=3.3k r392=1meg r393=1meg r495=1k c600=0.1u c121=0.01u c122=0.01u r419=4.7k r421=4.7k c506=0.01u
+ c112=0.01u c113=0.01u c115=0.027u c116=0.027u c117=0.0047u c118=0.0047u r423=12k r418=680
VB15 b15 0 15
VNB15 nb15 0 -15
VTRIG trig 0 input trig
VACC acc 0 input acc
RLATCH trig t {rlatch}
RDAC acc a {rdac}
* trigger: C111 differentiates the TRIG edge into node na; D89 passes the rising edge into the
* resonators' common input nb (R396/R395), R399/R398 turn Q63 on for its first ~0.25 ms
C111 t na 0.018u
D89 na d89k 1SS133
R396 d89k nb 22k
R395 nb 0 22k
R399 na q63b 22k
R398 q63b 0 10k
Q63 q63c q63b 0 2SC2603
* accent buffer IC48b: ACC x (1 + R406/R397)
X48B a n48b o48b opamp {gbw} 1e5 {opsat} {sr}
R397 n48b 0 4.7k
R406 n48b o48b 10k
* envelope: Q63 pulls Q64's base down through D90/R400; Q64 charges C119 from IC48b through R401;
* C119 runs down through R403 into Q65's base
R402 o48b q64b 1meg
Q64 q64c q64b o48b 2SA1115
R400 q64b d90a 47k
D90 d90a q63c 1SS133
R401 q64c env 2.7k
C119 env 0 0.047u
R403 env q65b 1meg
* F1: IC48a, bridged T R407 / C113, C112 / R394
X48A nb n48a o48a opamp {gbw} 1e5 {opsat} {sr}
R407 n48a o48a 470k
C113 n48a m48a {c113}
C112 m48a o48a {c112}
R394 m48a 0 2.2k
* F2: IC49a, bridged T R414 / C116, C115 / R411
X49A nb n49a o49a opamp {gbw} 1e5 {opsat} {sr}
R414 n49a o49a 330k
C116 n49a m49a {c116}
C115 m49a o49a {c115}
R411 m49a 0 2.2k
* F3: IC49b, bridged T R416 / C118, C117 / R404
X49B nb n49b o49b opamp {gbw} 1e5 {opsat} {sr}
R416 n49b o49b 470k
C118 n49b m49b {c118}
C117 m49b o49b {c117}
R404 m49b 0 2.2k
* the sum and its clipper D91/D92; R423/R418 into Q65's collector
R408 o48a s 12k
R415 o49a s 12k
R417 o49b s {r417}
D92 s 0 1SS133
D91 0 s 1SS133
R423 s q65c {r423}
R418 q65c 0 {r418}
* Q65 (marked S): collector at the divider, emitter at IC50a's virtual ground
Q65 q65c q65b x50a 2SC2603S
* IC50a with LEVEL (VR19) and C120 in its feedback
X50A 0 x50a o50a opamp {gbw}
P19 o50a o50a x50a 100k level
C120 x50a o50a 220p
* IC50b: Sallen-Key high-pass C121/C122, R422 to the (-) input (= V+), R420; gain 1 + R421/R419
C121 o50a h50 {c121}
C122 h50 p50b {c122}
R420 p50b 0 220k
R422 h50 n50b 22k
X50B p50b n50b out opamp {gbw} 1e5 {opsat} {sr}
R419 n50b 0 {r419}
R421 n50b out {r421}
* the multi-jack board's load (p.12; mix.cir runs it): R515 and C506 (SN 415300 up) to the jack,
* whose normal contact takes R516/R517 to the mix buses (AC ground)
R515 out jk 1k
C506 jk 0 {c506}
R516 jk 0 12k
R517 jk 0 12k
* TRIG OUT: Q66 discharges C114 for the trigger's length; Q67 then holds TRIG OUT at ~+14 V until
* C114 charges back through R409/R410 (~20 ms)
R392 t q66b {r392}
R393 q66b 0 {r393}
Q66 q66c q66b 0 2SC2603
C114 q66c 0 0.1u
R409 q67b q66c 47k
R410 e67 q67b 47k
R495 b15 e67 {r495}
Q67 c67 q67b e67 2SA1115
R412 c67 0 22k
R413 c67 trigout 1k
C600 trigout 0 {c600}
.control level 0.5
.output out out
.output trigout trigout
* Runtime partitions (tr808/docs/model.md, "Partitions"): run in this order; in= nodes come from the
* others. C111's differentiator and its loads are in both trig and res: its pulse into the
* resonators rises within a sample, which the samples between partitions can't carry.
.part trig in=q65b,q66b RLATCH RDAC C111 D89 R396 R395 R399 R398 Q63 X48B R397 R406 R402 Q64 R400 D90
+ R401 C119 R403 R392
.part res in=q63c,s RLATCH C111 D89 R396 R395 R399 R398 Q63 X48A R407 C113 C112 R394 X49A R414 C116
+ C115 R411 X49B R416 C118 C117 R404 R408 R415 R417
.part vca in=o48a,o49a,o49b,env sleep R408 R415 R417 D92 D91 R423 R418 Q65 R403 X50A P19 C120
.part out in=o50a sleep C121 C122 R420 R422 X50B R419 R421 R515 C506 R516 R517
.part tout in=t sleep R392 R393 Q66 C114 R409 R410 R495 Q67 R412 R413 C600
)cir";

inline constexpr const char *sd = R"cir(* TR-909 SNARE DRUM, voicing board 73133090 (service notes p.11, grid O-R 1-12; crop
* p11-sd-snare-drum.png; theory p.6). Transcription notes: docs/circuit/sd.md.
* Inputs: trig, the SD output of the TRIG latch IC1 (0/5 V for one 2 ms panel scan); acc, the SD
* accent DAC into IC34b pin 5 (p.11 draws it from RA1, IC2's RM0621), its open-circuit voltage
* 5 V * code / 63; noise, the noise bus (IC33 pin 9, 0/15 V) into R286.
* Output: out, IC40b's output at the connector to the multi-jack board (CN-5 pin 34, R503).
* Controls: tune (VR6 10k B), tone (VR7 500k B, a rheostat in ENV4's discharge), snappy (VR9 10k B,
* a rheostat in IC39b's feedback), level (VR8 50k B, a rheostat in IC40b's feedback).
* Drum: two triangle VCOs (comparator IC37a/IC38a, three 4069UB inverters each on IC36, whose
* rails IC35 drives to +-(TUNE + ENV1), integrator IC37b/IC38b), reset by the PULSE line (Q40) for
* the trigger, rounded by diode pairs into the VCAs Q50 (ENV3) and Q51 (ENV2). Snappy: the noise
* through IC40a's low-pass into the VCA Q48 (ENV4, held by C68, TONE sets its decay) and through
* IC39a's high-pass into Q47 (ENV5), summed by IC39b. IC40b sums both. No production changes on
* the voicing board (SN p.7); the jack board's C502 is from SN 415300 (c502=1p before: no 0 F).
.param rlatch=600 rdac=2.54k c502=0.01u
* parts a unit's calibration may set (docs/circuit/sd.md, "Calibration"); the printed values
.param c66=0.01u c67=0.47u c69=0.01u c71=0.0068u vr7=500k r248=470k r249=100k
VB15 b15 0 15
VNB15 nb15 0 -15
VTRIG trig 0 input trig
VACC acc 0 input acc
VNOISE noise 0 input noise
* IC1's output resistance (HD14174 at +5 V, a stand-in), and the RM0621's 2.54k (5k || ... || 160k)
RIC1 trig t {rlatch}
RDAC acc ai {rdac}
* --- trigger: Q39 pulls Q40's base down; Q40 (PNP) lifts the PULSE line to +15 V for the trigger
R239 t q39b 22k
R242 q39b 0 10k
Q39 q39c q39b 0 2SC2603
R246 q39c q40b 22k
R244 b15 q40b 10k
Q40 pulse q40b q40e 2SA1115
R245 b15 q40e 100
* --- accent: IC34b x3.13 (ac); IC34a -0.1 x ac, ENV4's discharge target. Q41 passes ac to the
* gated accent line (ENV2, ENV3, ENV5) while Q39 is on; Q42 charges ENV4 through D60, its base held
* down by C68 after the trigger (R256 runs it out, about 24 ms)
R240 n34b 0 4.7k
R241 n34b ac 10k
X34B ai n34b ac opamp {gbw} 1e5 {opsat} {sr}
R255 ac n34a 100k
R243 n34a ac10 10k
X34A 0 n34a ac10 opamp {gbw} 1e5
R257 q41b d58a 100k
D58 d58a q39c 1SS133
Q41 gacc q41b ac 2SA1115
R256 q42b c68n 1meg
C68 ac c68n 0.0068u
D61 c68n q39c 1SS133
Q42 q42c q42b ac 2SA1115
D60 q42c env4 1SS133
* ENV4: C67 (0.47u bipolar), clamped at -0.6 V by D59, discharged through TONE (VR7) and R254
* towards IC34a's output
C67 env4 0 {c67}
D59 0 env4 1SS133
P7 vr7b vr7b env4 {vr7} tone
R254 vr7b ac10 100k
* --- ENV1 and the 4069 rails: the PULSE line charges C66 through D56/R247; IC35a buffers TUNE's
* wiper (R249) plus ENV1 (R248) as VDD, IC35b inverts it as VSS, each sensed after its 1k.
* D57 (RD18JB2, 18 V across VDD-VSS) is left out: VDD-VSS stays under 11 V.
D56 pulse d56k 1SS133
R247 d56k env1 47k
C66 env1 0 {c66}
R287 b15 tt 100k
P6 tt tw tb 10k tune
R284 tb 0 10k
R249 tw p35a {r249}
R248 env1 p35a {r248}
X35A p35a vdd o35a opamp {gbw} 1e5
R250 o35a vdd 1k
R251 vdd n35b 100k
R252 n35b vss 100k
X35B 0 n35b o35b opamp {gbw} 1e5
R253 o35b vss 1k
* --- VCO-1: comparator IC37a (lightly loaded, it swings +-14 V: opsatlight; hysteresis R260/R262;
* the PULSE line forces it high through D62/R275, and at rest, while it is low, D62 conducts from
* the line and lifts its lower threshold: the two VCOs' coupling), IC36's inverters 9-8, 11-10,
* 13-12 in parallel (R259 in, R261 out), integrator IC37b with C69; Q44 shorts C69 through R263 for
* the trigger
D62 pulse d62k 1SS133
R275 d62k p37a 10k
R262 p37a 0 100k
R260 cmp1 p37a 100k
R269 tri1 n37a 100k
X37A p37a n37a cmp1 opamp {gbw} 1e5 {opsatlight} {sr}
R259 cmp1 i1 100k
* IC36's three inverters as one (models.cir 4069UB_N3/_P3, three 4069UB channels each: the N from
* y1 to VSS, the P to VDD, the gates at i1), with their input diodes (DCMOS3) to the rails
MN36A y1 i1 vss 4069UB_N3
MP36A y1 i1 vdd 4069UB_P3
D36AL vss i1 DCMOS3
D36AH i1 vdd DCMOS3
R261 y1 n37b 47k
C69 n37b tri1 {c69}
X37B 0 n37b tri1 opamp {gbw} 1e5
R258 pulse q44b 1meg
Q44 q44c q44b n37b 2SC2603
R263 q44c tri1 10k
* --- VCO-2: the same with IC38, IC36's inverters 1-2, 3-4, 5-6 (p.11 prints "1.3.6"), C71
D63 pulse d63k 1SS133
R274 d63k p38a 10k
R271 p38a 0 100k
R272 cmp2 p38a 100k
R273 tri2 n38a 100k
X38A p38a n38a cmp2 opamp {gbw} 1e5 {opsatlight} {sr}
R270 cmp2 i2 100k
MN36B y2 i2 vss 4069UB_N3
MP36B y2 i2 vdd 4069UB_P3
D36BL vss i2 DCMOS3
D36BH i2 vdd DCMOS3
R264 y2 n38b 47k
C71 n38b tri2 {c71}
X38B 0 n38b tri2 opamp {gbw} 1e5
R267 pulse q45b 1meg
Q45 q45c q45b n38b 2SC2603
R268 q45c tri2 10k
* --- VCO-1's shaper (R283 into D65/D66) and VCA Q50 (collector on the 47 ohm node, emitter into
* IC40b's sum); ENV3: the gated accent through D68 onto C83, R291 onto C70, R290 into Q50's base.
* Q46, from the PULSE line, holds C70 down for the trigger
R283 tri1 sh1 10k
D65 0 sh1 1SS133
D66 sh1 0 1SS133
R288 sh1 x50 1.5k
R289 x50 0 47
Q50 x50 q50b sum 2SC2603S
D68 gacc env3 1SS133
C83 env3 0 0.1u
R291 env3 env3b 220k
C70 env3b 0 0.0047u
R290 env3b q50b 22k
R281 pulse q46b 100k
R282 q46b 0 22k
Q46 env3b q46b 0 2SC2603
* --- VCO-2's shaper (R305 into D69/D70) and VCA Q51; ENV2: D67 onto C82, R295 onto C79, R293 into
* Q51's base; Q49 holds C79 down for the trigger
R305 tri2 sh2 10k
D69 0 sh2 1SS133
D70 sh2 0 1SS133
R304 sh2 x51 2.2k
R303 x51 0 47
Q51 x51 q51b sum 2SC2603S
D67 gacc env2 1SS133
C82 env2 0 0.047u
R295 env2 env2b 220k
C79 env2b 0 0.01u
R293 env2b q51b 220k
R292 pulse q49b 100k
R296 q49b 0 22k
Q49 env2b q49b 0 2SC2603
* --- snappy: the noise through R286/R285 and C85 (R302) into IC40a's Sallen-Key low-pass (R301,
* R300, C81, C84; 8.5 kHz)
R286 noise nz1 10k
R285 nz1 0 3.3k
C85 nz1 nz2 0.015u
R302 nz2 0 220k
R301 nz2 nz3 22k
R300 nz3 nz4 22k
C81 nz3 lp 0.0033u
C84 nz4 0 220p
X40A nz4 lp lp opamp {gbw} 1e5
* path A: C80 into R299/R298 and the VCA Q48 (2SD1469), ENV4 into its base through R297
C80 lp c80b 0.022u
R299 c80b x48 4.7k
R298 x48 0 47
R297 env4 q48b 220k
Q48 x48 q48b ssum 2SD1469
* path B: IC39a's Sallen-Key high-pass (C76, C75, R276, R277; 4 kHz) into R278/R279 and the VCA Q47
* (2SD1469); ENV5: the gated accent through D64 onto C73, R280 into Q47's base, which Q43 holds down
* for the trigger
C76 lp hp1 0.0022u
C75 hp1 hp2 0.0022u
R277 hp2 0 47k
R276 hp1 hp 6.8k
X39A hp2 hp hp opamp {gbw} 1e5
R278 hp x47 2.2k
R279 x47 0 47
D64 gacc env5 1SS133
C73 env5 0 470p
R280 env5 q47b 220k
R266 pulse q43b 100k
R265 q43b 0 22k
Q43 q47b q43b 0 2SC2603
Q47 x47 q47b ssum 2SD1469
* IC39b sums Q47 and Q48; SNAPPY (VR9) is its feedback, with C77; C78/R294 into IC40b's sum
P9 snap snap ssum 10k snappy
C77 ssum snap 10p
X39B 0 ssum snap opamp {gbw} 1e5 {opsat} {sr}
C78 snap c78p 0.47u
R294 c78p sum 47k
* --- IC40b sums the drum VCAs and the snappy; LEVEL (VR8) is its feedback
P8 out out sum 50k level
X40B 0 sum out opamp {gbw} 1e5 {opsat} {sr}
* the multi-jack board's load (p.12): R503 1k, C502 0.01u (SN 415300 up), and R504, R505 (12k
* each) into the mix buses' virtual grounds while no plug is in the SD jack
R503 out jk 1k
C502 jk 0 {c502}
R504 jk 0 12k
R505 jk 0 12k
.control tune 0.5
.control tone 0.5
.control snappy 0.5
.control level 0.5
.output out out
.output tri1 tri1
.output tri2 tri2
.output vdd vdd
.output env4 env4
.output snap snap
.output pulse pulse
* Runtime partitions (tr808/docs/model.md, "Partitions"): run in this order; in= nodes come from the
* others. A resistor or diode across a cut is listed on both sides.
.part trig in=env1,p37a,p38a,q44b,q45b,q46b,q49b,q43b,d58a,c68n RIC1 R239 R242 Q39 R246 R244 Q40 R245
+ D56 R247 D62 R275 D63 R274 R258 R267 R281 R292 R266 D58 D61
.part rails in=pulse D56 R247 C66 R287 P6 R284 R249 R248 X35A R250 R251 R252 X35B R253
.part acc in=q39c,q48b,env3b,env2b,q47b RDAC R240 R241 X34B R255 R243 X34A R257 D58 Q41 R256 C68 D61
+ Q42 D60 C67 D59 P7 R254 R297 D68 C83 R291 D67 C82 R295 D64 C73 R280
.part vco1 in=pulse,vdd,vss,sh1 D62 R275 R262 R260 R269 X37A R259 MN36A MP36A D36AL D36AH R261 C69 X37B R258 Q44 R263 R283
.part vco2 in=pulse,vdd,vss,sh2 D63 R274 R271 R272 R273 X38A R270 MN36B MP36B D36BL D36BH R264 C71 X38B R267 Q45 R268 R305
.part shape1 in=tri1,x50 R283 D65 D66 R288
.part shape2 in=tri2,x51 R305 D69 D70 R304
* The noise filter (C85/R302, IC40a) runs at twice the voice's rate: at the voice's, the trapezoidal
* rule moves IC40a's 8.5 kHz peak down and its top octave 2-4 dB lower (docs/circuit/sd.md §13).
* IC40a's output is the cut, an op-amp's.
.part noise over=2 R286 R285 C85 R302 R301 R300 C81 C84 X40A
.part snappy in=env4,env5,pulse,sum,lp sleep C80 R299 R298 Q48
+ R297 C76 C75 R277 R276 X39A R278 R279 Q47 R280 Q43 R266 R265 P9 C77 X39B C78 R294
.part out in=sh1,sh2,pulse,env3,env2,c78p R288 R289 Q50 R290 C70 Q46 R281 R282 R291 R304 R303 Q51 R293
+ C79 Q49 R292 R296 R295 R294 P8 X40B R503 C502 R504 R505
)cir";

inline constexpr const char *tomnoise = R"cir(* TR-909 TOM NOISE, voicing board (service notes p.11, grid K-N 14-17; crop p11-noise-tom-noise.png),
* shared by the three toms.
* Inputs: noise (the noise generator, IC33 pin 9), trig.lt, trig.mt, trig.ht (the three toms' TRIG
* latch outputs: the board's signals, each tom's trig), lt.g, mt.g, ht.g (the toms' gated accent
* lines: lt.cir's, mt.cir's and ht.cir's output g).
* Output: tn (IC27a's output, the TOM NOISE bus: each tom's input tomnoise.tn).
* The noise through R190/C161 and the Sallen-Key high-pass IC27b (C51, C52, R191, R192, about
* 720 Hz) into IC27a's virtual ground two ways: always through R194, and through C53, R195 and the
* VCA Q33. Any tom's TRIG holds Q34 on (D1-D3), which keeps Q33 off while the tom's gated accent
* charges C54 (D26, D44, D45); when the trigger ends, C54 runs down through R198 and R200 into Q33's
* base: a noise burst of about a millisecond at each hit's start.
* Late board (SN 426700 up, p.7: "to emphasize attack of TOM TOMs"): C54 0.0047u, R194 100k; before:
* C54 0.0022u, R194 47k.
.param rlatch=600 c54=0.0047u r194=100k
VB15 b15 0 15
VNB15 nb15 0 -15
VNOISE noise 0 input noise
VTLT trig_lt 0 input trig.lt
VTMT trig_mt 0 input trig.mt
VTHT trig_ht 0 input trig.ht
VGLT g_lt 0 input lt.g
VGMT g_mt 0 input mt.g
VGHT g_ht 0 input ht.g
* the TRIG latch's output resistance, on each line (the toms' netlists hold their own)
RLATCH1 trig_lt tlt {rlatch}
RLATCH2 trig_mt tmt {rlatch}
RLATCH3 trig_ht tht {rlatch}
* Q34: on while any tom's TRIG is high
D1 tlt dtrig 1SS133
D2 tmt dtrig 1SS133
D3 tht dtrig 1SS133
R196 dtrig q34b 22k
R197 q34b 0 10k
Q34 q34c q34b 0 2SC2603
* C54, charged from the toms' gated accent lines (D26 from LT's, D44 from MT's, D45 from HT's, by
* the lines' markers on p.11; NAVA has D45 and D26 the other way round), into Q33's base
D26 g_lt c54 1SS133
D44 g_mt c54 1SS133
D45 g_ht c54 1SS133
C54 c54 0 {c54}
R198 c54 q34c 100k
R200 q34c q33b 100k
* the noise: R190/C161, then IC27b's Sallen-Key high-pass (unity gain)
R190 noise n1 22k
C161 n1 0 0.0047u
C51 n1 n2 0.0047u
C52 n2 n3 0.0047u
R192 n3 0 220k
X27B n3 o27b o27b opamp {gbw} 1e5 {opsat} {sr}
R191 n2 o27b 10k
* the VCA Q33 (C53, R195 into R199) and the steady path R194, into IC27a's virtual ground
C53 o27b n4 0.047u
R195 n4 x 1k
R199 x 0 47
Q33 x q33b sum 2SC2603S
R194 o27b sum {r194}
X27A 0 sum o27a opamp {gbw} 1e5 {opsat} {sr}
R193 sum o27a 22k
* the toms' R114, R177, R387 (22k each into a 47 ohm node: ground to within 0.2 %)
R114 o27a 0 22k
R177 o27a 0 22k
R387 o27a 0 22k
.output tn o27a
* Runtime partitions (docs/model.md, "Partitions"): the filter (linear) and the rest, cut at IC27b's
* output.
.part filter R190 C161 C51 C52 R192 X27B R191
.part vca in=o27b RLATCH1 RLATCH2 RLATCH3 D1 D2 D3 R196 R197 Q34 D26 D44 D45 C54 R198 R200 C53 R195 R199 Q33 R194 X27A R193 R114 R177 R387
)cir";

inline const char *find(const char *name)
{
	struct Entry {
		const char *name, *text;
	};
	static const Entry all[] = {
		{"bd", bd},
		{"cr", cr},
		{"crclk", crclk},
		{"hc", hc},
		{"hh", hh},
		{"ht", ht},
		{"lt", lt},
		{"mix", mix},
		{"models", models},
		{"mt", mt},
		{"rd", rd},
		{"rdclk", rdclk},
		{"rs", rs},
		{"sd", sd},
		{"tomnoise", tomnoise},
	};
	for (const auto &e : all)
		if (std::strcmp(e.name, name) == 0)
			return e.text;
	return nullptr;
}

} // namespace tr909::netlists
