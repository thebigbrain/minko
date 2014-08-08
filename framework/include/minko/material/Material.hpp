/*
Copyright (c) 2013 Aerys

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

#pragma once

#include "minko/Common.hpp"

#include "minko/data/Provider.hpp"

namespace minko
{
	namespace material
	{
		class Material :
			public data::Provider
		{
		public:
			typedef std::shared_ptr<Material>	Ptr;

        private:
            std::string _name;

		public:
			inline static
			Ptr
			create(const std::string& name = "material")
			{
				return std::shared_ptr<Material>(new Material(name));
			}

			inline static
			Ptr
			create(Ptr source)
			{
				auto mat = create();

				mat->copyFrom(source);
                mat->_name = source->_name;

				return mat;
			}

            inline
            const std::string&
            name()
            {
                return _name;
            }

		protected:
			Material::Material(const std::string& name) :
	            data::Provider(),
                _name(name)
            {
	
            }
		};
	}
}
