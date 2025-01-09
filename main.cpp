#include "src/field_ctrl.hpp"
#include "src/serialize.hpp"
#include <filesystem>
#include <thread>
#include <unistd.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

using namespace std;
using namespace banli;
namespace fs = filesystem;

class BBB {
public:
    template <typename func_struct>
    int data_update(const string& name, func_struct& func)
    {
        return variable_func_call(name, func,
            field_left("x", x),
            field_left("y", y),
            field_left("bbb", bbb),
            field_left("array", array));
    }

private:
    int x = 12;
    float y = 25.6;
    string bbb = "bbb";
    vector<int> array = { 1, 3, 5, 7, 9 };
};

class AAA {
public:
    template <typename func_struct>
    int data_update(const string& name, func_struct& func)
    {
        return variable_func_call(name, func,
            field_left("x", x),
            field_left("y", y),
            field_left("f", f),
            field_left("aaa", aaa),
            field_left("b", b));
    }

private:
    int x = -20;
    float y = 3.141592;
    bool f = true;
    string aaa = "aaa";
    BBB b;
};

fs::path get_executable_directory()
{
    fs::path directory_path;

#if defined(_WIN32) || defined(_WIN64)
    // Windows
    wchar_t buffer[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(NULL, buffer, MAX_PATH) == 0) {
        throw runtime_error("Error getting module file name, last error: " + to_string(GetLastError()));
    }
    fs::path full_path(buffer);
    directory_path = full_path.parent_path();

#else
    // Linux
    char buffer[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
    if (count == -1) {
        throw runtime_error("Error getting executable path");
    }
    buffer[count] = '\0'; // Null-terminate the string
    fs::path full_path(buffer);
    directory_path = full_path.parent_path();
#endif

    return directory_path;
}

int main()
{
    fs::path directory_path;
    try {
        directory_path = get_executable_directory();
        cout << "Executable directory: " << directory_path << endl;
    } catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    string result_json = directory_path.string() + "/result.json";

    AAA a0, a1;
    string content = serialize(a0);

#if defined(_WIN32) || defined(_WIN64)
    string command = "echo " + content + " > " + result_json;
#else
    string command = "echo \"" + content + "\" > " + result_json;
#endif

    system(command.c_str());
    json_object* json_root = json_load(result_json);

    if (json_root != nullptr) {
        cout << "json_type : " << json_root->type << endl;
        cout << serialize(json_root) << endl;

        deserialize(a0, json_root);
        cout << serialize(a0) << endl;

        float floating = 3.6f;
        int integer = 10;
        vector<int> array = { 4, 5, 6, 7 };
        BBB b;

        set_field(a0, 10002, "x");
        set_field(a0, floating, "y");
        set_field(a0, false, "f");
        set_field(a0, "aaaxxx", "aaa");
        set_field(a0, floating, "b", "x"); // implicit conversion, but with loss of precision
        set_field(a0, integer, "b", "y"); // implicit conversion
        set_field(a0, "bbbyyyy", "b", "bbb");
        set_field(a0, array, "b", "array");

        // set_field(d, 3.8);  //wrong
        set_field(a0, "sss", "x"); // type match wrong
        set_field(a0, a1, "b", "x"); // type match wrong
        set_field(a0, 20, "hhh"); // field name wrong
        set_field(a0, 20, "b", "xxx"); // field name wrong
        set_field(a0, 20, "y", "x"); // wrong class type
        set_field(a0, 20, "b", "x", "z"); // wrong class type
        set_field(a0, 20, ""); // name empty wrong
        set_field(a0, 20, "b", ""); // name empty wrong

        string js = serialize(a0);
        deserialize(a1, js);
        cout << serialize(a1) << endl;

        set_field(a0, b, "b"); // operator=
        cout << serialize(a0) << endl;

        delete json_root;
    }
    return 0;
}
