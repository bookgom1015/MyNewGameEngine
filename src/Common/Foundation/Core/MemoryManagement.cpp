#include "Common/Foundation/Core/MemoryManagement.hpp"

#include <algorithm>

using namespace Common::Foundation::Core::MemoryManagement;

FreeListAllocator::FreeListAllocator(UINT blockSize, UINT numBlocks)
	: mBlockSize(std::max(static_cast<UINT>(8), blockSize)),
	mNumBlocks(numBlocks),
	mpFreeList(nullptr) {
	UINT memSize = mBlockSize * mNumBlocks;
	mpMemoryPool = std::malloc(memSize);

	for (UINT i = 0; i < mNumBlocks; ++i) {
		void* block = static_cast<char*>(mpMemoryPool) + i * mBlockSize;
		free(reinterpret_cast<void*>(block));
	}
}

FreeListAllocator::~FreeListAllocator() {
	std::free(mpMemoryPool);
}

void* FreeListAllocator::Allocate() {
	if (!mpFreeList) return nullptr;
	FreeBlock* block = mpFreeList;
	mpFreeList = mpFreeList->Next;
	return block;
}

void FreeListAllocator::Free(void* ptr) {
	FreeBlock* block = reinterpret_cast<FreeBlock*>(ptr);
	block->Next = mpFreeList;
	mpFreeList = block;
}

ArenaAllocator::ArenaAllocator(UINT size)
	: mPoolSize(size), mOffset(0) {
	mpMemoryPool = std::malloc(mPoolSize);
}

ArenaAllocator::~ArenaAllocator() {
	std::free(mpMemoryPool);
}

void* ArenaAllocator::Allocate(UINT size) {
	if (mOffset + size > mPoolSize) return nullptr;
	void* result = static_cast<char*>(mpMemoryPool) + mOffset;
	mOffset += size;
	return result;
}

void ArenaAllocator::Reset() {
	mOffset = 0;
}