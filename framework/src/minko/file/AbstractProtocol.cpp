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

#include "minko/file/AbstractProtocol.hpp"

#include "minko/file/Options.hpp"

using namespace minko;
using namespace minko::file;

AbstractProtocol::AbstractProtocol() :
    _file(File::create()),
    _options(Options::create()),
    _complete(Signal<Ptr>::create()),
    _progress(Signal<Ptr, float>::create()),
    _error(Signal<Ptr>::create())
{
}

std::string
AbstractProtocol::sanitizeFilename(const std::string& filename)
{
    auto f = filename;
    auto a = '\\';

    for (auto pos = f.find_first_of(a);
         pos != std::string::npos;
         pos = f.find_first_of(a))
    {
        f = f.replace(pos, 1, 1, '/');
    }

    return f;
}
