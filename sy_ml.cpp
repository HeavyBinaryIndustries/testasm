#include <iostream>
#include <fstream>
#include <vector>
#include <any>
#include <string>
#include <variant>
#include "sy_ml.h"

using namespace std;

MLList::MLList(string name, vector<pair<string, string>> attr) {
  Name = name;
  Attributes = attr;
}

MLList::~MLList() {
  for (int i = 0; i < Contents.size(); i++) {
    if (Contents[i].index() == 1) {
      delete get<1>(Contents[i]);
    }
  }
}

string MLList::GenStr() {
  string start_tag, content;
  start_tag = "<" + Name;
  for (int i = 0; i < Attributes.size(); i++) {
    start_tag += " " + Attributes[i].first + "=\"" + Attributes[i].second + "\"";
  }
  if (Contents.size()) {
    start_tag += ">";
    for (int i = 0; i < Contents.size(); i++) {
      if (Contents[i].index() == 0) {
        content += get<string>(Contents[i]);
      } else if (Contents[i].index() == 1) {
        content += get<MLList*>(Contents[i])->GenStr();
      }
    }
    return start_tag + content + "</" + Name + ">";
  } else {
    return start_tag + " />";
  }
}

string& MLList::AddContent(string str) {
  Contents.push_back(str);
  return get<string>(Contents.back());
}

MLList* MLList::AddList(string name, vector<pair<string, string>> attr) {
  MLList* c = new MLList(name, attr);
  Contents.push_back(c);
  return c;
}

XHTML::XHTML() {
  Html = (new MLList("html", {{"xmlns", "http://www.w3.org/1999/xhtml"}}));
  List.push_back(Html);
  Body = Html->AddList("body");
}

XHTML::~XHTML() {
  for (int i = 0; i < List.size(); i++) {
    delete List[i];
  }
}

MLList* XHTML::GetBody() {
  return Body;
}

string XHTML::GenStr() {
  string str = "<?xml version='1.0' encoding='utf-8'?>";
  for (int i = 0; i < List.size(); i++) {
    str += List[i]->GenStr();
  }
  return str;
}

int GenSelect(MLList* list, string name, vector<string>options, vector<string>labels, string selected) {
  list = list->AddList("select", {{"name", name}});
  MLList* s = NULL;
  if (selected.size()) {
    s = list->AddList("option", {{"value", selected}});
  }
  if (options.size() != labels.size()) {
    return -1;
  }
  for (int i = 0; i < options.size(); i++) {
    if (options[i] == selected) {
      s->AddContent(labels[i]);
    } else {
      list->AddList("option", {{"value", options[i]}})->AddContent(labels[i]);
    }
  }
  return 0;
}
