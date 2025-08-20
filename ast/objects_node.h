#pragma once

#include <cdk/ast/unary_operation_node.h>
#include <cdk/ast/expression_node.h>

namespace udf {

  /**
   * Class for describing objects nodes.
   */
  class objects_node: public cdk::unary_operation_node {

  public:
    objects_node(int lineno, cdk::expression_node *argument) :
      cdk::unary_operation_node(lineno, argument) {
    }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_objects_node(this, level);
    }

  };

} // udf
