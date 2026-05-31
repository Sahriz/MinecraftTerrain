#pragma once

#include <vector>
#include <cstdint>

#include "Core.h"   // pulls in the OpenGL loader (GLuint, glGenBuffers, ...)

// A fixed-size-bucket pool allocator for chunk geometry.
//
// Instead of one little pair of GPU buffers per chunk (which the driver rounds
// up to a memory page each, wasting hundreds of MB), a pool owns TWO big GPU
// buffers ("arenas") carved into equal-size "slots". Every slot can hold up to
// `bucketQuads` quads. A chunk borrows one slot, writes its geometry into that
// slot's byte range, and hands the slot back when it unloads.
//
// One pool == one size class. Use a small-bucket pool for flat/cheap chunks and
// a big-bucket pool for dense ones, so a tiny chunk never reserves a huge slot.

// One command per chunk for glMultiDrawElementsIndirect. The field order and
// sizes MUST match the GL spec exactly (5 tightly-packed 32-bit fields) - the
// driver reads this straight out of the GL_DRAW_INDIRECT_BUFFER.
struct DrawElementsIndirectCommand {
	GLuint count;         // indices for this chunk = quadCount * 6
	GLuint instanceCount; // always 1 (we are not instancing)
	GLuint firstIndex;    // start in the index arena, in INDEX units = FirstIndex(slot)
	GLint  baseVertex;    // added to every index = BaseVertex(slot)
	GLuint baseInstance;  // always 0
};

class ChunkPool {
public:
	// NOTE: a current OpenGL context is required - construct AFTER GL init.
	ChunkPool(int bucketQuads, int slotCount);
	~ChunkPool();

	ChunkPool(const ChunkPool&) = delete;
	ChunkPool& operator=(const ChunkPool&) = delete;
	ChunkPool(ChunkPool&& other) noexcept;
	ChunkPool& operator=(ChunkPool&& other) noexcept;

	// Borrow a free slot index, or -1 if the pool is full.
	int Claim();
	// Give a slot index back so it can be reused.
	void Release(int slot);

	bool HasFreeSlot() const { return !_freeSlots.empty(); }
	int  FreeSlotCount() const { return static_cast<int>(_freeSlots.size()); }

	int BucketQuads() const { return _bucketQuads; }
	int SlotCount()   const { return _slotCount; }

	GLuint Vao()            const { return _vao; }
	GLuint PackedArena()    const { return _packedDataArena; }
	GLuint IboArena()       const { return _iboArena; }
	GLuint IndirectBuffer() const { return _indirectBuffer; } // the multidraw command list
	GLuint OffsetSSBO()     const { return _offsetSSBO; }     // per-draw chunk offsets (binding 0)

	// Where a slot lives inside each arena (tight packing, no padding).
	GLintptr PackedByteOffset(int slot) const {
		return static_cast<GLintptr>(slot) * _bucketQuads * kBytesPerQuadPacked;
	}
	GLintptr IboByteOffset(int slot) const {
		return static_cast<GLintptr>(slot) * _bucketQuads * kBytesPerQuadIndex;
	}

	// For a future single glMultiDrawElementsIndirect call: these feed the
	// per-chunk draw command so chunk-local (0-based) indices land in the right
	// slot without touching the shader.
	GLint  BaseVertex(int slot) const { return slot * _bucketQuads * kVertsPerQuad; }
	GLuint FirstIndex(int slot) const { return static_cast<GLuint>(slot) * _bucketQuads * kIndicesPerQuad; }

private:
	// 1 quad = 4 vertices (one uint32 each) + 6 indices (one uint16 each).
	// Keep bucketQuads <= 16383 so a slot's local indices fit in uint16.
	static constexpr int kVertsPerQuad       = 4;
	static constexpr int kIndicesPerQuad     = 6;
	static constexpr int kBytesPerQuadPacked = kVertsPerQuad   * static_cast<int>(sizeof(uint32_t)); // 16
	static constexpr int kBytesPerQuadIndex  = kIndicesPerQuad * static_cast<int>(sizeof(uint16_t)); // 12

	void Destroy();

	int _bucketQuads = 0;
	int _slotCount   = 0;

	GLuint _packedDataArena = 0; // vertex/packed data for every slot
	GLuint _iboArena        = 0; // index data for every slot
	GLuint _vao             = 0; // describes how to read attribute 0 from the arena
	GLuint _indirectBuffer  = 0; // one DrawElementsIndirectCommand per slot, rebuilt each frame
	GLuint _offsetSSBO      = 0; // one vec2 chunk offset per draw, indexed by gl_DrawID

	std::vector<int> _freeSlots; // stack of currently-unused slot indices
};
