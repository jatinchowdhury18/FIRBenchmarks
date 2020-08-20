#ifndef JUCECONVOLUTION_H_INCLUDED
#define JUCECONVOLUTION_H_INCLUDED

#include "BaseFilter.h"

class JuceConvolution : public BaseFilter
{
public:
    JuceConvolution() {}
    virtual ~JuceConvolution() {}

    String getName() const { return "JuceConv"; }

    void prepare (double sampleRate, int samplesPerBlock) override
    {
        conv.prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });
        fs = sampleRate;
    }

    void processBlock (AudioBuffer<float>& buffer) override
    {
        dsp::AudioBlock<float> block (buffer);
        dsp::ProcessContextReplacing<float> ctx (block);
        conv.process (ctx);
    }

    void loadIR (const AudioBuffer<float>& irBuffer) override
    {
        conv.loadImpulseResponse (AudioBuffer<float> (irBuffer), fs, dsp::Convolution::Stereo::no,
            dsp::Convolution::Trim::no, dsp::Convolution::Normalise::no);

        conv.reset();
    }

private:
    dsp::Convolution conv;
    double fs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceConvolution)
};

#endif //  JUCECONVOLUTION_H_INCLUDED
