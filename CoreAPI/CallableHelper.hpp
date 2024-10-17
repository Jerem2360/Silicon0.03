#pragma once
#include "Forward.hpp"
#include <functional>
#include <map>

#define OUT


namespace Silicon {


	class CallableHelper {
	public:
		using functype = std::function<Ref<Object> (args_t, kwds_t)>;
	private:

		union _Impl {
			functype cfunc;
			Object* sfunc;

			inline _Impl() : cfunc(nullptr) {}
			inline ~_Impl() {}
		} impl;
		bool ftype;

	public:
		CallableHelper();
		CallableHelper(functype cfunc);
		CallableHelper(Object* sfunc);
		CallableHelper(std::nullptr_t);
		CallableHelper(const CallableHelper&);
		CallableHelper(CallableHelper&&) noexcept;
		CallableHelper& operator =(const CallableHelper&);
		CallableHelper& operator =(CallableHelper&&) noexcept;
		Ref<Object> operator ()(args_t args, kwds_t kwds = {}) const;
		bool operator ==(std::nullptr_t) const;
		explicit operator bool() const;
		BoundCallableHelper bind(Object*) const;
		
		static bool typeCheckArgs(args_t to_check, argtypes_t types, OUT Ref<Object>* exception);
		static bool typeCheckKwds(kwds_t to_check, kwdtypes_t types, OUT Ref<Object>* exception);

		~CallableHelper();
	};
	
	class BoundCallableHelper {
		CallableHelper* func;
		Object* self;

	public:
		BoundCallableHelper();
		BoundCallableHelper(CallableHelper&, Object*);
		BoundCallableHelper(const BoundCallableHelper&);
		BoundCallableHelper& operator =(const BoundCallableHelper&);
		Ref<Object> operator()(args_t args, kwds_t kwds = {}) const;
		~BoundCallableHelper();
	};

	class BoundPropertyHelper;

	class _PropertyValueAssigner {
		CallableHelper* target;

	public:
		_PropertyValueAssigner(CallableHelper*);
		CallableHelper& operator =(const CallableHelper&) const;
		CallableHelper& operator =(const CallableHelper::functype) const;
	};
	class PropertyHelper {
		CallableHelper _getter;
		CallableHelper _setter;

	public:
		const _PropertyValueAssigner getter;
		const _PropertyValueAssigner setter;

		PropertyHelper(CallableHelper getter = nullptr);
		PropertyHelper(const PropertyHelper&);
		PropertyHelper& operator =(const PropertyHelper&);
		BoundPropertyHelper bind(Object* instance);
	};

	class BoundPropertyHelper {
		CallableHelper& getter;
		CallableHelper& setter;
		Object* self;
		
	public:
		BoundPropertyHelper(CallableHelper& getter, CallableHelper& setter, Object* instance);
		BoundPropertyHelper(const BoundPropertyHelper&);
		BoundPropertyHelper& operator =(const BoundPropertyHelper&);

		Object* get();
		bool set(Object* value);

		~BoundPropertyHelper();
	};
	
	/*
	Implements the '|' operator to merge two vectors of same type.
	*/
	template<class T>
	inline std::vector<T> operator |(const std::vector<T>& lhs, const std::vector<T>& rhs) {
		std::vector<T> result = lhs;
		for (auto& t : rhs) {
			result.push_back(t);
		}
		return result;
	}
	template<class T>
	inline std::vector<T>& operator |=(std::vector<T>& lhs, const std::vector<T>& rhs) {
		for (auto& elem : rhs) {
			lhs.push_back(elem);
		}
		return lhs;
	}

	/*
	Implements the '|' operator to merge two hashmaps of the same key and value types.
	std::unordered_map uses bucketing to deal with conflicts.
	*/
	template<class TKey, class TValue, class Hash = std::hash<TKey>, class Equals = std::equal_to<TKey>>
	inline std::unordered_map<TKey, TValue, Hash, Equals> operator |(const std::unordered_map<TKey, TValue, Hash, Equals>& lhs, const std::unordered_map<TKey, TValue, Hash, Equals>& rhs) {
		std::unordered_map<TKey, TValue, Hash> result = {};

		for (auto&[k, v] : lhs) {
			result[k] = v;
		}
		for (auto&[k, v] : rhs) {
			result[k] = v;
		}
		return result;
	}
	template<class TKey, class TValue, class Hash = std::hash<TKey>, class Equals = std::equal_to<TKey>>
	inline std::unordered_map<TKey, TValue, Hash, Equals>& operator |=(std::unordered_map<TKey, TValue, Hash, Equals>& lhs, const std::unordered_map<TKey, TValue, Hash, Equals>& rhs) {
		for (auto& [k, v] : rhs) {
			lhs[k] = v;
		}
		return lhs;
	}

	/*
	Implements the '|' operator to merge two maps of the same key and value types.
	*/
	template<class TKey, class TValue, class Comparer = std::less<TKey>>
	inline std::map<TKey, TValue, Comparer> operator |(const std::map<TKey, TValue, Comparer>& lhs, const std::map<TKey, TValue, Comparer>& rhs) {
		std::map<TKey, TValue, Comparer> result = {};

		for (auto& [k, v] : lhs) {
			result[k] = v;
		}
		for (auto& [k, v] : rhs) {
			result[k] = v;
		}
		return result;
	}
	template<class TKey, class TValue, class Comparer = std::less<TKey>>
	inline std::map<TKey, TValue, Comparer>& operator |=(std::map<TKey, TValue, Comparer>& lhs, const std::map<TKey, TValue, Comparer>& rhs) {
		for (auto& [k, v] : rhs) {
			lhs[k] = v;
		}
		return lhs;
	}
 }

