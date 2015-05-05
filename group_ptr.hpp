#ifndef GROUP_PTR_HPP_
#define GROUP_PTR_HPP_

namespace detail {

class member_base;

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
struct member : public member_base,
                std::enable_shared_from_this<member<T> > {
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
template <typename T> group_ptr<T> make_group_ptr(T *p);

template <typename T>
class group_ptr {
public:
    group_ptr()
    {
    }

    group_ptr(T *p)
        : group_ptr(p, new detail::group)
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
        other.member_ = nullptr;
    }

    group_ptr<T> &operator=(group_ptr<T> const &other)
    {
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
        return *get();
    }

    T *operator ->() const
    {
        return get();
    }

    bool operator ==(group_ptr<T> const &other) const
    {
        return get() == other.get();
    }

    bool operator !=(group_ptr<T> const &other) const
    {
        return get() != other.get();
    }

    bool operator <(group_ptr<T> const &other) const
    {
        return get() < other.get();
    }

    bool operator <=(group_ptr<T> const &other) const
    {
        return get() <= other.get();
    }

    bool operator >(group_ptr<T> const &other) const
    {
        return get() > other.get();
    }

    bool operator >=(group_ptr<T> const &other) const
    {
        return get() >= other.get();
    }

    operator bool() const
    {
        return !!get();
    }
 
    T *get() const
    {
        return member_->p;
    }

    void reset()
    {
        reset(nullptr);
    }

    void reset(group_ptr<T> const &other)
    {
        reset(other.member_);
    }

    template <typename U>
    void add_to_group(group_ptr<U> const &other)
    {
        auto other_member_shared = other.member_->shared_from_this();
        detail::group *const this_group = member_->group;
        detail::group *const other_group = other.member_->group;

        other_group->refcount -= other.member_->refcount;
        other_group->members.erase(other_member_shared);

        if (other_group->refcount == 0) {
            delete other_group;
        }

        other.member_->group = member_->group;

        this_group->refcount += other.member_->refcount;
        this_group->members.insert(other_member_shared);
    }

    template <typename U>
    void merge_group(group_ptr<U> const &other)
    {
        detail::group *const this_group = member_->group;
        detail::group *const other_group = other.member_->group;

        this_group->refcount += other_group->refcount;

        this_group->members.insert(
            std::begin(other_group->members),
            std::end(other_group->members)
        );

        for (auto const &member : other_group->members) {
            member->group = member_->group;
        }

        delete other_group;
    }

private:
    detail::member<T> *member_;

    group_ptr(T *p, detail::group *group)
    {
        std::shared_ptr<detail::member<T> > this_member(new detail::member<T>);
        group->members.insert(this_member);

        member_ = this_member.get();
        member_->p = p;
        member_->group = group;
        member_->refcount = 1;
        member_->group->refcount++;
    }

    void reset(detail::member<T> *other_member)
    {
        if (other_member) {
            detail::group *const other_group = other_member->group;

            other_member->refcount++;
            other_group->refcount++;
        }

        if (member_) {
            detail::group *const this_group = member_->group;

            member_->refcount--;
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
        : member_(other->member_->shared_from_this())
    {
    }

    group_ptr<T> lock() const
    {
        return group_ptr<T>(member_);
    }

private:
    std::weak_ptr<detail::member<T> > member_;
};


template <typename T, typename... Args>
group_ptr<T> make_group_ptr(Args&&... args)
{
    return group_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
std::ostream &operator <<(std::ostream &stream, group_ptr<T> const &p)
{
    stream << p.get();
    return stream;
}

namespace std {

template <typename T>
struct hash<group_ptr<T> >
{
    std::size_t operator ()(group_ptr<T> const &p) const
    {
        return std::hash<T *>()(p.get());
    }
};

}

#endif
