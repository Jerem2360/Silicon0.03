#pragma once

enum class byte : unsigned char {};


template<class RT, class AT>
RT& bit_cast(AT& op) {
	return *(reinterpret_cast<RT*>(&op));
}

consteval inline byte operator ""_b(size_t src) {
	return static_cast<byte>(src);
}
