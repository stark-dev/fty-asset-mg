#pragma once
#include "testdb.h"

void createDb()
{
    auto   conn = tntdb::connectCached(DBConn::url);
    //conn.execute("create database assets_test;");
    //conn.execute("use assets_test;");

    conn.execute(R"(
        CREATE TABLE t_bios_asset_element_type (
            id_asset_element_type TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,
            name                  VARCHAR(50)      NOT NULL,

            PRIMARY KEY (id_asset_element_type),
            UNIQUE INDEX `UI_t_bios_asset_element_type` (`name` ASC)
        ) AUTO_INCREMENT = 1;
    )");

    conn.execute(R"(
        INSERT INTO
            t_bios_asset_element_type (name)
        VALUES
            ("group"),
            ("datacenter"),
            ("room"),
            ("row"),
            ("rack"),
            ("device");
    )");

    conn.execute(R"(
        CREATE TABLE t_bios_asset_device_type(
            id_asset_device_type TINYINT UNSIGNED   NOT NULL AUTO_INCREMENT,
            name                 VARCHAR(50)        NOT NULL,

            PRIMARY KEY (id_asset_device_type),
            UNIQUE INDEX `UI_t_bios_asset_device_type` (`name` ASC)
        );
     )");

    conn.execute(R"(
        INSERT INTO
            t_bios_asset_device_type (name)
        VALUES
            ("ups"),
            ("genset"),
            ("epdu"),
            ("pdu"),
            ("server"),
            ("feed"),
            ("sts"),
            ("switch"),
            ("storage"),
            ("router");
     )");

    conn.execute(R"(
        CREATE TABLE t_bios_asset_element (
          id_asset_element  INT UNSIGNED        NOT NULL AUTO_INCREMENT,
          name              VARCHAR(50)         NOT NULL,
          id_type           TINYINT UNSIGNED    NOT NULL,
          id_subtype        TINYINT UNSIGNED    NOT NULL DEFAULT 11,
          id_parent         INT UNSIGNED,
          status            VARCHAR(9)          NOT NULL DEFAULT 'nonactive',
          priority          TINYINT             NOT NULL DEFAULT 5,
          asset_tag         VARCHAR(50),

          PRIMARY KEY (id_asset_element),

          INDEX FK_ASSETELEMENT_ELEMENTTYPE_idx (id_type   ASC),
          INDEX FK_ASSETELEMENT_ELEMENTSUBTYPE_idx (id_subtype   ASC),
          INDEX FK_ASSETELEMENT_PARENTID_idx    (id_parent ASC),
          UNIQUE INDEX `UI_t_bios_asset_element_NAME` (`name` ASC),
          INDEX `UI_t_bios_asset_element_ASSET_TAG` (`asset_tag`  ASC),

          CONSTRAINT FK_ASSETELEMENT_ELEMENTTYPE
            FOREIGN KEY (id_type)
            REFERENCES t_bios_asset_element_type (id_asset_element_type)
            ON DELETE RESTRICT,

          CONSTRAINT FK_ASSETELEMENT_ELEMENTSUBTYPE
            FOREIGN KEY (id_subtype)
            REFERENCES t_bios_asset_device_type (id_asset_device_type)
            ON DELETE RESTRICT,

          CONSTRAINT FK_ASSETELEMENT_PARENTID
            FOREIGN KEY (id_parent)
            REFERENCES t_bios_asset_element (id_asset_element)
            ON DELETE RESTRICT
        );
    )");

    conn.execute(R"(
        CREATE TABLE t_bios_asset_ext_attributes(
          id_asset_ext_attribute    INT UNSIGNED NOT NULL AUTO_INCREMENT,
          keytag                    VARCHAR(40)  NOT NULL,
          value                     VARCHAR(255) NOT NULL,
          id_asset_element          INT UNSIGNED NOT NULL,
          read_only                 TINYINT      NOT NULL DEFAULT 0,

          PRIMARY KEY (id_asset_ext_attribute),

          INDEX FK_ASSETEXTATTR_ELEMENT_idx (id_asset_element ASC),
          UNIQUE INDEX `UI_t_bios_asset_ext_attributes` (`keytag`, `id_asset_element` ASC),

          CONSTRAINT FK_ASSETEXTATTR_ELEMENT
            FOREIGN KEY (id_asset_element)
            REFERENCES t_bios_asset_element (id_asset_element)
            ON DELETE RESTRICT
        );
    )");

    conn.execute(R"(
        CREATE TABLE t_bios_asset_link_type(
          id_asset_link_type   TINYINT UNSIGNED   NOT NULL AUTO_INCREMENT,
          name                 VARCHAR(50)        NOT NULL,

          PRIMARY KEY (id_asset_link_type),
          UNIQUE INDEX `UI_t_bios_asset_link_type_name` (`name` ASC)
        );
    )");

    conn.execute(R"(
        CREATE TABLE t_bios_asset_link (
          id_link               INT UNSIGNED        NOT NULL AUTO_INCREMENT,
          id_asset_device_src   INT UNSIGNED        NOT NULL,
          src_out               CHAR(4),
          id_asset_device_dest  INT UNSIGNED        NOT NULL,
          dest_in               CHAR(4),
          id_asset_link_type    TINYINT UNSIGNED    NOT NULL,

          PRIMARY KEY (id_link),

          INDEX FK_ASSETLINK_SRC_idx  (id_asset_device_src    ASC),
          INDEX FK_ASSETLINK_DEST_idx (id_asset_device_dest   ASC),
          INDEX FK_ASSETLINK_TYPE_idx (id_asset_link_type     ASC),

          CONSTRAINT FK_ASSETLINK_SRC
            FOREIGN KEY (id_asset_device_src)
            REFERENCES t_bios_asset_element(id_asset_element)
            ON DELETE RESTRICT,

          CONSTRAINT FK_ASSETLINK_DEST
            FOREIGN KEY (id_asset_device_dest)
            REFERENCES t_bios_asset_element(id_asset_element)
            ON DELETE RESTRICT,

          CONSTRAINT FK_ASSETLINK_TYPE
            FOREIGN KEY (id_asset_link_type)
            REFERENCES t_bios_asset_link_type(id_asset_link_type)
            ON DELETE RESTRICT
        );
    )");

    conn.execute(R"(
        CREATE VIEW v_bios_asset_link AS
            SELECT  v1.id_link,
                    v1.src_out,
                    v1.dest_in,
                    v1.id_asset_link_type,
                    v1.id_asset_device_src AS id_asset_element_src,
                    v1.id_asset_device_dest AS id_asset_element_dest
            FROM t_bios_asset_link v1;
    )");

    conn.execute(R"(
        INSERT INTO t_bios_asset_link_type (name) VALUES ('power chain');
    )");

    conn.execute(R"(
        CREATE TABLE t_bios_asset_group_relation (
          id_asset_group_relation INT UNSIGNED NOT NULL AUTO_INCREMENT,
          id_asset_group          INT UNSIGNED NOT NULL,
          id_asset_element        INT UNSIGNED NOT NULL,

          PRIMARY KEY (id_asset_group_relation),

          INDEX FK_ASSETGROUPRELATION_ELEMENT_idx (id_asset_element ASC),
          INDEX FK_ASSETGROUPRELATION_GROUP_idx   (id_asset_group   ASC),

          UNIQUE INDEX `UI_t_bios_asset_group_relation` (`id_asset_group`, `id_asset_element` ASC),

          CONSTRAINT FK_ASSETGROUPRELATION_ELEMENT
            FOREIGN KEY (id_asset_element)
            REFERENCES t_bios_asset_element (id_asset_element)
            ON DELETE RESTRICT,

          CONSTRAINT FK_ASSETGROUPRELATION_GROUP
            FOREIGN KEY (id_asset_group)
            REFERENCES t_bios_asset_element (id_asset_element)
            ON DELETE RESTRICT
        );
    )");

    conn.execute(R"(
        CREATE TABLE t_bios_monitor_asset_relation(
            id_ma_relation        INT UNSIGNED      NOT NULL AUTO_INCREMENT,
            id_discovered_device  SMALLINT UNSIGNED NOT NULL,
            id_asset_element      INT UNSIGNED      NOT NULL,

            PRIMARY KEY(id_ma_relation),

            INDEX(id_discovered_device),
            INDEX(id_asset_element),

            FOREIGN KEY (id_asset_element)
                REFERENCEs t_bios_asset_element(id_asset_element)
                ON DELETE RESTRICT
        );
    )");
}
