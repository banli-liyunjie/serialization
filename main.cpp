#include "src/reflection.hpp"
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
    int x = 12;
    float y = 25.6;
    string bbb = "bbb";
    vector<int> array = { 1, 3, 5, 7, 9 };
};

class AAA {
public:
    int x = -20;
    float y = 3.141592;
    bool f = false;
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

void example()
{
    basic_object obj_A0 = Type_Manager.make_instance("AAA");
    AAA a1;
    basic_object obj_A1(&a1); // pointer copy, with no subsequent memory deallocation
    basic_object obj_A2(new AAA(), true); // pointer takeover, with subsequent memory deallocation

    basic_object obj_B0 = Type_Manager.make_instance("BBB");
    obj_B0["x"] = 129; // pointer copy, constant overhead
    obj_B0["y"] = 3892.5;
    obj_B0["bbb"] = "hello bbb";
    obj_B0["array"] = vector<int>({ 2, 4, 6, 8, 10 });
    obj_B0["array"][2] = 10000;

    obj_A0["x"] = 100;
    obj_A0["y"] = 200.678;
    obj_A0["f"] = true;
    obj_A0["aaa"] = "hello aaa";
    obj_A0["b"] = obj_B0; // value copy

    try {
        cout << "A0::x " << obj_A0["x"].data_as<int>() << endl;
        cout << "A0::y " << obj_A0["y"].data_as<float>() << endl;
        cout << "A0::f " << obj_A0["f"].data_as<bool>() << endl;
        cout << "A0::aaa " << obj_A0["aaa"].data_as<string>() << endl;
        cout << "A0::b::x " << obj_A0["b"]["x"].data_as<int>() << endl;
        cout << "A0::b::y " << obj_A0["b"]["y"].data_as<float>() << endl;
        cout << "A0::b::bbb " << obj_A0["b"]["bbb"].data_as<string>() << endl;
        cout << "A0::b::array[0] " << obj_A0["b"]["array"].data_as<vector<int>>()[0] << endl;
        cout << "A0::b::array[2] " << obj_A0["b"]["array"].data_as<vector<int>>()[2] << endl;
    } catch (const std::exception& e) {
        cout << "Error: " << e.what() << endl;
    }

    obj_A1["x"] = 128.596; // 128
    obj_A1["y"] = 125; // 125f
    obj_A1["f"] = 129.685; // true
    obj_A1["aaa"] = 128.569; // nothing to do

    cout << "A1::x " << a1.x << endl;
    cout << "A1::y " << a1.y << endl;
    cout << "A1::f " << a1.f << endl;
    cout << "A1::aaa " << a1.aaa << endl;

    basic_object obj_aaa = obj_A1["aaa"]; // pointer copy
    obj_aaa = "hello aaa";
    cout << "A1::aaa " << a1.aaa << endl;

    basic_object obj_aaa_copy = obj_aaa; // build memory and copy
    obj_aaa_copy = "hello aaa 2";
    cout << "A1::aaa " << a1.aaa << endl;
    try {
        cout << "copy : " << obj_aaa_copy.data_as<string>() << endl;
    } catch (const std::exception& e) {
        cout << "Error: " << e.what() << endl;
    }
    obj_aaa_copy = obj_aaa; // value copy
    try {
        cout << "copy : " << obj_aaa_copy.data_as<string>() << endl;
    } catch (const std::exception& e) {
        cout << "Error: " << e.what() << endl;
    }
    cout << "end example" << endl;
}

int main()
{
    Type_Manager.register_type<BBB>("BBB");
    Type_Manager.register_type<AAA>("AAA");
    Type_Manager.register_field("x", &BBB::x);
    Type_Manager.register_field("y", &BBB::y);
    Type_Manager.register_field("bbb", &BBB::bbb);
    Type_Manager.register_field("array", &BBB::array);
    Type_Manager.register_field("x", &AAA::x);
    Type_Manager.register_field("y", &AAA::y);
    Type_Manager.register_field("f", &AAA::f);
    Type_Manager.register_field("aaa", &AAA::aaa);
    Type_Manager.register_field("b", &AAA::b);

    example();

    fs::path directory_path;
    try {
        directory_path = get_executable_directory();
        cout << "Executable directory: " << directory_path << endl;
    } catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    string result_json = directory_path.string() + "/result.json";

    basic_object a0 = Type_Manager.make_instance("AAA");
    string content = serialize(a0);

#if defined(_WIN32) || defined(_WIN64)
    string command = "echo " + content + " > " + result_json;
#else
    string command = "echo \"" + content + "\" > " + result_json;
#endif

    system(command.c_str());
    std::shared_ptr<json_object> json_root = json_load(result_json);

    if (json_root != nullptr) {
        cout << serialize(json_root) << endl;
        basic_object a1 = a0;
        a1["x"] = 101;
        a1["y"] = 2.5;
        a1["f"] = true;
        a1["aaa"] = "hello aaa !";
        a1["b"]["x"] = 102;
        a1["b"]["y"] = 2.6;
        a1["b"]["bbb"] = "hello bbb !";
        a1["b"]["array"][2] = 10000;
        cout << serialize(a1) << endl;
        basic_object a2 = a1;
        deserialize(a2, json_root);
        cout << serialize(a2) << endl;
        deserialize(a0, serialize(a1));
        cout << serialize(a0) << endl;
    }
    return 0;
}

