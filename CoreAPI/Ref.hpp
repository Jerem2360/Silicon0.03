#pragma once
#include "Object.hpp"


namespace Silicon {

	template<object_class T>
	class Ref {

		template<object_class T>
		friend class Ref;
		
		Object* target;

	public:
		inline Ref() :
			target(nullptr)
		{}
		inline Ref(T* src) :
			target(src)
		{
			if (this->target) {
				this->target->incRef();
			}
		}
		inline Ref(const Ref<T>& other) :
			target(other.target)
		{
			if (this->target) {
				this->target->incRef();
			}
		}
		inline Ref(Ref<T>&& other) noexcept :
			target(other.target)
		{
			other.target = nullptr;
		}
		inline Ref<T>& operator =(const Ref<T>& other) {
			Object* old = this->target;
			this->target = other.target;
			if (this->target) {
				this->target->incRef();
			}
			if (old) {
				old->decRef();
			}
			return *this;
		}
		inline Ref<T>& operator =(Ref<T>&& other) noexcept {
			if (this->target) {
				this->target->decRef();
			}
			this->target = other.target;
			other.target = nullptr;
			return *this;
		}
		// temporary
		inline bool operator ==(std::nullptr_t) const {
			return this->target == nullptr;
		}
		inline T* operator ->() {
			return static_cast<T*>(this->target);
		}
		inline const T* operator->() const {
			return static_cast<T*>(this->target);
		}
		inline explicit operator bool() const {
			return this->target != nullptr;
		}
		template<class TBase>
			requires std::is_base_of_v<TBase, T>
		inline operator Ref<TBase>() {
			return Ref<TBase>(this->target);
		}
		template<class TChild>
			requires std::is_base_of_v<T, TChild>
		inline Ref<TChild> DownCast() {
			return Ref<TChild>(dynamic_cast<TChild*>(this->target));
		}
		template<class TChild>
			requires std::is_base_of_v<T, TChild>
		inline const Ref<TChild> DownCast() const {
			return Ref<TChild>(dynamic_cast<TChild*>(this->target));
		}
		bool is(Ref<Object> other) const {
			return this->target == other.target;
		}
		inline ~Ref() {
			if (this->target) {
				this->target->decRef();
				this->target = nullptr;
			}
		}
	};
}

