#include "sema/SemanticAnalyzer.hpp"
#include "visitor/AstNodeInclude.hpp"

void SemanticAnalyzer::visit(ProgramNode &p_program)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */

    pushScope();

    SymbolEntry program_entry;
    program_entry.name = p_program.getNameCString();
    program_entry.kind = ProgramType;
    program_entry.level = 0;
    program_entry.type = "void";
    program_entry.attr_str = "";
    program_entry.line = p_program.getLocation().line;
    program_entry.column = p_program.getLocation().col;
    insert(program_entry);

    parent_entries_stack.push_back(program_entry);

    p_program.visitChildNodes(*this);

    parent_entries_stack.pop_back();

    popScope();

    printErrorMessages();
}

void SemanticAnalyzer::visit(DeclNode &p_decl)
{
    p_decl.visitChildNodes(*this);
}

void SemanticAnalyzer::visit(VariableNode &p_variable)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */

    SymbolEntry variable_entry;
    variable_entry.name = p_variable.getNameCString();

    variable_entry.level = getScopeLevel();
    variable_entry.type = p_variable.getTypeCString();
    variable_entry.attr_str = "";
    variable_entry.line = p_variable.getLocation().line;
    variable_entry.column = p_variable.getLocation().col;

    p_variable.visitChildNodes(*this);

    if (!child_entries_stack.empty() && child_entries_stack.top().kind == PropagateType)
    {
        variable_entry.kind = ConstantType;
        variable_entry.attr_str = child_entries_stack.top().attr_str;
        child_entries_stack.pop();
    }
    else if (parent_entries_stack.back().kind == ForLoopType)
    {
        variable_entry.kind = LoopVariableType;
    }
    else if (parent_entries_stack.back().kind == FunctionType)
    {
        variable_entry.kind = ParameterType;
    }
    else
    {
        variable_entry.kind = VariableType;
    }

    std::stringstream ss(variable_entry.type);
    std::string arr, dim_str;
    getline(ss, arr, ' ');
    if (getline(ss, arr))
    {
        for (size_t i = 0; i < arr.size(); i++)
        {
            if (arr[i] == '[')
            {
                dim_str.clear();
            }
            else if (arr[i] == ']')
            {
                if (std::stoi(dim_str) <= 0)
                {
                    // dimension error
                    variable_entry.attr_str = "error";

                    std::string error_message = "";
                    error_message += "'";
                    error_message += variable_entry.name;
                    error_message += "' declared as an array with an index that is not greater than 0";
                    listErrorMessage(variable_entry.line, variable_entry.column, error_message);
                    break;
                }
            }
            else
            {
                dim_str += arr[i];
            }
        }
    }

    insert(variable_entry);
    if (variable_entry.kind == LoopVariableType)
    {
        loop_table.push_back(variable_entry);
    }
}

void SemanticAnalyzer::visit(ConstantValueNode &p_constant_value)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
    SymbolEntry propagate_entry;
    propagate_entry.name = "";
    propagate_entry.kind = PropagateType;
    propagate_entry.level = getScopeLevel();
    propagate_entry.attr_str = p_constant_value.getConstantValueCString();
    propagate_entry.line = p_constant_value.getLocation().line;
    propagate_entry.column = p_constant_value.getLocation().col;

    PType::PrimitiveTypeEnum constant_type = p_constant_value.getTypeSharedPtr()->getPrimitiveType();

    switch (constant_type)
    {
    case PType::PrimitiveTypeEnum::kIntegerType:
        propagate_entry.type = "integer";
        break;
    case PType::PrimitiveTypeEnum::kRealType:
        propagate_entry.type = "real";
        break;
    case PType::PrimitiveTypeEnum::kBoolType:
        propagate_entry.type = "boolean";
        break;
    case PType::PrimitiveTypeEnum::kStringType:
        propagate_entry.type = "string";
        break;
    default:;
    }

    child_entries_stack.push(propagate_entry);
}

void SemanticAnalyzer::visit(FunctionNode &p_function)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */

    SymbolEntry function_entry;
    function_entry.name = p_function.getNameCString();
    function_entry.kind = FunctionType;
    function_entry.level = getScopeLevel();
    function_entry.attr_str = "";
    function_entry.line = p_function.getLocation().line;
    function_entry.column = p_function.getLocation().col;

    std::string proto_type = p_function.getPrototypeCString();
    std::stringstream ss(proto_type);
    std::getline(ss, function_entry.type, ' ');
    std::getline(ss, function_entry.attr_str);
    if (function_entry.attr_str.size() > 2)
    {
        function_entry.attr_str = function_entry.attr_str.substr(1, function_entry.attr_str.size() - 2);
    }
    else
    {
        function_entry.attr_str = "";
    }

    insert(function_entry);

    pushScope();

    parent_entries_stack.push_back(function_entry);

    p_function.visitChildNodes(*this);

    parent_entries_stack.pop_back();

    popScope();
}

void SemanticAnalyzer::visit(CompoundStatementNode &p_compound_statement)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
    bool addScope = true;
    if (parent_entries_stack.back().kind == FunctionType)
    {
        addScope = false;
    }

    SymbolEntry compound_statement_entry;
    compound_statement_entry.name = "";
    compound_statement_entry.kind = CompoundStatementType;
    compound_statement_entry.level = getScopeLevel();
    compound_statement_entry.type = "void";
    compound_statement_entry.attr_str = "";

    if (addScope)
    {
        pushScope();
    }

    parent_entries_stack.push_back(compound_statement_entry);

    p_compound_statement.visitChildNodes(*this);

    parent_entries_stack.pop_back();

    if (addScope)
    {
        popScope();
    }
}

void SemanticAnalyzer::visit(PrintNode &p_print)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */

    p_print.visitChildNodes(*this);

    SymbolEntry expression_entry = child_entries_stack.top();
    child_entries_stack.pop();

    if (expression_entry.kind != ConstantType && expression_entry.attr_str == "error")
    {
        // no need of semantic analysis
        return;
    }

    if (expression_entry.type != "integer" && expression_entry.type != "real" &&
        expression_entry.type != "boolean" && expression_entry.type != "string")
    {
        // error
        std::string error_message = "expression of print statement must be scalar type";
        listErrorMessage(expression_entry.line, expression_entry.column, error_message);
    }
}

void SemanticAnalyzer::visit(BinaryOperatorNode &p_bin_op)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */

    p_bin_op.visitChildNodes(*this);

    SymbolEntry right_operand_entry = child_entries_stack.top();
    child_entries_stack.pop();
    SymbolEntry left_operand_entry = child_entries_stack.top();
    child_entries_stack.pop();
    std::string operator_string = p_bin_op.getOpCString();

    SymbolEntry expression_entry;
    expression_entry.name = "";
    expression_entry.kind = PropagateType;
    expression_entry.level = getScopeLevel();
    expression_entry.attr_str = "";
    expression_entry.line = p_bin_op.getLocation().line;
    expression_entry.column = p_bin_op.getLocation().col;

    if ((left_operand_entry.kind != ConstantType && left_operand_entry.attr_str == "error") || (right_operand_entry.kind != ConstantType && right_operand_entry.attr_str == "error"))
    {
        // no need of semantic analysis
        expression_entry.type = "";
        expression_entry.attr_str = "error";
        child_entries_stack.push(expression_entry);
        return;
    }

    bool detect_error = false;
    if (operator_string == "+" || operator_string == "-" || operator_string == "*" || operator_string == "/")
    {
        if (left_operand_entry.type == "integer" && right_operand_entry.type == "integer")
        {
            expression_entry.type = "integer";
        }
        else if ((left_operand_entry.type == "real" && right_operand_entry.type == "integer") ||
                 (left_operand_entry.type == "integer" && right_operand_entry.type == "real") ||
                 (left_operand_entry.type == "real" && right_operand_entry.type == "real"))
        {
            expression_entry.type = "real";
        }
        else if (operator_string == "+" &&
                 left_operand_entry.type == "string" &&
                 right_operand_entry.type == "string")
        {
            expression_entry.type = "string";
        }
        else
        {
            // error
            detect_error = true;
        }
    }
    else if (operator_string == "mod")
    {
        if (left_operand_entry.type == "integer" && right_operand_entry.type == "integer")
        {
            expression_entry.type = "integer";
        }
        else
        {
            // error
            detect_error = true;
        }
    }
    else if (operator_string == "and" || operator_string == "or")
    {
        if (left_operand_entry.type == "boolean" && right_operand_entry.type == "boolean")
        {
            expression_entry.type = "boolean";
        }
        else
        {
            // error
            detect_error = true;
        }
    }
    else // relation operator "<" "<=" "=" "=>" ">" "<>"
    {
        if ((left_operand_entry.type == "integer" && right_operand_entry.type == "integer") ||
            (left_operand_entry.type == "real" && right_operand_entry.type == "integer") ||
            (left_operand_entry.type == "integer" && right_operand_entry.type == "real") ||
            (left_operand_entry.type == "real" && right_operand_entry.type == "real"))
        {
            expression_entry.type = "boolean";
        }
        else
        {
            // error
            detect_error = true;
        }
    }

    if (detect_error)
    {
        expression_entry.type = "";
        expression_entry.attr_str = "error";

        std::string error_message = "";
        error_message += "invalid operands to binary operator '" + operator_string + "' ";
        error_message += "('" + left_operand_entry.type + "' and";
        error_message += " '" + right_operand_entry.type + "')";
        listErrorMessage(expression_entry.line, expression_entry.column, error_message);
    }

    child_entries_stack.push(expression_entry);
}

void SemanticAnalyzer::visit(UnaryOperatorNode &p_un_op)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */

    p_un_op.visitChildNodes(*this);

    SymbolEntry operand_entry = child_entries_stack.top();
    child_entries_stack.pop();
    std::string operator_string = p_un_op.getOpCString();

    SymbolEntry expression_entry;
    expression_entry.name = "";
    expression_entry.kind = PropagateType;
    expression_entry.level = getScopeLevel();
    expression_entry.attr_str = "";
    expression_entry.line = p_un_op.getLocation().line;
    expression_entry.column = p_un_op.getLocation().col;

    bool detect_error = false;
    if (operator_string == "neg")
    {
        if (operand_entry.type == "integer")
        {
            expression_entry.type = "integer";
        }
        else if (operand_entry.type == "real")
        {
            expression_entry.type = "real";
        }
        else
        {
            // error
            detect_error = true;
        }
    }
    else if (operator_string == "not")
    {
        if (operand_entry.type == "boolean")
        {
            expression_entry.type = "boolean";
        }
        else
        {
            // error
            detect_error = true;
        }
    }

    if (detect_error)
    {
        expression_entry.type = "";
        expression_entry.attr_str = "error";

        std::string error_message = "";
        error_message += "invalid operand to unary operator '" + operator_string + "' ";
        error_message += "('" + operand_entry.type + "')";
        listErrorMessage(expression_entry.line, expression_entry.column, error_message);
    }

    child_entries_stack.push(expression_entry);
}

void SemanticAnalyzer::visit(FunctionInvocationNode &p_func_invocation)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */

    p_func_invocation.visitChildNodes(*this);

    SymbolEntry function_entry;
    function_entry.name = p_func_invocation.getNameCString();

    size_t narg = p_func_invocation.getNumOfArguments();
    size_t npar;

    if (!lookup(function_entry))
    {
        // error
        for (size_t i = 0; i < narg; i++)
        {
            child_entries_stack.pop();
        }

        function_entry.name = p_func_invocation.getNameCString();
        function_entry.kind = FunctionType;
        function_entry.type = "";
        function_entry.attr_str = "error";
        function_entry.line = p_func_invocation.getLocation().line;
        function_entry.column = p_func_invocation.getLocation().col;
        child_entries_stack.push(function_entry);

        return;
    }
    else if (function_entry.kind != FunctionType)
    {
        // error
        std::string error_message = "";
        error_message += "call of non-function symbol '" + function_entry.name + "'";
        listErrorMessage(p_func_invocation.getLocation().line, p_func_invocation.getLocation().col, error_message);

        for (size_t i = 0; i < narg; i++)
        {
            child_entries_stack.pop();
        }
        function_entry.attr_str = "error";
        function_entry.line = p_func_invocation.getLocation().line;
        function_entry.column = p_func_invocation.getLocation().col;
        child_entries_stack.push(function_entry);
        return;
    }
    else
    {
        std::string function_prototype = function_entry.attr_str;

        if (function_prototype.empty())
        {
            npar = 0;
        }
        else
        {
            npar = 1;
            for (size_t i = 0; i < function_prototype.size(); i++)
            {
                if (function_prototype[i] == ',')
                {
                    npar++;
                }
            }
        }

        if (narg != npar)
        {
            // error
            std::string error_message = "";
            error_message += "too few/much arguments provided for function '" + function_entry.name + "'";
            listErrorMessage(p_func_invocation.getLocation().line, p_func_invocation.getLocation().col, error_message);

            for (size_t i = 0; i < narg; i++)
            {
                child_entries_stack.pop();
            }
            function_entry.attr_str = "error";
            function_entry.line = p_func_invocation.getLocation().line;
            function_entry.column = p_func_invocation.getLocation().col;
            child_entries_stack.push(function_entry);
            return;
        }
    }

    if (narg == 0)
    {
        function_entry.level = getScopeLevel();
        child_entries_stack.push(function_entry);
        return;
    }

    std::string scalar_type = function_entry.type;
    std::string arr = function_entry.attr_str;

    std::stringstream ss(arr);
    std::vector<std::string> parameters_type;
    parameters_type.resize(npar);
    for (size_t i = 0; i < npar; i++)
    {
        getline(ss, parameters_type[i], ',');
        if (parameters_type[i][0] == ' ')
        {
            parameters_type[i] = parameters_type[i].substr(1, parameters_type[i].size() - 1);
        }
    }

    std::vector<size_t> arg_line, arg_col;
    std::vector<std::string> arguments_type;
    arg_line.resize(narg);
    arg_col.resize(narg);
    arguments_type.resize(narg);
    for (size_t i = arguments_type.size() - 1; i >= 0; i--)
    {
        arg_line[i] = child_entries_stack.top().line;
        arg_col[i] = child_entries_stack.top().column;
        arguments_type[i] = child_entries_stack.top().type;
        child_entries_stack.pop();
    }

    for (size_t i = 0; i < narg; i++)
    {
        if (arguments_type[i] != parameters_type[i] ||
            !(arguments_type[i] == "integer" && parameters_type[i] == "real"))
        {
            // error
            std::string error_message = "";
            error_message += "incompatible type passing '" + arguments_type[i] + "' ";
            error_message += "to parameter of type '" + parameters_type[i] + "'";
            listErrorMessage(arg_line[i], arg_col[i], error_message);

            function_entry.attr_str = "error";
            break;
        }
    }

    function_entry.level = getScopeLevel();
    child_entries_stack.push(function_entry);
}

void SemanticAnalyzer::visit(VariableReferenceNode &p_variable_ref)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */

    p_variable_ref.visitChildNodes(*this);

    SymbolEntry variable_entry;
    variable_entry.name = p_variable_ref.getNameCString();
    variable_entry.line = p_variable_ref.getLocation().line;
    variable_entry.column = p_variable_ref.getLocation().col;

    if (!lookup(variable_entry))
    {
        // error
        variable_entry.kind = VariableType;
        variable_entry.attr_str = "error";
    }
    else if (variable_entry.kind != ParameterType && variable_entry.kind != VariableType && variable_entry.kind != LoopVariableType && variable_entry.kind != ConstantType)
    {
        // error
        std::string error_message = "";
        error_message += "use of non-variable symbol '" + variable_entry.name + "'";
        listErrorMessage(p_variable_ref.getLocation().line, p_variable_ref.getLocation().col, error_message);

        variable_entry.attr_str = "error";
    }

    if (variable_entry.kind != ConstantType && variable_entry.attr_str == "error")
    {
        // no need of semantic analysis
        for (size_t i = 0; i < p_variable_ref.getNumOfDim(); i++)
        {
            child_entries_stack.pop();
        }

        child_entries_stack.push(variable_entry);
        return;
    }

    size_t ref_ndim = p_variable_ref.getNumOfDim();

    std::stringstream ss(variable_entry.type);
    std::string scalar_type, arr;
    size_t var_ndim = 0;
    getline(ss, scalar_type, ' ');
    if (getline(ss, arr))
    {
        for (size_t i = 0; i < arr.size(); i++)
        {
            if (arr[i] == '[')
            {
                var_ndim++;
            }
        }
    }

    SymbolEntry expression_entry;
    bool invalid_index = false;
    size_t invalid_index_line, invalid_index_column;
    for (size_t i = 0; i < ref_ndim; i++)
    {
        expression_entry = child_entries_stack.top();
        child_entries_stack.pop();

        if (expression_entry.type != "integer")
        {
            invalid_index = true;
            invalid_index_line = expression_entry.line;
            invalid_index_column = expression_entry.column;
        }
    }
    if (invalid_index)
    {
        // error
        std::string error_message = "index of array reference must be an integer";
        listErrorMessage(invalid_index_line, invalid_index_column, error_message);

        variable_entry.attr_str = "error";
        child_entries_stack.push(variable_entry);
        return;
    }

    if (ref_ndim > var_ndim)
    {
        // error
        std::string error_message = "";
        error_message += "there is an over array subscript on '" + variable_entry.name + "'";
        listErrorMessage(p_variable_ref.getLocation().line, p_variable_ref.getLocation().col, error_message);

        variable_entry.attr_str = "error";
        child_entries_stack.push(variable_entry);
    }
    else if (ref_ndim == var_ndim)
    {
        variable_entry.level = getScopeLevel();
        variable_entry.type = scalar_type;
        variable_entry.line = p_variable_ref.getLocation().line;
        variable_entry.column = p_variable_ref.getLocation().col;
        child_entries_stack.push(variable_entry);
    }
    else // ref_ndim < var_ndim
    {
        variable_entry.level = getScopeLevel();

        size_t dim_diff = var_ndim - ref_ndim;
        size_t sub_arr_idx;
        std::string sub_arr;
        for (size_t i = variable_entry.type.size(); i >= 0 && dim_diff > 0; i--)
        {
            if (variable_entry.type[i] == '[' || variable_entry.type[i] == ' ')
            {
                dim_diff--;
                sub_arr_idx = i;
            }
        }

        sub_arr = variable_entry.type.substr(sub_arr_idx, variable_entry.type.size() - sub_arr_idx);
        variable_entry.type = scalar_type + " " + sub_arr;

        variable_entry.line = p_variable_ref.getLocation().line;
        variable_entry.column = p_variable_ref.getLocation().col;
        child_entries_stack.push(variable_entry);
    }
}

void SemanticAnalyzer::visit(AssignmentNode &p_assignment)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */

    p_assignment.visitChildNodes(*this);

    SymbolEntry expression_entry = child_entries_stack.top();
    child_entries_stack.pop();
    SymbolEntry variable_reference_entry = child_entries_stack.top();
    child_entries_stack.pop();

    if (variable_reference_entry.kind != ConstantType && variable_reference_entry.attr_str == "error")
    {
        // no need of semantic analysis of the variable reference
        return;
    }
    else
    {
        if (variable_reference_entry.type != "integer" && variable_reference_entry.type != "real" &&
            variable_reference_entry.type != "boolean" && variable_reference_entry.type != "string")
        {
            // error
            std::string error_message = "array assignment is not allowed";
            listErrorMessage(variable_reference_entry.line, variable_reference_entry.column, error_message);

            return;
        }
        else if (variable_reference_entry.kind == ConstantType)
        {
            // error
            std::string error_message = "";
            error_message += "cannot assign to variable '" + variable_reference_entry.name + "' which is a constant";
            listErrorMessage(variable_reference_entry.line, variable_reference_entry.column, error_message);

            return;
        }
        else if (parent_entries_stack.back().kind != ForLoopType && variable_reference_entry.kind == LoopVariableType)
        {
            // error
            std::string error_message = "the value of loop variable cannot be modified inside the loop body";
            listErrorMessage(variable_reference_entry.line, variable_reference_entry.column, error_message);

            return;
        }
    }

    if (expression_entry.kind != ConstantType && expression_entry.attr_str == "error")
    {
        // no need of semantic analysis of the expression
        return;
    }
    else
    {
        if (expression_entry.type != "integer" && expression_entry.type != "real" &&
            expression_entry.type != "boolean" && expression_entry.type != "string")
        {
            // error
            std::string error_message = "array assignment is not allowed";
            listErrorMessage(expression_entry.line, expression_entry.column, error_message);
        }
        else if (variable_reference_entry.type != expression_entry.type &&
                 !(variable_reference_entry.type == "real" && expression_entry.type == "integer"))
        {
            // error
            std::string error_message = "";
            error_message += "assigning to '" + variable_reference_entry.type + "' ";
            error_message += "from incompatible type '" + expression_entry.type + "'";
            listErrorMessage(p_assignment.getLocation().line, p_assignment.getLocation().col, error_message);
        }
    }

    if (parent_entries_stack.back().kind == ForLoopType)
    {
        loop_table[loop_table.size() - 1].attr_str = expression_entry.attr_str;
    }
}

void SemanticAnalyzer::visit(ReadNode &p_read)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
    p_read.visitChildNodes(*this);

    SymbolEntry variable_reference_entry = child_entries_stack.top();
    child_entries_stack.pop();

    if (variable_reference_entry.kind != ConstantType && variable_reference_entry.attr_str == "error")
    {
        // no need of semantic analysis
        return;
    }

    if (variable_reference_entry.type != "integer" && variable_reference_entry.type != "real" &&
        variable_reference_entry.type != "boolean" && variable_reference_entry.type != "string")
    {
        // error
        std::string error_message = "variable reference of read statement must be scalar type";
        listErrorMessage(variable_reference_entry.line, variable_reference_entry.column, error_message);
    }
    else if (variable_reference_entry.kind == ConstantType || variable_reference_entry.kind == LoopVariableType)
    {
        // error
        std::string error_message = "variable reference of read statement cannot be a constant or loop variable";
        listErrorMessage(variable_reference_entry.line, variable_reference_entry.column, error_message);
    }
}

void SemanticAnalyzer::visit(IfNode &p_if)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */

    p_if.visitChildNodes(*this);

    SymbolEntry expression_entry = child_entries_stack.top();
    child_entries_stack.pop();

    if (expression_entry.kind != ConstantType && expression_entry.attr_str == "error")
    {
        // no need of semantic analysis
        return;
    }

    if (expression_entry.type != "boolean")
    {
        // error
        std::string error_message = "the expression of condition must be boolean type";
        listErrorMessage(expression_entry.line, expression_entry.column, error_message);
    }
}

void SemanticAnalyzer::visit(WhileNode &p_while)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */

    p_while.visitChildNodes(*this);

    SymbolEntry expression_entry = child_entries_stack.top();
    child_entries_stack.pop();

    if (expression_entry.type != "boolean")
    {
        // error
        std::string error_message = "the expression of condition must be boolean type";
        listErrorMessage(expression_entry.line, expression_entry.column, error_message);
    }
}

void SemanticAnalyzer::visit(ForNode &p_for)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */
    SymbolEntry for_loop_entry;
    for_loop_entry.name = "";
    for_loop_entry.kind = ForLoopType;
    for_loop_entry.level = getScopeLevel();
    for_loop_entry.type = "void";
    for_loop_entry.attr_str = "";

    pushScope();

    parent_entries_stack.push_back(for_loop_entry);

    p_for.visitChildNodes(*this);

    parent_entries_stack.pop_back();

    SymbolEntry loop_variable_entry = loop_table.back();
    loop_table.pop_back();
    SymbolEntry constant_value_entry = child_entries_stack.top();
    child_entries_stack.pop();

    if (stoi(loop_variable_entry.attr_str) > stoi(constant_value_entry.attr_str))
    {
        // error
        std::string error_message =
            "the lower bound and upper bound of iteration count must be in the incremental order";
        listErrorMessage(p_for.getLocation().line, p_for.getLocation().col, error_message);
    }

    popScope();
}

void SemanticAnalyzer::visit(ReturnNode &p_return)
{
    /*
     * TODO:
     *
     * 1. Push a new symbol table if this node forms a scope.
     * 2. Insert the symbol into current symbol table if this node is related to
     *    declaration (ProgramNode, VariableNode, FunctionNode).
     * 3. Travere child nodes of this node.
     * 4. Perform semantic analyses of this node.
     * 5. Pop the symbol table pushed at the 1st step.
     */

    p_return.visitChildNodes(*this);

    SymbolEntry expression_entry = child_entries_stack.top();
    child_entries_stack.pop();

    SymbolEntry function_entry;
    bool legal_region = false;
    for (const auto &parent_entry : parent_entries_stack)
    {
        if (parent_entry.kind == FunctionType && parent_entry.type != "void")
        {
            legal_region = true;
            function_entry = parent_entry;
            break;
        }
    }
    if (!legal_region)
    {
        // error
        std::string error_message = "program/procedure should not return a value";
        listErrorMessage(p_return.getLocation().line, p_return.getLocation().col, error_message);
        return;
    }

    if (expression_entry.kind != ConstantType && expression_entry.attr_str == "error")
    {
        // no need of semantic analysis
        return;
    }

    if (function_entry.type != expression_entry.type &&
        !(function_entry.type == "real" && expression_entry.type == "integer"))
    {
        // error
        std::string error_message = "";
        error_message += "return '" + expression_entry.type + "' ";
        error_message += "from a function with return type '" + function_entry.type + "'";
        listErrorMessage(expression_entry.line, expression_entry.column, error_message);
    }
}
