#pragma once
#include "Forward.hpp"
#include <cstdint>


namespace Silicon {

	enum class MemoryAccessMode : uint8_t {
		NONE = 0,
		READ = 1,
		WRITE = 2,
	};
	MemoryAccessMode operator |(MemoryAccessMode, MemoryAccessMode);
	MemoryAccessMode operator &(MemoryAccessMode, MemoryAccessMode);
	MemoryAccessMode operator ~(MemoryAccessMode);
	MemoryAccessMode operator ^(MemoryAccessMode, MemoryAccessMode);

	class _MemoryBlock {
		friend class Object;

	public:
		class element {
			friend class _MemoryBlock;

			_MemoryBlock* target;
			size_t index;

			element(_MemoryBlock*, size_t);

		public:
			std::byte& operator =(std::byte value);
			element& operator =(const element&) = delete;
			element& operator =(element&&) noexcept = delete;

			operator std::byte();
		};

	private:
		DECL_OPAQUE;

		_MemoryBlock(void*, size_t, MemoryAccessMode, Allocator* alloc = nullptr);
		_MemoryBlock(void*);

	public:
		_MemoryBlock(const _MemoryBlock&);
		_MemoryBlock(_MemoryBlock&&) noexcept;
		_MemoryBlock& operator =(const _MemoryBlock&);
		_MemoryBlock& operator =(_MemoryBlock&&) noexcept;

		bool readable() const;
		bool writable() const;

		_MemoryBlock as_readonly() const;
		void freeze();

		element operator [](size_t);

		size_t size() const;
	};

}

