#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <fstream>
#include <string>
#include <unordered_map>

using std::string;
using std::unordered_map;

class config_parser{
    public:
      config_parser();
      config_parser(const char* filename);

      /* Func: getInt, getDouble, getString
       * Takes in key from provided file
       * If paired value is found, casts as an int / double
       * returns false if key not found, true otherwise */
      bool getInt (int& value, const string& key);
      bool getDouble (double& value, const string& key);
      bool getString (string& value, const string& key);

      /* Quick string literal wrapper functions for the above */
      bool getInt (int& value, const char* key) 
        { return getInt(value, string(key)); }
      bool getDouble (double& value, const char* key)
        { return getDouble(value, string(key)); }
      bool getString (string& value, const char* key)
        { return getString(value, string(key)); }

    private:
      void fill_dict(const char* filename);
      unordered_map <string, string>  m_dict;
};
#endif
