#ifndef __DATASTRUCTURE_INL__
#define __DATASTRUCTURE_INL__

template <typename T, typename Alloc>
Common::Foundation::Core::DataStructure::Array<T, Alloc>::Array(UINT n)
	: mAlloc(sizeof(T)* n) {
	mpArray = static_cast<T*>(mAlloc.allocate(sizeof(T) * n));

	for (UINT i = 0; i < n; ++i)
		new (mpArray + i) T();
}

template <typename T, typename Alloc>
T& Common::Foundation::Core::DataStructure::Array<T, Alloc>::operator[](const UINT index) {
	auto node = mpArray + index;
	return *node;
}

template <typename T, typename Alloc>
Common::Foundation::Core::DataStructure::LinkedList<T, Alloc>::LinkedList()
	: mAlloc(sizeof(Node), 64), mpHead(nullptr) {
}

template <typename T, typename Alloc>
void Common::Foundation::Core::DataStructure::LinkedList<T, Alloc>::Push(const T& val) {
	Node* node = static_cast<Node*>(mAlloc.allocate());
	new (&node->Value) T(val);
	node->Next = mpHead;
	mpHead = node;
}

template <typename T, typename Alloc>
T Common::Foundation::Core::DataStructure::LinkedList<T, Alloc>::Pop() {
	Node* next = static_cast<Node*>(mpHead)->Next;
	T value = mpHead->Value;
	mAlloc.free(mpHead);
	mpHead = next;
	return value;
}

template <typename T, typename Alloc>
Common::Foundation::Core::DataStructure::Heap<T, Alloc>::Heap()
	: mAlloc(sizeof(T), 64) {
}

template <typename T, typename Alloc>
void Common::Foundation::Core::DataStructure::Heap<T, Alloc>::Push(const T& val) {
	T* obj = static_cast<T*>(mAlloc.allocate());
	new (obj) T(val);

	UINT curr = mLastIndex++;
	mpArray[curr] = obj;

	if (curr == 0) return;

	UINT parent = GetParentIndex(curr);

	while (curr > 0 && *mpArray[curr] < *mpArray[parent]) {
		std::swap(mpArray[curr], mpArray[parent]);
		curr = parent;
		parent = GetParentIndex(curr);
	}
}

template <typename T, typename Alloc>
T Common::Foundation::Core::DataStructure::Heap<T, Alloc>::Pop() {
	T value = *mpArray[0];
	mAlloc.free(mpArray[0]);

	if (mLastIndex == 1) {
		mLastIndex = 0;
		return value;
	}

	mpArray[0] = mpArray[--mLastIndex];

	UINT curr = 0;

	while (TRUE) {
		UINT left;
		UINT right;
		GetChildIndices(curr, left, right);

		if (left >= mLastIndex) break;

		UINT smaller = left;
		if (right < mLastIndex && *mpArray[right] < *mpArray[left])
			smaller = right;

		if (*mpArray[curr] <= *mpArray[smaller]) break;

		std::swap(mpArray[curr], mpArray[smaller]);
		curr = smaller;
	}

	return value;
}

template <typename T, typename Alloc>
constexpr UINT Common::Foundation::Core::DataStructure::Heap<T, Alloc>::GetParentIndex(UINT currIdx) const {
	return (currIdx - 1) / 2;
}

template <typename T, typename Alloc>
constexpr void Common::Foundation::Core::DataStructure::Heap<T, Alloc>::GetChildIndices(
	UINT currIdx, UINT& left, UINT& right) const {
	left = currIdx * 2 + 1;
	right = currIdx * 2 + 2;
}

template <typename T, typename Alloc>
constexpr UINT Common::Foundation::Core::DataStructure::Heap<T, Alloc>::ConvertToLevel(UINT index) const {
	return static_cast<UINT>(std::floor(static_cast<UINT>(std::log2(index + 1))));
}

template <typename Key, typename Value, typename Alloc>
Common::Foundation::Core::DataStructure::HashMap<Key, Value, Alloc>::HashMap(UINT n)
	: mAlloc(sizeof(Node)* n), mCapacity(n) {
	mpTable = static_cast<Node*>(mAlloc.allocate(sizeof(Node) * n));

	for (UINT i = 0; i < n; ++i)
		new (mpTable + i) Node();
}

template <typename Key, typename Value, typename Alloc>
UINT Common::Foundation::Core::DataStructure::HashMap<Key, Value, Alloc>::HashValue(const Key& key) const {
	if constexpr (std::is_trivially_copyable_v<Key>) {
		const unsigned char* p = reinterpret_cast<const unsigned char*>(&key);
		UINT hash = 0xcbf29ce484222325ULL;

		for (UINT i = 0; i < sizeof(Key); ++i) {
			hash ^= p[i];
			hash *= 0x100000001b3ULL;
		}

		return hash;
	}

	return std::hash<Key>{}(key);
}

template <typename Key, typename Value, typename Alloc>
UINT Common::Foundation::Core::DataStructure::HashMap<Key, Value, Alloc>::HashKey(const Key& key) const {
	if constexpr (requires(const Key & k) { k.Hash(); })
		return key.Hash() % mCapacity;
	else
		return hash_value(key) % mCapacity;
}

template <typename Key, typename Value, typename Alloc>
BOOL Common::Foundation::Core::DataStructure::HashMap<Key, Value, Alloc>::Insert(const Key& key, const Value& value) {
	UINT hash = hash_key(key);
	UINT currIdx = hash;

	Node newNode = { key, value, TRUE, currIdx };

	while (TRUE) {
		Node* node = mpTable + hash;

		if (!node->Occupied) {
			*node = newNode;
			return TRUE;
		}

		if (node->Key == key) {
			node->Value = value;
			return TRUE;
		}

		auto nodeDist = probe_dist(currIdx, node->HashIndex);
		auto newNodeDist = probe_dist(currIdx, newNode.HashIndex);

		if (newNodeDist > nodeDist) std::swap(newNode, *node);

		currIdx = (currIdx + 1) % mCapacity;
		if (currIdx == hash) return FALSE;
	}
}

template <typename Key, typename Value, typename Alloc>
Value& Common::Foundation::Core::DataStructure::HashMap<Key, Value, Alloc>::Get(const Key& key) {
	Node* node = find_slot(key);

	if (!node->Occupied) {
		node->Key = key;
		node->Value = Value{};
		node->Occupied = TRUE;
	}

	return node->Value;
}

template <typename Key, typename Value, typename Alloc>
BOOL Common::Foundation::Core::DataStructure::HashMap<Key, Value, Alloc>::Erase(const Key& key) {
	UINT hash = hash_key(key);

	UINT cnt = 0;
	UINT currIndex = hash;
	while (mpTable[currIndex].Occupied) {
		if (mpTable[currIndex].Key == key) break;
		currIndex = (currIndex + 1) % mCapacity;
		if (++cnt >= mCapacity) return FALSE;
	}

	if (!mpTable[currIndex].Occupied) return FALSE;

	mpTable[currIndex].Occupied = FALSE;

	UINT nextIndex = (currIndex + 1) % mCapacity;
	while (mpTable[nextIndex].Occupied) {
		UINT idealPos = hash_key(mpTable[nextIndex].Key);
		UINT dist = probe_dist(nextIndex, idealPos);
		if (dist == 0) break;

		mpTable[currIndex] = mpTable[nextIndex];
		mpTable[nextIndex].Occupied = FALSE;

		currIndex = nextIndex;
		nextIndex = (nextIndex + 1) % mCapacity;
	}

	return TRUE;
}

template <typename Key, typename Value, typename Alloc>
typename Common::Foundation::Core::DataStructure::HashMap<Key, Value, Alloc>::Node* Common::Foundation::Core::DataStructure::HashMap<Key, Value, Alloc>::FindSlot(const Key& key) const {
	UINT hash = hash_key(key);
	UINT cnt = 0;

	while (TRUE) {
		Node* node = mpTable + hash;

		if (!node->Occupied || node->Key == key)
			return node;

		hash = (hash + 1) % mCapacity;
		if (++cnt >= mCapacity) return nullptr;
	}
}

template <typename Key, typename Value, typename Alloc>
UINT Common::Foundation::Core::DataStructure::HashMap<Key, Value, Alloc>::ProbeDist(UINT currIdx, UINT hashIdx) const {
	return (currIdx + mCapacity - hashIdx) % mCapacity;
}

#endif // __DATASTRUCTURE_INL__