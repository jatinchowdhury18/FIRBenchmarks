pub struct FirFilter {
    order: usize,
    z_ptr: usize,
    h: Vec<f32>,
    z: Vec<f32>,
}

impl FirFilter {
    pub fn new(order: usize) -> Self {
        FirFilter {
            order: order,
            z_ptr: 0,
            h: Vec::with_capacity(order),
            z: Vec::with_capacity(2 * order),
        }
    }

    pub fn reset(&mut self) {
        self.h.resize(self.order, 0.0);
        self.z.resize(2 * self.order, 0.0);
        self.z_ptr = 0;
    }

    pub fn load_ir(&mut self, ir: &[f32]) {
        self.h.copy_from_slice(ir);
    }

    fn inner_product(a: &[f32], b: &[f32]) -> f32 {
        let mut result: f32 = 0.0;
        for (_, (aval, bval)) in a.iter().zip(b).enumerate() {
            result += aval * bval;
        }
        result
    }

    pub fn process_block(&mut self, block: &mut [f32]) {
        for (_, samp) in block.iter_mut().enumerate() {
            // insert input into double-buffered state
            self.z[self.z_ptr] = *samp;
            self.z[self.z_ptr + self.order] = *samp;

            // compute inner product over kernel and double-buffer state
            let z_slice = &self.z[self.z_ptr .. self.z_ptr + self.order];
            let y = FirFilter::inner_product(z_slice, &self.h);

            // iterate state pointer in reverse
            self.z_ptr = if self.z_ptr == 0 { self.order - 1 } else { self.z_ptr - 1 };

            *samp = y;
        }
    }
}
