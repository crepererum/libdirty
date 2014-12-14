#include <iostream>

#include "include/dirty.hpp"

class A {
    public:
        A(int x) : i(x), j(x * 10) {}
        virtual ~A() {}

        virtual int f(int a, int b) {
            return a + b;
        }

    private:
        int i;
        int j;
};

int g(A* _self, int a, int b) {
    return a * b;
}

namespace dirty{namespace member{
template struct rob<Af<int A::*>, &A::i, 5>;
template struct rob<Af<int A::*>, &A::j, 0>;
}}

int main() {
    A* a = new A(1);
    A* b = new A(2);
    A* c = new A(3);

    std::cout << "a->f(3, 4) = " << a->f(3,4) << std::endl;
    std::cout << "b->f(3, 4) = " << b->f(3,4) << std::endl;
    std::cout << "c->f(3, 4) = " << c->f(3,4) << std::endl;

    std::cout << "Replace vtable of a..." << std::flush;
    dirty::vtable_t vtable = dirty::vtable_copy(dirty::obj_get_vtable(a), 8);
    dirty::vtable_replace_entry(vtable, dirty::vtable_calc_idx(&A::f), (void*)&g);
    dirty::obj_replace_vtable(a, vtable);
    std::cout << "OK"<< std::endl;

    std::cout << "a->f(3, 4) = " << a->f(3,4) << std::endl;
    std::cout << "b->f(3, 4) = " << b->f(3,4) << std::endl;
    std::cout << "c->f(3, 4) = " << c->f(3,4) << std::endl;

    std::cout << "Patch vtable..." << std::flush;
    dirty::vtable_patch(&A::f, (void*)&g, a);
    std::cout << "OK" << std::endl;

    std::cout << "a->f(3, 4) = " << a->f(3,4) << std::endl;
    std::cout << "b->f(3, 4) = " << b->f(3,4) << std::endl;
    std::cout << "c->f(3, 4) = " << c->f(3,4) << std::endl;

    std::cout << "a: i=" << dirty::member::get<int A::*, 5>(a) << " j=" << dirty::member::get<int A::*, 0>(a) << std::endl;
    dirty::member::get<int A::*, 5>(a) = 1337;
    std::cout << "a: i=" << dirty::member::get<int A::*, 5>(a) << " j=" << dirty::member::get<int A::*, 0>(a) << std::endl;

    delete a;
    delete b;
    delete c;
    delete vtable;
}

