
function getString(b, pos, len) {
	var s = "";
	var fcc = String.fromCharCode;
	var i = pos;
	var max = pos+len;
	while (i < max) {
		var c = b[i++];
		if(c < 0x80) {
			if (c == 0) break;
			s += fcc(c);
		}
		else if (c < 0xE0)
			s += fcc(((c & 0x3F) << 6) | (b[i++] & 0x7F));
		else if (c < 0xF0) {
			var c2 = b[i++];
			s += fcc(((c & 0x1F) << 12) | ((c2 & 0x7F) << 6) | (b[i++] & 0x7F));
		}
		else {
			var c2 = b[i++];
			var c3 = b[i++];
			var u = ((c & 0x0F) << 18) | ((c2 & 0x7F) << 12) | ((c3 & 0x7F) << 6) | (b[i++] & 0x7F);
			// surrogate pair
			s += fcc((u >> 10) + 0xD7C0);
			s += fcc((u & 0x3FF) | 0xDC00);
		}
	}
	return s;
}

var libbin = Krom.loadBlob("data/plugins/tensor_bridge.js");
var ar = new Uint8Array(libbin, 0, libbin.byteLength);
var libstr = getString(ar, 0, ar.length);
(1, eval)(libstr);

// Tensorflow

let res = '';
let plugin = new arm.Plugin();
let h1 = new zui.Handle();

plugin.drawUI = function(ui) {
	if (ui.panel(h1, "Tensor")) {
		if (ui.button("Run")) {
			const model = tf.sequential();
			model.add(tf.layers.dense({units: 1, inputShape: [1]}));
			model.compile({loss: 'meanSquaredError', optimizer: 'sgd'});
			const xs = tf.tensor2d([1, 2, 3, 4], [4, 1]);
			const ys = tf.tensor2d([1, 3, 5, 7], [4, 1]);
			model.fit(xs, ys).then(() => {
				res = model.predict(tf.tensor2d([5], [1, 1])).toString();
			});
		}
		ui.text(res);
	}
};
