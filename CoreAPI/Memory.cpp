#include "Memory.hpp"
#include "../InternalAPI/ObjectMemory.hpp"
#include "../InternalAPI/Allocator.hpp"


namespace Silicon {
	struct _MemoryMetadata {
		bool is_raw;
		uint32_t refcnt;
		void* address;
		size_t size;
		Allocator* allocator;

		inline void free_target() {
			if (!this->address)
				return;
			if (!this->is_raw)
				return;
			if (this->allocator)
				this->allocator->free(this->address);
			else
				delete[] reinterpret_cast<std::byte*>(this->address);
		}
		inline void incRef() {
			if (!this->refcnt)
				return;
			this->refcnt++;
		}
		inline void decRef() {
			this->refcnt--;
			
			if (!this->refcnt) {
				this->free_target();
				delete this;
			}
		}
	};

	OPAQUE_DEF(_MemoryBlock) {
		_MemoryMetadata* memory;
		MemoryAccessMode access;

		inline void set(size_t index, std::byte value) {
			if (!this->memory) {
				throw std::bad_variant_access();
			}
			if (index >= this->memory->size) {
				throw std::bad_variant_access();
			}
			auto bytes = reinterpret_cast<std::byte*>(this->memory->address);
			bytes[index] = value;
		}
		inline std::byte& get(size_t index) {
			if (!this->memory) {
				throw std::bad_variant_access();
			}
			if (index >= this->memory->size) {
				throw std::bad_variant_access();
			}
			auto bytes = reinterpret_cast<std::byte*>(this->memory->address);
			return bytes[index];
		}
		inline void copy(_MemoryBlock* dst) const {
			dst->impl->memory = this->memory;
			this->memory->incRef();
			dst->impl->access = this->access;
		}
		inline void move(_MemoryBlock* dst) {
			dst->impl->memory = this->memory;
			dst->impl->access = this->access;
			this->memory = nullptr;
		}
		inline void release() {
			this->memory->decRef();
			this->memory = nullptr;
		}
		inline bool isnull() {
			if (!this->memory)
				return true;
			if (!this->memory->address) {
				this->release();
				return true;
			}
			return false;
		}
	};

	MemoryAccessMode operator |(MemoryAccessMode lhs, MemoryAccessMode rhs) {
		return static_cast<MemoryAccessMode>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
	}
	MemoryAccessMode operator &(MemoryAccessMode lhs, MemoryAccessMode rhs) {
		return static_cast<MemoryAccessMode>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
	}
	MemoryAccessMode operator ~(MemoryAccessMode op) {
		return static_cast<MemoryAccessMode>(~static_cast<uint8_t>(op));
	}
	MemoryAccessMode operator ^(MemoryAccessMode lhs, MemoryAccessMode rhs) {
		return static_cast<MemoryAccessMode>(static_cast<uint8_t>(lhs) ^ static_cast<uint8_t>(rhs));
	}

	_MemoryBlock::element::element(_MemoryBlock* target, size_t index) :
		target(target), index(index)
	{}
	std::byte& _MemoryBlock::element::operator=(std::byte value) {
		if (!this->target->writable())
			throw std::bad_variant_access();

		this->target->impl->set(this->index, value);
		return this->target->impl->get(this->index);
	}
	_MemoryBlock::element::operator std::byte() {
		if (!this->target->readable())
			throw std::bad_variant_access();
		return this->target->impl->get(this->index);
	}

	_MemoryBlock::_MemoryBlock(void* where, size_t size, MemoryAccessMode access, Allocator* alloc) {
		this->impl->memory = new _MemoryMetadata();
		this->impl->memory->incRef();
		this->impl->memory->address = where;
		this->impl->memory->size = static_cast<uint32_t>(size);
		this->impl->memory->allocator = alloc;
		this->impl->memory->is_raw = true;
		this->impl->access = access;
	}
	void on_obj_free(void* pv_block) {
		auto block = reinterpret_cast<_MemoryMetadata*>(pv_block);
		block->address = nullptr;
	}
	_MemoryBlock::_MemoryBlock(void* most_derived) {
		auto objmem = InternalAPI::ObjectMemory::from_most_derived(most_derived);
		auto layout = reinterpret_cast<InternalAPI::MemoryLayout*>(objmem.layout_ptr_address());
		this->impl->memory = new _MemoryMetadata();  
		this->impl->memory->address = most_derived;
		this->impl->memory->size = layout->totalsize();
		this->impl->memory->allocator = nullptr;
		this->impl->memory->is_raw = false;
		this->impl->access = MemoryAccessMode::READ;
		objmem.on_free(&on_obj_free, this->impl->memory);
	}
	_MemoryBlock::_MemoryBlock(const _MemoryBlock& other) {
		other.impl->copy(this);
	}
	_MemoryBlock::_MemoryBlock(_MemoryBlock&& other) noexcept {
		other.impl->move(this);
	}
	_MemoryBlock& _MemoryBlock::operator=(const _MemoryBlock& other) {
		auto tmp = this->impl->memory;
		other.impl->copy(this);
		if (tmp)
			tmp->decRef();
		return *this;
	}
	_MemoryBlock& _MemoryBlock::operator=(_MemoryBlock&& other) noexcept {
		auto tmp = this->impl->memory;
		other.impl->move(this);
		if (tmp)
			tmp->decRef();
		return *this;
	}
	bool _MemoryBlock::readable() const {
		if (this->impl->isnull())
			return false;
		return (this->impl->access & MemoryAccessMode::READ) != MemoryAccessMode::NONE;
	}
	bool _MemoryBlock::writable() const {
		if (this->impl->isnull())
			return false;
		return (this->impl->access & MemoryAccessMode::WRITE) != MemoryAccessMode::NONE;
	}
	_MemoryBlock _MemoryBlock::as_readonly() const {
		_MemoryBlock result(*this);
		result.freeze();
		return result;
	}
	void _MemoryBlock::freeze() {
		if (this->impl->isnull())
			return;
		MemoryAccessMode new_access = MemoryAccessMode::NONE;
		if (this->readable())
			new_access = MemoryAccessMode::READ;
		this->impl->access = new_access;
	}
	_MemoryBlock::element _MemoryBlock::operator[](size_t index) {
		return element(this, index);
	}
	size_t _MemoryBlock::size() const {
		if (this->impl->isnull())
			return 0;
		return this->impl->memory->size;
	}
}

