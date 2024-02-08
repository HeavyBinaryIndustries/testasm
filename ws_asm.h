#ifndef WSA_H
#define WSA_H

#include <stdint.h>
#include <string>
#include <array>
#include <vector>
#include <variant>
#include "hashtable.cpp"

using namespace std;

namespace wsa {
  enum class OperandType {
    None,
    Imm,
    Reg,
    Idr
  };

  enum class ObjectType {
    OpcReg,
    Reg,
    RM,
    RMRegOnly,
    RMMemOnly,
    RegAndRM,
    Imm,
    SpecReg
  };

  enum class RegType {
    None,
    A,
    C,
    D,
    B,
    SP,
    BP,
    SI,
    DI
  };

  class WSA_Buff {
  private:
    uint8_t* Buff;

    uint8_t* Current;

    int EnModRM, EnSIB;
    int Addr16, AltOprdSize;

    vector<uint8_t> Opecode;

    struct {
      int Mod;
      int Reg;
      int RM;
    } ModRM;

    struct {
      int Scale;
      int Index;
      int Base;
    } SIB;

    int32_t Offset;

    int nOffsetBytes;

    uint32_t ImmediateData;

    int nImmediateBytes;

    int delbuff;

  public:
    WSA_Buff();
    WSA_Buff(int size);
    WSA_Buff(uint8_t* buff);
    ~WSA_Buff();

    int Reset();

    int SetOpecode(vector<uint8_t> opc);
    int SetModRM(int reg = -1, int mod = -1, int rm = -1);
    int SetSIB(int s, int i, int b);
    int SetOffset(int32_t off, int nb);
    int SetImmData(uint32_t data, int w);
    int Enable16();
    int EnAltOprdSize();
    uint8_t* GetCurrent();

    void* PrevOff;
    void* PrevImm;

    int Print();

    int AddToLastOpecodeByte(int i);

    int Commit();
    int SetBuff(uint8_t* buff);

  };

  class Operand {
  protected:
    OperandType Type;
    int bWid;

  public:
    virtual ~Operand() {};

    int SetType(OperandType type);
    OperandType GetType();
    int SetWid(int w);

    virtual int GetWid();
    virtual uint32_t GetData() {return 0;};

    virtual int Fill(WSA_Buff* buff, int nb) {return -1;};
    virtual int FillRegField(WSA_Buff* buff) {return -1;};
    virtual int FillRM(WSA_Buff* buff) {return -1;};
    virtual int ModifyOpecode(WSA_Buff* buff) {return -1;};

    virtual int IsSigned() {return 0;};
  };

  class Immediate : public Operand {
  private:
    uint32_t Data;
    int Signed;

  public:
    Immediate(uint32_t data, int sign = 0, int w = 0);

    int SizeSpecified();
    int GetWid();
    uint32_t GetData();
    int IsSigned();

    int Fill(WSA_Buff* buff, int nb);
  };

  class Register : public Operand {
  private:
    RegType RType;
    int HighByte;

    int Encode();

  public:
    Register(string str);
    Register(RegType type, int w = 4, int high = 0);
    ~Register() {};

    RegType GetRegType();
    int IsHighByte();

    int FillRegField(WSA_Buff* buff);
    int FillRM(WSA_Buff* buff);
    int ModifyOpecode(WSA_Buff* buff);
  };

  class Indirect : public Operand {
  private:
    int AddrWid;
    RegType Base, Index;
    int Scale;
    int32_t Offset;

    int OffWid;

    int EncodeScale();

    int FillRM32(WSA_Buff* buff);
    int FillRM16(WSA_Buff* buff);


  public:
    Indirect(RegType b, RegType i, int s = 4, int32_t off = 0, int w = 4, int offw = 0);
    Indirect(string b, string i, int s = 4, int32_t off = 0, int w = 4, int offw = 0);
    ~Indirect() {};

    int FillRM(WSA_Buff* buff);
  };

  class Opecode {
  protected:
    vector<uint8_t> Code;
    int ExOpc;

    int sx;

    pair<vector<int>, vector<int>> OprdSize;

    pair<string, string> SizeSpec;

    vector<ObjectType> ObjectTypes;

    vector<RegType> SpecifiedRegisters;

    vector<Operand*> SwapOperand(vector<Operand*>);

  public:
    Opecode(vector<uint8_t> code, pair<vector<int>, vector<int>> size, pair<string, string> sizespec, vector<ObjectType> objtypes, int exopc = -1, vector<RegType> spreg = {});
    Opecode(vector<uint8_t> code);
    virtual ~Opecode() {};

    int AddSwap(int from, int to);

    vector<Operand*> ApplyOperandSize(string opsz, vector<Operand*> oprd);

    vector<uint8_t> GetCode();

    int SetSX();

    virtual int CheckOperand(vector<variant<Immediate, Register, Indirect>> oprd, string opsz = "");
    virtual int Encode(WSA_Buff* buff, vector<variant<Immediate, Register, Indirect>> oprd, string opsz = "");
  };

  class Instruction {
  protected:
    string Name;
    vector<Opecode*> Opecodes;

  public:
    Instruction(string name);
    virtual ~Instruction();

    string GetName();

    Opecode* AddOpecode(vector<uint8_t> code, pair<vector<int>, vector<int>> size, pair<string, string> sizespec, vector<ObjectType> objtypes, int exopc = -1, vector<RegType> spreg = {});

    int Encode(WSA_Buff* buff, vector<variant<Immediate, Register, Indirect>> oprd, string opsz = "");
  };

  class InstructionConditional : public Instruction {
  private:
  public:
    InstructionConditional(string name) :Instruction(name) {};

    int GetCCNum(string instname);

    int Encode(WSA_Buff* buff, int ncc, vector<variant<Immediate, Register, Indirect>> oprd, string opsz = "");
    int Encode(WSA_Buff* buff, string instname, vector<variant<Immediate, Register, Indirect>> oprd, string opsz = "");
  };

  class InstructionSet {
  private:
    vector<Instruction*> Instructions;
    vector<InstructionConditional*> InstructionsConditional;

    HashTable<Instruction*> Table;

  public:
    InstructionSet();
    ~InstructionSet();

    Instruction* AddInstruction(string name);
    InstructionConditional* AddInstructionConditional(string name);

    int Encode(WSA_Buff* buff, string inst, vector<variant<Immediate, Register, Indirect>> oprd, string opsz = "");
  };

  RegType GetRegType(string str);
  int EncodeCC(string cc);
  int EncodeRegType(RegType reg);
  int EnoughBytesSigned(int32_t n);
  int EnoughBytesUnSigned(uint32_t n);

}

#define PREF_OPRD_SIZE 0x66
#define PREF_ADDR_SIZE 0x67

#endif
