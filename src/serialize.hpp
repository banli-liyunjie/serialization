#include "json.hpp"
#include <array>
#include <mutex>
#include <type_traits>

struct to_serialize;
struct to_deserialize;

template <typename _T>
class serialize {
public:
    serialize() = default;
    serialize(const serialize& s) = default;
    virtual ~serialize() {};

    virtual void json_update() const = 0;
    virtual int data_update(const json_object* jo) = 0;

    friend to_serialize;
    friend to_deserialize;

protected:
    template <typename _SUB_T>
    void json_update(const std::string& name, const _SUB_T& sub_t) const
    {
        to_serialize ser;
        sub_jsons[name] = ser(sub_t);
    }
    template <typename _SUB_T>
    int data_update(_SUB_T& sub_t, const json_object* jo)
    {
        if (jo == nullptr) {
            std::cerr << typeid(_SUB_T).name() << " please check json_object or this class update func" << std::endl;
            return -1;
        }

        to_deserialize deser;
        return deser(sub_t, jo);
    }

private:
    inline static std::unordered_map<std::string, std::string> sub_jsons;
};

struct to_serialize {
    inline static std::recursive_mutex serialize_mutex;

    inline std::string operator()(const json_object* jo)
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

    template <typename _T>
    typename std::enable_if<std::is_arithmetic<_T>::value, std::string>::type
    operator()(const _T& t)
    {
        std::ostringstream oss;
        oss << t;
        return oss.str();
    }

    std::string operator()(const bool& b)
    {
        return b ? "true" : "false";
    }

    std::string operator()(const std::string& s)
    {
        return "\"" + s + "\"";
    }

    template <typename T>
    std::string operator()(const std::vector<T>& vec)
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
    typename std::enable_if<std::is_base_of<serialize<_T>, _T>::value, std::string>::type
    operator()(const _T& obj)
    {
        std::lock_guard<std::recursive_mutex> lock(serialize_mutex);

        std::ostringstream oss;
        obj.json_update();
        oss << "{";
        for (auto it = serialize<_T>::sub_jsons.begin(); it != serialize<_T>::sub_jsons.end(); ++it) {
            oss << "\"" << it->first << "\" : ";
            oss << it->second;
            if (std::next(it) != serialize<_T>::sub_jsons.end())
                oss << ", ";
        }
        oss << "}";
        return oss.str();
    }
};

struct to_deserialize {

    template <typename _T>
    typename std::enable_if<std::is_arithmetic<_T>::value, int>::type
    operator()(_T& t, const json_object* jo) // no type check
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

    int operator()(bool& b, const json_object* jo)
    {
        if (jo == nullptr || jo->type != json_type::JSON_BOOL) {
            std::cerr << "please check json_object" << std::endl;
            return -1;
        }
        b = jo->get_json_boolean();
        return 0;
    }

    int operator()(std::string& s, const json_object* jo)
    {
        if (jo == nullptr || jo->type != json_type::JSON_STRING) {
            std::cerr << "please check json_object" << std::endl;
            return -1;
        }
        s = jo->get_json_string();
        return 0;
    }

    template <typename T>
    int operator()(std::vector<T>& vec, const json_object* jo)
    {
        if (jo == nullptr || jo->type != json_type::JSON_ARRAY) {
            std::cerr << "please check json_object" << std::endl;
            return -1;
        }
        auto& sub_jo = std::get<std::vector<json_object*>>(jo->value);
        if (vec.size() > sub_jo.size())
            vec.resize(sub_jo.size());
        else if (vec.size() < sub_jo.size()) {
            T t;
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
    typename std::enable_if<std::is_base_of<serialize<_T>, _T>::value, int>::type
    operator()(_T& obj, const json_object* jo)
    {
        if (jo == nullptr || jo->type != json_type::JSON_CLASS) {
            std::cerr << "please check json_object" << std::endl;
            return -1;
        }
        return obj.data_update(jo);
    }
};
