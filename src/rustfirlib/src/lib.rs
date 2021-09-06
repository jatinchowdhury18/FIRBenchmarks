mod fir_filter;

pub use fir_filter::FirFilter;

#[no_mangle]
pub extern "C" fn create(order: usize) -> *mut FirFilter {
    Box::into_raw(Box::new(FirFilter::new(order)))
}

#[no_mangle]
pub unsafe extern "C" fn destroy(filter: *mut FirFilter) {
    assert!(!filter.is_null());
    Box::from_raw(filter);
}

#[no_mangle]
pub unsafe extern "C" fn process(
    filter: &mut FirFilter,
    block: *mut f32,
    num_samples: usize
) {
    let block = std::slice::from_raw_parts_mut(block, num_samples);
    filter.process_block(block);
}

#[no_mangle]
pub unsafe extern "C" fn load_ir(filter: &mut FirFilter, ir: *const f32, num_samples: usize) {
    let ir = std::slice::from_raw_parts(ir, num_samples);
    filter.load_ir(ir);
}

#[no_mangle]
pub extern "C" fn reset(filter: &mut FirFilter) {
    filter.reset();
}
