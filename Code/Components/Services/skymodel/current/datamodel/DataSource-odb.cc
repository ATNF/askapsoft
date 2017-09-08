#include <odb/pre.hxx>

#define ODB_COMMON_QUERY_COLUMNS_DEF
#include "DataSource-odb.h"
#undef ODB_COMMON_QUERY_COLUMNS_DEF

namespace odb
{
  // DataSource
  //

  template struct query_columns<
    ::askap::cp::sms::datamodel::DataSource,
    id_common,
    access::object_traits_impl< ::askap::cp::sms::datamodel::DataSource, id_common > >;

  const access::object_traits_impl< ::askap::cp::sms::datamodel::DataSource, id_common >::
  function_table_type*
  access::object_traits_impl< ::askap::cp::sms::datamodel::DataSource, id_common >::
  function_table[database_count];
}

#include <odb/post.hxx>
