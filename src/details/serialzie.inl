#pragma once

namespace banli {

inline std::string to_serialize::operator()(const json_object* jo)
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
        auto& v = std::get<std::vector<json_object*>>(jo->value);
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
        auto& u = std::get<std::unordered_map<std::string, json_object*>>(jo->value);
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

inline std::string to_serialize::operator()(const bool& b)
{
    return b ? "true" : "false";
}

inline std::string to_serialize::operator()(const std::string& s)
{
    return "\"" + s + "\"";
}

template <typename _T>
inline typename std::enable_if<std::is_arithmetic<_T>::value, std::string>::type
to_serialize::operator()(const _T& t)
{
    std::ostringstream oss;
    oss << t;
    return oss.str();
}

template <typename _T>
inline std::string to_serialize::operator()(const std::vector<_T>& vec)
{
    std::string json_string = "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        json_string += (*this)(vec[i]);
        if (i < vec.size() - 1) {
            json_string += ", ";
        }
    }
    json_string += "]";
    return json_string;
}

template <typename _T>
inline typename std::enable_if<has_data_update<_T>::value, std::string>::type
to_serialize::operator()(_T& obj)
{
    updata_class_json ucj(*this);
    obj.data_update("", ucj);
    return "{" + ucj.sub_json + "}";
}

to_serialize::updata_class_json::updata_class_json(to_serialize& p)
    : parent(p)
{
}

template <typename _T>
inline int to_serialize::updata_class_json::operator()(field_left<_T>& p)
{
    if (sub_json != "")
        sub_json += ", ";
    sub_json += "\"" + p.name + "\" : " + parent(p.left_value);
    return 0;
}

inline int to_deserialize::operator()(bool& b, const json_object* jo)
{
    if (jo == nullptr || jo->type != json_type::JSON_BOOL) {
        std::cerr << "please check json_object" << std::endl;
        return -1;
    }
    b = jo->get_json_boolean();
    return 0;
}

inline int to_deserialize::operator()(std::string& s, const json_object* jo)
{
    if (jo == nullptr || jo->type != json_type::JSON_STRING) {
        std::cerr << "please check json_object" << std::endl;
        return -1;
    }
    s = jo->get_json_string();
    return 0;
}

template <typename _T>
inline int to_deserialize::operator()(_T& t, const std::string& js)
{
    int ret = 0;
    json_object* jo = json_get(js);
    if (jo != nullptr) {
        ret |= (*this)(t, jo);
        delete jo;
    } else {
        ret = -1;
    }
    return ret;
}

template <typename _T>
inline typename std::enable_if<std::is_arithmetic<_T>::value, int>::type to_deserialize::operator()(_T& t, const json_object* jo)
{
    if (jo == nullptr) {
        std::cerr << "please check json_object" << std::endl;
        return -1;
    }
    if (jo->type == json_type::JSON_INTEGER) {
        t = (_T)(jo->get_json_integer());
    } else if (jo->type == json_type::JSON_FLOATING) {
        t = (_T)(jo->get_json_floating());
    } else {
        std::cerr << "please check json_object" << std::endl;
        return -1;
    }
    return 0;
}

template <typename _T>
inline int to_deserialize::operator()(std::vector<_T>& vec, const json_object* jo)
{
    if (jo == nullptr || jo->type != json_type::JSON_ARRAY) {
        std::cerr << "please check json_object" << std::endl;
        return -1;
    }
    auto& sub_jo = std::get<std::vector<json_object*>>(jo->value);
    if (vec.size() > sub_jo.size())
        vec.resize(sub_jo.size());
    else if (vec.size() < sub_jo.size()) {
        _T t;
        size_t c;
        vec.resize(sub_jo.size());
        std::fill(vec.begin() + c, vec.end(), t);
    }

    for (size_t i = 0; i < sub_jo.size(); ++i) {
        if ((*this)(vec[i], sub_jo[i]) != 0)
            return -1;
    }
    return 0;
}

template <typename _T>
inline typename std::enable_if<has_data_update<_T>::value, int>::type to_deserialize::operator()(_T& obj, const json_object* jo)
{
    if (jo == nullptr || jo->type != json_type::JSON_CLASS) {
        std::cerr << "please check json_object" << std::endl;
        return -1;
    }
    update_class_field ucf(jo, *this);
    return obj.data_update("", ucf);
}

to_deserialize::update_class_field::update_class_field(const json_object* _jo, to_deserialize& p)
    : jo(_jo)
    , parent(p)
{
}

template <typename _T>
inline int to_deserialize::update_class_field::operator()(field_left<_T>& p)
{
    return parent(p.left_value, (*jo)[p.name]);
}

}
