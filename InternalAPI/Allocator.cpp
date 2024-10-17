#include "Allocator.hpp"
#include <memory>


namespace Silicon {
	void* Allocator::reallocate(void* src, size_t oldsize, size_t newsize) {
		void* newmem = this->allocate(newsize);
		if (newmem == nullptr) {
			return nullptr;
		}
		size_t minsize = newsize < oldsize ? newsize : oldsize;
		std::memcpy(newmem, src, minsize);
		this->free(src);
		return newmem;
	}
}

