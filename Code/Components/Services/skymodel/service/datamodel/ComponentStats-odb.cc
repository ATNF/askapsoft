#include <odb/pre.hxx>

#define ODB_COMMON_QUERY_COLUMNS_DEF
#include "ComponentStats-odb.h"
#undef ODB_COMMON_QUERY_COLUMNS_DEF

namespace odb
{
  // ComponentStats
  //

  const access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_common >::
  function_table_type*
  access::view_traits_impl< ::askap::cp::sms::datamodel::ComponentStats, id_common >::
  function_table[database_count];
}

#include <odb/post.hxx>
