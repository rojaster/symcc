# TODO

- [X] Dockerfile to verbose
  - [X] Introduce developer image with required dependencies
  - [X] Introduce production image with binary only content
- [ ] Update dependencies
  - [ ] AFL -> AFL++
    - [ ] Update rust helper accordingly, copy from AdaCC this part
  - [ ] LLVM10 -> LLVM latest
    - [ ] Fix API accordingly
    - [ ] Might require to introduce some options for new pass manager
      - [ ] Experiment with LLVM Optimizations
  - [ ] Update Rust version
  - [X] Remove QSYM's backhanded dependencies
    - [ ] Clean up code accordingly
