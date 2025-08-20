#include <string>
#include <sstream>
#include "targets/type_checker.h"
#include "targets/postfix_writer.h"
#include ".auto/all_nodes.h"  // all_nodes.h is automatically generated
#include "udf_parser.tab.h"  // para tPUBLIC, tPRIVATE, etc.
#include "targets/frame_size_calculator.h"


//---------------------------------------------------------------------------

void udf::postfix_writer::do_nil_node(cdk::nil_node * const node, int lvl) {
  // EMPTY
}
void udf::postfix_writer::do_data_node(cdk::data_node * const node, int lvl) {
  // EMPTY
}
void udf::postfix_writer::do_double_node(cdk::double_node * const node, int lvl) {
  if (_inFunctionBody) {
    _pf.DOUBLE(node->value()); // load number to the stack
  } else {
    _pf.SDOUBLE(node->value());    // double is on the DATA segment
  }
}
void udf::postfix_writer::do_not_node(cdk::not_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl);
  _pf.INT(0);
  _pf.EQ();
}
void udf::postfix_writer::do_and_node(cdk::and_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl = ++_lbl;
  node->left()->accept(this, lvl);
  _pf.DUP32();
  _pf.JZ(mklbl(lbl));
  node->right()->accept(this, lvl);
  _pf.AND();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
}
void udf::postfix_writer::do_or_node(cdk::or_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl = ++_lbl;
  node->left()->accept(this, lvl);
  _pf.DUP32();
  _pf.JNZ(mklbl(lbl));
  node->right()->accept(this, lvl);
  _pf.OR();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_sequence_node(cdk::sequence_node * const node, int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    node->node(i)->accept(this, lvl);
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_integer_node(cdk::integer_node * const node, int lvl) {
  if(_inFunctionBody)
    _pf.INT(node->value());
  else
    _pf.SINT(node->value());
}

void udf::postfix_writer::do_string_node(cdk::string_node * const node, int lvl) {
  int lbl1;

  /* generate the string */
  _pf.RODATA(); // strings are DATA readonly
  _pf.ALIGN(); // make sure we are aligned
  _pf.LABEL(mklbl(lbl1 = ++_lbl)); // give the string a name
  _pf.SSTRING(node->value()); // output string characters

  /* leave the address on the stack */
  if (_inFunctionBody) {
    // local variable initializer
    _pf.TEXT();
    _pf.ADDR(mklbl(lbl1));
  } else {
    // global variable initializer
    _pf.DATA();
    _pf.SADDR(mklbl(lbl1));
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_unary_minus_node(cdk::unary_minus_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl); // determine the value
  _pf.NEG(); // 2-complement
}

void udf::postfix_writer::do_unary_plus_node(cdk::unary_plus_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl); // determine the value
}

//---------------------------------------------------------------------------
void udf::postfix_writer::do_add_node(cdk::add_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (node->type()->name() == cdk::TYPE_TENSOR) {
    _functions_to_declare.insert(
        node->left()->type()->name()  == cdk::TYPE_TENSOR &&
        node->right()->type()->name() == cdk::TYPE_TENSOR
        ? "tensor_add" : "tensor_add_scalar"
    );

    if (node->left()->type()->name() == cdk::TYPE_TENSOR &&
        node->right()->type()->name() == cdk::TYPE_TENSOR) {
      node->left()->accept(this, lvl+2);
      node->right()->accept(this, lvl+2);
      _pf.CALL("tensor_add");
      _pf.TRASH(4 * 2);
      _pf.LDFVAL32();

    }
    else if (node->left()->type()->name() == cdk::TYPE_TENSOR) {
      node->right()->accept(this, lvl+2);
      if (node->right()->type()->name() == cdk::TYPE_INT) _pf.I2D();
      node->left()->accept(this, lvl+2);
      _pf.CALL("tensor_add_scalar");
      _pf.TRASH(4 + 8);
      _pf.LDFVAL32();

    }
    else {
      node->left()->accept(this, lvl+2);
      if (node->left()->type()->name() == cdk::TYPE_INT) _pf.I2D();
      node->right()->accept(this, lvl+2);
      _pf.CALL("tensor_add_scalar");
      _pf.TRASH(4 + 8);
      _pf.LDFVAL32();
    }

  }
  else{
    node->left()->accept(this, lvl + 2);
    if (node->type()->name() == cdk::TYPE_DOUBLE && node->left()->type()->name() == cdk::TYPE_INT) {
      _pf.I2D();
    }
    else if(node->type()->name() == cdk::TYPE_POINTER && node->left()->type()->name() == cdk::TYPE_INT) {
      auto ref = cdk::reference_type::cast(node->type());

      if(ref->referenced()->name() == cdk::TYPE_DOUBLE) {
        _pf.INT(3);
      }
      else {
        _pf.INT(2);
      }
      _pf.SHTL();
    }

    node->right()->accept(this, lvl + 2);
    if(node->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT)) {
      _pf.I2D();
    }
    else if (node->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_INT)){
      auto ref = cdk::reference_type::cast(node->type());
      if(ref->referenced()->name() == cdk::TYPE_DOUBLE) {
        _pf.INT(3);
      }
      else {
        _pf.INT(2);
      }
      _pf.SHTL();
    }

    

    if(node->is_typed(cdk::TYPE_DOUBLE)) {
      _pf.DADD();
    }
    else {
      _pf.ADD();
    }
  }
  
  
}
void udf::postfix_writer::do_sub_node(cdk::sub_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (node->type()->name() == cdk::TYPE_TENSOR) {

    if (node->left()->type()->name() == cdk::TYPE_TENSOR &&
        node->right()->type()->name() == cdk::TYPE_TENSOR) {
      _functions_to_declare.insert("tensor_sub");
      node->left()->accept(this, lvl+2);
      node->right()->accept(this, lvl+2);
      _pf.CALL("tensor_sub");
      _pf.TRASH(4 * 2);
      _pf.LDFVAL32();
    }
    else if (node->left()->type()->name() == cdk::TYPE_TENSOR) {
      _functions_to_declare.insert("tensor_sub");
      node->right()->accept(this, lvl+2);
      if (node->right()->type()->name() == cdk::TYPE_INT) _pf.I2D();
      node->left()->accept(this, lvl+2);
      _pf.CALL("tensor_sub_scalar");
      _pf.TRASH(4 + 8);
      _pf.LDFVAL32();
    }
    else {
      node->left()->accept(this, lvl + 2);
      if (node->left()->type()->name() == cdk::TYPE_INT) _pf.I2D();
      _pf.DOUBLE(-1.0);
      node->right()->accept(this, lvl + 2);
      _functions_to_declare.insert("tensor_mul_scalar");
      _pf.CALL("tensor_mul_scalar");
      _pf.TRASH(4 + 8);
      _pf.LDFVAL32();
      _functions_to_declare.insert("tensor_add_scalar");
      _pf.CALL("tensor_add_scalar");
      _pf.TRASH(4 + 8);
      _pf.LDFVAL32();

    }

  }
  else{
    node->left()->accept(this, lvl + 2);
    if(node->is_typed(cdk::TYPE_DOUBLE) && node->left()->is_typed(cdk::TYPE_INT)) {
      _pf.I2D();
    }
    /*
    else if(node->is_typed(cdk::TYPE_POINTER) && node->left()->is_typed(cdk::TYPE_INT)) {
      if(cdk::reference_type_cast(node->type())->referenced()->name() == cdk::TYPE_DOUBLE) {
        _pf.INT(3);
      }
      else {
        _pf.INT(2);
      }
      _pf.SHTL();
    }*/

    node->right()->accept(this, lvl + 2);
    if(node->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT)) {
      _pf.I2D();
    }
    else if(node->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_INT)) {
      auto ref = cdk::reference_type::cast(node->type());
      if(ref->referenced()->name() == cdk::TYPE_DOUBLE) {
        _pf.INT(3);
      }
      else {
        _pf.INT(2);
      }
      _pf.SHTL();
    }
    if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER)){
      int lbl1;

      _pf.SUB();
      auto ref = cdk::reference_type::cast(node->left()->type());
      _pf.INT(ref->referenced()->size());
      _pf.DIV();
      _pf.DUP32();
      _pf.INT(0);
      _pf.LT();
      _pf.JZ(mklbl(lbl1 = ++_lbl));
      _pf.NEG();
      _pf.ALIGN();
      _pf.LABEL(mklbl(lbl1));
    }
    else{
      if(node->is_typed(cdk::TYPE_DOUBLE)) {
        _pf.DSUB();
      }
      else {
        _pf.SUB();
      }
    }
  }
}
void udf::postfix_writer::do_mul_node(cdk::mul_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (node->type()->name() == cdk::TYPE_TENSOR) {
    _functions_to_declare.insert(
        node->left()->type()->name()  == cdk::TYPE_TENSOR &&
        node->right()->type()->name() == cdk::TYPE_TENSOR
        ? "tensor_mul" : "tensor_mul_scalar"
    );

    if (node->left()->type()->name() == cdk::TYPE_TENSOR &&
        node->right()->type()->name() == cdk::TYPE_TENSOR) {
      node->left()->accept(this, lvl+2);
      node->right()->accept(this, lvl+2);
      _pf.CALL("tensor_mul");
      _pf.TRASH(4 * 2);
      _pf.LDFVAL32();

    }
    else if (node->left()->type()->name() == cdk::TYPE_TENSOR) {
      node->right()->accept(this, lvl+2);
      if (node->right()->type()->name() == cdk::TYPE_INT) _pf.I2D();
      node->left()->accept(this, lvl+2);
      _pf.CALL("tensor_mul_scalar");
      _pf.TRASH(4 + 8);
      _pf.LDFVAL32();

    }
    else {
      node->left()->accept(this, lvl+2);
      if (node->left()->type()->name() == cdk::TYPE_INT) _pf.I2D();
      node->right()->accept(this, lvl+2);
      _pf.CALL("tensor_mul_scalar");
      _pf.TRASH(4 + 8);
      _pf.LDFVAL32();
    }

  }
  else{
    node->left()->accept(this, lvl + 2);
    if (node->type()->name() == cdk::TYPE_DOUBLE && node->left()->type()->name() == cdk::TYPE_INT) _pf.I2D();

    node->right()->accept(this, lvl + 2);
    if (node->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_INT) _pf.I2D();

    if (node->type()->name() == cdk::TYPE_DOUBLE)
      _pf.DMUL();
    else
      _pf.MUL();
  }
}
void udf::postfix_writer::do_div_node(cdk::div_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (node->type()->name() == cdk::TYPE_TENSOR) {

    if (node->left()->type()->name() == cdk::TYPE_TENSOR &&
        node->right()->type()->name() == cdk::TYPE_TENSOR) {
      _functions_to_declare.insert("tensor_div");
      node->left()->accept(this, lvl+2);
      node->right()->accept(this, lvl+2);
      _pf.CALL("tensor_div");
      _pf.TRASH(4 * 2);
      _pf.LDFVAL32();
    }
    else if (node->left()->type()->name() == cdk::TYPE_TENSOR) {
      _functions_to_declare.insert("tensor_div");
      node->right()->accept(this, lvl+2);
      if (node->right()->type()->name() == cdk::TYPE_INT) _pf.I2D();
      node->left()->accept(this, lvl+2);
      _pf.CALL("tensor_div_scalar");
      _pf.TRASH(4 + 8);
      _pf.LDFVAL32();
    }
    else {
      node->left()->accept(this, lvl + 2);
      if (node->left()->type()->name() == cdk::TYPE_INT) _pf.I2D();
      _pf.DOUBLE(-1.0);
      node->right()->accept(this, lvl + 2);
      _functions_to_declare.insert("tensor_mul_scalar");
      _pf.CALL("tensor_mul_scalar");
      _pf.TRASH(4 + 8);
      _pf.LDFVAL32();
      _functions_to_declare.insert("tensor_add_scalar");
      _pf.CALL("tensor_add_scalar");
      _pf.TRASH(4 + 8);
      _pf.LDFVAL32();

    }

  }

  else{
    node->left()->accept(this, lvl + 2);
    if (node->type()->name() == cdk::TYPE_DOUBLE && node->left()->type()->name() == cdk::TYPE_INT) _pf.I2D();

    node->right()->accept(this, lvl + 2);
    if (node->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_INT) _pf.I2D();

    if (node->type()->name() == cdk::TYPE_DOUBLE)
      _pf.DDIV();
    else
      _pf.DIV();
  }
    
}
void udf::postfix_writer::do_mod_node(cdk::mod_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.MOD();
}
void udf::postfix_writer::do_lt_node(cdk::lt_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if(node->type()->name() == cdk::TYPE_DOUBLE && node->left()->type()->name() == cdk::TYPE_INT) {
    _pf.I2D();
  }

  node->right()->accept(this, lvl);
  if(node->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_INT) {
    _pf.I2D();
  }

  if(node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DCMP(); // double compare - returns -1, 0, 1
    _pf.INT(0); 
  }
  _pf.LT();
}

void udf::postfix_writer::do_le_node(cdk::le_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if(node->type()->name() == cdk::TYPE_DOUBLE && node->left()->type()->name() == cdk::TYPE_INT) {
    _pf.I2D();
  }

  node->right()->accept(this, lvl);
  if(node->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_INT) {
    _pf.I2D();
  }

  if(node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DCMP(); // double compare - returns -1, 0, 1
    _pf.INT(0); 
  }
  _pf.LE();
}

void udf::postfix_writer::do_ge_node(cdk::ge_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if(node->type()->name() == cdk::TYPE_DOUBLE && node->left()->type()->name() == cdk::TYPE_INT) {
    _pf.I2D();
  }

  node->right()->accept(this, lvl);
  if(node->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_INT) {
    _pf.I2D();
  }

  if(node->left()->type()->name() == cdk::TYPE_DOUBLE || node->right()->type()->name() == cdk::TYPE_DOUBLE) {
    _pf.DCMP(); // double compare - returns -1, 0, 1
    _pf.INT(0); 
  }
  _pf.GE();
}

void udf::postfix_writer::do_gt_node(cdk::gt_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  if(node->type()->name() == cdk::TYPE_DOUBLE && node->left()->type()->name() == cdk::TYPE_INT) {
    _pf.I2D();
  }

  node->right()->accept(this, lvl);
  if(node->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_INT) {
    _pf.I2D();
  }

  if(node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DCMP(); // double compare - returns -1, 0, 1
    _pf.INT(0); 
  }
  _pf.GT();
}

void udf::postfix_writer::do_ne_node(cdk::ne_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if(node->type()->name() == cdk::TYPE_TENSOR){
    node->left()->accept(this, lvl + 2);
    node->right()->accept(this, lvl + 2);
    _functions_to_declare.insert("tensor_equals");
    _pf.CALL("tensor_equals");
    _pf.TRASH(4 * 2);
    _pf.LDFVAL32();
    _pf.INT(1);
    _pf.NE();

  }
  else{
    node->left()->accept(this, lvl);
  if(node->type()->name() == cdk::TYPE_DOUBLE && node->left()->type()->name() == cdk::TYPE_INT) {
    _pf.I2D();
  }

  node->right()->accept(this, lvl);
  if(node->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_INT) {
    _pf.I2D();
  }

  if(node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DCMP(); // double compare - returns -1, 0, 1
    _pf.INT(0); 
  }
  _pf.NE();
  }
  
}

void udf::postfix_writer::do_eq_node(cdk::eq_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if(node->type()->name() == cdk::TYPE_TENSOR){
    node->left()->accept(this, lvl + 2);
    node->right()->accept(this, lvl + 2);
    _functions_to_declare.insert("tensor_equals");
    _pf.CALL("tensor_equals");
    _pf.TRASH(4 * 2);
    _pf.LDFVAL32();

  }
  else{
    node->left()->accept(this, lvl);
    if(node->type()->name() == cdk::TYPE_DOUBLE && node->left()->type()->name() == cdk::TYPE_INT) {
      _pf.I2D();
    }

    node->right()->accept(this, lvl);
    if(node->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_INT) {
      _pf.I2D();
    }

    if(node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE)) {
      _pf.DCMP(); // double compare - returns -1, 0, 1
      _pf.INT(0); 
    }
    _pf.EQ();
  }
  
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_variable_node(cdk::variable_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  
  const std::string &id = node->name();
  auto symbol = _symtab.find(id);
  if (symbol->global()) {
    _pf.ADDR(symbol->name());
  } else {
    _pf.LOCAL(symbol->offset());
  }
}

void udf::postfix_writer::do_rvalue_node(cdk::rvalue_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->lvalue()->accept(this, lvl);
  if (node->type()->name() == cdk::TYPE_DOUBLE) {
    _pf.LDDOUBLE();
  } else {
    // integers, pointers, and strings
    _pf.LDINT();
  }
}

void udf::postfix_writer::do_assignment_node(cdk::assignment_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->rvalue()->accept(this, lvl); 
  if(node->is_typed(cdk::TYPE_DOUBLE)){
    if(node->rvalue()->is_typed(cdk::TYPE_INT))
      _pf.I2D();
    _pf.DUP64();
  }
  else
    _pf.DUP32();

  node->lvalue()->accept(this, lvl);
  if(node->lvalue()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.STDOUBLE();
  }
  else {
    _pf.STINT();
  }
}

//---------------------------------------------------------------------------
void udf::postfix_writer::do_evaluation_node(udf::evaluation_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl);
  if (node->argument()->type()->name() == cdk::TYPE_INT ||
      node->argument()->type()->name() == cdk::TYPE_STRING ||
      node->argument()->type()->name() == cdk::TYPE_POINTER ||
      node->argument()->type()->name() == cdk::TYPE_TENSOR) {
    _pf.TRASH(4);
  } else if (node->argument()->type()->name() == cdk::TYPE_DOUBLE) {
    _pf.TRASH(8);
  } else if(node->argument()->type()->name() != cdk::TYPE_VOID){
    std::cerr << "ERROR: invalid type" << std::endl;
    exit(1);
  }
}


void udf::postfix_writer::do_write_node(udf::write_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  for (size_t ix = 0; ix < node->arguments()->size(); ix++) {
    auto child = dynamic_cast<cdk::expression_node*>(node->arguments()->node(ix));

    std::shared_ptr<cdk::basic_type> etype = child->type();
    child->accept(this, lvl); // expression to print
    if (etype->name() == cdk::TYPE_INT) {
      _functions_to_declare.insert("printi");
      _pf.CALL("printi");
      _pf.TRASH(4); // trash int
    } else if (etype->name() == cdk::TYPE_DOUBLE) {
      _functions_to_declare.insert("printd");
      _pf.CALL("printd");
      _pf.TRASH(8); // trash double
    } else if (etype->name() == cdk::TYPE_STRING) {
      _functions_to_declare.insert("prints");
      _pf.CALL("prints");
      _pf.TRASH(4); // trash char pointer
    } else if (etype->name() == cdk::TYPE_TENSOR) {
      _functions_to_declare.insert("tensor_print");
      _pf.CALL("tensor_print");
      _pf.TRASH(4); // trash tensor pointer
    } else {
      std::cerr << "cannot print expression of unknown type" << std::endl;
      return;
    }
  }

  if (node->newline()) {
    _functions_to_declare.insert("println");
    _pf.CALL("println");
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_input_node(udf::input_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if (node->type()->name() == cdk::TYPE_DOUBLE) {
    _functions_to_declare.insert("readd");
    _pf.CALL("readd");
    _pf.LDFVAL64();
  } else if (node->type()->name() == cdk::TYPE_INT) {
    _functions_to_declare.insert("readi");
    _pf.CALL("readi");
    _pf.LDFVAL32();
  } else {
    std::cerr << "FATAL: " << node->lineno() << ": cannot read type" << std::endl;
    return;
  }
}

//---------------------------------------------------------------------------
void udf::postfix_writer::do_for_node(udf::for_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl1 = ++_lbl; 
  _forIni.push(lbl1);
  int lbl2 = ++_lbl; 
  _forStep.push(lbl2);
  int lbl3 = ++_lbl; 
  _forEnd.push(lbl3);

  int i;
  int size = 0;
  cdk::expression_node *expNode, *lastCondition;

  _symtab.push();
 
  if(node->init()) {
    node->init()->accept(this,lvl+2);
  }
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl1));

  if(node->condition()) {
    int last = node->condition()->size()-1;
    lastCondition = dynamic_cast<cdk::expression_node *>(node->condition()->node(last));
    node->condition()->accept(this, lvl); 
    if(lastCondition->is_typed(cdk::TYPE_DOUBLE)) {
      _pf.D2I();
    }
  }

  _pf.JZ(mklbl(lbl3));
  node->instruction()->accept(this, lvl + 2);
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl2));

  if(node->step()) {
    node->step()->accept(this, lvl);
  }
  _pf.JMP(mklbl(lbl1));

  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl3));

  if(node->init()) {
    for(i = 0; i < (int)(node->init()->size()); i++) {
      expNode = dynamic_cast<cdk::expression_node *>(node->init()->node(i));
      if(expNode) {
        if(expNode->is_typed(cdk::TYPE_INT) || expNode->is_typed(cdk::TYPE_POINTER)) {
          size =+ 4;
        }
        if(expNode->is_typed(cdk::TYPE_DOUBLE)) {
          size =+ 8;
        }
      }
    }
  }

   if(node->condition()) {
    for(i = 0; i < (int)(node->condition()->size()-1); i++) {
      expNode = dynamic_cast<cdk::expression_node *>(node->condition()->node(i));
      if(expNode->is_typed(cdk::TYPE_INT) || expNode->is_typed(cdk::TYPE_POINTER)) {
          size =+ 4;
        }
      if(expNode->is_typed(cdk::TYPE_DOUBLE)) {
        size =+ 8;
      }
    }
  }

   if(node->step()) {
    for(i = 0; i < (int)(node->step()->size()); i++) {
      expNode = dynamic_cast<cdk::expression_node *>(node->step()->node(i));
      if(expNode->is_typed(cdk::TYPE_INT) || expNode->is_typed(cdk::TYPE_POINTER)) {
          size =+ 4;
        }
      if(expNode->is_typed(cdk::TYPE_DOUBLE)) {
        size =+ 8;
      }
    }
  }

  _symtab.pop();
  _pf.TRASH(size);
  _forIni.pop();
  _forStep.pop();
  _forEnd.pop();
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_if_node(udf::if_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl1 = ++_lbl;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl1));
  node->block()->accept(this, lvl + 2);
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl1));
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_if_else_node(udf::if_else_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl1 = ++_lbl;
  int lbl2 = ++_lbl;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl1));
 
  node->thenblock()->accept(this, lvl + 2);
  _pf.JMP(mklbl(lbl2));
  
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl1));
  node->elseblock()->accept(this, lvl + 2);

  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl2));
}

//---------------------------------------------------------------------------
void udf::postfix_writer::do_continue_node(udf::continue_node * const node, int lvl) {
  if(_forStep.size() > 0)
    _pf.JMP(mklbl(_forStep.top()));
  else
    std::cerr << "Continue instruction used outside for cycle." << std::endl;
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_break_node(udf::break_node * const node, int lvl) {
  if(_forEnd.size() > 0)
    _pf.JMP(mklbl(_forEnd.top()));
  else
    std::cerr << "Break instruction used outside for cycle." << std::endl;
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_function_call_node(udf::function_call_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;



  auto symbol = _symtab.find(node->identifier());

  size_t argumentsSize = 0;
  if(node->arguments()->size() > 0) {
    for(int i = node->arguments()->size() - 1; i >= 0; i--) {
      cdk::expression_node *arg = dynamic_cast<cdk::expression_node*>(node->arguments()->node(i));
      
      arg->accept(this, lvl + 2);
      
      if (symbol->argument_is_typed(i, cdk::TYPE_DOUBLE) && arg->is_typed(cdk::TYPE_INT)) {
        _pf.I2D();
      }
      argumentsSize += symbol->argument_size(i);
    }
  }
  _pf.CALL(node->identifier());
  if(argumentsSize != 0) _pf.TRASH(argumentsSize);

  if(symbol->type()->name() == cdk::TYPE_INT || symbol->type()->name() == cdk::TYPE_POINTER || 
    symbol->type()->name() == cdk::TYPE_STRING || symbol->type()->name() == cdk::TYPE_TENSOR) {
    _pf.LDFVAL32();
  }
  else if(symbol->type()->name() == cdk::TYPE_DOUBLE) {
    _pf.LDFVAL64();
  }
  else if (symbol->type()->name() != cdk::TYPE_VOID) {
    std::cerr << "Cannot call function '" + node->identifier() + "'" << std::endl;
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_return_node(udf::return_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if (_function->type()->name() != cdk::TYPE_VOID) {
    node->retval()->accept(this, lvl + 2);

    if (_function->type()->name() == cdk::TYPE_INT || _function->type()->name() == cdk::TYPE_STRING
        || _function->type()->name() == cdk::TYPE_POINTER || _function->type()->name() == cdk::TYPE_TENSOR) {
      _pf.STFVAL32();
    } else if (_function->type()->name() == cdk::TYPE_DOUBLE) {
      if (node->retval()->type()->name() == cdk::TYPE_INT) _pf.I2D();
      _pf.STFVAL64();
    }  else {
      std::cerr << node->lineno() << ": unknown return type" << std::endl;
    }
  }

  _pf.JMP(_currentBodyRetLabel);
  //_returnSeen = true; TODO
}

//---------------------------------------------------------------------------
void udf::postfix_writer::do_block_node(udf::block_node * const node, int lvl) {
  _symtab.push(); // for block-local vars
  if (node->declarations()) node->declarations()->accept(this, lvl + 2);
  if (node->instructions()) node->instructions()->accept(this, lvl + 2);
  _symtab.pop();
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_sizeof_node(udf::sizeof_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
 _pf.INT(node->expression()->type()->size());
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_variable_declaration_node(udf::variable_declaration_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  auto id = node->identifier();

  int offset = 0, typesize = node->type()->size();
  if(_inFunctionBody) {
    _offset -= typesize;
    offset = _offset;
  }
  else if(_inFunctionArgs) {
    offset = _offset;
    _offset += typesize;
  }
  else
    offset = 0;

  auto symbol = new_symbol();
  if (symbol) {
    symbol->set_offset(offset);
    reset_new_symbol();
  }

  if (_inFunctionBody) {
    // if we are dealing with local variables, then no action is needed
    // unless an initializer exists
    if (node->initializer()) {
      node->initializer()->accept(this, lvl);
      if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_STRING) || node->is_typed(cdk::TYPE_POINTER) || node->is_typed(cdk::TYPE_TENSOR)) {
        _pf.LOCAL(symbol->offset());
        _pf.STINT();
      } else if (node->is_typed(cdk::TYPE_DOUBLE)) {
        if (node->initializer()->is_typed(cdk::TYPE_INT))
          _pf.I2D();
        _pf.LOCAL(symbol->offset());
        _pf.STDOUBLE();
      } else {
        std::cerr << "cannot initialize" << std::endl;
      }
    }
  } else {
    if (!_function) {
      if (!node->initializer()) {
        _pf.BSS();
        _pf.ALIGN();
        _pf.LABEL(id);
        _pf.SALLOC(typesize);
      } else {
        if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_DOUBLE) || node->is_typed(cdk::TYPE_POINTER)) {
          _pf.DATA();
          _pf.ALIGN();
          if (node->qualifier() == tPUBLIC) {
            _pf.GLOBAL(id, _pf.OBJ());
          }
          _pf.LABEL(id);

          if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_POINTER)) {
            node->initializer()->accept(this, lvl);
          } else if (node->is_typed(cdk::TYPE_DOUBLE)) {
            if (node->initializer()->is_typed(cdk::TYPE_DOUBLE)) {
              node->initializer()->accept(this, lvl);
            } else if (node->initializer()->is_typed(cdk::TYPE_INT)) {
              cdk::integer_node *dclini = dynamic_cast<cdk::integer_node*>(node->initializer());
              cdk::double_node ddi(dclini->lineno(), dclini->value());
              ddi.accept(this, lvl);
            } else {
              std::cerr << node->lineno() << ": '" << id << "' has bad initializer for real value\n";
            }
          }
          else if(node->is_typed(cdk::TYPE_TENSOR)) {
            node->initializer()->accept(this, lvl);
          }
        } else if (node->is_typed(cdk::TYPE_STRING)) {
          _pf.DATA();
          _pf.ALIGN();
          _pf.LABEL(id);
          node->initializer()->accept(this, lvl);
        } else {
          std::cerr << node->lineno() << ": '" << id << "' has unexpected initializer\n";
        }
      }
    }
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_address_of_node(udf::address_of_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS
  node->argument()->accept(this, lvl + 2);
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_objects_node(udf::objects_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl);
  auto ref = cdk::reference_type::cast(node->type());
  if(ref->referenced()->name() == cdk::TYPE_DOUBLE)
    _pf.INT(3);
  else
    _pf.INT(2);

  _pf.SHTL();
  _pf.ALLOC();
  _pf.SP();
}

//---------------------------------------------------------------------------
void udf::postfix_writer::do_index_ptr_node(udf::index_ptr_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->base()->accept(this, lvl);
  node->index()->accept(this, lvl);
  _pf.INT(node->type()->size());
  _pf.MUL();
  _pf.ADD();
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_nullptr_node(udf::nullptr_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (_inFunctionBody) {
    _pf.INT(0);
  } else {
    _pf.SINT(0);
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_function_declaration_node(udf::function_declaration_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if (_inFunctionBody || _inFunctionArgs) {
    std::cerr << node->lineno() <<"cannot declare function in body or in args\n" << std::endl;
    return;
  }

  if (!new_symbol()) return;

  auto function = new_symbol();
  _functions_to_declare.insert(function->name());
  reset_new_symbol();
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_function_definition_node(udf::function_definition_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if (_inFunctionBody || _inFunctionArgs) {
    std::cerr << node->lineno() << ": cannot define function in body or in arguments\n";
    return;
  }

  _function = new_symbol();
  _functions_to_declare.erase(_function->name());  // just in case
  reset_new_symbol();

  _currentBodyRetLabel = mklbl(++_lbl);

  _offset = 8; // prepare for arguments (4: remember to account for return address)
  _symtab.push(); // scope of args
  
  if (node->arguments()->size() > 0) {
    _inFunctionArgs = true; //TODO really needed?
    node->arguments()->accept(this, lvl + 2);
    _inFunctionArgs = false; //TODO really needed?
  }

  _pf.TEXT();
  _pf.ALIGN();
  if (node->qualifier() == tPUBLIC) _pf.GLOBAL(_function->name(), _pf.FUNC());
  _pf.LABEL(_function->name());

  // compute stack size to be reserved for local variables
  frame_size_calculator lsc(_compiler, _symtab, _function);
  node->accept(&lsc, lvl);
  _pf.ENTER(lsc.localsize()); // total stack size reserved for local variables

  _offset = 0; // prepare for local variable

  // the following flag is a slight hack: it won't work with nested functions
  _inFunctionBody = true;
  node->block()->accept(this, lvl + 4); // block has its own scope
  _inFunctionBody = false;
  // _returnSeen = false; TODO - what is this?
  
  _pf.LABEL(_currentBodyRetLabel);
  _pf.LEAVE();
  _pf.RET();

  _symtab.pop(); // scope of arguments

  if (node->identifier() == "udf") {
    // declare external functions
    for (std::string s : _functions_to_declare)
      _pf.EXTERN(s);
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_tensor_node(udf::tensor_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  _functions_to_declare.insert("tensor_create");
  _functions_to_declare.insert("tensor_set");

  auto tensor_type = dynamic_cast<cdk::tensor_type*>(node);
  const auto &dims = tensor_type->dims();
  size_t n_dims = tensor_type->n_dims();
  size_t n_elements = tensor_type->size() / 8;

  for (auto it = dims.rbegin(); it != dims.rend(); ++it) // place dims in the stack in reverse order
    _pf.INT(*it);

  _pf.INT(n_dims);

  _pf.CALL("tensor_create");
  _pf.TRASH(n_dims * 4 + 4);
  _pf.LDFVAL32();
  
  for (size_t i = 0; i < n_elements; ++i) {
    size_t remainder = i;
    std::vector<size_t> indices(n_dims);
    for (int d_ix = n_dims - 1; d_ix >= 0; --d_ix) {
      indices[d_ix] = remainder % dims[d_ix];
      remainder /= dims[d_ix];
    }
    
    _pf.DUP32();

    for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
      _pf.INT(*it);
      _pf.SWAP32();
    }

    node->value(i)->accept(this, lvl); 
    
    if(node->value(i)->type()->name() == cdk::TYPE_INT)
      _pf.I2D();

    _pf.SWAP32();

    _pf.CALL("tensor_set");

    _pf.TRASH(n_dims * 4 + 8 + 4);
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_index_tensor_node(udf::index_tensor_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  _functions_to_declare.insert("tensor_get");

  node->base()->accept(this, lvl);

  _pf.LDFVAL32(); 
  _pf.DUP32();
  _pf.LOCAL(-4);
  _pf.STINT();

  auto tensor_type = cdk::tensor_type::cast(node->base()->type());
  const auto &dims = tensor_type->dims();
  size_t n_dims = dims.size();

  if (node->index()->size() != n_dims) {
    throw std::runtime_error("Invalid number of indices for tensor access.");
  }

  int ok_lbl = ++_lbl;
  int err_lbl = ++_lbl;

  for (int i = n_dims - 1; i >= 0; --i) {
    auto index_expr = dynamic_cast<cdk::expression_node *>(node->index()->node(i));
    index_expr->accept(this, lvl);

    _pf.DUP32();
    _pf.INT(dims[i]);
    _pf.GE();
    _pf.JNZ(mklbl(err_lbl));
  }

  _pf.LOCAL(-4);
  _pf.LDINT();

  _pf.CALL("tensor_get");

  _pf.TRASH(4 * n_dims + 4);

  _pf.JMP(mklbl(ok_lbl));

  _pf.ALIGN();
  _pf.LABEL(mklbl(err_lbl));
  _pf.RODATA();
  _pf.LABEL(mklbl(_lbl));
  _pf.SSTRING("Index out of bounds.\n");
  _pf.TEXT();
  _pf.ADDR(mklbl(_lbl));
  _pf.CALL("puts");
  _pf.CALL("abort");

  _pf.ALIGN();
  _pf.LABEL(mklbl(ok_lbl));
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_tensor_reshape_node(udf::tensor_reshape_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  size_t new_n_dims = node->dims().size();
  for (auto it = node->dims().rbegin(); it != node->dims().rend(); ++it) _pf.INT(*it);     
  _pf.INT(new_n_dims);
  _functions_to_declare.insert("tensor_reshape");
  node->tensor()->accept(this, lvl); 

  _pf.CALL("tensor_reshape");

  _pf.TRASH(4 + 4 * new_n_dims);
  _pf.LDFVAL32();

}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_tensor_contraction_node(udf::tensor_contraction_node * const node, int lvl) {
  int lbl_ok = ++_lbl;

  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);
  _functions_to_declare.insert("tensor_matmul");
  _pf.CALL("tensor_matmul");  
  _pf.TRASH(4 * 2);
  _pf.LDFVAL32();
  _pf.DUP32();
  _pf.JNZ(mklbl(lbl_ok));
  _functions_to_declare.insert("abort");
  _pf.CALL("abort");
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl_ok));
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_tensor_dims_node(udf::tensor_dims_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  _functions_to_declare.insert("tensor_get_dims");
  int lbl_ok = ++_lbl;
  
  node->tensor()->accept(this, lvl);

  _pf.CALL("tensor_get_dims");
  _pf.TRASH(4);
  _pf.LDFVAL32();
  _pf.DUP32();                
  _pf.JNZ(mklbl(lbl_ok));      
  _functions_to_declare.insert("abort");
  _pf.CALL("abort");           
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl_ok));
  _pf.TRASH(4);

}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_tensor_dim_node(udf::tensor_dim_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->idx()->accept(this, lvl + 2);     
  node->tensor()->accept(this, lvl + 2); 

  _functions_to_declare.insert("tensor_get_dim_size");
  _pf.CALL("tensor_get_dim_size");        
  _pf.TRASH(4 * 2);
  _pf.LDFVAL32();
  
  int lbl_ok = ++_lbl;
  _pf.DUP32();                         
  _pf.INT(-1);                           
  _pf.EQ();                               
  _pf.JZ(mklbl(lbl_ok));                  
  
  _functions_to_declare.insert("abort");
  _pf.CALL("abort");                     

  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl_ok));              

  _pf.TRASH(4);                           
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_tensor_capacity_node(udf::tensor_capacity_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->tensor()->accept(this, lvl);
  _functions_to_declare.insert("tensor_size");
  
  _pf.CALL("tensor_size");
  _pf.TRASH(4);
  _pf.LDFVAL32();
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_tensor_rank_node(udf::tensor_rank_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->tensor()->accept(this, lvl);
  _functions_to_declare.insert("tensor_get_n_dims");
  
  _pf.CALL("tensor_get_n_dims");
  _pf.TRASH(4);
  _pf.LDFVAL32();
}
