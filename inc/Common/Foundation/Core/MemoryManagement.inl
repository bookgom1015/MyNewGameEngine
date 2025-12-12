#ifndef __MEMORYMANAGEMENT_INL__
#define __MEMORYMANAGEMENT_INL__

template <typename T>
typename Common::Foundation::Core::MemoryManagement::CustomAllocator<T>::pointer Common::Foundation::Core::MemoryManagement::CustomAllocator<T>::allocate(size_type num, const_void_pointer hint) { 
	allocate(num); 
}

template <typename T>
typename Common::Foundation::Core::MemoryManagement::CustomAllocator<T>::pointer Common::Foundation::Core::MemoryManagement::CustomAllocator<T>::allocate(size_type num) {
	size_t memSize = sizeof(T) * num;
	return static_cast<pointer>(operator new(memSize));
}

template <typename T>
void Common::Foundation::Core::MemoryManagement::CustomAllocator<T>::deallocate(pointer ptr, size_type num) { 
	operator delete(ptr); 
}

template <typename T>
typename Common::Foundation::Core::MemoryManagement::CustomAllocator<T>::size_type Common::Foundation::Core::MemoryManagement::CustomAllocator<T>::max_size() const {
	return std::numeric_limits<size_type>::max() / sizeof(value_type);
}

#endif // __MEMORYMANAGEMENT_INL__