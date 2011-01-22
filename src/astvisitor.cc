#include <iostream>
#include <cstdlib>
#include "ast.h"
#include "astvisitor.h"
#include "typetable.h"

namespace clever { namespace ast {


/**
 * Displays an error message
 */
void ASTVisitor::error(std::string message) throw() {
	std::cerr << "Compile error: " << message << std::endl;
	exit(1);
}

/**
 * Performs a type compatible checking
 */
bool ASTVisitor::checkCompatibleTypes(Value* lhs, Value* rhs) throw() {
	/**
	 * Constants with different type cannot performs operation
	 */
	if (lhs->isConst() && lhs->hasSameKind(rhs) && !lhs->hasSameType(rhs)) {
		return false;
	}
	return true;
}

/**
 * Performs a constant folding optimization
 */
ConstantValue* ASTVisitor::constantFolding(int op, Value* lhs, Value* rhs) throw() {

#define DO_NUM_OPERATION(_op, type, x, y) \
	if (x->is##type()) return new ConstantValue(x->get##type() _op y->get##type());

#define DO_STR_OPERATION(_op, x, y) \
	if (x->isString()) return new ConstantValue(CSTRING(x->getString() _op y->getString()));

	/**
	 * Check if the variable value can be predicted
	 */
	if ((lhs->hasName() && lhs->isModified()) || (rhs->hasName() && rhs->isModified())) {
		return NULL;
	}

	switch (op) {
		case ast::PLUS:
			DO_NUM_OPERATION(+, Integer, lhs, rhs);
			DO_STR_OPERATION(+, lhs, rhs);
			DO_NUM_OPERATION(+, Double, lhs, rhs);
			break;
		case ast::MINUS:
			DO_NUM_OPERATION(-, Integer, lhs, rhs);
			DO_NUM_OPERATION(-, Double, lhs, rhs);
			break;
		case ast::DIV:
			DO_NUM_OPERATION(/, Integer, lhs, rhs);
			DO_NUM_OPERATION(/, Double, lhs, rhs);
			break;
		case ast::MULT:
			DO_NUM_OPERATION(*, Integer, lhs, rhs);
			DO_NUM_OPERATION(*, Double, lhs, rhs);
			break;
		case ast::OR:
			DO_NUM_OPERATION(|, Integer, lhs, rhs);
			break;
		case ast::XOR:
			DO_NUM_OPERATION(^, Integer, lhs, rhs);
			break;
		case ast::AND:
			DO_NUM_OPERATION(&, Integer, lhs, rhs);
			break;
		case ast::GREATER:
			DO_NUM_OPERATION(>, Integer, lhs, rhs);
			DO_NUM_OPERATION(>, Double, lhs, rhs);
			break;
		case ast::LESS:
			DO_NUM_OPERATION(<, Integer, lhs, rhs);
			DO_NUM_OPERATION(<, Double, lhs, rhs);
			break;
		case ast::GREATER_EQUAL:
			DO_NUM_OPERATION(>=, Integer, lhs, rhs);
			DO_NUM_OPERATION(>=, Double, lhs, rhs);
			break;
		case ast::LESS_EQUAL:
			DO_NUM_OPERATION(<=, Integer, lhs, rhs);
			DO_NUM_OPERATION(<=, Double, lhs, rhs);
			break;
		case ast::EQUAL:
			DO_NUM_OPERATION(==, Integer, lhs, rhs);
			DO_NUM_OPERATION(==, Double, lhs, rhs);
			break;
		case ast::NOT_EQUAL:
			DO_NUM_OPERATION(!=, Integer, lhs, rhs);
			DO_NUM_OPERATION(!=, Double, lhs, rhs);
			break;
		case ast::MOD:
			DO_NUM_OPERATION(%, Integer, lhs, rhs);
			break;
	}
	return NULL;
}

/**
 * Return the Value pointer related to value type
 */
Value* ASTVisitor::getValue(ast::Expression* expr) throw() {
	Value* value = expr->get_value();

	if (value && value->hasName()) {
		Value* var = m_ssa.fetchVar(value);

		/**
		 * If the variable is found, we should use its pointer instead of
		 * fetching on runtime
		 */
		if (EXPECTED(var != NULL)) {
			return var;
		}
		error("Inexistent variable!");
	}
	return value;
}

AST_VISITOR(BinaryExpression) {
	Value* lhs = getValue(expr->get_lhs());
	Value* rhs = getValue(expr->get_rhs());
	ConstantValue* result = NULL;

	if (!checkCompatibleTypes(lhs, rhs)) {
		error("Type mismatch!");
	}
	if (lhs->isPrimitive() && !expr->isAssigned()) {
		result = constantFolding(expr->get_op(), lhs, rhs);
	}
	if (result) {
		/**
		 * Don't generate the opcode, the expression was evaluated in
		 * compile-time
		 */
		expr->set_optimized(true);
		expr->set_result(result);
		return;
	}
	if (expr->isAssigned()) {
		expr->set_result(lhs);
		lhs->addRef();
		lhs->setModified();
	} else {
		expr->set_result(new Value);
	}

	lhs->addRef();
	rhs->addRef();

	switch (expr->get_op()) {
		case ast::PLUS:  pushOpcode(new Opcode(OP_PLUS,   &VM::plus_handler,   lhs, rhs, expr->get_value())); break;
		case ast::DIV:   pushOpcode(new Opcode(OP_DIV,    &VM::div_handler,    lhs, rhs, expr->get_value())); break;
		case ast::MULT:  pushOpcode(new Opcode(OP_MULT,   &VM::mult_handler,   lhs, rhs, expr->get_value())); break;
		case ast::MINUS: pushOpcode(new Opcode(OP_MINUS,  &VM::minus_handler,  lhs, rhs, expr->get_value())); break;
		case ast::XOR:   pushOpcode(new Opcode(OP_BW_XOR, &VM::bw_xor_handler, lhs, rhs, expr->get_value())); break;
		case ast::OR:    pushOpcode(new Opcode(OP_BW_OR,  &VM::bw_or_handler,  lhs, rhs, expr->get_value())); break;
		case ast::AND:   pushOpcode(new Opcode(OP_BW_AND, &VM::bw_and_handler, lhs, rhs, expr->get_value())); break;
		case ast::MOD:   pushOpcode(new Opcode(OP_MOD,    &VM::mod_handler,    lhs, rhs, expr->get_value())); break;
	}
}


/**
 * Generates the variable declaration opcode
 */
AST_VISITOR(VariableDecl) {
	ast::Expression* var_type = expr->get_type();
	ast::Expression* var_expr = expr->get_variable();
	ast::Expression* rhs_expr = expr->get_initial_value();
	NamedValue* variable = static_cast<NamedValue*>(var_expr->get_value());
	const Type* type = TypeTable::getType(var_type->get_value()->get_name());

	variable->set_type_ptr(type);

	/* Check if the declaration contains initialization */
	if (rhs_expr) {
		Value* value = getValue(rhs_expr);

		m_ssa.pushVar(variable);

		variable->setInitialized();
		variable->copy(value);

		variable->addRef();
		value->addRef();

		pushOpcode(new Opcode(OP_VAR_DECL, &VM::var_decl_handler, variable, value));
	} else {
		pushOpcode(new Opcode(OP_VAR_DECL, &VM::var_decl_handler, variable));
	}
}

/**
 * Generates the new block opcode
 */
AST_VISITOR(NewBlock) {
	/* Initializes new scope */
	m_ssa.newBlock();
}

/**
 * Generates the end block opcode
 */
AST_VISITOR(EndBlock) {
	/* Pop current scope */
	m_ssa.endBlock();
}

/**
 * Generates the pre increment opcode
 */
AST_VISITOR(PreIncrement) {
	Value* value = getValue(expr->get_expr());

	value->setModified();
	value->addRef();

	pushOpcode(new Opcode(OP_PRE_INC, &VM::pre_inc_handler, value, NULL, expr->get_value()));
}

/**
 * Generates the pos increment opcode
 */
AST_VISITOR(PosIncrement) {
	Value* value = getValue(expr->get_expr());

	value->setModified();
	value->addRef();

	pushOpcode(new Opcode(OP_POS_INC, &VM::pos_inc_handler, value, NULL, expr->get_value()));
}

/**
 * Generates the pre decrement opcode
 */
AST_VISITOR(PreDecrement) {
	Value* value = getValue(expr->get_expr());

	value->setModified();
	value->addRef();

	pushOpcode(new Opcode(OP_PRE_DEC, &VM::pre_dec_handler, value, NULL, expr->get_value()));
}

/**
 * Generates the pos decrement opcode
 */
AST_VISITOR(PosDecrement) {
	Value* value = getValue(expr->get_expr());

	value->setModified();
	value->addRef();

	pushOpcode(new Opcode(OP_POS_DEC, &VM::pos_dec_handler, value, NULL, expr->get_value()));
}

/**
 * Generates the JMPZ opcode for IF expression
 */
AST_VISITOR(IfExpression) {
	Value* value = getValue(expr->get_expr());
	Opcode* opcode = new Opcode(OP_JMPZ, &VM::jmpz_handler, value);
	Jmp jmp;

	jmp.push(opcode);
	m_jmps.push(jmp);

	value->addRef();

	pushOpcode(opcode);
}

/**
 * Generates a JMPZ opcode for ELSEIF expression
 */
AST_VISITOR(ElseIfExpression) {
	Value* value = getValue(expr->get_expr());
	Opcode* opcode = new Opcode(OP_JMPZ, &VM::jmpz_handler, value);
	ast::StartExpr* start_expr = static_cast<ast::StartExpr*>(expr->get_start_expr());

	/* Sets the if jmp to start of the ELSEIF expr */
	m_jmps.top().top()->set_jmp_addr1(start_expr->get_op_num());
	m_jmps.top().push(opcode);

	value->addRef();
	pushOpcode(opcode);
}

/**
 * Generates a JMP opcode for ELSE expression
 */
AST_VISITOR(ElseExpression) {
	Opcode* opcode = new Opcode(OP_JMP, &VM::jmp_handler);

	m_jmps.top().top()->set_jmp_addr1(getOpNum()+2);
	m_jmps.top().push(opcode);

	pushOpcode(opcode);
}

/**
 * Just set the jmp address of if-elsif-else to end of control structure
 */
AST_VISITOR(EndIfExpression) {
	Jmp jmp = m_jmps.top();

	/* Sets the jmp addr for the IF when there is no ELSE */
	if (m_jmps.top().size() == 1) {
		m_jmps.top().top()->set_jmp_addr1(getOpNum()+1);
	}

	while (!jmp.empty()) {
		Opcode* opcode = jmp.top();

		/* Points to out of if-elsif-else block */
		opcode->set_jmp_addr2(getOpNum()+1);

		jmp.pop();
	}

	m_jmps.pop();
}

/**
 * Generates the JMPZ opcode for WHILE expression
 */
AST_VISITOR(WhileExpression) {
	Value* value = getValue(expr->get_expr());
	Opcode* opcode = new Opcode(OP_JMPZ, &VM::jmpz_handler, value);
	Jmp jmp;

	jmp.push(opcode);
	m_jmps.push(jmp);
	m_brks.push(Jmp());

	value->addRef();

	pushOpcode(opcode);
}

/**
 * Just set the end jmp addr of WHILE expression
 */
AST_VISITOR(EndWhileExpression) {
	Opcode* opcode = new Opcode(OP_JMP, &VM::jmp_handler);
	ast::StartExpr* start_loop = static_cast<ast::StartExpr*>(expr->get_expr());
	unsigned int scope_out = getOpNum()+2;

	/* Points to out of WHILE block */
	while (!m_brks.top().empty()) {
		m_brks.top().top()->set_jmp_addr1(scope_out);
		m_brks.top().pop();
	}
	m_jmps.top().top()->set_jmp_addr1(scope_out);
	m_jmps.top().pop();
	m_jmps.pop();
	m_brks.pop();

	/* Points to start of WHILE expression */
	opcode->set_jmp_addr2(start_loop->get_op_num());

	pushOpcode(opcode);
}

/**
 * Just hold the current op number before the WHILE expression
 */
AST_VISITOR(StartExpr) {
	expr->set_op_num(getOpNum()+1);
}

/**
 * Generates opcode for logic expression which weren't optimized
 */
AST_VISITOR(LogicExpression) {
	Value* lhs = getValue(expr->get_lhs());
	Value* rhs = getValue(expr->get_rhs());
	ConstantValue* result = NULL;

	if (!checkCompatibleTypes(lhs, rhs)) {
		error("Type mismatch!");
	}

	if (lhs->isPrimitive()) {
		result = constantFolding(expr->get_op(), lhs, rhs);
	}
	if (result) {
		/**
		 * Don't generate the opcode, the expression was evaluated in
		 * compile-time
		 */
		expr->set_optimized(true);
		expr->set_result(result);
		return;
	}

	expr->set_result(new Value);

	lhs->addRef();
	rhs->addRef();

	switch (expr->get_op()) {
		case ast::GREATER:       pushOpcode(new Opcode(OP_GREATER,       &VM::greater_handler,       lhs, rhs, expr->get_value())); break;
		case ast::LESS:          pushOpcode(new Opcode(OP_LESS,          &VM::less_handler,          lhs, rhs, expr->get_value())); break;
		case ast::GREATER_EQUAL: pushOpcode(new Opcode(OP_GREATER_EQUAL, &VM::greater_equal_handler, lhs, rhs, expr->get_value())); break;
		case ast::LESS_EQUAL:    pushOpcode(new Opcode(OP_LESS_EQUAL,    &VM::less_equal_handler,    lhs, rhs, expr->get_value())); break;
		case ast::EQUAL:         pushOpcode(new Opcode(OP_EQUAL,         &VM::equal_handler,         lhs, rhs, expr->get_value())); break;
		case ast::NOT_EQUAL:     pushOpcode(new Opcode(OP_NOT_EQUAL,     &VM::not_equal_handler,     lhs, rhs, expr->get_value())); break;
	}
}

/**
 * Generates opcode for break statement
 */
AST_VISITOR(BreakExpression) {
	Opcode* opcode = new Opcode(OP_BREAK, &VM::break_handler);

	m_brks.top().push(opcode);

	pushOpcode(opcode);
}

/**
 * Creates a vector with the current value from a Value* pointers
 */
ValueVector* ASTVisitor::functionArgs(const ast::Arguments* args) throw() {
	ValueVector* values = new ValueVector();
	ast::Arguments::const_iterator it = args->begin(), end(args->end());

	values->reserve(args->size());

	while (it != end) {
		Value* value = getValue(*it);

		if (value) {
			value->addRef();
		}
		values->push_back(value);
		++it;
	}

	return values;
}

/**
 * Generates opcode for function call
 */
AST_VISITOR(FunctionCall) {
	const CString* name = expr->get_func()->get_name();
	FunctionPtr func = Compiler::getFunction(*name);
	CallableValue* call = new CallableValue(name);
	const ast::Arguments* args = expr->get_args();
	Value* arg_values = NULL;

	if (!func) {
		error("Function '" + *name + "' does not exists!");
	}

	call->set_callback(func);

	if (args) {
		arg_values = new Value;
		arg_values->set_type(Value::VECTOR);
		arg_values->setVector(functionArgs(args));
	}

	pushOpcode(new Opcode(OP_FCALL, &VM::fcall_handler, call, arg_values, expr->get_value()));
}

/**
 * Generates opcode for method call
 */
AST_VISITOR(MethodCall) {
	Value* variable = getValue(expr->get_variable());
	CallableValue* call = new CallableValue(expr->get_method()->get_value()->get_name());
	const MethodPtr method = variable->get_type_ptr()->getMethod(call->get_name());
	const ast::Arguments* args = expr->get_args();
	Value* arg_values = NULL;

	if (!method) {
		error("Method not found!");
	}

	call->set_type_ptr(variable->get_type_ptr());
	call->set_callback(method);
	call->set_context(variable);

	if (args) {
		arg_values = new Value;
		arg_values->set_type(Value::VECTOR);
		arg_values->setVector(functionArgs(args));
	}
	pushOpcode(new Opcode(OP_MCALL, &VM::mcall_handler, call, arg_values, expr->get_value()));
}

/**
 * Generates opcode for variable assignment
 */
AST_VISITOR(Assignment) {
	Value* lhs = getValue(expr->get_lhs());
	Value* rhs = getValue(expr->get_rhs());

	lhs->setModified();

	lhs->addRef();
	rhs->addRef();

	pushOpcode(new Opcode(OP_ASSIGN, &VM::assign_handler, lhs, rhs));
}

/**
 * Import statement
 */
AST_VISITOR(Import) {
	const CString* package = expr->get_package()->get_value()->get_name();
	ast::Expression* module = expr->get_module();

	if (module) {
		const CString* module_name = module->get_value()->get_name();

		Compiler::import(package, module_name);
	} else {
		Compiler::import(package);
	}
}

}} // clever::ast