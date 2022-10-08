# TODO

## Backlog

- [ ] Experiment with LLVM Optimizations
- [ ] Experiment with quick symbolization/concretization phases
- [ ] Clean up code
    - [ ] remove global cachedReadExpressions
    - [ ] remove global allocatedExpressions
    - [ ] clean up global namespace and organize symcc namespace properly
    - [X] ~~remove obsolete includes(currently we can remove pin.H)~~
    - [ ] Split h,cc files properly
    - [ ] run and fix clang-tidy issues

## In Progress

- [ ] Update dependencies in Dockerfile(compiler, 
ubuntu version, etc)
- [ ] LLVM10 -> LLVM latest. Fix API accordingly(Check AdaCC)

## Completed

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