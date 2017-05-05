#include <iostream>
#include "db_access.h"


static void prompt_pass(string& str);

db_access::db_access(const char* filename) 
  : m_config(filename)
{
    string database;
    string host;
    string user;
    string pass_type;
    string pass;
    int num_taps = 16; /* TODO: Possibly make this config-settable */

    bool success = true;

    if(!m_config.getString(database, "database")) {
        printf("db_access::db_access: Error: No key \"database\" Found!\n");
        success = false;
    }
    printf("In db constructor\n");

    if(!m_config.getString(host, "host")) {
        printf("db_access::db_access: Error: No key \"host\" Found!\n");
        success = false;
    }

    if(!m_config.getString(user, "user")) {
        printf("db_access::db_access: Error: No key \"user\" Found!\n");
        success = false;
    }

    if(!m_config.getString(pass_type, "pass_type")) {
        printf("db_access::db_access: Error: No key \"pass_type\" Found!\n");
        success = false;
    }

    if(success){
        if(pass_type == "plaintext" || pass_type == "base64") {
            if(!m_config.getString(pass, "pass")) {
                printf("db_access::db_access: Error: No key \"pass\" yet pass_type is %s!\n", 
                    pass_type.c_str());
                success = false;
            }
        }
    }

    if(success && pass_type == "base64") {
        pass = base64_decode(pass);

        /* Some base64 implementations append a newline, 
         * this will get rid of it if it does*/
        if (pass.back() == '\n')
          pass.erase(pass.size()-1);
    }

    else if(success && pass_type == "prompt")
        prompt_pass(pass);


    if (success) {
        init_mysql(host.c_str(), database.c_str(),
                   user.c_str(), pass.c_str());

        /* Initialize internal counter for flow taps */
        m_num_taps = num_taps;
        m_flow_meters = new flow_meter[num_taps];

        /* Grab update_oz to determine ticks necessary to push an update
         * If we don't find one, use default of 1.0. */
        double update_oz;
        if(!m_config.getDouble(update_oz, "update_oz"))
        {
            printf("Note: update_oz unset, using default %.2f\n", DEFAULT_UPDATE_OZ);
            update_oz = DEFAULT_UPDATE_OZ;
        }

        /* Iterate through the config file (1-indexed)
         * If the flow meter doesn't have a config option in the file
         * use the default provided 
         */
        for(int i = 1; i <= num_taps; i++) {
          double tpg;
          if(!m_config.getDouble(tpg, "flow" + 
                                      std::to_string(i) + 
                                      "_tpg")){
            printf("Note: flow%d_tpg unset, using default %.2f\n", i, DEFAULT_FLOW_TPG);
            tpg = DEFAULT_FLOW_TPG;
          }

          m_flow_meters[i-1].tpg = tpg;
          /* TODO: Make sure this is being honored correctly */
          m_flow_meters[i-1].update_ticks = tpg * update_oz / 128.0;
        }

        /* Grab times for active/archive update pushes */
        double active_rate  = DEFAULT_ACTIVE_RATE;
        double archive_rate = DEFAULT_ARCHIVE_RATE;
        if(!m_config.getDouble(active_rate, "active_rate")) {
            printf("Note: active_rate unset, using default %.2f\n", 
                DEFAULT_ACTIVE_RATE);
        }
        if(!m_config.getDouble(archive_rate, "archive_rate")) {
            printf("Note: archive_rate unset, using default %.2f\n", 
                DEFAULT_ARCHIVE_RATE);
        }
        m_active_rate.tv_sec    = static_cast<time_t> ((int)active_rate);
        m_active_rate.tv_nsec   = static_cast<long>   (fmod(active_rate, 1) * NS_TO_S);
        m_archive_rate.tv_sec   = static_cast<time_t> ((int)active_rate);
        m_archive_rate.tv_nsec  = static_cast<long>   (fmod(archive_rate, 1) * NS_TO_S);
    }
}

bool db_access::init_mysql(const char* host, const char* database,
                           const char* user, const char* pass )
{
    m_driver = get_driver_instance();
    m_conn = m_driver->connect(host, user, pass);
    m_conn->setSchema(database);

    /* TODO: Describe this. */
    m_pushactive = m_conn->prepareStatement("UPDATE active "
                                        "SET ticks=ticks+?, "
                                        "dirty=1, "
                                        "volremaining=(41+size*83)-(ticks*8.0/?)"
                                        "WHERE ontap = ?");

    /* TODO: Describe this. */
    m_pusharchive = m_conn->prepareStatement("INSERT INTO archive"
                                       " (name, ontap, size, volremaining) VALUES"
                                       " (?, ?, ?, ?)");
    
    m_stmt = m_conn->createStatement();
    return true;
}
db_access::~db_access()
{
    delete(m_conn);
    delete(m_pushactive);
    delete(m_pusharchive);
    delete(m_stmt);
    delete(m_res);
    delete(m_flow_meters);
}
bool db_access::add(unsigned int tap, int value)
{
    /* Check bounds first */
    /* Note: Using Unsigned, so only one case; no negatives */
    if(tap > m_num_taps)
        return false;

    m_flow_meters[tap].ticks += value;
    return true;
}

/*
 *
    m_pushactive = m_conn->prepareStatement("UPDATE active "
                                        "SET ticks=ticks+?, "
                                        "dirty=1, "
                                        "volremaining=(41+size*83)-(ticks*8.0/?)"
                                        "WHERE ontap = ?");
 *
 */



bool db_access::update(bool force)
{
    for(unsigned int i = 0; i < m_num_taps; i++)
    {
        /* Update flow meters if they've accrued enough ticks */
        if( (force && m_flow_meters[i].ticks > 0)
            || m_flow_meters[i].ticks > m_flow_meters[i].update_ticks)
        {
            /* TODO: Delete debug statements */
            printf("Flow meter %d at ticks %d \n", i, m_flow_meters[i].ticks);

            /* SET ticks = ticks + ? */
            /* Update ticks to add the new value accumulated */
            m_pushactive->setInt(1, m_flow_meters[i].ticks);

            /* SET volremaining=(41+size*83)-(ticks*8.0/?) */
            /* Update volremaining to be ...
             * totalvolume (41 or 124 pints) - (Pints counted from ticks) */
            m_pushactive->setInt(2, m_flow_meters[i].tpg);

            /* WHERE tap = i+1 */
            /* C-code stores these as 0-indexed, [15:0] but db has [16:1] */
            m_pushactive->setInt(3, i+1);

            /* Sets dirty bit on any values updated */
            m_pushactive->execute();

            /* Clear our accumulator */
            m_flow_meters[i].ticks = 0;
        }
    }
    return true;
}

bool db_access::clear(unsigned int tap)
{
    /* Check bounds first */
    if(tap > m_num_taps)
        return false;

    m_flow_meters[tap].ticks = 0;
    return true;
}

int db_access::access(unsigned int tap)
{
    /* Check bounds first */
    if(tap > m_num_taps)
        return 0;

    return m_flow_meters[tap].ticks;
}

void db_access::print()
{
    m_res = m_stmt->executeQuery("SELECT * FROM active");
    while(m_res->next())
    {
        std::cout << " ontap: " << m_res->getString("ontap");
        std::cout << " name: " << m_res->getString("name");
        std::cout << " ticks: " << m_res->getString("ticks");
        std::cout << " size: " << m_res->getString("size");
        std::cout << " volremaining: " << m_res->getString("volremaining");
        std::cout << " dirty: " << m_res->getString("dirty");
        std::cout << "\n";
    }
    if (m_res != NULL)
        delete m_res;
}

bool db_access::clear_db()
{
    m_res = m_stmt->executeQuery("UPDATE active SET ticks = 0");
    if (m_res != NULL)
        delete m_res;
    return true;
}

bool db_access::archive()
{
    /* Push to archive those that have been marked as dirty */
    m_res = m_stmt->executeQuery("SELECT ontap,name,ticks,size, volremaining FROM active WHERE dirty=1");
    int ontap;
    std::string name;
    int ticks;
    int size;
    double volremaining;
    while(m_res->next())
    {
        ontap = m_res->getInt("ontap");
        name = m_res->getString("name");
        ticks = m_res->getInt("ticks");
        size = m_res->getInt("size");
        volremaining = m_res->getDouble("volremaining");

        m_pusharchive->setString(1, name);
        m_pusharchive->setInt(2, ontap);
        m_pusharchive->setInt(3, size);
        m_pusharchive->setDouble(4, volremaining);

        m_pusharchive->execute();
    }
    if (m_res != NULL)
        delete m_res;


    return true;
}

static void prompt_pass(string& str) {
    char* pass = getpass("Enter Password: ");
    str = pass;
}

