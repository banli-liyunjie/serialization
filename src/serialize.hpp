#include "json.hpp"
#include <array>
#include <mutex>
#include <type_traits>

struct to_serialize;

template <typename _T>
class serialize {
public:
    serialize() = default;
    serialize(const serialize& s) = default;
    virtual ~serialize() {};

    virtual void json_update() const = 0;

    friend to_serialize;

protected:
    template <typename _SUB_T>
    void json_update(const std::string& name, const _T* t, const _SUB_T* sub_t) const
    {
        if (offset.find(name) == offset.end()) {
            offset.emplace(name, (size_t)sub_t - (size_t)t);
        }

        to_serialize ser;

        sub_jsons[name] = ser(*sub_t);
    }

private:
    inline static std::unordered_map<std::string, size_t> offset;
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

    template <typename T, size_t N>
    std::string operator()(const std::array<T, N>& arr)
    {
        std::string json_string = "[";
        for (size_t i = 0; i < N; ++i) {
            json_string += (*this)(arr[i]);
            if (i < N - 1) {
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
