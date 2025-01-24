#pragma once

namespace banli {

inline std::string to_serialize::operator()(const std::shared_ptr<json_object>& jo)
{
    std::ostringstream oss;
    if (jo->type == json_type::JSON_WRONG) {
        return "{wrong json object}";
    }
    switch (jo->type) {
    case json_type::JSON_NULL:
        return "null";
    case json_type::JSON_BOOL:
        return std::get<bool>(jo->value) ? "true" : "false";
    case json_type::JSON_STRING:
        return "\"" + std::get<std::string>(jo->value) + "\"";
    case json_type::JSON_INTEGER:
        oss << std::get<long long>(jo->value);
        break;
    case json_type::JSON_FLOATING:
        oss << std::get<double>(jo->value);
        break;
    case json_type::JSON_ARRAY: {
        oss << "[";
        auto& v = std::get<std::vector<std::shared_ptr<json_object>>>(jo->value);
        for (int i = 0; i < v.size(); ++i) {
            oss << (*this)(v[i]);
            if (i < v.size() - 1)
                oss << ", ";
        }
        oss << "]";
        break;
    }
    case json_type::JSON_CLASS: {
        oss << "{";
        auto& u = std::get<std::unordered_map<std::string, std::shared_ptr<json_object>>>(jo->value);
        for (auto it = u.begin(); it != u.end(); ++it) {
            oss << "\"" << it->first << "\" : ";
            oss << (*this)(it->second);
            if (std::next(it) != u.end())
                oss << ", ";
        }
        oss << "}";
        break;
    }
    default:
        return "wrong json type";
    }

    return oss.str();
}

inline std::string to_serialize::operator()(const basic_object& obj)
{
    return obj.to_json();
}

inline int to_deserialize::operator()(basic_object& obj, const std::shared_ptr<json_object>& jo)
{
    if (jo == nullptr || jo->type == json_type::JSON_WRONG) {
        return -1;
    }
    switch (jo->type) {
    case json_type::JSON_CLASS: {
        if (obj.is_valid() && obj.get_variable_type() == variable_type::CLASS) {
            auto& u = std::get<std::unordered_map<std::string, std::shared_ptr<json_object>>>(jo->value);
            for (auto it = u.begin(); it != u.end(); ++it) {
                basic_object obj_tmp = Type_Manager.get_field(obj, it->first);
                (*this)(obj_tmp, it->second);
            }
            break;
        }
    }
    case json_type::JSON_ARRAY: {
        if (obj.is_valid() && obj.get_variable_type() == variable_type::VECTOR) {
            auto& v = std::get<std::vector<std::shared_ptr<json_object>>>(jo->value);
            obj.vector_resize(v.size());
            for (int i = 0; i < v.size(); ++i) {
                basic_object obj_tmp = obj[i];
                (*this)(obj_tmp, v[i]);
            }
            break;
        }
    }
    case json_type::JSON_STRING: {
        if (obj.is_valid() && obj.get_variable_type() == variable_type::STRING) {
            obj = std::get<std::string>(jo->value);
            break;
        }
    }
    case json_type::JSON_INTEGER: {
        if (obj.is_valid() && obj.get_variable_type() == variable_type::NUMBER) {
            obj = std::get<long long>(jo->value);
            break;
        }
    }
    case json_type::JSON_FLOATING: {
        if (obj.is_valid() && obj.get_variable_type() == variable_type::NUMBER) {
            obj = std::get<double>(jo->value);
            break;
        }
    }
    case json_type::JSON_BOOL: {
        if (obj.is_valid() && obj.get_variable_type() == variable_type::BOOLEAN) {
            obj = std::get<bool>(jo->value);
            break;
        }
    }
    default:
        return -1;
    }
    return 0;
}

inline int to_deserialize::operator()(basic_object& obj, const std::string& js)
{
    int ret = 0;
    std::shared_ptr<json_object> jo = json_get(js);
    if (jo != nullptr) {
        ret |= (*this)(obj, jo);
    } else {
        ret = -1;
    }
    return ret;
}
}
