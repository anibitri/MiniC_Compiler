#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <string.h>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <unistd.h>

using namespace llvm;
using namespace llvm::sys;

FILE *pFile;



//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns one of these for known things.
enum TOKEN_TYPE {

  IDENT = -1,        // [a-zA-Z_][a-zA-Z_0-9]*
  ASSIGN = int('='), // '='

  // delimiters
  LBRA = int('{'),  // left brace
  RBRA = int('}'),  // right brace
  LPAR = int('('),  // left parenthesis
  LBOX = int('['),  // left bracket
  RBOX = int(']'),  // right bracket
  RPAR = int(')'),  // right parenthesis
  SC = int(';'),    // semicolon
  COMMA = int(','), // comma

  // types
  INT_TOK = -2,   // "int"
  VOID_TOK = -3,  // "void"
  FLOAT_TOK = -4, // "float"
  BOOL_TOK = -5,  // "bool"

  // keywords
  EXTERN = -6,  // "extern"
  IF = -7,      // "if"
  ELSE = -8,    // "else"
  WHILE = -9,   // "while"
  RETURN = -10, // "return"
  // TRUE   = -12,     // "true"
  // FALSE   = -13,     // "false"

  // literals
  INT_LIT = -14,   // [0-9]+
  FLOAT_LIT = -15, // [0-9]+.[0-9]+
  BOOL_LIT = -16,  // "true" or "false" key words

  // logical operators
  AND = -17, // "&&"
  OR = -18,  // "||"

  // operators
  PLUS = int('+'),    // addition or unary plus
  MINUS = int('-'),   // substraction or unary negative
  ASTERIX = int('*'), // multiplication
  DIV = int('/'),     // division
  MOD = int('%'),     // modular
  NOT = int('!'),     // unary negation

  // comparison operators
  EQ = -19,      // equal
  NE = -20,      // not equal
  LE = -21,      // less than or equal to
  LT = int('<'), // less than
  GE = -23,      // greater than or equal to
  GT = int('>'), // greater than

  // special tokens
  EOF_TOK = 0, // signal end of file

  // invalid
  INVALID = -100 // signal invalid token
};

namespace {
  inline bool use_color() {
    static int cached = -1;
    if (cached == -1) cached = isatty(1) ? 1 : 0;
    return cached;
  }

  inline std::string colorize(const char* code, const std::string& s) {
    if (!use_color()) return s;
    return std::string(code) + s + "\033[0m";
  }

  // Colors
  static constexpr const char* C_NODE  = "\033[1;36m"; // bright cyan
  static constexpr const char* C_NAME  = "\033[1;35m"; // bright magenta
  static constexpr const char* C_TYPE  = "\033[0;33m"; // yellow
  static constexpr const char* C_VALUE = "\033[1;37m"; // bright white

  inline std::string fmtNode(const std::string& kind) {
    return colorize(C_NODE, kind);
  }
  inline std::string fmtName(const std::string& name) {
    return colorize(C_NAME, "'" + name + "'");
  }
  inline std::string fmtType(const std::string& type) {
    return colorize(C_TYPE, "'" + type + "'");
  }
  inline std::string fmtValue(const std::string& val) {
    return colorize(C_VALUE, val);
  }

  // Prefix-format a multi-line child
  inline std::string formatChild(const std::string& s,
                                 const std::string& firstPrefix,
                                 const std::string& nextPrefix) {
    if (s.empty()) return firstPrefix + "<empty>";
    std::string out;
    size_t start = 0;
    bool first = true;
    while (true) {
      size_t nl = s.find('\n', start);
      std::string line = (nl == std::string::npos)
                           ? s.substr(start)
                           : s.substr(start, nl - start);
      out += (first ? firstPrefix : nextPrefix);
      out += line;
      if (nl == std::string::npos) break;
      out += "\n";
      start = nl + 1;
      first = false;
    }
    return out;
  }

  inline std::string opToString(int Op) {
    switch (Op) {
      case ASSIGN: return "=";
      case PLUS:   return "+";
      case MINUS:  return "-";
      case ASTERIX:return "*";
      case DIV:    return "/";
      case MOD:    return "%";
      case AND:    return "&&";
      case OR:     return "||";
      case EQ:     return "==";
      case NE:     return "!=";
      case LE:     return "<=";
      case LT:     return "<";
      case GE:     return ">=";
      case GT:     return ">";
      default:     return std::to_string(Op);
    }
  }
}

// TOKEN class is used to keep track of information about a token
class TOKEN {
public:
  TOKEN() = default;
  int type = -100;
  std::string lexeme;
  int lineNo;
  int columnNo;
  const std::string getIdentifierStr() const;
  const int getIntVal() const;
  const float getFloatVal() const;
  const bool getBoolVal() const;
};

static std::string globalLexeme;
static int lineNo, columnNo;

const std::string TOKEN::getIdentifierStr() const {
  if (type != IDENT) {
    fprintf(stderr, "%d:%d Error: %s\n", lineNo, columnNo,
            "getIdentifierStr called on non-IDENT token");
    exit(2);
  }
  return lexeme;
}

const int TOKEN::getIntVal() const {
  if (type != INT_LIT) {
    fprintf(stderr, "%d:%d Error: %s\n", lineNo, columnNo,
            "getIntVal called on non-INT_LIT token");
    exit(2);
  }
  return strtod(lexeme.c_str(), nullptr);
}

const float TOKEN::getFloatVal() const {
  if (type != FLOAT_LIT) {
    fprintf(stderr, "%d:%d Error: %s\n", lineNo, columnNo,
            "getFloatVal called on non-FLOAT_LIT token");
    exit(2);
  }
  return strtof(lexeme.c_str(), nullptr);
}

const bool TOKEN::getBoolVal() const {
  if (type != BOOL_LIT) {
    fprintf(stderr, "%d:%d Error: %s\n", lineNo, columnNo,
            "getBoolVal called on non-BOOL_LIT token");
    exit(2);
  }
  return (lexeme == "true");
}

static TOKEN returnTok(std::string lexVal, int tok_type) {
  TOKEN return_tok;
  return_tok.lexeme = lexVal;
  return_tok.type = tok_type;
  return_tok.lineNo = lineNo;
  return_tok.columnNo = columnNo - lexVal.length() - 1;
  return return_tok;
}

// Read file line by line -- or look for \n and if found add 1 to line number
// and reset column number to 0
/// gettok - Return the next token from standard input.
static TOKEN gettok() {

  static int LastChar = ' ';
  static int NextChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar)) {
    if (LastChar == '\n' || LastChar == '\r') {
      lineNo++;
      columnNo = 1;
    }
    LastChar = getc(pFile);
    columnNo++;
  }

  if (isalpha(LastChar) ||
      (LastChar == '_')) { // identifier: [a-zA-Z_][a-zA-Z_0-9]*
    globalLexeme = LastChar;
    columnNo++;

    while (isalnum((LastChar = getc(pFile))) || (LastChar == '_')) {
      globalLexeme += LastChar;
      columnNo++;
    }

    if (globalLexeme == "int")
      return returnTok("int", INT_TOK);
    if (globalLexeme == "bool")
      return returnTok("bool", BOOL_TOK);
    if (globalLexeme == "float")
      return returnTok("float", FLOAT_TOK);
    if (globalLexeme == "void")
      return returnTok("void", VOID_TOK);
    if (globalLexeme == "bool")
      return returnTok("bool", BOOL_TOK);
    if (globalLexeme == "extern")
      return returnTok("extern", EXTERN);
    if (globalLexeme == "if")
      return returnTok("if", IF);
    if (globalLexeme == "else")
      return returnTok("else", ELSE);
    if (globalLexeme == "while")
      return returnTok("while", WHILE);
    if (globalLexeme == "return")
      return returnTok("return", RETURN);
    if (globalLexeme == "true") {
      //   BoolVal = true;
      return returnTok("true", BOOL_LIT);
    }
    if (globalLexeme == "false") {
      //   BoolVal = false;
      return returnTok("false", BOOL_LIT);
    }
    return returnTok(globalLexeme.c_str(), IDENT);
  }

  if (LastChar == '=') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // EQ: ==
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("==", EQ);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("=", ASSIGN);
    }
  }

  if (LastChar == '{') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("{", LBRA);
  }
  if (LastChar == '}') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("}", RBRA);
  }
  if (LastChar == '(') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("(", LPAR);
  }
  if (LastChar == ')') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(")", RPAR);
  }
  if (LastChar == ';') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(";", SC);
  }
  if (LastChar == ',') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(",", COMMA);
  }

  if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9]+.
    std::string NumStr;

    if (LastChar == '.') { // Floatingpoint Number: .[0-9]+
      do {
        NumStr += LastChar;
        LastChar = getc(pFile);
        columnNo++;
      } while (isdigit(LastChar));

      //   FloatVal = strtof(NumStr.c_str(), nullptr);
      return returnTok(NumStr, FLOAT_LIT);
    } else {
      do { // Start of Number: [0-9]+
        NumStr += LastChar;
        LastChar = getc(pFile);
        columnNo++;
      } while (isdigit(LastChar));

      if (LastChar == '.') { // Floatingpoint Number: [0-9]+.[0-9]+)
        do {
          NumStr += LastChar;
          LastChar = getc(pFile);
          columnNo++;
        } while (isdigit(LastChar));

        // FloatVal = strtof(NumStr.c_str(), nullptr);
        return returnTok(NumStr, FLOAT_LIT);
      } else { // Integer : [0-9]+
        // IntVal = strtod(NumStr.c_str(), nullptr);
        return returnTok(NumStr, INT_LIT);
      }
    }
  }

  if (LastChar == '&') {
    NextChar = getc(pFile);
    if (NextChar == '&') { // AND: &&
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("&&", AND);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("&", int('&'));
    }
  }

  if (LastChar == '|') {
    NextChar = getc(pFile);
    if (NextChar == '|') { // OR: ||
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("||", OR);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("|", int('|'));
    }
  }

  if (LastChar == '!') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // NE: !=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("!=", NE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("!", NOT);
      ;
    }
  }

  if (LastChar == '<') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // LE: <=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("<=", LE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("<", LT);
    }
  }

  if (LastChar == '>') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // GE: >=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok(">=", GE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok(">", GT);
    }
  }

  if (LastChar == '/') { // could be division or could be the start of a comment
    LastChar = getc(pFile);
    columnNo++;
    if (LastChar == '/') { // definitely a comment
      do {
        LastChar = getc(pFile);
        columnNo++;
      } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

      if (LastChar != EOF)
        return gettok();
    } else
      return returnTok("/", DIV);
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF) {
    columnNo++;
    return returnTok("0", EOF_TOK);
  }

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  std::string s(1, ThisChar);
  LastChar = getc(pFile);
  columnNo++;
  return returnTok(s, int(ThisChar));
}

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static TOKEN CurTok;
static std::deque<TOKEN> tok_buffer;

static TOKEN getNextToken() {

  if (tok_buffer.size() == 0)
    tok_buffer.push_back(gettok());

  TOKEN temp = tok_buffer.front();
  tok_buffer.pop_front();

  return CurTok = temp;
}

static void putBackToken(TOKEN tok) { tok_buffer.push_front(tok); }

/// ASTnode - Base class for all AST nodes.
class ASTnode {

public:
  virtual ~ASTnode() {}
  virtual Value *codegen() { return nullptr; };
  virtual std::string to_string() const { return ""; };
};

/// IntASTnode - Class for integer literals like 1, 2, 10,
class IntASTnode : public ASTnode {
  int Val;
  TOKEN Tok;
  std::string Name;

public:
  IntASTnode(TOKEN tok, int val) : Val(val), Tok(tok) {}
  const std::string &getType() const { return Tok.lexeme; }

  virtual std::string to_string() const override {
    return fmtNode("IntAST") + " " + fmtValue(std::to_string(Val));
  }

  virtual Value *codegen() override;
};

/// BoolASTnode - Class for boolean literals true and false,
class BoolASTnode : public ASTnode {
  bool Bool;
  TOKEN Tok;

public:
  BoolASTnode(TOKEN tok, bool B) : Bool(B), Tok(tok) {}
  const std::string &getType() const { return Tok.lexeme; }

  virtual std::string to_string() const override {
    return fmtNode("BoolAST") + " " + fmtValue(Bool ? "true" : "false");
  }

  virtual Value *codegen() override;
};

/// FloatASTnode - Node class for floating point literals like "1.0".
class FloatASTnode : public ASTnode {
  double Val;
  TOKEN Tok;

public:
  FloatASTnode(TOKEN tok, double Val) : Val(Val), Tok(tok) {}
  const std::string &getType() const { return Tok.lexeme; }

  virtual std::string to_string() const override {
    return fmtNode("FloatAST") + " " + fmtValue(std::to_string(Val));
  }

  virtual Value *codegen() override;
};

/// VariableASTnode - Class for referencing a variable (i.e. identifier), like
/// "a".
enum IDENT_TYPE { IDENTIFIER = 0 };
class VariableASTnode : public ASTnode {
protected:
  TOKEN Tok;
  std::string Name;
  IDENT_TYPE VarType;

public:
  VariableASTnode(TOKEN tok, const std::string &Name)
      : Tok(tok), Name(Name), VarType(IDENT_TYPE::IDENTIFIER) {}
  const std::string &getName() const { return Name; }
  const std::string &getType() const { return Tok.lexeme; }
  const IDENT_TYPE getVarType() const { return VarType; }

  virtual std::string to_string() const override {
    return fmtNode("VariableAST") + " " + fmtName(Name);
  }

  virtual Value *codegen() override;
};

/// ParamAST - Class for a parameter declaration
class ParamAST {
  std::string Name;
  std::string Type;

public:
  ParamAST(const std::string &name, const std::string &type)
      : Name(name), Type(type) {}
  const std::string &getName() const { return Name; }
  const std::string &getType() const { return Type; }

  virtual std::string to_string() const {
    return fmtNode("ParamAST") + " " + fmtName(Name) + " " + fmtType(Type);
  }
};

/// DeclAST - Base class for declarations, variables and functions
class DeclAST : public ASTnode {

public:
  virtual ~DeclAST() {}
};

/// VarDeclAST - Class for a variable declaration
class VarDeclAST : public DeclAST {
  std::unique_ptr<VariableASTnode> Var;
  std::string Type;

public:
  VarDeclAST(std::unique_ptr<VariableASTnode> var, const std::string &type)
      : Var(std::move(var)), Type(type) {}
  const std::string &getType() const { return Type; }
  const std::string &getName() const { return Var->getName(); }

  virtual std::string to_string() const override {
    return fmtNode("VarDeclAST") + " " + fmtName(Var->getName()) + " " + fmtType(Type);
  }  
};

/// GlobVarDeclAST - Class for a Global variable declaration
class GlobVarDeclAST : public DeclAST {
  std::unique_ptr<VariableASTnode> Var;
  std::string Type;

public:
  GlobVarDeclAST(std::unique_ptr<VariableASTnode> var, const std::string &type)
      : Var(std::move(var)), Type(type) {}
  const std::string &getType() const { return Type; }
  const std::string &getName() const { return Var->getName(); }

  virtual std::string to_string() const override {
    return fmtNode("GlobalVarDeclAST") + " " + fmtName(Var->getName()) + " " + fmtType(Type) + " " + fmtValue("(global)");
  }

  virtual Value *codegen() override;
};

/// FunctionPrototypeAST - Class for a function declaration's signature
class FunctionPrototypeAST {
  std::string Name;
  std::string Type;
  std::vector<std::unique_ptr<ParamAST>> Params; // vector of parameters

public:
  FunctionPrototypeAST(const std::string &name, const std::string &type,
                       std::vector<std::unique_ptr<ParamAST>> params)
      : Name(name), Type(type), Params(std::move(params)) {}

  const std::string &getName() const { return Name; }
  const std::string &getType() const { return Type; }
  int getSize() const { return Params.size(); }
  std::vector<std::unique_ptr<ParamAST>> &getParams() { return Params; }

  virtual std::string to_string() const {
    std::string header = fmtNode("FunctionProtoAST") + " " + fmtName(Name) + " " + fmtType(Type);
    if (Params.empty()) return header;

    std::string result = header + "\n";

    for (size_t i = 0; i < Params.size(); ++i) {
      result += formatChild(Params[i] ? Params[i]->to_string() : "<null>",
                            (i + 1 < Params.size()) ? "|- " : "`- ",
                            (i + 1 < Params.size()) ? "|  " : "   ");
      if (i + 1 < Params.size()) result += "\n";
    }
    return result;
  }
  
  Function *codegen();
};

class ExprAST : public ASTnode {
  std::unique_ptr<ASTnode> Node1;
  int Op;
  std::unique_ptr<ASTnode> Node2;

public:
  ExprAST(std::unique_ptr<ASTnode> node1, int op,
          std::unique_ptr<ASTnode> node2)
      : Node1(std::move(node1)), Op(op), Node2(std::move(node2)) {}
  int getOp() const { return Op; }
  const std::string &getType();

  virtual std::string to_string() const override {
    std::string title = fmtNode("BinaryOperator") + " " + fmtValue("'" + opToString(Op) + "'");
    auto n1 = Node1 ? Node1->to_string() : std::string("<null>");
    auto n2 = Node2 ? Node2->to_string() : std::string("<null>");
    std::string result = title + "\n";
    result += formatChild(n1, "|- ", "|  ");
    result += "\n";
    result += formatChild(n2, "`- ", "   ");
    return result;
  }

  virtual Value *codegen() override;
};

/// BlockAST - Class for a block with declarations followed by statements
class BlockAST : public ASTnode {
  std::vector<std::unique_ptr<VarDeclAST>> LocalDecls; // vector of local decls
  std::vector<std::unique_ptr<ASTnode>> Stmts;         // vector of statements

public:
  BlockAST(std::vector<std::unique_ptr<VarDeclAST>> localDecls,
           std::vector<std::unique_ptr<ASTnode>> stmts)
      : LocalDecls(std::move(localDecls)), Stmts(std::move(stmts)) {}

  virtual std::string to_string() const override {
    std::string title = fmtNode("CompoundStmt");
    std::vector<std::string> children;

    // Print Declarations first
    for (const auto &decl : LocalDecls) {
      children.push_back(decl ? decl->to_string() : "<null>");
    }
    // Print Statements next
    for (const auto &stmt : Stmts) {
      children.push_back(stmt ? stmt->to_string() : "<null>");
    }

    if (children.empty()) return title;

    std::string result = title + "\n";

    for (size_t i = 0; i < children.size(); ++i) {
      result += formatChild(children[i],
      (i + 1 < children.size()) ? "|- " : "`- ",
      (i + 1 < children.size()) ? "|  " : "   ");

      if (i + 1 < children.size()) result += "\n";
    }
    return result;
  }

  virtual Value *codegen() override;
};

/// FunctionDeclAST - This class represents a function definition itself.
class FunctionDeclAST : public DeclAST {
  std::unique_ptr<FunctionPrototypeAST> Proto;
  std::unique_ptr<ASTnode> Block;

public:
  FunctionDeclAST(std::unique_ptr<FunctionPrototypeAST> Proto,
                  std::unique_ptr<ASTnode> Block)
      : Proto(std::move(Proto)), Block(std::move(Block)) {}

  virtual std::string to_string() const override {
    std::string title = fmtNode("FunctionDecl") + " " + fmtName(Proto ? Proto->getName() : "<null>") + " " + fmtType(Proto ? Proto->getType() : "<null>");
    std::string protoStr = Proto ? Proto->to_string() : "<null>";
    std::string bodyStr  = Block ? Block->to_string() : "<null>";

    std::string result = title + "\n";
    result += formatChild(protoStr, "|- ", "|  ");
    result += "\n";
    result += formatChild(bodyStr,  "`- ", "   ");
    return result;
  }
  
  Value *codegen() override;
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ASTnode {
  std::unique_ptr<ASTnode> Cond, Then, Else;

public:
  IfExprAST(std::unique_ptr<ASTnode> Cond, std::unique_ptr<ASTnode> Then,
            std::unique_ptr<ASTnode> Else)
      : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

    virtual std::string to_string() const override {
      std::string title = fmtNode("IfStmt");
      std::string cond = Cond ? Cond->to_string() : "<null>";
      std::string thenStr = Then ? Then->to_string() : "<null>";
      std::string elseStr = Else ? Else->to_string() : fmtValue("<no else>");

      std::string result = title + "\n";
      result += formatChild(cond,   "|- condition: ", "|  ");
      result += "\n";
      result += formatChild(thenStr,"|- then:      ", "|  ");
      result += "\n";
      result += formatChild(elseStr,"`- else:      ", "   ");
      return result;
  }

  virtual Value *codegen() override;
};

/// WhileExprAST - Expression class for while.
class WhileExprAST : public ASTnode {
  std::unique_ptr<ASTnode> Cond, Body;

public:
  WhileExprAST(std::unique_ptr<ASTnode> cond, std::unique_ptr<ASTnode> body)
      : Cond(std::move(cond)), Body(std::move(body)) {}

    virtual std::string to_string() const override {
      std::string title = fmtNode("WhileStmt");
      std::string cond = Cond ? Cond->to_string() : "<null>";
      std::string body = Body ? Body->to_string() : "<null>";
      std::string result = title + "\n";
      result += formatChild(cond, "|- condition: ", "|  ");
      result += "\n";
      result += formatChild(body, "`- body:      ", "   ");
      return result;
  }

  virtual Value *codegen() override;
};

/// ReturnAST - Class for a return value
class ReturnAST : public ASTnode {
  std::unique_ptr<ASTnode> Val;

public:
  ReturnAST(std::unique_ptr<ASTnode> value) : Val(std::move(value)) {}

    virtual std::string to_string() const override {
      std::string title = fmtNode("ReturnStmt");
      if (!Val) return title;
      std::string v = Val->to_string();
      return title + "\n" + formatChild(v, "`- ", "   ");
  }

  virtual Value *codegen() override;
};

/// ArgsAST - Class for a function argument in a function call
class ArgsAST : public ASTnode {
  std::string Callee;
  std::vector<std::unique_ptr<ASTnode>> ArgsList;

public:
  ArgsAST(const std::string &Callee, std::vector<std::unique_ptr<ASTnode>> list)
      : Callee(Callee), ArgsList(std::move(list)) {}

    virtual std::string to_string() const override {
      
      std::string title = fmtNode("CallExpr") + " " + fmtName(Callee);

      if (ArgsList.empty()) return title;

      std::string result = title + "\n";

      for (size_t i = 0; i < ArgsList.size(); ++i) {
        const auto &arg = ArgsList[i];

        std::string child = arg ? arg->to_string() : "<null>";
        result += formatChild(child, (i + 1 < ArgsList.size()) ? "|- " : "`- ", (i + 1 < ArgsList.size()) ? "|  " : "   ");

        if (i + 1 < ArgsList.size()) result += "\n";
      }
      return result;
  }

  virtual Value *codegen() override;
};

/// LogError* - These are little helper function for error handling.
std::unique_ptr<ASTnode> LogError(TOKEN tok, const char *Str) {
  fprintf(stderr, "%d:%d Error: %s\n", tok.lineNo, tok.columnNo, Str);
  exit(2);
  return nullptr;
}

std::unique_ptr<FunctionPrototypeAST> LogErrorP(TOKEN tok, const char *Str) {
  LogError(tok, Str);
  exit(2);
  return nullptr;
}

std::unique_ptr<ASTnode> LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  exit(2);
  return nullptr;
}

//===----------------------------------------------------------------------===//
// Recursive Descent - Function call for each production
//===----------------------------------------------------------------------===//

static std::unique_ptr<ASTnode> ParseDecl();
static std::unique_ptr<ASTnode> ParseStmt();
static std::unique_ptr<ASTnode> ParseBlock();
static std::unique_ptr<ASTnode> ParseExper();
static std::unique_ptr<ParamAST> ParseParam();
static std::unique_ptr<VarDeclAST> ParseLocalDecl();
static std::vector<std::unique_ptr<ASTnode>> ParseStmtListPrime();

static std::unique_ptr<ASTnode> ParseLogicalOr();
static std::unique_ptr<ASTnode> ParseLogicalAnd();
static std::unique_ptr<ASTnode> ParseEquality();
static std::unique_ptr<ASTnode> ParseInequality();
static std::unique_ptr<ASTnode> ParseTerm();
static std::unique_ptr<ASTnode> ParseFactor();
static std::unique_ptr<ASTnode> ParseUnary();
static std::unique_ptr<ASTnode> ParsePrimary();
static std::vector<std::unique_ptr<ASTnode>> ParseArgs();
static std::vector<std::unique_ptr<ASTnode>> ParseArgListTail();

// element ::= FLOAT_LIT
static std::unique_ptr<ASTnode> ParseFloatNumberExpr() {
  auto Result = std::make_unique<FloatASTnode>(CurTok, CurTok.getFloatVal());
  getNextToken(); // consume the number
  return std::move(Result);
}

// element ::= INT_LIT
static std::unique_ptr<ASTnode> ParseIntNumberExpr() {
  auto Result = std::make_unique<IntASTnode>(CurTok, CurTok.getIntVal());
  getNextToken(); // consume the number
  return std::move(Result);
}

// element ::= BOOL_LIT
static std::unique_ptr<ASTnode> ParseBoolExpr() {
  auto Result = std::make_unique<BoolASTnode>(CurTok, CurTok.getBoolVal());
  getNextToken(); // consume the number
  return std::move(Result);
}

// param_list_prime ::= "," param param_list_prime
//                   |  ε
static std::vector<std::unique_ptr<ParamAST>> ParseParamListPrime() {
  std::vector<std::unique_ptr<ParamAST>> param_list;

  if (CurTok.type == COMMA) { // more parameters in list
    getNextToken();           // eat ","

    auto param = ParseParam();
    if (param) {
      printf("found param in param_list_prime: %s\n", param->getName().c_str());
      param_list.push_back(std::move(param));
      auto param_list_prime = ParseParamListPrime();
      for (unsigned i = 0; i < param_list_prime.size(); i++) {
        param_list.push_back(std::move(param_list_prime.at(i)));
      }
    }
  } else if (CurTok.type == RPAR) { // FOLLOW(param_list_prime)
    // expand by param_list_prime ::= ε
    // do nothing
  } else {
    LogError(CurTok, "expected ',' or ')' in list of parameter declarations");
  }

  return param_list;
}

// param ::= var_type IDENT
static std::unique_ptr<ParamAST> ParseParam() {

  if (CurTok.type != INT_TOK && CurTok.type != FLOAT_TOK && CurTok.type != BOOL_TOK) {
    LogError("expected parameter type ('int', 'float' or 'bool')");
  }

  std::string Type = CurTok.lexeme; // type token just seen
  getNextToken();                   // eat the type token

  if (CurTok.type != IDENT) {
    LogError("expected identifier in parameter");
  }

  std::string Name = CurTok.getIdentifierStr();
  printf("found param: %s of type %s\n", Name.c_str(), Type.c_str());
  getNextToken(); // eat IDENT

  // construct and return parameter
  return std::make_unique<ParamAST>(Name, Type);
}

// param_list ::= param param_list_prime
static std::vector<std::unique_ptr<ParamAST>> ParseParamList() {
  std::vector<std::unique_ptr<ParamAST>> param_list;

  auto param = ParseParam();
  if (param) {
    param_list.push_back(std::move(param));
    auto param_list_prime = ParseParamListPrime();
    for (unsigned i = 0; i < param_list_prime.size(); i++) {
      param_list.push_back(std::move(param_list_prime.at(i)));
    }
  }

  return param_list;
}

// params ::= param_list
//         |  ε
static std::vector<std::unique_ptr<ParamAST>> ParseParams() {
  std::vector<std::unique_ptr<ParamAST>> param_list;

  std::string Type;
  std::string Name = "";

  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK ||
      CurTok.type == BOOL_TOK) { // FIRST(param_list)

    auto list = ParseParamList();
    for (unsigned i = 0; i < list.size(); i++) {
      param_list.push_back(std::move(list.at(i)));
    }

  } else if (CurTok.type == VOID_TOK) { // FIRST("void")
    // void
    // check that the next token is a )
    getNextToken(); // eat 'void'
    if (CurTok.type != RPAR) {
      LogError(CurTok, "expected ')', after 'void' in \
       end of function declaration");
    }
  } else if (CurTok.type == RPAR) { // FOLLOW(params)
    // expand by params ::= ε
    // do nothing
  } else {
    LogError(
        CurTok,
        "expected 'int', 'bool' or 'float' in function declaration or ') in \
       end of function declaration");
  }

  return param_list;
}

/*** TODO : Task 2 - Parser ***

// args ::= arg_list
//      |  ε
// arg_list ::= arg_list "," expr
//      | expr

// rval ::= rval "||" rval
//      | rval "&&" rval
//      | rval "==" rval | rval "!=" rval
//      | rval "<=" rval | rval "<" rval | rval ">=" rval | rval ">" rval
//      | rval "+" rval | rval "-" rval
//      | rval "*" rval | rval "/" rval | rval "%" rval
//      | "-" rval | "!" rval
//      | "(" expr ")"
//      | IDENT | IDENT "(" args ")"
//      | INT_LIT | FLOAT_LIT | BOOL_LIT
**/

// expr ::= IDENT "=" expr | logical_or
static std::unique_ptr<ASTnode> ParseExper() {

  if (CurTok.type == IDENT) {
    TOKEN idTok = CurTok;
    getNextToken(); // eat IDENT
    if (CurTok.type == ASSIGN) {
      getNextToken(); // eat '='
      auto rhs = ParseExper();

      if (!rhs) return nullptr;

      auto lhs = std::make_unique<VariableASTnode>(idTok, idTok.getIdentifierStr());
      return std::make_unique<ExprAST>(std::move(lhs), ASSIGN, std::move(rhs));
    } else {
      putBackToken(CurTok); // put back the token after IDENT
      putBackToken(idTok);  // put back IDENT
      getNextToken();      // read IDENT again
   }
  } 

  return ParseLogicalOr();
}

// logical_or ::= logical_and logical_or_tail
// logical_or_tail ::= "||" logical_and logical_or_tail | epsilon
static std::unique_ptr<ASTnode> ParseLogicalOr() {
  auto AND = ParseLogicalAnd();
  if (!AND) return nullptr;

  while (CurTok.type == OR) //FIRST(logical_or_tail)
  {
    int Op = CurTok.type;
    getNextToken(); // eat '||'
    auto NextAND = ParseLogicalAnd();
    if (!NextAND) return nullptr;

    AND = std::make_unique<ExprAST>(std::move(AND), OR, std::move(NextAND));
  }

  return AND;
}

// logical_and ::= equality logical_and_tail
// logical_and_tail ::= "&&" equality logical_and_tail | epsilon
static std::unique_ptr<ASTnode> ParseLogicalAnd() {
  auto EQ = ParseEquality();
  if (!EQ) return nullptr;

  while (CurTok.type == AND) {
    int Op = CurTok.type;
    getNextToken(); // eat '&&'
    auto NextEQ = ParseEquality();
    if (!NextEQ) return nullptr;

    EQ = std::make_unique<ExprAST>(std::move(EQ), AND, std::move(NextEQ));
  }

  return EQ;
}

// equality ::= inequality equality_tail
// equality_tail ::= "==" inequality equality_tail | "!=" inequality equality_tail | epsilon
static std::unique_ptr<ASTnode> ParseEquality() {
  auto INEQ = ParseInequality();
  if (!INEQ) return nullptr;

  while (CurTok.type == EQ || CurTok.type == NE) {
    int Op = CurTok.type;
    getNextToken(); // eat '==' or '!='
    auto NextINEQ = ParseInequality();
    if (!NextINEQ) return nullptr;

    INEQ = std::make_unique<ExprAST>(std::move(INEQ), (Op == EQ) ? EQ : NE, std::move(NextINEQ));
  }

  return INEQ;
}

// inequality ::= term inequality_tail
// inequality_tail ::= "<=" term inequality_tail | "<" term inequality_tail | ">=" term inequality_tail | ">" term inequality_tail | epsilon
static std::unique_ptr<ASTnode> ParseInequality() {
  auto TERM = ParseTerm();
  if (!TERM) return nullptr;

  while (CurTok.type == LE || CurTok.type == LT || CurTok.type == GE || CurTok.type == GT) {
    int Op = CurTok.type;
    getNextToken(); // eat '<=', '<', '>=', or '>'
    auto NextTERM = ParseTerm();
    if (!NextTERM) return nullptr;

    int OpInt;
    switch (Op) {
      case LE: OpInt = LE; break;
      case LT: OpInt = LT; break;
      case GE: OpInt = GE; break;
      case GT: OpInt = GT; break;
    }

    TERM = std::make_unique<ExprAST>(std::move(TERM), OpInt, std::move(NextTERM));
  }

  return TERM;
}

// term ::= factor term_tail
// term_tail ::= "+" factor term_tail | "-" factor term_tail | epsilon
static std::unique_ptr<ASTnode> ParseTerm() {
  auto factor = ParseFactor();
  if (!factor) return nullptr;

  while (CurTok.type == PLUS || CurTok.type == MINUS) {
    int Op = CurTok.type;
    getNextToken(); // eat '+' or '-'
    auto NextFactor = ParseFactor();
    if (!NextFactor) return nullptr;

    factor = std::make_unique<ExprAST>(std::move(factor), (Op == PLUS) ? PLUS : MINUS, std::move(NextFactor));
  }

  return factor;
}

// factor ::= unary factor_tail
// factor_tail ::= "*" unary factor_tail | "/" unary factor_tail | "%" unary factor_tail | epsilon
static std::unique_ptr<ASTnode> ParseFactor() {
  auto unary = ParseUnary();
  if (!unary) return nullptr;

  while (CurTok.type == ASTERIX || CurTok.type == DIV || CurTok.type == MOD) {
    int Op = CurTok.type;
    getNextToken(); // eat '*', '/', or '%'
    auto NextUnary = ParseUnary();
    if (!NextUnary) return nullptr;
    
    int OpInt;
    switch (Op) {
      case ASTERIX: OpInt = ASTERIX; break;
      case DIV: OpInt = DIV; break;
      case MOD: OpInt = MOD; break;
    }

    unary = std::make_unique<ExprAST>(std::move(unary), OpInt, std::move(NextUnary));
  }

  return unary;
}

// unary ::= "-" unary | "!" unary | primary
static std::unique_ptr<ASTnode> ParseUnary() {
  if (CurTok.type == MINUS) {
    // Lower "-x" into "0 - x"
    TOKEN zeroTok;
    zeroTok.type = INT_LIT;
    zeroTok.lexeme = "0";
    zeroTok.lineNo = CurTok.lineNo;
    zeroTok.columnNo = CurTok.columnNo;

    getNextToken(); // eat '-'
    auto operand = ParseUnary();
    if (!operand) return nullptr;

    auto lhs = std::make_unique<IntASTnode>(zeroTok, 0);
    return std::make_unique<ExprAST>(std::move(lhs), MINUS, std::move(operand));
  } else if (CurTok.type == NOT) {
    // Lower "!x" into "x == false"
    TOKEN falseTok;
    falseTok.type = BOOL_LIT;
    falseTok.lexeme = "false";
    falseTok.lineNo = CurTok.lineNo;
    falseTok.columnNo = CurTok.columnNo;

    getNextToken(); // eat '!'
    auto operand = ParseUnary();
    if (!operand) return nullptr;

    auto rhs = std::make_unique<BoolASTnode>(falseTok, false);
    return std::make_unique<ExprAST>(std::move(operand), EQ, std::move(rhs));
  } else {
    return ParsePrimary();
  }
}

// primary ::= "(" expr ")" | IDENT | IDENT "(" args ")" | NT_LIT | FLOAT_LIT | BOOL_LIT
static std::unique_ptr<ASTnode> ParsePrimary() {
  if (CurTok.type == LPAR) {
    getNextToken(); // eat '('
    auto expr = ParseExper();
    if (!expr) return nullptr;

    if (CurTok.type != RPAR)
      return LogError(CurTok, "expected ')'");
    getNextToken(); // eat ')'
    return expr;
  } else if (CurTok.type == IDENT) {
    TOKEN idTok = CurTok;
    getNextToken(); // eat IDENT

    if (CurTok.type == LPAR) { // function call
      getNextToken(); // eat '('
      auto args = ParseArgs();
      if (CurTok.type != RPAR)
        return LogError(CurTok, "expected ')'");
      getNextToken(); // eat ')'

      return std::make_unique<ArgsAST>(idTok.getIdentifierStr(), std::move(args));
    } else { // variable reference
      return std::make_unique<VariableASTnode>(idTok, idTok.getIdentifierStr());
    }
  } else if (CurTok.type == INT_LIT) {
    return ParseIntNumberExpr();
  } else if (CurTok.type == FLOAT_LIT) {
    return ParseFloatNumberExpr();
  } else if (CurTok.type == BOOL_LIT) {
    return ParseBoolExpr();
  } else {
    return LogError(CurTok, "expected '(', identifier, integer literal, float literal, or boolean literal");
  }
}

//args ::= arg_list | epsilon
static std::vector<std::unique_ptr<ASTnode>> ParseArgs() {
  std::vector<std::unique_ptr<ASTnode>> args_list;

  if (CurTok.type == RPAR) { // FOLLOW(arg_list)
    // expand by arg_list ::= ε
    // do nothing
  } else if (CurTok.type == NOT || CurTok.type == MINUS ||
             CurTok.type == PLUS || CurTok.type == LPAR ||
             CurTok.type == IDENT || CurTok.type == INT_LIT ||
             CurTok.type == BOOL_LIT || CurTok.type == FLOAT_LIT) { // FIRST(expr)
    auto expr = ParseExper();
    if (expr) {
      args_list.push_back(std::move(expr));
      while (CurTok.type == COMMA) {
        getNextToken(); // eat ','
        auto next_expr = ParseExper();
        if (next_expr) {
          args_list.push_back(std::move(next_expr));
        } else {
          printf("Error parsing argument in function call\n");
          return {};
        }
      }
    }
  } else {
    LogError(CurTok,
             "expected expression or ')' in function call argument list");
  }

  return args_list;
}

// arg_list ::= expr arg_list_tail
// arg_list_tail ::= "," expr arg_list_tail | epsilon
static std::vector<std::unique_ptr<ASTnode>> ParseArgListTail() {
  std::vector<std::unique_ptr<ASTnode>> arg_list;

  if (CurTok.type == COMMA) { // more arguments in list
    getNextToken();           // eat ","

    auto expr = ParseExper();
    if (expr) {
      arg_list.push_back(std::move(expr));
      auto arg_list_tail = ParseArgListTail();
      for (unsigned i = 0; i < arg_list_tail.size(); i++) {
        arg_list.push_back(std::move(arg_list_tail.at(i)));
      }
    }
  } else if (CurTok.type == RPAR) { // FOLLOW(arg_list_tail)
    // expand by arg_list_tail ::= ε
    // do nothing
  } else {
    LogError(CurTok, "expected ',' or ')' in list of function call arguments");
  }

  return arg_list;
}

// expr_stmt ::= expr ";"
//            |  ";"
static std::unique_ptr<ASTnode> ParseExperStmt() {

  if (CurTok.type == SC) { // empty statement
    getNextToken();        // eat ;
    return nullptr;
  } else {
    auto expr = ParseExper();
    if (expr) {
      if (CurTok.type == SC) {
        getNextToken(); // eat ;
        return expr;
      } else {
        LogError(CurTok, "expected ';' to end expression statement");
      }
    } else
      return nullptr;
  }
  return nullptr;
}

// else_stmt  ::= "else" block
//             |  ε
static std::unique_ptr<ASTnode> ParseElseStmt() {

  if (CurTok.type == ELSE) { // FIRST(else_stmt)
    // expand by else_stmt  ::= "else" "{" stmt "}"
    getNextToken(); // eat "else"

    if (!(CurTok.type == LBRA)) {
      return LogError(
          CurTok, "expected { to start else block of if-then-else statment");
    }
    auto Else = ParseBlock();
    if (!Else)
      return nullptr;
    return Else;
  } else if (CurTok.type == NOT || CurTok.type == MINUS ||
             CurTok.type == PLUS || CurTok.type == LPAR ||
             CurTok.type == IDENT || CurTok.type == INT_LIT ||
             CurTok.type == BOOL_LIT || CurTok.type == FLOAT_LIT ||
             CurTok.type == SC || CurTok.type == LBRA || CurTok.type == WHILE ||
             CurTok.type == IF || CurTok.type == ELSE ||
             CurTok.type == RETURN ||
             CurTok.type == RBRA) { // FOLLOW(else_stmt)
    // expand by else_stmt  ::= ε
    // return an empty statement
    return nullptr;
  } else
    LogError(CurTok, "expected 'else' or one of \
    '!', '-', '+', '(' , IDENT , INT_LIT, BOOL_LIT, FLOAT_LIT, ';', \
    '{', 'while', 'if', 'else', ε, 'return', '}' ");

  return nullptr;
}

// if_stmt ::= "if" "(" expr ")" block else_stmt
static std::unique_ptr<ASTnode> ParseIfStmt() {
  getNextToken(); // eat the if.
  if (CurTok.type == LPAR) {
    getNextToken(); // eat (
    // condition.
    auto Cond = ParseExper();
    if (!Cond)
      return nullptr;
    if (CurTok.type != RPAR)
      return LogError(CurTok, "expected )");
    getNextToken(); // eat )

    if (!(CurTok.type == LBRA)) {
      return LogError(CurTok, "expected { to start then block of if statment");
    }

    auto Then = ParseBlock();
    if (!Then)
      return nullptr;
    auto Else = ParseElseStmt();

    return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
                                       std::move(Else));

  } else
    return LogError(CurTok, "expected (");

  return nullptr;
}

// return_stmt ::= "return" ";"
//             |  "return" expr ";"
static std::unique_ptr<ASTnode> ParseReturnStmt() {
  getNextToken(); // eat the return
  if (CurTok.type == SC) {
    getNextToken(); // eat the ;
    // return a null value
    return std::make_unique<ReturnAST>(std::move(nullptr));
  } else if (CurTok.type == NOT || CurTok.type == MINUS ||
             CurTok.type == PLUS || CurTok.type == LPAR ||
             CurTok.type == IDENT || CurTok.type == BOOL_LIT ||
             CurTok.type == INT_LIT ||
             CurTok.type == FLOAT_LIT) { // FIRST(expr)
    auto val = ParseExper();
    if (!val)
      return nullptr;

    if (CurTok.type == SC) {
      getNextToken(); // eat the ;
      return std::make_unique<ReturnAST>(std::move(val));
    } else
      return LogError(CurTok, "expected ';'");
  } else
    return LogError(CurTok, "expected ';' or expression");

  return nullptr;
}

// while_stmt ::= "while" "(" expr ")" stmt
static std::unique_ptr<ASTnode> ParseWhileStmt() {

  getNextToken(); // eat the while.
  if (CurTok.type == LPAR) {
    getNextToken(); // eat (
    // condition.
    auto Cond = ParseExper();
    if (!Cond)
      return nullptr;
    if (CurTok.type != RPAR)
      return LogError(CurTok, "expected )");
    getNextToken(); // eat )

    auto Body = ParseStmt();
    if (!Body)
      return nullptr;

    return std::make_unique<WhileExprAST>(std::move(Cond), std::move(Body));
  } else
    return LogError(CurTok, "expected (");
}

// stmt ::= expr_stmt
//      |  block
//      |  if_stmt
//      |  while_stmt
//      |  return_stmt
static std::unique_ptr<ASTnode> ParseStmt() {

  if (CurTok.type == NOT || CurTok.type == MINUS || CurTok.type == PLUS ||
      CurTok.type == LPAR || CurTok.type == IDENT || CurTok.type == BOOL_LIT ||
      CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT ||
      CurTok.type == SC) { // FIRST(expr_stmt)
    // expand by stmt ::= expr_stmt
    auto expr_stmt = ParseExperStmt();
    fprintf(stderr, "Parsed an expression statement\n");
    return expr_stmt;
  } else if (CurTok.type == LBRA) { // FIRST(block)
    auto block_stmt = ParseBlock();
    if (block_stmt) {
      fprintf(stderr, "Parsed a block\n");
      return block_stmt;
    }
  } else if (CurTok.type == IF) { // FIRST(if_stmt)
    auto if_stmt = ParseIfStmt();
    if (if_stmt) {
      fprintf(stderr, "Parsed an if statment\n");
      return if_stmt;
    }
  } else if (CurTok.type == WHILE) { // FIRST(while_stmt)
    auto while_stmt = ParseWhileStmt();
    if (while_stmt) {
      fprintf(stderr, "Parsed a while statment\n");
      return while_stmt;
    }
  } else if (CurTok.type == RETURN) { // FIRST(return_stmt)
    auto return_stmt = ParseReturnStmt();
    if (return_stmt) {
      fprintf(stderr, "Parsed a return statment\n");
      return return_stmt;
    }
  }
  // else if(CurTok.type == RBRA) { // FOLLOW(stmt_list_prime)
  //  expand by stmt_list_prime ::= ε
  //  do nothing
  //}
  else { // syntax error
    return LogError(CurTok, "expected BLA BLA\n");
  }
  return nullptr;
}

// stmt_list ::= stmt stmt_list_prime
static std::vector<std::unique_ptr<ASTnode>> ParseStmtList() {
  std::vector<std::unique_ptr<ASTnode>> stmt_list; // vector of statements
  auto stmt = ParseStmt();
  if (stmt) {
    stmt_list.push_back(std::move(stmt));
  }
  auto stmt_list_prime = ParseStmtListPrime();
  for (unsigned i = 0; i < stmt_list_prime.size(); i++) {
    stmt_list.push_back(std::move(stmt_list_prime.at(i)));
  }
  return stmt_list;
}

// stmt_list_prime ::= stmt stmt_list_prime
//                  |  ε
static std::vector<std::unique_ptr<ASTnode>> ParseStmtListPrime() {
  std::vector<std::unique_ptr<ASTnode>> stmt_list; // vector of statements
  if (CurTok.type == NOT || CurTok.type == MINUS || CurTok.type == PLUS ||
      CurTok.type == LPAR || CurTok.type == IDENT || CurTok.type == BOOL_LIT ||
      CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT || CurTok.type == SC ||
      CurTok.type == LBRA || CurTok.type == WHILE || CurTok.type == IF ||
      CurTok.type == ELSE || CurTok.type == RETURN) { // FIRST(stmt)
    // expand by stmt_list ::= stmt stmt_list_prime
    auto stmt = ParseStmt();
    if (stmt) {
      stmt_list.push_back(std::move(stmt));
    }
    auto stmt_prime = ParseStmtListPrime();
    for (unsigned i = 0; i < stmt_prime.size(); i++) {
      stmt_list.push_back(std::move(stmt_prime.at(i)));
    }

  } else if (CurTok.type == RBRA) { // FOLLOW(stmt_list_prime)
    // expand by stmt_list_prime ::= ε
    // do nothing
  }
  return stmt_list; // note stmt_list can be empty as we can have empty blocks,
                    // etc.
}

// local_decls_prime ::= local_decl local_decls_prime
//                    |  ε
static std::vector<std::unique_ptr<VarDeclAST>> ParseLocalDeclsPrime() {
  std::vector<std::unique_ptr<VarDeclAST>>
      local_decls_prime; // vector of local decls

  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK ||
      CurTok.type == BOOL_TOK) { // FIRST(local_decl)
    auto local_decl = ParseLocalDecl();
    if (local_decl) {
      local_decls_prime.push_back(std::move(local_decl));
    }
    auto prime = ParseLocalDeclsPrime();
    for (unsigned i = 0; i < prime.size(); i++) {
      local_decls_prime.push_back(std::move(prime.at(i)));
    }
  } else if (CurTok.type == MINUS || CurTok.type == NOT ||
             CurTok.type == LPAR || CurTok.type == IDENT ||
             CurTok.type == INT_LIT || CurTok.type == FLOAT_LIT ||
             CurTok.type == BOOL_LIT || CurTok.type == SC ||
             CurTok.type == LBRA || CurTok.type == IF || CurTok.type == WHILE ||
             CurTok.type == RETURN) { // FOLLOW(local_decls_prime)
    // expand by local_decls_prime ::=  ε
    // do nothing;
  } else {
    LogError(
        CurTok,
        "expected '-', '!', ('' , IDENT , STRING_LIT , INT_LIT , FLOAT_LIT, \
      BOOL_LIT, ';', '{', 'if', 'while', 'return' after local variable declaration\n");
  }

  return local_decls_prime;
}

// local_decl ::= var_type IDENT ";"
// var_type ::= "int"
//           |  "float"
//           |  "bool"
static std::unique_ptr<VarDeclAST> ParseLocalDecl() {
  TOKEN PrevTok;
  std::string Type;
  std::string Name = "";
  std::unique_ptr<VarDeclAST> local_decl;

  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK ||
      CurTok.type == BOOL_TOK) { // FIRST(var_type)
    PrevTok = CurTok;
    getNextToken(); // eat 'int' or 'float or 'bool'
    if (CurTok.type == IDENT) {
      Type = PrevTok.lexeme;
      Name = CurTok.getIdentifierStr(); // save the identifier name
      auto ident = std::make_unique<VariableASTnode>(CurTok, Name);
      local_decl = std::make_unique<VarDeclAST>(std::move(ident), Type);

      getNextToken(); // eat 'IDENT'
      if (CurTok.type != SC) {
        LogError(CurTok, "Expected ';' to end local variable declaration");
        return nullptr;
      }
      getNextToken(); // eat ';'
      fprintf(stderr, "Parsed a local variable declaration\n");
    } else {
      LogError(CurTok, "expected identifier' in local variable declaration");
      return nullptr;
    }
  }
  return local_decl;
}

// local_decls ::= local_decl local_decls_prime
static std::vector<std::unique_ptr<VarDeclAST>> ParseLocalDecls() {
  std::vector<std::unique_ptr<VarDeclAST>> local_decls; // vector of local decls

  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK ||
      CurTok.type == BOOL_TOK) { // FIRST(local_decl)

    auto local_decl = ParseLocalDecl();
    if (local_decl) {
      local_decls.push_back(std::move(local_decl));
    }
    auto local_decls_prime = ParseLocalDeclsPrime();
    for (unsigned i = 0; i < local_decls_prime.size(); i++) {
      local_decls.push_back(std::move(local_decls_prime.at(i)));
    }

  } else if (CurTok.type == MINUS || CurTok.type == NOT ||
             CurTok.type == LPAR || CurTok.type == IDENT ||
             CurTok.type == INT_LIT || CurTok.type == RETURN ||
             CurTok.type == FLOAT_LIT || CurTok.type == BOOL_LIT ||
             CurTok.type == COMMA || CurTok.type == LBRA || CurTok.type == IF ||
             CurTok.type == WHILE) { // FOLLOW(local_decls)
                                     // do nothing
  } else {
    LogError(
        CurTok,
        "expected '-', '!', '(' , IDENT , STRING_LIT , INT_LIT , FLOAT_LIT, \
        BOOL_LIT, ';', '{', 'if', 'while', 'return'");
  }

  return local_decls;
}

// parse block
// block ::= "{" local_decls stmt_list "}"
static std::unique_ptr<ASTnode> ParseBlock() {
  std::vector<std::unique_ptr<VarDeclAST>> local_decls; // vector of local decls
  std::vector<std::unique_ptr<ASTnode>> stmt_list;      // vector of statements

  getNextToken(); // eat '{'

  local_decls = ParseLocalDecls();
  fprintf(stderr, "Parsed a set of local variable declaration\n");
  stmt_list = ParseStmtList();
  fprintf(stderr, "Parsed a list of statements\n");
  if (CurTok.type == RBRA)
    getNextToken(); // eat '}'
  else {            // syntax error
    LogError(CurTok, "expected '}' , close body of block");
    return nullptr;
  }

  return std::make_unique<BlockAST>(std::move(local_decls),
                                    std::move(stmt_list));
}


static std::vector<std::unique_ptr<ASTnode>> TopLevelDecls;


// decl ::= var_decl
//       |  fun_decl
static std::unique_ptr<ASTnode> ParseDecl() {
  std::string IdName;
  std::vector<std::unique_ptr<ParamAST>> param_list;

  TOKEN PrevTok = CurTok; // to keep track of the type token

  if (CurTok.type == VOID_TOK || CurTok.type == INT_TOK ||
      CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {
    getNextToken(); // eat the VOID_TOK, INT_TOK, BOOL_TOK or FLOAT_TOK

    IdName = CurTok.getIdentifierStr(); // save the identifier name

    if (CurTok.type == IDENT) {
      auto ident = std::make_unique<VariableASTnode>(CurTok, IdName);
      getNextToken(); // eat the IDENT
      if (CurTok.type ==
          SC) {         // found ';' then this is a global variable declaration.
        getNextToken(); // eat ;
        fprintf(stderr, "Parsed a variable declaration\n");

        if (PrevTok.type != VOID_TOK)
          return std::make_unique<GlobVarDeclAST>(std::move(ident),
                                                  PrevTok.lexeme);
        else
          return LogError(PrevTok,
                          "Cannot have variable declaration with type 'void'");
      } else if (CurTok.type ==
                 LPAR) { // found '(' then this is a function declaration.
        getNextToken();  // eat (

        auto P =
            ParseParams(); // parse the parameters, returns a vector of params
        // if (P.size() == 0) return nullptr;
        fprintf(stderr, "Parsed parameter list for function\n");

        if (CurTok.type != RPAR) // syntax error
          return LogError(CurTok, "expected ')' in function declaration");

        getNextToken();          // eat )
        if (CurTok.type != LBRA) // syntax error
          return LogError(
              CurTok, "expected '{' in function declaration, function body");

        auto B = ParseBlock(); // parse the function body
        if (!B)
          return nullptr;
        else
          fprintf(stderr, "Parsed block of statements in function\n");

        // now create a Function prototype
        // create a Function body
        // put these to together
        // and return a std::unique_ptr<FunctionDeclAST>
        fprintf(stderr, "Parsed a function declaration\n");

        auto Proto = std::make_unique<FunctionPrototypeAST>(
            IdName, PrevTok.lexeme, std::move(P));
        return std::make_unique<FunctionDeclAST>(std::move(Proto),
                                                 std::move(B));
      } else
        return LogError(CurTok, "expected ';' or ('");
    } else
      return LogError(CurTok, "expected an identifier");

  } else
    LogError(CurTok,
             "expected 'void', 'int' or 'float' or EOF token"); // syntax error

  return nullptr;
}

// decl_list_prime ::= decl decl_list_prime
//                  |  ε
static void ParseDeclListPrime() {
  if (CurTok.type == VOID_TOK || CurTok.type == INT_TOK ||
      CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) { // FIRST(decl)

    if (auto decl = ParseDecl()) {
      fprintf(stderr, "Parsed a top-level variable or function declaration\n");
      TopLevelDecls.push_back(std::move(decl));
    }
    ParseDeclListPrime();
  } else if (CurTok.type == EOF_TOK) { // FOLLOW(decl_list_prime)
    // expand by decl_list_prime ::= ε
    // do nothing
  } else { // syntax error
    LogError(CurTok, "expected 'void', 'int', 'bool' or 'float' or EOF token");
  }
}

// decl_list ::= decl decl_list_prime
static void ParseDeclList() {
  auto decl = ParseDecl();
  
  if (decl) {
    llvm::outs() << decl->to_string() << "\n";
    fprintf(stderr, "Parsed a top-level variable or function declaration\n");
    TopLevelDecls.push_back(std::move(decl));
    ParseDeclListPrime();

  }
}

// extern ::= "extern" type_spec IDENT "(" params ")" ";"
static std::unique_ptr<FunctionPrototypeAST> ParseExtern() {
  std::string IdName;
  TOKEN PrevTok;

  if (CurTok.type == EXTERN) {
    getNextToken(); // eat the EXTERN


    if (CurTok.type == VOID_TOK || CurTok.type == INT_TOK ||
        CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {

      PrevTok = CurTok; // to keep track of the type token
      getNextToken();   // eat the VOID_TOK, INT_TOK, BOOL_TOK or FLOAT_TOK
      if (CurTok.type == IDENT) {
        IdName = CurTok.getIdentifierStr(); // save the identifier name
        auto ident = std::make_unique<VariableASTnode>(CurTok, IdName);
        getNextToken(); // eat the IDENT

        if (CurTok.type == LPAR) {       // found '(' - this is an extern function declaration.
          getNextToken(); // eat (

          auto P = ParseParams(); // parse the parameters, returns a vector of params
          if (P.size() == 0)
            return nullptr;
          else
            fprintf(stderr, "Parsed parameter list for external function\n");

          if (CurTok.type != RPAR) // syntax error
            return LogErrorP(
                CurTok, "expected ')' in closing extern function declaration");

          getNextToken(); // eat )
          if (CurTok.type == SC) {
            getNextToken(); // eat ";"
            auto Proto = std::make_unique<FunctionPrototypeAST>(
                IdName, PrevTok.lexeme, std::move(P));
            return Proto;
          } else
            return LogErrorP(
                CurTok,
                "expected ;' in ending extern function declaration statement");
        } else
          return LogErrorP(CurTok,
                           "expected (' in extern function declaration");
      }

    } else
      printf("Current token: %s \n", CurTok.lexeme.c_str());
      LogErrorP(CurTok, "expected 'void', 'int' or 'float' in extern function "
                        "declaration\n"); // syntax error
  }

  return nullptr;
}

// extern_list_prime ::= extern extern_list_prime
//                   |  ε
static void ParseExternListPrime() {

  if (CurTok.type == EXTERN) { // FIRST(extern)
    if (auto Extern = ParseExtern()) {
      Extern ->codegen();
      fprintf(stderr,
              "Parsed a top-level external function declaration -- 2\n");
    }
    ParseExternListPrime();
  } else if (CurTok.type == VOID_TOK || CurTok.type == INT_TOK ||
             CurTok.type == FLOAT_TOK ||
             CurTok.type == BOOL_TOK) { // FOLLOW(extern_list_prime)
    // expand by decl_list_prime ::= ε
    // do nothing
  } else { // syntax error
    LogError(CurTok, "expected 'extern' or 'void',  'int' ,  'float',  'bool'");
  }
}

// extern_list ::= extern extern_list_prime
static void ParseExternList() {
  auto Extern = ParseExtern();
  
  if (Extern) {
    llvm::outs() << Extern->to_string() << "\n";
    llvm::outs() << Extern->codegen() << "\n";
    fprintf(stderr, "Parsed a top-level external function declaration -- 1\n");
    // fprintf(stderr, "Current token: %s \n", CurTok.lexeme.c_str());
    if (CurTok.type == EXTERN)
      ParseExternListPrime();
  }
}

// program ::= extern_list decl_list
static void parser() {
  printf("Starting Parsing\n");
  if (CurTok.type == EOF_TOK)
    return;
  ParseExternList();
  if (CurTok.type == EOF_TOK)
    return;
  ParseDeclList();
  if (CurTok.type == EOF_TOK)
    return;
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;

static std::map<std::string, AllocaInst*> NamedValues;
static std::map<std::string, Value *> GlobalNamedValues;

Function *getFunction(std::string Name) {
  // First, see if the function has already been added to the current module.
  if (auto *F = TheModule->getFunction(Name))
    return F;

  // If not, check whether we can find it in the global module table.
  // This is for external functions.
  // return ExternalFunctions[Name];
  return nullptr;
}

static Type* getLLVMType(const std::string& typeStr) {
  if (typeStr == "int") {
    return Type::getInt32Ty(TheContext);
  } else if (typeStr == "float") {
    return Type::getFloatTy(TheContext);
  } else if (typeStr == "bool") {
    return Type::getInt1Ty(TheContext);
  } else if (typeStr == "void") {
    return Type::getVoidTy(TheContext);
  } else {
    return nullptr; // Unknown type
  }
}

static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                         const std::string &VarName,
                                        Type* type) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                   TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(type, nullptr, VarName);
}

Value *LogErrorV(TOKEN Tok, const char *Str) {
  LogError(Tok, Str);
  return nullptr;
}

Value *FunctionDeclAST::codegen() {
  Function *F = Proto->codegen();
  if (!F) {
    return nullptr;
  }

  if (!F->empty()) {
    return LogErrorV(CurTok, "Function cannot be redefined.");
  }

  BasicBlock *EntryBB = BasicBlock::Create(TheContext, "entry", F);
  Builder.SetInsertPoint(EntryBB);

  NamedValues.clear();

  for (auto &Arg : F->args()) {
    AllocaInst *Alloca = CreateEntryBlockAlloca(F, std::string(Arg.getName()), Arg.getType());
    Builder.CreateStore(&Arg, Alloca);
    NamedValues[std::string(Arg.getName())] = Alloca;
  }

  if (Block) Block->codegen();

  if (!EntryBB->getTerminator()) {
    if (F->getReturnType()->isVoidTy()) {
      Builder.CreateRetVoid();
    } else if (F->getReturnType()->isIntegerTy(32)) {
      Builder.CreateRet(ConstantInt::get(Type::getInt32Ty(TheContext), 0));
    } else if (F->getReturnType()->isFloatTy()) {
      Builder.CreateRet(ConstantFP::get(Type::getFloatTy(TheContext), APFloat(0.0f)));
    } else if (F->getReturnType()->isIntegerTy(1)) {
      Builder.CreateRet(ConstantInt::getFalse(TheContext));
    }
  }

  verifyFunction(*F);
  return F;
}

Function *FunctionPrototypeAST::codegen() {
  // Build function type
  std::vector<llvm::Type*> ArgTypes;
  for (auto &P : Params) {
    ArgTypes.push_back(getLLVMType(P->getType()));
  }
  llvm::Type* ReturnType = getLLVMType(Type);
  if (!ReturnType) return nullptr;

  llvm::FunctionType *FT = llvm::FunctionType::get(ReturnType, ArgTypes, false);
  llvm::Function *F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, TheModule.get());

  // Name arguments
  unsigned idx = 0;
  for (auto &Arg : F->args()) {
    Arg.setName(Params[idx++]->getName());
  }
  return F;
}

Value *GlobVarDeclAST::codegen() {
  llvm::Type* varType = getLLVMType(Type);
  
  if(!varType) {
    return nullptr;
  }

  Constant *init = Constant::getNullValue(varType);
  auto *GlobalVariable = new llvm::GlobalVariable (*TheModule, varType, false, GlobalValue::ExternalLinkage, init, getName());
  GlobalNamedValues[getName()] = GlobalVariable;
  return GlobalVariable;
}

Value *ExprAST::codegen() {
  if(Op == ASSIGN) {
    auto *Var = static_cast<VariableASTnode*>(Node1.get());
    if(!Var) return LogErrorV(CurTok, "destination of assignment must be a variable");
    Value *RHS = Node2->codegen();
    if(!RHS) return nullptr;

    AllocaInst *Variable = NamedValues[Var->getName()];
    if(!Variable) {
      if (GlobalNamedValues.count(Var->getName())) {
        Builder.CreateStore(RHS, GlobalNamedValues[Var->getName()]);
        return RHS;
      } else {
        return LogErrorV(CurTok, "Unknown variable name in assignment");
      }

      return LogErrorV(CurTok, "Unknown variable name in assignment");
    }

    Builder.CreateStore(RHS, Variable);
    return RHS;
  }

  Value *L = Node1->codegen();
  Value *R = Node2->codegen();

  if (!L || !R)
    return nullptr;

  auto promoteFP = [&](Value*& X) {
    if (X->getType()->isIntegerTy(32)) {
      X = Builder.CreateSIToFP(X, Type::getDoubleTy(TheContext));
    }
  };

  bool isFloatL = L->getType()->isFloatingPointTy();
  bool isFloatR = R->getType()->isFloatingPointTy();
  bool floatOp = isFloatL || isFloatR;

  if (floatOp) {
    Type *targetFP = isFloatL ? L->getType() : R->getType();
    if (!isFloatL && L->getType()->isIntegerTy(32)) {
      L = Builder.CreateSIToFP(L, targetFP);
    }
    if (!isFloatR && R->getType()->isIntegerTy(32)) {
      R = Builder.CreateSIToFP(R, targetFP);
    }
  }

  switch (Op) {
  case PLUS:
    return floatOp ? Builder.CreateFAdd(L, R, "addtmp") : Builder.CreateAdd(L, R, "addtmp");
  case MINUS:
    return floatOp ? Builder.CreateFSub(L, R, "subtmp") : Builder.CreateSub(L, R, "subtmp");
  case ASTERIX:
    return floatOp ? Builder.CreateFMul(L, R, "multmp") : Builder.CreateMul(L, R, "multmp");
  case DIV:
    return floatOp ? Builder.CreateFDiv(L, R, "divtmp") : Builder.CreateSDiv(L, R, "divtmp");
  case MOD:
    return Builder.CreateSRem(L, R);
  case EQ:
    return floatOp ? Builder.CreateFCmpOEQ(L, R, "eqtmp") : Builder.CreateICmpEQ(L, R, "eqtmp");
  case NE:
    return floatOp ? Builder.CreateFCmpONE(L, R, "netmp") : Builder.CreateICmpNE(L, R, "netmp");
  case LT:
    return floatOp ? Builder.CreateFCmpOLT(L, R, "lttmp") : Builder.CreateICmpSLT(L, R, "lttmp");
  case LE:
    return floatOp ? Builder.CreateFCmpOLE(L, R, "letmp") : Builder.CreateICmpSLE(L, R, "letmp");
  case GT:
    return floatOp ? Builder.CreateFCmpOGT(L, R, "gttmp") : Builder.CreateICmpSGT(L, R, "gttmp");
  case GE:
    return floatOp ? Builder.CreateFCmpOGE(L, R, "getmp") : Builder.CreateICmpSGE(L, R, "getmp");
  case AND: {
    Function *F = Builder.GetInsertBlock()->getParent();
    BasicBlock *RHSBB = BasicBlock::Create(TheContext, "and.rhs", F);
    BasicBlock *MergeBB = BasicBlock::Create(TheContext, "and.end", F);
    Builder.CreateCondBr(L, RHSBB, MergeBB);
    Value *R2 = R;
    Builder.SetInsertPoint(MergeBB);
    PHINode *PN = Builder.CreatePHI(Type::getInt1Ty(TheContext), 2);
    PN->addIncoming(ConstantInt::getFalse(TheContext), Builder.GetInsertBlock()->getPrevNode());
    PN->addIncoming(R2, RHSBB);
    return PN;
  }
  case OR: {
    Function *F = Builder.GetInsertBlock()->getParent();
    BasicBlock *RHSBB = BasicBlock::Create(TheContext, "or.rhs", F);
    BasicBlock *MergeBB = BasicBlock::Create(TheContext, "or.end", F);
    Builder.CreateCondBr(L, MergeBB, RHSBB);
    Value *R2 = R;
    Builder.CreateBr(MergeBB);
    Builder.SetInsertPoint(MergeBB);
    PHINode *PN = Builder.CreatePHI(Type::getInt1Ty(TheContext), 2);
    PN->addIncoming(ConstantInt::getTrue(TheContext), Builder.GetInsertBlock()->getPrevNode());
    PN->addIncoming(R2, RHSBB);
    return PN;
  }
  }
  return LogErrorV(CurTok, "invalid binary operator");
}

Value *BlockAST::codegen() {
  std::map<std::string, AllocaInst*> OldScope = NamedValues;

  Function *F = Builder.GetInsertBlock()->getParent();
  for (auto &Decl : LocalDecls) {
    llvm::Type* type = getLLVMType(Decl->getType());
    AllocaInst *Alloca = CreateEntryBlockAlloca(F, Decl->getName(), type);
    NamedValues[Decl->getName()] = Alloca;
  }

  Value *Last = nullptr;
  for (auto &Stmt : Stmts) {
    if(Stmt) {
      Last = Stmt->codegen();
    }
  }

  NamedValues = OldScope;
  return Last;
}

Value *IfExprAST::codegen() {
  Value *ConditionValue = Cond->codegen();
  if (!ConditionValue) return nullptr;

  ConditionValue = Builder.CreateICmpNE(ConditionValue, ConstantInt::get(ConditionValue->getType(), 0), "ifcond");

  Function *F = Builder.GetInsertBlock()->getParent();
  BasicBlock *ThenBB = BasicBlock::Create(TheContext, "then", F);
  BasicBlock *ElseBB = Else ? BasicBlock::Create(TheContext, "else", F) : nullptr;
  BasicBlock *MergeBB = BasicBlock::Create(TheContext, "ifcont", F);

  Builder.CreateCondBr(ConditionValue, ThenBB, Else ? ElseBB : MergeBB);

  Builder.SetInsertPoint(ThenBB);
  Value *ThenValue = Then->codegen();
  (void)ThenValue;
  Builder.CreateBr(MergeBB);

  Value *ElseValue = nullptr;
  if (Else) {
    Builder.SetInsertPoint(ElseBB);
    ElseValue = Else->codegen();
    (void)ElseValue;
    Builder.CreateBr(MergeBB);
  }

  Builder.SetInsertPoint(MergeBB);
  return Constant::getNullValue(Type::getInt32Ty(TheContext));  
}

Value *WhileExprAST::codegen() {
  Function *F = Builder.GetInsertBlock()->getParent();

  BasicBlock *ConditionBB = BasicBlock::Create(TheContext, "while.cond", F);
  BasicBlock *BodyBB = BasicBlock::Create(TheContext, "while.body", F);
  BasicBlock *EndBB = BasicBlock::Create(TheContext, "while.end", F);

  Builder.CreateBr(ConditionBB);

  Builder.SetInsertPoint(ConditionBB);
  Value *ConditionValue = Cond->codegen();
  if (!ConditionValue) return nullptr;

  if(ConditionValue->getType()->isIntegerTy(1)) {

  } else if(ConditionValue->getType()->isIntegerTy()) {
    ConditionValue = Builder.CreateICmpNE(ConditionValue, ConstantInt::get(ConditionValue->getType(), 0));
  } else if(ConditionValue->getType()->isFloatingPointTy()) {
    ConditionValue = Builder.CreateFCmpONE(ConditionValue, ConstantFP::get(ConditionValue->getType(), 0.0));
  } 

  Builder.CreateCondBr(ConditionValue, BodyBB, EndBB);

  Builder.SetInsertPoint(BodyBB);
  if (Body) Body->codegen();
  Builder.CreateBr(ConditionBB);

  Builder.SetInsertPoint(EndBB);
  return Constant::getNullValue(Type::getInt32Ty(TheContext));
}

Value *ReturnAST::codegen() {
  if(!Val) {
    Builder.CreateRetVoid();
    return nullptr;
  }

  Value *V = Val->codegen();
  Builder.CreateRet(V);
  return V;
}

Value *ArgsAST::codegen() {
  Function *CalleeF = getFunction(Callee);
  if (!CalleeF) CalleeF = TheModule->getFunction(Callee);
  if (!CalleeF) return LogErrorV(CurTok, "Unknown function referenced");

  if (CalleeF->arg_size() != ArgsList.size())
    return LogErrorV(CurTok, "Incorrect # arguments passed");

  std::vector<Value *> ArgsV;
  size_t idx = 0;
  for (auto &Args : ArgsList) {
    Value *ArgValue =  Args->codegen();
    if (!ArgValue) return nullptr;

    Type* ParamType = CalleeF->getFunctionType()->getParamType(idx++);

    if (ParamType->isDoubleTy() && ArgValue->getType()->isIntegerTy(32)) {
      ArgValue = Builder.CreateSIToFP(ArgValue, ParamType);
    }
    ArgsV.push_back(ArgValue);
  }

  return Builder.CreateCall(CalleeF, ArgsV);
}

Value *IntASTnode::codegen() {
  return ConstantInt::get(TheContext, APInt(32, Val, true));
}

Value *FloatASTnode::codegen() {
  return ConstantFP::get(Type::getFloatTy(TheContext), Val);
}

Value *BoolASTnode::codegen() {
  return ConstantInt::get(TheContext, APInt(1, Bool, false));
}

Value *VariableASTnode::codegen() {
  // Look this variable up in the function.
  if (auto *A = NamedValues[Name]) {
    return Builder.CreateLoad(A->getAllocatedType(), A, Name.c_str());
  }

  auto it = GlobalNamedValues.find(Name);
  if (it != GlobalNamedValues.end()) {
    auto *GlobalValue = llvm::cast<llvm::GlobalVariable>(it->second);
    return Builder.CreateLoad(GlobalValue->getValueType(), GlobalValue, Name.c_str());
  }

  return LogErrorV(CurTok, "Unknown variable name");
}




//===----------------------------------------------------------------------===//
// AST Printer
//===----------------------------------------------------------------------===//

// void IntASTnode::display(int tabs) {
//   printf("%s\n",getType().c_str());
// }

llvm::raw_ostream& operator<<(llvm::raw_ostream& os, const ASTnode& ast) {
  os << ast.to_string();
  return os;
}


//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char **argv) {
  if (argc == 2) {
    pFile = fopen(argv[1], "r");
    if (pFile == NULL)
      perror("Error opening file");
  } else {
    std::cout << "Usage: ./code InputFile\n";
    return 1;
  }

  // initialize line number and column numbers to zero
  lineNo = 1;
  columnNo = 1;

  // get the first token
  getNextToken();
  // while (CurTok.type != EOF_TOK) {
  //   fprintf(stderr, "Token: %s with type %d\n", CurTok.lexeme.c_str(),
  //           CurTok.type);
  //   getNextToken();
  // }
  // fprintf(stderr, "Lexer Finished\n");

  // Make the module, which holds all the code.
  TheModule = std::make_unique<Module>("mini-c", TheContext);


  /* UNCOMMENT : Task 2 - Parser */
  parser();
  fprintf(stderr, "Parsing Finished\n");  

  for(auto &decl : TopLevelDecls) {
    decl->codegen();
  }

  printf(
      "********************* FINAL IR (begin) ****************************\n");
  // Print out all of the generated code into a file called output.ll
  // printf("%s\n", argv[1]);
  auto Filename = "output.ll";
  std::error_code EC;
  raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

  if (EC) {
    errs() << "Could not open file: " << EC.message();
    return 1;
  }
  // TheModule->print(errs(), nullptr); // print IR to terminal
  TheModule->print(dest, nullptr);
  printf(
      "********************* FINAL IR (end) ******************************\n");

  fclose(pFile); // close the file that contains the code that was parsed
  return 0;
}
