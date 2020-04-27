#pragma once
#include <mariadb/mysql.h>
#include <tntdb.h>
#include <unistd.h>
#include <fty_common_db_dbpath.h>
#include <ftw.h>

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
        mysql_real_connect(m_con, nullptr, nullptr, nullptr, nullptr, 0, socket().c_str(), 0);
        mysql_query(m_con, "create database test;");
        DBConn::url = "mysql:unix_socket=" + socket() + ";db=test";
    }

    ~TestDB()
    {
        mysql_shutdown(m_con, SHUTDOWN_DEFAULT);
        mysql_close(m_con);
        DBConn::url = m_oldUrl;
        //mysql_library_end();
    }

    void removeDir(const std::string& path)
    {
        nftw(path.c_str(), &TestDB::unlink, 64, FTW_DEPTH | FTW_PHYS);
    }

    std::string socket() const
    {
        return "/tmp/" + m_name + ".sock";
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
};

