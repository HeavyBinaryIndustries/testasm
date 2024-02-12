#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <tuple>
#include "ws_asm.h"
#include "sy_ml.h"
#include <sstream>
#include <stdlib.h>

using namespace std;

vector<string> instnames = {
  "add",
  "or",
  "adc",
  "sbb",
  "and",
  "sub",
  "xor",
  "cmp",
  "inc",
  "dec",
  "push",
  "pop",
  "imul",
  "jcc",
  "test",
  "xchg",
  "mov",
  "lea",
  "nop",
  "ret",
  "call",
  "jmp",
  "div",
  "setcc",
  "sal",
  "sar",
  "shl",
  "shr"
};

vector<string>ccstrings = {
  "o",
  "no",
  "b",
  "nae",
  "c",
  "nb",
  "ae",
  "nc",
  "z",
  "e",
  "nz",
  "ne",
  "be",
  "na",
  "nbe",
  "a",

  "s",
  "ns",
  "p",
  "pe",
  "np",
  "po",
  "l",
  "nge",
  "nl",
  "ge",
  "le",
  "ng",
  "nle",
  "g"
};

vector<string> optypes = {
  "none",
  "reg",
  "idr",
  "imm"
};

vector<string> labeloptypes = {
  "なし",
  "汎用レジスタ",
  "間接参照",
  "即値"
};

vector<string> regtypes = {
  "none",
  "eax",
  "ecx",
  "edx",
  "ebx",
  "esp",
  "ebp",
  "esi",
  "edi",
  "ax",
  "cx",
  "dx",
  "bx",
  "sp",
  "bp",
  "si",
  "di",
  "al",
  "cl",
  "dl",
  "bl",
  "ah",
  "ch",
  "dh",
  "bh"
};

vector <string> labelregtypes = {
  "なし",
  "eax",
  "ecx",
  "edx",
  "ebx",
  "esp",
  "ebp",
  "esi",
  "edi",
  "ax",
  "cx",
  "dx",
  "bx",
  "sp",
  "bp",
  "si",
  "di",
  "al",
  "cl",
  "dl",
  "bl",
  "ah",
  "ch",
  "dh",
  "bh"
};

vector <string> scales = {
  "1",
  "2",
  "4",
  "8"
};

vector <string> sizes = {
  "1",
  "2",
  "4"
};

vector<vector<string>> ParseParam(char* str) {
  int start = 0, len = 0;
  std::vector<std::vector<std::string>> vec;
  std::string s(str);
  while(*str) {
    switch (*str) {
      case '=':
        vec.push_back({s.substr(start, len)});
        start+=len+1;
        len = 0;
        break;
      case '&':
        vec.back().push_back(s.substr(start, len));
        start+=len+1;
        len = 0;
        break;
      default:
        len++;
        break;
    }
    str++;
  }
  return vec;
}

string GetParam(vector<vector<string>> vec, string p) {
  for (int i = 0; i < vec.size(); i++) {
    if (vec[i].size() < 2) continue;
    if (vec[i][0] == p) return vec[i][1];
  }
  return "";
}

int isnum(string n) {
  if (!n.size()) return 0;

  for (int i = n[0] == '-'; i < n.size(); i++) {
    if (n[i] < '0' || '9' < n[i]) return 0;
  }
  return 1;
}

pair<variant<wsa::Immediate, wsa::Register, wsa::Indirect>, int> GenOprd(string type, string base, string index, string scale, string offset, string width) {
  if (!isnum(scale) || !isnum(width)) return {wsa::Register(""), -1};
  if (offset != "" && !isnum(offset)) return {wsa::Register(""), -1};
  int s = stoi(scale);
  int o = offset == "" ? 0 : stoi(offset);
  int w = stoi(width);
  if (type == "reg") {
    wsa::Register reg(base);
    if (reg.GetRegType() == wsa::RegType::None) {
      return {wsa::Register(""), 0};
    }
    return {reg, 0};
  } else if (type == "idr") {
    if (s != 1 && s != 2 && s != 4 && s != 8) return {wsa::Register(""), -1};
    if (w != 1 && w != 2 && w != 4) return {wsa::Register(""), -1};
    return {wsa::Indirect(base, index, s, o, w), 0};
  } else if (type == "imm") {
    return {wsa::Immediate(o, 1), 0};
  } else {
    return {wsa::Register(""), -1};
  }
}

string OprdString(string type, string base, string index, string scale, string offset, string width) {
  if (type == "reg") {
    return "%" + base;
  } else if (type == "idr") {
    string s = offset;
    s += "(";
    if (base != "none") {
      s += "%" + base;
    }
    s += ", ";
    if (index != "none") {
      s += "%" + index;
    }
    s += ", ";
    s += scale;
    s += ")";

    return s;
  } else if (type == "imm") {
    if (offset == "") return "0";
    return offset;
  } else {
    return "";
  }
}

int main() {
  char buff[0xffff];
  strncpy(buff, getenv("QUERY_STRING"), 0xfffe);
  vector<vector<string>> params = ParseParam(buff);

  XHTML xhtml;
  xhtml.GetBody()->AddList("h1")->AddContent("test assembler");
  MLList* form = xhtml.GetBody()->AddList("div", {{"style", "width: 1080px;"}})->AddList("form", {{"action", "testasm.cgi"}, {"method", "get"}});
  MLList* table = form->AddList("table", {{"border", "1"}, {"width", "100%"}});

  MLList* line = table->AddList("tr");
  line->AddList("th", {{"width", "30%"}});
  line->AddList("th", {{"width", "12%"}})->AddContent("命令");
  line->AddList("th", {{"width", "10%"}})->AddContent("cc");
  line->AddList("th", {{"width", "16%"}})->AddContent("第一オペランド");
  line->AddList("th", {{"width", "16%"}})->AddContent("第ニオペランド");
  line->AddList("th", {{"width", "16%"}})->AddContent("第三オペランド");

  line = table->AddList("tr");
  line->AddList("td");
  GenSelect(line->AddList("td"), "instruction", instnames, instnames, GetParam(params, "instruction"));
  GenSelect(line->AddList("td"), "cc", ccstrings, ccstrings, GetParam(params, "cc"));
  GenSelect(line->AddList("td"), "oprd1type", optypes, labeloptypes, GetParam(params, "oprd1type"));
  GenSelect(line->AddList("td"), "oprd2type", optypes, labeloptypes, GetParam(params, "oprd2type"));
  GenSelect(line->AddList("td"), "oprd3type", optypes, labeloptypes, GetParam(params, "oprd3type"));

  line = table->AddList("tr");
  line->AddList("th")->AddContent("ベース/直接参照");
  line->AddList("td");
  line->AddList("td");
  GenSelect(line->AddList("td"), "oprd1base", regtypes, labelregtypes, GetParam(params, "oprd1base"));
  GenSelect(line->AddList("td"), "oprd2base", regtypes, labelregtypes, GetParam(params, "oprd2base"));
  GenSelect(line->AddList("td"), "oprd3base", regtypes, labelregtypes, GetParam(params, "oprd3base"));

  line = table->AddList("tr");
  line->AddList("th")->AddContent("インデックス");
  line->AddList("td");
  line->AddList("td");
  GenSelect(line->AddList("td"), "oprd1index", regtypes, labelregtypes, GetParam(params, "oprd1index"));
  GenSelect(line->AddList("td"), "oprd2index", regtypes, labelregtypes, GetParam(params, "oprd2index"));
  GenSelect(line->AddList("td"), "oprd3index", regtypes, labelregtypes, GetParam(params, "oprd3index"));

  line = table->AddList("tr");
  line->AddList("th")->AddContent("スケール");
  line->AddList("td");
  line->AddList("td");
  GenSelect(line->AddList("td"), "oprd1scale", scales, scales, GetParam(params, "oprd1scale"));
  GenSelect(line->AddList("td"), "oprd2scale", scales, scales, GetParam(params, "oprd2scale"));
  GenSelect(line->AddList("td"), "oprd3scale", scales, scales, GetParam(params, "oprd3scale"));

  line = table->AddList("tr");
  line->AddList("th")->AddContent("オペランドサイズ(間接参照)");
  line->AddList("td");
  line->AddList("td");
  GenSelect(line->AddList("td"), "oprd1width", sizes, sizes, GetParam(params, "oprd1width"));
  GenSelect(line->AddList("td"), "oprd2width", sizes, sizes, GetParam(params, "oprd2width"));
  GenSelect(line->AddList("td"), "oprd3width", sizes, sizes, GetParam(params, "oprd3width"));

  line = table->AddList("tr");
  line->AddList("th")->AddContent("オフセット/即値/アドレス(10進数)");
  line->AddList("td");
  line->AddList("td");
  line->AddList("td")->AddList("input", {{"type", "number"}, {"name", "oprd1offset"}});
  line->AddList("td")->AddList("input", {{"type", "number"}, {"name", "oprd2offset"}});
  line->AddList("td")->AddList("input", {{"type", "number"}, {"name", "oprd3offset"}});

  form->AddList("input", {{"type", "submit"}, {"value", "確定"}, {"style", "width:300px; height:48px; font-size:36px; float:right;"}});

  xhtml.GetBody()->AddList("h4")->AddContent("例");
  MLList* list = xhtml.GetBody()->AddList("ul");
  list->AddList("li")->AddList("a", {{"href", "/cgi-bin/testasm.cgi?instruction=add&cc=o&oprd1type=reg&oprd2type=reg&oprd3type=none&oprd1base=eax&oprd2base=ecx&oprd3base=none&oprd1index=none&oprd2index=none&oprd3index=none&oprd1scale=1&oprd2scale=1&oprd3scale=1&oprd1width=1&oprd2width=1&oprd3width=1&oprd1offset=&oprd2offset=&oprd3offset="}})->AddContent("add %eax, %ecx");
  list->AddList("li")->AddList("a", {{"href", "/cgi-bin/testasm.cgi?instruction=push&cc=o&oprd1type=reg&oprd2type=none&oprd3type=none&oprd1base=ebp&oprd2base=none&oprd3base=none&oprd1index=none&oprd2index=none&oprd3index=none&oprd1scale=1&oprd2scale=1&oprd3scale=1&oprd1width=1&oprd2width=1&oprd3width=1&oprd1offset=&oprd2offset=&oprd3offset="}})->AddContent("push %ebp");
  list->AddList("li")->AddList("a", {{"href", "/cgi-bin/testasm.cgi?instruction=mov&cc=o&oprd1type=reg&oprd2type=reg&oprd3type=none&oprd1base=esp&oprd2base=ebp&oprd3base=none&oprd1index=none&oprd2index=none&oprd3index=none&oprd1scale=1&oprd2scale=1&oprd3scale=1&oprd1width=1&oprd2width=1&oprd3width=1&oprd1offset=&oprd2offset=&oprd3offset="}})->AddContent("mov %esp, %ebp");
  list->AddList("li")->AddList("a", {{"href", "/cgi-bin/testasm.cgi?instruction=mov&cc=o&oprd1type=imm&oprd2type=reg&oprd3type=none&oprd1base=none&oprd2base=eax&oprd3base=none&oprd1index=none&oprd2index=none&oprd3index=none&oprd1scale=1&oprd2scale=1&oprd3scale=1&oprd1width=1&oprd2width=1&oprd3width=1&oprd1offset=&oprd2offset=&oprd3offset="}})->AddContent("mov $0, %eax");

  xhtml.GetBody()->AddList("h4")->AddContent("参考");
  xhtml.GetBody()->AddList("h5")->AddContent("IA-32 インテル® アーキテクチャソフトウェア・デベロッパーズ・マニュアル");
  MLList* ref = xhtml.GetBody()->AddList("ul");
  ref->AddList("li")->AddList("a", {{"href", "https://www.intel.co.jp/content/dam/www/public/ijkk/jp/ja/documents/developer/IA32_Arh_Dev_Man_Vol1_Online_i.pdf"}})->AddContent("上巻");
  ref->AddList("li")->AddList("a", {{"href", "https://www.intel.co.jp/content/dam/www/public/ijkk/jp/ja/documents/developer/IA32_Arh_Dev_Man_Vol2A_i.pdf"}})->AddContent("中巻A");
  ref->AddList("li")->AddList("a", {{"href", "https://www.intel.co.jp/content/dam/www/public/ijkk/jp/ja/documents/developer/IA32_Arh_Dev_Man_Vol2B_i.pdf"}})->AddContent("中巻B");
  ref->AddList("li")->AddList("a", {{"href", "https://www.intel.co.jp/content/dam/www/public/ijkk/jp/ja/documents/developer/IA32_Arh_Dev_Man_Vol3_i.pdf"}})->AddContent("下巻");

  ref->AddList("li")->AddList("a", {{"href", "https://www.intel.co.jp/content/dam/www/public/ijkk/jp/ja/documents/developer/IA32_Arh_Dev_Man_Vol2B_i.pdf#page=316"}})->AddContent("オペコードマップ");

  MLList* log = form->AddList("div")->AddList("textarea", {{"name", "log"}, {"style", "width:100%; height:120px"}});
  MLList* data = form->AddList("div")->AddList("textarea", {{"name", "data"}, {"style", "width:100%; height:120px"}});

  log->AddContent("");
  data->AddContent("");

  string inst = GetParam(params, "instruction");

  if (inst.size()) {
    string inst = GetParam(params, "instruction"), cc = GetParam(params, "cc");
    if (inst == "jcc") {
      inst = "j" + cc;
    } else if (inst == "setcc") {
      inst = "set" + cc;
    }

    tuple<string, string, string, string, string, string> ops1, ops2, ops3;
    ops1 = {
      GetParam(params, "oprd1type"),
      GetParam(params, "oprd1base"),
      GetParam(params, "oprd1index"),
      GetParam(params, "oprd1scale"),
      GetParam(params, "oprd1offset"),
      GetParam(params, "oprd1width")
    };
    ops2 = {
      GetParam(params, "oprd2type"),
      GetParam(params, "oprd2base"),
      GetParam(params, "oprd2index"),
      GetParam(params, "oprd2scale"),
      GetParam(params, "oprd2offset"),
      GetParam(params, "oprd2width")
    };
    ops3 = {
      GetParam(params, "oprd3type"),
      GetParam(params, "oprd3base"),
      GetParam(params, "oprd3index"),
      GetParam(params, "oprd3scale"),
      GetParam(params, "oprd3offset"),
      GetParam(params, "oprd3width")
    };

    log->AddContent(inst + " " + apply(OprdString, ops1) + ", " + apply(OprdString, ops2) + " " + apply(OprdString, ops3));

    pair<variant<wsa::Immediate, wsa::Register, wsa::Indirect>, int> op(wsa::Register(""), 1);

    int error = 0;

    vector<variant<wsa::Immediate, wsa::Register, wsa::Indirect>> vecop = {};
    if (get<0>(ops1) != "none") {
      op = apply(GenOprd, ops1);
      if (op.second) {
        error = 1;
      } else {
        vecop.push_back(op.first);
      }
    }
    if (!error) {
      if (get<0>(ops2) != "none") {
        if (get<0>(ops1) == "none") {
          error = 1;
        }
        op = apply(GenOprd, ops2);
        if (op.second) {
          error = 1;
        } else {
          vecop.push_back(op.first);
        }
      }
    }
    if (!error) {
      if (get<0>(ops3) != "none") {
        if (get<0>(ops2) == "none") {
          error = 1;
        }
        op = apply(GenOprd, ops3);
        if (op.second) {
          error = 1;
        } else {
          vecop.push_back(op.first);
        }
      }
    }
    if (!error) {
      uint8_t m[0xff];
      wsa::WSA_Buff wsabuff(m);
      wsa::InstructionSet set;
      if (set.Encode(&wsabuff, inst, vecop)) {
        log->AddContent("\nオペランドの組み合わせに合致するオペコードがありません");
      } else {
        wsabuff.Commit();
        stringstream ss;
        for (uint8_t* p = m; p < wsabuff.GetCurrent(); p++) {
          if (*p < 0x10) ss << "0";
          ss << std::hex << (int)*p;
          ss << " ";
        }
        data->AddContent(ss.str());
      }
    } else {
      log->AddContent("\nオペランドの形式が間違っています");
    }
  }


  printf("Content-Type: text/html\r\n\r\n");
  cout << xhtml.GenStr();
}
