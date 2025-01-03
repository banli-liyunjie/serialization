#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

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
    json_type type = json_type::JSON_WRONG;
    std::variant<std::string, long long, double, bool, std::vector<json_object*>, std::unordered_map<std::string, json_object*>> value;
    json_object() = default;
    ~json_object();

    std::string get_json_string() const;
    long long get_json_integer() const;
    double get_json_floating() const;
    bool get_json_boolean() const;
    json_object* operator[](size_t index) const;
    json_object* operator[](const std::string& key) const;

private:
};

json_object* json_load(const std::string& file_name);

json_object* json_get(const std::string& json_string);
