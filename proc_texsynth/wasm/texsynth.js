// https://github.com/armory3d/armorpaint_plugins/tree/master/plugin_texsynth
// https://github.com/EmbarkStudios/texture-synthesis

let plugin = new arm.Plugin();
let h1 = new zui.Handle();

let wasmbin = Krom.loadBlob("data/plugins/texsynth_rust2048.wasm");
let module = new WebAssembly.Module(wasmbin);
let exports = new WebAssembly.Instance(module).exports;

plugin.drawUI = function(ui) {
	if (ui.panel(h1, "Texture Synthesis")) {
		if (ui.button("Make Tiling")) {
			let addr_img = exports.main_img();
			let addr_img_tile = exports.main_img_tile();
			init_image(exports.memory.buffer, addr_img, addr_img_tile);
			let addr_out = exports.main_tiling();
			write_texture(exports.memory.buffer, addr_out);
		}
	}
};

let init_image = function(ab, addr_img, addr_img_tile) {
	let l = arm.Context.layer;
	let bytes = l.texpaint.getPixels().b;
	let bytes_tile = l.texpaint_pack.getPixels().b;
	let view = new Uint8Array(ab);
	let res = arm.Config.getTextureRes();
	for (let i = 0; i < res * res * 4; ++i) {
		view[i + addr_img] = bytes[i];
		view[i + addr_img_tile] = bytes_tile[Math.ceil(i / 4) * 4];
	}
};

let write_texture = function(ab, addr) {
	let res = arm.Config.getTextureRes();
	let pixels = new arm.Bytes(new ArrayBuffer(res * res * 4))
	let view = new Uint8Array(ab);
	for (let i = 0; i < res * res * 4; ++i) {
		pixels.b[i] = view[i + addr];
	}
	let image = arm.Image.fromBytes(pixels, res, res);
	var asset = {name: "tex_synth.png", file: "/tex_synth.png", id: arm.Project.assetId++};
	iron.Data.cachedImages.h[asset.file] = image;
	arm.Project.assets.push(asset);
	arm.Project.assetNames.push(asset.name);
	arm.Project.assetMap.h[asset.id] = image;
};
