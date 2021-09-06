#ifndef RUSTFIR_H_INCLUDED
#define RUSTFIR_H_INCLUDED

#include "BaseFilter.h"
#include "rustfirlib/rust_fir.h"

/** FIR processor using an FIR engine written in Rust */
class RustFIR : public BaseFilter
{
public:
    RustFIR (int order)
    {
        filterProc.reset (rust_fir::create (order));
    }

    ~RustFIR() override = default;

    String getName() const override { return "RustFIR"; }

    void prepare (double /*sampleRate*/, int /*samplesPerBlock*/) override
    {
        rust_fir::reset (filterProc.get());
    }

    void processBlock (AudioBuffer<float>& buffer) override
    {
        rust_fir::process (filterProc.get(), buffer.getWritePointer (0), buffer.getNumSamples());
    }

    void loadIR (const AudioBuffer<float>& irBuffer) override
    {
        rust_fir::load_ir (filterProc.get(), irBuffer.getReadPointer (0), irBuffer.getNumSamples());
    }

private:
    using RustFIRPtr = std::unique_ptr<rust_fir::FirFilter, decltype(&rust_fir::destroy)>;
    RustFIRPtr filterProc {nullptr, &rust_fir::destroy};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RustFIR)
};

#endif // RUSTFIR_H_INCLUDED
