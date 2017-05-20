#ifndef DB_ACCESS_H
#define DB_ACCESS_H

#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include <mutex>
#include <string>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "config_parser.h"
#include "base64.h"

static const double DEFAULT_FLOW_TPG      = 10313.0;
static const double DEFAULT_UPDATE_OZ     = 1.0;
static const double DEFAULT_ACTIVE_RATE   = 1.0;
static const double DEFAULT_ARCHIVE_RATE  = 30.0;
static const double S_TO_MS               = 1000.0;

class db_access{
    public:
        struct flow_meter {
            int ticks=0;                /* Ticks as measured by flow meter sensor */
            double tpg=DEFAULT_FLOW_TPG;/* Ticks Per Gallon, configurable */
            int update_ticks=0;         /* Number of ticks required to make a 
                                         * line considered 'dirty' */
        };

        /* Func: constructor
         * Instantiates the config parser class m_config
         * through load_config with the provided file name.
         * If still in a safe state, then runs the 
         * init_mysql function to connect to the database*/
        db_access(const char* filename);
        
        /* Func: deconstructor
         * Deletes all dynamically allocated variables */
        ~db_access();

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
         * This will zero out all ticks on the 
         * active DATABASE This does not clear 
         * any local accrue'd ticks */
        bool clear_db(); 

        /* Func: archive: 
         * Copies off all rows from active that have their
         * dirty bit set to archive. Returns TRUE if there
         * were no exceptions. Returns FALSE if any 
         * exceptions caught. */
        bool archive();

        /* Func: access
         * Returns the current accrued ticks at index tap
         * Note this is just the value since last update() */
        int access(unsigned int tap);

        /* Func: Print
         * Prints to stdout the current format of the 
         * active table. Also works as a simple 
         * example for fetching values */
        void print();

        /* Func: get_archive_rates
         * Returns the rate, in ms, that the 
         * archiving function should run at. */
        int get_archive_rate() { return m_archive_rate; }

        /* Func: get_archive_rates
         * Returns the rate, in ms, that the active updating
         * function should run at. */
        int get_active_rate()  { return m_active_rate;  }

        /* Func: is_safe()
         * Returns true if the database was successfully 
         * connected to and the class is in a safe state
         * to begin processing */
        bool is_safe()         { return m_safe; }

    private:
        /* Func: load_config
         * Once config_parser has been created, this
         * function will be able to pull all desired
         * configuration items and store them where
         * appropriate */
        bool load_config( string& host, string& database,
                            string& user, string& pass );

        /* Func: init_mysql
         * Initializes the connection to the desired
         * host and database. Initializes all common
         * SQL Types for the class as well */
        bool init_mysql(const char* host, const char* database,
                           const char* user, const char* pass );

        bool                m_safe;
        unsigned int        m_num_taps;
        std::mutex          m_lock;
        int                 m_archive_rate;
        int                 m_active_rate;
        config_parser       m_config;
        flow_meter*         m_flow_meters;
        sql::Driver*        m_driver;
        sql::Connection*    m_conn;
        sql::Statement*     m_stmt;
        sql::ResultSet*     m_res;
        sql::PreparedStatement* m_pushactive;
        sql::PreparedStatement* m_pusharchive;
};

#endif
