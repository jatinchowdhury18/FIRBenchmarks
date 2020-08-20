#ifndef BASEFILTER_H_INCLUDED
#define BASEFILTER_H_INCLUDED

#include <JuceHeader.h>
#include <chrono>

/** Base class for a generic FIR filter */
class BaseFilter
{
public:
    BaseFilter() {}
    virtual ~BaseFilter() {}

    /** Returns the name of this processor */
    virtual String getName() const = 0;

    /** Set sample rate and buffer size details for the processor */
    virtual void prepare (double sampleRate, int samplesPerBlock) = 0;
    
    /** Process a buffer of samples with this processor */
    virtual void processBlock (AudioBuffer<float>& buffer) = 0;

    /** Load an impulse response to process with this filter */
    virtual void loadIR (const AudioBuffer<float>& irBuffer) = 0;

    /** Return the length of time (ms) needed for this processor to process the inputBuffer */
    double runBenchMs (const AudioBuffer<float> inputBuffer, const int blockSize)
    {
        AudioBuffer<float> processBuffer (1, blockSize);

        const int numChannels = inputBuffer.getNumChannels();
        const int numSamples = inputBuffer.getNumSamples();

        // start timer
        auto start = std::chrono::high_resolution_clock::now();

        // run process
        int sampleCount = 0;
        while (sampleCount < numSamples)
        {
            int samplesToProcess = jmin (blockSize, numSamples - sampleCount);

            processBuffer.copyFrom (0, 0, inputBuffer, 0, sampleCount, samplesToProcess);

            processBlock (processBuffer);

            sampleCount += samplesToProcess;
        }

        // end timer
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds> (stop - start);

        return (double) duration.count() / 1000.0;
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BaseFilter)
};

#endif // BASEFILTER_H_INCLUDED
