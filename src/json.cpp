#include "json.hpp"
#include <filesystem>
#include <fstream>

using namespace std;

json_object::~json_object()
{
    if (type == json_type::JSON_ARRAY) {
        for (auto p : get<vector<json_object*>>(value)) {
            delete (p);
        }
    } else if (type == json_type::JSON_CLASS) {
        for (auto p : get<unordered_map<std::string, json_object*>>(value)) {
            delete (p.second);
        }
    }
}

string json_object::get_json_string() const
{
    if (type == json_type::JSON_STRING)
        return get<string>(value);
    else
        return "";
}

long long json_object::get_json_integer() const
{
    if (type == json_type::JSON_INTEGER)
        return get<long long>(value);
    else
        return 0;
}

double json_object::get_json_floating() const
{
    if (type == json_type::JSON_FLOATING)
        return get<double>(value);
    else
        return 0.0f;
}

bool json_object::get_json_boolean() const
{
    if (type == json_type::JSON_BOOL)
        return get<bool>(value);
    else
        return false;
}

json_object* json_object::operator[](size_t index) const
{
    if (type == json_type::JSON_ARRAY && index < get<vector<json_object*>>(value).size())
        return get<vector<json_object*>>(value)[index];
    else
        return nullptr;
}

json_object* json_object::operator[](const string& key) const
{
    if (type == json_type::JSON_CLASS) {
        auto& u = get<unordered_map<std::string, json_object*>>(value);
        if (u.find(key) != u.end())
            return u.at(key);
        else
            return nullptr;
    } else
        return nullptr;
}

size_t analyze_json_string(json_object* jo, string& json_string, size_t pos)
{
    string non_string = " \n\t\r";

    size_t loc = json_string.find_first_not_of(non_string, pos);

    if (loc != string::npos) {
        switch (json_string[loc]) {
        case '{': {
            jo->value = unordered_map<std::string, json_object*>();
            size_t times = 0;
            while (({
                loc = json_string.find_first_not_of(non_string, loc + 1);
                loc != string::npos && (json_string[loc] == '"' || (json_string[loc] == ',' && times > 0));
            })) {
                if (json_string[loc] == ',') {
                    loc = json_string.find_first_not_of(non_string, loc + 1);
                    if (json_string[loc] != '"') {
                        cerr << "fields in the object do not start with the character '\"'" << endl;
                        goto CLASS_WRONG;
                    }
                }
                size_t next;
                while (({
                    next = json_string.find_first_of('"', loc + 1);
                    next != string::npos && (json_string[next - 1] == '\\');
                })) { }
                if (next == string::npos) {
                    cerr << "missing character '\"'" << endl;
                    goto CLASS_WRONG;
                }
                string key = json_string.substr(loc + 1, next - loc - 1);
                auto& u = get<unordered_map<std::string, json_object*>>(jo->value);
                if (u.find(key) != u.end()) {
                    cerr << "duplicate key : " << "\"" << key << "\"" << endl;
                    goto CLASS_WRONG;
                }
                loc = json_string.find_first_not_of(non_string, next + 1);
                if (loc == string::npos || json_string[loc] != ':') {
                    cerr << "the key " << "(\"" << key << "\")" << " must be followed by the character ':'" << endl;
                    goto CLASS_WRONG;
                }
                json_object* sub_object = new json_object();
                loc = analyze_json_string(sub_object, json_string, loc + 1);
                if (sub_object->type == json_type::JSON_WRONG) {
                    delete sub_object;
                    cerr << "the key " << "(\"" << key << "\")" << " does not have a valid object" << endl;
                    goto CLASS_WRONG;
                }
                u.emplace(key, sub_object);
                times++;
            }
            if (loc == string::npos || json_string[loc] != '}') {
                cerr << "missing character '}' or starts with the character ','" << endl;
                goto CLASS_WRONG;
            }
            if (pos == 0 && json_string.find_first_not_of(non_string, loc + 1) != string::npos) {
                cerr << "extra characters at the end" << endl;
                goto CLASS_WRONG;
            }

            pos = loc;
            jo->type = json_type::JSON_CLASS;
            break;

        CLASS_WRONG:
            for (auto p : get<unordered_map<string, json_object*>>(jo->value)) {
                delete (p.second);
            }
            jo->type = json_type::JSON_WRONG;
            break;
        }

        case '[': {
            jo->value = vector<json_object*>();
            size_t times = 0;
            while (({
                loc = json_string.find_first_not_of(non_string, loc + 1);
                loc != string::npos&& json_string[loc] != ']' && (!(json_string[loc] == ',' && times == 0));
            })) {
                if (json_string[loc] == ',') {
                    loc = json_string.find_first_not_of(non_string, loc + 1);
                    if (loc == string::npos) {
                        cerr << "unexpected end" << endl;
                        goto ARRAY_WRONG;
                    }
                }
                json_object* sub_object = new json_object();
                loc = analyze_json_string(sub_object, json_string, loc);
                if (sub_object->type == json_type::JSON_WRONG) {
                    delete sub_object;
                    cerr << "the array does not have a valid object" << endl;
                    goto ARRAY_WRONG;
                }
                get<vector<json_object*>>(jo->value).emplace_back(sub_object);
                times++;
            }
            if (loc == string::npos || loc == ',') {
                cerr << "missing character ']' or starts with the character ','" << endl;
                goto ARRAY_WRONG;
            }
            if (pos == 0 && json_string.find_first_not_of(non_string, loc + 1) != string::npos) {
                cerr << "extra characters at the end" << endl;
                goto ARRAY_WRONG;
            }

            pos = loc;
            jo->type = json_type::JSON_ARRAY;
            break;

        ARRAY_WRONG:
            auto& v = get<vector<json_object*>>(jo->value);
            for (auto& p : v) {
                delete p;
            }
            v.clear();
            jo->type = json_type::JSON_WRONG;
            break;
        }

        case '"': {
            jo->value = "";
            size_t next;
            while (({
                next = json_string.find_first_of('"', loc + 1);
                next != string::npos && (json_string[next - 1] == '\\');
            })) { }
            if (next == string::npos) {
                cerr << "missing character '\"'" << endl;
            } else if (pos == 0 && json_string.find_first_not_of(non_string, next + 1) != string::npos) {
                cerr << "extra characters at the end" << endl;
            } else {
                jo->type = json_type::JSON_STRING;
                get<string>(jo->value) = json_string.substr(loc + 1, next - loc - 1);
                pos = next;
            }
            break;
        }
        default: {
            if (json_string[loc] == 't' && loc + 3 < json_string.size()) {
                if (json_string.substr(loc, 4) == "true") {
                    if (pos == 0 && json_string.find_first_not_of(non_string, loc + 3 + 1) != string::npos) {
                        cerr << "extra characters at the end" << endl;
                        goto WRONG;
                    }

                    jo->value = true;
                    pos = loc + 3;
                    jo->type = json_type::JSON_BOOL;
                } else {
                    goto WRONG;
                }
            } else if (json_string[loc] == 'f' && loc + 4 < json_string.size()) {
                if (pos == 0 && json_string.find_first_not_of(non_string, loc + 4 + 1) != string::npos) {
                    cerr << "extra characters at the end" << endl;
                    goto WRONG;
                }

                if (json_string.substr(loc, 5) == "false") {
                    jo->value = false;
                    pos = loc + 4;
                    jo->type = json_type::JSON_BOOL;
                } else {
                    goto WRONG;
                }

            } else if (json_string[loc] == 'n' && loc + 3 < json_string.size()) {
                if (pos == 0 && json_string.find_first_not_of(non_string, loc + 3 + 1) != string::npos) {
                    cerr << "extra characters at the end" << endl;
                    goto WRONG;
                }

                if (json_string.substr(loc, 4) == "null") {
                    pos = loc + 3;
                    jo->type = json_type::JSON_NULL;
                } else {
                    goto WRONG;
                }
            } else {
                if (json_string[loc] != '-' && !isdigit(json_string[loc])) {
                    cerr << "invalid json number or string"<<endl;
                    goto WRONG;
                }
                bool is_floating_point = false;
                bool has_digits = false;
                size_t point_loc;
                size_t end = loc;

                if (json_string[end] == '-')
                    end++;

                for (; end < json_string.size(); end++) {
                    if (isdigit(json_string[end])) {
                        has_digits = true;
                    } else if (json_string[end] == '.' && has_digits && !is_floating_point) {
                        point_loc = end;
                        is_floating_point = true;
                    } else {
                        break;
                    }
                }
                if (!has_digits || (is_floating_point && point_loc == end - 1) || (end < json_string.size() && json_string[loc] == '.')) {
                    cerr << "invalid json number or string" << endl;
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
                    cerr << "invalid json number or string";
                    goto WRONG;
                }
                if (pos == 0 && json_string.find_first_not_of(non_string, end + 1) != string::npos) {
                    cerr << "extra characters at the end" << endl;
                    goto WRONG;
                }

                string num_str = json_string.substr(loc, end - loc + 1);
                pos = end;
                if (is_floating_point) {
                    jo->value = stod(num_str);
                    jo->type = json_type::JSON_FLOATING;
                } else {
                    jo->value = stoll(num_str);
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
        jo->type = json_type::JSON_WRONG;
    }

    return pos;
}

json_object* json_load(const string& file_name)
{
    if (!filesystem::exists(file_name)) {
        cerr << "file does not exist : " << file_name << endl;
        return nullptr;
    }

    ifstream input_file(file_name);

    if (!input_file) {
        cerr << "error opening file: " << file_name << endl;
        return nullptr;
    }
    std::string json_string((std::istreambuf_iterator<char>(input_file)),
        std::istreambuf_iterator<char>());
    input_file.close();

    json_object* jo = new json_object();

    analyze_json_string(jo, json_string, 0);

    if (jo->type != json_type::JSON_WRONG) //!
        return jo;
    delete (jo);
    return nullptr;
}
