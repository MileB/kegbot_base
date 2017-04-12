#include <iostream>
#include "config_parser.h"

using namespace std;

int main(int argc, char** argv)
{
  config_parser conf("./kegbot.cfg");
  string key("flow1_gpt");
  int flow;
  double dflow;
  string str;

  if (!conf.getInt(flow, key))
    cout << "sux" << endl;
  cout << flow << endl;

  if (!conf.getDouble(dflow, key))
    cout << "sux" << endl;
  cout << dflow << endl;

  if (!conf.getInt(flow, "flow2_gpt"))
    cout << "sux" << endl;
  cout << flow << endl;

  if (!conf.getString(str, key))
    cout << "sux" << endl;
  cout << str << endl;

  return 0;
}
