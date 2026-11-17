// Generated from machines/tr808/core/netlists/*.cir by tools/cmake/netlists.cmake.
#pragma once

namespace tr808::netlists {

inline constexpr const char *accent = R"cir(* TR-808 common trig and accent, main board (service notes p.8; theory p.4). Inputs: mono (IC6
* pin 10, the CMOS monostable on B1: 15 V idle, 0 V for the 1 ms after a step pulse), acc (the
* ACCENT instrument data from PF3, 0/5 V). Output: trig (Q32's emitter, the COMMON TRIG bus).
* Control: accent (VR3 10k B, AC LEVEL).
* Q31 holds Q32's base down until the monostable fires; then Q32 follows node x: the 5 V line
* through Q34 without accent, VR3's wiper (5-15 V) with it.
VB1 b1 0 15
V5 v5 0 5
VMONO mono 0 input mono
VACC acc 0 input acc
P3 b1 w3 v5 10k accent
R114 w3 x 4.7k
R119 acc q33b 22k
R118 q33b 0 10k
Q33 q33c q33b 0 2SC945
R123 q34b q33c 15k
R122 b1 q34b 33k
Q34 x q34b v5 2SC945
R120 x y 4.7k
R117 mono q31b 47k
Q31 y q31b 0 2SC945
R121 b1 c25 100
C25 c25 0 47u
Q32 c25 y trig 2SC945
R113 trig 0 4.7k
.control accent 0.5
.output trig trig
)cir";

inline constexpr const char *bd = R"cir(* TR-808 BASS DRUM, main board OP3116-130 (service notes p.9, grid E-K 27-38).
* Inputs: trig, the common trig bus (main board Q32's emitter); data, the BD instrument data
* line from the CPU (0/5 V). Output: out, the voice after C50/R174, into its LEVEL feed.
* Controls: tone (VR5 10k C), decay (VR6 500k B), level (VR4 100k A).
.param c42=0.015u c41=0.015u r154=10k r177=82k r170=470k p6=500k
VB2 b2 0 15
VNB2 nb2 0 -15
VTRIG trig 0 input trig
VDATA data 0 input data
* trigger gate: Q39 on with BD data pulls Q40's base down; Q40 passes the common trig.
R152 data q39b 22k
R153 q39b 0 10k
Q39 q39c q39b 0 2SC945
R154 q40b q39c {r154}
R155 trig q40b 4.7k
Q40 t q40b trig 2SA733
* pulse shaper: Q41 discharges C38 during the trigger, Q42 lets go of node p for a few ms after
* it, which switches Q43 (R165 shorted) and, as Q42 turns back on, sends a negative pulse
* through C39/R161 into the resonator.
R157 t q41b 8.2k
R158 q41b 0 2.7k
Q41 q42b q41b 0 2SC945
R156 b2 q42b 1meg
C38 q42b 0 0.1u
Q42 p q42b 0 2SC945
R160 b2 p 22k
C39 p q 0.033u
D52 q 0 1S2473
R161 q m 1meg
R159 p q43b 100k
Q43 u q43b 0 2SC945
* the trigger into IC12a's + input
C40 t n 0.015u
R163 t n 100k
R162 n 0 4.7k
D53 0 n 1S2473
* bridged-T resonator around IC12a
X12A n v w opamp {gbw} 1e5 {opsat} {sr}
C41 v m {c41}
C42 m w {c42}
R166 m u 6.8k
R165 u 0 47k
R167 v w 1meg
* DECAY: IC12b inverts the resonator's output and feeds it back into its centre through R170
R164 w y 47k
R169 y z 47k
P6 c43 c43 y {p6} decay
C43 c43 z 33u
R168 y12p 0 22k
X12B y12p y z opamp {gbw} 1e5 {opsat} {sr}
R170 z m {r170}
* TONE: a low-pass, R171 and VR5 (wiper to pin 1) with R172 across it, into C45
R171 w k 220
P5 k l l 10k tone c0.15
R172 k l 10k
C45 l 0 0.1u
* LEVEL (VR4) and the output follower, behind the R177/R176 divider (R177 the surface-mounted 82k)
C47 l lv 0.47u
P4 lv lw 0 100k level a0.15
C49 lw q44i 0.47u
R176 q44b 0 100k
R177 q44i q44b {r177}
Q44 b2 q44b e44 2SC945
R173 e44 nb2 6.8k
C48 e44 0 220p
C50 e44 o50 1u
R174 o50 0 100k
R175 o50 out 1k
* the mix resistor on the jack's normal contact (Jack board B), into IC4a's virtual ground
* with a cable in the jack (plug), the contact opens and the cable's input loads the voice
SMIX out 0 !plug 22k 100k
.control plug 0
.control tone 0.5
.control decay 0.5
.control level 0.8
.output out out
.output res w
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
.part trig in=q43b,m R152 R153 Q39 R154 R155 Q40 R157 R158 Q41 R156 C38 Q42 R160 C39 D52 R161
+ R159 C40 R163 R162 D53
.part res in=t,p,q C40 R163 R162 D53 X12A C41 C42 R166 R165 R167 R164 R169 P6 C43 R168 X12B R170
+ R171 P5 R172 C45 C47 P4 C49 R176 R177 Q44 R173 C48 C50 R174 R175 SMIX R159 Q43 R161
)cir";

inline constexpr const char *cb = R"cir(* TR-808 COWBELL, voicing board (service notes p.13). Inputs: ntrig (the narrowed trig, IC2a),
* data (CB instrument data, 0/5 V), o1 and o2 (the oscillators at TP6 and TP7, trimmed by TM1 and
* TM2 to 1.85 ms (540 Hz) and 1.25 ms (800 Hz)). Output: out (IC6a through R163).
* Controls: level (VR5 5k A, before the buffer).
* Gate Q1/Q5 charges C9 through R122/D2; R82 and C34 across C9 make the fast first drop. The
* envelope feeds two swing VCAs (Q15 on o1, Q14 on o2) through R28/D3 and R29/D4; their sum goes
* through IC2b's band-pass.
.param c9=0.47u c34=1u r82=33k r28=100k r29=100k
VB1 b1 0 15
VNB1 nb1 0 -15
VNT ntrig 0 input ntrig
VDATA data 0 input data
VO1 o1 0 input o1
VO2 o2 0 input o2
R4 data q1b 22k
R5 q1b 0 10k
Q1 q1c q1b 0 2SC945
R9 q5b q1c 22k
R14 ntrig q5b 4.7k
Q5 q5c q5b ntrig 2SA733
R122 q5c d2a 1k
D2 d2a env 1S2473
C9 env 0 {c9}
R82 env c34 {r82}
C34 c34 0 {c34}
* swing VCAs
R62 o1 sq1 33k
R63 sq1 0 2.2k
C32 sq1 q15b 10u
R30 b1 q15b 470k
Q15 q15c q15b q15e 2SC945
R31 q15e 0 100
R64 o2 sq2 33k
R65 sq2 0 2.2k
C33 sq2 q14b 10u
R32 b1 q14b 470k
Q14 q14c q14b q14e 2SC945
R33 q14e 0 100
R28 env v1 {r28}
D3 v1 q15c 1S2473
R29 env v2 {r29}
D4 v2 q14c 1S2473
* IC2b band-pass (multiple feedback)
R26 v1 c30 10k
C30 c30 p 0.0022u
R27 v2 c31 10k
C31 c31 p 0.0022u
R24 p 0 2.2k
C29 p bp 0.0033u
C28 p n2b 0.01u
R25 n2b bp 470k
X2B 0 n2b bp opamp {gbw}
* LEVEL (VR5 5k A) and the IC6a follower
C75 bp c75 1u
R116 c75 lv 8.2k
P5 lv lw 0 5k level a0.15
C76 lw p6 0.022u
R117 p6 0 22k
X6A p6 o6 o6 opamp {gbw}
R163 o6 out 1k
* the mix resistor on the jack's normal contact (Jack board B), into IC4a's virtual ground
* with a cable in the jack (plug), the contact opens and the cable's input loads the voice
SMIX out 0 !plug 47k 100k
.control plug 0
.control level 0.8
.output out out
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
.part env in=v1,v2 R4 R5 Q1 R9 R14 Q5 R122 D2 C9 R82 C34 R28 R29
.part vca in=env R62 R63 C32 R30 Q15 R31 R64 R65 C33 R32 Q14 R33 R28 D3 R29 D4 R26 C30 R27 C31
+ R24 C29 C28 R25 X2B C75 R116 P5 C76 R117 X6A R163 SMIX
)cir";

inline constexpr const char *cp = R"cir(* TR-808 HAND CLAP (SW12 at CP), main board (service notes p.9, grid H-T 2-14; theory p.6, Fig. 13).
* Inputs: trig (common trig bus), data (CP instrument data, 0/5 V), wn (white noise from IC24).
* Output: out (IC19b through R296; TP5). Controls: offset (TM3 10k B,
* "C.P. OFFSET", the service trim), level (VR17 100k A).
* C140 is printed .0027 in the schematic; 0.027 uF (the Yocto clone's value) gives the 30 ms pulse
* the theory describes (R350 x C140). The "PORTION CHANGED" box adds a 10k (called R200) from Q70's
* emitter to -15 V in later units: r200cp.
.param r106=10k r346=680 r332=27k c140=0.027u c144=0.47u r200cp=1e12 cmpvg=0.5m c128=0.0047u c129=0.0047u c138=0.047u
VB7 b7 0 15
VNB7 nb7 0 -15
VB8 b8 0 15
VNB8 nb8 0 -15
VTRIG trig 0 input trig
VDATA data 0 input data
VWN wn 0 input wn
* trigger gate (SW12 sends it here at CP; ma.cir has the MA side)
R103 data q28b 22k
R104 q28b 0 10k
Q28 q28c q28b 0 2SC945
R106 q29b q28c {r106}
R105 trig q29b 4.7k
Q29 t q29b trig 2SA733
R12SW t tcp 0.1
* the noise band-pass IC21b (multiple feedback, about 1 kHz)
R332 wn n21 {r332}
C127 wn n21 0.0018u
R333 n21 0 10k
C129 n21 o21 {c129}
C128 n21 v21 {c128}
R334 v21 o21 100k
X21B 0 v21 o21 opamp {gbw}
* "reverb": Q69 follows the trigger into C139 (D69) and on through R348 into C138; that
* envelope biases the swing VCA Q70, whose base also gets the band-passed noise
R342 o21 nq 15k
R346 nq 0 {r346}
C137 nq q70b 0.033u
Q69 b8 tcp e69 2SC945
D69 e69 e2 1S2473
C139 e2 0 0.018u
R348 e2 e1 1meg
C138 e1 0 {c138}
R349 e1 q70b 1meg
Q70 c70 q70b e70 2SC945
R369 e70 0 330
R200CP e70 nb8 {r200cp}
R363 b8 c70 10k
C146 c70 0 0.018u
C152 c70 c152 0.0047u
R376 c152 mix 47k
* the clap's sawtooth envelope: the AN6912 IC23 on -15 V only, open collectors
R360 tcp n8 27k
R361 n8 nb8 22k
R353 0 na 4.7k
R352 na nb 4.7k
R351 nb nb8 4.7k
BC23C nb n8 o14 nb8 comparator vg={cmpvg} vcm=-1.5 vos=-2m
R350 0 o14 1meg
C140 o14 0 {c140}
BC23B o14 na o1 nb8 comparator vg={cmpvg} vcm=-1.5 vos=-2m
R355 o1 n4 2.7k
R354 n4 0 5.6k
BC23A na o1 o13 nb8 comparator vg={cmpvg} vcm=-1.5 vos=-2m
R365 0 o13 82k
D71 o13 c144 1S2473
C144 c144 0 {c144}
R356 c144 n5 1meg
BC23D n5 n4 o2 nb8 comparator vg={cmpvg} vcm=-1.5 vos=-2m
C142 n5 o2 220p
R359 0 o2 10k
C141 o1 x73 0.001u
D70 nb8 x73 1S2473
R357 x73 q73b 68k
R358 o2 q73b 68k
Q73 q73c q73b nb8 2SC945
R367 q73c c144 330
* the exponential converter Q72/Q71 (PNP pair on R366) sets IC22's I_abc; Q71's base gets the
* offset (TM3) and the accent memory (Q69, D68, C143, R362)
R368 q73c q72b 470k
R371 q72b 0 15k
R366 b8 e72 15k
Q72 abc q72b e72 2SA733
Q71 nb8 q71b e72 2SA733
R364 q71b 0 2.2k
R370 tm3w q71b 47k
P3 0 tm3w nb8 10k offset
D68 e69 e3 1S2473
C143 e3 0 1u
R362 e3 q71b 330k
R374 abc pin1 10k
* IC22 BA662(A): OTA (textbook model, as the TB-303's) and its unity buffer
C136 o21 c136 0.022u
R372 c136 ota_n 39k
R382 ota_n 0 1k
R381 ota_p 0 1k
B22 pin1 nb8 ota_p ota_n ota6 ota rs=450
R383 ota6 0 82k
E22 ota8 0 ota6 0 1
C150 ota8 c150 0.001u
R378 c150 mix 100k
* mixer IC19b with LEVEL (VR17) in its feedback
R390 mix s19 0.1
R373 p19 0 47k
X19B p19 s19 o19 opamp {gbw}
R375 s19 o19 470k
C149 s19 o19 220p
P17 c145 c145 s19 100k level a0.15
C145 c145 o19 33u
R296 o19 out 1k
* the mix resistor on the jack's normal contact (Jack board B), into IC4a's virtual ground
* with a cable in the jack (plug), the contact opens and the cable's input loads the voice
SMIX out 0 !plug 33k 100k
.control plug 0
.control offset 0.5
.control level 0.8
.output out out
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
.part noise R332 C127 R333 C129 C128 R334 X21B
.part trig in=q70b,q71b R103 R104 Q28 R106 R105 Q29 R12SW R360 R361 Q69 D69 C139 R348 C138 R349
+ D68 C143 R362
.part env in=tcp,q72b R360 R361 R353 R352 R351 BC23C R350 C140 BC23B R355 R354 BC23A R365 D71
+ C144 R356 BC23D C142 R359 C141 D70 R357 R358 Q73 R367 R368
.part exp in=q73c,e3,pin1 R368 R371 R366 Q72 Q71 R364 R370 P3 R362 R374
.part vca in=o21,e1,mix sleep R342 R346 C137 R349 Q70 R369 R200CP R363 C146 C152 R376
.part ota in=o21,abc,mix sleep R374 C136 R372 R382 R381 B22 R383 E22 C150 R378
.part out in=c152,c150 R376 R378 R390 R373 X19B R375 C149 P17 C145 R296 SMIX
)cir";

inline constexpr const char *cy = R"cir(* TR-808 CYMBAL, voicing board (service notes p.13; Werner, Abel and Smith, ICMC/SMC 2014).
* Inputs: ntrig (the narrowed trig), data (CY instrument data, 0/5 V), hi and lo (the metallic
* source's bands, IC3). Output: out (IC6b through R166). Controls: decay (VR2 2M B), tone
* (VR4 20k B), level (VR6 100k A).
* Gate Q2/Q6 drives Q19, which charges three envelopes through D6, D7, D8: the high band's (C38,
* R87/C37, R94, C47; swing VCA Q16, short), the middle's (C40, R88, C39, R90, C50; Q17) and the low
* band's (C41, discharged by R93 and DECAY; followed by Q20 into R105/C45/R104; Q18), Q20's emitter
* also pulling the middle band's envelope through R91/R92. Each VCA's output is high-passed (IC5a,
* IC5b; Q25) and TONE balances them into IC6b.
.param p2=2meg c41=1u r93=470k c45=2.2u r125=2.2k c37=2.2u c39=0.47u c38=1u c40=1u r87=22k r88=33k
VB1 b1 0 15
VB2 b2 0 15
VNB1 nb1 0 -15
VNB2 nb2 0 -15
VNT ntrig 0 input ntrig
VDATA data 0 input data
VHI hi 0 input hi
VLO lo 0 input lo
R3 data q2b 22k
R6 q2b 0 10k
Q2 q2c q2b 0 2SC945
R10 q6b q2c 22k
R15 ntrig q6b 4.7k
Q6 q6c q6b ntrig 2SA733
R83 q6c 0 22k
* Q19 charges the envelopes
R84 q6c q19b 10k
R86 b1 c35 100
C35 c35 0 47u
R85 c35 q19c 100
Q19 q19c q19b e0 2SC945
D6 e0 a1 1S2473
C38 a1 0 {c38}
R87 a1 c37 {r87}
C37 c37 0 {c37}
R94 a1 b1n 39k
C47 b1n 0 0.0015u
D5 b1n q16c 1S2473
C36 q16c 0 0.0015u
D7 e0 a2 1S2473
C40 a2 0 {c40}
R88 a2 c39 {r88}
C39 c39 0 {c39}
R90 c39 b2n 33k
C50 b2n 0 0.0015u
D11 b2n q17c 1S2473
C43 q17c 0 0.0015u
D8 e0 c41 1S2473
C41 c41 0 {c41}
R93 c41 vr2 {r93}
P2 vr2 vr2 0 {p2} decay
Q20 b1 c41 e20 2SC945
R92 e20 j 33k
R89 j 0 10k
R91 b2n j 33k
R105 e20 c45 33k
C45 c45 0 {c45}
R104 c45 l0 22k
D12 l0 q18c 1S2473
* the three swing VCAs, on the bands
C42 hi q16b 0.022u
R96 b1 r96 1meg
R97 r96 q16b 1meg
Q16 q16c q16b q16e 2SC945
R95 q16e 0 100
C44 hi q17b 0.022u
R100 b1 r100 1meg
R99 r100 q17b 1meg
Q17 q17c q17b q17e 2SC945
R98 q17e 0 100
C46 lo q18b 0.022u
R102 b1 r102 1meg
R103 r102 q18b 1meg
Q18 q18c q18b q18e 2SC945
R101 q18e 0 100
* high-passes: Q16's through C49/R106 and IC5a (Sallen-Key, gain 2.77), Q17's through IC5b
* (gain 2), Q18's through Q25 and R123/C57
C49 b1n c49 0.0033u
R106 c49 0 6.8k
C53 c49 s5a 0.001u
C54 s5a p5a 0.001u
R109 p5a 0 18k
R108 s5a o5a 18k
R111 n5a 0 22k
R110 n5a o5a 39k
X5A p5a n5a o5a opamp {gbw}
C51 b2n s5b 0.001u
C52 s5b p5b 0.001u
R113 p5b 0 18k
R107 s5b o5b 18k
R115 n5b 0 39k
R114 n5b o5b 39k
X5B p5b n5b o5b opamp {gbw}
C48 l0 h25 0.0015u
R124 h25 e25 22k
C59 h25 q25b 0.0015u
R127 q25b 0 82k
Q25 b2 q25b e25 2SC945
R126 e25 nb2 22k
C58 e25 c58 0.01u
R123 c58 c57 100k
C57 c57 0 0.0033u
R121 c57 t2 10k
* TONE (VR4: pin 1 at the high band's node, pin 3 through R125 at the others', wiper to ground)
C55 o5a c55 0.01u
R112 c55 t1 22k
C56 o5b c56 0.01u
R120 c56 t2 10k
P4 r125 0 t1 20k tone
R125 r125 t2 {r125}
R119 t1 m 22k
R129 t2 m 15k
* IC6b with LEVEL (VR6) in its feedback
C90 m n6b 0.01u
R118 p6b 0 47k
X6B p6b n6b o6b opamp {gbw}
R128 n6b o6b 470k
C77 n6b o6b 220p
P6 c78 c78 n6b 100k level a0.15
C78 c78 o6b 33u
R166 o6b out 1k
* the mix resistor on the jack's normal contact (Jack board B), into IC4a's virtual ground
* with a cable in the jack (plug), the contact opens and the cable's input loads the voice
SMIX out 0 !plug 47k 100k
.control plug 0
.control decay 0.5
.control tone 0.5
.control level 0.8
.output out out
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
.part env in=b1n,b2n,l0 R94 R90 R91 R104 R3 R6 Q2 R10 R15 Q6 R83 R84 R86 C35 R85 Q19 D6 C38 R87
+ C37 D7 C40 R88 C39 D8 C41 R93 P2 Q20 R92 R89 R105 C45
.part hi in=a1 sleep R94 C47 D5 C36 C42 R96 R97 Q16 R95 C49 R106 C53 C54 R109 R108 R111 R110 X5A
.part mid in=c39,j sleep R90 R91 C50 D11 C43 C44 R100 R99 Q17 R98 C51 C52 R113 R107 R115 R114 X5B
.part lo in=c45 sleep R104 D12 C46 R102 R103 Q18 R101 C48 R124 C59 R127 Q25 R126
.part out in=o5a,o5b,e25 C58 R123 C57 R121 C55 R112 C56 R120 P4 R125 R119 R129 C90 R118 X6B R128
+ C77 P6 C78 R166 SMIX
)cir";

inline constexpr const char *hats = R"cir(* TR-808 OPEN HIHAT and CLS'D HIHAT, voicing board (service notes p.13): one circuit, since a CH
* trigger chokes the OH envelope (Q23 through R173). Inputs: ntrig (the narrowed trig), dataoh,
* datach (instrument data, 0/5 V), hi (the metallic source's high band, IC3b). Outputs: oh (IC7a
* through R169), ch (IC7b through R172). Controls: decay (VR3 2M B, OH DECAY), levelo (VR7),
* levelc (VR8), both 100k A.
.param c63=0.47u p3=2meg r132=100k c60=10u c62=0.47u r139=1meg
VB3 b3 0 15
VNB3 nb3 0 -15
VNT ntrig 0 input ntrig
VDOH dataoh 0 input dataoh
VDCH datach 0 input datach
VHI hi 0 input hi
* gates: Q3/Q7 (OH), Q4/Q8 (CH)
R1 dataoh q3b 22k
R8 q3b 0 10k
Q3 q3c q3b 0 2SC945
R12 q7b q3c 22k
R17 ntrig q7b 4.7k
Q7 q7c q7b ntrig 2SA733
R131 q7c j0 22k
R130 j0 0 10k
R2 datach q4b 22k
R7 q4b 0 10k
Q4 q4c q4b 0 2SC945
R11 q8b q4c 22k
R16 ntrig q8b 4.7k
Q8 q8c q8b ntrig 2SA733
R13 q8c 0 22k
* OH envelope: Q24 charges C62; Q21 empties C60 at the trigger, which then charges through OH
* DECAY and R132 until Q22 turns on and empties C62 through R134; a CH (Q28's emitter, R173)
* turns Q23 on and does it at once
R136 b3 s3 100
C61 s3 0 47u
R137 s3 q24c 470
R135 q7c q24b 10k
Q24 q24c q24b eoh 2SC945
C62 eoh 0 {c62}
R139 eoh 0 {r139}
R134 eoh q22c 100k
Q21 k j0 0 2SC945
P3 r132 r132 b3 {p3} decay
R132 r132 k {r132}
C60 k 0 {c60}
R133 k q22b 10k
Q22 q22c q22b 0 2SC945
Q23 q22c q23b 0 2SC945
R173 ech q23b 330k
R140 eoh q29b 33k
Q29 b3 q29b e29 2SC945
R143 e29 0 33k
R144 e29 noh 33k
* CH envelope: Q28 charges C63
R138 s3 q28c 470
R141 q8c q28b 33k
Q28 q28c q28b ech 2SC945
C63 ech 0 {c63}
R142 ech nch 33k
* OH swing VCA Q27 on the high band, high-pass (Q26), IC7a with LEVEL (VR7)
C67 hi q27b 0.022u
R148 b3 r148 1meg
R149 r148 q27b 1meg
Q27 q27c q27b q27e 2SC945
R145 q27e 0 100
C65 q27c 0 0.0015u
D13 noh q27c 1S2473
C64 noh 0 0.0015u
C66 noh h26 0.0015u
R147 h26 e26 2.7k
C68 h26 q26b 0.0015u
R146 q26b 0 68k
Q26 b3 q26b e26 2SC945
R156 e26 nb3 22k
C80 e26 c80 470p
R160 c80 n7a 39k
R158 p7a 0 47k
X7A p7a n7a o7a opamp {gbw}
R159 n7a o7a 470k
C79 n7a o7a 220p
P7 c81 c81 n7a 100k levelo a0.15
C81 c81 o7a 33u
R169 o7a oh 1k
* CH swing VCA Q30, high-pass (Q31), IC7b with LEVEL (VR8)
C71 hi q30b 0.022u
R151 b3 r151 1meg
R150 r151 q30b 1meg
Q30 q30c q30b q30e 2SC945
R152 q30e 0 100
C70 q30c 0 0.0015u
D14 nch q30c 1S2473
C69 nch 0 0.0015u
C72 nch h31 0.001u
R153 h31 e31 2.7k
C73 h31 q31b 0.001u
R155 q31b 0 68k
Q31 b3 q31b e31 2SC945
R154 e31 nb3 22k
C74 e31 c74 470p
R162 c74 n7b 33k
R157 p7b 0 47k
X7B p7b n7b o7b opamp {gbw}
R161 n7b o7b 470k
C82 n7b o7b 220p
P8 c83 c83 n7b 100k levelc a0.15
C83 c83 o7b 33u
R172 o7b ch 1k
* the mix resistor on the jack's normal contact (Jack board B), into IC4a's virtual ground
* with a cable in a jack (plug_oh, plug_ch), its contact opens and the cable's input loads the voice
SMIX oh 0 !plug_oh 56k 100k
SMIXCH ch 0 !plug_ch 47k 100k
.control plug_oh 0
.control plug_ch 0
.control decay 0.5
.control levelo 0.8
.control levelc 0.8
.output oh oh
.output ch ch
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
* The gates, the open hat's envelope and timer with the choke, the closed hat's envelope, then each
* hat's VCA and output, which sleep on their own while their envelope is down.
.part gates in=q24b,q28b R1 R8 Q3 R12 R17 Q7 R131 R130 R135 R2 R7 Q4 R11 R16 Q8 R13 R141
.part ohenv in=q7c,ech,noh,q28c R131 R130 R135 R136 C61 R137 Q24 C62 R139 R134 Q21 P3 R132 C60
+ R133 Q22 Q23 R173 R140 Q29 R143 R144 R138
.part chenv in=q8c,s3,nch,q23b R141 R138 Q28 C63 R142 R173
.part oh in=e29 sleep R144 C67 R148 R149 Q27 R145 C65 D13 C64 C66 R147 C68 R146 Q26 R156 C80
+ R160 R158 X7A R159 C79 P7 C81 R169 SMIX
.part ch in=ech sleep R142 C71 R151 R150 Q30 R152 C70 D14 C69 C72 R153 C73 R155 Q31 R154 C74
+ R162 R157 X7B R161 C82 P8 C83 R172 SMIXCH
)cir";

inline constexpr const char *ht = R"cir(* TR-808 HI TOM / HI CONGA, main board (service notes p.9, grid O-T 15-27). As the low tom.
* Inputs: trig, data (HT instrument data), pn (pink noise). Output: out (IC16b through R294).
* Controls: tuning (VR15 500R B), conga (SW10: 1 = HC), level (VR14 100k A).
.param r268=10k c103=0.027u c105=0.0056u c104=0.022u r286=820k p15=500
VB6 b6 0 15
VB5 b5 0 15
VTRIG trig 0 input trig
VDATA data 0 input data
VPN pn 0 input pn
R267 data q56b 22k
R271 q56b 0 10k
Q56 q56c q56b 0 2SC945
R268 q57b q56c {r268}
R269 trig q57b 4.7k
Q57 t q57b trig 2SA733
R270 t 0 4.7k
C99 t a 0.0027u
D60 0 a 1S2473
R282 a b 22k
R281 b 0 5.6k
S10B b r280 conga
R280 r280 0 1.5k
X18A b v o opamp {gbw} 1e5 {opsat} {sr}
C103 v m {c103}
C105 m o {c105}
S10A m c104 !conga
C104 c104 o {c104}
R284 v o 820k
R287 m tw 1k
P15 0 tw vr15a {p15} tuning
D84 m vr15a 1S188FM
D85 vr15a m 1S188FM
R285 o n18b 33k
R283 n18b o18b 33k
X18B 0 n18b o18b opamp {gbw} 1e5 {opsat} {sr}
R286 o18b m {r286}
R274 t d59a 4.7k
D59 d59a e 1S2473
C100 e 0 0.033u
R276 e f 2.2meg
C98 f g 0.047u
R273 pn g 15k
R272 g 0 3.3k
R275 f q58b 33k
Q58 q58c q58b 0 2SC945
R279 b6 q58c 4.7k
C101 q58c h 0.027u
R277 h 0 22k
R278 h k 22k
C102 k 0 0.01u
S10C k noise !conga
S10D noise 0 conga
R289 o18b s 100k
R288 noise s 100k
C108 s s16 0.033u
R290 p16 0 47k
X16B p16 s16 o16 opamp {gbw}
R291 s16 o16 470k
C110 s16 o16 0.001u
P14 c109 c109 s16 100k level a0.15
C109 c109 o16 33u
R294 o16 out 1k
* the mix resistor on the jack's normal contact (Jack board B), into IC4a's virtual ground
* with a cable in the jack (plug), the contact opens and the cable's input loads the voice
SMIX out 0 !plug 33k 100k
.control plug 0
.control tuning 0.5
.control conga 0
.control level 0.8
.output out out
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
.part trig in=f R267 R271 Q56 R268 R269 Q57 R270 C99 D60 R282 R281 S10B R280 R274 D59 C100 R276
.part res in=b X18A C103 C105 S10A C104 R284 R287 P15 D84 D85 R285 R283 X18B R286
.part noise in=e,s sleep R276 C98 R273 R272 R275 Q58 R279 C101 R277 R278 C102 S10C S10D R288
.part out in=o18b,noise R289 R288 C108 R290 X16B R291 C110 P14 C109 R294 SMIX
)cir";

inline constexpr const char *lt = R"cir(* TR-808 LOW TOM / LOW CONGA, main board (service notes p.9, grid B-H 15-27).
* Inputs: trig (common trig bus), data (LT instrument data, 0/5 V), pn (pink noise from IC24b,
* main board). Output: out, IC13b's output through R238, into the LT/LC jack and the mix.
* Controls: tuning (VR11 500R B), conga (SW8: 1 = LC, 0 = LT), level (VR10 100k A).
.param r213=10k c76=0.056u c78=0.012u c77=0.047u r230=820k p11=500
VB4 b4 0 15
VB3 b3 0 15
VTRIG trig 0 input trig
VDATA data 0 input data
VPN pn 0 input pn
* trigger gate
R211 data q50b 22k
R212 q50b 0 10k
Q50 q50c q50b 0 2SC945
R213 q51b q50c {r213}
R214 trig q51b 4.7k
Q51 t q51b trig 2SA733
R215 t 0 4.7k
* the trigger into IC15a's + input; SW8 (LC) loads it with R224
C73 t a 0.0068u
D56 0 a 1S2473
R226 a b 22k
R225 b 0 4.7k
S8B b r224 conga
R224 r224 0 1.5k
* bridged-T around IC15a; SW8 (LT) adds C77 across C78. The leg from the centre to ground is
* R231 and TUNING (VR11), with the germanium pair D80/D81 across VR11's other end: as they
* conduct on the big early swings the leg's resistance drops and the pitch rises.
X15A b v o opamp {gbw} 1e5 {opsat} {sr}
C76 v m {c76}
C78 m o {c78}
S8A m c77 !conga
C77 c77 o {c77}
R228 v o 820k
R231 m tw 1k
P11 0 tw vr11a {p11} tuning
D80 m vr11a 1S188FM
D81 vr11a m 1S188FM
* IC15b inverts the output; R230 feeds it back into the centre
R229 o n15b 33k
R227 n15b o15b 33k
X15B 0 n15b o15b opamp {gbw} 1e5 {opsat} {sr}
R230 o15b m {r230}
* the tom's noise: D55/C72 hold the trigger, R218 into C71 with the pink noise (R216, R217)
* through R220 into Q52, whose collector (R221) goes through C74, R222/C75 to SW8 (LT: into the mix)
R219 t d55a 4.7k
D55 d55a e 1S2473
C72 e 0 0.039u
R218 e f 2.2meg
C71 f g 0.047u
R216 pn g 15k
R217 g 0 3.3k
R220 f q52b 33k
Q52 q52c q52b 0 2SC945
R221 b4 q52c 4.7k
C74 q52c h 0.047u
R223 h 0 22k
R222 h k 22k
C75 k 0 0.018u
S8C k noise !conga
S8D noise 0 conga
* mixer IC13b with LEVEL (VR10) in its feedback
R233 o15b s 100k
R232 noise s 100k
C81 s s13 0.047u
R234 p13 0 47k
X13B p13 s13 o13 opamp {gbw}
R235 s13 o13 470k
C83 s13 o13 0.001u
P10 c82 c82 s13 100k level a0.15
C82 c82 o13 33u
R238 o13 out 1k
* the mix resistor on the jack's normal contact (Jack board B), into IC4a's virtual ground
* with a cable in the jack (plug), the contact opens and the cable's input loads the voice
SMIX out 0 !plug 33k 100k
.control plug 0
.control tuning 0.5
.control conga 0
.control level 0.8
.output out out
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
.part trig in=f R211 R212 Q50 R213 R214 Q51 R215 C73 D56 R226 R225 S8B R224 R219 D55 C72 R218
.part res in=b X15A C76 C78 S8A C77 R228 R231 P11 D80 D81 R229 R227 X15B R230
.part noise in=e,s sleep R218 C71 R216 R217 R220 Q52 R221 C74 R223 R222 C75 S8C S8D R232
.part out in=o15b,noise R233 R232 C81 R234 X13B R235 C83 P10 C82 R238 SMIX
)cir";

inline constexpr const char *ma = R"cir(* TR-808 MARACAS (SW12 at MA), main board (service notes p.9, grid P-T 2-14; theory p.6).
* Inputs: trig (common trig bus), data (CP instrument data, 0/5 V), wn (white noise from IC24).
* Output: out (IC19b through R296). Control: level (VR17 100k A).
* Q67/Q66 open a window (C135, R345; C134, R341) in which Q66 draws current through D65 from Q65,
* whose base the white noise drives; Q68 high-passes it into the mixer IC19b, as the clap's.
.param c134=0.033u c135=0.1u c133=0.001u c132=0.001u c131=0.01u r106=10k
VB8 b8 0 15
VNB8 nb8 0 -15
VTRIG trig 0 input trig
VDATA data 0 input data
VWN wn 0 input wn
R103 data q28b 22k
R104 q28b 0 10k
Q28 q28c q28b 0 2SC945
R106 q29b q28c {r106}
R105 trig q29b 4.7k
Q29 tma q29b trig 2SA733
D67 tma m1 1S2473
C135 m1 0 {c135}
R345 m1 0 150k
R344 m1 q67b 220k
Q67 m2 q67b 0 2SC945
R343 b8 m2 100k
R341 m2 m3 470k
D66 m2 m3 1S2473
C134 m3 0 {c134}
Q66 0 m3 q66e 2SA733
R337 q66e x 10k
R335 wn r335 6.8k
C130 r335 q65b 0.047u
R336 q65b 0 1meg
Q65 q65c q65b b8 2SA733
D65 q65c x 1S2473
C131 x y {c131}
R338 y 0 15k
C132 y h {c132}
R339 h e68 3.3k
C133 h q68b {c133}
R340 q68b 0 68k
Q68 b8 q68b e68 2SC945
R347 e68 nb8 22k
C151 e68 c151 0.01u
R377 c151 s19 100k
* mixer IC19b with LEVEL (VR17) in its feedback
R373 p19 0 47k
X19B p19 s19 o19 opamp {gbw}
R375 s19 o19 470k
C149 s19 o19 220p
P17 c145 c145 s19 100k level a0.15
C145 c145 o19 33u
R296 o19 out 1k
* the mix resistor on the jack's normal contact (Jack board B), into IC4a's virtual ground
* with a cable in the jack (plug), the contact opens and the cable's input loads the voice
SMIX out 0 !plug 33k 100k
.control plug 0
.control level 0.8
.output out out
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
.part trig in=x R103 R104 Q28 R106 R105 Q29 D67 C135 R345 R344 Q67 R343 R341 D66 C134 Q66 R337
.part noise in=m3,s19 Q66 R337 R335 C130 R336 Q65 D65 C131 R338 C132 R339 C133 R340 Q68 R347
+ C151 R377
.part out in=c151 R377 R373 X19B R375 C149 P17 C145 R296 SMIX
)cir";

inline constexpr const char *metal = R"cir(* TR-808 voicing board: the metallic source's mixer and band-passes (service notes p.13). The six
* HD14584 Schmitt oscillators (IC1, on the 4.7 V rail Q9 makes) are inputs o1-o6 (their square
* waves); R50, R48, R46, R39, R37, R35 (120k each) mix them into R53. IC3b picks out the high band
* (bridged T, about 7.1 kHz) for the cymbal's Q16/Q17 and the hats' Q27/Q30, IC3a the low band
* (about 3.4 kHz) for the cymbal's Q18. Outputs: hi (IC3b pin 7), lo (IC3a pin 1).
VO1 o1 0 input o1
VO2 o2 0 input o2
VO3 o3 0 input o3
VO4 o4 0 input o4
VO5 o5 0 input o5
VO6 o6 0 input o6
R50 o1 mix 120k
R48 o2 mix 120k
R46 o3 mix 120k
R39 o4 mix 120k
R37 o5 mix 120k
R35 o6 mix 120k
R53 mix 0 1k
* high band, IC3b
C11 mix c11 0.001u
R55 c11 n3b 22k
C15 n3b m3b 0.0033u
C16 m3b hi 0.0033u
R58 m3b 0 560
R59 n3b hi 82k
X3B 0 n3b hi opamp {gbw}
* low band, IC3a
C10 mix c10 0.0033u
R52 c10 n3a 33k
C13 n3a m3a 0.0068u
C14 m3a lo 0.0068u
R56 m3a 0 560
R57 n3a lo 82k
X3A 0 n3a lo opamp {gbw}
.output hi hi
.output lo lo
)cir";

inline constexpr const char *mix = R"cir(* TR-808 mixer and master out, voicing board and Jack board B (service notes p.13). Inputs: each
* voice's output at its MULTI OUT jack, whose normal contact feeds the mix through R1-R11 into IC4a's
* virtual ground (the voices' netlists load their outputs with the same resistors). MASTER VOLUME
* VR1 (50k A) into IC4b; out: the HI master output (C1, R14) and lo (R13/R12). Control: volume.
* Input mute: the power on/off detector's point A (SN p.5: 0 V for 1-2 s after power on, else
* -15 V), whose 0 V turns Q13 on across R74: C21 then shorts IC4b's feedback and the output is
* muted while the board charges. C20 is taken as the gate's filter to ground (docs/circuit.md, §15).
* Step ticks: the common trig's edges leak into the mix (the archive.org and Synthmania units: a
* tick at both edges of every step's 1 ms pulse, about 70 dB under the voices). Not on the
* schematic and its path isn't known: a network fitted to the archive.org unit's averaged tick
* couples the trig bus into the summing node (a 30 us low-pass, high-passes of 95 and 534 us, a
* few pF into sum). Off here (rtk3 1e15); the fitted unit's values are in docs/model.md,
* "Calibration".
.param rtk3=1e15 ctk3=0.534f
* The mix resistors R1-R11 as switches, the jacks' normal contacts: closed they are the resistors, open
* (plug_<voice>, a cable in that MULTI OUT jack) the voice is out of the mix.
VBD bd 0 input bd
VSD sd 0 input sd
VLT lt 0 input lt
VMT mt 0 input mt
VHT ht 0 input ht
VRS rs 0 input rs
VCP cp 0 input cp
VOH oh 0 input oh
VCH ch 0 input ch
VCB cb 0 input cb
VCY cy 0 input cy
VMUTE mutea 0 input mute
VTRIG trig 0 input trig
SR5 bd sum !plug_bd 22k 1e15
SR4 sd sum !plug_sd 33k 1e15
SR2 lt sum !plug_lt 33k 1e15
SR1 mt sum !plug_mt 33k 1e15
SR3 ht sum !plug_ht 33k 1e15
SR6 rs sum !plug_rs 33k 1e15
SR7 cp sum !plug_cp 33k 1e15
SR10 oh sum !plug_oh 56k 1e15
SR11 ch sum !plug_ch 47k 1e15
SR8 cb sum !plug_cb 47k 1e15
SR9 cy sum !plug_cy 47k 1e15
RTK1 trig tk1 1k
CTK1 tk1 0 29.6n
CTK2 tk1 tk2 0.95n
RTK2 tk2 0 100k
CTK3 tk2 tk3 {ctk3}
RTK3 tk3 sum {rtk3}
C17 sum n4a 1u
X4A 0 n4a o4a opamp {gbw} 1e5 {opsat} {sr}
R75 n4a o4a 33k
C18 n4a o4a 220p
C19 o4a vr1 1u
P1 vr1 vw 0 50k volume a0.15
C23 vw c23 1u
R77 c23 n4b 22k
X4B 0 n4b o4b opamp {gbw} 1e5 {opsat} {sr}
R76 n4b o4b 33k
C22 n4b o4b 220p
R74 n4b c21 330k
C21 c21 o4b 33u
R73 mutea q13g 10k
C20 q13g 0 0.001u
J13 n4b q13g c21 2SK30AY
C1 o4b hi1 10u
R14 hi1 out 1k
R13 hi1 lo 27k
R12 lo 0 3.3k
RLOAD out 0 100k
.control volume 0.7
.control plug_bd 0
.control plug_sd 0
.control plug_lt 0
.control plug_mt 0
.control plug_ht 0
.control plug_rs 0
.control plug_cp 0
.control plug_cb 0
.control plug_cy 0
.control plug_oh 0
.control plug_ch 0
.output out out
.output lo lo
)cir";

inline constexpr const char *models = R"cir(* Semiconductor models for the TR-808 netlists (DC equations only: the solver has no charge
* storage). Parts from the service notes' parts list (p.16) and the note on p.8: NPNs 2SC945 (P),
* PNPs 2SA733 (P), Q23-Q26 2SA1015 (GR), diodes 1S2473 (Si) and 1S188FM (Ge), op-amps uPC4558C.
* Values: docs/circuit.md, "Semiconductors".
.model 2SC945 NPN(IS=1.1e-14 BF=300 BR=6 NF=1 NR=1 VAF=110 ISE=1e-14 NE=1.5)
.model 2SA733 PNP(IS=1.2e-14 BF=280 BR=6 NF=1 NR=1 VAF=90 ISE=2e-14 NE=1.5)
.model 2SA1015 PNP(IS=1.0e-14 BF=300 BR=6 NF=1 NR=1 VAF=90 ISE=2e-14 NE=1.5)
.model 1S2473 D(IS=2.52n N=1.752 RS=0.568)
.model 1S188FM D(IS=2e-7 N=1.3 RS=10)
.model 2SK30AY NJF(VTO=-0.9 BETA=2.5e-3 LAMBDA=0.01 IS=1e-14 N=1)
.model 2SK30AGR NJF(VTO=-1.5 BETA=2.2e-3 LAMBDA=0.01 IS=1e-14 N=1)
* Op-amps: uPC4558C, single pole, 3 MHz gain-bandwidth (NEC data sheet: typ. 3 MHz unity gain).
.param gbw=3meg
* Op-amp outputs clip at +-13.5 V (uPC4558C at +-15 V: +-13 V typical into 10k).
.param opsat=13.5
* Slew rate, V/us (uPC4558C: typ. 1 V/us), for every op-amp given opsat or ntsat (docs/model.md).
.param sr=1
)cir";

inline constexpr const char *mt = R"cir(* TR-808 MID TOM / MID CONGA, main board (service notes p.9, grid H-N 15-27). As the low tom,
* with SW9 also switching C89 across C90 (the bridged-T's input side) in MT.
* Inputs: trig, data (MT instrument data), pn (pink noise). Output: out (IC16a through R239).
* Controls: tuning (VR13 500R B), conga (SW9: 1 = MC), level (VR12 100k A).
.param r242=10k c90=0.027u c92=0.012u c91=0.027u c89=0.0082u r259=820k p13=500
VB5 b5 0 15
VTRIG trig 0 input trig
VDATA data 0 input data
VPN pn 0 input pn
R240 data q53b 22k
R241 q53b 0 10k
Q53 q53c q53b 0 2SC945
R242 q54b q53c {r242}
R243 trig q54b 4.7k
Q54 t q54b trig 2SA733
R251 t 0 4.7k
C86 t a 0.0027u
D58 0 a 1S2473
R255 a b 22k
R254 b 0 6.8k
S9B b r253 conga
R253 r253 0 2.2k
X17A b v o opamp {gbw} 1e5 {opsat} {sr}
S9C v c89 !conga
C89 c89 m {c89}
C90 v m {c90}
C92 m o {c92}
S9A m c91 !conga
C91 c91 o {c91}
R257 v o 820k
R260 m tw 1k
P13 0 tw vr13a {p13} tuning
D82 m vr13a 1S188FM
D83 vr13a m 1S188FM
R258 o n17b 33k
R256 n17b o17b 33k
X17B 0 n17b o17b opamp {gbw} 1e5 {opsat} {sr}
R259 o17b m {r259}
R244 t d57a 4.7k
D57 d57a e 1S2473
C85 e 0 0.033u
R247 e f 2.2meg
C84 f g 0.047u
R245 pn g 15k
R246 g 0 3.3k
R248 f q55b 33k
Q55 q55c q55b 0 2SC945
R252 b5 q55c 4.7k
C87 q55c h 0.033u
R249 h 0 22k
R250 h k 22k
C88 k 0 0.015u
S9D k noise !conga
S9E noise 0 conga
R262 o17b s 100k
R261 noise s 100k
C96 s s16 0.039u
R263 p16 0 47k
X16A p16 s16 o16 opamp {gbw}
R266 s16 o16 470k
C97 s16 o16 0.001u
P12 c95 c95 s16 100k level a0.15
C95 c95 o16 33u
R239 o16 out 1k
* the mix resistor on the jack's normal contact (Jack board B), into IC4a's virtual ground
* with a cable in the jack (plug), the contact opens and the cable's input loads the voice
SMIX out 0 !plug 33k 100k
.control plug 0
.control tuning 0.5
.control conga 0
.control level 0.8
.output out out
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
.part trig in=f R240 R241 Q53 R242 R243 Q54 R251 C86 D58 R255 R254 S9B R253 R244 D57 C85 R247
.part res in=b X17A S9C C89 C90 C92 S9A C91 R257 R260 P13 D82 D83 R258 R256 X17B R259
.part noise in=e,s sleep R247 C84 R245 R246 R248 Q55 R252 C87 R249 R250 C88 S9D S9E R261
.part out in=o17b,noise R262 R261 C96 R263 X16A R266 C97 P12 C95 R239 SMIX
)cir";

inline constexpr const char *mtrig = R"cir(* TR-808 voicing board: the common trig narrowed for CB, CY, OH and CH (service notes p.13; p.4:
* "trig variation range is narrowed to 7V-14V by 1/2 IC2 (pins 1-3) on the voicing board to
* increase S/N ratio"). Input: trig (the common trig bus). Output: ntrig (IC2a's output, clipping).
* An accented trig drives IC2a into its output limit, so that limit (ntsat) is the accented level.
* IC2 runs from +B1, +15 V through R165 (100) that also feeds IC5 and IC6 (three uPC4558s, 3 mA
* each typ.): about 14.1 V. The uPC4558 swings 1-2 V under its rail at these loads (+-14 V typ.
* into 10k, +-13 V into 2k; +-12 / +-10 V min.), so a typical part tops out near 12.8 V, not the
* 13.5 V of the other op-amps (opsat). The played unit's is 11.4 V (unitCalibration).
.param ntsat=12.8
VTRIG trig 0 input trig
R19 trig p2 100k
R18 trig d1a 3.3k
D1 d1a 0 1S2473
R20 d1a p2 15k
R21 p2 0 3.3k
R22 n2 0 3.3k
R23 n2 ntrig 100k
X2A p2 n2 ntrig opamp {gbw} 1e5 {ntsat} {sr}
.output ntrig ntrig
)cir";

inline constexpr const char *noise = R"cir(* TR-808 noise generator, main board (service notes p.8, the "6 lot ~" version drawn below the
* first): Q35, a "selected noise" 2SC828, has its emitter-base junction in breakdown through R128;
* its noise (input nz, a voltage at the emitter) goes through C31 into IC24a (non-inverting, gain
* set by TM4 with C200, R130 with C202 22p across it) for the white noise (wn, TP4, trimmed to
* 130 mV rms), and through IC24b's Sallen-Key low-pass (R133, R132, C32, C33; about 270 Hz) for
* the pink (pn). Controls: tm4 (the trimmer, 22k B as a rheostat).
* The first version (units before SN 010600, SN p.15 change 5) by its params: R131 300k, no C202
* (c202a=0 c202b=0 put it between ground and ground) and R127 4.7k from TM4 to ground (n200=127
* n127=127 put it under C200). That one is DC coupled and C200 isn't fitted; with no offsets in the
* model, C200 in series with R127 changes nothing above 1 Hz. Its R129 330k into pin 2 carries only
* the op-amp's bias current, which the model's hasn't: left out. IC24a's pins 1 (wn) and 2 are
* nodes "1" and "2", so that C202's ends can be params.
.param r131=100k c202a=2 c202b=1 n200=0 n127=0
VNZ nz 0 input nz
C31 nz p24 1u
R131 p24 0 {r131}
X24A p24 2 1 opamp {gbw}
R130 2 1 1meg
C202 {c202a} {c202b} 22p
P4 2 2 tm4b 22k tm4
C200 tm4b {n200} 10u
R133 1 x 22k
C32 x pn 0.039u
R132 x p24b 22k
C33 p24b 0 0.018u
X24B p24b pn pn opamp {gbw}
R127 {n127} 0 4.7k
.control tm4 0.5
.output wn 1
.output pn pn
)cir";

inline constexpr const char *rs = R"cir(* TR-808 RIM SHOT / CLAVES, main board (service notes p.9, grid L-T 28-38).
* Inputs: trig (common trig bus), data (RS instrument data, 0/5 V). Output: out (IC19a through
* R295). Controls: claves (SW11: 1 = CL), level (VR16 100k A).
* Rim shot: IC21's bridged T (about 455 Hz) and IC20a's (about 1.7 kHz with C118) drive the base
* of the swing VCA Q62, whose collector chops the envelope on C24; Q63 high-passes it.
* Claves: IC20a's bridged T at about 2.5 kHz (C118 out, R313 feeding back IC20b), straight out.
* Q74 shorts the output stage's LEVEL leg (muting IC20's leakage) until a trigger turns Q61 on.
* R324 has no value in the service notes; it alone sets the rim shot's level. The Yocto clone has
* 220k, which leaves RS 2 Vpp against the p.14 table's 3 and ~5 dB under Michael Fischer's unit
* against his other voices; 150k meets the table with the printed parts (the fitted unit: 120k).
.param c24=0.47u c115=0.0047u c116=0.0047u r298=10k r324=150k r313=390k c117=0.0022u c119=0.0022u c113=0.047u r305=1meg r318=220k r320=22k
+ c118=0.0022u
VB7 b7 0 15
VNB7 nb7 0 -15
VTRIG trig 0 input trig
VDATA data 0 input data
R297 data q59b 22k
R303 q59b 0 10k
Q59 q59c q59b 0 2SC945
R298 q60b q59c {r298}
R299 trig q60b 4.7k
Q60 t q60b trig 2SA733
R300 t 0 22k
* the output's mute: Q61 pulls Q74's gate to -15 V on a trigger, C113 lets it back up
C112 t q61b 0.022u
R304 q61b nb7 47k
Q61 g q61b nb7 2SC945
R305 g 0 {r305}
C113 g 0 {c113}
R327 g o19 470k
* rim shot envelope and swing VCA
D61 t d61k 1S2473
R107 d61k e1 1k
C24 e1 0 {c24}
R108 e1 d63a 10k
D63 d63a e 1S2473
Q62 e q62b 0 2SC945
R321 b7 q62b 2.2meg
C120 q62b vca 10u
C121 e hp 0.0047u
C122 hp q63b 0.0047u
R322 hp e63 10k
R325 q63b 0 330k
Q63 b7 q63b e63 2SC945
R323 e63 nb7 22k
R324 e63 rs {r324}
* IC21: bridged T, about 455 Hz
C111 t c111 0.01u
R307 c111 0 100k
D62 c111 d62k 1S2473
R301 d62k p21 47k
R302 p21 0 10k
X21 p21 v21 o21 opamp {gbw} 1e5 {opsat} {sr}
C116 v21 m21 {c116}
C115 m21 o21 {c115}
R315 m21 0 5.6k
R316 v21 o21 1meg
R318 o21 vca {r318}
R317 vca 0 10k
* IC20a: bridged T, about 1.7 kHz (RS, with C118) or 2.5 kHz (CL, with R313 from IC20b)
C114 t c114 0.001u
R306 c114 0 22k
D64 c114 d64k 1S2473
R310 d64k p20 33k
R311 p20 0 10k
X20A p20 v20 o20 opamp {gbw} 1e5 {opsat} {sr}
C119 v20 m20 {c119}
C117 m20 o20 {c117}
R312 m20 0 1k
R308 v20 o20 820k
S11A m20 c118 !claves
C118 c118 o20 {c118}
S11B m20 r313 claves
R313 r313 o20b {r313}
R314 o20 n20b 33k
R309 n20b o20b 33k
X20B 0 n20b o20b opamp {gbw} 1e5 {opsat} {sr}
R320 o20b vca {r320}
R319 o20b cl 220k
* SW11 picks the voice; IC19a with LEVEL (VR16) in its feedback, Q74 across VR16
S11C rs sel !claves
S11D cl sel claves
C125 sel n19 0.01u
R326 n19 s19 10k
R331 p19 0 47k
X19A p19 s19 o19 opamp {gbw}
R330 s19 o19 470k
C153 s19 o19 22p
C126 s19 c126 33u
P16 o19 o19 c126 100k level a0.15
J74 c126 g o19 2SK30AY
R295 o19 out 1k
* the mix resistor on the jack's normal contact (Jack board B), into IC4a's virtual ground
* with a cable in the jack (plug), the contact opens and the cable's input loads the voice
SMIX out 0 !plug 33k 100k
.control plug 0
.control claves 0
.control level 0.8
.output out out
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
* The resonators' input networks are in both trig (as the gate's load) and res: C114 makes a spike
* about two samples long, which the samples between partitions can't carry.
.part trig in=o19,d63a R297 R303 Q59 R298 R299 Q60 R300 C112 R304 Q61 R305 C113 R327 D61 R107
+ C24 R108 C111 R307 D62 R301 R302 C114 R306 D64 R310 R311
.part res in=t,e1,rs,cl C111 R307 D62 R301 R302 C114 R306 D64 R310 R311 R108 D63 Q62 R321 C120
+ C121 R322 C122 R325 Q63 R323 R324 X21 C116 C115 R315 R316 R318 R317 X20A C119 C117 R312 R308
+ S11A C118 S11B R313 R314 R309 X20B R320 R319
.part out in=e63,o20b,g R324 R319 S11C S11D C125 R326 R331 X19A R330 C153 C126 P16 J74 R295 SMIX
)cir";

inline constexpr const char *sd = R"cir(* TR-808 SNARE DRUM, main board (service notes p.9, grid A-H 2-14).
* Inputs: trig (common trig bus), data (SD instrument data, 0/5 V), wn (white noise from IC24,
* main board). Output: out, IC13a's output through R207, into the SD jack and the mix.
* Controls: tone (VR8 100k B), snappy (VR9 10k B), level (VR7 100k A).
* The "PORTION CHANGED" box on p.9 gives later values: C58 .056, C61 .015, R191 10k, R200 0.
.param c60=0.0068u c59=0.027u c57=0.0068u c58=0.027u c61=0.0068u r191=8.2k r200=47k r182=10k r194=2.2meg
VB3 b3 0 15
VNB3 nb3 0 -15
VTRIG trig 0 input trig
VDATA data 0 input data
VWN wn 0 input wn
* trigger gate (Q45, Q46), as the bass drum's
R180 data q45b 22k
R181 q45b 0 10k
Q45 q45c q45b 0 2SC945
R182 q46b q45c {r182}
R183 trig q46b 4.7k
Q46 t q46b trig 2SA733
* the low resonator, IC14a (about 238 Hz)
R189 t n1 100k
C57 t n1 {c57}
R190 n1 0 680
X14A n1 v1 o1 opamp {gbw} 1e5 {opsat} {sr}
C58 v1 m1 {c58}
C59 m1 o1 {c59}
R196 m1 0 680
R197 v1 o1 820k
* the high resonator, IC14b (about 476 Hz)
R191 t n2 {r191}
R192 n2 0 220
X14B n2 v2 o2 opamp {gbw} 1e5 {opsat} {sr}
C60 v2 m2 {c60}
C61 m2 o2 {c61}
R195 m2 0 2.2k
R198 v2 o2 1meg
* TONE (VR8): between the high resonator (pin 3, through R200 and C65) and the low (pin 1, C63)
R200 o2 r200b {r200}
C65 r200b t3 0.047u
C63 o1 t1 0.047u
P8 t3 tw t1 100k tone
R204 tw s 100k
* SNAPPY (VR9 off the gated trigger) charges C51 through Q47; the envelope runs down R184, R185
* into the swing VCA Q48, whose collector the white noise chops through D54.
P9 t sw r199a 10k snappy
R199 r199a 0 1k
R186 sw q47b 33k
Q47 q47c q47b e47 2SC945
R187 q47c b3f 330
R193 b3f b3 100
C55 b3f 0 47u
C51 e47 0 0.47u
R184 e47 e1 15k
C52 e1 0 0.1u
R185 e1 e2 15k
C53 e2 0 0.001u
D54 e2 x 1S2473
C54 x 0 0.0027u
Q48 x q48b q48e 2SC945
R188 q48e 0 100
R194 b3 q48b {r194}
C56 wn q48b 0.022u
* the noise's high-pass (C66, C67, R201 bootstrapped from Q49, R202) and Q49 follower
C66 e2 hp1 0.0018u
R201 hp1 e49 22k
C67 hp1 q49b 0.0018u
R202 q49b 0 47k
Q49 b3 q49b e49 2SC945
R203 e49 nb3 22k
C69 e49 r205a 0.01u
R205 r205a s 27k
* mixer IC13a with LEVEL (VR7) in its feedback
R206 p13 0 47k
X13A p13 s o13 opamp {gbw}
R210 s o13 470k
C70 s o13 220p
P7 c68 c68 s 100k level a0.15
C68 c68 o13 33u
R207 o13 out 1k
* the mix resistor on the jack's normal contact (Jack board B), into IC4a's virtual ground
* with a cable in the jack (plug), the contact opens and the cable's input loads the voice
SMIX out 0 !plug 33k 100k
.control plug 0
.control tone 0.5
.control snappy 0.5
.control level 0.8
.output out out
* Runtime partitions (docs/model.md, "Partitions"): run in this order; in= nodes come from the others.
.part trig in=e2 R180 R181 Q45 R182 R183 Q46 R189 C57 R190 R191 R192 P9 R199 R186 Q47 R187 R193
+ C55 C51 R184 C52 R185
.part res in=t,s R189 C57 R190 R191 R192 X14A C58 C59 R196 R197 X14B C60 C61 R195 R198 R200 C65
+ C63 P8 R204
.part snap in=e1,s sleep R185 C53 D54 C54 Q48 R188 R194 C56 C66 R201 C67 R202 Q49 R203 C69 R205
.part out in=tw,r205a R204 R205 R206 X13A R210 C70 P7 C68 R207 SMIX
)cir";

} // namespace tr808::netlists
