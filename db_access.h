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
#include <math.h>
#include <time.h>

#include "config_parser.h"
#include "base64.h"

static const double DEFAULT_FLOW_TPG      = 10313.0;
static const double DEFAULT_UPDATE_OZ     = 1.0;
static const double DEFAULT_ACTIVE_RATE   = 1.0;
static const double DEFAULT_ARCHIVE_RATE  = 30.0;
static const double NS_TO_S  = 1000000000.0;

class db_access{
    public:
        struct flow_meter
        {
            int ticks=0;
            double tpg=DEFAULT_FLOW_TPG;
            int update_ticks=0;
        };

        db_access(const char* filename);
        ~db_access();

        /* TODO: Error Handling based on MySQL conn status, return bool maybe? */
        /* Func: add
         * Adds value to ticks accrued at index tap */
        bool add(unsigned int tap, int value);

        /* Func: update
         * Updates DB with all accrue'd flow ticks 
         * Then zero's out local flow ticks array 
         * Unless force is set the ticks must be
         * above a threshold to update */
        bool update(bool force);

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

        /* TODO: Write these */
        timespec get_archive_rate() { return m_archive_rate; }
        timespec get_active_rate()  { return m_active_rate;  }

    private:
        bool init_mysql(const char* host, const char* database,
                           const char* user, const char* pass );
        int                 UPDATE_THRESHOLD_TICKS;
        unsigned int        m_num_taps;
        struct timespec     m_archive_rate;
        struct timespec     m_active_rate;
        flow_meter*         m_flow_meters;
        config_parser       m_config;
        sql::Driver*        m_driver;
        sql::Connection*    m_conn;
        sql::Statement*     m_stmt;
        sql::ResultSet*     m_res;
        sql::PreparedStatement* m_pushactive;
        sql::PreparedStatement* m_pusharchive;
};

#endif
