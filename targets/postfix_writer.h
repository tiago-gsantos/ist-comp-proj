#pragma once

#include "targets/basic_ast_visitor.h"
#include <stack>
#include <sstream>
#include <set>
#include <cdk/emitters/basic_postfix_emitter.h>

namespace udf {

  //!
  //! Traverse syntax tree and generate the corresponding assembly code.
  //!
  class postfix_writer: public basic_ast_visitor {
    bool _inFunctionBody = false;
    bool _inFunctionArgs = false;
    cdk::symbol_table<udf::symbol> &_symtab;
    std::stack<int> _forIni, _forStep, _forEnd;
    std::set<std::string> _functions_to_declare;
    int _offset;
    std::shared_ptr<udf::symbol> _function;
    std::string _currentFunctionName;
    std::string _currentBodyRetLabel;
    cdk::basic_postfix_emitter &_pf;
    int _lbl;

  public:
    postfix_writer(std::shared_ptr<cdk::compiler> compiler,
               cdk::symbol_table<udf::symbol> &symtab,
               cdk::basic_postfix_emitter &pf) :
    basic_ast_visitor(compiler),
    _inFunctionBody(false),
    _inFunctionArgs(false),
    _symtab(symtab),
    _forIni(), _forStep(), _forEnd(),
    _functions_to_declare(),
    _offset(0),
    _function(nullptr),
    _currentFunctionName(""),
    _currentBodyRetLabel(""),
    _pf(pf),
    _lbl(0) {
    }


  public:
    ~postfix_writer() {
      os().flush();
    }

  private:
    /** Method used to generate sequential labels. */
    inline std::string mklbl(int lbl) {
      std::ostringstream oss;
      if (lbl < 0)
        oss << ".L" << -lbl;
      else
        oss << "_L" << lbl;
      return oss.str();
    }

  public:
  // do not edit these lines
#define __IN_VISITOR_HEADER__
#include ".auto/visitor_decls.h"       // automatically generated
#undef __IN_VISITOR_HEADER__
  // do not edit these lines: end

  };

} // udf
