// nanosvg bridge for armorpaint

#include <stdio.h>
#include <string.h>
#include <math.h>
// #define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
#include <emscripten.h>

char* buf; /* Pointer to null terminated svg string input */
unsigned char* pixels; /* Rasterized pixel output */
int w;
int h;

EMSCRIPTEN_KEEPALIVE char* init(int buf_size) {
	buf = (char*)malloc(sizeof(char) * buf_size);
	return buf;
}

EMSCRIPTEN_KEEPALIVE void parse() {
	NSVGimage* image = nsvgParse(buf, "px", 96.0f);
	w = (int)image->width;
	h = (int)image->height;
	pixels = (unsigned char*)malloc(sizeof(unsigned char) * w * h * 4);
	struct NSVGrasterizer* rast;
	rast = nsvgCreateRasterizer();
	nsvgRasterize(rast, image, 0, 0, 1, pixels, w, h, w * 4);
	nsvgDelete(image);
	nsvgDeleteRasterizer(rast);
}

EMSCRIPTEN_KEEPALIVE unsigned char* get_pixels() { return pixels; }
EMSCRIPTEN_KEEPALIVE int get_pixels_w() { return w; }
EMSCRIPTEN_KEEPALIVE int get_pixels_h() { return h; }

EMSCRIPTEN_KEEPALIVE void destroy() {
	free(buf);
	free(pixels);
}
