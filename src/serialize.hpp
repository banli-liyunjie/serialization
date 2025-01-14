#pragma once
#include "details/fieldstr.hpp"
#include "json.hpp"

namespace banli {
struct to_serialize {

    inline std::string operator()(const std::shared_ptr<json_object>& jo);
    inline std::string operator()(const bool& b);
    inline std::string operator()(const std::string& s);

    template <typename _T>
    inline typename std::enable_if<std::is_arithmetic<_T>::value, std::string>::type operator()(const _T& t);
    template <typename _T>
    inline std::string operator()(const std::vector<_T>& vec);
    template <typename _T>
    inline typename std::enable_if<has_data_update<_T>::value, std::string>::type operator()(_T& obj);

private:
    struct updata_class_json {
        std::string sub_json;
        to_serialize& parent;
        updata_class_json(to_serialize& p);
        template <typename _T>
        inline int operator()(field_left<_T>& p);
    };
};

struct to_deserialize {

    inline int operator()(bool& b, const std::shared_ptr<json_object>& jo);
    inline int operator()(std::string& s, const std::shared_ptr<json_object>& jo);

    template <typename _T>
    inline int operator()(_T& t, const std::string& js);
    template <typename _T>
    inline typename std::enable_if<std::is_arithmetic<_T>::value, int>::type operator()(_T& t, const std::shared_ptr<json_object>& jo); // no type check
    template <typename _T>
    inline int operator()(std::vector<_T>& vec, const std::shared_ptr<json_object>& jo);

    template <typename _T>
    inline typename std::enable_if<has_data_update<_T>::value, int>::type operator()(_T& obj, const std::shared_ptr<json_object>& jo);

private:
    struct update_class_field {
        const std::shared_ptr<json_object>& jo;
        to_deserialize& parent;
        update_class_field(const std::shared_ptr<json_object>& _jo, to_deserialize& p);
        template <typename _T>
        inline int operator()(field_left<_T>& p);
    };
};

inline static to_serialize serialize;
inline static to_deserialize deserialize;
}

#include "details/serialzie.inl"
