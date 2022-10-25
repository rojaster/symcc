# TODO

## Backlog

- [ ] Experiment with LLVM Optimizations
- [ ] Clean up code
    - [ ] remove global cachedReadExpressions
    - [ ] remove global allocatedExpressions
    - [ ] clean up global namespace and organize symcc namespace properly
    - [X] ~~remove obsolete includes(currently we can remove pin.H)~~
    - [ ] Split h,cc files properly
    - [ ] run and fix clang-tidy issues

## In Progress

- [ ] Fix a bug with invalidation of the symbolized/concretized expression
    - [ ] try option without invalidated, regenerate z3 expression each time
    - [ ] Invalidation should be either removed or re-checked(@see TODO expr.h::427)
- [ ] Add input data dependency tests
- [ ] Update dependencies in Dockerfile(compiler, 
ubuntu version, etc)
    - [X] Use ubuntu 22.04 instead of 20.04
    - [X] Use compilers: llvm-7,12 + gcc 11.2
        - [ ] create new pass manager register hook because of llvm-13+(there is no LegacyPM anymore) 


## Completed
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