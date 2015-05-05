/***********************************************************************
 * 
 * Copyright (c) 2014, Carnegie Mellon University
 * All rights reserved.
 * 
 * Authors: Michael Koval <mkoval@cs.cmu.edu>
 *          Pras Velagapudi <pkv@cs.cmu.edu>
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 ************************************************************************/
#ifndef GROUP_PTR_HPP_
#define GROUP_PTR_HPP_

#include <memory>
#include <unordered_set>

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
        : member_(nullptr)
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
        : member_(nullptr)
    {
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

    operator group_ptr<T const>() const
    {
        return group_ptr<T const>(member_);
    }
 
    T *get() const
    {
        return member_->p;
    }

    void swap(group_ptr<T> &other)
    {
        std::swap(member_, other.member_);
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
    typedef typename std::remove_const<T>::type nonconst_type;

    detail::member<nonconst_type> *member_;

    group_ptr(detail::member<nonconst_type> *member)
        : member_(member)
    {
        member_->refcount++;
        member_->group->refcount++;
    }

    group_ptr(T *p, detail::group *group)
    {
        std::shared_ptr<detail::member<nonconst_type> > this_member(
            new detail::member<nonconst_type>
        );
        group->members.insert(this_member);

        member_ = this_member.get();
        member_->p = const_cast<nonconst_type *>(p);
        member_->group = group;

        member_->refcount = 1;
        member_->group->refcount++;
    }

    void reset(detail::member<nonconst_type> *other_member)
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
    group_weak_ptr() = default;
    ~group_weak_ptr() = default;

    group_weak_ptr(group_ptr<T> const &other)
        : group_weak_ptr(other.member_->shared_from_this())
    {
    }

    operator group_weak_ptr<T const>() const
    {
        return group_weak_ptr<T const>(member_);
    }

    void reset()
    {
        member_.reset();
    }

    void swap(group_weak_ptr<T> &other)
    {
        member_.swap(other.member_);
    }

    bool expired() const
    {
        return member_.expired();
    }

    group_ptr<T> lock() const
    {
        auto member_ptr = member_.lock();
        if (member_ptr) {
            return group_ptr<T>(member_ptr.get());
        } else { 
            return group_ptr<T>();
        }
    }

private:
    typedef typename std::remove_const<T>::type nonconst_type;

    std::weak_ptr<detail::member<nonconst_type> > member_;

    group_weak_ptr(std::weak_ptr<detail::member<nonconst_type> > const &member)
        : member_(member)
    {
    }

    template <typename U> friend class group_weak_ptr;
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

template <typename T>
void swap(group_ptr<T> &lhs, group_ptr<T> &rhs)
{
    lhs.swap(rhs);
}

template <typename T>
void swap(group_weak_ptr<T> &lhs, group_weak_ptr<T> &rhs)
{
    lhs.swap(rhs);
}

}

#endif
