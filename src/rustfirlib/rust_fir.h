#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <new>

namespace rust_fir {

struct FirFilter;

extern "C" {

FirFilter *create(uintptr_t order);

void destroy(FirFilter *filter);

void load_ir(FirFilter *filter, const float *ir, uintptr_t num_samples);

void process(FirFilter *filter, float *block, uintptr_t num_samples);

void reset(FirFilter *filter);

} // extern "C"

} // namespace rust_fir
