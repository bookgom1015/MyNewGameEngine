#pragma once

#include "MemoryManagement.hpp"

#include <windef.h>
#include <cmath>

namespace Common::Foundation::Core {
	namespace DataStructure {
		template <typename T, typename Alloc = MemoryManagement::ArenaAllocator>
		class Array {
		public:
			Array(UINT n);

		public:
			T& operator[](const UINT index);

		private:
			Alloc mAlloc;
			T* mpArray;
		};

		template <typename T, typename Alloc = MemoryManagement::FreeListAllocator>
		class LinkedList {
		public:
			struct Node {
				T Value;
				Node* Next;
			};

		public:
			LinkedList();

		public:
			void Push(const T& val);
			T Pop();

		private:
			Alloc mAlloc;
			Node* mpHead;
		};

		template <typename T, typename Alloc = MemoryManagement::FreeListAllocator>
		class Heap {
		public:
			Heap();

		public:
			void Push(const T& val);
			T Pop();

		private:
			__forceinline constexpr UINT GetParentIndex(UINT currIdx) const;
			__forceinline constexpr void GetChildIndices(
				UINT currIdx, UINT& left, UINT& right) const;
			__forceinline constexpr UINT ConvertToLevel(UINT index) const;

		private:
			Alloc mAlloc;
			T* mpArray[64] = { nullptr };

			UINT mLastIndex = 0;
		};

		template <typename Key, typename Value, typename Alloc = MemoryManagement::ArenaAllocator>
		class HashMap {
		public:
			struct Node {
				Key Key;
				Value Value;
				BOOL Occupied = FALSE;
				UINT HashIndex = 0;
			};

		public:
			HashMap(UINT n);

		public:
			UINT HashValue(const Key& key) const;
			UINT HashKey(const Key& key) const;

			BOOL Insert(const Key& key, const Value& value);
			Value& Get(const Key& key);
			BOOL Erase(const Key& key);

			Node* FindSlot(const Key& key) const;

			UINT ProbeDist(UINT currIdx, UINT hashIdx) const;

		private:
			Alloc mAlloc;
			Node* mpTable;

			UINT mCapacity;
		};
	}
}

#include "DataStructure.inl"