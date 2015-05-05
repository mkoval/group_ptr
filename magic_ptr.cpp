#include <iostream>
#include <unordered_set>
#include "group_ptr.hpp"

struct A {
    A(std::string const &name)
        : name_(name)
    {
        std::cout << "+A(" << name_ << ")" << std::endl;
    }

    ~A()
    {
        std::cout << "-A(" << name_ << ")" << std::endl;
    }

    std::string name_;
};

struct B {
    B(std::string const &name)
        : name_(name)
    {
        std::cout << "+B(" << name_ << ")" << std::endl;
    }

    ~B()
    {
        std::cout << "-B(" << name_ << ")" << std::endl;
    }

    std::string name_;
};

int main(int argc, char **argv)
{
    std::cout << "+main" << std::endl;
    {
        auto g1a = make_group_ptr<>(new A("G1a"));

        std::cout << "+scope" << std::endl;
        {
            auto g1b = make_group_ptr<>(new A("G1b"));
            auto g1c = make_group_ptr<>(new A("G1c"));
            g1a.add_to_group(g1b);
            g1a.add_to_group(g1c);
            g1c.reset(g1b);

            std::cout << "+scope" << std::endl;
            {
                auto g2a = make_group_ptr<>(new B("G2a"));
                auto g2b = make_group_ptr<>(new B("G2b"));
                g2a.add_to_group<>(g2b);
                g1a.merge_group<B>(g2a);
            }
            std::cout << "-scope" << std::endl;
        }
        std::cout << "-scope" << std::endl;
    }
    std::cout << "-main" << std::endl;

    return 0;
}
