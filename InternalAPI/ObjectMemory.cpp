#include "ObjectMemory.hpp"


namespace Silicon {

	namespace InternalAPI {

		struct ObjectHead {
			Allocator* allocator;
			AllocationMethod allocmethod;
			MemoryLayout* layout;
			void (*free_cb)(void*) = nullptr;
			void* param;
		};


		OPAQUE_DEF(ObjectMemory) {
			ObjectHead* head;
		};
		size_t MemoryLayout::totalsize() const {
			return sizeof(ObjectHead) + this->c_size + this->c_pad + (this->fieldcount * sizeof(void*));
		}
		void* MemoryLayout::c_most_derived(void* _head) const {
			byte* head = reinterpret_cast<byte*>(_head);
			return head + sizeof(ObjectHead);
		}
		void* MemoryLayout::c_root(void* _head) const {
			byte* head = reinterpret_cast<byte*>(_head);
			return head + sizeof(ObjectHead) + this->c_root_offset;
		}
		void* MemoryLayout::fields(void* _head) const {
			byte* head = reinterpret_cast<byte*>(_head);
			return head + sizeof(ObjectHead) + this->c_size + this->c_pad;
		}
		MemoryLayout::MemoryLayout(size_t c_size, uint16_t fieldcount, size_t c_align, size_t c_root_offset) :
			c_size(c_size), c_root_offset(c_root_offset), fieldcount(fieldcount), c_pad(c_align - (c_size % c_align))
		{}


		ObjectMemory::ObjectMemory(ObjectHead* head) {
			INIT_OPAQUE;
			this->impl->head = head;
		}
		ObjectMemory::ObjectMemory(const ObjectMemory& op) {
			this->impl->head = op.impl->head;
		}
		ObjectMemory& ObjectMemory::operator=(const ObjectMemory& op) {
			this->impl->head = op.impl->head;
			return *this;
		}
		ObjectMemory ObjectMemory::_init_with_root_thisptr(void* _root_instance, void* _mostderived) {
			byte* root_instance = reinterpret_cast<byte*>(_root_instance);
			byte* mostderived = reinterpret_cast<byte*>(_mostderived);

			auto result = from_most_derived(_mostderived);
			MemoryLayout* layout = result.impl->head->layout;
			if (layout->c_root_offset == inthandling::int_max<size_t>) {
				layout->c_root_offset = root_instance - mostderived;
			}
			return result;
		}
		ObjectMemory ObjectMemory::allocate(const MemoryLayout* layout, void* where, Allocator* allocator) {
			size_t totalsize = layout->totalsize();

			byte* result;
			AllocationMethod allocmethod;

			if (where != nullptr) {
				result = reinterpret_cast<byte*>(where);
				allocmethod = AllocationMethod::EXTERNAL;
			}
			else if (allocator != nullptr) {
				result = reinterpret_cast<byte*>(allocator->allocate(totalsize));
				allocmethod = AllocationMethod::ALLOCATOR;
			}
			else {
				result = new byte[totalsize];
				allocmethod = AllocationMethod::NEW;
			}

			if (result == nullptr) {
				throw std::bad_alloc();
			}

			ObjectHead* head = reinterpret_cast<ObjectHead*>(result);
			head->allocator = (allocmethod == AllocationMethod::ALLOCATOR ? allocator : nullptr);
			head->allocmethod = allocmethod;
			head->layout = const_cast<MemoryLayout*>(layout);

			return ObjectMemory(head);
		}
		ObjectMemory ObjectMemory::from_most_derived(void* mostderived) {
			byte* head = reinterpret_cast<byte*>(mostderived) - sizeof(ObjectHead);
			return ObjectMemory(reinterpret_cast<ObjectHead*>(head));
		}
		ObjectMemory ObjectMemory::at(void* start) {
			return ObjectMemory(reinterpret_cast<ObjectHead*>(start));
		}
		void* ObjectMemory::fields() {
			if (this->impl == nullptr) {
				return nullptr;
			}
			if (this->impl->head == nullptr) {
				return nullptr;
			}
			return this->impl->head->layout->fields(this->impl->head);
		}
		void* ObjectMemory::layout_ptr_address() {
			return &this->impl->head->layout;
		}
		void ObjectMemory::free() {
			if (this->impl == nullptr) {
				return;
			}
			if (this->impl->head == nullptr) {
				return;
			}
			if (this->impl->head->free_cb) {
				this->impl->head->free_cb(this->impl->head->param);
			}

			byte* to_del;
			switch (this->impl->head->allocmethod) {
			case AllocationMethod::EXTERNAL:
				break;

			case AllocationMethod::NEW:
				to_del = reinterpret_cast<byte*>(this->impl->head);
				delete[] to_del;
				break;

			case AllocationMethod::ALLOCATOR:
				if (this->impl->head->allocator == nullptr) {
					throw bad_allocator();
				}
				this->impl->head->allocator->free(this->impl->head);
				break;

			default:
				throw bad_allocmethod();
			}
			this->impl->head = nullptr;
		}
		void* ObjectMemory::most_derived() {
			return reinterpret_cast<byte*>(this->impl->head) + sizeof(ObjectHead);
		}
		void ObjectMemory::on_free(void (*cb) (void*), void* param) {
			if (!this->impl->head) {
				return;
			}
			this->impl->head->free_cb = cb;
			this->impl->head->param = param;
		}
		bool ObjectMemory::is_external() {
			return this->impl->head->allocmethod == AllocationMethod::EXTERNAL;
		}
		ObjectMemory::~ObjectMemory() {}
	}
}
