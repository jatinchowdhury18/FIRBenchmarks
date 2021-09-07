#include <JuceHeader.h>
#include "JuceConvolution.h"
#include "JuceFIR.h"
#include "RustFIR.h"
#include "InnerProdFIR.h"
#include "SIMDFIR.h"

namespace
{
    constexpr double sampleRate = 48000.0;
    constexpr double numSeconds = 10.0;
    constexpr int numSamples = int (numSeconds * sampleRate);
    constexpr int blockSize = 512;
    constexpr int numIter = 100;

    // power of 2 and prime IR sizes
    // constexpr int irSizes[] = {16, 17, 31, 32, 64, 67, 127, 128, 256, 257, 509, 512};
    constexpr int irSizes[] = {16, 17, 31, 32, 64, 67, 127, 128};
}

AudioBuffer<float> createRandomBuffer (Random& rand, const int size);
void testAccuracies();

int main()
{
    // if running debug mode, check the accuracy
    // of each FIR processor
// #if JUCE_DEBUG
    testAccuracies();
// #endif

    std::cout << "SIMD size: " << dsp::SIMDRegister<float>::SIMDNumElements << std::endl;

    // initialize global random object with the same seed for consistency
    Random rand (0x1234);

    const auto inputBuffer = createRandomBuffer (rand, numSamples);
    for (int irSize : irSizes)
    {
        std::cout << "Running with IR size: " << irSize << " samples" << std::endl;

        auto irBuffer = createRandomBuffer (rand, irSize);

        std::vector<std::unique_ptr<BaseFilter>> filters;
        // filters.push_back (std::make_unique<JuceConvolution>());
        filters.push_back (std::make_unique<JuceFIR>());
        filters.push_back (std::make_unique<RustFIR> (irSize));
        filters.push_back (std::make_unique<InnerProdFIR> (irSize));
        filters.push_back (std::make_unique<SimdFIR> (irSize));

        // get average time (ms) to process 10 seconds of audio
        for (auto& f : filters)
        {
            double timeSum = 0.0;

            for (int i = 0; i < numIter; ++i)
            {
                f->prepare (sampleRate, blockSize);
                f->loadIR (irBuffer);
                auto time = f->runBenchMs (inputBuffer, blockSize);
                timeSum += time;
            }

            double timeAvg = timeSum / (double) numIter;
            std::cout << f->getName() << ": " << timeAvg << std::endl;
        }

        std::cout << std::endl;
    }
    
    return 0;
}

// create buffer of random values (-0.5, 0.5)
AudioBuffer<float> createRandomBuffer (Random& rand, const int size)
{
    AudioBuffer<float> buffer (1, size);

    for (int i = 0; i < size; ++i)
        buffer.setSample (0, i, 2.0f * rand.nextFloat() - 1.0f);

    return std::move (buffer);
}

// check the accuracy of each FIR processor
void testAccuracies()
{
    Random rand;

    // set up buffers
    const auto irSize = 33; // irSizes[0];
    const auto testBuffer = createRandomBuffer (rand, blockSize);
    const auto irBuffer = createRandomBuffer (rand, irSize);

    // process with an FIR processor
    auto runFIR = [=] (BaseFilter* fir) -> AudioBuffer<float>
    {
        AudioBuffer<float> copyBuff;
        copyBuff.makeCopyOf (testBuffer);
        fir->prepare (sampleRate, blockSize);
        fir->loadIR (irBuffer);
        fir->processBlock (copyBuff);

        return std::move (copyBuff);
    };

    // Use JUCE FIR as reference processor
    std::unique_ptr<BaseFilter> refFIR = std::make_unique<JuceFIR>();
    auto refBuffer = runFIR (refFIR.get());

    // check that all samples in the buffer are with range of the reference
    auto checkAccuracy = [&refBuffer = std::as_const (refBuffer)] (const AudioBuffer<float>& buffer)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            auto refSample = refBuffer.getSample (0, i);
            auto sample = buffer.getSample (0, i);
            jassert (isWithin (sample, refSample, 0.00001f));
        }
    };

    // Run check for each fliter
    std::vector<std::unique_ptr<BaseFilter>> filters;
    filters.push_back (std::make_unique<InnerProdFIR> (irSize));
    filters.push_back (std::make_unique<RustFIR> (irSize));
    filters.push_back (std::make_unique<SimdFIR> (irSize));

    for (auto& f : filters)
    {
        std::cout << "Testing accuracy for " << f->getName() << std::endl;
        auto outBuffer = runFIR (f.get());
        checkAccuracy (outBuffer);
    }

    std::cout << "Done checking accuracy!" << std::endl;
}
