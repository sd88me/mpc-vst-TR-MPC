# Lemus TR-909 core (AGPL-3.0)

From github.com/lemus-dev/lemus, commit 48f10291bd3aeb7b1b17193ebe68696549f2a114 (2026-11-17):
machines/tr909/core (the voicing board only: circuit, voiceboard, noise, hihat, cymbals,
oversampler, parallel, revisions.h) and its netlists, without the test_*.cir circuits. The CPU board
(machine, factory, upd7810) is left out: 9X9's own sequencer writes the board's latches.

generated/tr909/netlists.h comes from tools/netlists.cmake (Lemus's tools/cmake/netlists.cmake).

No local changes yet. The ROMs aren't shipped: the user supplies hn61256p__c43.ic69 (hi-hat),
__c42.ic62 (crash) and __c44.ic54 (ride), matched by CRC.
