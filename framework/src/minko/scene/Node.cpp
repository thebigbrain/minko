/*
Copyright (c) 2014 Aerys

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "minko/scene/Node.hpp"
#include "minko/Common.hpp"
#include "minko/CloneOption.hpp"

#include "minko/component/AbstractComponent.hpp"
#include "minko/component/AbstractRebindableComponent.hpp"
#include "minko/scene/NodeSet.hpp"
#include "minko/data/Container.hpp"
#include "minko/data/StructureProvider.hpp"
#include "minko/Uuid.hpp"

using namespace minko;
using namespace minko::scene;
using namespace minko::component;

unsigned int Node::_lastId = 0;

Node::Node() :
    _id(_lastId++),
    _name("Node_" + std::to_string(_id)),
    _root(nullptr),
    _parent(nullptr),
    _container(data::Container::create()),
    _data(data::StructureProvider::create("node")),
    _added(Signal<Ptr, Ptr, Ptr>::create()),
    _removed(Signal<Ptr, Ptr, Ptr>::create()),
	_componentAdded(Signal<Ptr, Ptr, Node::AbsCmpPtr>::create()),
	_componentRemoved(Signal<Ptr, Ptr, Node::AbsCmpPtr>::create()),
    _layoutsChanged(Signal<Ptr, Ptr>::create()),
    _uuid(minko::Uuid::getUuid())
{
    _container->addProvider(_data);

    _data->set<Layouts>("layouts", Layout::Group::DEFAULT);
}

Layouts
Node::layouts() const
{
    return _data->get<Layouts>("layouts");
}

Node::Ptr
Node::clone(const CloneOption& option)
{
	auto clone = cloneNode();
	
	std::map<Node::Ptr, Node::Ptr>		nodeMap;		// map linking nodes to their clone
	std::map<AbsCmpPtr, AbsCmpPtr>	componentsMap;	// map linking components to their clone
	
	listItems(clone, nodeMap, componentsMap);
	
	rebindComponentsDependencies(componentsMap, nodeMap, option);

	for (auto itn = nodeMap.begin(); itn != nodeMap.end(); itn++)
	{
		auto node = itn->first;

		auto originComponents = node->components<AbstractComponent>();

		for (auto itc = componentsMap.begin(); itc != componentsMap.end(); itc++)
		{
			auto component = itc->first;

			// if the current node has a particular component, we clone it
			if (std::find(originComponents.begin(), originComponents.end(), component) != originComponents.end())
			{
				nodeMap[node]->addComponent(componentsMap[component]);
			}			
		}	
	}

	return nodeMap[shared_from_this()];
}

Node::Ptr
Node::cloneNode()
{	
	Node::Ptr clone = Node::create();

	clone->_name = shared_from_this()->name() + "_clone";

	for (auto child : children())
		clone->addChild(child->cloneNode());

	return clone;
}

void
Node::listItems(Node::Ptr clonedRoot, std::map<Node::Ptr, Node::Ptr>& nodeMap, std::map<AbsCmpPtr, AbsCmpPtr>& components)
{
	for (auto component : _components)
	{
		components[component] = component->clone(CloneOption::DEEP);
	}	

	nodeMap[shared_from_this()] = clonedRoot;

	for (int childId = 0; childId < children().size(); childId++)
	{
		auto child = children().at(childId);
		auto clonedChild = clonedRoot->children().at(childId);

		child->listItems(clonedChild, nodeMap, components);
	}	
}

void 
Node::rebindComponentsDependencies(std::map<AbsCmpPtr, AbsCmpPtr>& componentsMap, std::map<Node::Ptr, Node::Ptr> nodeMap, CloneOption option)
{
	for (auto itc = componentsMap.begin(); itc != componentsMap.end(); itc++)
	{
		auto comp = itc->first;
		auto compClone = std::dynamic_pointer_cast<AbstractRebindableComponent>(itc->second);
		
		if (compClone != nullptr)
		{
			compClone->rebindDependencies(componentsMap, nodeMap, option);
		}
	}
}

Node::Ptr
Node::layouts(Layouts value)
{
    if (value != layouts())
    {
        _data->set<Layouts>("layouts", value);

        // bubble down
        auto descendants = NodeSet::create(shared_from_this())->descendants(true);
        for (auto descendant : descendants->nodes())
            descendant->_layoutsChanged->execute(descendant, shared_from_this());

        // bubble up
        auto ancestors = NodeSet::create(shared_from_this())->ancestors();
        for (auto ancestor : ancestors->nodes())
            ancestor->_layoutsChanged->execute(ancestor, shared_from_this());
    }

    return shared_from_this();
}

Node::Ptr
Node::addChild(Node::Ptr child)
{
    if (child->_parent)
        child->_parent->removeChild(child);

    _children.push_back(child);

    child->_parent = shared_from_this();
    child->updateRoot();

    // bubble down
    auto descendants = NodeSet::create(child)->descendants(true);
    for (auto descendant : descendants->nodes())
        descendant->_added->execute(descendant, child, shared_from_this());

    // bubble up
    auto ancestors = NodeSet::create(shared_from_this())->ancestors(true);
    for (auto ancestor : ancestors->nodes())
        ancestor->_added->execute(ancestor, child, shared_from_this());

    return shared_from_this();
}

Node::Ptr
Node::removeChild(Node::Ptr child)
{
    auto it = std::find(_children.begin(), _children.end(), child);

    if (it == _children.end())
        throw std::invalid_argument("child");

    _children.erase(it);

    child->_parent = nullptr;
    child->updateRoot();

    // bubble down
    auto descendants = NodeSet::create(child)->descendants(true);
    for (auto descendant : descendants->nodes())
        descendant->_removed->execute(descendant, child, shared_from_this());

    // bubble up
    auto ancestors = NodeSet::create(shared_from_this())->ancestors(true);
    for (auto ancestor : ancestors->nodes())
        ancestor->_removed->execute(ancestor, child, shared_from_this());

    return shared_from_this();
}

Node::Ptr
Node::removeChildren()
{
    int numChildren = _children.size();

    for (int i = numChildren - 1; i >= 0; --i)
        removeChild(_children[i]);

    return shared_from_this();
}

bool
Node::contains(Node::Ptr node)
{
    return std::find(_children.begin(), _children.end(), node) != _children.end();
}

Node::Ptr
Node::addComponent(std::shared_ptr<AbstractComponent> component)
{
    if (!component)
        throw std::invalid_argument("component");

    if (hasComponent(component))
        throw std::logic_error("The same component cannot be added twice.");

    _components.push_back(component);
    component->_targets.push_back(shared_from_this());

    component->targetAdded()->execute(component, shared_from_this());

    // bubble down
    auto descendants = NodeSet::create(shared_from_this())->descendants(true);
    for (auto descendant : descendants->nodes())
        descendant->_componentAdded->execute(descendant, shared_from_this(), component);

    // bubble up
    auto ancestors = NodeSet::create(shared_from_this())->ancestors();
    for (auto ancestor : ancestors->nodes())
        ancestor->_componentAdded->execute(ancestor, shared_from_this(), component);

    return shared_from_this();
}

Node::Ptr
Node::removeComponent(std::shared_ptr<AbstractComponent> component)
{
    if (!component)
        throw std::invalid_argument("component");

	auto it = std::find(_components.begin(), _components.end(), component);

    if (it == _components.end())
        throw std::invalid_argument("component");

    _components.erase(it);
    component->_targets.erase(
        std::find(component->_targets.begin(), component->_targets.end(), shared_from_this())
    );

       component->targetRemoved()->execute(component, shared_from_this());

    // bubble down
    auto descendants = NodeSet::create(shared_from_this())->descendants(true);
    for (auto descendant : descendants->nodes())
        descendant->_componentRemoved->execute(descendant, shared_from_this(), component);

    // bubble up
    auto ancestors = NodeSet::create(shared_from_this())->ancestors();
    for (auto ancestor : ancestors->nodes())
        ancestor->_componentRemoved->execute(ancestor, shared_from_this(), component);

    return shared_from_this();
}

bool
Node::hasComponent(std::shared_ptr<AbstractComponent> component)
{
    return std::find(_components.begin(), _components.end(), component) != _components.end();
}

void
Node::updateRoot()
{
    _root = _parent ? (_parent->_root ? _parent->_root : _parent) : shared_from_this();
    _depth = _parent ? _parent->_depth + 1 : 0;

    for (auto child : _children)
        child->updateRoot();
}
