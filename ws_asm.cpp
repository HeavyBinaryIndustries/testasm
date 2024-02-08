#ifndef WSA_CPP
#define WSA_CPP

#include <iostream>

#include <stdint.h>
#include <cstddef>
#include <cstring>
#include <vector>
#include <variant>

#include "ws_asm.h"
#include "hashtable.cpp"

using namespace std;
using namespace wsa;

namespace wsa {
  int WSA_Buff::Reset() {
    EnModRM = EnSIB = 0;
    Addr16 = AltOprdSize = 0;
    Opecode = {};

    ModRM.Mod = ModRM.Reg = ModRM.RM = -1;
    SIB.Scale = SIB.Index = SIB.Base = -1;

    Offset = 0;
    nOffsetBytes = 0;

    ImmediateData = 0;
    nImmediateBytes = 0;

    return 0;
  }

  WSA_Buff::WSA_Buff() {
    Buff = NULL;
    Current = NULL;
    delbuff = 0;
  }

  WSA_Buff::WSA_Buff(int size) {
    Buff = new uint8_t[size];
    Current = Buff;

    delbuff = 1;

    Reset();
  }

  WSA_Buff::WSA_Buff(uint8_t* buff) {
    Buff = buff;
    Current = Buff;

    delbuff = 0;

    Reset();
  }

  WSA_Buff::~WSA_Buff() {
    if (delbuff) delete(Buff);
  }

  int WSA_Buff::SetBuff(uint8_t* buff) {
    if (delbuff) {
      delete Buff;
    }
    Buff = buff;
    Current = buff;
    return 0;
  }

  int WSA_Buff::Print() {
    for (uint8_t* ptr = Buff; ptr < Current; ptr++) {
      printf("%02x ", *ptr);
    }
    puts("");
    return 0;
  }

  int WSA_Buff::SetOpecode(vector<uint8_t> opc) {
    if (Opecode.size()) return -1;
    Opecode = opc;
    return 0;
  }

  int WSA_Buff::SetModRM(int reg, int mod, int rm) {
    if (reg >= 0) {
      if (ModRM.Reg < 0) {
        ModRM.Reg = reg;
      } else {
        return -1;
      }
    }
    if (mod >= 0) {
      if (ModRM.Mod < 0) {
        ModRM.Mod = mod;
      }
      else {
        return -1;
      }
    }

    if (rm >= 0) {
      if (ModRM.RM < 0) {
        ModRM.RM = rm;
      } else {
        return -1;
      }
    }

    EnModRM = 1;
    return 0;
  }

  int WSA_Buff::SetSIB(int s, int i, int b) {
    if (EnSIB) {
      return -1;
    } else {
      EnSIB = 1;

      SIB.Scale = s;
      SIB.Index = i;
      SIB.Base = b;
      return 0;
    }
  }

  int WSA_Buff::SetOffset(int32_t off, int nb) {
    if (nOffsetBytes) {
      return -1;
    }
    Offset = off;
    nOffsetBytes = nb;
    return 0;
  }

  int WSA_Buff::SetImmData(uint32_t data, int w) {
    if (nImmediateBytes) {
      return -1;
    } else {
      ImmediateData = data;
      nImmediateBytes = w;
      return 0;
    }
  }

  int WSA_Buff::Enable16() {
    Addr16 = 1;
    return 0;
  }
  int WSA_Buff::EnAltOprdSize() {
    AltOprdSize = 1;
    return 0;
  }

  int WSA_Buff::AddToLastOpecodeByte(int i) {
    if (Opecode.size()) {
      Opecode.back() += i;
      return 0;
    } else {
      return -1;
    }
  }

  uint8_t* WSA_Buff::GetCurrent() {
    return Current;
  }

  int WSA_Buff::Commit() {
    if (Addr16) {
      *Current++ = PREF_ADDR_SIZE;
    }
    if (AltOprdSize) {
      *Current++ = PREF_OPRD_SIZE;
    }

    for (int i = 0; i < Opecode.size(); i++) {
      *Current++ = Opecode[i];
    }

    if (EnModRM) {
      *Current++ = ((ModRM.Mod & 0x3) << 6)
                  | ((ModRM.Reg & 0x7) << 3)
                  | ((ModRM.RM & 0x7));
    }

    if (EnSIB) {
      *Current++ = ((SIB.Scale & 0x3) << 6)
                  | ((SIB.Index & 0x7) << 3)
                  | ((SIB.Base & 0x7));
    }

    if (nOffsetBytes) {
      PrevOff = Current;
      memcpy(Current, &Offset, nOffsetBytes);
      Current += nOffsetBytes;
    }

    if (nImmediateBytes) {
      PrevImm = Current;
      memcpy(Current, &ImmediateData, nImmediateBytes);
      Current += nImmediateBytes;
    }

    Reset();
    return 0;
  }

  int Operand::SetType(OperandType type) {
    Type = type;
    return 0;
  }

  OperandType Operand::GetType() {
    return Type;
  }

  int Operand::SetWid(int w) {
    bWid = w;
    return bWid;
  }

  int Operand::GetWid() {
    return bWid;
  }

  Immediate::Immediate(uint32_t data, int s, int w) {
    Type = OperandType::Imm;
    bWid = w;

    Data = data;
    Signed = s;
  }

  int Immediate::Fill(WSA_Buff* buff, int nb) {
    return buff->SetImmData(Data, nb);
  }

  int Immediate::GetWid() {
    if (bWid) return bWid;
    if (Signed) {
      return EnoughBytesSigned((int32_t)Data);
    } else {
      return EnoughBytesUnSigned(Data);
    }
  }

  int Immediate::SizeSpecified() {
    if (bWid) return 1;
    return 0;
  }

  uint32_t Immediate::GetData() {
    return Data;
  }

  int Immediate::IsSigned() {
    return Signed;
  }

  Register::Register(string str) {
    Type = OperandType::Reg;

    bWid = 0;
    if (str[0] == 'e') {
      bWid = 4;
      str = str.substr(1);
    }
    if (str.size() != 2) {
      RType = RegType::None;
    } else if (str == "sp") {
      if (!bWid) bWid = 2;
      RType = RegType::SP;
    } else if (str == "bp") {
      if (!bWid) bWid = 2;
      RType = RegType::BP;
    } else if (str == "si") {
      if (!bWid) bWid = 2;
      RType = RegType::SI;
    } else if (str == "di") {
      if (!bWid) bWid = 2;
      RType = RegType::DI;
    } else if ('a' <= str[0] && str[0] <= 'd' && str != "di") {
      switch(str[0]) {
        case 'a':
          RType = RegType::A;
          break;
        case 'c':
          RType = RegType::C;
          break;
        case 'd':
          RType = RegType::D;
          break;
        case 'b':
          RType = RegType::B;
          break;
      }
      switch (str[1]) {
        case 'x':
          if (!bWid) bWid = 2;
          break;

        case 'h':
          if (bWid) {
            RType = RegType::None;
          } else {
            bWid = 1;
          }
          HighByte = 1;
          break;

        case 'l':
          if (bWid) {
            RType = RegType::None;
          } else {
            bWid = 1;
          }
          HighByte = 0;
          break;

        default:
          RType = RegType::None;
          break;
      }
    } else {
      RType = RegType::None;
    }
    if (RType == RegType::None) Type = OperandType::None;
  }

  Register::Register(RegType type, int w, int high) {
    Type = OperandType::Reg;
    bWid = w;

    RType = type;
    HighByte = high;
  }

  RegType Register::GetRegType() {
    return RType;
  }

  int Register::IsHighByte() {
    return HighByte;
  }

  int Register::Encode() {
    switch(bWid) {
      case 1: {
        if (HighByte) {
          switch (RType) {
            case RegType::A: return 4; //reg ah
            case RegType::C: return 5;
            case RegType::D: return 6;
            case RegType::B: return 7;
            default: return -1;
          }
        } else {
          switch (RType) {
            case RegType::A: return 0; //reg al
            case RegType::C: return 1;
            case RegType::D: return 2;
            case RegType::B: return 3;
            default: return -1;
          }
        }
      }
      case 2: {
        return EncodeRegType(RType);
      }
      case 4: {
        return EncodeRegType(RType);
      }
      default: return -1;
    }
  }

  int Register::FillRegField(WSA_Buff* buff) {
    return buff->SetModRM(Encode());
  }

  int Register::FillRM(WSA_Buff* buff) {
    return buff->SetModRM(-1, 3, Encode());
  }

  int Register::ModifyOpecode(WSA_Buff* buff) {
    return buff->AddToLastOpecodeByte(Encode());
  }

  Indirect::Indirect(string b, string i, int s, int32_t off, int w, int offw) {
    Type = OperandType::Idr;
    bWid = w;
    Register rbase(b);
    Register rindex(i);
    Base = rbase.GetRegType();
    Index = rindex.GetRegType();
    if (Base != RegType::None) {
      if (Index != RegType::None) {
        if (rbase.GetWid() != rindex.GetWid()) {
          cout << "wsa Indirect: base reg \"" + b + "\"'s width is differrent from index reg \"" + i + "\"" << endl;
          exit(-1);
          Type = OperandType::None;
        } else {
          AddrWid = rbase.GetWid();
        }
      } else {
        AddrWid = rbase.GetWid();
      }
    } else if (Index != RegType::None) {
      AddrWid = rindex.GetWid();
    } else {
      AddrWid = 4;
    }
    Scale = s;
    Offset = off;
    OffWid = offw;
  }


  Indirect::Indirect(RegType b, RegType i, int s, int32_t off, int w, int offw) {
    Type = OperandType::Idr;
    bWid = w;

    Base = b;
    Index = i;
    Scale = s;
    Offset = off;
    AddrWid = 4;

    OffWid = offw;
  }

  int Indirect::EncodeScale() {
    switch(Scale) {
      case 1:
        return 0;
      case 2:
        return 1;
      case 4:
        return 2;
      case 8:
        return 3;
      default:
        return -1;
    }
  }

  int Indirect::FillRM32(WSA_Buff* buff) {

    if (Base == RegType::None && Index == RegType::None) {
      buff->SetModRM(-1, 0, 5);
      buff->SetOffset(Offset, 4);
      return 0;
    }

    int bOff = EnoughBytesSigned(Offset);
    if (OffWid) bOff = OffWid;
    int mod;
    switch (bOff) {
      case 0:
        mod = 0;
        break;
      case 1:
        mod = 1;
        break;
      default:
        mod = 2;
        bOff = 4;
        break;
    }

    if (Index != RegType::None) {
      int s = EncodeScale(), i = EncodeRegType(Index), b;

      if (Base == RegType::BP && bOff == 0) {
        mod = 1;
        bOff = 1;
      }

      if (Index == RegType::SP) {
        return -1;
      }

      if (Base == RegType::None) {
        mod = 0;
        b = 5;
        bOff = 4;
      } else {
        b = EncodeRegType(Base);
      }

      buff->SetModRM(-1, mod, 4);
      buff->SetSIB(s, i, b);
      buff->SetOffset(Offset, bOff);
      return 0;
    } else {
      if (Base == RegType::SP) {
        buff->SetModRM(-1, mod, 4);
        buff->SetSIB(4, 0, 4);
        buff->SetOffset(Offset, bOff);
        return 0;
      }

      if (Base == RegType::BP && bOff == 0) {
        mod = 1;
        bOff = 1;
      }

      buff->SetModRM(-1, mod, EncodeRegType(Base));
      buff->SetOffset(Offset, bOff);
      return 0;
    }
  }

  int Indirect::FillRM16(WSA_Buff* buff) {
    buff->Enable16();

    int bOff = EnoughBytesSigned(Offset);
    if (OffWid) bOff = OffWid;
    int mod;
    switch (bOff) {
      case 0:
        mod = 0;
        break;
      case 1:
        mod = 1;
        break;
      case 2:
        mod = 2;
        break;
      default:
        return -1;
    }

    if (Base != RegType::None && Index != RegType::None) {
      if (Base == RegType::B) {
        if (Index == RegType::SI) {
          buff->SetModRM(-1, mod, 0);
        } else if (Index == RegType::DI) {
          buff->SetModRM(-1, mod, 1);
        } else {
          return -1;
        }
      } else if (Base == RegType::BP) {
        if (Index == RegType::SI) {
          buff->SetModRM(-1, mod, 2);
        } else if (Index == RegType::DI) {
          buff->SetModRM(-1, mod, 3);
        } else {
          return -1;
        }
      } else {
        return -1;
      }

      buff->SetOffset(Offset, bOff);
      return 0;
    }

    if (Base == RegType::None && Index != RegType::None) {
      Base = Index;
    }

    if (Base != RegType::None) {
      switch (Base) {
        case RegType::SI:
          buff->SetModRM(-1, mod, 4);
          break;
        case RegType::DI:
          buff->SetModRM(-1, mod, 5);
          break;
        case RegType::BP:
          if (bOff == 0) {
            bOff = 1;
            mod = 1;
          }
          buff->SetModRM(-1, mod, 6);
          break;
        case RegType::B:
          buff->SetModRM(-1, mod, 7);
          break;
        default:
          return -1;
      }
      buff->SetOffset(Offset, bOff);
      return 0;
    } else {
      return -1;
    }
  }

  int Indirect::FillRM(WSA_Buff* buff) {
    if (AddrWid == 4) {
      return FillRM32(buff);
    } else if (AddrWid == 2) {
      return FillRM16(buff);
    } else {
      return -1;
    }
  }

  Opecode::Opecode(vector<uint8_t> code) {
    Code = code;
    ExOpc = 0;
    sx = 0;
  }

  Opecode::Opecode(vector<uint8_t> code, pair<vector<int>, vector<int>> size, pair<string, string> sizespec, vector<ObjectType> objtypes, int exopc, vector<RegType> spreg) {
    Code = code;
    if (exopc >= 0) {
      ExOpc = exopc;
    } else {
      ExOpc = 0;
    }

    if (size.second.size()) {
      if (size.second.size() != size.first.size()) {
        cout << "Opecode Constructer: Number of operands is different from each other." << endl;
        exit(-1);
      }
    } else {
      if (sizespec.second.size()) {
        exit(-1);
      }
    }

    OprdSize = size;

    if (objtypes.size() != size.first.size()) {
      cout << "Opecode Constructer: Number of operands is different from each other." << endl;
      exit(-1);
    }

    ObjectTypes = objtypes;
    SizeSpec = sizespec;

    if (spreg.size() > size.first.size()) {
      exit(-1);
    }

    SpecifiedRegisters = spreg;

    sx = 0;
  }

  vector<uint8_t> Opecode::GetCode() {
    return Code;
  }

  int Opecode::SetSX() {
    sx = 1;
    return 0;
  }

  int Opecode::CheckOperand(vector<variant<Immediate, Register, Indirect>> oprd, string opsz) {
    if (oprd.size() != OprdSize.first.size()) {
      return 0;
    }

    if (oprd.size() == 0) return 1;

    int nsp = 0;

    for (int i = 0; i < oprd.size(); i++) {
      int t = oprd[i].index();

      switch (ObjectTypes[i]) {
        case ObjectType::OpcReg:
          if (t != 1) return 0;
          break;
        case ObjectType::Reg:
          if (t != 1) return 0;
          break;
        case ObjectType::RM:
          if (t != 1 && t != 2) return 0;
          break;
        case ObjectType::RMRegOnly:
          if (t != 1) return 0;
          break;
        case ObjectType::RMMemOnly:
          if (t != 2) return 0;
          break;
        case ObjectType::RegAndRM:
          if (t != 1) return 0;
          break;
        case ObjectType::Imm:
          if (t != 0) return 0;
          break;
        case ObjectType::SpecReg:
          if (t != 1) return  0;
          if (nsp < i) return 0;
          if (SpecifiedRegisters[nsp] != get<Register>(oprd[i]).GetRegType()) return 1;
          nsp++;
          break;
        default:
          return 0;
      }
    }

    int def = 0, alt = 0;
    if (opsz.size()) {
      if (opsz == SizeSpec.first) {
        def = 1;
      } else if (opsz == SizeSpec.second) {
        alt = 1;
      } else {
        return 0;
      }
    } else {
      def = 1;
      if (OprdSize.second.size()) alt = 1;
    }

    for (int i = 0; i < oprd.size(); i++) {
      int w;

      Immediate imm(0, 0);
      Register reg(RegType::None, 0);
      Indirect idr(RegType::None, RegType::None);

      switch (oprd[i].index()) {
        case 0:
          imm = get<Immediate>(oprd[i]);
          if (imm.SizeSpecified()) {
            w = imm.GetWid();
            if (def && OprdSize.first[i] != w) def = 0;
            if (alt && OprdSize.second[i] != w) alt = 0;
          } else {
            if (imm.IsSigned() == sx) {
              w = imm.GetWid();
            } else if (imm.IsSigned()) {
              w = EnoughBytesUnSigned((uint32_t)imm.GetData());
            } else {
              w = EnoughBytesSigned((uint32_t)imm.GetData());
            }

            if (def && OprdSize.first[i] < w) def = 0;
            if (alt && OprdSize.second[i] < w) alt = 0;
          }
          break;
        case 1:
          reg = get<Register>(oprd[i]);
          w = reg.GetWid();
          if (def && OprdSize.first[i] != w) def = 0;
          if (alt && OprdSize.second[i] != w) alt = 0;
          break;
        case 2:
          idr = get<Indirect>(oprd[i]);
          w = idr.GetWid();
          if (w && def && OprdSize.first[i] != w) def = 0;
          if (w && alt && OprdSize.second[i] != w) alt = 0;
          break;
      }
      if (!(def || alt)) return  0;
    }

    if (def) return 1;
    if (alt) return 2;
    return  0;
  }

  int Opecode::Encode(WSA_Buff* buff, vector<variant<Immediate, Register, Indirect>> oprd, string opsz) {
    int size = CheckOperand(oprd, opsz);
    if (!size) return -1;
    if (size == 2) buff->EnAltOprdSize();

    buff->SetOpecode(Code);

    int r = 0;

    for (int i = 0; i < oprd.size(); i++) {
      Immediate imm(0, 0);
      Register reg(RegType::None, 0);
      Indirect idr(RegType::None, RegType::None);

      switch (oprd[i].index()) {
        case 0:
          imm = get<Immediate>(oprd[i]);
          break;
        case 1:
          reg = get<Register>(oprd[i]);
          reg.SetWid(size == 2 ? OprdSize.second[i] : OprdSize.first[i]);
          break;
        case 2:
          idr = get<Indirect>(oprd[i]);
          idr.SetWid(size == 2 ? OprdSize.second[i] : OprdSize.first[i]);
          break;
      }

      switch (ObjectTypes[i]) {
        case ObjectType::OpcReg:
          reg.ModifyOpecode(buff);
          break;

        case ObjectType::Reg:
          reg.FillRegField(buff);
          r = 2;
          break;

        case ObjectType::RM:
          if (oprd[i].index() == 1) {
            reg.FillRM(buff);
          } else {
            idr.FillRM(buff);
          }
          if (r != 2) r = 1;
          break;

        case ObjectType::RMRegOnly:
          reg.FillRM(buff);
          if (r != 2) r = 1;
          break;

        case ObjectType::RMMemOnly:
          idr.FillRM(buff);
          if (r != 2) r = 1;
          break;

        case ObjectType::RegAndRM:
          reg.FillRegField(buff);
          reg.FillRM(buff);
          r = 2;
          break;

        case ObjectType::Imm:
          imm.Fill(buff, size == 2 ? OprdSize.second[i] : OprdSize.first[i]);
          break;
      }
    }

    if (r == 1) buff->SetModRM(ExOpc);
    return 0;
  }

  Instruction::Instruction(string name) {
    Name = name;
  }

  Instruction::~Instruction() {
    for (int i = 0; i < Opecodes.size(); i++) {
      delete Opecodes[i];
    }
  }

  string Instruction::GetName() {
    return Name;
  }

  Opecode* Instruction::AddOpecode(vector<uint8_t> code, pair<vector<int>, vector<int>> size, pair<string, string> sizespec, vector<ObjectType> objtypes, int exopc, vector<RegType> spreg) {
    Opecodes.push_back(new Opecode(code, size, sizespec, objtypes, exopc, spreg));
    return Opecodes.back();
  }

  int Instruction::Encode(WSA_Buff* buff, vector<variant<Immediate, Register, Indirect>> oprd, string opsz) {
    int ret = -1;

    for (int i = 0; i < Opecodes.size(); i++) {
      if (Opecodes[i]->CheckOperand(oprd, opsz)) {
        return Opecodes[i]->Encode(buff, oprd, opsz);
      }
    }

    return ret;
  }

  int InstructionConditional::GetCCNum(string instname) {
    if (instname.size() <= Name.size()) return -1;
    if (instname.substr(0, Name.size()) != Name) return -1;
    return EncodeCC(instname.substr(Name.size()));
  }

  int InstructionConditional::Encode(WSA_Buff* buff, int ncc, vector<variant<Immediate, Register, Indirect>> oprd, string opsz) {
    int ret = Instruction::Encode(buff, oprd, opsz);
    if (!ret) buff->AddToLastOpecodeByte(ncc);
    return ret;
  }

  int InstructionConditional::Encode(WSA_Buff* buff, string instname, vector<variant<Immediate, Register, Indirect>> oprd, string opsz) {
    int ret = Instruction::Encode(buff, oprd, opsz);
    if (!ret) buff->AddToLastOpecodeByte(GetCCNum(instname));
    return ret;
  }

  Instruction* InstructionSet::AddInstruction(string name) {
    Instructions.push_back(new Instruction(name));
    Instruction* ret = Instructions.back();
    Table.Get(name) = ret;
    return ret;
  }

  InstructionConditional* InstructionSet::AddInstructionConditional(string name) {
    InstructionsConditional.push_back(new InstructionConditional(name));
    return InstructionsConditional.back();
  }

  int InstructionSet::Encode(WSA_Buff* buff, string inst, vector<variant<Immediate, Register, Indirect>> oprd, string opsz) {
    Instruction** ptr = Table.Find(inst);
    if (ptr) {
      return (*ptr)->Encode(buff, oprd, opsz);
    } else {
      for (int i = 0; i < InstructionsConditional.size(); i++) {
        int n = InstructionsConditional[i]->GetCCNum(inst);
        if (n >= 0) return InstructionsConditional[i]->Encode(buff, n, oprd, opsz);
      }
    }
    return -1;
  }

  InstructionSet::InstructionSet() {
    vector<string> instnames = {"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"};

    for (int i = 0; i < instnames.size(); i++) {
      Instruction* inst = AddInstruction(instnames[i]);

      inst->AddOpecode({(uint8_t)(i * 8)}, {{1, 1}, {}}, {"b", ""}, {ObjectType::Reg, ObjectType::RM});
      inst->AddOpecode({(uint8_t)(i * 8 + 1)}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::Reg, ObjectType::RM});
      inst->AddOpecode({(uint8_t)(i * 8 + 2)}, {{1, 1}, {}}, {"b", ""}, {ObjectType::RM, ObjectType::Reg});
      inst->AddOpecode({(uint8_t)(i * 8 + 3)}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::RM, ObjectType::Reg});

      inst->AddOpecode({0x83}, {{4, 1}, {2, 1}}, {"l", "w"}, {ObjectType::Imm, ObjectType::RM}, i)->SetSX();

      inst->AddOpecode({(uint8_t)(i * 8 + 4)}, {{1, 1}, {}}, {"b", ""}, {ObjectType::Imm, ObjectType::SpecReg});
      inst->AddOpecode({(uint8_t)(i * 8 + 5)}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::Imm, ObjectType::SpecReg});

      inst->AddOpecode({0x80}, {{1, 1}, {}}, {"b", ""}, {ObjectType::Imm, ObjectType::RM}, i);
      inst->AddOpecode({0x81}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::Imm, ObjectType::RM}, i);
    }

    Instruction* inst = AddInstruction("inc");

    inst->AddOpecode({0x40}, {{4}, {2}}, {"l", "w"}, {ObjectType::OpcReg});
    inst->AddOpecode({0xfe}, {{1}, {}}, {"b", ""}, {ObjectType::RM}, 0);
    inst->AddOpecode({0xfe}, {{4}, {2}}, {"l", "w"}, {ObjectType::RM}, 0);

    inst = AddInstruction("dec");

    inst->AddOpecode({0x48}, {{4}, {2}}, {"l", "w"}, {ObjectType::OpcReg});
    inst->AddOpecode({0xfe}, {{1}, {}}, {"b", ""}, {ObjectType::RM}, 1);
    inst->AddOpecode({0xfe}, {{4}, {2}}, {"l", "w"}, {ObjectType::RM}, 1);

    inst = AddInstruction("push");

    inst->AddOpecode({0x50}, {{4}, {2}}, {"l", "w"}, {ObjectType::OpcReg});

    inst->AddOpecode({0xff}, {{4}, {2}}, {"l", "w"}, {ObjectType::RM}, 6);
    inst->AddOpecode({0x6a}, {{1}, {}}, {"w", ""}, {ObjectType::Imm});//
    inst->AddOpecode({0x68}, {{4}, {2}}, {"l", "w"}, {ObjectType::Imm});

    inst = AddInstruction("pop");

    inst->AddOpecode({0x58}, {{4}, {2}}, {"l", "w"}, {ObjectType::OpcReg});
    inst->AddOpecode({0x8f}, {{4}, {2}}, {"l", "w"}, {ObjectType::RM}, 0);

    inst = AddInstruction("imul");

    inst->AddOpecode({0x69}, {{4, 4, 4}, {2, 2, 2}}, {"l", "w"}, {ObjectType::Imm, ObjectType::RM, ObjectType::Reg});
    inst->AddOpecode({0x69}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::Imm, ObjectType::RegAndRM});

    inst->AddOpecode({0x6b}, {{4, 4, 1}, {2, 2, 1}}, {"l", "w"}, {ObjectType::Imm, ObjectType::RM, ObjectType::Reg})->SetSX();
    inst->AddOpecode({0x6b}, {{4, 1}, {2, 1}}, {"l", "w"}, {ObjectType::Imm, ObjectType::RegAndRM})->SetSX();

    inst->AddOpecode({0xf6}, {{1, 1}, {}}, {"b", ""}, {ObjectType::RM, ObjectType::SpecReg}, 5, {RegType::A});
    inst->AddOpecode({0xf7}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::RM, ObjectType::SpecReg}, 5, {RegType::A});

    inst->AddOpecode({0x0f, 0xaf}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::RM, ObjectType::Reg});

    inst = AddInstructionConditional("j");
    inst->AddOpecode({0x70}, {{1}, {}}, {"", ""}, {ObjectType::Imm})->SetSX();
    inst->AddOpecode({0x0f, 0x80}, {{4}, {2}}, {"", ""}, {ObjectType::Imm})->SetSX();

    inst = AddInstruction("test");

    inst->AddOpecode({0x84}, {{1, 1}, {}}, {"b", ""}, {ObjectType::Reg, ObjectType::RM});
    inst->AddOpecode({0x84}, {{1, 1}, {}}, {"b", ""}, {ObjectType::RM, ObjectType::Reg});

    inst->AddOpecode({0x85}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::Reg, ObjectType::RM});
    inst->AddOpecode({0x85}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::RM, ObjectType::Reg});

    inst->AddOpecode({0xa8}, {{1, 1}, {}}, {"b", ""}, {ObjectType::SpecReg, ObjectType::Imm}, -1, {RegType::A});
    inst->AddOpecode({0xa8}, {{1, 1}, {}}, {"b", ""}, {ObjectType::Imm, ObjectType::SpecReg}, -1, {RegType::A});

    inst->AddOpecode({0xa9}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::SpecReg, ObjectType::Imm}, -1, {RegType::A});
    inst->AddOpecode({0xa9}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::Imm, ObjectType::SpecReg}, -1, {RegType::A});

    inst->AddOpecode({0xf6}, {{1, 1}, {}}, {"b", ""}, {ObjectType::Imm, ObjectType::RM}, 0);
    inst->AddOpecode({0xf6}, {{1, 1}, {}}, {"b", ""}, {ObjectType::RM, ObjectType::Imm}, 0);

    inst->AddOpecode({0xf7}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::Imm, ObjectType::RM}, 0);
    inst->AddOpecode({0xf7}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::RM, ObjectType::Imm}, 0);

    inst = AddInstruction("xchg");

    inst->AddOpecode({0x90}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::OpcReg, ObjectType::SpecReg}, -1, {RegType::A});
    inst->AddOpecode({0x90}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::SpecReg, ObjectType::OpcReg}, -1, {RegType::A});

    inst->AddOpecode({0x86}, {{1, 1}, {}}, {"b", ""}, {ObjectType::Reg, ObjectType::RM});
    inst->AddOpecode({0x86}, {{1, 1}, {}}, {"b", ""}, {ObjectType::RM, ObjectType::Reg});

    inst->AddOpecode({0x87}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::Reg, ObjectType::RM});
    inst->AddOpecode({0x87}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::RM, ObjectType::Reg});

    inst = AddInstruction("mov");

    inst->AddOpecode({0x88}, {{1, 1}, {}}, {"b", ""}, {ObjectType::Reg, ObjectType::RM});
    inst->AddOpecode({0x89}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::Reg, ObjectType::RM});

    inst->AddOpecode({0x8a}, {{1, 1}, {}}, {"b", ""}, {ObjectType::RM, ObjectType::Reg});
    inst->AddOpecode({0x8b}, {{4, 4}, {2, 2}}, {"l", ""}, {ObjectType::RM, ObjectType::Reg});

    inst->AddOpecode({0xb0}, {{1, 1}, {}}, {"b", ""}, {ObjectType::Imm, ObjectType::OpcReg});
    inst->AddOpecode({0xb8}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::Imm, ObjectType::OpcReg});

    inst->AddOpecode({0xc6}, {{1, 1}, {}}, {"b", ""}, {ObjectType::Imm, ObjectType::RM}, 0);
    inst->AddOpecode({0xc7}, {{4, 4}, {2, 2}}, {"l", "w"}, {ObjectType::Imm, ObjectType::RM}, 0);

    inst = AddInstruction("lea");
    inst->AddOpecode({0x8d}, {{4, 0}, {2, 0}}, {"l", "w"}, {ObjectType::RM, ObjectType::Reg});

    inst = AddInstruction("nop");
    inst->AddOpecode({0x90}, {{}, {}}, {"", ""}, {});

    inst = AddInstruction("ret");
    inst->AddOpecode({0xc3}, {{}, {}}, {"", ""}, {});

    inst = AddInstruction("leave");
    inst->AddOpecode({0xc9}, {{}, {}}, {"", ""}, {});

    inst = AddInstruction("call");
    inst->AddOpecode({0xe8}, {{4}, {2}}, {"", ""}, {ObjectType::Imm})->SetSX();

    inst = AddInstruction("jmp");
    inst->AddOpecode({0xeb}, {{1}, {}}, {"", ""}, {ObjectType::Imm})->SetSX();
    inst->AddOpecode({0xe9}, {{4}, {2}}, {"", ""}, {ObjectType::Imm})->SetSX();

    inst = AddInstruction("div");
    inst->AddOpecode({0xf6}, {{1, 2}, {}}, {"b", ""}, {ObjectType::RM, ObjectType::SpecReg}, 6, {RegType::A});
    inst->AddOpecode({0xf6}, {{1}, {}}, {"b", ""}, {ObjectType::RM}, 6);
    inst->AddOpecode({0xf7}, {{4}, {2}}, {"b", ""}, {ObjectType::RM}, 6);

    inst = AddInstructionConditional("set");
    inst->AddOpecode({0x0f, 0x90}, {{1}, {}}, {"", ""}, {ObjectType::RM}, 0);

    instnames = {"sal", "sar", "shl", "shr"};
    vector<int> exopcs = {4, 7, 4, 5};

    for (int i = 0; i < instnames.size(); i++) {
      inst = AddInstruction(instnames[i]);
      inst->AddOpecode({0xd0}, {{1}, {}}, {"b", ""}, {ObjectType::RM}, exopcs[i]);
      inst->AddOpecode({0xd1}, {{4}, {2}}, {"l", "w"}, {ObjectType::RM}, exopcs[i]);
      inst->AddOpecode({0xd2}, {{1, 1}, {}}, {"b", ""}, {ObjectType::RM, ObjectType::SpecReg}, exopcs[i], {RegType::C});
      inst->AddOpecode({0xd3}, {{4, 1}, {2, 1}}, {"l", "w"}, {ObjectType::RM, ObjectType::SpecReg}, exopcs[i], {RegType::C});
      inst->AddOpecode({0xc0}, {{1, 1}, {}}, {"b", ""}, {ObjectType::Imm, ObjectType::RM}, exopcs[i]);
      inst->AddOpecode({0xc1}, {{4, 1}, {2, 1}}, {"l", "w"}, {ObjectType::Imm, ObjectType::RM}, exopcs[i]);
    }

    inst = AddInstruction("movzx");
    inst->AddOpecode({0x0f, 0xb6}, {{4, 1}, {2, 1}}, {"", ""}, {ObjectType::RM, ObjectType::Reg});
    inst->AddOpecode({0x0f, 0xb7}, {{4, 2}, {}}, {"", ""}, {ObjectType::RM, ObjectType::Reg});

    inst = AddInstruction("movsx");
    inst->AddOpecode({0x0f, 0xbe}, {{4, 1}, {2, 1}}, {"", ""}, {ObjectType::RM, ObjectType::Reg});
    inst->AddOpecode({0x0f, 0xbf}, {{4, 2}, {}}, {"", ""}, {ObjectType::RM, ObjectType::Reg});


  }

  InstructionSet::~InstructionSet() {
    for (int i = 0; i < Instructions.size(); i++) {
      delete Instructions[i];
    }
  }

  int EnoughBytesSigned(int32_t n) {
    if (n >= 0) {
      if (n == 0) return 0;
      if (n <= 0x7f) return 1;
      if (n <= 0x7fff) return 2;
      if (n <= 0x7fffffff) return 4;
      return -1;
    } else {
      if (n >= -0x80) return 1;
      if (n >= -0x8000) return 2;
      if (n >= -0x80000000) return 4;
      return -1;
    }
  }

  int EnoughBytesUnSigned(uint32_t n) {
      if (n == 0) return 0;
      if (n <= 0xff) return 1;
      if (n <= 0xffff) return 2;
      return 4;
  }

  vector<vector<string>>ccstring = {
    {"o"},
    {"no"},
    {"b", "nae", "c"},
    {"nb", "ae", "nc"},
    {"z", "e"},
    {"nz", "ne"},
    {"be", "na"},
    {"nbe", "a"},

    {"s"},
    {"ns"},
    {"p", "pe"},
    {"np", "po"},
    {"l", "nge"},
    {"nl", "ge"},
    {"le", "ng"},
    {"nle", "g"}
  };

  RegType GetRegType(string str) {
    if (str[0] == 'e') {
      str = str.substr(1);
    }
    if (str.size() != 2) {
      return RegType::None;
    } else if (str == "sp") {
      return RegType::SP;
    } else if (str == "bp") {
      return RegType::BP;
    } else if (str == "si") {
      return RegType::SI;
    } else if (str == "di") {
      return RegType::DI;
    } else if ('a' <= str[0] && str[0] <= 'd') {
      RegType r = RegType::None;
      switch(str[0]) {
        case 'a':
          r = RegType::A;
          break;
        case 'c':
          r = RegType::C;
          break;
        case 'd':
          r = RegType::D;
          break;
        case 'b':
          r = RegType::B;
          break;
      }
      switch (str[1]) {
        case 'x':
          break;

        case 'h':
          break;

        case 'l':
          break;

        default:
          r = RegType::None;
          break;
      }
      return r;
    } else {
      return RegType::None;
    }
  }

  int EncodeCC(string cc) {
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < ccstring[i].size(); j++) {
        if (ccstring[i][j] == cc) return i;
      }
    }
    return -1;
  }

  int EncodeRegType(RegType type) {
    switch(type) {
      case RegType::A: return 0;
      case RegType::C: return 1;
      case RegType::D: return 2;
      case RegType::B: return 3;
      case RegType::SP: return 4;
      case RegType::BP: return 5;
      case RegType::SI: return 6;
      case RegType::DI: return 7;
      default: return -1;
    }
  }

  int EncodeScale(int i) {
    switch (i) {
      case 1:
        return 0;
      case 2:
        return 1;
      case 4:
        return 2;
      case 8:
        return 3;
      default:
        return -1;
    }
  }
}

#endif
