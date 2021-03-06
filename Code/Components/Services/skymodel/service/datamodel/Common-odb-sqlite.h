// This file was generated by ODB, object-relational mapping (ORM)
// compiler for C++.
//

#ifndef COMMON_ODB_SQLITE_H
#define COMMON_ODB_SQLITE_H

// Begin prologue.
//
#include <odb/boost/version.hxx>
#if ODB_BOOST_VERSION != 2040000 // 2.4.0
#  error ODB and C++ compilers see different libodb-boost interface versions
#endif
#include <boost/shared_ptr.hpp>
#include <odb/boost/smart-ptr/pointer-traits.hxx>
#include <odb/boost/smart-ptr/wrapper-traits.hxx>
#include <odb/boost/date-time/sqlite/posix-time-traits.hxx>
//
// End prologue.

#include <odb/version.hxx>

#if (ODB_VERSION != 20400UL)
#error ODB runtime version mismatch
#endif

#include <odb/pre.hxx>

#include "Common.h"

#include "Common-odb.h"

#include <odb/details/buffer.hxx>

#include <odb/sqlite/version.hxx>
#include <odb/sqlite/forward.hxx>
#include <odb/sqlite/binding.hxx>
#include <odb/sqlite/sqlite-types.hxx>
#include <odb/sqlite/query.hxx>
#include <odb/sqlite/query-dynamic.hxx>

namespace odb
{
}

#include "Common-odb-sqlite.i"

#include <odb/post.hxx>

#endif // COMMON_ODB_SQLITE_H
