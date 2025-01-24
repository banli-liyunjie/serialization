#pragma once
#include "json.hpp"
#include "reflection.hpp"

namespace banli {
struct to_serialize {
    inline std::string operator()(const std::shared_ptr<json_object>& jo);
    inline std::string operator()(const basic_object& obj);
};

struct to_deserialize {

    inline int operator()(basic_object& obj, const std::shared_ptr<json_object>& jo);
    inline int operator()(basic_object& obj, const std::string& js);
};

inline static to_serialize serialize;
inline static to_deserialize deserialize;
}

#include "details/serialzie.inl"
