/*
File that contains most forward declarations, typedefs, concepts and helper types of
the API.
*/
#pragma once
#include <concepts>
#include <map>
#include <functional>
#include <string>
#include "../macros.hpp"


namespace Silicon {
	class Object;
	class Type;
	class TypeDef;
	class BoundCallableHelper;
	class Allocator;


	namespace _Helpers {
		template<class T>
		concept _object_subclass = std::same_as<Object, T> || std::same_as<Type, T> || std::is_base_of_v<Object, T>;

		template<class T>
		concept _has_typeobj = requires () {
			T::typeObject;
		};

		class _TypeInitializer {

		public:
			inline _TypeInitializer() {}
			inline Type* operator +(std::function<Type* ()> func) {
				return func();
			}
		};
		// comparison primitive to be passed to map object when the type of the key is const char*
		class _strcomparer : public std::function<bool(const char*, const char*)> {
		public:
			inline bool operator ()(const char* lhs, const char* rhs) const {
				return std::strcmp(lhs, rhs) < 0;
			}
		};
		// hashing primitive to be passed to a hashmap or hashset when the type of data in const char*
		class _strhash : public std::function<size_t(const char*)> {
		public:
			inline size_t operator()(const char* str) const {
				return std::hash<std::string>()(str);
			}
		}; 
		// comparison primitive to be passed to map objects when the type of the key is Ref<Object>
		template<class T>
			requires std::is_base_of_v<Object, T>
		class _refcomparer : public std::function<bool(Ref<T>&, Ref<T>&)> {
		public:
			inline bool operator ()(const Ref<T>& lhs, const Ref<T>& rhs) const {
				return lhs.is(rhs);
			}
		};
		// hashing primitive to be passed to map objects when the type of the key is Ref<Object>
		template<class T>
			requires std::is_base_of_v<Object, T>
		class _refhash : public std::function<size_t(Ref<T>)> {
			inline size_t operator ()(Ref<T> op) const {
				return static_cast<size_t>(op.operator->());  // shoul
			}
		};
	}

	template<class T>
	concept object_class = _Helpers::_object_subclass<T>;

	template<class T>
	concept complete_obj_class = _Helpers::_object_subclass<T> && _Helpers::_has_typeobj<T>;

	template<object_class T>
	class Ref;


	template<class T>
	using namedict = std::unordered_map<std::string, T>;
	template<class TRef, class T>
	using refhashdict = std::unordered_map<Ref<TRef>, T, _Helpers::_refhash<TRef>, _Helpers::_refcomparer<TRef>>;
	template<class TRef, class T>
	using refdict = std::map<Ref<TRef>, T, _Helpers::_refcomparer<TRef>>;

	typedef std::vector<Ref<Object>> args_t;
	typedef refdict<Object, Ref<Object>> kwds_t;
	typedef std::vector<Ref<Type>> argtypes_t;
	typedef refdict<Object, Ref<Type>> kwdtypes_t;

	/*
	The 'Type' object associated with class T.
	*/
	template<object_class T>
	
	Type* typeof = T::typeObject;

	/*template<class TSrc, class TDst>
	TDst& bit_cast(TSrc& src) {
		return *reinterpret_cast<TDst*>(&src);
	}*/
}

