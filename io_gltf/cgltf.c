// cgltf bridge for armorpaint

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include <emscripten.h>
#include <math.h>

void* buf; /* Pointer to glb or gltf file data */
size_t size; /* Size of the file data */

cgltf_data* data = NULL;

int index_count;
int vertex_count;
unsigned int* inda = NULL;
short* posa;
short* nora;
short* texa;
float scale_pos;

unsigned int* read_u8_array(cgltf_accessor* a) {
	cgltf_buffer_view* v = a->buffer_view;
	unsigned char* ar = (unsigned char*)v->buffer->data + v->offset;
	unsigned int* res = (unsigned int*)malloc(sizeof(unsigned int) * v->size);
	for (int i = 0; i < v->size; ++i) {
		res[i] = ar[i];
	}
	return res;
}

unsigned int* read_u16_array(cgltf_accessor* a) {
	cgltf_buffer_view* v = a->buffer_view;
	unsigned short* ar = (unsigned short*)v->buffer->data + v->offset / 2;
	unsigned int* res = (unsigned int*)malloc(sizeof(unsigned int) * v->size / 2);
	for (int i = 0; i < v->size / 2; ++i) {
		res[i] = ar[i];
	}
	return res;
}

unsigned int* read_u32_array(cgltf_accessor* a) {
	cgltf_buffer_view* v = a->buffer_view;
	unsigned int* ar = (unsigned int*)v->buffer->data + v->offset / 4;
	unsigned int* res = (unsigned int*)malloc(sizeof(unsigned int) * v->size / 4);
	for (int i = 0; i < v->size / 4; ++i) {
		res[i] = ar[i];
	}
	return res;
}

float* read_f32_array(cgltf_accessor* a) {
	cgltf_buffer_view* v = a->buffer_view;
	float* ar = (float*)v->buffer->data + v->offset / 4;
	float* res = (float*)malloc(sizeof(float) * v->size / 4);
	for (int i = 0; i < v->size / 4; ++i) {
		res[i] = ar[i];
	}
	return res;
}

EMSCRIPTEN_KEEPALIVE char* init(int bufSize) {
	size = bufSize;
	buf = (char*)malloc(sizeof(char) * bufSize);
	return buf;
}

EMSCRIPTEN_KEEPALIVE void parse() {
	if (inda != NULL) {
		free(inda);
		free(posa);
		free(nora);
		free(texa);
	}

	cgltf_options options = {0};
	cgltf_result result = cgltf_parse(&options, buf, size, &data);
	if (result != cgltf_result_success) { return; }
	cgltf_load_buffers(&options, data, NULL);

	cgltf_mesh* mesh = &data->meshes[0];
	cgltf_primitive* prim = &mesh->primitives[0];

	cgltf_accessor* a = prim->indices;
	int elem_size = a->buffer_view->size / a->count;
	index_count = a->count;
	inda = elem_size == 1 ? read_u8_array(a)  :
		   elem_size == 2 ? read_u16_array(a) :
						    read_u32_array(a);

	float* posa32;
	float* nora32;
	float* texa32;
	for (int i = 0; i < prim->attributes_count; ++i) {
		cgltf_attribute* attrib = &prim->attributes[i];
		if (attrib->type == cgltf_attribute_type_position) {
			posa32 = read_f32_array(attrib->data);
		}
		else if (attrib->type == cgltf_attribute_type_normal) {
			nora32 = read_f32_array(attrib->data);
		}
		else if (attrib->type == cgltf_attribute_type_texcoord) {
			texa32 = read_f32_array(attrib->data);
		}
	}

	vertex_count = prim->attributes[0].data->count; // Assume VEC3 position

	// Pack positions to (-1, 1) range
	float hx = 0.0;
	float hy = 0.0;
	float hz = 0.0;
	for (int i = 0; i < vertex_count; ++i) {
		float f = fabsf(posa32[i * 3]);
		if (hx < f) hx = f;
		f = fabsf(posa32[i * 3 + 1]);
		if (hy < f) hy = f;
		f = fabsf(posa32[i * 3 + 2]);
		if (hz < f) hz = f;
	}
	scale_pos = fmax(hx, fmax(hy, hz));
	float inv = 1 / scale_pos;

	// Pack into 16bit
	posa = (short*)malloc(sizeof(short) * vertex_count * 4);
	nora = (short*)malloc(sizeof(short) * vertex_count * 2);
	texa = (short*)malloc(sizeof(short) * vertex_count * 2);
	for (int i = 0; i < vertex_count; ++i) {
		posa[i * 4    ] = posa32[i * 3    ] * 32767 * inv;
		posa[i * 4 + 1] = posa32[i * 3 + 1] * 32767 * inv;
		posa[i * 4 + 2] = posa32[i * 3 + 2] * 32767 * inv;
		posa[i * 4 + 3] = nora32[i * 3 + 2] * 32767;
		nora[i * 2    ] = nora32[i * 3    ] * 32767;
		nora[i * 2 + 1] = nora32[i * 3 + 1] * 32767;
		texa[i * 2    ] = texa32[i * 2    ] * 32767;
		texa[i * 2 + 1] = texa32[i * 2 + 1] * 32767;
	}

	free(posa32);
	free(nora32);
	free(texa32);
}

EMSCRIPTEN_KEEPALIVE void destroy() {
	cgltf_free(data);
	free(buf);
}

EMSCRIPTEN_KEEPALIVE int get_index_count() { return index_count; }
EMSCRIPTEN_KEEPALIVE int get_vertex_count() { return vertex_count; }
EMSCRIPTEN_KEEPALIVE float get_scale_pos() { return scale_pos; }
EMSCRIPTEN_KEEPALIVE unsigned int *get_indices() { return inda; }
EMSCRIPTEN_KEEPALIVE short *get_positions() { return posa; }
EMSCRIPTEN_KEEPALIVE short *get_normals() { return nora; }
EMSCRIPTEN_KEEPALIVE short *get_uvs() { return texa; }
