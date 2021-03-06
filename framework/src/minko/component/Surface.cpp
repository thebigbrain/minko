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

#include "minko/component/Surface.hpp"

#include "minko/scene/Node.hpp"
#include "minko/geometry/Geometry.hpp"
#include "minko/render/Effect.hpp"
#include "minko/render/DrawCall.hpp"
#include "minko/render/Pass.hpp"
#include "minko/render/Program.hpp"
#include "minko/data/Container.hpp"
#include "minko/data/ArrayProvider.hpp"
#include "minko/component/Renderer.hpp"
#include "minko/CloneOption.hpp"
#include "minko/material/Material.hpp"

using namespace minko;
using namespace minko::scene;
using namespace minko::material;
using namespace minko::data;
using namespace minko::component;
using namespace minko::geometry;
using namespace minko::render;

Surface::Surface(std::string                name,
                 Geometry::Ptr                 geometry,
				 material::Material::Ptr	material,
                 Effect::Ptr                effect,
                 const std::string&            technique) :
    AbstractComponent(),
    _name(name),
    _geometry(geometry),
    _material(material),
    _effect(effect),
    _technique(technique),
    _visible(true),
    _rendererToVisibility(),
    _rendererToComputedVisibility(),
    _techniqueChanged(TechniqueChangedSignal::create()),
    _visibilityChanged(VisibilityChangedSignal::create()),
    _computedVisibilityChanged(VisibilityChangedSignal::create()),
    _targetAddedSlot(nullptr),
    _targetRemovedSlot(nullptr),
    _addedSlot(nullptr),
    _removedSlot(nullptr)
{
    if (_effect == nullptr)
        throw std::invalid_argument("effect");
    if (!_effect->hasTechnique(_technique))
        throw std::logic_error("Effect does not provide a '" + _technique + "' technique.");
}

Surface::Surface(const Surface& surface, const CloneOption& option) :
	AbstractComponent(surface, option),
	_name(surface._name),
	_geometry(surface._geometry), //needed for skinning: option == CloneOption::SHALLOW ? surface._geometry : surface._geometry->clone()
	_material(option == CloneOption::SHALLOW ? surface._material : std::static_pointer_cast<Material>(surface._material->clone())),
	_effect(surface._effect),
	_technique(surface._technique),
	_visible(surface._visible),
	_rendererToVisibility(surface._rendererToVisibility),
	_rendererToComputedVisibility(surface._rendererToComputedVisibility),
	_techniqueChanged(TechniqueChangedSignal::create()),
	_visibilityChanged(VisibilityChangedSignal::create()),
	_computedVisibilityChanged(VisibilityChangedSignal::create()),
	_targetAddedSlot(nullptr),
	_targetRemovedSlot(nullptr),
	_addedSlot(nullptr),
	_removedSlot(nullptr)
{
	if (_effect == nullptr)
		throw std::invalid_argument("effect");
	if (!_effect->hasTechnique(_technique))
		throw std::logic_error("Effect does not provide a '" + _technique + "' technique.");
}

AbstractComponent::Ptr
Surface::clone(const CloneOption& option)
{
	Ptr surface(new Surface(*this, option));

	surface->initialize();

	return surface;
}

void
Surface::initialize()
{
    _targetAddedSlot = targetAdded()->connect(std::bind(
        &Surface::targetAddedHandler,
        std::static_pointer_cast<Surface>(shared_from_this()),
        std::placeholders::_1,
        std::placeholders::_2
    ));

    _targetRemovedSlot = targetRemoved()->connect(std::bind(
        &Surface::targetRemovedHandler,
        std::static_pointer_cast<Surface>(shared_from_this()),
        std::placeholders::_1,
        std::placeholders::_2
    ));
}

void
Surface::targetAddedHandler(AbstractComponent::Ptr    ctrl,
                            scene::Node::Ptr        target)

{
    auto targetData    = target->data();

    _addedSlot = target->added()->connect(std::bind(
        &Surface::addedHandler,
        std::static_pointer_cast<Surface>(shared_from_this()),
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3
    ));

    _removedSlot = target->removed()->connect(std::bind(
        &Surface::removedHandler,
        std::static_pointer_cast<Surface>(shared_from_this()),
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3
    ));

	auto arrayProviderMaterial = std::static_pointer_cast<data::ArrayProvider>(_material);

    if (arrayProviderMaterial)
		targetData->addProvider(std::static_pointer_cast<data::ArrayProvider>(_material));
    else
		targetData->addProvider(std::static_pointer_cast<data::ArrayProvider>(_material));

    targetData->addProvider(_geometry->data());
    targetData->addProvider(_effect->data());
}

void
Surface::targetRemovedHandler(AbstractComponent::Ptr    ctrl,
                              scene::Node::Ptr            target)
{
    auto data = target->data();

    _removedSlot    = nullptr;

	data->removeProvider(std::static_pointer_cast<data::ArrayProvider>(_material));
    data->removeProvider(_geometry->data());
    data->removeProvider(_effect->data());
}

void
Surface::addedHandler(Node::Ptr, Node::Ptr target, Node::Ptr)
{
}

void
Surface::removedHandler(Node::Ptr, Node::Ptr target, Node::Ptr)
{
}

void
Surface::geometry(geometry::Geometry::Ptr newGeometry)
{
    for (unsigned int i = 0; i < targets().size(); ++i)
    {
        std::shared_ptr<scene::Node> target = targets()[i];

        target->data()->removeProvider(_geometry->data());
        target->data()->addProvider(newGeometry->data());
    }

    _geometry = newGeometry;
}

void
Surface::effect(render::Effect::Ptr        effect,
                const std::string&        technique)
{
    setEffectAndTechnique(effect, technique, true);
}

void
Surface::visible(component::Renderer::Ptr    renderer,
                 bool                        value)
{
    if (visible(renderer) != value)
    {
        _rendererToVisibility[renderer] = value;
        _visibilityChanged->execute(
            std::static_pointer_cast<Surface>(shared_from_this()),
            renderer,
            value
        );
    }
}

void
Surface::computedVisibility(component::Renderer::Ptr    renderer,
                            bool                        value)
{
    if (computedVisibility(renderer) != value)
    {
        _rendererToComputedVisibility[renderer] = value;
        _computedVisibilityChanged->execute(
            std::static_pointer_cast<Surface>(shared_from_this()),
            renderer,
            value
        );
    }
}

void
Surface::setEffectAndTechnique(Effect::Ptr            effect,
                               const std::string&    technique,
                               bool                    updateDrawcalls)
{
    if (_effect == effect && _technique == technique)
        return;

    if (effect == nullptr)
        throw std::invalid_argument("effect");
    if (!effect->hasTechnique(technique))
        throw std::logic_error("Effect does not provide a '" + technique + "' technique.");

    for (auto& n : targets())
    {
        n->data()->removeProvider(_effect->data());
        n->data()->addProvider(effect->data());
    }

    _effect        = effect;
    _technique    = technique;

    _techniqueChanged->execute(
        std::static_pointer_cast<Surface>(shared_from_this()),
        _technique,
        updateDrawcalls
    );
}