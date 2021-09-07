#ifndef SIMDFIR_H_INCLUDED
#define SIMDFIR_H_INCLUDED

#include "BaseFilter.h"

#if JUCE_MAC
#include <Accelerate/Accelerate.h>
#endif

/** Dynamically allocated array that uses over-allocation to ensure SIMD alignment */
template <typename T>
struct AlignedArray
{
    AlignedArray (int N)
    {
        padded_length = N + dsp::SIMDRegister<T>::size();
        arr_unaligned = new T[padded_length];
        arr = dsp::SIMDRegister<T>::getNextSIMDAlignedPtr (arr_unaligned);
    }

    ~AlignedArray()
    {
        delete[] arr_unaligned;
    }

    T* arr_unaligned;
    T* arr;
    int padded_length;
};

/** FIR processor using SIMD inner product */
class SimdFIR : public BaseFilter
{
public:
    SimdFIR (int order) :
        order (order),
        h (order),
        z (2 * order)
    {
    }

    ~SimdFIR() override = default;

    String getName() const override { return "SimdFIR"; }

    void prepare (double /*sampleRate*/, int /*samplesPerBlock*/) override
    {
        zPtr = 0; // reset state pointer
        FloatVectorOperations::fill (z.arr, 0.0f, 2 * order); // clear existing state
    }

    void loadIR (const AudioBuffer<float>& irBuffer) override
    {
        auto* data = irBuffer.getReadPointer (0);
        FloatVectorOperations::copy (h.arr, data, order);
    }

    // inner product using SIMD registers
    inline float simdInnerProduct (float* in, float* kernel, int numSamples, float y = 0.0f)
    {
#if JUCE_MAC
        // On Mac we can use vDSP, which (I think) uses SIMD internally
        vDSP_dotpr (in, 1, kernel, 1, &y, numSamples);
#else
        constexpr size_t simdN = dsp::SIMDRegister<float>::SIMDNumElements;

        // compute SIMD products
        int idx = 0;
        for (; idx <= numSamples - simdN; idx += simdN)
        {
            auto simdIn = chowdsp::SIMDUtils::loadUnaligned (in + idx);
            auto simdKernel = dsp::SIMDRegister<float>::fromRawArray (kernel + idx);
            y += (simdIn * simdKernel).sum();
        }

        // compute leftover samples
        y = std::inner_product (in + idx, in + numSamples, kernel + idx, y);
#endif

        return y;
    }

    void processBlock (AudioBuffer<float>& b) override
    {
        auto* buffer = b.getWritePointer (0);
        const int numSamples = b.getNumSamples();

        float y = 0.0f;
        for (int n = 0; n < numSamples; ++n)
        {
            // load input into double-buffered state
            z.arr[zPtr] = buffer[n];
            z.arr[zPtr + order] = buffer[n];

            // compute SIMD inner product over kernel and double-buffer state
            y = simdInnerProduct (z.arr + zPtr, h.arr, order);

            zPtr = (zPtr == 0 ? order - 1 : zPtr - 1); // iterate state pointer in reverse

            buffer[n] = y;
        }
    }

protected:
    AlignedArray<float> h;
    const int order;

private:
    AlignedArray<float> z;
    int zPtr = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimdFIR)
};

#endif // SIMDFIR_H_INCLUDED
