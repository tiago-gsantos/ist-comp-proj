#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

  /**
   * Class for describing tensor reshape nodes.
   */
  class tensor_reshape_node: public cdk::expression_node {
    cdk::expression_node *_tensor; 
    std::vector<size_t> _dims;

  public:
    tensor_reshape_node(int lineno, cdk::expression_node *tensor, const std::vector<size_t> &dims) :
        cdk::expression_node(lineno), _tensor(tensor), _dims(dims) {
    }
  
    cdk::expression_node *tensor() { return _tensor; }

    const std::vector<size_t> &dims() const {
      return _dims;
    }

    size_t size() const {
      size_t size = 0;
      if (_dims.size() >= 1) size = _dims.at(0);
      for (size_t ix = 1; ix < _dims.size(); ix++)
        size *= _dims.at(ix);
      return size;
    }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_tensor_reshape_node(this, level);
    }

  };

} // udf