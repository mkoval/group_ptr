#include "group_ptr.hpp"
#include <iostream>
#include <vector>

class BodyNode;

class Skeleton {
public:
    virtual ~Skeleton()
    {
        std::cout << "-Skeleton(" << name_ << ")" << std::endl;
    }

    static group_ptr<Skeleton> create(std::string const &name)
    {
        group_ptr<Skeleton> p(new Skeleton(name));
        // TODO: replace this with enable_shared_from_this equivalent
        p->self_ = p;
        return p;
    }

    group_ptr<BodyNode> add(group_ptr<BodyNode> const &node)
    {
        self_.lock().add_to_group(node);
        nodes_.push_back(node);

        // TODO: repeat this for the subtree rooted at node

        return node;
    }

private:
    std::string name_;
    group_weak_ptr<Skeleton> self_;
    std::vector<group_weak_ptr<BodyNode> > nodes_;

    Skeleton(std::string const &name)
        : name_(name)
    {
        std::cout << "+Skeleton(" << name_ << ")" << std::endl;
    }
};


class BodyNode {
public:
    ~BodyNode()
    {
        std::cout << "-BodyNode(" << name_ << ")" << std::endl;
    }

    static group_ptr<BodyNode> create(std::string const &name)
    {
        group_ptr<BodyNode> p(new BodyNode(name));
        // TODO: replace this with enable_shared_from_this equivalent
        p->self_ = p;
        return p;
    }

private:
    std::string name_;
    group_weak_ptr<BodyNode> self_;

    BodyNode(std::string const &name)
        : name_(name)
    {
        std::cout << "+BodyNode(" << name_ << ")" << std::endl;
    }

    friend class Skeleton;
};

int main(int argc, char **argv)
{
    std::cout << "Entering main" << std::endl;
    group_ptr<BodyNode> r2d2_link3;

    {
        std::cout << "Constructing R2D2 with three links." << std::endl;
        group_ptr<Skeleton> r2d2 = Skeleton::create("r2d2");
        r2d2->add(BodyNode::create("r2d2/link1"));
        r2d2->add(BodyNode::create("r2d2/link2"));
        r2d2_link3 = r2d2->add(BodyNode::create("r2d2/link3"));

        {
            std::cout << "Constructing C3P0 with four links." << std::endl;
            group_ptr<Skeleton> c3p0 = Skeleton::create("c3p0");
            c3p0->add(BodyNode::create("c3p0/link1"));
            c3p0->add(BodyNode::create("c3p0/link2"));
            c3p0->add(BodyNode::create("c3p0/link3"));

            group_ptr<BodyNode> c3p0_link4 = c3p0->add(BodyNode::create("c3p0/link4"));

            std::cout << "Moving link4 from C3P0 to R2D2." << std::endl;
            r2d2->add(c3p0_link4);

            std::cout << "C3P0 is going out of scope..." << std::endl;
        }

        std::cout << "R2D2 should not go out of scope because of link3" << std::endl;
    }

    std::cout << "Also clearing link3, R2D2 is going out of scope..." << std::endl;
    r2d2_link3.reset();

    std::cout << "Leaving main" << std::endl;

    return 0;
}
