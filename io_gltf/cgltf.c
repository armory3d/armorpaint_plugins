// cgltf bridge for armorpaint

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include <emscripten.h>
#include <math.h>

void* buf; /* Pointer to glb or gltf file data */
size_t size; /* Size of the file data */

cgltf_options options = {0};
cgltf_data* data = NULL;

int* inda;
int inda_count;
short* posa;
short* nora;
short* texa;
int vert_count;

int* readU8Array(cgltf_accessor* a) {
	cgltf_buffer_view* v = a->buffer_view;
	cgltf_buffer* b = v->buffer;
	int* res = (int*)malloc(sizeof(int) * a->count);
	int pos = v->offset;
	int i = 0;
	while (pos < v->offset + v->size) {
		char* ar = (char*)b->data;
		res[i] = ar[pos];
		pos += 1;
		i++;
	}
	return res;
}

int* readU16Array(cgltf_accessor* a) {
	cgltf_buffer_view* v = a->buffer_view;
	cgltf_buffer* b = v->buffer;
	int* res = (int*)malloc(sizeof(int) * a->count);
	int pos = v->offset;
	int i = 0;
	while (pos < v->offset + v->size) {
		short* ar = (short*)b->data;
		res[i] = ar[pos];
		pos += 2;
		i++;
	}
	return res;
}

int* readU32Array(cgltf_accessor* a) {
	cgltf_buffer_view* v = a->buffer_view;
	cgltf_buffer* b = v->buffer;
	int* res = (int*)malloc(sizeof(int) * a->count);
	int pos = v->offset;
	int i = 0;
	while (pos < v->offset + v->size) {
		int* ar = (int*)b->data;
		res[i] = ar[pos];
		pos += 4;
		i++;
	}
	return res;
}

float* readF32Array(cgltf_accessor* a) {
	cgltf_buffer_view* v = a->buffer_view;
	cgltf_buffer* b = v->buffer;
	float* res = (float*)malloc(sizeof(float) * a->count);
	int pos = v->offset;
	int i = 0;
	while (pos < v->offset + v->size) {
		float* ar = (float*)b->data;
		res[i] = ar[pos];
		pos += 4;
		i++;
	}
	return res;
}

EMSCRIPTEN_KEEPALIVE char* init(int bufSize) {
	size = bufSize;
	buf = (char*)malloc(sizeof(char) * bufSize);
	return buf;
}

EMSCRIPTEN_KEEPALIVE void parse() {
	cgltf_result result = cgltf_parse(&options, buf, size, &data);
	if (result != cgltf_result_success) { return; }

	cgltf_mesh* mesh = &data->meshes[0];
	cgltf_primitive* prim = &mesh->primitives[0];

	cgltf_accessor* a = prim->indices;
	int elemSize = a->buffer_view->size / a->count;
	inda_count = a->count;
	inda = elemSize == 1 ? readU8Array(a)  :
		   elemSize == 2 ? readU16Array(a) :
						   readU32Array(a);

	float* posa32;
	float* nora32;
	float* texa32;
	for (int i = 0; i < prim->attributes_count; ++i) {
		cgltf_attribute* attrib = &prim->attributes[i];
		cgltf_accessor* a = attrib->data; // type
		if (attrib->type == cgltf_attribute_type_position) {
			posa32 = readF32Array(a);
		}
		else if (attrib->type == cgltf_attribute_type_normal) {
			nora32 = readF32Array(a);
		}
		else if (attrib->type == cgltf_attribute_type_texcoord) {
			texa32 = readF32Array(a);
		}
	}

	vert_count = prim->attributes[0].data->count / 3; // Assume position

	// Pack positions to (-1, 1) range
	float hx = 0.0;
	float hy = 0.0;
	float hz = 0.0;
	for (int i = 0; i < vert_count; ++i) {
		float f = fabsf(posa32[i * 3]);
		if (hx < f) hx = f;
		f = fabsf(posa32[i * 3 + 1]);
		if (hy < f) hy = f;
		f = fabsf(posa32[i * 3 + 2]);
		if (hz < f) hz = f;
	}
	float scalePos = fmax(hx, fmax(hy, hz));
	float inv = 1 / scalePos;

	// Pack into 16bit
	posa = (short*)malloc(sizeof(short) * vert_count * 4);
	nora = (short*)malloc(sizeof(short) * vert_count * 2);
	texa = (short*)malloc(sizeof(short) * vert_count * 2);
	for (int i = 0; i < vert_count; ++i) {
		posa[i * 4    ] = posa32[i * 3    ] * 32767 * inv;
		posa[i * 4 + 1] = posa32[i * 3 + 1] * 32767 * inv;
		posa[i * 4 + 2] = posa32[i * 3 + 2] * 32767 * inv;
		posa[i * 4 + 3] = nora32[i * 3 + 2] * 32767;
		nora[i * 2    ] = nora32[i * 3    ] * 32767;
		nora[i * 2 + 1] = nora32[i * 3 + 1] * 32767;
		texa[i * 2    ] = texa32[i * 2    ] * 32767;
		texa[i * 2 + 1] = texa32[i * 2 + 1] * 32767;
	}
}

EMSCRIPTEN_KEEPALIVE void destroy() {
	cgltf_free(data);
}

EMSCRIPTEN_KEEPALIVE int *getIndices() { return inda; }
EMSCRIPTEN_KEEPALIVE int getIndexCount() { return inda_count; }
EMSCRIPTEN_KEEPALIVE int getVertexCount() { return vert_count; }
EMSCRIPTEN_KEEPALIVE short *getPositions() { return posa; }
EMSCRIPTEN_KEEPALIVE short *getNormals() { return nora; }
EMSCRIPTEN_KEEPALIVE short *getUVs() { return texa; }
