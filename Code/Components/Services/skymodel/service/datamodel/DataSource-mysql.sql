/* This file was generated by ODB, object-relational mapping (ORM)
 * compiler for C++.
 */

DROP TABLE IF EXISTS `DataSource`;

CREATE TABLE IF NOT EXISTS `schema_version` (
  `name` VARCHAR(255) NOT NULL PRIMARY KEY,
  `version` BIGINT UNSIGNED NOT NULL,
  `migration` TINYINT(1) NOT NULL)
 ENGINE=InnoDB;

DELETE FROM `schema_version`
  WHERE `name` = '';

CREATE TABLE `DataSource` (
  `version` BIGINT NOT NULL,
  `data_source_id` BIGINT NOT NULL PRIMARY KEY AUTO_INCREMENT,
  `name` TEXT NOT NULL,
  `catalogue_id` TEXT NULL)
 ENGINE=InnoDB;

CREATE INDEX `data_source_id_i`
  ON `DataSource` (`data_source_id`);

INSERT IGNORE INTO `schema_version` (
  `name`, `version`, `migration`)
  VALUES ('', 2, 0);

