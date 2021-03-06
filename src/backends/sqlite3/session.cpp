//
// Copyright (C) 2004-2006 Maciej Sobczak, Stephen Hutton, David Courtney
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//


#include "soci-sqlite3.h"

#include <sstream>
#include <string>

#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

using namespace soci;
using namespace soci::details;
using namespace sqlite_api;

namespace // anonymous
{

// helper function for hardcoded queries
void execude_hardcoded(sqlite_api::sqlite3* conn, char const* const query, char const* const errMsg)
{
    char *zErrMsg = 0;
    int const res = sqlite3_exec(conn, query, 0, 0, &zErrMsg);
    if (res != SQLITE_OK)
    {
        std::ostringstream ss;
        ss << errMsg << " " << zErrMsg;
        sqlite3_free(zErrMsg);
        throw soci_error(ss.str());
    }
}

} // namespace anonymous


sqlite3_session_backend::sqlite3_session_backend(
    std::string const & connectString)
{
    int timeout = 0;
    std::string synchronous;
    std::string dbname(connectString);
    std::stringstream ssconn(connectString);
    while (!ssconn.eof() && ssconn.str().find('=') >= 0)
    {
        std::string key, val;
        std::getline(ssconn, key, '=');
        std::getline(ssconn, val, ' ');
        if ("dbname" == key || "db" == key)
        {
            dbname = val;
        }
        else if ("timeout" == key)
        {
            std::istringstream converter(val);
            converter >> timeout;
        }
        else if ("synchronous" == key)
        {
            synchronous = val;
        }
    }

    int res = sqlite3_open(dbname.c_str(), &conn_);
    if (SQLITE_OK != res)
    {
        const char *zErrMsg = sqlite3_errmsg(conn_);
        std::ostringstream ss;
        ss << "Cannot establish connection to the database. " << zErrMsg;
        throw soci_error(ss.str());
    }

    if (!synchronous.empty())
    {
        std::string const query("pragma synchronous=" + synchronous);
        std::string const errMsg("Query failed: " + query);
        execude_hardcoded(conn_, query.c_str(), errMsg.c_str());
    }

    res = sqlite3_busy_timeout(conn_, timeout * 1000);
    if (SQLITE_OK != res)
    {
        const char *zErrMsg = sqlite3_errmsg(conn_);
        std::ostringstream ss;
        ss << "Failed to set busy timeout for connection. " << zErrMsg;
        throw soci_error(ss.str());
    }
}

sqlite3_session_backend::~sqlite3_session_backend()
{
    clean_up();
}

void sqlite3_session_backend::begin()
{
    execude_hardcoded(conn_, "BEGIN", "Cannot begin transaction.");
}

void sqlite3_session_backend::commit()
{
    execude_hardcoded(conn_, "COMMIT", "Cannot commit transaction.");
}

void sqlite3_session_backend::rollback()
{
    execude_hardcoded(conn_, "ROLLBACK", "Cannot rollback transaction.");
}

void sqlite3_session_backend::clean_up()
{
    sqlite3_close(conn_);
}

sqlite3_statement_backend * sqlite3_session_backend::make_statement_backend()
{
    return new sqlite3_statement_backend(*this);
}

sqlite3_rowid_backend * sqlite3_session_backend::make_rowid_backend()
{
    return new sqlite3_rowid_backend(*this);
}

sqlite3_blob_backend * sqlite3_session_backend::make_blob_backend()
{
    return new sqlite3_blob_backend(*this);
}
