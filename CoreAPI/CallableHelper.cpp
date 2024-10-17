#include "Ref.hpp"
#include "CallableHelper.hpp"

namespace Silicon {
	CallableHelper::CallableHelper() : 
		CallableHelper(nullptr) 
	{}
	CallableHelper::CallableHelper(functype cfunc) :
		ftype(true)
	{
		this->impl.cfunc = cfunc;
	}
	CallableHelper::CallableHelper(Object* sfunc) :
		ftype(false)
	{
		this->impl.sfunc = sfunc;
		this->impl.sfunc->incRef();
	}
	CallableHelper::CallableHelper(std::nullptr_t) :
		ftype(true)
	{}
	CallableHelper::CallableHelper(const CallableHelper& other) :
		ftype(other.ftype)
	{
		if (this->ftype) {
			this->impl.cfunc = other.impl.cfunc;
		}
		else {
			this->impl.sfunc = other.impl.sfunc;
			if (this->impl.sfunc)
				this->impl.sfunc->incRef();
		}
	}
	CallableHelper::CallableHelper(CallableHelper&& other) noexcept :
		ftype(other.ftype) 
	{
		if (this->ftype) {
			this->impl.cfunc = other.impl.cfunc;
			other.impl.cfunc = nullptr;
		}
		else {
			this->impl.sfunc = other.impl.sfunc;
			other.impl.sfunc = nullptr;
		}
	}
	CallableHelper& CallableHelper::operator=(const CallableHelper& other) {
		this->ftype = other.ftype;
		if (this->ftype) {
			this->impl.cfunc = other.impl.cfunc;
		}
		else {
			auto tmp = this->impl.sfunc;
			this->impl.sfunc = other.impl.sfunc;
			if (this->impl.sfunc)
				this->impl.sfunc->incRef();
			if (tmp)
				tmp->decRef();
		}
		return *this;
	}
	CallableHelper& CallableHelper::operator=(CallableHelper&& other) noexcept {
		Object* tmp = nullptr;

		if (!this->ftype) {
			tmp = this->impl.sfunc;
		}
		this->ftype = other.ftype;
		if (this->ftype) {
			this->impl.cfunc = other.impl.cfunc;
		}
		else {
			this->impl.sfunc = other.impl.sfunc;
			this->impl.sfunc->incRef();
		}
		if (tmp)
			tmp->decRef();
		return *this;
	}
	Ref<Object> CallableHelper::operator()(args_t args, kwds_t kwds) const {
		if (this->ftype) {
			try {
				return this->impl.cfunc(args, kwds);
			} catch (SiliconException& exc) {
				exc;
				throw;
			}
			catch (std::exception& exc) {
				exc;
				// throw SiliconException(new CppException(exc.what());
				throw;
			}
			catch (...) {
				// throw SiliconException(new ExternalException());
				throw;
			}
		}
		BoundCallableHelper call_impl;
		if (!this->impl.sfunc->getType()->get_method("operator ()", this->impl.sfunc, &call_impl)) {
			return nullptr;
		}
		return call_impl(args, kwds);
	}
	bool CallableHelper::operator==(std::nullptr_t) const {
		if (this->ftype) {
			return this->impl.cfunc == nullptr;
		}
		return this->impl.sfunc == nullptr;
	}
	CallableHelper::operator bool() const {
		return *this != nullptr;
	}
	BoundCallableHelper CallableHelper::bind(Object* self) const {
		return BoundCallableHelper(const_cast<CallableHelper&>(*this), self);
	}
	bool CallableHelper::typeCheckArgs(args_t args, argtypes_t types, Ref<Object>* exception) {
		*exception = nullptr;
		if (types.size() != args.size()) {
			return false;
		}
		for (int i = 0; i < args.size(); i++) {
			if (types[i] == nullptr) {
				continue;
			}
			if (args[i] == nullptr) {
				return false;
			}
			if (!typeof<Type>->instance_check(types[i])) {
				throw SiliconException(nullptr);
			}
			Ref<Type> tp = types[i].DownCast<Type>();
			if (!tp->instance_check(args[i])) {
				return false;
			}
		}
		return true;
	}
	bool CallableHelper::typeCheckKwds(kwds_t kwds, kwdtypes_t types, Ref<Object>* exception) {
		*exception = nullptr;
		if (kwds.size() != types.size()) {
			return false;
		}
		for (auto& [k, v] : kwds) {
			if (!types.contains(k)) {
				return false;
			}
			if (types[k] == nullptr) {
				continue;
			}
			if (v == nullptr) {
				return false;
			}
			if (!typeof<Type>->instance_check(types[k])) {
				throw SiliconException(nullptr);
			}
			auto tp = types[k].DownCast<Type>();
			if (!tp->instance_check(v)) {
				return false;
			}
		}
		return true;
	}

	CallableHelper::~CallableHelper() {
		if (!this->ftype && this->impl.sfunc) {
			this->impl.sfunc->decRef();
			this->impl.sfunc = nullptr;
		}
	}

	BoundCallableHelper::BoundCallableHelper() :
		func(nullptr), self(nullptr)
	{}
	
	BoundCallableHelper::BoundCallableHelper(CallableHelper& func, Object* self) :
		func(&func), self(self)
	{
		if (this->self) {
			this->self->incRef();
		}
	}
	BoundCallableHelper::BoundCallableHelper(const BoundCallableHelper& other) :
		func(other.func), self(other.self)
	{
		if (this->self) {
			this->self->incRef();
		}
	}
	BoundCallableHelper& BoundCallableHelper::operator=(const BoundCallableHelper& other) {
		Object* old = this->self;
		this->func = other.func;
		this->self = other.self;
		if (this->self) {
			this->self->incRef();
		}
		if (old) {
			old->decRef();
		}
		return *this;
	}
	Ref<Object> BoundCallableHelper::operator()(args_t args, kwds_t kwds) const {
		args.insert(args.begin(), this->self);
		return this->func->operator()(args, kwds);
	}
	BoundCallableHelper::~BoundCallableHelper() {
		if (this->self) {
			this->self->decRef();
			this->self = nullptr;
		}
	}

	_PropertyValueAssigner::_PropertyValueAssigner(CallableHelper* target) :
		target(target)
	{}
	CallableHelper& _PropertyValueAssigner::operator=(const CallableHelper& op) const {
		return (*this->target = op);
	}
	CallableHelper& _PropertyValueAssigner::operator=(const CallableHelper::functype op) const {
		return (*this->target = CallableHelper(op));
	}
	PropertyHelper::PropertyHelper(CallableHelper getter) :
		_getter(getter), _setter(nullptr), getter(&_getter), setter(&_setter)
	{}
	PropertyHelper::PropertyHelper(const PropertyHelper& other) :
		_getter(other._getter), _setter(other._setter), getter(&_getter), setter(&_setter)
	{}
	PropertyHelper& PropertyHelper::operator=(const PropertyHelper& other) {
		this->_getter = other._getter;
		this->_setter = other._setter;
		return *this;
	}
	BoundPropertyHelper PropertyHelper::bind(Object* instance) {
		return { this->_getter, this->_setter, instance };
	}
	BoundPropertyHelper::BoundPropertyHelper(CallableHelper& getter, CallableHelper& setter, Object* owner) :
		getter(getter), setter(setter), self(owner)
	{
		if (this->self) {
			this->self->incRef();
		}
	}
	BoundPropertyHelper::BoundPropertyHelper(const BoundPropertyHelper& other) :
		getter(other.getter), setter(other.setter), self(other.self)
	{
		if (this->self) {
			this->self->incRef();
		}
	}
	BoundPropertyHelper& BoundPropertyHelper::operator =(const BoundPropertyHelper& other) {
		this->getter = other.getter;
		this->setter = other.setter;
		auto tmp = this->self;
		this->self = other.self;
		if (this->self) {
			this->self->incRef();
		}
		if (tmp) {
			tmp->decRef();
		}
		return *this;
	}
	Object* BoundPropertyHelper::get() {
		if (!this->getter) {
			return nullptr;
		}
		return this->getter({ this->self }).operator->();
	}
	bool BoundPropertyHelper::set(Object* value) {
		if (!this->setter) {
			return false;
		}
		return this->setter({ this->self, value }) != nullptr;
	}
	BoundPropertyHelper::~BoundPropertyHelper() {
		if (this->self) {
			this->self->decRef();
		}
	}
}

