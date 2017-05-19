#include <iostream>
#include "db_access.h"


static void prompt_pass(string& str);

/* ----------------------------------------------
 * Func: constructor
 * Instantiates the config parser class m_config
 * via the provided file name.
 * If still in a safe state, then runs the 
 * init_mysql function to connect to the database
 * ----------------------------------------------*/
db_access::db_access(const char* filename) 
  : m_safe(false), 
    m_config(filename),
    m_flow_meters(NULL),
    m_driver(NULL),
    m_conn(NULL),
    m_stmt(NULL),
    m_res(NULL),
    m_pushactive(NULL),
    m_pusharchive(NULL)
{
  m_safe = m_config.isSafe();

  string host, database;
  string user, pass;

  if(m_safe) {
    m_safe = load_config(host, database, user, pass);
  }

  if(m_safe) {
    m_safe = init_mysql(host.c_str(), database.c_str(), 
        user.c_str(), pass.c_str());
  }
}

/* --------------------------------------------
 * Func: deconstructor
 * Deletes all dynamically allocated variables 
 * --------------------------------------------*/
db_access::~db_access()
{
  if (m_flow_meters != NULL)
    delete[] m_flow_meters;
  if (m_stmt != NULL)
    delete m_stmt;
  if (m_pushactive != NULL)
    delete m_pushactive;
  if (m_pusharchive != NULL)
    delete m_pusharchive;
  if (m_conn != NULL)
    delete m_conn;
  if (m_res != NULL)
    delete m_res;
}

/* ------------------------------------------
 * Func: add
 * Adds value to ticks accrued at index tap 
 * ------------------------------------------*/
bool db_access::add(unsigned int tap, int value)
{
  /* Check bounds first */
  /* Note: Using Unsigned, so only one case; no negatives */
  if(tap >= m_num_taps)
    return false;

  m_flow_meters[tap].ticks += value;
  return true;
}

/* ---------------------------------------
 * Func: update
 * Updates DB with all accrue'd flow ticks 
 * Then zero's out local flow ticks array 
 * Unless force is set the ticks must be
 * above a threshold to update 
 * -------------------------------------*/
bool db_access::update(bool force)
{
  bool retval = true;
  for(unsigned int i = 0; i < m_num_taps; i++)
  {
    /* Update flow meters if they've accrued enough ticks 
     * Or if the request was forced and it's nonzero */
    if( (force && m_flow_meters[i].ticks > 0)
        || m_flow_meters[i].ticks > m_flow_meters[i].update_ticks)
    {
      try {
        /* First acquire mutex before attempting to update active table */
        std::lock_guard<std::mutex> guard (m_lock);

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

        /* Mutex m_lock implicitly free'd */
      }
      catch (sql::SQLException &e) {
        syslog(LOG_ERR, "Could not push to Active. (MySQL Error Code %d)", 
            e.getErrorCode());
        retval = false;
      }
    }
  }
  return retval;
} 

/* --------------------------------------------
 * Func: clear
 * Sets accrued flow ticks of one local index
 * back to zero. Use case yet unknown, but may
 * be needed 
 * ------------------------------------------*/
bool db_access::clear(unsigned int tap)
{
  /* Check bounds first */
  if(tap >= m_num_taps)
    return false;

  m_flow_meters[tap].ticks = 0;
  return true;
}

/* -----------------------------------
 * Func: clear_db 
 * This will zero out all ticks on the 
 * active DATABASE This does not clear 
 * any local accrue'd ticks 
 * ----------------------------------*/
bool db_access::clear_db()
{
  bool retval = true;
  /* First acquire mutex before running query */
  std::lock_guard<std::mutex> guard (m_lock);

  try {
  m_res = m_stmt->executeQuery("UPDATE active SET ticks = 0");
  }
  catch (sql::SQLException &e) {
    syslog(LOG_ERR, "Could not connect to database. (MySQL Error Code %d)", 
        e.getErrorCode());
    retval = false;
  }

  if (m_res != NULL)
    delete m_res;

  return retval;
  /* Mutex m_lock implicitly free'd */
}

/* ------------------------------------------------
 * Func: archive: 
 * Copies off all rows from active that have their
 * dirty bit set to archive. Returns TRUE if there
 * were no exceptions. Returns FALSE if any 
 * exceptions caught. 
 * -----------------------------------------------*/
bool db_access::archive()
{
  bool retval=true;
  /* First acquire mutex before attempting to update archive table */
  std::lock_guard<std::mutex> guard (m_lock);

  try {
    /* Push to archive those that have been marked as dirty */
    m_res = m_stmt->executeQuery("SELECT ontap,name,size,volremaining "
        "FROM active WHERE dirty=1");
    int ontap;
    std::string name;
    int size;
    double volremaining;
    while(m_res->next())
    {
      ontap = m_res->getInt("ontap");
      name = m_res->getString("name");
      size = m_res->getInt("size");
      volremaining = m_res->getDouble("volremaining");

      m_pusharchive->setString(1, name);
      m_pusharchive->setInt(2, ontap);
      m_pusharchive->setInt(3, size);
      m_pusharchive->setDouble(4, volremaining);

      m_pusharchive->execute();
    }
  }
  catch (sql::SQLException &e) {
    syslog(LOG_ERR, "Could not push to Archive. (MySQL Error Code %d)", 
        e.getErrorCode());
    retval = false;
  }
  if (m_res != NULL)
    delete m_res;

  return retval;
  /* Mutex m_lock implicitly free'd */
}

/* ------------------------------------------------
 * Func: access
 * Returns the current accrued ticks at index tap
 * Note this is just the value since last update() 
 * -----------------------------------------------*/
int db_access::access(unsigned int tap)
{
  /* Check bounds first */
  if(tap >= m_num_taps)
    return 0;

  return m_flow_meters[tap].ticks;
}

/* ------------------------------------------
 * Func: load_config
 * Once config_parser has been created, this
 * function will be able to pull all desired
 * configuration items and store them where
 * appropriate 
 * -----------------------------------------*/
bool db_access::load_config( string& host, string& database,
    string& user, string& pass )
{
  string pass_type;
  int num_taps = 16; /* TODO: Possibly make this config-settable */

  bool success = true;


  if(!m_config.getString(database, "database")) {
    syslog(LOG_ERR, "kegbot.cfg: No key \"database\" Found!");
    success = false;
  }

  if(!m_config.getString(host, "host")) {
    syslog(LOG_ERR, "kegbot.cfg: No key \"host\" Found!");
    success = false;
  }

  if(!m_config.getString(user, "user")) {
    syslog(LOG_ERR, "kegbot.cfg: No key \"user\" Found!");
    success = false;
  }

  if(!m_config.getString(pass_type, "pass_type")) {
    syslog(LOG_ERR, "kegbot.cfg: No key \"pass_type\" Found!");
    success = false;
  }

  if(success){
    if(pass_type == "plaintext" || pass_type == "base64") {
      if(!m_config.getString(pass, "pass")) {
        syslog(LOG_ERR, "kegbot.cfg: No key \"pass\" yet pass_type is %s!", 
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
    if(!m_config.getDouble(update_oz, "update_oz")){
      syslog(LOG_INFO, "kegbot.cfg: update_oz unset, using default %.2f", 
          DEFAULT_UPDATE_OZ);
      update_oz = DEFAULT_UPDATE_OZ;
    }

    /* Iterate through the config file (1-indexed)
     * If the flow meter doesn't have a config option in the file
     * use the default provided and print to info 
     */
    for(int i = 1; i <= num_taps; i++) {
      double tpg;
      if(!m_config.getDouble(tpg, "flow" + std::to_string(i) +  "_tpg")){
        syslog(LOG_INFO, "kegbot.cfg: flow%d_tpg unset, using default %.2f", 
            i, DEFAULT_FLOW_TPG);
        tpg = DEFAULT_FLOW_TPG;
      }

      m_flow_meters[i-1].tpg = tpg;
      m_flow_meters[i-1].update_ticks = tpg * update_oz / 128.0;
    }

    /* Grab times for active/archive update pushes */
    double active_rate  = DEFAULT_ACTIVE_RATE;
    double archive_rate = DEFAULT_ARCHIVE_RATE;
    if(!m_config.getDouble(active_rate, "active_rate")) {
      syslog(LOG_INFO, "kegbot.cfg: active_rate unset, using default %.2fs", 
          DEFAULT_ACTIVE_RATE);
    }
    if(!m_config.getDouble(archive_rate, "archive_rate")) {
      syslog(LOG_INFO, "kegbot.cfg: archive_rate unset, using default %.2fs", 
          DEFAULT_ARCHIVE_RATE);
    }

    /* If the conversion gave a < 1 value, just use default
     * to avoid a rounded off value of 0 for delay */
    m_active_rate  = (int)(active_rate  * S_TO_MS);
    if(m_active_rate == 0) 
      m_active_rate = DEFAULT_ACTIVE_RATE;

    m_archive_rate = (int)(archive_rate * S_TO_MS);
    if(m_archive_rate == 0)
      m_archive_rate = DEFAULT_ARCHIVE_RATE;
  }

  return success;
}

/* -------------------------------------------
 * Func: Print
 * Prints to stdout the current format of the 
 * active table. Also works as a simple 
 * example for fetching values 
 * -----------------------------------------*/
void db_access::print()
{
  /* First acquire mutex before running query */
  std::lock_guard<std::mutex> guard (m_lock);

  try {
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
  }
  catch (sql::SQLException &e) {
    syslog(LOG_ERR, "Could not connect to database. (MySQL Error Code %d)", 
        e.getErrorCode());
  }
  if (m_res != NULL)
    delete m_res;
  /* Mutex m_lock implicitly free'd */
}

/* ------------------------------------------
 * Func: init_mysql
 * Initializes the connection to the desired
 * host and database. Initializes all common
 * SQL Types for the class as well 
 * ----------------------------------------*/
bool db_access::init_mysql(const char* host, const char* database,
    const char* user, const char* pass )
{
  bool retval = true;
  try {
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
  }
  catch (sql::SQLException &e) {
    syslog(LOG_ERR, "Could not connect to database. (MySQL Error Code %d)", e.getErrorCode());
    retval = false;
  }
  return retval;
}






static void prompt_pass(string& str) {
  char* pass = getpass("Enter Password: ");
  str = pass;
}

