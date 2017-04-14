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
         * If we don't find one, use default of 1.0.
         * TODO: Make this set as a static const in the class for pretty */
        double update_oz;
        if(!m_config.getDouble(update_oz, "update_oz"))
        {
            printf("Note: update_oz unset, using default 1.0\n");
            update_oz = 1.0;
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
    }
}

bool db_access::init_mysql(const char* host, const char* database,
                           const char* user, const char* pass )
{
    m_driver = get_driver_instance();
    m_conn = m_driver->connect(host, user, pass);
    m_conn->setSchema(database);

    m_pushactive = m_conn->prepareStatement("UPDATE active SET ticks=ticks+?,"
                                       " dirty=1 WHERE ontap = ?");


    m_pusharchive = m_conn->prepareStatement("INSERT INTO archive"
                                       " (name, ontap, size, volremaining) VALUES"
                                       " (?, ?, ?, ?)");
    
    m_getname = m_conn->prepareStatement("SELECT name FROM stock WHERE ontap=?");

    m_stmt = m_conn->createStatement();
}
db_access::~db_access()
{
    delete(m_conn);
    delete(m_pushactive);
    delete(m_pusharchive);
    delete(m_getname);
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
bool db_access::update()
{
    for(unsigned int i = 0; i < m_num_taps; i++)
    {
        /* Update flow meters if they've accrued enough ticks */
        if(m_flow_meters[i].ticks > m_flow_meters[i].update_ticks)
        {
            printf("Flow meter %d at ticks %d \n", i+1, m_flow_meters[i].ticks);
            /* SET ticks = ticks + m_flow_ticks[i] */
            m_pushactive->setInt(1, m_flow_meters[i].ticks);

            /* WHERE tap = i+1 */
            m_pushactive->setInt(2, i+1);

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
    /* Note: Using Unsigned, so only one case; no negatives */
    if(tap > m_num_taps)
        return false;

    m_flow_meters[tap-1].ticks = 0;
    return true;
}

int db_access::access(unsigned int tap)
{
    /* Check bounds first */
    /* Note: Using Unsigned, so only one case; no negatives */
    if(tap > m_num_taps)
        return 0;

    return m_flow_meters[tap-1].ticks;
}

void db_access::print()
{
    m_res = m_stmt->executeQuery("SELECT * FROM active");
    while(m_res->next())
    {
        std::cout << "sensor: " << m_res->getString("sensor");
        std::cout << " ontap: " << m_res->getString("ontap");
        std::cout << " ticks: " << m_res->getString("ticks");
        std::cout << " size: " << m_res->getString("size");
        std::cout << "\n";
    }
}

bool db_access::clear_db()
{
    m_res = m_stmt->executeQuery("UPDATE active SET ticks = 0");
    return true;
}

bool db_access::archive()
{
    /* Push to archive those that have been marked as dirty */
    m_res = m_stmt->executeQuery("SELECT ontap,ticks,size FROM active WHERE dirty=1");
    int size;
    int ticks;
    int ontap;
    std::string name;
    sql::ResultSet* temp=NULL;
    double volremaining;
    double volgone;
    double totalvol;
    while(m_res->next())
    {
        size = m_res->getInt("size");
        ticks = m_res->getInt("ticks");
        ontap = m_res->getInt("ontap");
        
        /* If there have been no ticks, do not report it */
        /* Sixtel */
        if(size == 0)
            totalvol = 41; /* In pints */
        /* Half barrel */
        else
            totalvol = 124; /* In pints */

        /* TODO: CHECK THIS MATH */
        volgone = (ticks * 8.0) / m_flow_meters[ontap-1].tpg; /* In pints */
        volremaining = totalvol - volgone;             /* In pints */

        /* Set archive beer's name using ontap as a reference */
        m_getname->setInt(1, ontap);
        temp = m_getname->executeQuery();

        /* Get name if there's a valid ontap */
        if(temp->next())
            name = temp->getString("name");
        /* Not a valid name set up for this beer! */
        else
            name = "NO NAME NO TAP";

        m_pusharchive->setString(1, name);
        m_pusharchive->setInt(2, ontap);
        m_pusharchive->setInt(3, size);
        m_pusharchive->setDouble(4, volremaining);

        m_pusharchive->execute();
    }

    if(temp != NULL)
        delete temp;

    return true;
}


static void prompt_pass(string& str) {
    char* pass = getpass("Enter Password: ");
    str = pass;
}

