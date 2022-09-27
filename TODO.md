# TODO

## Backlog

- [ ] Experiment with LLVM Optimizations
## In Progress

- [ ] Update dependencies in Dockerfile(compiler, 
ubuntu version, etc)
- [ ] LLVM10 -> LLVM latest. Fix API accordingly(Check AdaCC)
- [ ] New algorithm to make quick slicing for symbolic expressions
- [ ] Fix std::tmpnam warning(Introduce buffer rather sending Filename to Solver)

## Completed

- [X] ~~Update rust helper accordingly, copy from AdaCC this part~~
- [x] ~~Update Rust version~~
- [x] ~~Remove QSYM's backhanded dependencies~~
- [x] ~~Clean up code accordingly~~
- [x] ~~Dockerfile to verbose~~
- [x] ~~Introduce developer image with required dependencies~~
- [x] ~~Introduce production image with binary only content~~
- [X] ~~AFL -> AFL++~~