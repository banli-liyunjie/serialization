
#pragma once
#include <string>

template <typename _T>
struct field_left {
    const std::string name;
    const std::string type_id;
    _T& left_value;
    field_left() = delete;
    field_left(const std::string& _name, _T& value)
        : name(_name)
        , left_value(value)
    {
    }
};

#define ERROR_FIELD_NAME 999999

template <typename func_struct, typename... Args>
int variable_func_call(const std::string& name, func_struct& func, Args... args)
{
    int ret = 0;
    if (name.empty()) {
        ((ret |= func(args)), ...);
    } else {
        ret = ERROR_FIELD_NAME;
        (((args.name == name) ? (ret = func(args)) : 0), ...); // only one option is selected, and no duplicate name check is performed
    }
    return ret;
}

template <typename T>
class has_data_update {
private:
    template <typename U>
    static auto test(int) -> decltype(std::declval<U>().data_update(std::declval<std::string>(), std::declval<int&>()), std::true_type());

    template <typename>
    static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};
