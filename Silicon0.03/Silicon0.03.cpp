#include <iostream>
#include <cstdlib>
#include "Ref.hpp"


struct S {
	size_t field1;
	size_t field2;
};

struct T {
	int field1;
	int field2;
};

int main()
{
	using namespace Silicon;

	Ref<Type> object_type = Object::typeObject;

	std::cout << "instance check" << Object::typeObject->instance_check(object_type) << '\n';

	std::cout << "end\n";
}
