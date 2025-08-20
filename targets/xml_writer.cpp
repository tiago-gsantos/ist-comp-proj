#include ".auto/all_nodes.h"
#include <string>
#include <iostream>
#include <memory>
#include <cdk/types/types.h>
#include "targets/type_checker.h"
#include "targets/xml_writer.h"
#include "targets/symbol.h"
#include "udf_parser.tab.h"

static std::string qualifier_name(int qualifier) {
  if (qualifier == tPUBLIC) return "public";
  if (qualifier == tPRIVATE) return "private";
  if (qualifier == tFORWARD) return "forward";
  else
    return "unknown qualifier";
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_data_node(cdk::data_node *const node, int lvl) {
  // EMPTY
}

void udf::xml_writer::do_nil_node(cdk::nil_node *const node, int lvl) {
  // EMPTY
}

void udf::xml_writer::do_sequence_node(cdk::sequence_node *const node, int lvl) {
  os() << std::string(lvl, ' ') << "<" << node->label() << " size='" << node->size() << "'>" << std::endl;
  for (size_t i = 0; i < node->size(); i++) {
    cdk::basic_node *n = node->node(i);
    if (n == NULL) break;
    n->accept(this, lvl + 2);
  }
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_double_node(cdk::double_node *const node, int lvl) {
  process_literal(node, lvl);
}

void udf::xml_writer::do_integer_node(cdk::integer_node *const node, int lvl) {
  process_literal(node, lvl);
}

void udf::xml_writer::do_string_node(cdk::string_node *const node, int lvl) {
  process_literal(node, lvl);
}

//---------------------------------------------------------------------------
//protected:
void udf::xml_writer::do_unary_operation(cdk::unary_operation_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  node->argument()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_binary_operation(cdk::binary_operation_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_unary_minus_node(cdk::unary_minus_node *const node, int lvl) {
  do_unary_operation(node, lvl);
}

void udf::xml_writer::do_unary_plus_node(cdk::unary_plus_node *const node, int lvl) {
  do_unary_operation(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_add_node(cdk::add_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}
void udf::xml_writer::do_sub_node(cdk::sub_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}
void udf::xml_writer::do_div_node(cdk::div_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}
void udf::xml_writer::do_mod_node(cdk::mod_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}
void udf::xml_writer::do_mul_node(cdk::mul_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}
void udf::xml_writer::do_eq_node(cdk::eq_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}
void udf::xml_writer::do_ge_node(cdk::ge_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}
void udf::xml_writer::do_gt_node(cdk::gt_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}
void udf::xml_writer::do_le_node(cdk::le_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}
void udf::xml_writer::do_lt_node(cdk::lt_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}
void udf::xml_writer::do_ne_node(cdk::ne_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}

//===========================================================================

void udf::xml_writer::do_variable_node(cdk::variable_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  os() << std::string(lvl, ' ') << "<" << node->label() << ">" << node->name() << "</" << node->label() << ">" << std::endl;
}

void udf::xml_writer::do_rvalue_node(cdk::rvalue_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  node->lvalue()->accept(this, lvl + 4);
  closeTag(node, lvl);
}

void udf::xml_writer::do_assignment_node(cdk::assignment_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  node->lvalue()->accept(this, lvl + 2);
  node->rvalue()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

//===========================================================================

void udf::xml_writer::do_or_node(cdk::or_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}
void udf::xml_writer::do_and_node(cdk::and_node *const node, int lvl) {
  do_binary_operation(node, lvl);
}
void udf::xml_writer::do_not_node(cdk::not_node *const node, int lvl) {
  do_unary_operation(node, lvl);
}

void udf::xml_writer::do_write_node(udf::write_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  node->arguments()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_input_node(udf::input_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_evaluation_node(udf::evaluation_node *const node, int lvl) {
  openTag(node, lvl);
  node->argument()->accept(this, lvl);
  closeTag(node, lvl);
}

void udf::xml_writer::do_block_node(udf::block_node *const node, int lvl) {
  openTag(node, lvl);
  openTag("declarations", lvl);
  if (node->declarations()) {
    node->declarations()->accept(this, lvl + 4);
  }
  closeTag("declarations", lvl);
  openTag("instructions", lvl);
  if (node->instructions()) {
    node->instructions()->accept(this, lvl + 4);
  }
  closeTag("instructions", lvl);
  closeTag(node, lvl);
}

void udf::xml_writer::do_index_ptr_node(udf::index_ptr_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  openTag("base", lvl + 2);
  node->base()->accept(this, lvl + 4);
  closeTag("base", lvl + 2);
  openTag("index", lvl + 2);
  node->index()->accept(this, lvl + 4);
  closeTag("index", lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_index_tensor_node(udf::index_tensor_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  openTag("base", lvl + 2);
  node->base()->accept(this, lvl + 4);
  closeTag("base", lvl + 2);
  openTag("index", lvl + 2);
  node->index()->accept(this, lvl + 4);
  closeTag("index", lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_address_of_node(udf::address_of_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  node->argument()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_objects_node(udf::objects_node *const node, int lvl) {
  do_unary_operation(node, lvl);
}

void udf::xml_writer::do_nullptr_node(udf::nullptr_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS
  openTag(node, lvl);
  closeTag(node, lvl);
}


void udf::xml_writer::do_for_node(udf::for_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  openTag("init", lvl);
  node->init()->accept(this, lvl + 4);
  closeTag("init", lvl);
  openTag("condition", lvl);
  node->condition()->accept(this, lvl + 4);
  closeTag("condition", lvl);
  openTag("step", lvl);
  node->step()->accept(this, lvl + 4);
  closeTag("step", lvl);
  openTag("instruction", lvl);
  node->instruction()->accept(this, lvl + 4);
  closeTag("instruction", lvl);
  closeTag(node, lvl);
}

void udf::xml_writer::do_break_node(udf::break_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  closeTag(node, lvl);
}

void udf::xml_writer::do_continue_node(udf::continue_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_if_node(udf::if_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  openTag("condition", lvl + 2);
  node->condition()->accept(this, lvl + 4);
  closeTag("condition", lvl + 2);
  openTag("block", lvl + 2);
  node->block()->accept(this, lvl + 4);
  closeTag("block", lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_if_else_node(udf::if_else_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  openTag("condition", lvl + 2);
  node->condition()->accept(this, lvl + 4);
  closeTag("condition", lvl + 2);
  openTag("block", lvl + 2);
  node->thenblock()->accept(this, lvl + 4);
  closeTag("block", lvl + 2);
  if (node->elseblock()) {
    openTag("else", lvl + 2);
    node->elseblock()->accept(this, lvl + 4);
    closeTag("else", lvl + 2);
  }
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_function_call_node(udf::function_call_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  os() << std::string(lvl, ' ') << "<" << node->label() << " name='" << node->identifier() << "'>" << std::endl;
  openTag("arguments", lvl + 2);
  if (node->arguments()) node->arguments()->accept(this, lvl + 4);
  closeTag("arguments", lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_function_declaration_node(udf::function_declaration_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  os() << std::string(lvl, ' ') << "<" << node->label() << " name='" << node->identifier() << "' qualifier='"
      << qualifier_name(node->qualifier()) << "' type='" << cdk::to_string(node->type()) << "'>" << std::endl;

  openTag("arguments", lvl + 2);
  if (node->arguments()) {
    _symtab.push();
    node->arguments()->accept(this, lvl + 4);
    _symtab.pop();
  }
  closeTag("arguments", lvl + 2);
  closeTag(node, lvl);
}

void udf::xml_writer::do_function_definition_node(udf::function_definition_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  _symtab.push(); // scope of args

  os() << std::string(lvl, ' ') << "<" << node->label() << " name='" << node->identifier() << "' qualifier='"
      << qualifier_name(node->qualifier()) << "' type='" << cdk::to_string(node->type()) << "'>" << std::endl;

  openTag("arguments", lvl + 2);
  if (node->arguments()) {
    node->arguments()->accept(this, lvl + 4);
  }
  closeTag("arguments", lvl + 2);
  node->block()->accept(this, lvl + 2);
  closeTag(node, lvl);

  _symtab.pop(); // scope of args
}

void udf::xml_writer::do_return_node(udf::return_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  if (node->retval()) node->retval()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_variable_declaration_node(udf::variable_declaration_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  os() << std::string(lvl, ' ') << "<" << node->label() << " name='" << node->identifier() << "' qualifier='"
      << qualifier_name(node->qualifier()) << "' type='" << cdk::to_string(node->type()) << "'>" << std::endl;

  if (node->initializer()) {
    openTag("initializer", lvl + 2);
    node->initializer()->accept(this, lvl + 4);
    closeTag("initializer", lvl + 2);
  }
  closeTag(node, lvl);
}


//---------------------------------------------------------------------------

void udf::xml_writer::do_tensor_node(udf::tensor_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS
  openTag(node, lvl);
  openTag("values", lvl + 2);
  if(node->values())node->values()->accept(this, lvl + 4);
  closeTag("values", lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_tensor_capacity_node(udf::tensor_capacity_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS
  openTag(node, lvl);
  node->tensor()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------


void udf::xml_writer::do_tensor_dim_node(udf::tensor_dim_node *const node, int lvl) {
  openTag(node, lvl);
  openTag("tensor", lvl + 2);
  node->tensor()->accept(this, lvl + 4);
  closeTag("tensor", lvl + 2);
  openTag("index", lvl + 2);
  node->idx()->accept(this, lvl + 4);
  closeTag("index", lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_tensor_dims_node(udf::tensor_dims_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS
  openTag(node, lvl);
  node->tensor()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_tensor_rank_node(udf::tensor_rank_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS
  openTag(node, lvl);
  node->tensor()->accept(this, lvl + 2);
  closeTag(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_tensor_contraction_node(udf::tensor_contraction_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS
  do_binary_operation(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_tensor_reshape_node(udf::tensor_reshape_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS
  openTag(node, lvl);
  openTag("tensor", lvl + 2);
  node->tensor()->accept(this, lvl + 4);
  closeTag("tensor", lvl + 2);

  os() << std::string(lvl + 2, ' ')
       << "<dimensions dims='";
  const auto &dims = node->dims();
  for (size_t i = 0; i < dims.size(); ++i) {
    os() << dims[i];
    if (i + 1 < dims.size()) os() << ",";
  }
  os() << "' size='" << node->size() << "'/>" << std::endl;

  closeTag(node, lvl);
}

//---------------------------------------------------------------------------

void udf::xml_writer::do_sizeof_node(udf::sizeof_node *const node, int lvl) {
  //ASSERT_SAFE_EXPRESSIONS;
  openTag(node, lvl);
  node->expression()->accept(this, lvl + 2);
  closeTag(node, lvl);
}