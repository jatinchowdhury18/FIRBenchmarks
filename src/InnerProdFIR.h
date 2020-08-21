#ifndef FIRFILTER_H_INCLUDED
#define FIRFILTER_H_INCLUDED

#include "BaseFilter.h"
#include <numeric>

/** FIR processor using std::inner_product */
class InnerProdFIR : public BaseFilter
{
public:
    InnerProdFIR (int order) :
        order (order)
    {
        // allocate memory
        // (smart pointers would be preferred, but introduce a small overhead)
        h = new float[order];
        z = new float[2 * order];
    }

    virtual ~InnerProdFIR()
    {
        // deallocate memory
        delete[] h;
        delete[] z;
    }

    String getName() const override { return "InnerProdFIR"; }

    void prepare (double /*sampleRate*/, int /*samplesPerBlock*/) override
    {
        zPtr = 0;// reset state pointer
        std::fill (z, &z[2 * order], 0.0f); // clear existing state
    }

    void loadIR (const AudioBuffer<float>& irBuffer) override
    {
        auto* data = irBuffer.getReadPointer (0);
        std::copy (data, &data[order], h);
    }

    void processBlock (AudioBuffer<float>& b) override
    {
        auto* buffer = b.getWritePointer (0);
        const int numSamples = b.getNumSamples();

        float y = 0.0f;
        for (int n = 0; n < numSamples; ++n)
        {
            // insert input into double-buffered state
            z[zPtr] = buffer[n];
            z[zPtr + order] = buffer[n];

            // compute inner product over kernel and double-buffer state
            y = std::inner_product (z + zPtr, z + zPtr + order, h, 0.0f);

            zPtr = (zPtr == 0 ? order - 1 : zPtr - 1); // iterate state pointer in reverse

            buffer[n] = y;
        }
    }

protected:
    float* h;
    const int order;

private:
    float* z;
    int zPtr = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InnerProdFIR)
};

#endif //FIRFILTER_H_INCLUDED
