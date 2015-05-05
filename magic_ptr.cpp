#include <iostream>
#include <unordered_set>

// Reference Implementation

template <typename T> class group;
template <typename T> class member;
template <typename T> class group_ptr;
template <typename T> class group_weak_ptr;
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
    std::unordered_set<std::shared_ptr<member<T> > > members;
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
        if (p) {
            delete p;
        }
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

    void add_to_group(group_ptr<T> const &other)
    {
        std::shared_ptr<member<T> > const this_member = member_.lock();
        std::shared_ptr<member<T> > const other_member = other.member_.lock();
        group<T> *const this_group = this_member->group;
        group<T> *const other_group = other_member->group;

        other_group->refcount -= other_member->refcount;
        other_group->members.erase(other_member);

        if (other_group->refcount == 0) {
            delete other_group;
        }

        other_member->group = this_member->group;

        this_group->refcount += other_member->refcount;
        this_group->members.insert(other_member);
    }

    void merge_group(group_ptr<T> const &other)
    {
        std::shared_ptr<member<T> > const this_member = member_.lock();
        std::shared_ptr<member<T> > const other_member = other.member_.lock();
        group<T> *const this_group = this_member->group;
        group<T> *const other_group = other_member->group;

        this_group->refcount += other_group->refcount;

        this_group->members.insert(
            std::begin(other_group->members),
            std::end(other_group->members)
        );

        for (std::shared_ptr<member<T> > const &member : other_group->members) {
            member->group = this_member->group;
        }

        delete other_group;
    }

private:
    std::weak_ptr<member<T> > member_;

    group_ptr(T *p, group<T> *group)
    {
        std::shared_ptr<member<T> > this_member(new member<T>);

        this_member->p = p;
        this_member->group = group;
        this_member->refcount = 1;
        this_member->group->refcount++;

        group->members.insert(this_member);
        member_ = this_member;
    }

    void reset()
    {
        reset(std::weak_ptr<member<T> >());
    }

    void reset(std::weak_ptr<member<T> > const &other_member)
    {
        std::shared_ptr<member<T> > const this_member = member_.lock();
        group<T> *const this_group = this_member->group;

        if (this_member) {
            this_member->refcount--;
            this_group->refcount--;

            if (this_group->refcount == 0) {
                delete this_group;
            }
        }

        member_ = other_member;

        std::shared_ptr<member<T> > const new_member = member_.lock();
        if (new_member) {
            new_member->refcount++;
            new_member->group->refcount++;
        }
    }

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
