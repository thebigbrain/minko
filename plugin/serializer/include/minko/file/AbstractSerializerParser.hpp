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

#pragma once

#include "minko/SerializerCommon.hpp"
#include "minko/file/AbstractParser.hpp"
#include "minko/component/JobManager.hpp"
#include "minko/deserialize/Unpacker.hpp"

namespace minko
{
    namespace file
    {
        struct SceneVersion
        {
            int version;
            int major;
            int minor;
            int patch;
        };

        class AbstractSerializerParser:
            public AbstractParser
        {
        public:
            typedef std::shared_ptr<AbstractSerializerParser>                Ptr;
            typedef msgpack::type::tuple<unsigned int, short, std::string>    SerializedAsset;
            typedef std::shared_ptr<AssetLibrary>                           AssetLibraryPtr;
            typedef std::shared_ptr<Options>                                OptionsPtr;

        private:
            typedef std::shared_ptr<component::JobManager::Job>             JobPtr;
            typedef std::shared_ptr<Dependency>                             DependencyPtr;
            typedef std::function<void(
                unsigned short,
                AssetLibraryPtr,
                OptionsPtr,
                const std::string&,
                DependencyPtr,
                short,
                std::list<JobPtr>&
            )>                                                              AssetDeserializeFunction;

        protected:
            DependencyPtr                       _dependencies;
            std::shared_ptr<GeometryParser>     _geometryParser;
            std::shared_ptr<MaterialParser>     _materialParser;
            std::shared_ptr<TextureParser>      _textureParser;

            std::string                                                 _lastParsedAssetName;
            std::list<std::shared_ptr<component::JobManager::Job>>      _jobList;

            int                                                         _magicNumber;

            unsigned int                                                _fileSize;
            short                                                       _headerSize;
            unsigned int                                                _dependenciesSize;
            unsigned int                                                _sceneDataSize;

            SceneVersion                                            _version;

        private:
            static std::unordered_map<uint, AssetDeserializeFunction>   _assetTypeToFunction;

        public:
            virtual
            void
            parse(const std::string&                filename,
                  const std::string&                resolvedFilename,
                  std::shared_ptr<Options>          options,
                  const std::vector<unsigned char>& data,
                  AssetLibraryPtr                   assetLibrary);

            static
            void
            registerAssetFunction(uint assetTypeId, AssetDeserializeFunction f);

        protected:
            void
            extractDependencies(AssetLibraryPtr                         assetLibrary,
                                const std::vector<unsigned char>&       data,
                                short                                   dataOffset,
                                unsigned int                            dependenciesSize,
                                std::shared_ptr<Options>                options,
                                std::string&                            assetFilePath);

            inline
            void
            dependency(std::shared_ptr<Dependency> dependecies)
            {
                _dependencies = dependecies;
            }

        protected:
            AbstractSerializerParser();

            void
            deserializeAsset(SerializedAsset&                           asset,
                              AssetLibraryPtr                           assetLibrary,
                              std::shared_ptr<Options>                  options,
                              std::string&                              assetFilePath);

            std::string
            extractFolderPath(const std::string& filepath);

            bool
            readHeader(const std::string&                               filename,
                       const std::vector<unsigned char>&                data,
                       int                                              extension = 0x00);

            int
            readInt(const std::vector<unsigned char>& data, int offset)
            {
                return (int)(data[offset] << 24 | data[offset + 1] << 16 | data[offset + 2] << 8 | data[offset + 3]);
            }

            unsigned int
            readUInt(const std::vector<unsigned char>& data, int offset)
            {
                return (unsigned int)(data[offset] << 24 | data[offset + 1] << 16 | data[offset + 2] << 8 | data[offset + 3]);
            }

            short
            readShort(const std::vector<unsigned char>& data, int offset)
            {
                return (short)(data[offset] << 8 | data[offset + 1]);
            }

            static
            void
            deserializeTexture(unsigned short metaData,
                               AssetLibraryPtr assetLibrary,
                               OptionsPtr           options,
                               const std::string&   assetCompletePath,
                               DependencyPtr dependency,
                               short assetId,
                               std::list<JobPtr>& jobs);
        };
    }
}
