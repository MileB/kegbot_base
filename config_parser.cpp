#include "config_parser.h"
#include <iostream>

/* Helper function 
 * Strips all whitespace
 * characters out of str */
void strip(string& str);

config_parser::config_parser(const char* filename)
{
  printf("In constructor...");
  fill_dict(filename);
}

void config_parser::fill_dict(const char* filename)
{
  std::ifstream file(filename);
  string line;
  string key, value;
  if(file.is_open()) {
    while(file.good()) {
      getline(file, line);

      strip(line);
      /* Ignore commented and empty lines */
      if(!line.empty() && line[0] != '#'){
        //std::cout << line << std::endl;
        size_t sep = line.find(':');

        /* Make sure we have a valid seperator ':' */
        if (sep != string::npos)
          key = line.substr(0, sep);
        strip(key);

        /* Grab value to accompany the key */
        if (sep != string::npos && !key.empty())
          value = line.substr(sep+1, line.length() - sep);
        strip(value);

        /* Got a key value pair, add to dictionary */
        if (!key.empty() && !value.empty())
          m_dict.insert( {{key, value}} );
      }

      /* Keep our strings clean between loops */
      line.clear();
      key.clear();
      value.clear();
    }
  }
}

/* Helper function */
void strip(string& str)
{
  string::iterator i = str.begin();
  while(i != str.end())
  {
    if (std::isspace(*i)) 
      str.erase(i);

    else
      i++;
  }
}

bool config_parser::getInt (int& value, const string& key)
{
  unordered_map<string,string>::const_iterator iter;
  iter = m_dict.find(key);

  /* Key not found! */
  if (iter == m_dict.end())
    return false;

  /* Got our key, return its paired value */
  value = std::stoi(iter->second);
  return true;
}

bool config_parser::getDouble (double& value, const string& key)
{
  unordered_map<string,string>::const_iterator iter;
  iter = m_dict.find(key);

  /* Key not found! */
  if (iter == m_dict.end())
    return false;

  /* Got our key, return its paired value */
  value = std::stod(iter->second);
  return true;
}

bool config_parser::getString (string& value, const string& key)
{
  unordered_map<string,string>::const_iterator iter;
  iter = m_dict.find(key);

  /* Key not found! */
  if (iter == m_dict.end())
    return false;

  /* Got our key, return its paired value */
  value = iter->second;
  return true;
}
