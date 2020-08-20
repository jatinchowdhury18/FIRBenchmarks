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
        z = new float[order];
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
        std::fill (z, &z[order], 0.0f); // clear existing state
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
            z[zPtr] = buffer[n]; // insert input into state

            // compute two inner products between kernel and wrapped state buffer
            y = std::inner_product (z + zPtr, z + order, h, 0.0f);
            y = std::inner_product (z, z + zPtr, h + (order - zPtr), y);

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
