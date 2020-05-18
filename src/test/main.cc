#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include <ftw.h>
#include <fty_common_db_dbpath.h>
#include <mariadb/mysql.h>
#include <tntdb.h>
#include <unistd.h>
#include "src/asset/asset.h"

class TestDB
{
public:
    TestDB(const std::string& name)
        : m_name(name)
    {
        std::string dbpath = "/tmp/" + name + "_db";

        // clang-format off
        auto serverOptions = strlist({
            name,
            "--datadir=" + dbpath,
            "--collation-server=utf8_bin",
            "--character-set-server=utf8",
            "--local-infile=0"
        });

        auto serverGroups = strlist({
            name,
            "embedded",
            "server",
        });
        // clang-format on

        removeDir(dbpath);
        mkdir(dbpath.c_str(), 0777);

        mysql_library_init(serverOptions.size()-1, serverOptions.data(), serverGroups.data());
        m_con = mysql_init(nullptr);

        mysql_options(m_con, MYSQL_READ_DEFAULT_GROUP, "client");
        mysql_options(m_con, MARIADB_OPT_UNIXSOCKET, socket().c_str());
        mysql_options(m_con, MYSQL_OPT_USE_EMBEDDED_CONNECTION, NULL);

        clearStrList(serverOptions);
        clearStrList(serverGroups);
        m_oldUrl = DBConn::url;
        m_rcon = mysql_real_connect(m_con, nullptr, nullptr, nullptr, nullptr, 0, socket().c_str(), 0);
        mysql_query(m_rcon, "create database test;");
        DBConn::url = "mysql:unix_socket=" + socket() + ";db=test";
    }

    ~TestDB()
    {
        mysql_close(m_con);
        DBConn::url = m_oldUrl;
    }

    void removeDir(const std::string& path)
    {
        nftw(path.c_str(), &TestDB::unlink, 64, FTW_DEPTH | FTW_PHYS);
    }

    std::string socket() const
    {
        return "/tmp/" + m_name + ".sock";
    }

    MYSQL* rcon()
    {
        return m_rcon;
    }

private:
    static int unlink(const char* fpath, const struct stat* /*sb*/, int /*typeflag*/, struct FTW* /*ftwbuf*/)
    {
        return remove(fpath);
    }

    std::vector<char*> strlist(const std::vector<std::string>& input)
    {
        std::vector<char*> result;

        result.reserve(input.size() + 1);
        for(const std::string& it: input) {
            result.push_back(strdup(it.data()));
        }
        result.push_back(nullptr);

        return result;
    }

    void clearStrList(const std::vector<char*>& input)
    {
        for(char* it: input) {
            free(it);
        }
    }

private:
    MYSQL*      m_con;
    std::string m_name;
    std::string m_oldUrl;
    MYSQL*      m_rcon;
};

TEST_CASE("Create db")
{
    TestDB db("asset");
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

    SECTION("Create asset")
    {
        fty::AssetImpl asset;
        asset.setInternalName("router");
        asset.setPriority(1);
        asset.setAssetType("device");
        asset.setAssetSubtype("router");
        asset.setAssetStatus(fty::AssetStatus::Active);
        asset.setExtEntry("name", "Router 1");

        CHECK_NOTHROW(asset.save());

        fty::AssetImpl asset2;
        asset2.setInternalName("router");
        asset2.setPriority(1);
        asset2.setAssetType("device");
        asset2.setAssetSubtype("router");
        asset2.setAssetStatus(fty::AssetStatus::Active);
        asset2.setExtEntry("name", "Router 2");

        CHECK_NOTHROW(asset2.save());

        asset.setPriority(2);
        asset.save();

        std::string sql = R"(UPDATE             t_bios_asset_element         SET             id_type = (SELECT id_asset_element_type FROM t_bios_asset_element_type WHERE name = "device"),             id_subtype = (SELECT id_asset_device_type FROM t_bios_asset_device_type WHERE name = "router"),             id_parent = (SELECT id_asset_element from (SELECT * FROM t_bios_asset_element) AS e where e.name = "datacenter-6"),             status = "active",             priority = 2,             asset_tag = ""         WHERE             id_asset_element = 1;)";
        mysql_autocommit(db.rcon(), 0);
        mysql_query(db.rcon(), sql.c_str());
        mysql_commit(db.rcon());

        //conn.beginTransaction();
        tntdb::Transaction trans(conn);
        conn.execute(sql);
        trans.commit();
        //conn.commitTransaction();
    }
}
