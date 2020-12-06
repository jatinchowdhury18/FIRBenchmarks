# FIR Filter Benchmarks

This repository contains benchmarks validating the performance
of various FIR filtering algorithms. Current algorithms include:
- [JUCE FIR Filter](https://docs.juce.com/master/classdsp_1_1FIR_1_1Filter.html)
- [JUCE Convolution](https://docs.juce.com/master/classdsp_1_1Convolution.html)
- [std::inner_product](https://en.cppreference.com/w/cpp/algorithm/inner_produc)
- SIMD-acclerated inner product

For further explanation of each algorithm, please see this
[writeup](https://ccrma.stanford.edu/~jatin/Notebooks/FIRBenchmarks.html).

## Results

So far, I have the results of the benchmarks from a Windows machine
(a 2017 Dell laptop with an Intel i7 CPU), and a Mac machine (a
2019 MacBook Pro with an Intel i9 CPU).

<img src="./results/figures/win_pow.png" alt="Pic" width="400"> <img src="./results/figures/mac_pow.png" alt="Pic" width="400">

For more results, see the `./results/figures/` directory.

Benchmark results and algorithm optimization from the community
is greatly appreciated!

## Running

If you wish to build and run the benchmarks for yourself, use
the following steps (requires `cmake`):

```bash
# clone repository
$ git clone https://github.com/FIRBenchmarks
$ cd FIRBenchmarks/

# update submodules
$ git submodule update --init

# build with CMake
$ cmake -Bbuild
$ cmake --build build --config Release

# run benchmarks
$ ./build/FIRTesting_artefacts/FIRTesting
# On Windows use this instead: ./build/FIRTesting_artefacts/Release/FIRTesting.exe
```

## License

The code in this repository is licensed under the BSD 3-clause 
license. Enjoy!
