#pragma once

#include <glm/glm.hpp>
#include <hpx/hpx.hpp>
#include <sys/types.h>

#include "Log.h"

const uint64_t INVALID_VERTEX = std::numeric_limits<uint64_t>::max();
//10 MSB encode blockID --> 1024 blocks possible
const uint64_t BLOCK_INDEX_SHIFT = 54;
const uint64_t BLOCK_INDEX_MASK = 0xFFC0000000000000ull;
const uint64_t VERTEX_INDEX_MASK = ~BLOCK_INDEX_MASK;
const uint64_t INVALID_BLOCK = 0xFFC0000000000000ull;

class DataManager {
public:
    DataManager() = default;
    virtual ~DataManager() = default;

    virtual uint64_t getNumVertices() const = 0;
    virtual uint64_t getNumVerticesLocal(bool withGhost = false) const = 0;

    virtual void init(uint32_t blockIndex, uint32_t numBlocks) = 0;

private:
    // uncopyable object
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;
};


template<typename T>
class RegularGridManager : public DataManager {
public:
    // case 1 = default;
    // case 2 empty implementation
    virtual ~ RegularGridManager(){
        // TODO 
        if (this->blockData){
            delete[] this->blockData;
        }
    }

protected:
    RegularGridManager():blockData(nullptr){}

    uint64_t getNumVertices() const {
        return this->gridSize.x * this->gridSize.y * this->gridSize.z;
    }


    uint64_t getNumVerticesLocal(bool withGhost) const {
        if (withGhost)
            return this->blockSizeWithGhost.x * this->blockSizeWithGhost.y * this->blockSizeWithGhost.z;

        return this->blockSize.x * this->blockSize.y * this->blockSize.z;
    }

    virtual void init(uint32_t blockIndex, uint32_t numBlocks){
        this->gridSize = this->getSize();

        if (numBlocks > 1) {
            // Compute block size based on prime factorization of block count
            std::vector<uint32_t> primeFactors;
            uint32_t z = 2;
            uint32_t n = numBlocks;
            while (z * z <= n) {
                if (n % z == 0) {
                    primeFactors.push_back(z);
                    n = (n / z);
                } else {
                    ++z;
                }
            }
            if (n > 1)
                primeFactors.push_back(n);

            this->baseBlockSize = this->gridSize;
            for (uint32_t f : primeFactors) {
                uint32_t maxDim = 0;
                for (uint32_t d = 1; d < 3; ++d)
                    if (this->baseBlockSize[d] > this->baseBlockSize[maxDim])
                        maxDim = d;
                this->baseBlockSize[maxDim] /= f;
            }

            this->numBlocks = this->gridSize / this->baseBlockSize;

            // Compute index of local block
            n = blockIndex;
            this->blockIndex3D.z = n / (this->numBlocks.x * this->numBlocks.y);
            n = n % (this->numBlocks.x * this->numBlocks.y);
            this->blockIndex3D.y = n / this->numBlocks.x;
            this->blockIndex3D.x = n % this->numBlocks.x;

            // Determine offset and size of this block
            this->blockOffset = this->blockIndex3D * this->baseBlockSize;
            this->blockSize = this->baseBlockSize;

            // Fix size of last block along each dimension
            for (uint32_t d = 0; d < 3; ++d)
                if (this->blockIndex3D[d] == this->numBlocks[d] - 1)
                    this->blockSize[d] = this->gridSize[d] - (this->numBlocks[d] - 1) * this->baseBlockSize[d];
        } else {
            this->blockOffset = glm::uvec3(0, 0, 0);
            this->blockSize = this->gridSize;
            this->numBlocks = glm::uvec3(1, 1, 1);
            this->baseBlockSize = this->gridSize;
            this->blockIndex3D = glm::uvec3(0, 0, 0);
        }

        // Compute ghost layer
        this->blockOffsetWithGhost = this->blockOffset;
        this->blockSizeWithGhost = this->blockSize;

        glm::uvec3 beginNonGhost = glm::uvec3(0, 0, 0);
        glm::uvec3 endNonGhost = this->blockSize;

        for (uint32_t d = 0; d < 3; ++d) {
            if (this->blockIndex3D[d] > 0) {
                --this->blockOffsetWithGhost[d];
                ++this->blockSizeWithGhost[d];

                ++beginNonGhost[d];
                ++endNonGhost[d];
            }

            if (this->blockIndex3D[d] < this->numBlocks[d] - 1) {
                ++this->blockSizeWithGhost[d];
            }
        }

        //Store global index of block origin and local (negative) index of global origin
        this->blockOffsetIndex = this->blockOffsetWithGhost.x + this->blockOffsetWithGhost.y * this->gridSize.x + this->blockOffsetWithGhost.z * this->gridSize.x * this->gridSize.y;
        this->blockCoordGlobalOrigin = this->blockOffsetWithGhost.x + this->blockOffsetWithGhost.y * this->blockSizeWithGhost.x + this->blockOffsetWithGhost.z * this->blockSizeWithGhost.x * this->blockSizeWithGhost.y;

        // Compute block mask
        uint64_t numVerticesWithGhost = this->getNumVerticesLocal(true);
        this->blockMask.resize(numVerticesWithGhost, 0);

        for (uint32_t z = 0; z < this->blockSizeWithGhost.z; ++z) {
            for (uint32_t y = 0; y < this->blockSizeWithGhost.y; ++y) {
                for (uint32_t x = 0; x < this->blockSizeWithGhost.x; ++x) {
                    uint8_t mask = 0;

                    // Check if ghost
                    if (x < beginNonGhost.x || x >= endNonGhost.x || y < beginNonGhost.y || y >= endNonGhost.y || z < beginNonGhost.z || z >= endNonGhost.z)
                        mask |= 0x80;

                    // Has -x neighbor
                    if (x > 0)
                        mask |= 0x20;

                    // Has +x neighbor
                    if (x < this->blockSizeWithGhost.x - 1)
                        mask |= 0x10;

                    // Has -y neighbor
                    if (y > 0)
                        mask |= 0x8;

                    // Has +y neighbor
                    if (y < this->blockSizeWithGhost.y - 1)
                        mask |= 0x4;

                    // Has -z neighbor
                    if (z > 0)
                        mask |= 0x2;

                    // Has +z neighbor
                    if (z < this->blockSizeWithGhost.z - 1)
                        mask |= 0x1;

                    this->blockMask[z * this->blockSizeWithGhost.x * this->blockSizeWithGhost.y + y * this->blockSizeWithGhost.x + x] = mask;
                }
            }
        }

        // Read data and release internal reader memory
        this->blockData = new T[numVerticesWithGhost];
        if (this->blockData == nullptr) {
            // LogError().tag(std::to_string(blockIndex)) << "Failed to allocate block memory";
            return;
        }

        this->readBlock(this->blockOffsetWithGhost, this->blockSizeWithGhost, this->blockData);
        this->release();


        // Store block index in msb
        this->blockIndex = static_cast<uint64_t>(blockIndex) << BLOCK_INDEX_SHIFT;


        // Print info
        if (blockIndex == 0) {
            Log() << "Grid size: (" << this->gridSize.x << ", " << this->gridSize.y << ", " << this->gridSize.z << ")";
            Log() << "Num blocks: (" << this->numBlocks.x << ", " << this->numBlocks.y << ", " << this->numBlocks.z << ")";;
            Log() << "Vertices: " << this->getNumVertices();
        }
        Log().tag(std::to_string(blockIndex)) << "Block index: (" << this->blockIndex3D.x << ", " << this->blockIndex3D.y << ", " << this->blockIndex3D.z << ")";
        Log().tag(std::to_string(blockIndex)) << "Block offset: (" << this->blockOffsetWithGhost.x << ", " << this->blockOffsetWithGhost.y << ", " << this->blockOffsetWithGhost.z << ")";
        Log().tag(std::to_string(blockIndex)) << "Block size: (" << this->blockSizeWithGhost.x << ", " << this->blockSizeWithGhost.y << ", " << this->blockSizeWithGhost.z << ")";
        Log().tag(std::to_string(blockIndex)) << "Vertices (local): " << this->getNumVerticesLocal(false);
        Log().tag(std::to_string(blockIndex)) << "Vertices (ghost): " << this->getNumVerticesLocal(true) - this->getNumVerticesLocal(false);

        Log().tag(std::to_string(blockIndex)) << "Values: " << byteString(numVerticesWithGhost * sizeof(float));
        Log().tag(std::to_string(blockIndex)) << "Mask: " << byteString(numVerticesWithGhost * sizeof(uint8_t));
    }

    virtual glm::uvec3 getSize() = 0;
    virtual void readBlock(const glm::uvec3& offset, const glm::uvec3& size, T* dataOut) = 0;
    virtual void release() = 0;

private:

    uint64_t blockIndex;

    glm::uvec3 gridSize;
    glm::uvec3 numBlocks;
    glm::uvec3 blockIndex3D;
    glm::uvec3 baseBlockSize;

    // Exact block of this locality
    glm::uvec3 blockOffset;
    uint64_t blockOffsetIndex;
    uint64_t blockCoordGlobalOrigin;
    glm::uvec3 blockSize;

    // Block including ghost layer
    glm::uvec3 blockOffsetWithGhost;
    glm::uvec3 blockSizeWithGhost;

    T* blockData;
    std::vector<uint8_t> blockMask; // msb to lsb: [ghost cell, unused, -x, +x, -y, +y, -z, +z]
};