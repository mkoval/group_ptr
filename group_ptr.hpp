#ifndef GROUP_PTR_HPP_
#define GROUP_PTR_HPP_

namespace detail {

class group;
class member_base;
template <typename T> class member;

struct group {
    group()
        : refcount(0)
    {
    }

    ~group()
    {
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
    }

    virtual ~member()
    {
        if (p) {
            delete p;
        }
    }

    T *p;
};

}

template <typename T> class group_ptr;
template <typename T> class group_weak_ptr;
template <typename T>
group_ptr<T> make_group_ptr(T *p);

template <typename T>
class group_ptr {
public:
    group_ptr()
    {
    }

    ~group_ptr()
    {
        reset();
    }

    group_ptr(group_ptr<T> const &other)
    {
        *this = other;
    }

    group_ptr(group_ptr<T> &&other)
    {
        member_ = other.member_;
        other.member_.reset();
    }

    group_ptr<T> &operator=(group_ptr<T> const &other)
    {
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
        reset(std::shared_ptr<detail::member<T> >());
    }

    void reset(group_ptr<T> const &other)
    {
        reset(other.member_.lock());
    }

    template <typename U>
    void add_to_group(group_ptr<U> const &other)
    {
        std::shared_ptr<detail::member<T> > const this_member = member_.lock();
        std::shared_ptr<detail::member<U> > const other_member = other.member_.lock();
        detail::group *const this_group = this_member->group;
        detail::group *const other_group = other_member->group;

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
        std::shared_ptr<detail::member<T> > const this_member = member_.lock();
        std::shared_ptr<detail::member<U> > const other_member = other.member_.lock();
        detail::group *const this_group = this_member->group;
        detail::group *const other_group = other_member->group;

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
    std::weak_ptr<detail::member<T> > member_;

    group_ptr(T *p, detail::group *group)
    {
        std::shared_ptr<detail::member<T> > this_member(new detail::member<T>);

        this_member->p = p;
        this_member->group = group;
        this_member->refcount = 1;
        this_member->group->refcount++;

        group->members.insert(this_member);
        member_ = this_member;
    }

    void reset(std::shared_ptr<detail::member<T> > const &other_member)
    {
        std::shared_ptr<detail::member<T> > const this_member = member_.lock();

        if (other_member) {
            detail::group *const other_group = other_member->group;

            other_member->refcount++;
            other_group->refcount++;
        }

        if (this_member) {
            detail::group *const this_group = this_member->group;

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
    std::weak_ptr<detail::member<T> > member_;
};


template <typename T>
group_ptr<T> make_group_ptr(T *p)
{
    return group_ptr<T>(p, new detail::group);
}

#endif
