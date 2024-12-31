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

string answer0, answer1;

void task0()
{
    AAA a;
    to_serialize ser;

    for (int i = 0; i < 10; ++i) {
        a.x = i;
        a.y = 10 - i;
        a.f = true;
        a.aaa = "task0";
        answer0 = ser(a);
        sleep(1);
    }
}

void task1()
{
    AAA a;
    to_serialize ser;

    for (int i = 0; i < 10; ++i) {
        a.x = i + 100;
        a.y = 10 - i + 100;
        a.f = false;
        a.aaa = "task1";
        answer1 = ser(a);
        sleep(1);
    }
}

int main()
{
    thread t1(task0), t2(task1);

    t1.join();
    t2.join();

    cout << answer0 << endl
         << answer1 << endl;

    return 0;
}