#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cdk/types/basic_type.h>

namespace udf {

  class symbol {
    bool _constant; 
    int _qualifier;
    std::shared_ptr<cdk::basic_type> _type;
    std::string _name;
    bool _initialized;
    bool _function;
    bool _forward = false;
    int _offset = 0;
    std::vector<std::shared_ptr<cdk::basic_type>> _arguments;
    long _value;  // hack! TODO ?

  public:
    symbol(bool constant, int qualifier,
           std::shared_ptr<cdk::basic_type> type,
           const std::string &name,
           bool initialized, bool function, bool forward)
      : _constant(constant),
        _qualifier(qualifier),
        _type(type),
        _name(name),
        _initialized(initialized),
        _function(function),
        _forward(forward),
        _value(0)
    {
    }

    virtual ~symbol() {
      // EMPTY
    }

    bool isConstant() const { return _constant; }
    int qualifier() const { return _qualifier; }

    std::shared_ptr<cdk::basic_type> type() const { return _type; }
    void set_type(std::shared_ptr<cdk::basic_type> t) { _type = t; }

    const std::string &name() const { return _name; }

    bool isFunction() const { return _function; }
    bool isInitialized() const { return _initialized; }
    bool forward() const { return _forward; }

    const std::vector<std::shared_ptr<cdk::basic_type>> &arguments() const { return _arguments; }
    void set_argument_types(const std::vector<std::shared_ptr<cdk::basic_type>> &types) { _arguments = types; }
    bool argument_is_typed(size_t ax, cdk::typename_type name) const {
      return _arguments[ax]->name() == name;
    }
    size_t argument_size(size_t ax) const {
      return _arguments[ax]->size();
    }
    std::shared_ptr<cdk::basic_type> argument_type(size_t ax) const {
      return _arguments[ax];
    }
    size_t number_of_arguments() const {
      return _arguments.size();
    }

    long value() const { return _value; }
    void setValue(long v) { _value = v; }

    int offset() const {
      return _offset;
    }
    void set_offset(int offset) {
      _offset = offset;
    }
    bool global() const {
      return _offset == 0;
    }
  };

  inline auto make_symbol(bool constant, int qualifier,
                          std::shared_ptr<cdk::basic_type> type,
                          const std::string &name,
                          bool initialized, bool function,
                          bool forward = false) {
    return std::make_shared<symbol>(constant, qualifier, type, name, initialized, function, forward);
  }

} // udf