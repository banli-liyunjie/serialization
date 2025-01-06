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
namespace fs = filesystem;

class BBB : public serialize<BBB> {
public:
    using serialize<BBB>::json_update;
    using serialize<BBB>::data_update;
    void json_update()
    {
        json_update(string("x"), this->x);
        json_update(string("y"), this->y);
        json_update(string("bbb"), this->bbb);
        json_update(string("array"), this->array);
    }
    int data_update(const json_object* jo)
    {
        auto& object = *jo;
        int ret = 0;
        ret |= data_update(this->x, object["x"]);
        ret |= data_update(this->y, object["y"]);
        ret |= data_update(this->bbb, object["bbb"]);
        ret |= data_update(this->array, object["array"]);
        return ret;
    }

private:
    int x = 12;
    float y = 25.6;
    string bbb = "bbb";
    vector<int> array = { 1, 3, 5, 7, 9 };
};

class AAA : public serialize<AAA> {
public:
    using serialize<AAA>::json_update;
    using serialize<AAA>::data_update;
    void json_update()
    {
        json_update(string("x"), this->x);
        json_update(string("y"), this->y);
        json_update(string("f"), this->f);
        json_update(string("aaa"), this->aaa);
        json_update(string("b"), this->b);
    }
    int data_update(const json_object* jo)
    {
        auto& object = *jo;
        int ret = 0;
        ret |= data_update(this->x, object["x"]);
        ret |= data_update(this->y, object["y"]);
        ret |= data_update(this->f, object["f"]);
        ret |= data_update(this->aaa, object["aaa"]);
        ret |= data_update(this->b, object["b"]);
        return ret;
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

    AAA a;
    to_serialize ser;
    to_deserialize deser;

    string content = ser(a);

#if defined(_WIN32) || defined(_WIN64)
    string command = "echo " + content + " > " + result_json;
#else
    string command = "echo \"" + content + "\" > " + result_json;
#endif

    system(command.c_str());

    json_object* json_root = json_load(result_json);

    if (json_root != nullptr) {
        cout << "json_type : " << json_root->type << endl;
        cout << ser(json_root) << endl;

        deser(a, json_root);
        cout << ser(a) << endl;

        cout << ser(5) << endl;
        // cout << ser(vector<int>(1, 3, 4, 5)) << endl;

        delete json_root;
    }
    return 0;
}