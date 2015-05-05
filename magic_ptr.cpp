#include <iostream>
#include <unordered_set>

// Reference Implementation

class group;
class member_base;
template <typename T> class member;
template <typename T> class group_ptr;
template <typename T> class group_weak_ptr;
template <typename T>
group_ptr<T> make_group_ptr(T *p);

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
    std::unordered_set<std::shared_ptr<member_base> > members;
};

struct member_base {
    member_base()
        : group(nullptr)
        , refcount(0)
    {
    }

    virtual ~member_base() = default;

    group *group;
    int refcount;
};

template <typename T>
struct member : public member_base {
    member()
        : p(nullptr)
    {
        std::cout << "\t+member<" << this << ">" << std::endl;
    }

    virtual ~member()
    {
        if (p) {
            delete p;
        }
        std::cout << "\t-member<" << this << ">" << std::endl;
    }

    T *p;
};

template <typename T>
class group_ptr {
public:
    group_ptr()
    {
        std::cout << "\tgroup_ptr<" << this << ">()" << std::endl;
    }

    ~group_ptr()
    {
        std::cout << "\t~group_ptr<" << this << ">" << std::endl;
        reset();
    }

    group_ptr(group_ptr<T> const &other)
    {
        std::cout << "\tgroup_ptr<" << this << ">(" << &other << ")" << std::endl;
        *this = other;
    }

    group_ptr(group_ptr<T> &&other)
    {
        member_ = other.member_;
        other.member_.reset();
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
        other.member_.reset();
        return *this;
    }

    T &operator *() const
    {
        return *member_.lock()->p;
    }

    T *operator ->() const
    {
        return member_.lock()->p;
    }

    void reset()
    {
        reset(std::shared_ptr<member<T> >());
    }

    void reset(group_ptr<T> const &other)
    {
        reset(other.member_.lock());
    }

    template <typename U>
    void add_to_group(group_ptr<U> const &other)
    {
        std::shared_ptr<member<T> > const this_member = member_.lock();
        std::shared_ptr<member<U> > const other_member = other.member_.lock();
        group *const this_group = this_member->group;
        group *const other_group = other_member->group;

        other_group->refcount -= other_member->refcount;
        other_group->members.erase(other_member);

        if (other_group->refcount == 0) {
            delete other_group;
        }

        other_member->group = this_member->group;

        this_group->refcount += other_member->refcount;
        this_group->members.insert(other_member);
    }

    template <typename U>
    void merge_group(group_ptr<U> const &other)
    {
        std::shared_ptr<member<T> > const this_member = member_.lock();
        std::shared_ptr<member<U> > const other_member = other.member_.lock();
        group *const this_group = this_member->group;
        group *const other_group = other_member->group;

        this_group->refcount += other_group->refcount;

        this_group->members.insert(
            std::begin(other_group->members),
            std::end(other_group->members)
        );

        for (auto const &member : other_group->members) {
            member->group = this_member->group;
        }

        delete other_group;
    }

private:
    std::weak_ptr<member<T> > member_;

    group_ptr(T *p, group *group)
    {
        std::shared_ptr<member<T> > this_member(new member<T>);

        this_member->p = p;
        this_member->group = group;
        this_member->refcount = 1;
        this_member->group->refcount++;

        group->members.insert(this_member);
        member_ = this_member;
    }

    void reset(std::shared_ptr<member<T> > const &other_member)
    {
        std::shared_ptr<member<T> > const this_member = member_.lock();

        if (other_member) {
            group *const other_group = other_member->group;

            other_member->refcount++;
            other_group->refcount++;
        }

        if (this_member) {
            group *const this_group = this_member->group;

            this_member->refcount--;
            this_group->refcount--;

            if (this_group->refcount == 0) {
                delete this_group;
            }
        }

        member_ = other_member;
    }

    template <typename U> friend class group_ptr;
    friend class group_weak_ptr<T>;
    friend group_ptr<T> make_group_ptr<>(T *p);
};

template <typename T>
class group_weak_ptr {
public:
    group_weak_ptr(group_ptr<T> const &other)
        : member_(other.member_)
    {
    }

    group_ptr<T> lock() const
    {
        return group_ptr<T>(member_);
    }

private:
    std::weak_ptr<member<T> > member_;
};


template <typename T>
group_ptr<T> make_group_ptr(T *p)
{
    return group_ptr<T>(p, new group);
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
