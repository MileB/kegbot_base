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

      bool getInt (int& value, const string& key);
      bool getDouble (double& value, const string& key);
      bool getString (string& value, const string& key);

      bool getInt (int& value, const char* key) { return getInt(value, string(key)); }
      bool getDouble (double& value, const char* key);
      bool getString (string& value, const char* key);

    private:
      void fill_dict(const char* filename);
      unordered_map <string, string>  m_dict;
};
#endif
