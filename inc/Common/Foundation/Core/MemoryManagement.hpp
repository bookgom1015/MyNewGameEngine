#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <Windows.h>

#include <utility>
#include <limits>

namespace Common::Foundation::Core {
	namespace MemoryManagement {
		template <typename T>
		class CustomAllocator {
		public:
			using value_type = T;
			using pointer = T*;
			using const_pointer = const T*;
			using void_pointer = void*;
			using const_void_pointer = const void*;
			using size_type = UINT;
			using difference_type = std::ptrdiff_t;

			template <typename U>
			struct rebind {
				using Other = CustomAllocator<U>;
			};

		public:
			CustomAllocator() = default;
			virtual ~CustomAllocator() = default;

			template <typename U>
			CustomAllocator(const CustomAllocator<U>& ref) {}

		public:
			pointer allocate(size_type num, const_void_pointer hint);
			pointer	allocate(size_type num);
			void deallocate(pointer ptr, size_type num);
			size_type max_size() const;

			template <typename U, typename... Args>
			void construct(U* ptr, Args&& ...args) {
				new(ptr) U(std::forward<Args>(args)...);
			}

			template <typename U>
			void destroy(U* ptr) {
				ptr->~U();
			}
		};

		class FreeListAllocator {
		public:
			struct FreeBlock {
				FreeBlock* Next;
			};

		public:
			FreeListAllocator(UINT blockSize, UINT numBlocks);
			FreeListAllocator(const FreeListAllocator& ref) = delete;
			virtual ~FreeListAllocator();

		public:
			void* Allocate();
			void Free(void* ptr);

		private:
			FreeBlock* mpFreeList;
			void* mpMemoryPool;

			UINT mBlockSize;
			UINT mNumBlocks;
		};

		class ArenaAllocator {
		public:
			ArenaAllocator(UINT size);
			virtual ~ArenaAllocator();

		public:
			void* Allocate(UINT size);
			void Reset();

		private:
			void* mpMemoryPool;
			UINT mPoolSize;
			UINT mOffset;
		};
	}
}

#include "MemoryManagement.inl"