#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

  /**
   * Class for describing tensor dim nodes.
   */
  class tensor_dim_node: public cdk::expression_node {
    cdk::expression_node *_tensor; 
    cdk::expression_node *_idx;
    
  public:
    tensor_dim_node(int lineno, cdk::expression_node *tensor, cdk::expression_node *idx) :
        cdk::expression_node(lineno), _tensor(tensor), _idx(idx) {
    }
  
    cdk::expression_node *tensor() {
       return _tensor; 
    }

    cdk::expression_node *idx() {
       return _idx; 
    }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_tensor_dim_node(this, level);
    }

  };

} // udf