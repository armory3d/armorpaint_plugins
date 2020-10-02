
#include "tinyusdz/tinyusdz.hh"
#include <emscripten.h>

uint8_t* buf; /* Pointer to usdc file data */
size_t size; /* Size of the file data */

int index_count;
int vertex_count;
unsigned int* inda = NULL;
short* posa;
short* nora;
short* texa;
float scale_pos;

EMSCRIPTEN_KEEPALIVE uint8_t* init(int bufSize) {
	size = bufSize;
	buf = (uint8_t*)malloc(sizeof(uint8_t) * bufSize);
	return buf;
}

EMSCRIPTEN_KEEPALIVE void parse() {
	if (inda != NULL) {
		free(inda);
		free(posa);
		free(nora);
		free(texa);
	}

	tinyusdz::Scene scene;
	std::string warn;
	std::string err;
	tinyusdz::LoadUSDCFromMemory(buf, size, &scene, &warn, &err);
	tinyusdz::GeomMesh mesh = scene.geom_meshes[0];

	std::vector<float> vertices = mesh.points.buffer.GetAsVec3fArray();
	std::vector<uint32_t> dst_facevarying_indices;
	std::vector<float> dst_facevarying_normals;
	std::vector<float> dst_facevarying_texcoords;

	std::vector<float> facevarying_normals;
	mesh.GetFacevaryingNormals(&facevarying_normals);
	std::vector<float> facevarying_texcoords;
	mesh.GetFacevaryingTexcoords(&facevarying_texcoords);

	// Make facevarying indices
    size_t face_offset = 0;
    for (size_t fid = 0; fid < mesh.faceVertexCounts.size(); fid++) {
        int f_count = mesh.faceVertexCounts[fid];
        if (f_count == 3) {
            for (size_t f = 0; f < f_count; f++) {
	            dst_facevarying_indices.push_back(mesh.faceVertexIndices[face_offset + f]);
	            if (facevarying_normals.size()) {
	                dst_facevarying_normals.push_back(facevarying_normals[3 * (face_offset + f) + 0]);
	                dst_facevarying_normals.push_back(facevarying_normals[3 * (face_offset + f) + 1]);
	                dst_facevarying_normals.push_back(facevarying_normals[3 * (face_offset + f) + 2]);
	            }
	            if (facevarying_texcoords.size()) {
	                dst_facevarying_texcoords.push_back(facevarying_texcoords[2 * (face_offset + f) + 0]);
	                dst_facevarying_texcoords.push_back(facevarying_texcoords[2 * (face_offset + f) + 1]);
	            }
            }
        }
        else {
	        // Simple triangulation with triangle-fan decomposition
	        for (size_t f = 0; f < f_count - 2; f++) {
	            size_t f0 = 0;
	            size_t f1 = f + 1;
	            size_t f2 = f + 2;
	            size_t fid0 = face_offset + f0;
            	size_t fid1 = face_offset + f1;
            	size_t fid2 = face_offset + f2;
	            dst_facevarying_indices.push_back(mesh.faceVertexIndices[fid0]);
	            dst_facevarying_indices.push_back(mesh.faceVertexIndices[fid1]);
	            dst_facevarying_indices.push_back(mesh.faceVertexIndices[fid2]);
	            if (facevarying_normals.size()) {
		            dst_facevarying_normals.push_back(facevarying_normals[3 * fid0 + 0]);
		            dst_facevarying_normals.push_back(facevarying_normals[3 * fid0 + 1]);
		            dst_facevarying_normals.push_back(facevarying_normals[3 * fid0 + 2]);

		            dst_facevarying_normals.push_back(facevarying_normals[3 * fid1 + 0]);
		            dst_facevarying_normals.push_back(facevarying_normals[3 * fid1 + 1]);
		            dst_facevarying_normals.push_back(facevarying_normals[3 * fid1 + 2]);

		            dst_facevarying_normals.push_back(facevarying_normals[3 * fid2 + 0]);
		            dst_facevarying_normals.push_back(facevarying_normals[3 * fid2 + 1]);
		            dst_facevarying_normals.push_back(facevarying_normals[3 * fid2 + 2]);
	            }
	            if (facevarying_texcoords.size()) {
	            	size_t fid0 = face_offset + f0;
	            	size_t fid1 = face_offset + f1;
	            	size_t fid2 = face_offset + f2;

		            dst_facevarying_texcoords.push_back(facevarying_texcoords[2 * fid0 + 0]);
		            dst_facevarying_texcoords.push_back(facevarying_texcoords[2 * fid0 + 1]);

		            dst_facevarying_texcoords.push_back(facevarying_texcoords[2 * fid1 + 0]);
		            dst_facevarying_texcoords.push_back(facevarying_texcoords[2 * fid1 + 1]);

		            dst_facevarying_texcoords.push_back(facevarying_texcoords[2 * fid2 + 0]);
		            dst_facevarying_texcoords.push_back(facevarying_texcoords[2 * fid2 + 1]);
	            }
	        }
        }
        face_offset += f_count;
    }

    vertex_count = dst_facevarying_indices.size();
    index_count = dst_facevarying_indices.size();

    // Pack positions to (-1, 1) range
	float hx = 0.0;
	float hy = 0.0;
	float hz = 0.0;
	for (int i = 0; i < vertices.size() / 3; ++i) {
		float f = fabsf(vertices[i * 3]);
		if (hx < f) hx = f;
		f = fabsf(vertices[i * 3 + 1]);
		if (hy < f) hy = f;
		f = fabsf(vertices[i * 3 + 2]);
		if (hz < f) hz = f;
	}
	scale_pos = fmax(hx, fmax(hy, hz));
	float inv = 1 / scale_pos;

	// Pack into 16bit
	inda = (unsigned int*)malloc(sizeof(unsigned int) * index_count);
	for (int i = 0; i < index_count; ++i) {
		inda[i] = i;
	}
	posa = (short*)malloc(sizeof(short) * vertex_count * 4);
	nora = (short*)malloc(sizeof(short) * vertex_count * 2);
	texa = (short*)malloc(sizeof(short) * vertex_count * 2);

	for (int i = 0; i < vertex_count; ++i) {
		posa[i * 4    ] = vertices[dst_facevarying_indices[i] * 3    ] * 32767 * inv;
		posa[i * 4 + 1] = vertices[dst_facevarying_indices[i] * 3 + 1] * 32767 * inv;
		posa[i * 4 + 2] = vertices[dst_facevarying_indices[i] * 3 + 2] * 32767 * inv;
		posa[i * 4 + 3] = dst_facevarying_normals[i * 3 + 2] * 32767;
		nora[i * 2    ] = dst_facevarying_normals[i * 3    ] * 32767;
		nora[i * 2 + 1] = dst_facevarying_normals[i * 3 + 1] * 32767;
		texa[i * 2    ] = dst_facevarying_texcoords[i * 2    ] * 32767;
		texa[i * 2 + 1] = (1.0 - dst_facevarying_texcoords[i * 2 + 1]) * 32767;
	}
}

EMSCRIPTEN_KEEPALIVE void destroy() {
	free(buf);
}

EMSCRIPTEN_KEEPALIVE int get_index_count() { return index_count; }
EMSCRIPTEN_KEEPALIVE int get_vertex_count() { return vertex_count; }
EMSCRIPTEN_KEEPALIVE float get_scale_pos() { return scale_pos; }
EMSCRIPTEN_KEEPALIVE unsigned int *get_indices() { return inda; }
EMSCRIPTEN_KEEPALIVE short *get_positions() { return posa; }
EMSCRIPTEN_KEEPALIVE short *get_normals() { return nora; }
EMSCRIPTEN_KEEPALIVE short *get_uvs() { return texa; }
