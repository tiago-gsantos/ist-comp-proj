#pragma once

#include <string>
#include <cdk/ast/typed_node.h>
#include <cdk/ast/sequence_node.h>
#include <cdk/types/basic_type.h>

namespace udf {

  /**
   * Class for describing function declaration nodes.
   */
  class function_declaration_node: public cdk::typed_node {
    int _qualifier;
    std::string _identifier;
    cdk::sequence_node *_arguments;

  public:
    function_declaration_node(int lineno, int qualifier, const std::string &identifier, cdk::sequence_node *arguments) :
        cdk::typed_node(lineno), _qualifier(qualifier), _identifier(identifier), _arguments(arguments) {
    }

    function_declaration_node(int lineno, int qualifier, std::shared_ptr<cdk::basic_type> funType, const std::string &identifier,
                              cdk::sequence_node *arguments) :
        cdk::typed_node(lineno), _qualifier(qualifier), _identifier(identifier), _arguments(arguments) {
      type(funType);
    }

    int qualifier() { return _qualifier; }
    
    const std::string& identifier() const { return _identifier; }

    cdk::typed_node* argument(size_t ax) { 
      return dynamic_cast<cdk::typed_node*>(_arguments->node(ax));
    }

    cdk::sequence_node* arguments() { return _arguments; }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_function_declaration_node(this, level);
    }

  };

} // udf
