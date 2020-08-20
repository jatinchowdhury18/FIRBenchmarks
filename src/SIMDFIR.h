#ifndef SIMDFIR_H_INCLUDED
#define SIMDFIR_H_INCLUDED

#include "BaseFilter.h"

template <typename T>
struct AlignedArray
{
    AlignedArray (int N)
    {
        arr_unaligned = new T[2 * N];
        arr = dsp::SIMDRegister<T>::getNextSIMDAlignedPtr (arr_unaligned);
    }

    ~AlignedArray()
    {
        delete[] arr_unaligned;
    }

    T* arr_unaligned;
    T* arr;
};

class SimdFIR : public BaseFilter
{
public:
    SimdFIR (int order) :
        order (order),
        h (order)
    {
        z = new float[2 * order];
    }

    virtual ~SimdFIR()
    {
        delete[] z;
    }

    String getName() const { return "SimdFIR"; }

    void prepare (double /*sampleRate*/, int /*samplesPerBlock*/) override
    {
        zPtr = 0;
        FloatVectorOperations::fill (z, 0.0f, 2 * order);
    }

    void loadIR (const AudioBuffer<float>& irBuffer) override
    {
        auto* data = irBuffer.getReadPointer (0);
        FloatVectorOperations::copy (h.arr, data, order);
    }

    inline dsp::SIMDRegister<float> loadUnaligned (float* x)
    {
        dsp::SIMDRegister<float> reg (0.0f);
        for (int i = 0; i < dsp::SIMDRegister<float>::SIMDNumElements; ++i)
            reg.set (i, x[i]);

        return reg;
    }

    inline float simdInnerProduct (float* in, float* kernel, int numSamples, float y = 0.0f)
    {
        constexpr size_t simdN = dsp::SIMDRegister<float>::SIMDNumElements;
        int idx = 0;
        for (; idx <= numSamples - simdN; idx += simdN)
        {
            auto simdIn = loadUnaligned (in + idx);
            auto simdKernel = dsp::SIMDRegister<float>::fromRawArray (kernel + idx);
            y += (simdIn * simdKernel).sum();
        }

        // comupte leftover samples
        y = std::inner_product (in + idx, in + numSamples, kernel + idx, y);

        return y;
    }

    void processBlock (AudioBuffer<float>& b) override
    {
        auto* buffer = b.getWritePointer (0);
        const int numSamples = b.getNumSamples();

        float y = 0.0f;
        for (int n = 0; n < numSamples; ++n)
        {
            z[zPtr] = buffer[n];
            z[zPtr + order] = buffer[n];

            y = simdInnerProduct (z + zPtr, h.arr, order);

            zPtr = (zPtr == 0 ? order - 1 : zPtr - 1);

            buffer[n] = y;
        }
    }

protected:
    AlignedArray<float> h;
    const int order;

private:
    float* z;
    int zPtr = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimdFIR)
};

#endif // SIMDFIR_H_INCLUDED
