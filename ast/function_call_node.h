#pragma once

#include <string>
#include <cdk/ast/expression_node.h>
#include <cdk/ast/sequence_node.h>

namespace udf {

  /**
   * Class for describing function_call nodes.
   */
  class function_call_node: public cdk::expression_node {
    std::string _identifier;
    cdk::sequence_node *_arguments;

  public:
    /*
    Constructor for a function call without arguments.
    */
    function_call_node(int lineno, const std::string &identifier) :
        cdk::expression_node(lineno), _identifier(identifier), _arguments(new cdk::sequence_node(lineno)) {
    }

    /*
    Constructor for a function call with arguments.
    */
    function_call_node(int lineno, const std::string &identifier, cdk::sequence_node *arguments) :
        cdk::expression_node(lineno), _identifier(identifier), _arguments(arguments) {
    }

    const std::string& identifier() { return _identifier; }
    
    cdk::sequence_node* arguments() { return _arguments; }

    cdk::expression_node *argument(size_t ix) {
      return dynamic_cast<cdk::expression_node*>(_arguments->node(ix));
    }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_function_call_node(this, level);
    }

  };

} // udf
