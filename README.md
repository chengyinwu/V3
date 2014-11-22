                           V3 Source Code
          Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan

       NOTE THAT V3 IS CURRENTLY FOR NON-COMMERCIAL USE ONLY !!!
       EXPLOIT V3 WITHOUT EXPLICIT PERMISSIONS IS PROHIBITED !!!

## AUTHOR: (e-mail: <author.v3@gmail.com>)

 * Cheng-Yin Wu  (e-mail: <gro070916@yahoo.com.tw>; <d99943034@ntu.edu.tw>)
 * Chung-Yang (Ric) Huang  (e-mail: <ric@cc.ee.ntu.edu.tw>)

 Affiliations: National Taiwan University, Taipei, Taiwan.

## INTRODUCTION:

 V3 is a new and extensible framework for hardware verification and 
 debugging researches on both Boolean-level and word-level designs. 
 It is a powerful tool for users and an elaborate framework for 
 developers as well. 

 The architecture of V3 consists of four logical layers: 

 1. Language layer parses in and writes out Boolean-level (e.g. AIGER) 
    as well as word-level (e.g. BTOR, Verilog) designs. 
 2. Network layer properly models design behaviours into networks while 
    preserving abstraction levels of designs. 
 3. Application layer is equipped with verification applications, including 
    design simulation. In particular, state-of-the-art SAT-based model checking 
    algorithms (e.g. bounded model checking and induction) are implemented for 
    tackling both safety and liveness properties. Furthermore, V3 confirms 
    verification results and provides counterexample visualization for 
    improving design debugging. 
 4. Solver layer adopts certain representative SAT solvers (e.g. MiniSAT) 
    as well as SMT solvers (e.g. Boolector) as core engines for formal 
    applications. 

 The communication between layers are defined by extensible APIs: Network layer 
 provides network API for network creation and information retrieval while 
 solver layer defines generic solver API for applications to communicate with 
 different solvers. 

 Due to publication issues, we reserve our implementation of several algorithms. 

## DOWNLOAD:

 Please download the latest V3 here:
    git clone https://chengyinwu@bitbucket.org/chengyinwu/V3.git

## INSTALL: 

 Please perform the following instructions under directory "engine": 
    `cd <directory-where-you-extracted-V3>; cd ./engine;`

 This release of V3 requires at least the following engines:

 - Minisat   : a satisfiability solver (SAT solver)
 - Boolector : a word-level solver (SMT solver)
 - QuteRTL   : a RTL (Verilog) front-end

 Please install these engines by executing corresponding scripts under the directory.
 Or please follow our documentation "Linking Engines with V3" for manual installation.

 - Minisat   : ./minisat.script
 - Boolector : ./boolector.script
 - QuteRTL   : ./quteRTL.script

 Now you are ready to install V3 with the procedures as described in COMPILATION.

## COMPILATION:

 `make clean; make`

## EXECUTION:

 `./v3 or ./bin/v3`

 Please also find the tutorial and documentation for V3 programmers here: 
    http://dvlab.ee.ntu.edu.tw/~publication/V3/download.html

## COMMANDS:

### ========== Common Commands : ==========

 DOfile:             Execute the commands in the dofile.

 HELp:               Print this help message.

 HIStory:            Print command history.

 Quit:               Quit the execution.

 SET LOgfile:        Redirect messages to files.

 USAGE:              Report resource usage.
 
### ========== I/O Commands : ==========

 REAd Aig:           Read AIGER Designs.

 REAd Btor:          Read BTOR Designs.

 REAd Rtl:           Read RTL (Verilog) Designs.

 WRIte Aig:          Write AIGER Designs.

 WRIte Btor:         Write BTOR Network.

 WRIte Rtl:          Write RTL (Verilog) Designs.
 
### ========== Print Commands : ==========

 PLOt NTk:           Plot Network Topology.

 PRInt NEt:          Print Net Information.

 PRInt NTk:          Print Network Information.
 
### ========== Synthesis Commands : ==========

 BLAst NTk:          Bit-blast Word-level Networks into Boolean-level Networks.

 DUPlicate NTk:      Duplicate Current Ntk from Verbosity Settings.

 EXPand NTk:         Perform Time-frame Expansion for Networks.

 FLAtten NTk:        Flatten Hierarchical Networks.

 MITer NTk:          Miter Two Networks.

 PRInt NTKVerbosity: Print Verbosities for Network Duplication.

 REDuce NTk:         Perform COI Reduction on Current Network.

 REWrite NTk:        Perform Rule-based Rewriting on Current Network.

 SET NTKVerbosity:   Set Verbosities for Network Duplication.

 STRash NTk:         Perform Structural Hashing on Current Network.
 
### ========== Manipulation Commands : ==========

 @ CD:                Change Design for Current Network.

 @ LN:                Link a Network with an instance of Current Network.

 @ LS:                List Network Instances of Current Network.

 @ PWD:               Print the Path of Current Network.
 
### ========== Extraction Commands : ==========

 ELAborate FSM:      Elaborate Network and Construct FSM from Input Specification.

 EXTract FSM:        Extract Finite State Machines from Current Network.

 PLOT FSM:           Plot Finite State Machines into *.png files.

 WRIte FSM:          Output Finite State Machines Specifications.
 
### ========== Simulation Commands : ==========

 PLOt TRace:         Plot simulation or counterexample traces.

 SIM NTk:            Simulate on Current Network.

### ========== Verification Commands : ==========

 CHEck REsult:       Verify Verification Result.

 ELAborate PRoperty: Elaborate Properties on a Duplicated Network.

 PLOt REsult:        Plot Verification Results.

 PRInt REport:       Print Verbosities for Verification Report.

 PRInt SOlver:       Print Active Solver for Verification.

 SET PRoperty:       Set Properties on Current Network.

 SET REport:         Set Verbosities for Verification Report.

 SET SOlver:         Set Active Solver for Verification.

 VERify BMC:         Perform Bounded Model Checking.

 VERify ITP:         Perform Interpolation-based Model Checking.

 VERify KLIVE:       Perform K-Liveness for Liveness Checking.

 VERify PDR:         Perform Property Directed Reachability.

 VERify SEC:         Perform Sequential Equivalence Checking.

 VERify SIM:         Perform (Constrained) Random Simulation.

 VERify UMC:         Perform Unbounded Model Checking.

 WRIte REsult:       Write Verification Results.
 
### ========== Model Checking Commands : ==========

 READ PROperty:      Read property specification from external file.

 RUN:                Run V3 Model Checker.

 WRITE PROperty:     Write property specification into file.
 
### ========== Debugging Commands : ==========

 OPTimize TRace:     Optimize a Counterexample Trace.

 SIMplify TRace:     Simplify Counterexample Traces.

## LICENSE:

 Please refer to the COPYING file.

 NOTE THAT V3 IS CURRENTLY FOR NON-COMMERCIAL USE ONLY.
