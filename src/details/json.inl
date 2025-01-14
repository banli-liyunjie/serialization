#pragma once

#include <filesystem>
#include <fstream>

namespace banli {

json_object::json_object()
{
    object_count++;
}

json_object::~json_object()
{
    object_count--;
}

inline std::string json_object::get_json_string() const
{
    if (type == json_type::JSON_STRING && std::holds_alternative<std::string>(value))
        return std::get<std::string>(value);
    else
        return "this is not a string";
}

inline long long json_object::get_json_integer() const
{
    if (type == json_type::JSON_INTEGER && std::holds_alternative<long long>(value))
        return std::get<long long>(value);
    else
        return 0;
}

inline double json_object::get_json_floating() const
{
    if (type == json_type::JSON_FLOATING && std::holds_alternative<double>(value))
        return std::get<double>(value);
    else
        return 0.0f;
}

inline bool json_object::get_json_boolean() const
{
    if (type == json_type::JSON_BOOL && std::holds_alternative<bool>(value))
        return std::get<bool>(value);
    else
        return false;
}

inline std::shared_ptr<json_object> json_object::operator[](size_t index) const
{
    if (type == json_type::JSON_ARRAY && std::holds_alternative<std::vector<std::shared_ptr<json_object>>>(value) && index < std::get<std::vector<std::shared_ptr<json_object>>>(value).size())
        return std::get<std::vector<std::shared_ptr<json_object>>>(value)[index];
    else
        return nullptr;
}

inline std::shared_ptr<json_object> json_object::operator[](const std::string& key) const
{
    if (type == json_type::JSON_CLASS && std::holds_alternative<std::unordered_map<std::string, std::shared_ptr<json_object>>>(value)) {
        auto& u = std::get<std::unordered_map<std::string, std::shared_ptr<json_object>>>(value);
        if (u.find(key) != u.end())
            return u.at(key);
        else
            return nullptr;
    } else
        return nullptr;
}

inline size_t analyze_json_string(std::shared_ptr<json_object> jo, const std::string& json_string, size_t pos)
{
    std::string non_string = " \n\t\r";

    size_t loc = json_string.find_first_not_of(non_string, pos);

    if (loc != std::string::npos) {
        switch (json_string[loc]) {
        case '{': {
            jo->value = std::unordered_map<std::string, std::shared_ptr<json_object>>();
            size_t times = 0;
            while (({
                loc = json_string.find_first_not_of(non_string, loc + 1);
                loc != std::string::npos && (json_string[loc] == '"' || (json_string[loc] == ',' && times > 0));
            })) {
                if (json_string[loc] == ',') {
                    loc = json_string.find_first_not_of(non_string, loc + 1);
                    if (json_string[loc] != '"') {
                        std::cerr << "fields in the object do not start with the character '\"'" << std::endl;
                        goto CLASS_WRONG;
                    }
                }
                size_t next;
                while (({
                    next = json_string.find_first_of('"', loc + 1);
                    next != std::string::npos && (json_string[next - 1] == '\\');
                })) { }
                if (next == std::string::npos) {
                    std::cerr << "missing character '\"'" << std::endl;
                    goto CLASS_WRONG;
                }
                std::string key = json_string.substr(loc + 1, next - loc - 1);
                auto& u = std::get<std::unordered_map<std::string, std::shared_ptr<json_object>>>(jo->value);
                if (u.find(key) != u.end()) {
                    std::cerr << "duplicate key : " << "\"" << key << "\"" << std::endl;
                    goto CLASS_WRONG;
                }
                loc = json_string.find_first_not_of(non_string, next + 1);
                if (loc == std::string::npos || json_string[loc] != ':') {
                    std::cerr << "the key " << "(\"" << key << "\")" << " must be followed by the character ':'" << std::endl;
                    goto CLASS_WRONG;
                }
                std::shared_ptr<json_object> sub_object = std::make_shared<json_object>();
                loc = analyze_json_string(sub_object, json_string, loc + 1);
                if (sub_object->type == json_type::JSON_WRONG) {
                    std::cerr << "the key " << "(\"" << key << "\")" << " does not have a valid object" << std::endl;
                    goto CLASS_WRONG;
                }
                u.emplace(key, sub_object);
                times++;
            }
            if (loc == std::string::npos || json_string[loc] != '}') {
                std::cerr << "missing character '}' or starts with the character ','" << std::endl;
                goto CLASS_WRONG;
            }
            if (pos == 0 && json_string.find_first_not_of(non_string, loc + 1) != std::string::npos) {
                std::cerr << "extra characters at the end" << std::endl;
                goto CLASS_WRONG;
            }

            pos = loc;
            jo->type = json_type::JSON_CLASS;
            break;

        CLASS_WRONG:
            auto& u = std::get<std::unordered_map<std::string, std::shared_ptr<json_object>>>(jo->value);
            u.clear();
            jo->type = json_type::JSON_WRONG;
            break;
        }

        case '[': {
            jo->value = std::vector<std::shared_ptr<json_object>>();
            size_t times = 0;
            while (({
                loc = json_string.find_first_not_of(non_string, loc + 1);
                loc != std::string::npos&& json_string[loc] != ']' && (!(json_string[loc] == ',' && times == 0));
            })) {
                if (json_string[loc] == ',') {
                    loc = json_string.find_first_not_of(non_string, loc + 1);
                    if (loc == std::string::npos) {
                        std::cerr << "unexpected end" << std::endl;
                        goto ARRAY_WRONG;
                    }
                }
                std::shared_ptr<json_object> sub_object = std::make_shared<json_object>();
                loc = analyze_json_string(sub_object, json_string, loc);
                if (sub_object->type == json_type::JSON_WRONG) {
                    std::cerr << "the array does not have a valid object" << std::endl;
                    goto ARRAY_WRONG;
                }
                std::get<std::vector<std::shared_ptr<json_object>>>(jo->value).emplace_back(sub_object);
                times++;
            }
            if (loc == std::string::npos || loc == ',') {
                std::cerr << "missing character ']' or starts with the character ','" << std::endl;
                goto ARRAY_WRONG;
            }
            if (pos == 0 && json_string.find_first_not_of(non_string, loc + 1) != std::string::npos) {
                std::cerr << "extra characters at the end" << std::endl;
                goto ARRAY_WRONG;
            }

            pos = loc;
            jo->type = json_type::JSON_ARRAY;
            break;

        ARRAY_WRONG:
            auto& v = std::get<std::vector<std::shared_ptr<json_object>>>(jo->value);
            v.clear();
            jo->type = json_type::JSON_WRONG;
            break;
        }

        case '"': {
            jo->value = std::string();
            size_t next;
            while (({
                next = json_string.find_first_of('"', loc + 1);
                next != std::string::npos && (json_string[next - 1] == '\\');
            })) { }
            if (next == std::string::npos) {
                std::cerr << "missing character '\"'" << std::endl;
            } else if (pos == 0 && json_string.find_first_not_of(non_string, next + 1) != std::string::npos) {
                std::cerr << "extra characters at the end" << std::endl;
            } else {
                jo->type = json_type::JSON_STRING;
                std::get<std::string>(jo->value) = json_string.substr(loc + 1, next - loc - 1);
                pos = next;
            }
            break;
        }
        default: {
            if (json_string[loc] == 't' && loc + 3 < json_string.size()) {
                if (json_string.substr(loc, 4) == "true") {
                    if (pos == 0 && json_string.find_first_not_of(non_string, loc + 3 + 1) != std::string::npos) {
                        std::cerr << "extra characters at the end" << std::endl;
                        goto WRONG;
                    }

                    jo->value = true;
                    pos = loc + 3;
                    jo->type = json_type::JSON_BOOL;
                } else {
                    std::cerr << "invalid json number or boolean" << std::endl;
                    goto WRONG;
                }
            } else if (json_string[loc] == 'f' && loc + 4 < json_string.size()) {
                if (pos == 0 && json_string.find_first_not_of(non_string, loc + 4 + 1) != std::string::npos) {
                    std::cerr << "extra characters at the end" << std::endl;
                    goto WRONG;
                }

                if (json_string.substr(loc, 5) == "false") {
                    jo->value = false;
                    pos = loc + 4;
                    jo->type = json_type::JSON_BOOL;
                } else {
                    std::cerr << "invalid json number or boolean" << std::endl;
                    goto WRONG;
                }

            } else if (json_string[loc] == 'n' && loc + 3 < json_string.size()) {
                if (pos == 0 && json_string.find_first_not_of(non_string, loc + 3 + 1) != std::string::npos) {
                    std::cerr << "extra characters at the end" << std::endl;
                    goto WRONG;
                }

                if (json_string.substr(loc, 4) == "null") {
                    pos = loc + 3;
                    jo->type = json_type::JSON_NULL;
                } else {
                    std::cerr << "invalid json number or boolean" << std::endl;
                    goto WRONG;
                }
            } else {
                if (json_string[loc] != '-' && !std::isdigit(json_string[loc])) {
                    std::cerr << "invalid json number or string" << std::endl;
                    goto WRONG;
                }
                bool is_floating_point = false;
                bool has_digits = false;
                size_t point_loc;
                size_t end = loc;

                if (json_string[end] == '-')
                    end++;

                for (; end < json_string.size(); end++) {
                    if (std::isdigit(json_string[end])) {
                        has_digits = true;
                    } else if (json_string[end] == '.' && has_digits && !is_floating_point) {
                        point_loc = end;
                        is_floating_point = true;
                    } else {
                        break;
                    }
                }
                if (!has_digits || (is_floating_point && point_loc == end - 1) || (end < json_string.size() && json_string[loc] == '.')) {
                    std::cerr << "invalid json number or string" << std::endl;
                    goto WRONG;
                }

                end--;
                size_t count0 = 0, int_end = is_floating_point ? point_loc - 1 : end, int_start = json_string[loc] == '-' ? loc + 1 : loc;
                for (int i = int_start; i <= int_end; ++i) {
                    if (json_string[i] == '0') {
                        count0++;
                    } else {
                        break;
                    }
                }
                if (count0 > 1 || (count0 == 1 && int_end > int_start)) {
                    std::cerr << "invalid json number or string" << std::endl;
                    goto WRONG;
                }
                if (pos == 0 && json_string.find_first_not_of(non_string, end + 1) != std::string::npos) {
                    std::cerr << "extra characters at the end" << std::endl;
                    goto WRONG;
                }

                std::string num_str = json_string.substr(loc, end - loc + 1);
                pos = end;
                if (is_floating_point) {
                    jo->value = std::stod(num_str);
                    jo->type = json_type::JSON_FLOATING;
                } else {
                    jo->value = std::stoll(num_str);
                    jo->type = json_type::JSON_INTEGER;
                }
            }
            break;
        WRONG:
            jo->type = json_type::JSON_WRONG;
            break;
        }
        }
    } else {
        std::cerr << "invalid json string" << std::endl;
        jo->type = json_type::JSON_WRONG;
    }

    return pos;
}

inline std::shared_ptr<json_object> json_load(const std::string& file_name)
{
    if (!std::filesystem::exists(file_name)) {
        std::cerr << "file does not exist : " << file_name << std::endl;
        return nullptr;
    }

    std::ifstream input_file(file_name);

    if (!input_file) {
        std::cerr << "error opening file: " << file_name << std::endl;
        return nullptr;
    }
    std::string json_string((std::istreambuf_iterator<char>(input_file)),
        std::istreambuf_iterator<char>());
    input_file.close();

    return json_get(json_string);
}

std::shared_ptr<json_object> json_get(const std::string& json_string)
{
    std::shared_ptr<json_object> jo = std::make_shared<json_object>();

    analyze_json_string(jo, json_string, 0);

    if (jo->type != json_type::JSON_WRONG) //!
        return jo;
    return nullptr;
}

}
