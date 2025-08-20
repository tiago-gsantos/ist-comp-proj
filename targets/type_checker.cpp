#include "targets/type_checker.h"
#include ".auto/all_nodes.h"  // automatically generated
#include <cdk/types/types.h>

#include "udf_parser.tab.h"

#define ASSERT_UNSPEC { if (node->type() != nullptr && !node->is_typed(cdk::TYPE_UNSPEC)) return; }

//---------------------------------------------------------------------------

udf::type_checker::~type_checker() {
  os().flush();
}

//---------------------------------------------------------------------------

bool udf::type_checker::pointerTypeComparison(std::shared_ptr<cdk::basic_type> left, std::shared_ptr<cdk::basic_type> right) {
  while (left->name() == cdk::TYPE_POINTER && right->name() == cdk::TYPE_POINTER) {
    left  = cdk::reference_type::cast(left)->referenced();
    right = cdk::reference_type::cast(right)->referenced();
  }
  return (left->name() == cdk::TYPE_INT && right->name() == cdk::TYPE_INT) ||
         (left->name() == cdk::TYPE_DOUBLE && right->name() == cdk::TYPE_DOUBLE) ||
         (left->name() == cdk::TYPE_STRING && right->name() == cdk::TYPE_STRING) ||
         (left->name() == cdk::TYPE_TENSOR && right->name() == cdk::TYPE_TENSOR);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_sequence_node(cdk::sequence_node *const node, int lvl) {
  for (size_t i = 0; i < node->size(); i++)
    node->node(i)->accept(this, lvl);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_nil_node(cdk::nil_node *const node, int lvl) {
  // EMPTY
}

void udf::type_checker::do_data_node(cdk::data_node *const node, int lvl) {
  // EMPTY
}

//---------------------------------------------------------------------------

void udf::type_checker::do_integer_node(cdk::integer_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_double_node(cdk::double_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
}

void udf::type_checker::do_string_node(cdk::string_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
}

//---------------------------------------------------------------------------

void udf::type_checker::processUnaryExpression(cdk::unary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);

  if(node->argument()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->argument());

    if(!input) throw std::string("node with unspecified type");

    node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }

  if (!node->argument()->is_typed(cdk::TYPE_INT))
    throw std::string("wrong type in argument of unary expression");

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::processBinaryExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;

  node->left()->accept(this, lvl + 2);
  if(node->left()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->left());

    if(!input) throw std::string("node with unspecified type");

    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if(!node->left()->is_typed(cdk::TYPE_INT))
    throw std::string("wrong type in left argument of binary expression");

  node->right()->accept(this, lvl + 2);
  if(node->right()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->right());

    if(!input) throw std::string("node with unspecified type");

    node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if(!node->right()->is_typed(cdk::TYPE_INT))
    throw std::string("wrong type in right argument of binary expression");

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_unary_minus_node(cdk::unary_minus_node *const node, int lvl) {
  processUnaryExpression(node, lvl);
}

void udf::type_checker::do_unary_plus_node(cdk::unary_plus_node *const node, int lvl) {
  processUnaryExpression(node, lvl);
}

void udf::type_checker::do_not_node(cdk::not_node *const node, int lvl) {
  processUnaryExpression(node, lvl);
}

void udf::type_checker::do_and_node(cdk::and_node *const node, int lvl) {
  processBinaryExpression(node, lvl);
}

void udf::type_checker::do_or_node(cdk::or_node *const node, int lvl) {
  processBinaryExpression(node, lvl);
}

//---------------------------------------------------------------------------

void udf::type_checker::processIDTExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;

  node->left()->accept(this, lvl + 2);
  if(node->left()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->left());
    
    if(!input) throw std::string("node with unspecified type");
    
    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }

  node->right()->accept(this, lvl + 2);
  if(node->right()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->right());

    if(!input) throw std::string("node with unspecified type");

    node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    cdk::tensor_type *tensor1 = dynamic_cast<cdk::tensor_type*>(node->left());
    cdk::tensor_type *tensor2 = dynamic_cast<cdk::tensor_type*>(node->right());

    if(!tensor1 || !tensor2) throw std::string("node with unspecified type");

    if(tensor1->dims() != tensor2->dims()){
      throw std::string("tensor dimensions don't match");
    }
    node->type(cdk::tensor_type::create(tensor1->dims()));
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_INT)) {
    cdk::tensor_type *tensor = dynamic_cast<cdk::tensor_type*>(node->left());

    if(!tensor) throw std::string("node with unspecified type");
    node->type(cdk::tensor_type::create(tensor->dims()));
  } else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    cdk::tensor_type *tensor2 = dynamic_cast<cdk::tensor_type*>(node->right());
    if(!tensor2) throw std::string("node with unspecified type");
    node->type(cdk::tensor_type::create(tensor2->dims()));
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    
    cdk::tensor_type *tensor1 = dynamic_cast<cdk::tensor_type*>(node->left());
    cdk::tensor_type *tensor2 = dynamic_cast<cdk::tensor_type*>(node->right());
    if(!tensor1 || !tensor2) throw std::string("node with unspecified type");

    node->type(cdk::tensor_type::create(tensor1->dims()));
  } else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    cdk::tensor_type *tensor2 = dynamic_cast<cdk::tensor_type*>(node->right());
    if(!tensor2) throw std::string("node with unspecified type");
    node->type(cdk::tensor_type::create(tensor2->dims()));
  } else {
    throw std::string("wrong types in binary expression");
  }
}

void udf::type_checker::processPIDTExpression(cdk::binary_operation_node *const node, int lvl, bool acceptPointerPointer) {
  ASSERT_UNSPEC;

  node->left()->accept(this, lvl + 2);
  if(node->left()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->left());
    
    if(!input) throw std::string("node with unspecified type");
    
    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }

  node->right()->accept(this, lvl + 2);
  if(node->right()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->right());

    if(!input) throw std::string("node with unspecified type");

    node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(node->left()->type());
  } else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_POINTER)) {
    node->type(node->right()->type());
  } else if (acceptPointerPointer && 
              node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER) && 
              pointerTypeComparison(node->left()->type(), node->right()->type())) { // TODO - é preciso verificar ponteiros?
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    cdk::tensor_type *tensor1 = dynamic_cast<cdk::tensor_type*>(node->left());
    cdk::tensor_type *tensor2 = dynamic_cast<cdk::tensor_type*>(node->right());

    if(!tensor1 || !tensor2) throw std::string("node with unspecified type");

    if(tensor1->dims() != tensor2->dims()){
      throw std::string("tensor dimensions don't match");
    }
    node->type(cdk::tensor_type::create(tensor1->dims()));
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_INT)) {
    cdk::tensor_type *tensor1 = dynamic_cast<cdk::tensor_type*>(node->left());
    if(!tensor1) throw std::string("node with unspecified type");
    node->type(cdk::tensor_type::create(tensor1->dims()));
  } else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    cdk::tensor_type *tensor2 = dynamic_cast<cdk::tensor_type*>(node->right());
    if(!tensor2) throw std::string("node with unspecified type");
    node->type(cdk::tensor_type::create(tensor2->dims()));
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    cdk::tensor_type *tensor1 = dynamic_cast<cdk::tensor_type*>(node->left());
    if(!tensor1) throw std::string("node with unspecified type");
    node->type(cdk::tensor_type::create(tensor1->dims()));
  } else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    cdk::tensor_type *tensor2 = dynamic_cast<cdk::tensor_type*>(node->right());
    if(!tensor2) throw std::string("node with unspecified type");
    node->type(cdk::tensor_type::create(tensor2->dims()));
  } else {
    throw std::string("wrong types in binary expression");
  }
}

// TODO - can we compare double to int?
void udf::type_checker::processScalarLogicalExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;

  node->left()->accept(this, lvl + 2);
  if(node->left()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->left());

    if(!input) throw std::string("node with unspecified type");

    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }

  node->right()->accept(this, lvl + 2);
  if(node->right()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->right());

    if(!input) throw std::string("node with unspecified type");

    node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }

  if (!(node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT)) &&
      !(node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_DOUBLE)))
    throw std::string("integer or double expression expected in binary logical expression");
  
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

// TODO - Can we compare double to int
void udf::type_checker::processGeneralLogicalExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;

  node->left()->accept(this, lvl + 2);
  if(node->left()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->left());

    if(!input) throw std::string("node with unspecified type");

    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }

  node->right()->accept(this, lvl + 2);
  if(node->right()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->right());

    if(!input) throw std::string("node with unspecified type");

    node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }

  if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER) &&
              pointerTypeComparison(node->left()->type(), node->right()->type())) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    cdk::tensor_type *tensor1 = dynamic_cast<cdk::tensor_type*>(node->left());
    cdk::tensor_type *tensor2 = dynamic_cast<cdk::tensor_type*>(node->right());

    if(!tensor1 || !tensor2) throw std::string("node with unspecified type");

    if(tensor1->dims() != tensor2->dims()){
      throw std::string("tensor dimensions don't match");
    }
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else {
    throw std::string("wrong types in binary expression");
  }
}

void udf::type_checker::do_add_node(cdk::add_node *const node, int lvl) {
  processPIDTExpression(node, lvl, false);
}
// TODO - change int - pointer
void udf::type_checker::do_sub_node(cdk::sub_node *const node, int lvl) {
  processPIDTExpression(node, lvl, true);
}
void udf::type_checker::do_mul_node(cdk::mul_node *const node, int lvl) {
  processIDTExpression(node, lvl);
}
void udf::type_checker::do_div_node(cdk::div_node *const node, int lvl) {
  processIDTExpression(node, lvl);
}
void udf::type_checker::do_mod_node(cdk::mod_node *const node, int lvl) {
  processBinaryExpression(node, lvl);
}
void udf::type_checker::do_lt_node(cdk::lt_node *const node, int lvl) {
  processScalarLogicalExpression(node, lvl);
}
void udf::type_checker::do_le_node(cdk::le_node *const node, int lvl) {
  processScalarLogicalExpression(node, lvl);
}
void udf::type_checker::do_ge_node(cdk::ge_node *const node, int lvl) {
  processScalarLogicalExpression(node, lvl);
}
void udf::type_checker::do_gt_node(cdk::gt_node *const node, int lvl) {
  processScalarLogicalExpression(node, lvl);
}
void udf::type_checker::do_ne_node(cdk::ne_node *const node, int lvl) {
  processGeneralLogicalExpression(node, lvl);
}
void udf::type_checker::do_eq_node(cdk::eq_node *const node, int lvl) {
  processGeneralLogicalExpression(node, lvl);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_variable_node(cdk::variable_node *const node, int lvl) {
  ASSERT_UNSPEC;
  const std::string &id = node->name();
  std::shared_ptr<udf::symbol> symbol = _symtab.find(id);

  if (symbol != nullptr) {
    node->type(symbol->type());
  } else {
    throw std::string("undeclared variable '" + id + "'.");
  }
}

void udf::type_checker::do_rvalue_node(cdk::rvalue_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->lvalue()->accept(this, lvl);
  node->type(node->lvalue()->type());
}


void udf::type_checker::do_assignment_node(cdk::assignment_node *const node, int lvl) {
  ASSERT_UNSPEC;
  
  node->lvalue()->accept(this, lvl + 2);
  if(node->lvalue()->is_typed(cdk::TYPE_UNSPEC))
    throw std::string("left value must have a type");
  
  node->rvalue()->accept(this, lvl + 2);
  if(node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node *>(node->rvalue());
    udf::objects_node *stackr = dynamic_cast<udf::objects_node *>(node->rvalue());
    
    if(input) {
      if(!node->lvalue()->is_typed(cdk::TYPE_INT) && !node->lvalue()->is_typed(cdk::TYPE_DOUBLE))
        throw std::string("invalid expression for lvalue node");
      
      node->rvalue()->type(node->lvalue()->type());
    }
    else if(stackr) {
      if(!node->lvalue()->is_typed(cdk::TYPE_POINTER))
        throw std::string("pointer is required to allocate");
      
      node->rvalue()->type(node->lvalue()->type());
    }
    else
      throw std::string("Unknown node with unspecified type");
  }

  if(node->lvalue()->is_typed(cdk::TYPE_INT) && node->rvalue()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if(node->lvalue()->is_typed(cdk::TYPE_DOUBLE) && 
            (node->rvalue()->is_typed(cdk::TYPE_DOUBLE) || node->rvalue()->is_typed(cdk::TYPE_INT))) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  }
  else if(node->lvalue()->is_typed(cdk::TYPE_STRING) && node->rvalue()->is_typed(cdk::TYPE_STRING)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
  }
  else if(node->lvalue()->is_typed(cdk::TYPE_POINTER) && node->rvalue()->is_typed(cdk::TYPE_POINTER) &&
            pointerTypeComparison(node->lvalue()->type(), node->rvalue()->type())){
    node->type(node->lvalue()->type());
  }
  else if(node->lvalue()->is_typed(cdk::TYPE_POINTER) && node->rvalue()->is_typed(cdk::TYPE_INT)) {
    node->type(node->lvalue()->type());
  }
  else if(node->lvalue()->is_typed(cdk::TYPE_TENSOR) && node->rvalue()->is_typed(cdk::TYPE_TENSOR)){
    cdk::tensor_type *tensor1 = dynamic_cast<cdk::tensor_type*>(node->lvalue());
    cdk::tensor_type *tensor2 = dynamic_cast<cdk::tensor_type*>(node->rvalue());
    if(!tensor1 || !tensor2) throw std::string("node with unspecified type");
    
    if(tensor1->dims() != tensor2->dims()){
      throw std::string("tensor dimensions don't match");
    }
    node->type(cdk::tensor_type::create(tensor1->dims()));
  } else {
    throw std::string("wrong types in assignment");
  }
}

//---------------------------------------------------------------------------

void udf::type_checker::do_evaluation_node(udf::evaluation_node *const node, int lvl) {
  node->argument()->accept(this, lvl + 2);
  if(node->argument()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->argument());

    if(!input) throw std::string("node with unspecified type");

    node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
}

// TODO - do I need to check UNSPEC when we have "write input"
void udf::type_checker::do_write_node(udf::write_node *const node, int lvl) {
  node->arguments()->accept(this, lvl + 2);

  for(size_t i = 0; i < node->arguments()->size(); i++) {
    cdk::expression_node *expression = dynamic_cast<cdk::expression_node *>(node->arguments()->node(i));

    if(!expression)
      throw std::string("Invalid node in write arguments.");

    if(expression->is_typed(cdk::TYPE_VOID)|| expression->is_typed(cdk::TYPE_POINTER))
      throw std::string("Wrong type in write argument.");
  }
}

//---------------------------------------------------------------------------

void udf::type_checker::do_input_node(udf::input_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(0, cdk::TYPE_UNSPEC));
}

//---------------------------------------------------------------------------

// TODO - How to check for init and step
void udf::type_checker::do_for_node(udf::for_node *const node, int lvl) {
  if (node->init())
    node->init()->accept(this, lvl + 4);
  if (node->condition()) {
    node->condition()->accept(this, lvl + 4);
    for(size_t i = 0; i < node->condition()->size(); i++) {
      cdk::expression_node *expression = dynamic_cast<cdk::expression_node *>(node->condition()->node(i));

      if(!expression)
        throw std::string("Invalid node in tensor arguments.");

      if(!expression->is_typed(cdk::TYPE_INT))
        throw std::string("Wrong type in for condition");
    }
  }
  if (node->step())
    node->step()->accept(this, lvl + 4);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_if_node(udf::if_node *const node, int lvl) {
  node->condition()->accept(this, lvl + 4);

  if(node->condition()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->condition());

    if(!input) throw std::string("node with unspecified type");

    node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }

  if (!node->condition()->is_typed(cdk::TYPE_INT))
    throw std::string("expected integer condition");
}

void udf::type_checker::do_if_else_node(udf::if_else_node *const node, int lvl) {
  node->condition()->accept(this, lvl + 4);

  if(node->condition()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->condition());

    if(!input) throw std::string("node with unspecified type");

    node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }

  if (!node->condition()->is_typed(cdk::TYPE_INT))
    throw std::string("expected integer condition");
}

//---------------------------------------------------------------------------

void udf::type_checker::do_continue_node(udf::continue_node *const node, int lvl) {
  //EMPTY
}

//---------------------------------------------------------------------------

void udf::type_checker::do_break_node(udf::break_node *const node, int lvl) {
  // EMPTY
}

//---------------------------------------------------------------------------

void udf::type_checker::do_function_call_node(udf::function_call_node *const node, int lvl) {
  ASSERT_UNSPEC;

  const std::string &id = node->identifier();
  auto symbol = _symtab.find(id);
  if (symbol == nullptr) throw std::string("symbol '" + id + "' is undeclared.");
  if (!symbol->isFunction()) throw std::string("symbol '" + id + "' is not a function.");

  node->type(symbol->type());

  if (node->arguments()->size() != symbol->number_of_arguments())
    throw std::string(
        "number of arguments in call (" + std::to_string(node->arguments()->size()) + ") must match declaration ("
            + std::to_string(symbol->number_of_arguments()) + ").");

  node->arguments()->accept(this, lvl + 4);
  for (size_t ax = 0; ax < node->arguments()->size(); ax++) {
    if (node->argument(ax)->type() == symbol->argument_type(ax)) {
      if (node->argument(ax)->type()->name() == cdk::TYPE_POINTER
      && symbol->argument_type(ax)->name() == cdk::TYPE_POINTER
      && !pointerTypeComparison(node->argument(ax)->type(), symbol->argument_type(ax))) {
        throw std::string("type mismatch for argument " + std::to_string(ax + 1) + " of '" + id + "'.");
      }
      continue;
    };
    if (symbol->argument_is_typed(ax, cdk::TYPE_DOUBLE) && node->argument(ax)->is_typed(cdk::TYPE_INT)) {
      continue;
    }
    throw std::string("type mismatch for argument " + std::to_string(ax + 1) + " of '" + id + "'.");
  }
    
}

//---------------------------------------------------------------------------


void udf::type_checker::do_return_node(udf::return_node *const node, int lvl) {
  if (node->retval()) {
    if (_function->type()->name() == cdk::TYPE_VOID)
      throw std::string("void function cannot return values");

    node->retval()->accept(this, lvl + 2);

    // function is auto: set the type of the return expression
    if (_function->type() == nullptr) {
      _function->set_type(node->retval()->type());
      return;
    }
  
    auto fnType = _function->type()->name();
    if (fnType == cdk::TYPE_INT) {
      if(!node->retval()->is_typed(cdk::TYPE_INT))
        throw std::string("wrong type for initializer (integer expected).");
    } else if (fnType == cdk::TYPE_DOUBLE) {
      if(!node->retval()->is_typed(cdk::TYPE_INT) && !node->retval()->is_typed(cdk::TYPE_DOUBLE))
        throw std::string("wrong type for initializer (integer or double expected)."); // TODO - is this possible?
    } else if (fnType == cdk::TYPE_STRING) {
      if(!node->retval()->is_typed(cdk::TYPE_STRING))
        throw std::string("wrong type for initializer (string expected).");
    } else if (fnType == cdk::TYPE_POINTER && node->retval()->is_typed(cdk::TYPE_POINTER)) {
      if(!pointerTypeComparison(_function->type(), node->retval()->type()))
        throw std::string("wrong type for initializer (pointer expected).");
    } else {
      throw std::string("unknown type for initializer.");
    }
  }
}

//---------------------------------------------------------------------------

void udf::type_checker::do_block_node(udf::block_node *const node, int lvl) {
  if (node->declarations())
    node->declarations()->accept(this, lvl + 2);
  
  if (node->instructions())
    node->instructions()->accept(this, lvl + 2);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_sizeof_node(udf::sizeof_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->expression()->accept(this, lvl + 2);
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

//---------------------------------------------------------------------------
void udf::type_checker::do_variable_declaration_node(udf::variable_declaration_node *const node, int lvl) {
  if (node->initializer()) {
    node->initializer()->accept(this, lvl + 2);
    if (node->initializer()->is_typed(cdk::TYPE_UNSPEC)){
      udf::input_node *input = dynamic_cast<udf::input_node*>(node->initializer());

      if(input != nullptr) {
        if(node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_DOUBLE))
          node->initializer()->type(node->type());
        else
          throw std::string("Unable to read input.");
      }
      else
        throw std::string("Unknown node with unspecified type.");
    } else if (node->type() == nullptr){
      node->type(node->initializer()->type());
    }
    else if (node->is_typed(cdk::TYPE_INT)) {
      if (!node->initializer()->is_typed(cdk::TYPE_INT)) throw std::string("wrong type for initializer (integer expected).");
    } else if (node->is_typed(cdk::TYPE_DOUBLE)) {
      if (!node->initializer()->is_typed(cdk::TYPE_INT) && !node->initializer()->is_typed(cdk::TYPE_DOUBLE)) {
        throw std::string("wrong type for initializer (integer or double expected).");
      }
    } else if (node->is_typed(cdk::TYPE_STRING)) {
      if (!node->initializer()->is_typed(cdk::TYPE_STRING)) {
        throw std::string("wrong type for initializer (string expected).");
      }
    } else if (node->is_typed(cdk::TYPE_POINTER)) { // TODO - Check if correct
      if (!node->initializer()->is_typed(cdk::TYPE_POINTER) || !pointerTypeComparison(node->type(), node->initializer()->type())) {
        throw std::string("wrong type for initializer (pointer expected).");
      }

    } else if (node->is_typed(cdk::TYPE_TENSOR)) {
      if (!node->initializer()->is_typed(cdk::TYPE_TENSOR)) {
        throw std::string("wrong type for initializer (string expected).");
      }
    } else {
      throw std::string("unknown type for initializer.");
    }
  }

  const std::string &id = node->identifier();
  auto symbol = udf::make_symbol(false, node->qualifier(), node->type(), id, (bool)node->initializer(), false);
  if (_symtab.insert(id, symbol)) {
    _parent->set_new_symbol(symbol);  // advise parent that a symbol has been inserted
  } else {
    throw std::string("variable '" + id + "' redeclared");
  }
}

//---------------------------------------------------------------------------

void udf::type_checker::do_address_of_node(udf::address_of_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);

  if(node->argument()->is_typed(cdk::TYPE_UNSPEC) || node->argument()->is_typed(cdk::TYPE_VOID))
    throw std::string("wrong type for address-of operation");

  node->type(cdk::reference_type::create(4, node->argument()->type()));
}

//---------------------------------------------------------------------------

void udf::type_checker::do_objects_node(udf::objects_node *const node, int lvl) {
  ASSERT_UNSPEC;
  
  node->argument()->accept(this, lvl + 2);
  if(node->argument()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node *>(node->argument());

    if(!input) throw std::string("Unknown node with unspecified type.");
    
    node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if(!node->argument()->is_typed(cdk::TYPE_INT))
    throw std::string("Integer expression expected in allocation expression.");

  // TODO - Maybe find a way to not always use 8 bytes, like a flag on the ".h"
  auto mytype = cdk::reference_type::create(4, cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  node->type(mytype);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_index_ptr_node(udf::index_ptr_node *const node, int lvl) {
  ASSERT_UNSPEC;

  node->base()->accept(this, lvl + 2);
  if(!node->base()->is_typed(cdk::TYPE_POINTER)) throw std::string("pointer expression expected in index left-value");

  node->index()->accept(this, lvl + 2);
  if(node->index()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->index());

    if(!input) throw std::string("Unknown node with unspecified type.");
    
    node->index()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if(!node->index()->is_typed(cdk::TYPE_INT))
    throw std::string("integer expression expected in left-value index");

  auto ref = cdk::reference_type::cast(node->base()->type());
  node->type(ref->referenced());
}

//---------------------------------------------------------------------------

void udf::type_checker::do_nullptr_node(udf::nullptr_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::reference_type::create(4, nullptr)); // TODO why size 4?
}

//---------------------------------------------------------------------------


void udf::type_checker::do_function_declaration_node(udf::function_declaration_node *const node, int lvl) {
  std::string id;

  if (node->identifier() == "udf")
    id = "_main";
  else if (node->identifier() == "_main")
    id = "._main";
  else
    id = node->identifier();

  
  auto function = udf::make_symbol(false, node->qualifier(), node->type(), id, false, true, true);

  std::vector < std::shared_ptr < cdk::basic_type >> argtypes;
  
  if (node->arguments()) {
    for (size_t ax = 0; ax < node->arguments()->size(); ax++)
      argtypes.push_back(node->argument(ax)->type());
  }
  function->set_argument_types(argtypes);

  std::shared_ptr<udf::symbol> previous = _symtab.find(function->name());
  if (previous) {
    if (previous->forward()
        && ((previous->qualifier() == tPUBLIC && node->qualifier() == tPUBLIC)
            || (previous->qualifier() == tPRIVATE && node->qualifier() == tPRIVATE))) {
      _symtab.replace(function->name(), function);
      _parent->set_new_symbol(function);
    } else {
      throw std::string("conflicting definition for '" + function->name() + "'");
    }
  } else {
    _symtab.insert(function->name(), function);
    _parent->set_new_symbol(function);
  }
}

//---------------------------------------------------------------------------


void udf::type_checker::do_function_definition_node(udf::function_definition_node *const node, int lvl) {
  std::string id;
  
  if (node->identifier() == "udf")
    id = "_main";
  else if (node->identifier() == "_main")
    id = "._main";
  else
    id = node->identifier();

  _inBlockReturnType = nullptr;

  
  auto function = udf::make_symbol(false, node->qualifier(), node->type(), id, false, true);

  std::vector < std::shared_ptr < cdk::basic_type >> argtypes;
  for (size_t ax = 0; ax < node->arguments()->size(); ax++)
    argtypes.push_back(node->argument(ax)->type());
  function->set_argument_types(argtypes);
  std::shared_ptr<udf::symbol> previous = _symtab.find(function->name());
  if (previous) {
    if (previous->forward()
        && ((previous->qualifier() == tPUBLIC && node->qualifier() == tPUBLIC)
            || (previous->qualifier() == tPRIVATE && node->qualifier() == tPRIVATE)
            || (previous->qualifier() == tFORWARD && node->qualifier() == tPUBLIC))) {
      _symtab.replace(function->name(), function);
      _parent->set_new_symbol(function);
    } else {
      throw std::string("conflicting definition for '" + function->name() + "'");
    }
  } else {
    _symtab.insert(function->name(), function);
    _parent->set_new_symbol(function);
  }
}

//---------------------------------------------------------------------------

void udf::type_checker::do_tensor_node(udf::tensor_node *const node, int lvl) {
  ASSERT_UNSPEC;
  if(!node->is_typed(cdk::TYPE_TENSOR))
    throw std::string("wrong type (tensor expected)");
  node->values()->accept(this, lvl + 2);
  
  for(size_t i = 0; i < node->values()->size(); i++) {
    cdk::expression_node *expression = dynamic_cast<cdk::expression_node *>(node->values()->node(i));

    if(!expression)
      throw std::string("Invalid node in tensor arguments.");

    if(!expression->is_typed(cdk::TYPE_INT) && !expression->is_typed(cdk::TYPE_DOUBLE))
      throw std::string("Wrong type in tensor values.");
  }
  
}

//---------------------------------------------------------------------------

void udf::type_checker::do_index_tensor_node(udf::index_tensor_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->base()->accept(this, lvl + 2);
  if(!node->base()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("wrong type for operation (tensor expected)");
  
  cdk::tensor_type *tensor = dynamic_cast<cdk::tensor_type*>(node->base());
  if(!tensor) throw std::string("node with unspecified type");
  
  
  node->index()->accept(this, lvl + 2);
  if(node->index()->size() != tensor->n_dims())
    throw std::string("Invalid indexation. Number of arguments must match tensor dims");

  // TODO - check if each index is within dims limits

  for(size_t i = 0; i < node->index()->size(); i++) {
    cdk::expression_node *expression = dynamic_cast<cdk::expression_node *>(node->index()->node(i));

    if(!expression)
      throw std::string("Invalid node in tensor index arguments.");

    if(!expression->is_typed(cdk::TYPE_INT))
      throw std::string("Wrong type in tensor index values.");
  }
  node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE)); 

}

//---------------------------------------------------------------------------

void udf::type_checker::do_tensor_reshape_node(udf::tensor_reshape_node * const node, int lvl) {
  ASSERT_UNSPEC;
  
  node->tensor()->accept(this, lvl + 2);
  if(!node->tensor()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("wrong type for operation (tensor expected)");
  
  cdk::tensor_type *tensor = dynamic_cast<cdk::tensor_type*>(node->tensor());
  if(!tensor) throw std::string("node with unspecified type");

  if(tensor->size() != node->size())
    throw std::string("Invalid reshape values");
  
  node->type(cdk::tensor_type::create(node->dims()));
}

//---------------------------------------------------------------------------

void udf::type_checker::do_tensor_contraction_node(udf::tensor_contraction_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  if(!node->left()->is_typed(cdk::TYPE_TENSOR) || !node->right()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("wrong type for operation (tensor expected)");

  cdk::tensor_type *tensor1 = dynamic_cast<cdk::tensor_type*>(node->left());
  cdk::tensor_type *tensor2 = dynamic_cast<cdk::tensor_type*>(node->right());
  if(!tensor1 || !tensor2) throw std::string("node with unspecified type");
  if(tensor1->dim(tensor1->n_dims()-1) != tensor2->dim(0))
    throw std::string("Tensors not compatible");
  
  std::vector<size_t> newDims;
  newDims.reserve(tensor1->n_dims() + tensor2->n_dims() - 1);
  for (size_t i = 0; i < tensor1->n_dims(); ++i) {
    newDims.push_back(tensor1->dim(i));
  }

  for (size_t j = 1; j < tensor2->n_dims(); ++j) {
    newDims.push_back(tensor2->dim(j));
  }
  
  node->type(cdk::tensor_type::create(newDims));
}

//---------------------------------------------------------------------------

void udf::type_checker::do_tensor_dims_node(udf::tensor_dims_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->tensor()->accept(this, lvl + 2);

  if(!node->tensor()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("wrong type for operation (tensor expected)");
  
  node->type(cdk::reference_type::create(4, cdk::primitive_type::create(4, cdk::TYPE_INT)));
}

//---------------------------------------------------------------------------

void udf::type_checker::do_tensor_dim_node(udf::tensor_dim_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->tensor()->accept(this, lvl + 2);

  if(!node->tensor()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("wrong type for operation (tensor expected)");

  node->idx()->accept(this, lvl + 2);
  if(node->idx()->is_typed(cdk::TYPE_UNSPEC)) {
    udf::input_node *input = dynamic_cast<udf::input_node*>(node->idx());

    if(!input) throw std::string("Unknown node with unspecified type.");
    
    node->idx()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  else if(!node->idx()->type() == cdk::TYPE_INT)
    throw std::string("Index in dim must be an Integer");

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

//---------------------------------------------------------------------------

void udf::type_checker::do_tensor_capacity_node(udf::tensor_capacity_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->tensor()->accept(this, lvl + 2);

  if(!node->tensor()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("wrong type for operation (tensor expected)");

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

//---------------------------------------------------------------------------

void udf::type_checker::do_tensor_rank_node(udf::tensor_rank_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->tensor()->accept(this, lvl + 2);

  if(!node->tensor()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("wrong type for operation (tensor expected)");

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}
