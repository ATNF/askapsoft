#include <odb/pre.hxx>

#define ODB_COMMON_QUERY_COLUMNS_DEF
#include "ContinuumComponent-odb.h"
#undef ODB_COMMON_QUERY_COLUMNS_DEF

namespace odb
{
  // ContinuumComponent
  //

  template struct query_columns<
    ::askap::cp::sms::datamodel::Polarisation,
    id_common,
    query_columns_base< ::askap::cp::sms::datamodel::ContinuumComponent, id_common >::polarisation_alias_ >;

  template struct query_columns<
    ::askap::cp::sms::datamodel::DataSource,
    id_common,
    query_columns_base< ::askap::cp::sms::datamodel::ContinuumComponent, id_common >::data_source_alias_ >;

  template struct query_columns<
    ::askap::cp::sms::datamodel::ContinuumComponent,
    id_common,
    access::object_traits_impl< ::askap::cp::sms::datamodel::ContinuumComponent, id_common > >;

  template struct pointer_query_columns<
    ::askap::cp::sms::datamodel::ContinuumComponent,
    id_common,
    access::object_traits_impl< ::askap::cp::sms::datamodel::ContinuumComponent, id_common > >;

  const access::object_traits_impl< ::askap::cp::sms::datamodel::ContinuumComponent, id_common >::
  function_table_type*
  access::object_traits_impl< ::askap::cp::sms::datamodel::ContinuumComponent, id_common >::
  function_table[database_count];
}

#include <odb/post.hxx>
