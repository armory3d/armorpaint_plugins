
// Expose bridge to armorcore/C

use texture_synthesis::{
	Dims, Example, Session,
};

#[no_mangle]
pub extern fn texsynth_inpaint(w: libc::size_t, h: libc::size_t, output_ptr: *const u8, image_ptr: *const u8, mask_ptr: *const u8, tiling: bool) {
	let len8 = (w * h) as usize;
	let len32 = (w * h * 4) as usize;
	let output_vec = unsafe { std::slice::from_raw_parts_mut(output_ptr as *mut u8, len32) };
	let image_vec = unsafe { std::slice::from_raw_parts(image_ptr as *mut u8, len32).to_vec() };
	let mask_vec = unsafe { std::slice::from_raw_parts(mask_ptr as *mut u8, len8).to_vec() };
	internal_texsynth_inpaint(w as u32, h as u32, output_vec, image_vec, mask_vec, tiling);
}

fn internal_texsynth_inpaint(w: u32, h: u32, output_ptr: &mut [u8], image_ptr: Vec<u8>, mask_ptr: Vec<u8>, tiling: bool) {
	let image_buffer = image::ImageBuffer::from_raw(w, h, image_ptr).unwrap();
	let image = image::DynamicImage::ImageRgba8(image_buffer);
	let mask_buffer = image::ImageBuffer::from_raw(w, h, mask_ptr).unwrap();
	let mask = image::DynamicImage::ImageLuma8(mask_buffer);
	let mut example = Example::new(image);
	if !tiling {
		example.set_sample_method(mask.clone());
	}
	let texsynth = Session::builder()
		.inpaint_example(mask, example, Dims::new(w, h))
		.tiling_mode(tiling)
		.build().unwrap();
	let generated = texsynth.run(None);
	let pixels = generated.into_image().to_bytes();
	for i in 0..pixels.len() {
		output_ptr[i] = pixels[i];
	}
}
