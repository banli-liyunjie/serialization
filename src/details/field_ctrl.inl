#pragma once
#include <typeinfo>

namespace banli {

template <typename _T, typename _SUB_T>
inline typename std::enable_if<has_data_update<_T>::value, int>::type to_set_field::operator()(_T& t, const _SUB_T& value, const std::string& name)
{
    if (name.empty()) {
        std::cerr << "wrong : empty field name" << std::endl;
        return error_return::ERROR_EMPTY_NAME;
    }
    updata_class_field ucf(value, *this);
    int ret = t.data_update(name, ucf);
    if (ret == ERROR_FIELD_NAME) {
        std::cerr << "can't find field named " << name << std::endl;
        ret = 0;
    }
    return ret;
}

template <typename _T>
inline typename std::enable_if<has_data_update<_T>::value, int>::type to_set_field::operator()(_T& t, const char* value, const std::string& name)
{
    std::string v(value);
    return (*this)(t, v, name);
}

template <typename _T, typename _SUB_T, typename... Args>
inline typename std::enable_if<has_data_update<_T>::value, int>::type to_set_field::operator()(_T& t, const _SUB_T& value, const std::string& name, const Args&... args)
{
    if (name.empty()) {
        std::cerr << "wrong : empty field name" << std::endl;
        return error_return::ERROR_EMPTY_NAME;
    }
    updata_class_field ucf(value, *this, std::string(args)...);
    int ret = t.data_update(name, ucf);
    if (ret == ERROR_FIELD_NAME) {
        std::cerr << "can't find field named " << name << std::endl;
        ret = 0;
    }
    return ret;
}

template <typename _T, typename... Args>
inline typename std::enable_if<has_data_update<_T>::value, int>::type to_set_field::operator()(_T& t, const char* value, const std::string& name, const Args&... args)
{
    std::string v(value);
    return (*this)(t, v, name, args...);
}

template <typename _T, typename _SUB_T, typename... Args>
inline typename std::enable_if<!has_data_update<_T>::value, int>::type to_set_field::operator()(_T& t, const _SUB_T& value, const std::string& name, const Args&... args)
{
    return error_return::ERROR_TYPE_TO_CALL;
}

template <typename _T, typename... Args>
to_set_field::updata_class_field<_T, Args...>::updata_class_field(const _T& v, to_set_field& p, const Args&... args)
    : value(v)
    , parent(p)
    , params(std::make_tuple(args...))
{
}

template <typename _T, typename... Args>
template <typename _U>
inline typename std::enable_if<std::is_convertible<_T, _U>::value && (sizeof...(Args) == 0), int>::type to_set_field::updata_class_field<_T, Args...>::operator()(field_left<_U>& p)
{
    p.left_value = static_cast<_U>(value);
    return 0;
}

template <typename _T, typename... Args>
template <typename _U>
inline typename std::enable_if<std::is_convertible<_T, _U>::value && (sizeof...(Args) != 0), int>::type to_set_field::updata_class_field<_T, Args...>::operator()(field_left<_U>& p)
{
    constexpr size_t a = sizeof...(Args);
    return call_to_set_field(p, std::make_index_sequence<a> {});
}

template <typename _T, typename... Args>
template <typename _U>
inline typename std::enable_if<!std::is_convertible<_T, _U>::value && (sizeof...(Args) == 0), int>::type to_set_field::updata_class_field<_T, Args...>::operator()(field_left<_U>& p)
{
    std::cerr << "the type of the field (\"" << p.name << "\") does not match the type of the value set" << std::endl;
    return error_return::ERROR_TYPE_MATCH;
}

template <typename _T, typename... Args>
template <typename _U>
inline typename std::enable_if<!std::is_convertible<_T, _U>::value && (sizeof...(Args) != 0), int>::type to_set_field::updata_class_field<_T, Args...>::operator()(field_left<_U>& p)
{
    constexpr size_t a = sizeof...(Args);
    return call_to_set_field(p, std::make_index_sequence<a> {});
}

template <typename _T, typename... Args>
template <typename _U, std::size_t... I>
int to_set_field::updata_class_field<_T, Args...>::call_to_set_field(field_left<_U>& p, std::index_sequence<I...>)
{
    int ret = parent(p.left_value, value, std::get<I>(params)...);
    if (ret == error_return::ERROR_TYPE_TO_CALL) {
        std::cerr << "the type of the field (\"" << p.name << "\") is not class type, or it does not implement the data_update method. " << std::endl;
        ret = 0;
    }
    return ret;
}

}
