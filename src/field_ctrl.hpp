#pragma once
#include "details/fieldstr.hpp"
#include <iostream>
#include <tuple>

namespace banli {

struct to_set_field {

    template <typename _T, typename _SUB_T>
    inline typename std::enable_if<has_data_update<_T>::value, int>::type operator()(_T& t, const _SUB_T& value, const std::string& name);
    template <typename _T>
    inline typename std::enable_if<has_data_update<_T>::value, int>::type operator()(_T& t, const char* value, const std::string& name);
    template <typename _T, typename _SUB_T, typename... Args>
    inline typename std::enable_if<has_data_update<_T>::value, int>::type operator()(_T& t, const _SUB_T& value, const std::string& name, const Args&... args);
    template <typename _T, typename... Args>
    inline typename std::enable_if<has_data_update<_T>::value, int>::type operator()(_T& t, const char* value, const std::string& name, const Args&... args);
    template <typename _T, typename _SUB_T, typename... Args>
    inline typename std::enable_if<!has_data_update<_T>::value, int>::type operator()(_T& t, const _SUB_T& value, const std::string& name, const Args&... args);

private:
    enum error_return {
        NON_ERROR = 0,
        ERROR_EMPTY_NAME = 1,
        ERROR_TYPE_TO_CALL = 2,
        ERROR_TYPE_MATCH = 4
    };

    template <typename _T, typename... Args>
    struct updata_class_field {
        static_assert((std::is_same<Args, std::string>::value && ...), "All parameters must be of type std::string");

        std::tuple<Args...> params;
        _T value;
        to_set_field& parent;

        updata_class_field(const _T& v, to_set_field& parent, const Args&... args);

        template <typename _U>
        inline typename std::enable_if<std::is_convertible<_T, _U>::value && (sizeof...(Args) == 0), int>::type operator()(field_left<_U>& p);
        template <typename _U>
        inline typename std::enable_if<std::is_convertible<_T, _U>::value && (sizeof...(Args) != 0), int>::type operator()(field_left<_U>& p);
        template <typename _U>
        inline typename std::enable_if<!std::is_convertible<_T, _U>::value && (sizeof...(Args) == 0), int>::type operator()(field_left<_U>& p);
        template <typename _U>
        inline typename std::enable_if<!std::is_convertible<_T, _U>::value && (sizeof...(Args) != 0), int>::type operator()(field_left<_U>& p);

        template <typename _U, std::size_t... I>
        int call_to_set_field(field_left<_U>& p, std::index_sequence<I...>);
    };
};

inline static to_set_field set_field;

}

#include "details/field_ctrl.inl"
