#ifndef JUCEFIR_H_INCLUDED
#define JUCEFIR_H_INCLUDED

#include "BaseFilter.h"

/** FIR processor using juce::dsp::FIR */
class JuceFIR : public BaseFilter
{
public:
    JuceFIR() {}
    virtual ~JuceFIR() {}

    String getName() const override { return "JuceFIR"; }

    void prepare (double sampleRate, int samplesPerBlock) override
    {
        filt.prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });
    }

    void processBlock (AudioBuffer<float>& buffer) override
    {
        dsp::AudioBlock<float> block (buffer);
        dsp::ProcessContextReplacing<float> ctx (block);
        filt.process (ctx);
    }

    void loadIR (const AudioBuffer<float>& irBuffer) override
    {
        filt.coefficients = new dsp::FIR::Coefficients<float> (irBuffer.getReadPointer (0), irBuffer.getNumSamples());
        filt.reset();
    }

private:
    dsp::FIR::Filter<float> filt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceFIR)
};

#endif // JUCEFIR_H_INCLUDED
