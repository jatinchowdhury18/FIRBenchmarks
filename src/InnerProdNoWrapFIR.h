#ifndef NOWRAPFIRFILTER_H_INCLUDED
#define NOWRAPFIRFILTER_H_INCLUDED

#include "InnerProdFIR.h"
#include <numeric>

/** FIR processor using std::inner_product and double bufferring */
class InnerProdNoWrapFIR : public InnerProdFIR
{
public:
    InnerProdNoWrapFIR (int order) :
        InnerProdFIR (order)
    {
        // allocate memory
        z = new float[2 * order];
    }

    virtual ~InnerProdNoWrapFIR()
    {
        // deallocate memory
        delete[] z;
    }

    String getName() const override { return "InnerProdNoWrapFIR"; }

    void prepare (double /*sampleRate*/, int /*samplesPerBlock*/) override
    {
        zPtr = 0;// reset state pointer
        std::fill (z, &z[2 * order], 0.0f); // clear existing state
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

private:
    float* z;
    int zPtr = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InnerProdNoWrapFIR)
};

#endif //NOWRAPFIRFILTER_H_INCLUDED
