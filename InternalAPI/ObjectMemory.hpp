#pragma once
#include <cstdlib>
#include <string>
#include <cstdint>
#include <concepts>
#include <functional>
#include "Allocator.hpp"
#include "../byteworkaround.hpp"
#include "../macros.hpp"
#include "../inthandling.hpp"


namespace Silicon {


	namespace InternalAPI {

		struct ObjectHead;  // do not export the complete definition to the outside world.

		class bad_allocmethod : std::exception {
		public:
			inline bad_allocmethod() : exception("bad allocation method.") {}
		};

		class bad_allocator : std::exception {
		public:
			inline bad_allocator() : exception("bad allocator.") {}
		};

		enum class AllocationMethod : uint8_t {
			NONE,
			EXTERNAL,
			NEW,
			ALLOCATOR
		};

		struct MemoryLayout {
			size_t c_size;
			size_t c_root_offset;
			size_t c_pad;
			uint16_t fieldcount;

			size_t totalsize() const;
			void* c_most_derived(void*) const;
			void* c_root(void*) const;
			void* fields(void*) const;

			MemoryLayout(size_t c_size, uint16_t fieldcount, size_t c_align, size_t c_root_offset = inthandling::int_max<size_t>);

			template<class T>
			static consteval size_t totalsizeof(const size_t fieldCount) {
				return 24 + sizeof(T) + (alignof(T) - (sizeof(T) % alignof(T))) + (fieldCount * sizeof(void*));
			}
		};


		class ObjectMemory {
			DECL_OPAQUE;

			ObjectMemory(ObjectHead*);

			static ObjectMemory _init_with_root_thisptr(void*, void*);

		public:
			ObjectMemory(const ObjectMemory&);
			ObjectMemory& operator =(const ObjectMemory&);
			//std::byte& operator [](size_t);

			static ObjectMemory allocate(const MemoryLayout*, void* where, Allocator* allocator);
			template<class TRoot>
				requires std::is_polymorphic_v<TRoot>
			static inline ObjectMemory init_with_root_thisptr(const TRoot* thisptr) {
				return _init_with_root_thisptr(const_cast<TRoot*>(thisptr), dynamic_cast<void*>(const_cast<TRoot*>(thisptr)));
			}
			static ObjectMemory from_most_derived(void* mostderived);
			
			template<class TThis>
				requires std::is_polymorphic_v<TThis>
			static inline ObjectMemory from_thisptr(const TThis* thisptr) {
				return from_most_derived(dynamic_cast<void*>(const_cast<TThis*>(thisptr)));
			}
			static ObjectMemory at(void* start);
			void* fields();
			void* layout_ptr_address();
			void free();
			void* most_derived();
			void on_free(void (*cb)(void*), void* param);
			/*
			Return true if this memory block was 
			allocated externally.
			*/
			bool is_external();

			~ObjectMemory();
		};
	}
}

