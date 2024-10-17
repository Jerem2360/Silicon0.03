#pragma once
#include "Forward.hpp"
#include "cstdint"
#include "CallableHelper.hpp"
#include "../macros.hpp"
#include <string>


//#define call_cpp_ctor(obj, type, ...) obj->type::type(__VA_ARGS__)


namespace Silicon {

	template<class T, class ...TArgs>
		requires std::same_as<T, Object> || std::same_as<T, Type> || std::is_base_of_v<Object, T>
	inline void call_cpp_ctor(T* obj, TArgs ...args) {
		obj->T::__call_ctor__(args...);
	}

	/*template<class T, class ...TArgs>
	using Ctor = void (T::*)(TArgs ...a);*/

	class TooManyFieldsError : public std::exception {
		using std::exception::exception;
	};
	
	namespace InternalAPI {
		struct MemoryLayout;
	}

	class SiliconException : public std::exception {
		DECL_OPAQUE;
	public:
		SiliconException(Ref<Object> exception_obj);
		SiliconException(const char* msg);
		SiliconException(const SiliconException&);
		const char* what() const override;
		Ref<Object> exc();
		~SiliconException();
	};


	class Object {
	private:

		friend class Type;
		friend class TypeDef;
		friend class CallableHelper;
		friend class BoundCallableHelper;
		friend class BoundPropertyHelper;

		template<class T, class TArgs>
		friend void call_cpp_ctor(T*, TArgs...);

		template<object_class T>
		friend class Ref;

		Type* rtti;
		uint32_t refcount;

		void incRef();
		void decRef();


		void __call_ctor__(Type* rtti);

	protected:
		Object(Type* rtti);
		void* operator new(size_t, Type* rtti);

	public:

		static Type* typeObject;

		void* operator new(size_t);
		void* operator new(size_t, void* where, InternalAPI::MemoryLayout*, Allocator* allocator);
		explicit Object();
		Object(const Object&) = delete;
		Object& operator =(const Object&) = delete;
		// static Object* create(Type* rtti, Allocator* allocator = nullptr);
		Type* getType();
		/*
		Store an object in-place at the specified location, given the
		number of bytes available.
		Returns true if the object was successfully stored.

		In-place storage is a data storage mechanism that allows class
		members to be stored in memory using raw bytes instead of being
		wrapped by an object instance. It allows saving memory space,
		but in counterpart, is only used if the available memory space
		at the location is smaller or equal to 8.
		*/
		bool inplace_store(void* where, size_t available_space);
		/*
		Load an object from the specified in-place location, given the
		number of bytes available.
		Returns nullptr if the object could not be loaded.
		*/
		static Object* inplace_load(Type* rtti, void* where, size_t available_space);

		virtual ~Object();
		void operator delete(void*);
		void operator delete(void*, void*, InternalAPI::MemoryLayout*, Allocator*);
	};

	class _TypeMethodDefHelper {
		namedict<CallableHelper>& target;
		const char* method_name;

	public:
		_TypeMethodDefHelper(namedict<CallableHelper>&, const char*);
		CallableHelper& operator =(CallableHelper&) const;
		CallableHelper& operator =(CallableHelper::functype) const;
	};


	/*
	Class representing the definition of a type's contents, 
	its bases as well as its metatype.
	Call bindCppType<T>() to make the type's instances store 
	a C++ instance of class T as their C data. This mechanism
	does not support polymorphism.
	*/
	class TypeDef {
		friend class _TypeMethodDefHelper;
		friend class Type;

		const char* name;
		std::vector<Type*> bases;
		namedict<CallableHelper> instance_methods;
		namedict<CallableHelper> class_methods;
		namedict<CallableHelper> static_methods;
		namedict<Type*> fields;
		namedict<PropertyHelper> properties;
		size_t c_size;
		size_t c_align;

		void _bindCppType(size_t, size_t);

	public:
		const _TypeMethodDefHelper new_impl;
		const _TypeMethodDefHelper free_impl;
		const _TypeMethodDefHelper init_impl;
		const _TypeMethodDefHelper call_impl;
		const _TypeMethodDefHelper del_impl;
		const _TypeMethodDefHelper member_impl;
		const _TypeMethodDefHelper subclassof_impl;
		const _TypeMethodDefHelper instanceof_impl;
		std::function<bool(void*, size_t, Object*)> inplace_write;
		std::function<Object* (void*, size_t)> inplace_read;
		// ...
		TypeDef(const char* name, std::vector<Type*> bases);

		TypeDef& operator =(const TypeDef&) = delete;
		template<class T>
			requires std::is_base_of_v<Object, T>
		inline void bindCppType() {
			this->_bindCppType(sizeof(T), alignof(T));
		}
		bool addInstanceMethod(const char* name, CallableHelper& func);
		bool addInstanceMethod(const char* name, CallableHelper::functype func);
		bool addField(const char* name, Type* type);
		PropertyHelper& addProperty(const char* name, CallableHelper getter = nullptr);

		InternalAPI::MemoryLayout _computeLayout(uint16_t);

		Type* build();

		~TypeDef();
	};

	class Type : public Object {
		friend class Object;

		const char* name;
		std::vector<Type*> bases;
		namedict<CallableHelper> instance_methods;
		namedict<CallableHelper> class_methods;
		namedict<CallableHelper> static_methods;
		namedict<Type*> field_types;
		namedict<size_t> fields;
		namedict<Object*> static_fields;
		namedict<PropertyHelper> properties;
		std::function<bool(void*, size_t, Object*)> inplace_writer;
		std::function<Object* (void*, size_t)> inplace_reader;
		InternalAPI::MemoryLayout* layout;

	public:

		static Type* typeObject;
		void* operator new(size_t, void* where, InternalAPI::MemoryLayout*, Allocator* allocator);
		void* operator new(size_t);
		void* operator new(size_t, Type* metatype);

		Type(TypeDef& definition, Type* metatype = nullptr);
		Type(const Type&) = delete;
		Type& operator =(const Type&) = delete;

		bool get_method(const char* name, OUT CallableHelper const**) const;
		bool get_method(const char* name, Ref<Object> owner, OUT BoundCallableHelper*) const;
		bool supports_inplace_storage() const;
		bool inplace_store(void* where, size_t available_space, Ref<Object> obj);
		Ref<Object> inplace_load(void* where, size_t available_space);
		bool subclass_check(Ref<Type> subclass);
		bool instance_check(Ref<Object> instance);
		std::vector<Ref<Type>> getBases();
		~Type();
		const InternalAPI::MemoryLayout* get_layout();
	};


	/*class MemoryAddressObject : public Object {
		void* value;


	public:
		static Type* typeObject;
	};*/
}

