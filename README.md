---
title: Freedom Voting Machine — ReadMe
description: Open-source paper-trail voting machine based on an auditable triple-counted consensus algorithm, semi-anonymized public paper ledger, blockchain, and AI-assisted verification.
---
The Freedom Voting Machine is an open-source paper trail voting machine system based on an auditable triple counted consensus algorithm, semi-anonymized public paper ledger, blockchain, and AI-assisted ledger verification and fraud detection.

## OneDrive

You can find all of our [OneDrive folder documents here](https://1drv.ms/u/s!AmVeQ_oN1h-3nMIWEiwK8srjm8m9Ow?e=t91k6s).

## Volunteers

We are in need of volunteers for a wide range of different skill sets from voting machine researchers to organizers, programmers, engineers, cyber-security and more. You can find instructions on how to contribute open-source to this project in the [`Contributing.md'](https://github.com/FreedomGovernment/FreedomVotingMachine/blob/master/Contributing.md) documentation, where you can also find instructions on how to join the community chat on our GitHub issue tickets.

## Status

**Code (slop pass):** As of 2026-09-11 the FVM has a working 10-step
happy path in C++ (ID scan → anonymize → paper roll → privacy mask →
seal → blockchain → triple count → AI verify → audit → lookup). It
compiles and runs on Linux (GCC 13). The code is deliberately rough
("slop") — the goal is to surface problems for a later QA pass.
See `Code/README.md` for the SLOP-SHORTCUT list and `AGENTS_PLAN.md`
for the agent operational plan.

## How It Works (Plain English)

The Freedom Voting Machine is a voting machine that you can audit with
a pen and a magnifying glass. Here is what happens when you vote:

1. **You show your ID.** The State scans your voter ID. The machine
   remembers that you showed up, but it does NOT remember who you are.
   Your name and ID number are thrown away immediately.

2. **You make your vote.** You fill out a paper ballot (or press buttons
   on the machine). This is your actual vote. It is private.

3. **The machine anonymizes your ballot.** It assigns your vote a random
   code (like a serial number on a library book). This code is the ONLY
   thing that links you to your vote — and it cannot be reversed to find
   your name.

4. **Your vote is printed to a paper roll.** The machine prints ONLY
   your line to a long roll of paper. The paper roll is the public
   record. You can look at your line and confirm it says what you voted
   for. You cannot see other people's votes — they are masked.

5. **The paper is sealed.** The roll re-rolls and is locked in a
   container. No one can go back and change a line.

6. **Your vote is added to a blockchain.** The machine records your
   anonymous vote in a digital chain. Each entry is linked to the
   previous one with a hash. If anyone tries to change an old entry,
   the chain breaks and everyone knows.

7. **Three separate counts must agree.** The machine counts your vote
   three different ways: from the digital data, from the paper text,
   and from the blockchain. All three must say the same thing. If they
   disagree, the election is flagged for a manual audit.

8. **An AI checks the ledger.** (Currently a placeholder.) In the
   future, an AI will look for patterns that suggest fraud — duplicate
   votes, impossibly fast ballots, statistical anomalies.

9. **You can look up your vote.** Later, using the code on your paper
   stub, you can check the public ledger to confirm your vote was
   counted. You cannot see who else voted — only that your vote is
   there.

The key idea: **every vote leaves a physical paper trail.** Even if the
computer is hacked, the paper roll is the ground truth. And because the
paper is sealed and the blockchain is hash-chained, no one can go back
and change a vote without everyone noticing.

## Tech Base

- **ASCII Crabs core** — the all-contiguous stack machine
  (`AStarStarship/ASCIICrabs`), the data + compute layer. Now compiles
  and runs on Linux (GCC 13, C++23) as of 2026-09-11. EDITABLE; the
  Captain holds a master copy as the rebuild source.
- **CrabsBed** (`AStarStarship/CrabsBed`, renamed from CrabsTek
  2026-09-10) — the Crabs-native embedded firmware / hardware-control
  FDK. Takes over the name of the defunct **mbed OS**; reuses
  HAL/drivers/RTOS from the local mbed tree at `~/3P/mbed-os/`.
- **SubsecondDb** (`AStarStarship/SubsecondDb`) — the full-stack
  PostgreSQL database layer (the ledger/chain storage).
- **CrabsToolkit** (`AStarStarship/CrabsToolkit`) — the tooling used
  to build the core.
- **KiCAD** — still used for the PCB/hardware.

**Method (S-tier research):** the core is built by **prompt engineering against a folder tree of markdown spec files** (`ASCIICrabs/_Spec/`), leveraging the fact that the all-contiguous stack machine makes **undo cheap** (roll back a bad generation and re-prompt without diff hell). The FVM is the forcing-function application that proves the core works: triple-counted consensus, semi-anonymized public paper ledger, blockchain, and AI-assisted ledger verification, with **SCRIPT protocol** message passing for auditability.

**Sequencing (2026-09-10 decision):** (1) vibe-code the FVM with the existing components, (2) inspect the slop for critical problems, (3) write the ASCII Crabs core update plan from what the FVM reveals. The core is *not* rebuilt until the FVM's problems tell us what it needs.

## License

Copyright [Freedom Government](https://freedomgovernment.github.io); most rights reserved, Third-party commercialization prohibited, mandatory improvement donations, licensed under the AStartup Strong Source-available License that YOU MUST CONSENT TO at <https://github.com/FreedomGovernment/FreedomVotingMachine>.

