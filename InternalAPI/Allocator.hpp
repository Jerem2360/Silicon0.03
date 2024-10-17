#pragma once


namespace Silicon {
	class Allocator {
	public:
		virtual void* allocate(size_t) = 0;
		virtual void free(void*) = 0;
		virtual void* reallocate(void*, size_t, size_t);
	};
}

