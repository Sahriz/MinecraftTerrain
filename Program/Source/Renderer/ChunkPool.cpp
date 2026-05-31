#include "Renderer/ChunkPool.h"

#include <utility>

ChunkPool::ChunkPool(int bucketQuads, int slotCount)
	: _bucketQuads(bucketQuads), _slotCount(slotCount) {
	const GLsizeiptr packedSize =
		static_cast<GLsizeiptr>(_slotCount) * _bucketQuads * kBytesPerQuadPacked;
	const GLsizeiptr iboSize =
		static_cast<GLsizeiptr>(_slotCount) * _bucketQuads * kBytesPerQuadIndex;

	// One big buffer for all vertex data...
	glGenBuffers(1, &_packedDataArena);
	glBindBuffer(GL_ARRAY_BUFFER, _packedDataArena);
	glBufferData(GL_ARRAY_BUFFER, packedSize, nullptr, GL_DYNAMIC_DRAW);

	// ...and one big buffer for all index data.
	glGenBuffers(1, &_iboArena);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboArena);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, iboSize, nullptr, GL_DYNAMIC_DRAW);

	// One VAO for the whole pool: attribute 0 is a uint per vertex, read from
	// the packed arena, with the index arena bound as the element buffer.
	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);
	glBindBuffer(GL_ARRAY_BUFFER, _packedDataArena);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(uint32_t), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _iboArena);
	glBindVertexArray(0);

	// Two more per-pool buffers for the single glMultiDrawElementsIndirect call.
	// Both are sized for the worst case (one entry per slot) and refilled with
	// glBufferSubData each frame, so the count we actually draw can shrink/grow
	// freely without reallocating. GL_DYNAMIC_DRAW hints "updated often, drawn
	// from often".
	//
	//  - indirect buffer: the list of draw commands the GPU walks.
	//  - offset SSBO:      one vec2 world offset per command, read in the vertex
	//                      shader via gl_DrawID so no per-chunk uniform is needed.
	glGenBuffers(1, &_indirectBuffer);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _indirectBuffer);
	glBufferData(GL_DRAW_INDIRECT_BUFFER,
		static_cast<GLsizeiptr>(_slotCount) * sizeof(DrawElementsIndirectCommand),
		nullptr, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &_offsetSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _offsetSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		static_cast<GLsizeiptr>(_slotCount) * 2 * static_cast<GLsizeiptr>(sizeof(float)),
		nullptr, GL_DYNAMIC_DRAW);

	// Every slot starts free. Push in reverse so Claim() hands out 0, 1, 2, ...
	_freeSlots.reserve(_slotCount);
	for (int s = _slotCount - 1; s >= 0; --s) {
		_freeSlots.push_back(s);
	}
}

ChunkPool::~ChunkPool() {
	Destroy();
}

void ChunkPool::Destroy() {
	if (_vao)             glDeleteVertexArrays(1, &_vao);
	if (_packedDataArena) glDeleteBuffers(1, &_packedDataArena);
	if (_iboArena)        glDeleteBuffers(1, &_iboArena);
	if (_indirectBuffer)  glDeleteBuffers(1, &_indirectBuffer);
	if (_offsetSSBO)      glDeleteBuffers(1, &_offsetSSBO);
	_vao = 0;
	_packedDataArena = 0;
	_iboArena = 0;
	_indirectBuffer = 0;
	_offsetSSBO = 0;
}

ChunkPool::ChunkPool(ChunkPool&& other) noexcept
	: _bucketQuads(other._bucketQuads),
	  _slotCount(other._slotCount),
	  _packedDataArena(other._packedDataArena),
	  _iboArena(other._iboArena),
	  _vao(other._vao),
	  _indirectBuffer(other._indirectBuffer),
	  _offsetSSBO(other._offsetSSBO),
	  _freeSlots(std::move(other._freeSlots)) {
	other._packedDataArena = 0;
	other._iboArena = 0;
	other._vao = 0;
	other._indirectBuffer = 0;
	other._offsetSSBO = 0;
}

ChunkPool& ChunkPool::operator=(ChunkPool&& other) noexcept {
	if (this != &other) {
		Destroy();
		_bucketQuads     = other._bucketQuads;
		_slotCount       = other._slotCount;
		_packedDataArena = other._packedDataArena;
		_iboArena        = other._iboArena;
		_vao             = other._vao;
		_indirectBuffer  = other._indirectBuffer;
		_offsetSSBO      = other._offsetSSBO;
		_freeSlots       = std::move(other._freeSlots);
		other._packedDataArena = 0;
		other._iboArena = 0;
		other._vao = 0;
		other._indirectBuffer = 0;
		other._offsetSSBO = 0;
	}
	return *this;
}

int ChunkPool::Claim() {
	if (_freeSlots.empty()) return -1;
	const int slot = _freeSlots.back();
	_freeSlots.pop_back();
	return slot;
}

void ChunkPool::Release(int slot) {
	_freeSlots.push_back(slot);
}
