#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/lvalue_node.h>

namespace udf {
  
  /**
   * Class for describing index_ptr nodes.
   */
  class index_ptr_node: public cdk::lvalue_node {
    cdk::expression_node *_base;
    cdk::expression_node *_index;

  public:
    index_ptr_node(int lineno, cdk::expression_node *base, cdk::expression_node *index) :
        cdk::lvalue_node(lineno), _base(base), _index(index) {
    }

    cdk::expression_node* base() { return _base; }

    cdk::expression_node* index() { return _index; }
    
    void accept(basic_ast_visitor *sp, int level) {
      sp->do_index_ptr_node(this, level);
    }

  };

} // udf
