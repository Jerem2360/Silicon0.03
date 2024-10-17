#include "Ref.hpp"
#include "../InternalAPI/ObjectMemory.hpp"
#include "Object.hpp"
#include <iostream>


namespace Silicon {

	typedef byte _dummy;

	class fatal_error : public std::exception {
		using std::exception::exception;
	};
	void throw_fatal_error(const char* msg) {
		std::cerr << msg << "\n";
		throw fatal_error(msg);
	}

	constexpr size_t totalsizeof_type = InternalAPI::MemoryLayout::totalsizeof<Type>(0);

	byte object_type_memory[totalsizeof_type];
	byte type_type_memory[totalsizeof_type];
	void* first_allocs_dst;

	struct RootTypes {
		Type* object_type;
		Type* type_type;

		inline static bool initialized = false;
	};

	struct TypeSystemRoot {
		static_assert(std::is_base_of_v<Object, Type>, "Object should be a subclass of type.");
		void* _object_type_address = &(object_type_memory[0]);
		void* _type_type_address = &(type_type_memory[0]);
		Type* _object_type = nullptr;
		Type* _type_type = nullptr;

		inline static TypeSystemRoot* _current = nullptr;
		inline static Type* object_type = nullptr;
		inline static Type* type_type = nullptr;


		inline static bool initialized() {
			return object_type && type_type;
		}

	private:
		static _dummy init();
		static _dummy dummy;
	};

	OPAQUE_DEF(SiliconException) {
		Ref<Object> exc;
		const char* msg = nullptr;
	};
	SiliconException::SiliconException(Ref<Object> exc) :
		SiliconException("A silicon exception was thrown, but handling these is not supported yet.")
	{
		this->impl->exc = exc;
	}
	SiliconException::SiliconException(const char* msg) :
		exception(msg)
	{
		INIT_OPAQUE;
		this->impl->exc = nullptr;
		this->impl->msg = msg;
	}

	SiliconException::SiliconException(const SiliconException& src) :
		exception(src.impl->msg) 
	{
		INIT_OPAQUE;
		this->impl->exc = src.impl->exc;
		this->impl->msg = src.impl->msg;
	}
	Ref<Object> SiliconException::exc() {
		return this->impl->exc;
	}
	const char* SiliconException::what() const {
		return this->impl->msg;
	}
	SiliconException::~SiliconException() {}




	_TypeMethodDefHelper::_TypeMethodDefHelper(namedict<CallableHelper>& target, const char* method_name) :
		target(target), method_name(method_name)
	{}
	CallableHelper& _TypeMethodDefHelper::operator=(CallableHelper& method) const {
		auto [where, success] = this->target.insert_or_assign(this->method_name, method);
		if (!success)
			throw_fatal_error("failed to insert a method into a type def.");
		return where->second;
	}
	CallableHelper& _TypeMethodDefHelper::operator=(CallableHelper::functype method) const {
		auto [where, success] = this->target.insert_or_assign(this->method_name, CallableHelper(method));
		if (!success)
			throw_fatal_error("failed to insert a method into a type def.");
		return where->second;
	}
	void TypeDef::_bindCppType(size_t c_size, size_t c_align) {
		this->c_size = c_size;
		this->c_align = c_align;
	}
	TypeDef::TypeDef(const char* name, std::vector<Type*> bases) :
		name(name), bases(bases), instance_methods(), class_methods(), static_methods(), fields(), c_size(0), c_align(0),
		new_impl(this->class_methods, "operator new"),
		free_impl(this->class_methods, "operator free"),
		init_impl(this->instance_methods, name),
		call_impl(this->instance_methods, "operator ()"),
		del_impl(this->instance_methods, "operator del"),
		member_impl(this->instance_methods, "operator ."),
		subclassof_impl(this->class_methods, "operator subclassof"),
		instanceof_impl(this->class_methods, "operator instanceof"),
		inplace_write(nullptr),
		inplace_read(nullptr)
	{
		for (Type* tp : bases) {
			if (tp != nullptr) {
				tp->incRef();
			}
		}
	}
	bool TypeDef::addInstanceMethod(const char* name, CallableHelper& func) {
		if (this->instance_methods.contains(name)) {
			return false;
		}
		this->instance_methods[name] = func;
		return true;
	}
	bool TypeDef::addInstanceMethod(const char* name, CallableHelper::functype func) {
		if (this->instance_methods.contains(name)) {
			return false;
		}
		this->instance_methods[name] = CallableHelper(func);
		return true;
	}
	bool TypeDef::addField(const char* name, Type* type) {
		if (this->fields.contains(name)) {
			return false;
		}
		if (type != nullptr) {
			type->incRef();
		}
		this->fields[name] = type;
		return true;
	}
	PropertyHelper& TypeDef::addProperty(const char* name, CallableHelper getter) {
		this->properties[name] = PropertyHelper(getter);
		return this->properties[name];
	}
	InternalAPI::MemoryLayout TypeDef::_computeLayout(uint16_t field_count) {
		return { this->c_size, field_count, this->c_align };
	}
	TypeDef::~TypeDef() {
		for (Type* tp : this->bases) {
			if (tp != nullptr) {
				tp->decRef();
			}
		}
		for (auto& pair : this->fields) {
			if (pair.second != nullptr) {
				pair.second->decRef();
			}
		}
	}
	void* Object::operator new(size_t sz) {
		return operator new(sz, typeObject);
	}
	void* Object::operator new(size_t sz, Type* rtti) {
		BoundCallableHelper new_helper;
		if (!rtti->get_method("operator new", rtti, &new_helper)) {
			return nullptr;  // error, but should never happen.
		}
		Ref<Object> obj = new_helper({}, {});
		return nullptr; // missing a type to encapsulate such return values
	}
	void* Object::operator new(size_t sz, void* where, InternalAPI::MemoryLayout* layout, Allocator* allocator) {
		if (!TypeSystemRoot::initialized()) {
			auto layout = new InternalAPI::MemoryLayout(sizeof(Type), 0, alignof(Type), sizeof(Type) - sizeof(Object));
			return InternalAPI::ObjectMemory::allocate(layout, where, nullptr).most_derived();
		}
		return InternalAPI::ObjectMemory::allocate(layout, where, allocator).most_derived();
	}
	Object::Object() : Object(typeObject)
	{}

	Object::Object(Type* rtti) :
		rtti(rtti)
	{
		auto mem = InternalAPI::ObjectMemory::init_with_root_thisptr(this);
		if (TypeSystemRoot::initialized()) {
			this->refcount = 0;
		}
		if (this->rtti) {
			this->rtti->incRef();
		}
	}
	/*Object::Object(const Object& other) :
		rtti(other.rtti), refcount(0)
	{
		if (this->rtti) {
			this->rtti->incRef();
		}
	}*/
	/*Object& Object::operator=(const Object& other) {
		Type* old = this->rtti;
		this->rtti = other.rtti;
		if (this->rtti) {
			this->rtti->incRef();
		}
		if (old) {
			old->decRef();
		}
		return *this;
	}*/
	void Object::__call_ctor__(Type* rtti) {
		this->Object::Object(rtti);
	}
	void Object::incRef() {
		this->refcount++;
	}
	void Object::decRef() {
		this->refcount--;
		if (this->refcount <= 0) {
			delete this;
		}
	}
	Type* Object::getType() {
		return this->rtti;
	}
	bool Object::inplace_store(void* where, size_t available_space) {
		if (!this->rtti->supports_inplace_storage()) {
			return false;
		}
		return this->rtti->inplace_writer(where, available_space, this);
	}
	Object* Object::inplace_load(Type* rtti, void* where, size_t available_space) {
		if (!rtti->supports_inplace_storage()) {
			return nullptr;
		}
		return rtti->inplace_reader(where, available_space);
	}
	Object::~Object() {
		if (this->rtti) {
			this->rtti->decRef();
			this->rtti = nullptr;
		}
	}
	void Object::operator delete(void* ptr) {
		InternalAPI::ObjectMemory::from_most_derived(ptr).free();
	}
	void Object::operator delete(void* ptr, void* where, InternalAPI::MemoryLayout* layout, Allocator* allocator) {
		Object::operator delete(ptr);
	}



	void* Type::operator new(size_t sz) {
		return Object::operator new(sz, typeObject);
	}
	void* Type::operator new(size_t sz, void* where, InternalAPI::MemoryLayout* layout, Allocator* allocator) {
		return Object::operator new(sz, where, layout, allocator);
	}
	void* Type::operator new(size_t sz, Type* metatype) {
		return Object::operator new(sz, metatype);
	}
	Type::Type(TypeDef& definition, Type* metatype) : Object(metatype)
	{
		this->bases = definition.bases;
		this->name = definition.name;

		this->instance_methods = {};
		this->class_methods = {};
		this->static_methods = {};
		this->static_fields = {};
		this->properties = {};
		bool bases_support_inplace_storage = true;
		this->inplace_writer = nullptr;
		this->inplace_reader = nullptr;

		uint16_t fieldcount = 0;
		for (Type* base : this->bases) {

			// update the layout counter
			fieldcount += base->layout->fieldcount;

			// add reference to that base
			base->incRef();

			// inherit from that base's methods
			this->instance_methods |= base->instance_methods;
			this->class_methods |= base->class_methods;
			this->static_methods |= base->static_methods;

			// same for static fields and properties
			this->static_fields |= base->static_fields;
			this->properties |= base->properties;

			// a class can only support inplace storage if all of its bases do so as well
			bases_support_inplace_storage &= base->supports_inplace_storage();
		}
		if (bases_support_inplace_storage) {
			this->inplace_writer = definition.inplace_write;
			this->inplace_reader = definition.inplace_read;
		}
		else {
			this->inplace_writer = nullptr;
			this->inplace_reader = nullptr;
		}

		// now add methods defined by the user, so they can override that from base classes.
		this->instance_methods |= definition.instance_methods;
		this->class_methods |= definition.class_methods;
		this->static_methods |= definition.static_methods;
		this->properties |= definition.properties;

		// define the field layout for instances
		this->fields = {};
		this->field_types = {};
		for (auto& [name, type] : definition.fields) {
			this->fields[name] = fieldcount++;
			this->field_types[name] = type;
			type->incRef();
		}

		this->layout = new InternalAPI::MemoryLayout(definition._computeLayout(fieldcount));
	}

	bool Type::get_method(const char* name, CallableHelper const** out) const {
		if (this->instance_methods.contains(name)) {
			*out =  &this->instance_methods.at(name);
			return true;
		}
		if (this->class_methods.contains(name)) {
			*out = &this->class_methods.at(name);
			return true;
		}
		if (this->static_methods.contains(name)) {
			*out = &this->static_methods.at(name);
			return true;
		}
		return false;
	}
	bool Type::get_method(const char* name, Ref<Object> instance, BoundCallableHelper* out) const {
		const CallableHelper* meth;
		if (!this->get_method(name, &meth)) {
			return false;
		}
		*out = meth->bind(instance.operator->());
		return true;
	}

	const InternalAPI::MemoryLayout* Type::get_layout() {
		return this->layout;
	}
	bool Type::supports_inplace_storage() const {
		return this->inplace_writer && this->inplace_reader;
	}
	bool Type::inplace_store(void* where, size_t available_space, Ref<Object> obj) {
		return this->inplace_writer(where, available_space, obj.operator->());
	}
	Ref<Object> Type::inplace_load(void* where, size_t available_space) {
		return this->inplace_reader(where, available_space);
	}
	bool Type::subclass_check(Ref<Type> subclass) {
		BoundCallableHelper impl;
		if (!this->get_method("operator subclassof", this, &impl)) {
			throw SiliconException(nullptr);
		}
		return (bool)impl({ subclass }, {});
	}
	bool Type::instance_check(Ref<Object> instance) {
		BoundCallableHelper impl;
		if (!this->get_method("operator instanceof", this,&impl)) {
			throw SiliconException(nullptr);
		}
		return (bool)impl({ instance }, {});
	}
	std::vector<Ref<Type>> Type::getBases() {
		std::vector<Ref<Type>> result{};
		for (auto base : this->bases) {
			result.push_back(base);
		}
		return result;
	}

	Type::~Type() {
		for (Type*& base : this->bases) {
			if (base) {
				base->decRef();
				base = nullptr;
			}
		}
		for (auto& [name, type] : this->field_types) {
			if (type) {
				type->decRef();
				type = nullptr;
			}
		}
		if (this->layout) {
			delete this->layout;
			this->layout = nullptr;
		}
	}

	bool _basic_subclasscheck(Ref<Type> cls, Ref<Type> subclass) {
		if (cls.is(subclass)) {
			return true;
		}
		for (auto base : subclass->getBases()) {
			if (_basic_subclasscheck(base, subclass)) {
				return true;
			}
		}
		return false;
	}

	bool _basic_instancecheck(Ref<Type> cls, Ref<Object> instance) {
		return _basic_subclasscheck(cls, instance->getType());
	}


	_dummy TypeSystemRoot::init() {

		std::cout << std::hash<std::string>()("operator instanceof") << '\n';
		std::cout << std::hash<std::string>()("operator free") << '\n';

		_current = new TypeSystemRoot();

		for (size_t i = 0; i < totalsizeof_type; i++) {
			object_type_memory[i] = 0_b;
		}
		for (size_t i = 0; i < totalsizeof_type; i++) {
			
		}

		_current->_object_type = reinterpret_cast<Type*>(InternalAPI::ObjectMemory::at(_current->_object_type_address).most_derived());
		_current->_type_type = reinterpret_cast<Type*>(InternalAPI::ObjectMemory::at(_current->_type_type_address).most_derived());


		auto object_typedef = TypeDef("Object", {});
		object_typedef.bindCppType<Object>();

		object_typedef.new_impl = [](args_t args, kwds_t kwds) -> Ref<Object> {
			Ref<Object> exc;
			if (!CallableHelper::typeCheckArgs(args, { typeof<Type> }, &exc)) {
				throw SiliconException(exc);
			}
			if (!CallableHelper::typeCheckKwds(kwds, {}, &exc)) {
				throw SiliconException(exc);
			}
			Ref<Type> cls = args[0].DownCast<Type>();

			auto mem = InternalAPI::ObjectMemory::allocate(cls->get_layout(), nullptr, nullptr);

			return nullptr;
		};
		object_typedef.init_impl = [](args_t args, kwds_t kwds) -> Ref<Object> {
			Ref<Object> exc;
			if (!CallableHelper::typeCheckArgs(args, { typeof<Object>, typeof<Type> }, &exc)) {
				throw SiliconException(exc);
			}
			if (!CallableHelper::typeCheckKwds(kwds, {}, &exc)) {
				throw SiliconException(exc);
			}
			// call_cpp_ctor(args[0], Object, args[1].DownCast<Type>().operator->());
			//call_cpp_ctor(args[0].operator->(), (Type*)nullptr);
			return nullptr;
		};
		object_typedef.subclassof_impl = [](args_t args, kwds_t kwds) -> Ref<Object> {
			if (args.size() != 2) {
				return nullptr;  // should throw SiliconException in the future.
			}
			if (kwds.size()) {
				return nullptr;  // same as above
			}
			if (!_basic_instancecheck(typeof<Type>, args[0])) {
				return nullptr;  // same
			}
			if (!_basic_instancecheck(typeof<Type>, args[1])) {
				return nullptr;  // same
			}

			auto cls = args[0].DownCast<Type>();
			auto other = args[1].DownCast<Type>();

			if (cls.is(other)) {
				return other;  // synonym of true for now, until BoolObject is implemented
			}
			
			for (Ref<Type> tp : other->getBases()) {
				if (cls->subclass_check(tp)) {
					return other;  // same as above.
				}
			}
			return nullptr;  // needs to be replaced with BoolObject later
		};
		object_typedef.instanceof_impl = [](args_t args, kwds_t kwds) -> Ref<Object> {
			if (args.size() != 2) {
				return nullptr;  // should throw SiliconException in the future.
			}
			if (kwds.size()) {
				return nullptr;  // same as above
			}
			if (!_basic_instancecheck(typeof<Type>, args[0])) {
				return nullptr; // same
			}

			auto cls = args[0].DownCast<Type>();
			auto instance = args[1];

			return cls->subclass_check(instance->getType()) ? instance : nullptr;  // needs to be replaced with BoolObject later
		};
		object_typedef.free_impl = [](args_t args, kwds_t kwds) -> Ref<Object> {
			Ref<Object> exc;
			if (!CallableHelper::typeCheckArgs(args, { typeof<Type>,  typeof<Object> }, &exc)) {
				throw SiliconException(exc);
			}
			if (!CallableHelper::typeCheckKwds(kwds, {}, &exc)) {
				throw SiliconException(exc);
			}
			auto mem = InternalAPI::ObjectMemory::from_thisptr(args[1].operator->());
			mem.free();
			return nullptr;
		};

		/*
		Default behaviour of inplace storage. This function is only used upon calls
		to an object's base class. Otherwise, inplace storage is simply not used to 
		reduce the number of checks.
		*/
		object_typedef.inplace_write = [](void* where, size_t sz, Object* to_write) -> bool {
			if (sz < sizeof(Object*)) {
				return false;
			}
			Object** addr = reinterpret_cast<Object**>(where);
			*addr = to_write;
			return true;
		};
		object_typedef.inplace_read = [](void* where, size_t sz) -> Object* {
			if (sz < sizeof(Object*)) {
				return nullptr;
			}
			Object** addr = reinterpret_cast<Object**>(where);
			return *addr;
		};

		auto type_typedef = TypeDef("Type", { _current->_object_type });
		type_typedef.bindCppType<Type>();

		type_typedef.init_impl = [](args_t args, kwds_t kwds) -> Ref<Object> {
			Ref<Object> exc;
			if (!CallableHelper::typeCheckArgs(args, { typeof<Object>, typeof<Object>, typeof<Type> }, &exc)) {
				throw SiliconException(exc);
			}
			if (!CallableHelper::typeCheckKwds(kwds, {}, &exc)) {
				throw SiliconException(exc);
			}
			// to do: call args[0]->Type::Type(args[1], args[2])
			return nullptr;
		};

		_current->_object_type = new(_current->_object_type_address, nullptr, nullptr) Type(object_typedef, _current->_type_type);
		_current->_type_type = new(_current->_type_type_address, nullptr, nullptr) Type(type_typedef, _current->_type_type);


		object_type = _current->_object_type;
		type_type = _current->_type_type;

		delete _current;
		_current = nullptr;
		return (_dummy)0;
	}
	_dummy TypeSystemRoot::dummy = init();

	Type* Object::typeObject = TypeSystemRoot::object_type;
	Type* Type::typeObject = TypeSystemRoot::type_type;


	/*TYPEOBJ(MemoryAddressObject) {
		TypeDef definition("MemoryAddress", { typeof<Object> });
		definition.bindCppType<MemoryAddressObject>();
		return new Type(definition);
	};*/
}

