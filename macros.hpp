#pragma once
#include <memory>

// Declare a class as having opaque fields. Opaque fields are stored in an extra pointer member using the PImpl technique.
#define DECL_OPAQUE struct _Impl; std::unique_ptr<_Impl> impl
// Declare and define the opaque fields and methods of a class. Should be used in source files.
#define OPAQUE_DEF(owner) struct owner::_Impl
// Initialize the pointer member for a class's opaque fields. Used in constructors.
#define INIT_OPAQUE this->impl = std::unique_ptr<_Impl>(new _Impl())

#define TYPEOBJ(cls) ::Silicon::Type* cls::typeObject = ::Silicon::_Helpers::_TypeInitializer() + []() -> ::Silicon::Type*

#define TYPEOF(cls) cls::typeObject

