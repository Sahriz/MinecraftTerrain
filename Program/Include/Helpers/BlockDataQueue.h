#pragma once

#include <vector>
#include <mutex>
#include <cstdint>
#include <utility>
#include <glm.hpp>

// One chunk's worth of block IDs, handed from the render thread (which reads
// them back from the GPU) to the world thread (which feeds them to physics).
// blockIDs holds the interior voxels only, indexed x + y*width + z*width*height.
struct ChunkBlockData {
    glm::vec2 coord;
    std::vector<uint16_t> blockIDs;
};

// The decoupling seam between the render thread and the world thread. The
// render thread only ever Push()es finished readbacks; the world thread only
// ever Drain()s them. Neither side knows anything about the other - they share
// only this queue, exactly like the input/active-chunk snapshots already do.
class BlockDataQueue {
public:
    void Push(glm::vec2 coord, std::vector<uint16_t>&& blockIDs) {
        std::lock_guard<std::mutex> lock(_mutex);
        _pending.push_back({ coord, std::move(blockIDs) });
    }

    // Hand every queued chunk to sink(coord, std::move(blockIDs)), then clear the
    // backlog. The lock is held only long enough to swap the backlog out, so the
    // (potentially slow) sink work never blocks the producing render thread.
    template <typename Fn>
    void Drain(Fn&& sink) {
        std::vector<ChunkBlockData> batch;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            batch.swap(_pending);
        }
        for (auto& item : batch) {
            sink(item.coord, std::move(item.blockIDs));
        }
    }

private:
    std::mutex _mutex;
    std::vector<ChunkBlockData> _pending;
};
