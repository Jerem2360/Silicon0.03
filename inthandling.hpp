#include <concepts>
// return the number of bits that int_type is made of as a constant expression.
#define bitsof(int_type) _bitsof<int_type>()


namespace inthandling {
	// constrains a class template parameter to be a complete type.
	template<class T>
	concept is_complete_type = requires () {
		sizeof(T);
	};

	/// <summary>
	/// Return the number of bits that its type parameter is made of.
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <returns></returns>
	template<class T>
	inline consteval int __bitsof() {
		return sizeof(T) * 8;
	}

	template<class T>
		requires is_complete_type<T>
	inline constexpr int bitsof = __bitsof<T>();

	template<class T>
		requires std::signed_integral<T>
	inline constexpr T __int_min() {
		return (INT8_MIN * (__bitsof<T>() == 8)) +
			(INT16_MIN * (__bitsof<T>() == 16)) +
			(INT32_MIN * (__bitsof<T>() == 32)) +
			(INT64_MIN * (__bitsof<T>() == 64));
	}

	template<class T>
		requires std::unsigned_integral<T>
	inline constexpr T __int_min() {
		return 0;
	}

	template<class T>
		requires std::signed_integral<T>
	inline constexpr T __int_max() {
		return (INT8_MAX * (__bitsof<T>() == 8)) +
			(INT16_MAX * (__bitsof<T>() == 16)) +
			(INT32_MAX * (__bitsof<T>() == 32)) +
			(INT64_MAX * (__bitsof<T>() == 64));
	}

	template<class T>
		requires std::unsigned_integral<T>
	inline constexpr T __int_max() {
		return (UINT8_MAX * (__bitsof<T>() == 8)) +
			(UINT16_MAX * (__bitsof<T>() == 16)) +
			(UINT32_MAX * (__bitsof<T>() == 32)) +
			(UINT64_MAX * (__bitsof<T>() == 64));
	}

	/// <summary>
	/// Contains the smallest integer that its type parameter can represent.
	/// The type parameter must be an integral type.
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template<class T>
		requires std::integral<T>
	inline const T int_min = __int_min<T>();

	/// <summary>
	/// Contains the largest integer that its type parameter can represent.
	/// The type parameter must be an integral type.
	/// </summary>
	/// <typeparam name="T"></typeparam>
	template<class T>
		requires std::integral<T>
	inline const T int_max = __int_max<T>();


	template<class T>
	requires std::integral<T>
	inline constexpr T min(T i1, T i2) {
		return i1 < i2 ? i1 : i2;
	}
	template<class T>
	requires std::integral<T>
	inline constexpr T max(T i1, T i2) {
		return i1 > i2 ? i1 : i2;
	}
			
}

