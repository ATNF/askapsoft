// This file was generated by ODB, object-relational mapping (ORM)
// compiler for C++.
//

#include <odb/pre.hxx>

#define ODB_MYSQL_QUERY_COLUMNS_DEF
#include "Common-odb-mysql.h"
#undef ODB_MYSQL_QUERY_COLUMNS_DEF

#include <cassert>
#include <cstring>  // std::memcpy

#include <odb/schema-catalog-impl.hxx>
#include <odb/function-table.hxx>

#include <odb/mysql/traits.hxx>
#include <odb/mysql/database.hxx>
#include <odb/mysql/transaction.hxx>
#include <odb/mysql/connection.hxx>
#include <odb/mysql/statement.hxx>
#include <odb/mysql/statement-cache.hxx>
#include <odb/mysql/container-statements.hxx>
#include <odb/mysql/exceptions.hxx>
#include <odb/mysql/enum.hxx>

namespace odb
{
}

namespace odb
{
  static bool
  create_schema (database& db, unsigned short pass, bool drop)
  {
    ODB_POTENTIALLY_UNUSED (db);
    ODB_POTENTIALLY_UNUSED (pass);
    ODB_POTENTIALLY_UNUSED (drop);

    if (drop)
    {
      switch (pass)
      {
        case 1:
        {
          return true;
        }
        case 2:
        {
          db.execute ("CREATE TABLE IF NOT EXISTS `schema_version` (\n"
                      "  `name` VARCHAR(255) NOT NULL PRIMARY KEY,\n"
                      "  `version` BIGINT UNSIGNED NOT NULL,\n"
                      "  `migration` TINYINT(1) NOT NULL)\n"
                      " ENGINE=InnoDB");
          db.execute ("DELETE FROM `schema_version`\n"
                      "  WHERE `name` = ''");
          return false;
        }
      }
    }
    else
    {
      switch (pass)
      {
        case 1:
        {
          return true;
        }
        case 2:
        {
          db.execute ("CREATE TABLE IF NOT EXISTS `schema_version` (\n"
                      "  `name` VARCHAR(255) NOT NULL PRIMARY KEY,\n"
                      "  `version` BIGINT UNSIGNED NOT NULL,\n"
                      "  `migration` TINYINT(1) NOT NULL)\n"
                      " ENGINE=InnoDB");
          db.execute ("INSERT IGNORE INTO `schema_version` (\n"
                      "  `name`, `version`, `migration`)\n"
                      "  VALUES ('', 2, 0)");
          return false;
        }
      }
    }

    return false;
  }

  static const schema_catalog_create_entry
  create_schema_entry_ (
    id_mysql,
    "",
    &create_schema);

  static const schema_catalog_migrate_entry
  migrate_schema_entry_1_ (
    id_mysql,
    "",
    1ULL,
    0);

  static bool
  migrate_schema_2 (database& db, unsigned short pass, bool pre)
  {
    ODB_POTENTIALLY_UNUSED (db);
    ODB_POTENTIALLY_UNUSED (pass);
    ODB_POTENTIALLY_UNUSED (pre);

    if (pre)
    {
      switch (pass)
      {
        case 1:
        {
          return true;
        }
        case 2:
        {
          db.execute ("UPDATE `schema_version`\n"
                      "  SET `version` = 2, `migration` = 1\n"
                      "  WHERE `name` = ''");
          return false;
        }
      }
    }
    else
    {
      switch (pass)
      {
        case 1:
        {
          return true;
        }
        case 2:
        {
          db.execute ("UPDATE `schema_version`\n"
                      "  SET `migration` = 0\n"
                      "  WHERE `name` = ''");
          return false;
        }
      }
    }

    return false;
  }

  static const schema_catalog_migrate_entry
  migrate_schema_entry_2_ (
    id_mysql,
    "",
    2ULL,
    &migrate_schema_2);
}

#include <odb/post.hxx>