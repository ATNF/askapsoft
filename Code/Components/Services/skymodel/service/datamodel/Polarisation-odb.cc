#include <odb/pre.hxx>

#define ODB_COMMON_QUERY_COLUMNS_DEF
#include "Polarisation-odb.h"
#undef ODB_COMMON_QUERY_COLUMNS_DEF

namespace odb
{
  // Polarisation
  //

  template struct query_columns<
    ::askap::cp::sms::datamodel::Polarisation,
    id_common,
    access::object_traits_impl< ::askap::cp::sms::datamodel::Polarisation, id_common > >;

  const access::object_traits_impl< ::askap::cp::sms::datamodel::Polarisation, id_common >::
  function_table_type*
  access::object_traits_impl< ::askap::cp::sms::datamodel::Polarisation, id_common >::
  function_table[database_count];
}

#include <odb/post.hxx>
