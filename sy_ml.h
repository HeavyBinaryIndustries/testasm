#ifndef SYML_H
#define SYML_H

#include <iostream>
#include <fstream>
#include <vector>
#include <any>
#include <variant>
#include <string>

using namespace std;


class MLList {
private:
  string Name;
  vector<pair<string, string>> Attributes;
  vector<variant<string, MLList*>> Contents;

public:
  MLList(string name, vector<pair<string, string>> attr = {});
  ~MLList();

  string GenStr();
  string GetName();

  string& AddContent(string str);
  MLList* AddList(string name, vector<pair<string, string>> attr = {});
};

class XHTML {
private:
  vector<MLList*> List;
  MLList* Html;
  MLList* Body;

public:
  XHTML();
  ~XHTML();

  MLList* GetBody();

  string GenStr();
};

int GenSelect(MLList* List, string name, vector<string>options, vector<string>labels, string selected);

#endif
