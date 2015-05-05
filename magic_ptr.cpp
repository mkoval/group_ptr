#include <iostream>
#include <unordered_set>

// Reference Implementation

template <typename T> class group;
template <typename T> class member;
template <typename T> class group_ptr;
template <typename T>
group_ptr<T> make_group_ptr(T *p);

template <typename T>
struct group {
    group()
        : refcount(0)
    {
        std::cout << "\t+group<" << this << ">" << std::endl;
    }

    ~group()
    {
        std::cout << "\t-group<" << this << ">" << std::endl;
    }

    int refcount;
    std::unordered_set<member<T> *> members;
};

template <typename T>
struct member {
    member()
        : p(nullptr)
        , group(nullptr)
        , refcount(0)
    {
        std::cout << "\t+member<" << this << ">" << std::endl;
    }

    ~member()
    {
        std::cout << "\t-member<" << this << ">" << std::endl;
    }


    T *p;
    group<T> *group;
    int refcount;
};

template <typename T>
class group_ptr {
public:
    group_ptr()
        : member_(nullptr)
    {
        std::cout << "\tgroup_ptr<" << this << ">()" << std::endl;
    }

    ~group_ptr()
    {
        std::cout << "\t~group_ptr<" << this << ">" << std::endl;
        reset();
    }

    group_ptr(group_ptr<T> const &other)
        : member_(nullptr)
    {
        std::cout << "\tgroup_ptr<" << this << ">(" << &other << ")" << std::endl;
        *this = other;
    }

    group_ptr(group_ptr<T> &&other)
        : member_(nullptr)
    {
        member_ = other.member_;
        other.member_ = nullptr;
    }

    group_ptr<T> &operator=(group_ptr<T> const &other)
    {
        std::cout << "\tgroup_ptr<" << this << ">::operator=(" << &other << ")" << std::endl;
        reset(other.member_);
        return *this;
    }

    group_ptr<T> &operator=(group_ptr<T> &&other)
    {
        member_ = other.member_;
        other.member_ = nullptr;
        return *this;
    }

    T &operator *() const
    {
        return *member_->p;
    }

    T *operator ->() const
    {
        return member_->p;
    }

    void add_to_group(group_ptr<T> const &other)
    {
        other.member_->group->refcount -= other.member_->refcount;
        other.member_->group->members.erase(other.member_);

        if (other.member_->group->refcount == 0) {
            delete other.member_->group;
        }

        other.member_->group = member_->group;

        member_->group->refcount += other.member_->refcount;
        member_->group->members.insert(other.member_);
    }

    void merge_group(group_ptr<T> const &other)
    {
        group<T> *const other_group = other.member_->group;

        member_->group->refcount += other_group->refcount;

        member_->group->members.insert(
            std::begin(other_group->members),
            std::end(other_group->members)
        );

        for (member<T> *const member : other_group->members) {
            member->group = member_->group;
        }

        delete other_group;
    }

private:
    member<T> *member_;

    group_ptr(T *p, group<T> *group)
        : member_(new member<T>)
    {
        std::cout << "\tgroup_ptr<" << this << ">(" << p << ", " << group << ")"
                  << " member = " << member_ << std::endl;

        member_->p = p;
        member_->group = group;
        member_->refcount = 1;
        member_->group->refcount++;

        group->members.insert(member_);
    }

    void reset(member<T> *new_member = nullptr)
    {
        std::cout << "\t\tif (member_ = " << member_ << ")" << std::endl;
        if (member_) {
            std::cout << "\t\tmember->refcount = " << member_->refcount << std::endl;
            --(member_->refcount);
            std::cout << "\t\tmember->group->refcount = " << member_->group->refcount << std::endl;
            --(member_->group->refcount);

            if (member_->group->refcount == 0) {
                group<T> *const group = member_->group;

                for (member<T> *member : group->members) {
                    delete member->p;
                    delete member;
                }

                delete group;
            }
        }

        std::cout << "\t\tmember_ = " << new_member << std::endl;
        member_ = new_member;

        if (member_) {
            ++(member_->refcount);
            ++(member_->group->refcount);
        }
    }

    friend group_ptr<T> make_group_ptr<>(T *p);
};

template <typename T>
group_ptr<T> make_group_ptr(T *p)
{
    return group_ptr<T>(p, new group<T>);
}

// Example

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

int main(int argc, char **argv)
{
    std::cout << "+main" << std::endl;
    {
        auto g1a = make_group_ptr<A>(new A("G1a"));

        std::cout << "+scope" << std::endl;
        {
            auto g1b = make_group_ptr<A>(new A("G1b"));
            g1a.add_to_group(g1b);

            auto g2a = make_group_ptr<A>(new A("G2a"));
            auto g2b = make_group_ptr<A>(new A("G2b"));
            g2a.add_to_group(g2b);
            g1a.merge_group(g2a);
        }
        std::cout << "-scope" << std::endl;
    }
    std::cout << "-main" << std::endl;

    return 0;
}
