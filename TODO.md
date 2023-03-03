# TODO

## In Progress

> DO NOT FORGET TO UPDATE CODE IN REGARD WITH ADACC updates from `fuzzbench-tasex`

- [ ] Add option to compile to use required `libSymRuntime.so` depends on TaSex or SymCC hack
    - [ ] Check what libcxx linked against the binary for CXX targets
    - [ ] Think about to bring flag to switch between tasex hack and pure approach, monitor will switch
- [ ] Rust `symcc_fuzzing_helper` requires refining to confirm new changes
    - [ ] Check command timeout to run SymCC
        - Timeout is not synchronized with solver timeout.
- [ ] Fix a bug with invalidation of the symbolized/concretized expression
    - [ ] try option without invalidated, regenerate z3 expression each time
    - [ ] Invalidation should be either removed or re-checked(@see TODO expr.h::427)

## Completed
- [X] ~~Port tasex to AdaCC to integrate into FuzzBench~~
- [X] ~~Add option to set up solver timeout in milliseconds, be aware no overflow detection for negative numbers~~
- [X] ~~Add input data dependency tests~~
- [X] ~~Experiment with quick symbolization/concretization phases~~
- [X] ~~LLVM10 -> LLVM latest. Fix API accordingly(Check AdaCC)~~
- [X] ~~New algorithm to make quick slicing for symbolic expressions~~
- [X] ~~Fix std::tmpnam warning(Introduce buffer rather sending Filename to Solver)~~
- [X] ~~Update rust helper accordingly, copy from AdaCC this part~~
- [x] ~~Update Rust version~~
- [x] ~~Remove QSYM's backhanded dependencies~~
- [x] ~~Clean up code accordingly~~
- [x] ~~Dockerfile to verbose~~
- [x] ~~Introduce developer image with required dependencies~~
- [x] ~~Introduce production image with binary only content~~
- [X] ~~AFL -> AFL++~~
- [X] ~~Update dependencies in Dockerfile(compiler, ubuntu version, etc)~~
- [X] ~~Use ubuntu 22.04 instead of 20.04~~
- [X] ~~Use compilers: llvm-7,12 + gcc 11.2~~
- [X] ~~create new pass manager register hook because of llvm-13+(there is no LegacyPM anymore)~~
- [X] ~~finish new pass manager integration in adacc part for fuzzbench experimentation~~
- [X] ~~Abandon experimental features from libcxx and move to c++17+~~
- [X] ~~Fix regular expression to parse `solving_time` properly~~
- [X] ~~Fix tests related to a parsing `solving_time` accordingly~~