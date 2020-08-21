# %%
from IPython.display import display, Image

# %% [markdown]
# # Fast FIR Filtering
#
# In audio signal processing, it is often neccesary
# to process an audio stream with an impulse response (IR),
# using a technique known as [FIR filtering](https://ccrma.stanford.edu/~jos/filters/FIR_Digital_Filters.html).
# For long IRs, it is typical to use an FFT-based convolution
# algorithm to implement the filter in the frequency domain, however
# for shorter IRs these filters are often implemented in the time domain.
# As with all real-time audio processing, it is desirable to make these
# filtering algorithms as fast and efficient as possible. With that in mind
# I've decided to develop a small framework to compare the performance of
# different FIR filtering algorithms.
#
# Since I typically use the [JUCE](https://github.com/juce-framework/JUCE)
# framework for my audio applications and plugins, I decided to use JUCE for
# creating my benchmarking framework. Fortunately, JUCE also has it's own
# built-in filtering algorithms `juce::dsp::FIR`, and `juce::dsp::Convolution`
# that I could use as a reference.
# %% [markdown]
# ## FIR Filtering Background
#
# For an impulse response of length $N$, the output of a time-domain FIR
# filter can be computed as follows:
# $$
# y[n] = b_0 x[n] + b_1 x[n-1] + b_2 x[n-2] + ... b_{N-1} x[n - N + 1] = \sum_{m=0}^{N-1} b_m x[n-m]
# $$
#
# The typical way to implement something like this in code would be to use
# a loop. Indeed, this is how the built-in JUCE FIR filter is implemented.
#
# A common question is how to determine the point at which a frequency-domain
# filter is more optimal than a time-domain filter. This can be determined
# theoretically by examining the computational complexity of each operation,
# however in practical use, there are a lot of factors at play, including the
# implementation in code, the compiler used to compile the code, and the CPU
# that then runs the code. The [JUCE documentation](https://docs.juce.com/master/classdsp_1_1FIR_1_1Filter.html#details) explains:
# > Using FIRFilter is fast enough for FIRCoefficients with a size lower than 128 samples. For longer filters, it might be more efficient to use the class Convolution instead, which does the same processing in the frequency domain thanks to FFT.
#
# The JUCE Convolution implementation uses a Fast Fourier Transform (FFT) based on the
# open-source [kissfft](https://github.com/mborgerding/kissfft) implementation, however
# it also allows users to link to a faster (though more restrictively licensed) FFT
# library, [fftw](http://www.fftw.org/).
#
# What we're going to explore here is if we can develop a time-domain FIR filtering
# algorithm that can out-perform the JUCE FIR algorithm. From there we can examine
# if these improvements change the point at which switching to the JUCE Convolution
# algorithm is optimal.
# %% [markdown]
# ## Inner Product
#
# In order to obtain a faster FIR filtering algorithm, let's see if we
# can use the C++ standard library function
# [`std::inner_product`](https://en.cppreference.com/w/cpp/algorithm/inner_product)
# to avoid using extra loops. First we must define two arrays `h[]` to
# store the filter kernel, and `z[]` to store the filter state ($x[n]$, $x[n-1]$,
# $x[n-2]$, etc. from the equation above). Since it would be inefficient to
# shift the entire state buffer by a sample at every time step, we can use
# a "state pointer", `zPtr`, to iterate through the state array, and then "wrap"
# back to the beginning when it reaches the end of the array. Finally, by
# paying attention to where the state pointer needs to "wrap", we can implement
# an FIR filtering algorithm that replaces any extra loops with two calls to
# `std::inner_product`. The code example below demonstrates the algorithm.
#
# ```cpp
# struct InnerProdFIR
# {
#     int zPtr = 0; // state pointer
#     float z[FILTER_ORDER]; // filter state
#     float h[FILTER_ORDER]; // filter kernel
#
#     void process (float* buffer, int numSamples)
#     {
#         float y = 0.0f;
#         for (int n = 0; n < numSamples; ++n)
#         {
#             z[zPtr] = buffer[n]; // insert input into state
# 
#             // compute two inner products between kernel and wrapped state buffer
#             y = std::inner_product (z + zPtr, z + FILTER_ORDER, h, 0.0f);
#             y = std::inner_product (z, z + zPtr, h + (FILTER_ORDER - zPtr), y);
# 
#             zPtr = (zPtr == 0 ? FILTER_ORDER - 1 : zPtr - 1); // iterate state pointer in reverse
# 
#             buffer[n] = y;
#         }        
#     }
# };
# ```
# Something to think about is why `std::inner_product` might be faster than
# a simple for-loop? After all, some implementations of the STL use loops
# internally. This question is part of a larger conversation about why STL
# algorithms are useful in the first place, a question which (in my opinion)
# is answered convincingly in Bjarne Stroustrup's
# [The C++ Programming Language](https://www.stroustrup.com/4th.html). Regardless,
# the generic nature of the STL allows compilers to internally optimize these
# algorithms, which unfortunately, also means these implementations might perform
# differently on different machines. That said, I think it's safe to assume that
# `std::inner_product` will always perform at least as well as coding an equivalent
# algorithm by hand.
# %% [markdown]
# ## Double-Buffered Inner Product
#
# While the above implementation looks nice, I started wondering if the two calls to
# `std::inner_product` could be condensed into a single call. It turns out that they
# can, by replacing the filter state buffer with a double-buffer, thereby ensuring that
# a contiguous segment of the state data is always available for computing the inner product.
# This implementation could look something like this:
#
# ```cpp
# struct InnerProdNoWrapFIR
# {
#     int zPtr = 0; // state pointer
#     float z[2 * FILTER_ORDER]; // filter state
#     float h[FILTER_ORDER]; // filter kernel
#
#     void process (float* buffer, int numSamples)
#     {
#         float y = 0.0f;
#         for (int n = 0; n < numSamples; ++n)
#         {
#             // insert input into double-buffered state
#             z[zPtr] = buffer[n];
#             z[zPtr + FILTER_ORDER] = buffer[n];
# 
#             // compute inner product over kernel and double-buffer state
#             y = std::inner_product (z + zPtr, z + zPtr + FILTER_ORDER, h, 0.0f);
# 
#             zPtr = (zPtr == 0 ? FILTER_ORDER - 1 : zPtr - 1); // iterate state pointer in reverse
# 
#             buffer[n] = y;
#         }        
#     }
# };
# ```
# In theory, this change should allow the optimizations happening within
# `std::inner_product` to be applied to a longer contiguous stream of data, as well as
# avoiding the (albeit minimal) overhead of the second function call.
# %% [markdown]
# ## SIMD Optimization
#
# Having now determined that we can reduce the majority of the FIR filtering algorithm
# to a single call to an "inner product" function, I started thinking about the possibility
# of further optimizing the algorithm by using a custom "inner product" implementation that
# is further optimized using SIMD instructions. For those who may be unfamiliar, SIMD is
# an acronym for "Single Instruction Multiple Data", and refers to a type of parallel computing
# that allows the same operation to be computed on multiple data simultaneously. Most modern
# CPUs support some form of SIMD instructions. Fortunately, the JUCE framework contains wrappers
# that allow different types of SIMD instructions to be accessed through a common API. Using the JUCE
# style, the "inner product" function can be implemented as follows:
# ```cpp
# // inner product using SIMD registers
# inline float simdInnerProduct (float* in, float* kernel, int numSamples, float y = 0.0f)
# {
#     constexpr size_t simdN = dsp::SIMDRegister<float>::SIMDNumElements;
# 
#     // compute SIMD products
#     int idx = 0;
#     for (; idx <= numSamples - simdN; idx += simdN)
#     {
#         auto simdIn = loadUnaligned (in + idx);
#         auto simdKernel = dsp::SIMDRegister<float>::fromRawArray (kernel + idx);
#         y += (simdIn * simdKernel).sum();
#     }
# 
#     // compute leftover samples
#     y = std::inner_product (in + idx, in + numSamples, kernel + idx, y);
# 
#     return y;
# }
# ```
# Note that this implementation depends on the kernel array being aligned to the correct
# byte boundary supported by the SIMD instructions on your CPU. In theory, this parallelization
# should allow for performance improvements up to ~4x.
#
# Side note: for more algorithms implementing FIR filtering with SIMD instructions, check out
# Henry Gomersall's [SSE-Convolution](https://github.com/hgomersall/SSE-convolution).
# %% [markdown]
# ## Benchmarking Results
#
# In the above sections, we've seen some potential ways to improve the performance
# of FIR filtering algorithms. However, as Nassim Taleb often notes: "In theory, there is no
# difference between theory and practice. In practice, there is." In programming, it is typical
# to validate performance improvements with benchmarks, to show that an algorithm is sufficiently
# optimal in practice.
#
# For validating FIR filtering algorithms, I've set up a system that processes 10 seconds of
# audio at 48 kHz sample rate, using each FIR filtering algorithm. I've also set up the benchmark
# to use different sized impulse responses, both sizes that are powers of 2, and as well a prime
# numbers. For each processor and each IR size, the process is timed, and averaged over a few
# hundred iterations. Speed is then measured as the seconds of audio processed per millisecond
# of processing time.
#
# On my personal computer, a 2017 Dell laptop running Windows 10 with an Intel i7 CPU, I've obtained
# the following results:
# %%
pow2 = Image('figures/win_pow.png')
prime = Image('figures/win_prime.png')
display(pow2, prime)
# %% [markdown]
# From the above charts it's clear that the JUCE FIR processor, and the two algorithms `std::inner_product`
# are all pretty comparable, with very similar performance, especially as the IR size increases. The
# SIMD FIR algorithm seems to have the best performance of all the time-domain algorithms, for all IR
# sizes larger than 30. In fact, for most IR sizes between 32 and 128, the SIMD FIR algorithm is by far
# the most performant! That said, for IR sizes larger than 128, the JUCE Convolution algorithm starts to
# overtake the SIMD algorithm.
# %% [markdown]
# ## Finally
#
# Unfortunately, I have yet to be able to obtain reliable benchmarking data from any Mac or Linux machines.
# If you are able to run benchmarks on your machine, or if you have ideas for improving the performance of
# any of the above algorithms, I'd love any community contributions! Feel free to check out this project
# on [GitHub](https://github.com/jatinchowdhury18/JuceFIRBenchmarks).
#
# Hopefully this article has been informative, and you've learned something new about FIR filtering algorithms.
# I'm very much still learning myself, but I've found this exploration to be quite educational, and I hope to
# continue working on it in the future. Onward!
# %%
