#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/sequence_node.h>
#include <cdk/ast/lvalue_node.h>

namespace udf {
  
  /**
   * Class for describing index tensor nodes.
   */
  class index_tensor_node: public cdk::lvalue_node {
    cdk::expression_node *_base;
    cdk::sequence_node *_index;

  public:
    index_tensor_node(int lineno, cdk::expression_node *base, cdk::sequence_node *index) :
        cdk::lvalue_node(lineno), _base(base), _index(index) {
    }

    cdk::expression_node* base() { return _base; }
    
    cdk::sequence_node* index() { return _index; }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_index_tensor_node(this, level);
    }

  };

} // udf
