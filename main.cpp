#include "src/serialize.hpp"
#include <thread>
#include <unistd.h>

using namespace std;

class BBB : public serialize<BBB> {
public:
    using serialize<BBB>::json_update;
    void json_update() const
    {
        json_update(string("x"), this, &this->x);
        json_update(string("y"), this, &this->y);
        json_update(string("bbb"), this, &this->bbb);
        json_update(string("array"), this, &this->array);
    }

private:
    int x = 12;
    float y = 25.6;
    string bbb = "bbb";
    vector<int> array = { 3, 6, 7, 8 };
};

class AAA : public serialize<AAA> {
public:
    using serialize<AAA>::json_update;
    void json_update() const
    {
        json_update(string("x"), this, &this->x);
        json_update(string("y"), this, &this->y);
        json_update(string("f"), this, &this->f);
        json_update(string("aaa"), this, &this->aaa);
        json_update(string("b"), this, &this->b);
    }

    // private:
    int x = -20;
    float y = 3.141592;
    bool f = true;
    string aaa = "aaa";
    BBB b;
};

int main()
{
    json_object* json_root = json_load("C:/Users/yunjie.li/Documents/code/c_json_serialization/result.json");
    cout << "load finish\n";
    if (json_root != nullptr) {
        cout << "json_type : " << json_root->type << endl;

        to_serialize ser;

        cout << ser(json_root) << endl;

        delete json_root;
    }
    return 0;
}