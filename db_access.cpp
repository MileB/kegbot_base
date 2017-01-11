#include "db_access.h"
db_access::db_access(const char* db, const char* user, const char* pass, unsigned int num_taps)
{
    m_driver = get_driver_instance();
    m_conn = m_driver->connect("127.0.0.1:3306", user, pass);
    m_conn->setSchema(db);

    m_pushactive = m_conn->prepareStatement("UPDATE active SET ticks = ticks + ?"
                                       " WHERE ontap = ?");

    m_pusharchive = m_conn->prepareStatement("INSERT INTO archive"
                                       " (name, ontap, size, volremaining) VALUES"
                                       " (?, ?, ?, ?)");
    
    m_getname = m_conn->prepareStatement("SELECT name FROM stock WHERE ontap=?");

    m_stmt = m_conn->createStatement();

    /* TODO: If the table currently has less then num_taps entries
     * we should be able to just create them here. */
    /* Initialize internal counter for flow taps */
    m_num_taps = num_taps;
    m_flow_ticks = new int[num_taps] ();
}

db_access::~db_access()
{
    delete(m_conn);
    delete(m_pushactive);
    delete(m_pusharchive);
    delete(m_getname);
    delete(m_stmt);
    delete(m_res);
    delete(m_flow_ticks);
}
bool db_access::add(unsigned int tap, int value)
{
    /* Check bounds first */
    /* Note: Using Unsigned, so only one case; no negatives */
    if(tap > m_num_taps)
        return false;

    m_flow_ticks[tap] += value;
    return true;
}
bool db_access::update()
{
    for(unsigned int i = 0; i < m_num_taps; i++)
    {
        /* SET ticks = ticks + m_flow_ticks[i] */
        m_pushactive->setInt(1, m_flow_ticks[i]);

        /* WHERE tap = i+1 */
        m_pushactive->setInt(2, i+1);

        m_pushactive->execute();

        /* Clear our accumulator */
        m_flow_ticks[i] = 0;
    }
    return true;
}

bool db_access::clear(unsigned int tap)
{
    /* Check bounds first */
    /* Note: Using Unsigned, so only one case; no negatives */
    if(tap > m_num_taps)
        return false;

    m_flow_ticks[tap+1] = 0;
    return true;
}

int db_access::access(unsigned int tap)
{
    /* Check bounds first */
    /* Note: Using Unsigned, so only one case; no negatives */
    if(tap > m_num_taps)
        return 0;

    return m_flow_ticks[tap+1];
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
    m_res = m_stmt->executeQuery("SELECT ontap,ticks,size FROM active");
    int size;
    int ticks;
    int ontap;
    std::string name;
    sql::ResultSet* temp;
    double volremaining;
    double volgone;
    double totalvol;
    while(m_res->next())
    {
        size = m_res->getInt("size");
        ticks = m_res->getInt("ticks");
        ontap = m_res->getInt("ontap");
        
        /* Sixtel */
        if(size == 0)
            totalvol = 41;
        /* Half barrel */
        else
            totalvol = 124;

        /* TODO: CHECK THIS MATH */
        volgone = (ticks * 8.0) / 10300.0;
        volremaining = totalvol - volgone;

        /* Set archive beer's name using ontap as a reference */
        m_getname->setInt(1, ontap);
        temp = m_getname->executeQuery();

        /* Get name if there's a valid ontap */
        if(temp->next())
            name = temp->getString("name");
        else
            name = "NO NAME NO TAP";

        m_pusharchive->setString(1, name);
        m_pusharchive->setInt(2, ontap);
        m_pusharchive->setInt(3, size);
        m_pusharchive->setDouble(4, volremaining);

        m_pusharchive->execute();
    }

    delete temp;
    return true;
}
