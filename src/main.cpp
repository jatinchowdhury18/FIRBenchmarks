#include <JuceHeader.h>
#include "JuceConvolution.h"
#include "JuceFIR.h"
#include "InnerProdFIR.h"
#include "InnerProdNoWrapFIR.h"
#include "SIMDFIR.h"

namespace
{
    constexpr double sampleRate = 48000.0;
    constexpr double numSeconds = 10.0f;
    constexpr int numSamples = int (numSeconds * sampleRate);
    constexpr int blockSize = 512;
    constexpr int numIter = 200;

    constexpr int irSizes[] = {16, 17, 31, 32, 64, 67, 127, 128, 256, 257, 509, 512};
}

AudioBuffer<float> createRandomBuffer (Random& rand, const int size);
void testAccuracies();

int main()
{
#if JUCE_DEBUG
    testAccuracies();
#endif

    Random rand (0x1234);

    const auto inputBuffer = createRandomBuffer (rand, numSamples);
    auto irBuffer = createRandomBuffer (rand, irSizes[2]);

    // setup IR
    for (int irSize : irSizes)
    {
        std::cout << "Running with IR size: " << irSize << " samples" << std::endl;

        auto irBuffer = createRandomBuffer (rand, irSize);

        std::vector<std::unique_ptr<BaseFilter>> filters;
        filters.push_back (std::make_unique<JuceConvolution>());
        filters.push_back (std::make_unique<JuceFIR>());
        filters.push_back (std::make_unique<InnerProdFIR> (irSize));
        filters.push_back (std::make_unique<InnerProdNoWrapFIR> (irSize));
        filters.push_back (std::make_unique<SimdFIR> (irSize));

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

AudioBuffer<float> createRandomBuffer (Random& rand, const int size)
{
    AudioBuffer<float> buffer (1, size);

    for (int i = 0; i < size; ++i)
        buffer.setSample (0, i, 2.0f * rand.nextFloat() - 1.0f);

    return std::move (buffer);
}

void testAccuracies()
{
    Random rand;

    const auto irSize = 33; // irSizes[0];
    const auto testBuffer = createRandomBuffer (rand, blockSize);
    const auto irBuffer = createRandomBuffer (rand, irSize);

    auto runFIR = [=] (BaseFilter* fir) -> AudioBuffer<float>
    {
        AudioBuffer<float> copyBuff;
        copyBuff.makeCopyOf (testBuffer);
        fir->prepare (sampleRate, blockSize);
        fir->loadIR (irBuffer);
        fir->processBlock (copyBuff);

        return std::move (copyBuff);
    };

    std::unique_ptr<BaseFilter> refFIR = std::make_unique<JuceFIR>(); // reference FIR
    auto refBuffer = runFIR (refFIR.get());

    auto checkAccuracy = [&refBuffer = std::as_const (refBuffer)] (const AudioBuffer<float>& buffer)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            auto refSample = refBuffer.getSample (0, i);
            auto sample = buffer.getSample (0, i);
            jassert (isWithin (sample, refSample, 0.00001f));
        }
    };

    std::vector<std::unique_ptr<BaseFilter>> filters;
    // filters.push_back (std::make_unique<JuceConvolution>());
    filters.push_back (std::make_unique<InnerProdFIR> (irSize));
    filters.push_back (std::make_unique<InnerProdNoWrapFIR> (irSize));
    filters.push_back (std::make_unique<SimdFIR> (irSize));

    for (auto& f : filters)
    {
        std::cout << "Testing accuracy for " << f->getName() << std::endl;
        auto outBuffer = runFIR (f.get());
        checkAccuracy (outBuffer);
    }
}
