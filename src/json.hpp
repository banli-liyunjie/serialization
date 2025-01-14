#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace banli {

enum json_type {
    JSON_WRONG,
    JSON_NULL,
    JSON_STRING,
    JSON_INTEGER,
    JSON_FLOATING,
    JSON_BOOL,
    JSON_ARRAY,
    JSON_CLASS
};

class json_object {
public:
    json_object();
    ~json_object();

    inline std::string get_json_string() const;
    inline long long get_json_integer() const;
    inline double get_json_floating() const;
    inline bool get_json_boolean() const;
    inline std::shared_ptr<json_object> operator[](size_t index) const;
    inline std::shared_ptr<json_object> operator[](const std::string& key) const;

    json_type type = json_type::JSON_WRONG;
    std::variant<std::string, long long, double, bool, std::vector<std::shared_ptr<json_object>>, std::unordered_map<std::string, std::shared_ptr<json_object>>> value;
    inline static int object_count = 0;
};

inline std::shared_ptr<json_object> json_load(const std::string& file_name);
inline std::shared_ptr<json_object> json_get(const std::string& json_string);
}

#include "details/json.inl"
