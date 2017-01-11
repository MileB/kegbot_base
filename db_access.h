#ifndef DB_ACCESS_H
#define DB_ACCESS_H

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include <string.h>
#include <unistd.h>

class db_access{
    public:
        /* TODO: Take pass in as a hash somehow if possible so that we aren't storing cleartext passes */
        db_access(const char* db, const char* user, const char* pass, unsigned int num_taps);
        ~db_access();

        /* TODO: Error Handling based on MySQL conn status, return bool maybe? */
        /* Func: add
         * Adds value to ticks accrued at index tap */
        bool add(unsigned int tap, int value);

        /* Func: update
         * Updates DB with all accrue'd flow ticks 
         * Then zero's out local flow ticks array */
        bool update();

        /* Func: clear
         * Sets accrued flow ticks of one local index
         * back to zero. Use case yet unknown, but may
         * be needed */
        bool clear(unsigned int tap);

        /* Func: clear_db
         * !DANGEROUS!
         * This will zero out all ticks on the active DATABASE
         * This does not clear any local accrue'd ticks */
        bool clear_db();

        /* TODO
         * Document this. Probably. */
        bool archive();

        /* Func: access
         * Returns the current accrued ticks at index tap
         * Note this is just the value since last update() */
        /* TODO: I'd like to overload this to be []'s */
        int access(unsigned int tap);

        /* Func: Print
         * Prints to stdout the current format of the active table
         * Also works as a simple example for fetching values */
        void print();

    private:
        int* m_flow_ticks;
        unsigned int m_num_taps;
        sql::Driver*        m_driver;
        sql::Connection*    m_conn;
        sql::Statement*     m_stmt;
        sql::ResultSet*     m_res;
        sql::PreparedStatement* m_pushactive;
        sql::PreparedStatement* m_pusharchive;
        sql::PreparedStatement* m_getname;
};

#endif
