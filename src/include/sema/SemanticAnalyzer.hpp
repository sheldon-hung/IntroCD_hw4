#ifndef SEMA_SEMANTIC_ANALYZER_H
#define SEMA_SEMANTIC_ANALYZER_H

#include "visitor/AstNodeVisitor.hpp"

#include "AST/PType.hpp"

#include <vector>
#include <stack>
#include <string>
#include <iostream>
#include <sstream>

enum PNameType
{
  ProgramType,
  FunctionType,
  ParameterType,
  VariableType,
  LoopVariableType,
  ConstantType,

  PropagateType, // for implementing propagation of semantic analysis

  ForLoopType,          // for implementing for loop body detection
  CompoundStatementType // for implementing CompoundStatement body detection
};

struct SymbolEntry
{
  std::string name;
  PNameType kind;
  uint16_t level;
  std::string type;
  std::string attr_str;
  uint32_t line;
  uint32_t column;
};

class SymbolTable
{
public:
  std::vector<SymbolEntry> entries;

public:
  bool insert(SymbolEntry insert_entry)
  {
    // first detect if there are same names in the table
    for (const auto &entry : this->entries)
    {
      if (entry.name == insert_entry.name)
      {
        return false;
      }
    }
    this->entries.push_back(insert_entry);
    return true;
  }

  bool lookup(SymbolEntry &get_entry)
  {
    // search though the table
    for (const auto &entry : this->entries)
    {
      if (entry.name == get_entry.name)
      {
        get_entry = entry;
        return true;
      }
    }
    return false;
  }

  void dumpDemarcation(const char chr)
  {
    for (size_t i = 0; i < 110; ++i)
    {
      printf("%c", chr);
    }
    puts("");
  }

  void dumpSymbolEntry(const SymbolEntry dump_entry)
  {
    printf("%-33s", dump_entry.name.c_str());

    std::string kind_str = "";
    switch (dump_entry.kind)
    {
    case ProgramType:
      kind_str = "program";
      break;
    case FunctionType:
      kind_str = "function";
      break;
    case ParameterType:
      kind_str = "parameter";
      break;
    case VariableType:
      kind_str = "variable";
      break;
    case LoopVariableType:
      kind_str = "loop_var";
      break;
    case ConstantType:
      kind_str = "constant";
      break;
    default:;
    }
    printf("%-11s", kind_str.c_str());

    std::string scope_str = "";
    if (dump_entry.level == 0)
    {
      scope_str = "(global)";
    }
    else
    {
      scope_str = "(local)";
    }
    printf("%d%-10s", dump_entry.level, scope_str.c_str());

    printf("%-17s", dump_entry.type.c_str());

    if (dump_entry.kind != ConstantType && dump_entry.attr_str == "error")
    {
      std::string empty_str = "";
      printf("%-11s", empty_str.c_str());
    }
    else
    {
      printf("%-11s", dump_entry.attr_str.c_str());
    }

    puts("");
  }

  void dumpSymbolTable(void)
  {
    dumpDemarcation('=');
    printf("%-33s%-11s%-11s%-17s%-11s\n", "Name", "Kind", "Level", "Type", "Attribute");

    dumpDemarcation('-');

    for (const auto &entry : entries)
    {
      dumpSymbolEntry(entry);
    }

    dumpDemarcation('-');
  }
};

class SemanticAnalyzer final : public AstNodeVisitor
{
private:
  // TODO: something like symbol manager (manage symbol tables)
  //       context manager, return type manager
  std::vector<SymbolTable> tables;
  std::vector<SymbolEntry> loop_table;
  std::vector<SymbolEntry> parent_entries_stack;
  std::stack<SymbolEntry> child_entries_stack;
  bool dumpSymbolTable = true;
  std::vector<std::string> source_code;
  std::vector<std::string> error_messages;

  void pushScope()
  {
    SymbolTable empty_table;
    tables.push_back(empty_table);
  }
  void popScope()
  {
    if (dumpSymbolTable)
    {
      tables.back().dumpSymbolTable();
    }
    tables.pop_back();
  }

  bool insert(SymbolEntry insert_entry)
  {
    for (const auto &entry : loop_table)
    {
      if (entry.name == insert_entry.name)
      {
        std::string error_message = "";
        error_message += "symbol '" + insert_entry.name + "' is redeclared";
        listErrorMessage(insert_entry.line, insert_entry.column, error_message);
        return false;
      }
    }

    if (tables[tables.size() - 1].insert(insert_entry))
    {
      return true;
    }
    else
    {
      std::string error_message = "";
      error_message += "symbol '" + insert_entry.name + "' is redeclared";
      listErrorMessage(insert_entry.line, insert_entry.column, error_message);
      return false;
    }
  }

  bool lookup(SymbolEntry &get_entry)
  {
    for (int i = tables.size() - 1; i >= 0; i--)
    {
      if (tables[i].lookup(get_entry))
      {
        return true;
      }
    }

    std::string error_message = "";
    error_message += "use of undeclared symbol '" + get_entry.name + "'";
    listErrorMessage(get_entry.line, get_entry.column, error_message);

    return false;
  }

  uint16_t getScopeLevel()
  {
    return tables.size() - 1;
  }

public:
  void setSymbolTableDump(bool D)
  {
    dumpSymbolTable = D;
  }

  void setSourceCode(char *code[200])
  {
    source_code.resize(200);
    for (int i = 0; i < 200; i++)
    {
      if (code[i])
      {
        source_code[i] = code[i];
      }
      else
      {
        source_code[i] = "";
      }
    }
  }

  void listErrorMessage(uint32_t line, uint32_t column, std::string message)
  {
    std::string error_message = "";

    error_message += "<Error> Found in line " + std::to_string(line) + ", column " + std::to_string(column) + ": " + message + "\n";

    error_message += "    " + source_code[line] + "\n";

    error_message += "   ";
    for (uint32_t i = 0; i < column; i++)
    {
      error_message += " ";
    }
    error_message += "^\n";

    error_messages.push_back(error_message);
  }

  void printErrorMessages()
  {
    if (error_messages.empty())
    {
      // TODO: do not print this if there's any semantic error
      printf("\n"
             "|---------------------------------------------------|\n"
             "|  There is no syntactic error and semantic error!  |\n"
             "|---------------------------------------------------|\n");
    }
    else
    {
      for (const auto &error_message : error_messages)
      {
        std::cerr << error_message;
      }
    }
  }

public:
  ~SemanticAnalyzer() = default;
  SemanticAnalyzer() = default;

  void visit(ProgramNode &p_program) override;
  void visit(DeclNode &p_decl) override;
  void visit(VariableNode &p_variable) override;
  void visit(ConstantValueNode &p_constant_value) override;
  void visit(FunctionNode &p_function) override;
  void visit(CompoundStatementNode &p_compound_statement) override;
  void visit(PrintNode &p_print) override;
  void visit(BinaryOperatorNode &p_bin_op) override;
  void visit(UnaryOperatorNode &p_un_op) override;
  void visit(FunctionInvocationNode &p_func_invocation) override;
  void visit(VariableReferenceNode &p_variable_ref) override;
  void visit(AssignmentNode &p_assignment) override;
  void visit(ReadNode &p_read) override;
  void visit(IfNode &p_if) override;
  void visit(WhileNode &p_while) override;
  void visit(ForNode &p_for) override;
  void visit(ReturnNode &p_return) override;
};

#endif
