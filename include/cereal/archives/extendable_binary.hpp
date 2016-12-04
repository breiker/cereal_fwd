/*! \file extendable_binary.hpp
    \brief Binary input and output archives with backward and forward compatiblity */
/*
  Copyright (c) 2016, Randolph Voorhies, Shane Grant, Michal Breiter
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of cereal nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL RANDOLPH VOORHIES OR SHANE GRANT OR MICHAL BREITER BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef CEREAL_ARCHIVES_EXTENDABLE_BINARY_HPP_
#define CEREAL_ARCHIVES_EXTENDABLE_BINARY_HPP_

#include <cereal/cereal.hpp>
#include <sstream>
#include <limits>

#include <cereal/details/extendable_binary_details.hpp>

namespace cereal
{
  // ######################################################################
  //! An output archive designed to save data in a compact binary representation portable over different architectures
  /*! This archive outputs data to a stream in an compact binary representation but some metadata is needed to support
   *  forward and backward compatibility.

      This archive will save data in little endian order. Most of today's architectures are little endian
      so byte swap is needed only for big endian architectures.

      When using a binary archive and a file stream, you must use the
      std::ios::binary format flag to avoid having your data altered
      inadvertently. This is especially important on Windows since on Linux it's noop.

    \ingroup Archives */
  class ExtendableBinaryOutputArchive : public OutputArchive<ExtendableBinaryOutputArchive, Flags::ForwardSupport>
  {
    public:
      //! A class containing various advanced options for the ExtendableBinaryOutput archive
      class Options
      {
        public:
          //! Represents desired endianness
          enum class Endianness : std::uint8_t
          { big, little };

          //! Default options, preserve system endianness
          static Options Default(){ return Options(); }

          //! Save as little endian
          static Options LittleEndian(){ return Options( Endianness::little ); }

          //! Save as big endian
          static Options BigEndian(){ return Options( Endianness::big ); }

          //! Specify specific options for the ExtendableBinaryOutputArchive
          /*! @param outputEndian The desired endianness of saved (output) data */
          explicit Options( Endianness outputEndian = getEndianness() ) :
            itsOutputEndianness( outputEndian ) { }

        private:
          //! Gets the endianness of the system
          inline static Endianness getEndianness()
          { return extendable_binary_detail::is_little_endian() ? Endianness::little : Endianness::big; }

          //! Checks if Options is set for little endian
          inline std::uint8_t is_little_endian() const
          { return itsOutputEndianness == Endianness::little; }

          friend class ExtendableBinaryOutputArchive;
          Endianness itsOutputEndianness;
      };

      //! Construct, outputting to the provided stream
      /*! @param stream The stream to output to. Should be opened with std::ios::binary flag.
          @param options The ExtendableBinary specific options to use.  See the Options struct
                         for the values of default parameters */
      ExtendableBinaryOutputArchive(std::ostream & stream, Options const & options = Options::Default()) :
        OutputArchive<ExtendableBinaryOutputArchive, Flags::ForwardSupport>(this),
        itsStream(stream),
        itsConvertEndianness( extendable_binary_detail::is_little_endian() ^ options.is_little_endian() )
      {
        const auto littleEndian = options.is_little_endian();
        this->saveBinary<sizeof(std::uint8_t)>( &littleEndian, sizeof(std::uint8_t) );
      }

      ~ExtendableBinaryOutputArchive() CEREAL_NOEXCEPT = default;

      //! Writes size bytes of data to the output stream
      template <std::size_t DataSize> inline
      void saveBinary( const void * data, std::size_t size )
      {
        std::size_t writtenSize = 0;

        if( itsConvertEndianness )
        {
          for( std::size_t i = 0; i < size; i += DataSize )
            for( std::size_t j = 0; j < DataSize; ++j )
              writtenSize += static_cast<std::size_t>( itsStream.rdbuf()->sputn( reinterpret_cast<const char*>( data ) + DataSize - j - 1 + i, 1 ) );
        }
        else
          writtenSize = static_cast<std::size_t>( itsStream.rdbuf()->sputn( reinterpret_cast<const char*>( data ), size ) );

        if(writtenSize != size)
          throw Exception("Failed to write " + std::to_string(size) + " bytes to output stream! Wrote " + std::to_string(writtenSize));
      }

      //! Writes varint to the stream
      /*! Based on protobuf usage */
      template <class T>
      void saveVarint(T v)
      {
        static_assert(sizeof(T) <= (extendable_binary_detail::maxVarintSize*7)/8, "value is to big to be saved as varint");
        static_assert(std::is_unsigned<T>::value, "only unsigned varints are supported");
        std::array<std::uint8_t, extendable_binary_detail::maxVarintSize> buffer;
        std::size_t size = 0;
        while (v > 0x7F) {
          buffer[size] = (static_cast<std::uint8_t>(v) & 0x7f) | 0x80;
          v >>= 7;
          ++size;
        }
        buffer[size] = static_cast<std::uint8_t>(v) & 0x7f;
        // we don't want bit swap here
        saveBinary<sizeof(std::uint8_t)>(buffer.data(), size + 1);
      }

      void saveObjectBeginning()
      {
        // if object to save - save it
        if(objectDataNeedsSaving) {
          saveObjectData(false);
        }
        objectDataNeedsSaving = true;
      }

      void saveClassVersion(std::uint32_t version)
      {
        classVersion = version;
      }

      void saveObjectId(std::uint32_t objectId_)
      {
        isPointer = true;
        objectId = objectId_;
      }

      void savePointerValidityTag(bool /*valid*/)
      {
        isPointer = true;
      }

      void savePolymorphicId(std::int32_t polymorphicId_)
      {
        isPointer = true;
        polymorphicId = polymorphicId_;
      }

      void savePolymorphicName(const std::string& name)
      {
        polymorphicName = name;
      }

      void savingOtherField()
      {
        if(objectDataNeedsSaving) {
          saveObjectData(false);
        }
      }

      //! Writes object properites to stream. Called in prologue for objects
      void saveObjectEnd()
      {
        if(objectDataNeedsSaving) {
          // no other fields were saved we have to save objectData with empty marker
          saveObjectData(true);
        } else{
          // there were fields in class, have to save end marker
          saveEndMarker();
          /* TODO we are saving last_field marker for pointers even if it's not needed
           * Easiest way to solve it wold be to make queue of save object types in archive to know that we don't have to save it.
           * But that would mean that we would have to allocate some memory. We could also make specializations for prologue for pointer types
           * but that would mean that we make specializations for pointer types.
           * We could make generic wrappers around pointers and specialize for them.
           */
        }
      }

    private:

      //! Writes objectData to stream
      /*! @param endOfObject if it's object end and we didn't save any objects we can ship */
      void saveObjectData(bool endOfObject)
      {
        using namespace extendable_binary_detail;
        if(isPointer)  {
          PointerMarkers finalMarker = PointerMarkers::None;
          if(objectId > 0) {
            finalMarker |= PointerMarkers::IsSharedPtr;
          }
          if(polymorphicId > 0) {
            finalMarker |= PointerMarkers::IsPolymorphicPointer;
          }
          if(endOfObject) {
            finalMarker |= PointerMarkers::Empty;
          }
          std::uint8_t t = writeType(FieldType::pointer, static_cast<std::uint8_t>(finalMarker));
          saveBinary<sizeof(std::uint8_t)>(&t, sizeof(std::uint8_t));
          if(objectId > 0) {
            saveVarint(objectId);
          }
          if(polymorphicId > 0) {
            saveBinary<sizeof(std::int32_t)>(&polymorphicId, sizeof(std::int32_t));
            // maybe simple save binary
            // information if new
            // information if polymorphic
            // 0 is unused so simple negate msb and add 1 would be sufficient)
          }
          if(false == polymorphicName.empty()) {
            saveVarint(polymorphicName.size());
            saveBinary<sizeof(decltype(polymorphicName)::value_type)>( polymorphicName.c_str(),
                polymorphicName.size() * sizeof(decltype(polymorphicName)::value_type));
          }
        } else {
          ClassMarkers finalMarker = ClassMarkers::None;
          if(classVersion > 0) {
            finalMarker |= ClassMarkers::HasVersion;
          }
          if(endOfObject) {
            finalMarker |= ClassMarkers::EmptyClass;
          }
          std::uint8_t t = writeType(FieldType::class_t, static_cast<std::uint8_t>(finalMarker));
          saveBinary<sizeof(std::uint8_t)>(&t, sizeof(std::uint8_t));
          // save needed data
          if(classVersion > 0) {
            saveVarint(classVersion);
          }
        }
        // reset variables
        objectDataNeedsSaving = false;
        classVersion = 0;
        isPointer = false;
        objectId = 0;
        // version is only saved when method has version argument so we have to reset it
        polymorphicId = 0;
        polymorphicName.clear();
      }

      //! Writes last_field marker to stream
      void saveEndMarker()
      {
        using namespace extendable_binary_detail;
        std::uint8_t t = writeType(FieldType::last_field, 0);
        saveBinary<sizeof(std::uint8_t)>(&t, sizeof(std::uint8_t));
      }

    private:
      bool objectDataNeedsSaving = false;
      //bool savedSomeFields = false;
      std::uint32_t classVersion = 0;
      bool isPointer = false;
      std::uint32_t objectId = 0;
      std::uint32_t polymorphicId = 0;
      std::string polymorphicName;

      std::ostream & itsStream;
      const uint8_t itsConvertEndianness; //!< If set to true, we will need to swap bytes upon saving
  };

  // ######################################################################
  //! An input archive designed to load data saved using ExtendableBinaryOutputArchive
  /*! This archive outputs data to a stream in an extremely compact binary
      representation with as little extra metadata as possible.

      This archive will load the endianness of the serialized data and
      if necessary transform it to match that of the local machine.  This comes
      at a significant performance cost compared to non portable archives if
      the transformation is necessary, but causes no performance hit if swapping is not necessary.


      The archive will save all integer values as variadic integegers. Exception () will be thrown when on deserialization
      value will be bigger than can be stored to target field. It is still recommended to use fixed size integers.

      When using a binary archive and a file stream, you must use the
      std::ios::binary format flag to avoid having your data altered
      inadvertently.

    \ingroup Archives */
  class ExtendableBinaryInputArchive : public InputArchive<ExtendableBinaryInputArchive, Flags::ForwardSupport>
  {
    public:
      //! A class containing various advanced options for the ExtendableBinaryInput archive
      class Options
      {
        public:
          //! Represents desired endianness
          enum class Endianness : std::uint8_t
          { big, little };

          //! Default options, preserve system endianness
          static Options Default(){ return Options(); }

          //! Load into little endian
          static Options LittleEndian(){ return Options( Endianness::little ); }

          //! Load into big endian
          static Options BigEndian(){ return Options( Endianness::big ); }

          //! Specify specific options for the ExtendableBinaryInputArchive
          /*! @param inputEndian The desired endianness of loaded (input) data */
          explicit Options( Endianness inputEndian = getEndianness() ) :
            itsInputEndianness( inputEndian ) { }

        private:
          //! Gets the endianness of the system
          inline static Endianness getEndianness()
          { return extendable_binary_detail::is_little_endian() ? Endianness::little : Endianness::big; }

          //! Checks if Options is set for little endian
          inline std::uint8_t is_little_endian() const
          { return itsInputEndianness == Endianness::little; }

          friend class ExtendableBinaryInputArchive;
          Endianness itsInputEndianness;
      };

      //! Construct, loading from the provided stream
      /*! @param stream The stream to read from. Should be opened with std::ios::binary flag.
          @param options The ExtendableBinary specific options to use.  See the Options struct
                         for the values of default parameters */
      ExtendableBinaryInputArchive(std::istream & stream, Options const & options = Options::Default()) :
        InputArchive<ExtendableBinaryInputArchive, Flags::ForwardSupport>(this),
        sharedObjectStream(std::ios::binary | std::ios::in | std::ios::out),
        itsStream(stream, sharedObjectStream),
        itsConvertEndianness( false )
      {
        uint8_t streamLittleEndian;
        this->loadBinary<sizeof(std::uint8_t)>( &streamLittleEndian, sizeof(std::uint8_t));
        itsConvertEndianness = options.is_little_endian() ^ streamLittleEndian;
      }

      ~ExtendableBinaryInputArchive() CEREAL_NOEXCEPT = default;

      //! Reads size bytes of data from the input stream
      /*! @param data The data to save
          @param size The number of bytes in the data
          @tparam DataSize T The size of the actual type of the data elements being loaded */
      template <std::size_t DataSize> inline
      void loadBinary( void * const data, std::size_t size )
      {
        // load data
        auto const readSize = itsStream.readBinary( reinterpret_cast<char*>( data ), size );
        if(false == savedShared.saving.empty()) {
          sharedObjectStream.write(reinterpret_cast<char*>(data), size);
        }

        if(readSize != size)
          throw Exception("Failed to read " + std::to_string(size) + " bytes from input stream! Read " + std::to_string(readSize));

        // flip bits if needed
        if( itsConvertEndianness )
        {
          std::uint8_t * ptr = reinterpret_cast<std::uint8_t*>( data );
          for( std::size_t i = 0; i < size; i += DataSize )
            extendable_binary_detail::swap_bytes<DataSize>( ptr + i );
        }
      }
      
      //! Discards size bytes from the input stream
      /*! @param size The number of bytes in the data
          Throws if not enough bytes are read */
      inline void skipData(std::size_t size) {
        if(savedShared.saving.empty()) {
          itsStream.skipData(size);
        } else {
          itsStream.readToOtherStream(size, sharedObjectStream);
        }
      }

      //! Load type tag from input stream
      /*! @return if type is not ommited_field */
      inline bool loadTypeTag()
      {
        using namespace extendable_binary_detail;
        std::uint8_t v;
        loadBinary<sizeof(std::uint8_t)>(&v, sizeof(std::uint8_t));
        lastTypeTag = extendable_binary_detail::readType(v);
        return lastTypeTag.first != FieldType::omitted_field;
      }
      //! Gets last type tag from input stream
      /*! @tparam expected_type type which is expected to be loaded
          Throws if type is not expected or omitted */
      template <extendable_binary_detail::FieldType expected_type>
      inline auto getTypeTag() -> decltype(extendable_binary_detail::readType(std::uint8_t{}))
      {
        using namespace extendable_binary_detail;
        static_assert(expected_type != FieldType::last_field, "should go to loadEndOfClass");
        if(lastTypeTag.first != expected_type && lastTypeTag.first != FieldType::omitted_field)
          throw Exception("Loaded wrong lastTypeTag, expected: " + std::to_string(static_cast<int>(expected_type))
                          + " got: " + std::to_string(static_cast<int>(lastTypeTag.first)) );
        return lastTypeTag;
      }

      //! Gets last type tag from input stream
      /*! @tparam expected_type type which is expected to be loaded
          Doesn't throw exception on type different than expected_type @see getTypeTag
          TODO varargs, template argument is unusd now */
      template <extendable_binary_detail::FieldType expected_type>
      inline auto getTypeTagNoError() -> decltype(extendable_binary_detail::readType(std::uint8_t{}))
      {
        using namespace extendable_binary_detail;
        static_assert(expected_type != FieldType::last_field, "should go to loadEndOfClass");
        return lastTypeTag;
      }

      //! Load varint from the stream
      /*! Based on protobuf usage */
      template <class T>
      inline void loadVarint(T& v)
      {
        static_assert(sizeof(T) <= (extendable_binary_detail::maxVarintSize*7)/8, "value is to big to be a varint");
        static_assert(std::is_unsigned<T>::value, "only unsigned varints are supported");
#if 0
        std::uint8_t first;
          std::array<std::uint8_t, extendable_binary_detail::maxVarintSize> buffer;
          buffer.fill(0);
          std::size_t size = 1;
          loadBinary<sizeof(std::uint8_t)>(&first, sizeof(std::uint8_t));
          buffer[size - 1] = first & 0x7f;
          while(first >= 0x80 && size < extendable_binary_detail::maxVarintSize) {
            loadBinary<sizeof(std::uint8_t)>(&first, sizeof(std::uint8_t));
            buffer[size - 1] |= (first & 0x7f) << (7 * size);
            buffer[size]     |= (first & 0x7f) >> (size);
            ++size;
          }
          for(std::size_t i = sizeof(T); i <= size; i++) {
            if(buffer[i] > 0) {
              throw Exception("Varint is to big to be loaded" + std::to_string(size) + " dest:" + std::to_string(sizeof(v)) + " at:" + std::to_string(i));
            }
          }
          // TODO limits
          std::memcpy(&v, buffer.data(), sizeof(T));
#endif
        std::uint32_t f = 0, s = 0;
        auto load = [&]() {
          std::uint32_t f1 = 0, f2 = 0, f3 = 0, f4 = 0, s1 = 0, s2 = 0, s3 = 0, s4 = 0, s5 = 0, s6 = 0;
          loadBinary<sizeof(std::uint8_t)>(&f1, sizeof(std::uint8_t));
          if(f1 < 0x80) {
            f = f1;
            return;
          }
          loadBinary<sizeof(std::uint8_t)>(&f2, sizeof(std::uint8_t));
          if(f2 < 0x80) {
            f = (f1 - 0x80) | (f2 << 7);
            return;
          }
          loadBinary<sizeof(std::uint8_t)>(&f3, sizeof(std::uint8_t));
          if(f3 < 0x80) {
            f = (f1 - 0x80) | ((f2 - 0x80) << 7) | (f3 << 14);
            return;
          }
          loadBinary<sizeof(std::uint8_t)>(&f4, sizeof(std::uint8_t));
          if(f4 < 0x80) {
            f = (f1 - 0x80) | ((f2 - 0x80) << 7) | ((f3 - 0x80) << 14) | (f4 << 21);
            return;
          }
          loadBinary<sizeof(std::uint8_t)>(&s1, sizeof(std::uint8_t));
          if(s1 < 0x80) {
            f = (f1 - 0x80) | ((f2 - 0x80) << 7) | ((f3 - 0x80) << 14) | ((f4 - 0x80) << 21) | (s1 << 28);
            s = s1 >> 3;
            return;
          }
          loadBinary<sizeof(std::uint8_t)>(&s2, sizeof(std::uint8_t));
          if(s2 < 0x80) {
            f = (f1 - 0x80) | ((f2 - 0x80) << 7) | ((f3 - 0x80) << 14) | ((f4 - 0x80) << 21) | ((s1 - 0x80) << 28);
            s = ((s1 - 0x80) >> 4) | (s2 << 3);
            return;
          }
          loadBinary<sizeof(std::uint8_t)>(&s3, sizeof(std::uint8_t));
          if(s3 < 0x80) {
            f = (f1 - 0x80) | ((f2 - 0x80) << 7) | ((f3 - 0x80) << 14) | ((f4 - 0x80) << 21) | ((s1 - 0x80) << 28);
            s = ((s1 - 0x80) >> 4) | ((s2 - 0x80) << 3) | s3 << 10;
            return;
          }
          loadBinary<sizeof(std::uint8_t)>(&s4, sizeof(std::uint8_t));
          if(s4 < 0x80) {
            f = (f1 - 0x80) | ((f2 - 0x80) << 7) | ((f3 - 0x80) << 14) | ((f4 - 0x80) << 21) | ((s1 - 0x80) << 28);
            s = ((s1 - 0x80) >> 4) | ((s2 - 0x80) << 3) | ((s3 - 0x80) << 10) | s4 << 17;
            return;
          }
          loadBinary<sizeof(std::uint8_t)>(&s5, sizeof(std::uint8_t));
          if(s5 < 0x80) {
            f = (f1 - 0x80) | ((f2 - 0x80) << 7) | ((f3 - 0x80) << 14) | ((f4 - 0x80) << 21) | ((s1 - 0x80) << 28);
            s = ((s1 - 0x80) >> 4) | ((s2 - 0x80) << 3) | ((s3 - 0x80) << 10) | ((s4 - 0x80) << 17) | s5 << 24;
            return;
          }
          loadBinary<sizeof(std::uint8_t)>(&s6, sizeof(std::uint8_t));
          if(s6 < 0x80) {
            f = (f1 - 0x80) | ((f2 - 0x80) << 7) | ((f3 - 0x80) << 14) | ((f4 - 0x80) << 21) | ((s1 - 0x80) << 28);
            s = ((s1 - 0x80) >> 4) | ((s2 - 0x80) << 3)  | ((s3 - 0x80) << 10) | ((s4 - 0x80) << 17) | ((s5 - 0x80) << 24) | s6 << 31;
            return;
          } else {
            throw Exception("To big varint");
          }
        };
        load();
        if(sizeof(T) == 8) {
          std::uint64_t dest = s;
          dest <<= (sizeof(s)*8);
          dest |= f;
          std::memcpy(&v, &dest, sizeof(v));
        } else {
          std::memcpy(&v, &f, sizeof(v));
        }
      }

      //! Load varint from the stream, discard input value
      inline void skipVarint()
      {
        // TODO can implement faster approach, without interpreting
        std::uint64_t ignore;
        loadVarint(ignore);
    }

      inline void loadObjectBeginning()
      {
        resetObjectDetails();
        // TODO omitted class
        // TODO block serialize
        using namespace extendable_binary_detail;
        const auto type = getTypeTagNoError<FieldType::class_t>();
        switch(type.first) {
          case FieldType::class_t: {
            loadClassData(type.second);
            break;
          }
          case FieldType::pointer: {
            loadPointerData(type.second);
            break;
          }
          case FieldType::omitted_field: {
            throw Exception("TODO omitted class");
          }
          default: {
            throw Exception("Unexpected type expected class or pointer, got:" + std::to_string(static_cast<int>(type.first)));
          }
        }
      }

      inline void loadEndOfObjectData()
      {
        if(emptyClass) {
          // empty class or shared pointer with object which was already saved
        } else {
          loadTypeTag();
          loadEndOfClass(true);
        }
        resetObjectDetails();
      }

      inline void loadOmittedObject()
      {
        loadEndOfClass(false);
      }

      inline std::uint32_t getLoadedClassVersion() const
      {
        return classVersion;
      }

      inline std::int32_t getLoadedPolymorphicId() const
      {
        return polymorphicId;
      }

      inline const std::string& getLoadedPolymorphicName() const
      {
        return polymorphicName;
      }

      inline std::uint32_t getLoadedObjectId() const
      {
        return objectId;
      }

      inline bool getLoadedPointerValidity() const
      {
        return false == emptyClass;
      }

      inline void setLastSizeTag(std::size_t size)
      {
        lastSizeTag = size;
      }

      inline std::size_t getLastSizeTag() const
      {
        return lastSizeTag;
      }

    private:

      struct SavedShared {
        std::vector<std::pair<std::uint32_t, extendable_binary_detail::StreamPos>> saving; // alternatively can use stack and recursion
        std::map<std::uint32_t, extendable_binary_detail::StreamPos> saved;
        std::set<std::uint32_t> loaded;
      };

      inline void resetObjectDetails()
      {
        // should be reset on all fields?
        classVersion = 0;
        emptyClass = false; // TODO when reset?
        objectId = 0;
        polymorphicId = 0;
        polymorphicName.clear();
      }

      inline void loadClassData(std::uint8_t classMarkers)
      {
        using namespace extendable_binary_detail;
        ClassMarkers markers = static_cast<ClassMarkers>(classMarkers);
        if(markers & ClassMarkers::EmptyClass) {
          emptyClass = true;
        }
        if(markers & ClassMarkers::HasVersion) {
          loadVarint(classVersion);
        }
      }

      void loadSharedPointer()
      {
        const auto normalObjectId = objectId & ~detail::msb_32bit;
        const auto wasSkipped = savedShared.saved.find(normalObjectId);
        // usual path
        if(wasSkipped == savedShared.saved.end())
          return;

        const bool isNewObjectInStream = (objectId & detail::msb_32bit) != 0;
        const auto alreadyLoaded = savedShared.loaded.find(normalObjectId);
        if (false == isNewObjectInStream) {
          /* Object was not loaded before according to stream order. */
          if (alreadyLoaded == savedShared.loaded.end()) {
            // TODO we can delete it here (if we made a copy)
            savedShared.loaded.emplace(normalObjectId);
            // we change objectId to indicate that we want to load it now
            objectId = objectId | detail::msb_32bit;
            pushLoadShared(wasSkipped->second);
            emptyClass = false; // unneeded redundancy?
          }
        } else if (alreadyLoaded == savedShared.loaded.end()) {
          /* NewObjectInStream, was skipped and not loaded before.
           * Here we are loading it with stream order (stream indicates that it's new object) so there's no need to
           * push additional stream position, it is naturally next in stream.
           * We just have to mark that that object is now being loaded so we don't load it again and make duplicate with
           * different address. */
          savedShared.loaded.emplace(normalObjectId);
        } else if (alreadyLoaded != savedShared.loaded.end()) {
          /* NewObjectInStream, was skipped but was loaded before.
           * We don't want to load it for the second time. We have move forward in the stream to the end of object. */
          objectId = normalObjectId;
          emptyClass = true;
          // move forward
          itsStream.skipData(wasSkipped->second.end - wasSkipped->second.start);
        }
      }

      inline void loadPointerData(std::uint8_t pointerMarkers)
      {
        using namespace extendable_binary_detail;
        PointerMarkers markers = static_cast<PointerMarkers>(pointerMarkers);
        if (markers & PointerMarkers::Empty) {
          emptyClass = true;
        }
        if (markers & PointerMarkers::IsSharedPtr) {
          loadVarint(objectId);
          loadSharedPointer();
        }
        if (markers & PointerMarkers::IsPolymorphicPointer) {
          loadBinary<sizeof(std::int32_t)>(&polymorphicId, sizeof(std::int32_t));
          if (polymorphicId & detail::msb_32bit) { // TODO change msb to lsb?
            std::uint32_t nameSize;
            loadVarint(nameSize);
            // TODO limit max size for safety
            polymorphicName.resize(nameSize);
            using char_type = decltype(polymorphicName)::value_type;
            loadBinary<sizeof(char_type)>(const_cast<char_type *>(polymorphicName.data()),
                                          nameSize * sizeof(char_type));
            // TODO change to uint8_t (may not match on sending side)
            // normally it would be multiply by one, but on other platforms we could just have problems
            // TODO const_cast doesn't seem to be best idea, we know that variable is not const but it just doesn't seem right
            //      Similar code is used in string serialization
            //      Do we have warranty that cow is disabled (on all platforms)?
          }
        }
      }

      //! Skip data in archive loading at least one element until class depth 0 is reached.
      /*! At the beginning if isInObject is set to true depth is assumed to be 1, 0 otherwise.
          That means that if isInObject is set to truedata will be skipped untill FieldType::last_field
          is reached for same depth.
          If isInObject is set to false at least one field will be loaded.*/
      inline void loadEndOfClass(bool isInObject)
      {
        using namespace extendable_binary_detail;
        using return_type = decltype(extendable_binary_detail::readType(std::uint8_t{}));
        int class_depth = isInObject ? 1 : 0; // we are in an object
        std::size_t lastIgnoredSizeTag = 0; // TODO use other size type?

        bool firstPass = true;
        do {
          if(false == firstPass) {
            loadTypeTag();
          }
          firstPass = false;

          return_type type = getTypeTagNoError<FieldType::class_t>();
          switch (type.first) {
            case FieldType::positive_integer:
            case FieldType::negative_integer: {
              skipData(getIntSizeFromTagSize(type.second));
              break;
            }
            case FieldType::floating_point: {
              skipData(getFloatSizeFromTagSize(type.second));
              break;
            }
            case FieldType::integer_packed:
              break; // one byte
            case FieldType::omitted_field:
              break; // one byte
            case FieldType::last_field: {
              /* what we expected, but only if we didn't go into next class_field */
              if(isSkippedSharedObjectEnd(class_depth)) {
                popSaveShared();
              }
              --class_depth;
              break; // one byte
            }
            case FieldType::class_t: {
              ClassMarkers markers = static_cast<ClassMarkers>(type.second);
              if(false == (markers & ClassMarkers::EmptyClass)) {
                ++class_depth;
              }
              if(markers & ClassMarkers::HasVersion) {
                skipVarint();
              }
              break;
            }
            case FieldType::pointer: {
              PointerMarkers markers = static_cast<PointerMarkers>(type.second);
              if(false == (markers & PointerMarkers::Empty)) {
                ++class_depth;
              }
              if(markers & PointerMarkers::IsSharedPtr) {
                std::uint32_t objectIdTmp;
                loadVarint(objectIdTmp);
                if(objectIdTmp & detail::msb_32bit) {
                  pushSaveShared(objectIdTmp & ~detail::msb_32bit, class_depth);
                }
//                throw Exception("not implemented skipped shared ptr");
                /* Only loading non registered shared pointer is not supported so don't throw exception here.
                 * Only temporary */
              }
              if(markers & PointerMarkers::IsPolymorphicPointer) {
                loadBinary<sizeof(std::int32_t)>(&polymorphicId, sizeof(std::int32_t));
                if(polymorphicId & detail::msb_32bit) { // TODO change msb to lsb?
                  std::uint32_t nameSize;
                  loadVarint(nameSize);
                  // TODO limit max size for safety
                  polymorphicName.resize(nameSize);
                  using char_type = decltype(polymorphicName)::value_type;
                  loadBinary<sizeof(char_type)>(const_cast<char_type*>(polymorphicName.data()),
                                                nameSize * sizeof(char_type));
                  // TODO change to uint8_t (may not match on sending side)
                  // normally it would be multiply by one, but on other platforms we could just have problems
                  // TODO const_cast doesn't seem to be best idea, we know that variable is not const but it just doesn't seem right
                  //      Similar code is used in string serialization
                  //      Do we have warranty that cow is disabled (on all platforms)?
                  registerPolymorphicName(polymorphicId, polymorphicName);
                  /* Needed if polymorphic pointer's class name is saved but field is not loaded.
                   * If polymorphic pointer of the same class is saved later class name would be unknown since class name is saved only once. */
                }
              }
              break;
            }
            case FieldType::packed_array: {
              std::size_t size;
              loadVarint(size);
              skipData(type.second * size);
              break;
            }
            case FieldType::packed_struct: {
              throw Exception("packed_struct is not supported yet");
            }
            case FieldType::size_tag: {
              /* We need to load size tag because it can be needed to load BinaryData (packed_array) later */
              const auto sizeTagSize = getIntSizeFromTagSize(type.second);
              if(sizeTagSize > sizeof(lastIgnoredSizeTag)) {
                throw Exception("Size tag is to big to be loaded");
              }
              loadBinary<sizeof(lastIgnoredSizeTag)>(&lastIgnoredSizeTag,
                                            sizeTagSize);
              // TODO check if endian swapping is right here
              break;
            }
            default:
              throw Exception("Unknown type of field: " + std::to_string(static_cast<int>(type.first)));
              // delete, it's already present in readType function
          }
        } while(class_depth > 0);
      }

      inline void pushSaveShared(std::uint32_t skippedObjectId, int classDepth)
      {
        savedShared.saving.emplace_back(
            std::make_pair(skippedObjectId,
                           extendable_binary_detail::StreamPos{sharedObjectStream.tellp(),
                                                               static_cast<std::streamoff>(classDepth)
                           }));
      }

      /* Called at the end of shared object loading */
      inline bool isSkippedSharedObjectEnd(int classDepth)
      {
        return false == savedShared.saving.empty()
               && savedShared.saving.back().second.end == static_cast<std::streamoff>(classDepth);
      }

      /** End of saving skipped shared object */
      inline void popSaveShared()
      {
        if (savedShared.saving.empty()) // maybe check earlier
          throw Exception("unexpected end of shared object");
        auto &last = savedShared.saving.back();
        last.second.end = sharedObjectStream.tellp();
        savedShared.saved.emplace(last.first, last.second);
        savedShared.saving.pop_back();
      }

      inline void pushLoadShared(extendable_binary_detail::StreamPos &pos)
      {
        itsStream.pushReadingPos(pos);
      }


    private:
      std::uint32_t classVersion = 0;
      bool emptyClass = false;
      std::uint32_t objectId = 0;
      std::int32_t polymorphicId = 0;
      std::string polymorphicName;

      std::size_t lastSizeTag = 0;
      std::pair<extendable_binary_detail::FieldType, std::uint8_t> lastTypeTag =
          {extendable_binary_detail::FieldType::last_field, 0};

      SavedShared savedShared;
      std::stringstream sharedObjectStream;
      extendable_binary_detail::StreamAdapter itsStream;

      uint8_t itsConvertEndianness; //!< If set to true, we will need to swap bytes upon loading
  };

  // ######################################################################
  // Common BinaryArchive serialization functions

  //! Saving for bool types to extendable binary
  template <class T> inline
  typename std::enable_if<std::is_same<T, bool>::value, void>::type
  CEREAL_SAVE_FUNCTION_NAME(ExtendableBinaryOutputArchive & ar, T const & t)
  {
    using namespace extendable_binary_detail;
    std::uint8_t v = writeType(FieldType::integer_packed, (t ? 1 : 0));
    ar.template saveBinary<sizeof(std::uint8_t)>(&v, sizeof(std::uint8_t));
    // sizeof bool is implementation defined
  }

  //! Loading for bool types from extendable binary
  template <class T> inline
  typename std::enable_if<std::is_same<T, bool>::value, void>::type
  CEREAL_LOAD_FUNCTION_NAME(ExtendableBinaryInputArchive & ar, T & t)
  {
    using namespace extendable_binary_detail;
    const auto type = ar.getTypeTag<FieldType::integer_packed>();
    if( type.first == FieldType::omitted_field )
      return;
    t = type.second;
  }

  //! Saving for integer types to extendable binary
  template<class T> inline
  typename std::enable_if<std::is_integral<T>::value &&
      !std::is_same<T, bool>::value, void>::type
  CEREAL_SAVE_FUNCTION_NAME(ExtendableBinaryOutputArchive & ar, T const & t)
  {
    static_assert(sizeof(T) <= 32, "Only integers up to 32 bytes are supported");
    using namespace extendable_binary_detail;
    using unsigned_type = typename std::make_unsigned<T>::type;
    // can be stored in the same byte as type
    if( t < 0xf && t >= 0 ) {
      std::uint8_t v = writeType(FieldType::integer_packed, static_cast<std::uint8_t>(t));
      ar.template saveBinary<sizeof(std::uint8_t)>(&v, sizeof(v));
    } else {
      // note that abs of minimal value for signed type may not to stored the same signed type
      // http://stackoverflow.com/questions/17313579/is-there-a-safe-way-to-get-the-unsigned-absolute-value-of-a-signed-integer-with
      const unsigned_type absolute = t >= 0 ? t : -static_cast<unsigned_type>(t);
      const auto neededBytes = getIntSizeTagFromByteCount(getHighestBit(absolute));
      const auto fieldType = t >= 0 ? FieldType::positive_integer : FieldType::negative_integer;
      std::uint8_t v = writeType(fieldType, neededBytes);
      ar.template saveBinary<sizeof(std::uint8_t)>(&v, sizeof(v));
      // TODO fix for big endian
      ar.saveBinary<sizeof(T)>(std::addressof(absolute), neededBytes);
    }
  }

  //! Loading for integer types from extendable binary
  template<class T> inline
  typename std::enable_if<std::is_integral<T>::value &&
      !std::is_same<T, bool>::value, void>::type
  CEREAL_LOAD_FUNCTION_NAME(ExtendableBinaryInputArchive & ar, T & t)
  {
    using namespace extendable_binary_detail;
    auto type = ar.getTypeTagNoError<FieldType::integer_packed>();
    switch(type.first) {
      case FieldType::omitted_field:
        return;
      case FieldType::integer_packed: {
        t = type.second;
        break;
      }
      case FieldType::positive_integer: {
        const auto neededByteSize = getIntSizeFromTagSize(type.second);
        if(neededByteSize > sizeof(T)) {
          throw Exception("Integer is to big to be loaded");
        }
        t = 0;
        // TODO fix big endian, size of elem may be wrong
        ar.template loadBinary<sizeof(T)>(std::addressof(t), neededByteSize);
        break;
      }
      case FieldType::negative_integer: {
        const auto neededByteSize = getIntSizeFromTagSize(type.second);
        if(neededByteSize > sizeof(T)) {
          throw Exception("Integer is to big to be loaded");
        }
        if(std::is_unsigned<T>::value) {
          throw Exception("Negative value cannot be loaded to unsigned type");
        }
        t = 0;
        // TODO fix big endian, size of elem may be wrong
        ar.template loadBinary<sizeof(T)>(std::addressof(t), neededByteSize);
        t = -t;
        break;
      }
      default:
        throw Exception("Unexpected type expected: integer got:" + std::to_string(static_cast<int>(type.first)));
    }
  }

  //! Saving for floating point to extendable binary
  template<class T> inline
  typename std::enable_if<std::is_floating_point<T>::value, void>::type
  CEREAL_SAVE_FUNCTION_NAME(ExtendableBinaryOutputArchive & ar, T const & t)
  {
    static_assert( !std::is_floating_point<T>::value ||
                   (std::is_floating_point<T>::value && std::numeric_limits<T>::is_iec559),
                   "Extendable binary only supports IEEE 754 standardized floating point" );
    static_assert((sizeof(t) == 4) || (sizeof(t) == 8) || (sizeof(t) == 16), "unsupported float size");
    using namespace extendable_binary_detail;
    std::uint8_t floatSize;
    switch(sizeof(t)) {
      case 4:
        floatSize = 1;
        break;
      case 8:
        floatSize = 2;
        break;
      case 16:
        floatSize = 3;
        break;
    }
    std::uint8_t v = writeType(FieldType::floating_point, floatSize);
    ar.template saveBinary<sizeof(std::uint8_t)>(&v, sizeof(std::uint8_t));
    ar.template saveBinary<sizeof(T)>(std::addressof(t), sizeof(t));
  }

  //! Loading for floating point types from extendable binary
  template<class T> inline
  typename std::enable_if<std::is_floating_point<T>::value, void>::type
  CEREAL_LOAD_FUNCTION_NAME(ExtendableBinaryInputArchive & ar, T & t)
  {
    static_assert( !std::is_floating_point<T>::value ||
                   (std::is_floating_point<T>::value && std::numeric_limits<T>::is_iec559),
                   "Extendable binary only supports IEEE 754 standardized floating point" );
    static_assert((sizeof(t) == 4) || (sizeof(t) == 8) || (sizeof(t) == 16), "unsupported float size");
    using namespace extendable_binary_detail;
    auto type = ar.getTypeTag<FieldType::floating_point>();
    if(type.first == FieldType::omitted_field)
      return;
    switch(type.second) {
      case 1: {
        float dest;
        ar.template loadBinary<sizeof(dest)>(std::addressof(dest), sizeof(dest));
        // in case of double/long double, TODO make separate specialization
        t = dest;
        break;
      }
      case 2: {
        double dest;
        ar.template loadBinary<sizeof(dest)>(std::addressof(dest), sizeof(dest));
        // in case of float/long double, TODO make separate specialization
        t = dest;
        break;
      }
      // https://en.wikipedia.org/wiki/Long_double
      // FIXME can be different size
      case 3: {
        long double dest;
        ar.template loadBinary<sizeof(dest)>(std::addressof(dest), sizeof(dest));
        // in case of float/double, TODO make separate specialization
        t = dest;
        break;
      }
      default:
        throw Exception("Not supported size of floating point: " + std::to_string(type.second));
    }
  }

  //! Serializing NVP types to extendable binary
  template <class Archive, class T> inline
  CEREAL_ARCHIVE_RESTRICT(ExtendableBinaryInputArchive, ExtendableBinaryOutputArchive)
  CEREAL_SERIALIZE_FUNCTION_NAME( Archive & ar, NameValuePair<T> & t )
  {
    ar( t.value );
  }

  //! Saving SizeTag to extendable binary
  template <class T> inline
  void CEREAL_SAVE_FUNCTION_NAME(ExtendableBinaryOutputArchive & ar, SizeTag<T> const & t)
  {
    using namespace extendable_binary_detail;
    static_assert(sizeof(t.size) <= 32, "Only integers up to 32 bytes are supported");
    if(t.size < 0) {
      throw Exception("Negative SizeTag is not suppported");
    }
    const auto neededBytes = getIntSizeTagFromByteCount(getHighestBit(t.size));
    const auto fieldType = FieldType::size_tag;
    std::uint8_t v = writeType(fieldType, neededBytes);
    ar.template saveBinary<sizeof(std::uint8_t)>(&v, sizeof(v));
    // TODO fix for big endian
    ar.saveBinary<sizeof(T)>(std::addressof(t.size), static_cast<std::size_t>(neededBytes));
  }

  //! Loading SizeTag from extendable binary
  template <class T> inline
  void CEREAL_LOAD_FUNCTION_NAME(ExtendableBinaryInputArchive & ar, SizeTag<T> & t)
  {
    using namespace extendable_binary_detail;
    auto type = ar.getTypeTag<FieldType::size_tag>();
    if(type.first == FieldType::omitted_field) {
      return;
    } else {
      const auto neededByteSize = getIntSizeFromTagSize(type.second);
      if(neededByteSize > sizeof(t.size)) {
        throw Exception("Size tag integer is to big to be loaded");
      }
      t.size = 0;
      // TODO fix big endian, size of elem may be wrong
      ar.template loadBinary<sizeof(T)>(std::addressof(t.size), neededByteSize);
      ar.setLastSizeTag(t.size);
    }
  }

  //! Saving OmittedFieldTag to extendable binary
  inline
  void CEREAL_SAVE_FUNCTION_NAME(ExtendableBinaryOutputArchive & ar, OmittedFieldTag const &)
  {
    using namespace extendable_binary_detail;
    std::uint8_t v = writeType(FieldType::omitted_field, 0);
    ar.template saveBinary<sizeof(std::uint8_t)>(&v, sizeof(std::uint8_t));
  }

  //! Loading OmittedFieldTag from extendable binary
  inline
  void CEREAL_LOAD_FUNCTION_NAME(ExtendableBinaryInputArchive & ar, OmittedFieldTag &)
  {
    ar.loadOmittedObject();
  }

  //! Saving binary data to extendable binary
  template <class T> inline
  void CEREAL_SAVE_FUNCTION_NAME(ExtendableBinaryOutputArchive & ar, BinaryData<T> const & bd)
  {
    using TT = typename std::remove_pointer<T>::type;
    static_assert( !std::is_floating_point<TT>::value ||
                   (std::is_floating_point<TT>::value && std::numeric_limits<TT>::is_iec559),
                   "Extendable binary only supports IEEE 754 standardized floating point" );

    using namespace extendable_binary_detail;
    std::uint8_t packedSizeOfElem;
    if(sizeof(TT) < 0xf) {
      packedSizeOfElem = sizeof(TT);
    } else {
      packedSizeOfElem = 0xf;
    }
    std::uint8_t v = writeType(FieldType::packed_array, packedSizeOfElem);
    ar.template saveBinary<sizeof(std::uint8_t)>(&v, sizeof(std::uint8_t));
    if(sizeof(TT) >= 0xf) {
      ar.saveVarint(sizeof(TT));
    }
    // TODO size is saved twice for string, once before in size tag @see types/string.hpp
    ar.saveVarint(bd.size / sizeof(TT));
    ar.template saveBinary<sizeof(TT)>( bd.data, static_cast<std::size_t>( bd.size ) );
  }

  //! Loading binary data from extendable binary
  template <class T> inline
  void CEREAL_LOAD_FUNCTION_NAME(ExtendableBinaryInputArchive & ar, BinaryData<T> & bd)
  {
    typedef typename std::remove_pointer<T>::type TT;
    static_assert( !std::is_floating_point<TT>::value ||
                   (std::is_floating_point<TT>::value && std::numeric_limits<TT>::is_iec559),
                   "Extendable binary only supports IEEE 754 standardized floating point" );
    using namespace extendable_binary_detail;
    const auto type = ar.getTypeTag<FieldType::packed_array>();
    if(type.first == FieldType::omitted_field)
      return;
    std::uint64_t sizeOfElem;
    if(type.second == 0xf) {
      ar.loadVarint(sizeOfElem);
    } else {
      sizeOfElem = type.second;
    }
    if(sizeof(TT) != sizeOfElem) {
      // We could allow mismatch here, only problem would be how to make endian swap
      throw Exception("Wrong dest type size, expected:" + std::to_string(sizeOfElem) + " got:" + std::to_string(sizeof(TT)));
    }
    std::uint64_t numberOfElements;
    ar.loadVarint(numberOfElements);
    std::uint64_t wholeSize = numberOfElements * sizeOfElem;
    if( wholeSize > bd.size )  {
      throw Exception("BinaryData is bigger than dest var");
    }
    ar.template loadBinary<sizeof(TT)>( bd.data, wholeSize );
  }

  //! Saving VersionIdTag to extendable binary
  template <class T> inline
  void CEREAL_SAVE_FUNCTION_NAME(ExtendableBinaryOutputArchive & ar, detail::VersionIdTag<T> const & version)
  {
    ar.saveClassVersion(version.version_id);
  }

  //! Loading VersionIdTag from extendable binary
  template <class T> inline
  void CEREAL_LOAD_FUNCTION_NAME(ExtendableBinaryInputArchive & ar, detail::VersionIdTag<T> & version)
  {
    version.version_id = ar.getLoadedClassVersion();
  }

  template <class T>
  struct specialize<ExtendableBinaryOutputArchive, detail::VersionIdTag<T>, specialization::non_member_load_save> {};
  template <class T>
  struct specialize<ExtendableBinaryInputArchive, detail::VersionIdTag<T>, specialization::non_member_load_save> {};

  //! Saving PolymorphicIdTag to extendable binary
  template <class T> inline
  void CEREAL_SAVE_FUNCTION_NAME(ExtendableBinaryOutputArchive & ar, detail::PolymorphicIdTag<T> const & polymorphicIdTag)
  {
    ar.savePolymorphicId(polymorphicIdTag.polymorphic_id);
  }

  //! Loading PolymorphicIdTag from extendable binary
  template <class T> inline
  void CEREAL_LOAD_FUNCTION_NAME(ExtendableBinaryInputArchive & ar, detail::PolymorphicIdTag<T> & polymorphicIdTag)
  {
    polymorphicIdTag.polymorphic_id = ar.getLoadedPolymorphicId();
  }

  template <class T>
  struct specialize<ExtendableBinaryOutputArchive, detail::PolymorphicIdTag<T>, specialization::non_member_load_save> {};
  template <class T>
  struct specialize<ExtendableBinaryInputArchive, detail::PolymorphicIdTag<T>, specialization::non_member_load_save> {};

  //! Saving PolymorphicKeyTag to extendable binary
  template <class T> inline
  void CEREAL_SAVE_FUNCTION_NAME(ExtendableBinaryOutputArchive & ar, detail::PolymorphicKeyTag<T> const & polymorphicKeyTag)
  {
    ar.savePolymorphicName(polymorphicKeyTag.polymorphic_key);
  }

  //! Loading PolymorphicKeyTag from extendable binary
  template <class T> inline
  void CEREAL_LOAD_FUNCTION_NAME(ExtendableBinaryInputArchive & ar, detail::PolymorphicKeyTag<T> & polymorphicKeyTag)
  {
    polymorphicKeyTag.polymorphic_key = ar.getLoadedPolymorphicName();
  }

  template <class T>
  struct specialize<ExtendableBinaryOutputArchive, detail::PolymorphicKeyTag<T>, specialization::non_member_load_save> {};
  template <class T>
  struct specialize<ExtendableBinaryInputArchive, detail::PolymorphicKeyTag<T>, specialization::non_member_load_save> {};

  //! Saving ObjectIdTag to extendable binary
  template <class T> inline
  void CEREAL_SAVE_FUNCTION_NAME(ExtendableBinaryOutputArchive & ar, detail::ObjectIdTag<T> const & objectIdTag)
  {
    ar.saveObjectId(objectIdTag.object_id);
  }

  //! Loading ObjectIdTag from extendable binary
  template <class T> inline
  void CEREAL_LOAD_FUNCTION_NAME(ExtendableBinaryInputArchive & ar, detail::ObjectIdTag<T> & objectIdTag)
  {
    objectIdTag.object_id = ar.getLoadedObjectId();
  }

  template <class T>
  struct specialize<ExtendableBinaryOutputArchive, detail::ObjectIdTag<T>, specialization::non_member_load_save> {};
  template <class T>
  struct specialize<ExtendableBinaryInputArchive, detail::ObjectIdTag<T>, specialization::non_member_load_save> {};

  //! Saving PointerValidityTag to extendable binary
  template <class T> inline
  void CEREAL_SAVE_FUNCTION_NAME(ExtendableBinaryOutputArchive & ar, detail::PointerValidityTag<T> const & pointerValidityTag)
  {
    ar.savePointerValidityTag(pointerValidityTag.valid != 0);
  }

  //! Loading PointerValidityTag from extendable binary
  template <class T> inline
  void CEREAL_LOAD_FUNCTION_NAME(ExtendableBinaryInputArchive & ar, detail::PointerValidityTag<T> & pointerValidityTag)
  {
    pointerValidityTag.valid = ar.getLoadedPointerValidity();
  }

  template <class T>
  struct specialize<ExtendableBinaryOutputArchive, detail::PointerValidityTag<T>, specialization::non_member_load_save> {};
  template <class T>
  struct specialize<ExtendableBinaryInputArchive, detail::PointerValidityTag<T>, specialization::non_member_load_save> {};

  // ######################################################################
  // ExtendableBinaryArchive prologue and epilogue functions
  // ######################################################################

  // ###SimpleTypes########################################################
  //! Types which has empty prologue and epilogoue (version with one template argument)
  /*! For most of this types handling is already done in serialize function */
  template<class T>
  struct is_extendablebinary_empty_prologue_and_epilogue1 :
      traits::detail::meta_bool_or<
          std::is_arithmetic<T>::value,
          std::is_same<T, std::nullptr_t>::value,
          std::is_same<T, OmittedFieldTag>::value
      > {};
  template <class T>
  struct is_extendablebinary_empty_prologue_and_epilogue1<SizeTag<T>> : std::true_type {};
  template <class T>
  struct is_extendablebinary_empty_prologue_and_epilogue1<BinaryData<T>> : std::true_type {};
  // TODO for input archive cache size, and store it specially for BinaryData
  // TODO move to traits namespace

  //! Prologue for arithmetic types for ExtendableBinary archives
  template <class T, traits::EnableIf<is_extendablebinary_empty_prologue_and_epilogue1<T>::value> = traits::sfinae> inline
  void prologue( ExtendableBinaryOutputArchive & ar, T const & )
  {
    ar.savingOtherField();
  }

  //! Prologue for arithmetic types for ExtendableBinary archives
  template <class T, traits::EnableIf<is_extendablebinary_empty_prologue_and_epilogue1<T>::value> = traits::sfinae> inline
  FieldSerialized prologueLoad( ExtendableBinaryInputArchive & ar, T const & )
  {
    return ar.loadTypeTag() ? FieldSerialized::YES : FieldSerialized::NO;
  }

  //! Epilogue for arithmetic types for ExtendableBinary archives
  template <class T, traits::EnableIf<is_extendablebinary_empty_prologue_and_epilogue1<T>::value> = traits::sfinae> inline
  void epilogue( ExtendableBinaryOutputArchive &, T const & )
  { }

  //! Epilogue for arithmetic types for ExtendableBinary archives
  template <class T, traits::EnableIf<is_extendablebinary_empty_prologue_and_epilogue1<T>::value> = traits::sfinae> inline
  void epilogue( ExtendableBinaryInputArchive &, T const & )
  { }

  // ###Internal Types#####################################################
  //! Types which has empty internal prologue and epilogoue (version with one template argument)
  /*! For most of this types handling is already done in serialize function
   * Internal handing not starting new field @see savingOtherField for empty class optimalization */
  template<class T>
  struct is_extendablebinary_internal_prologue_and_epilogue1 : std::false_type {};
  template <class T>
  struct is_extendablebinary_internal_prologue_and_epilogue1<NameValuePair<T>> : std::true_type {};
  /*! saving and loading of weak pointer calls shared_ptr serialization function which makes one more level in archive
   *  That happends only for polymorphic types, that's why we need this strange code
   */
  template <class T>
  struct is_extendablebinary_internal_prologue_and_epilogue1<std::weak_ptr<T>>
      : std::integral_constant<bool, std::is_polymorphic<T>::value>::type {};
// TODO change shared_ptr saving to separate function
  template <class T>
  struct is_extendablebinary_internal_prologue_and_epilogue1<memory_detail::PtrWrapper<T>> : std::true_type {};
  // TODO change for pointers
  // TODO move to traits namespace
  template <class T>
  struct is_extendablebinary_internal_prologue_and_epilogue1<detail::VersionIdTag<T>> : std::true_type {};
  template <class T>
  struct is_extendablebinary_internal_prologue_and_epilogue1<detail::ObjectIdTag<T>> : std::true_type {};
  template <class T>
  struct is_extendablebinary_internal_prologue_and_epilogue1<detail::PointerValidityTag<T>> : std::true_type {};
  template <class T>
  struct is_extendablebinary_internal_prologue_and_epilogue1<detail::PolymorphicIdTag<T>> : std::true_type {};
  template <class T>
  struct is_extendablebinary_internal_prologue_and_epilogue1<detail::PolymorphicKeyTag<T>> : std::true_type {};
  template <class T, std::size_t V>
  struct is_extendablebinary_internal_prologue_and_epilogue1<std::array<T, V>> : std::true_type {};

  //! Prologue for internal types for ExtendableBinary archives
  template <class T, traits::EnableIf<is_extendablebinary_internal_prologue_and_epilogue1<T>::value> = traits::sfinae> inline
  void prologue( ExtendableBinaryOutputArchive &, T const & )
  { }

  //! Prologue for internal types for ExtendableBinary archives
  template <class T, traits::EnableIf<is_extendablebinary_internal_prologue_and_epilogue1<T>::value> = traits::sfinae> inline
  FieldSerialized prologueLoad( ExtendableBinaryInputArchive &, T const & )
  {
    return FieldSerialized::INTERNAL;
  }

  //! Epilogue for internal types for ExtendableBinary archives
  template <class T, traits::EnableIf<is_extendablebinary_internal_prologue_and_epilogue1<T>::value> = traits::sfinae> inline
  void epilogue( ExtendableBinaryOutputArchive &, T const & )
  { }

  //! Epilogue for internal types for ExtendableBinary archives
  template <class T, traits::EnableIf<is_extendablebinary_internal_prologue_and_epilogue1<T>::value> = traits::sfinae> inline
  void epilogue( ExtendableBinaryInputArchive &, T const & )
  { }

#if 0 // have to change SizeTag to not save size two times in string and vector
  // ####string############################################
  //! Prologue for strings for ExtendableBinary archives
  template<class CharT, class Traits, class Alloc> inline
  void prologue(ExtendableBinaryOutputArchive & ar, std::basic_string<CharT, Traits, Alloc> const &)
  {
    ar.savingOtherField();
  }

  //! Prologue for strings for ExtendableBinary archives
  template<class CharT, class Traits, class Alloc> inline
  FieldSerialized prologueLoad(ExtendableBinaryInputArchive &, std::basic_string<CharT, Traits, Alloc> const &)
  {
    return FieldSerialized::INTERNAL;
  }

  //! Epilogue for strings for ExtendableBinary archives
  template<class CharT, class Traits, class Alloc> inline
  void epilogue(ExtendableBinaryOutputArchive &, std::basic_string<CharT, Traits, Alloc> const &)
  { }

  //! Epilogue for strings for ExtendableBinary archives
  template<class CharT, class Traits, class Alloc> inline
  void epilogue(ExtendableBinaryInputArchive &, std::basic_string<CharT, Traits, Alloc> const &)
  { }
#endif

  // ###Objects###########################################################
  //! Prologue for all other types for ExtendableBinary archives (except minimal types)
  /*! Starts a new node, named either automatically or by some NVP,
      that may be given data by the type about to be archived

      Minimal types do not start or finish nodes */
  template <class T, traits::EnableIf<!traits::has_minimal_base_class_serialization<T, traits::has_minimal_output_serialization, ExtendableBinaryOutputArchive>::value,
                                      !traits::has_minimal_output_serialization<T, ExtendableBinaryOutputArchive>::value,
                                      !is_extendablebinary_empty_prologue_and_epilogue1<T>::value,
                                      !is_extendablebinary_internal_prologue_and_epilogue1<T>::value> = traits::sfinae>
  inline void prologue( ExtendableBinaryOutputArchive & ar, T const & )
  {
    ar.saveObjectBeginning();
  }

  //! Prologue for all other types for ExtendableBinary archives
  template <class T, traits::EnableIf<!traits::has_minimal_base_class_serialization<T, traits::has_minimal_input_serialization, ExtendableBinaryInputArchive>::value,
                                      !traits::has_minimal_input_serialization<T, ExtendableBinaryInputArchive>::value,
                                      !is_extendablebinary_empty_prologue_and_epilogue1<T>::value,
                                      !is_extendablebinary_internal_prologue_and_epilogue1<T>::value> = traits::sfinae>
  inline FieldSerialized prologueLoad( ExtendableBinaryInputArchive & ar, T const & )
  {
    const bool load = ar.loadTypeTag();
    if(load) {
      ar.loadObjectBeginning();
    }
    return load ? FieldSerialized::YES : FieldSerialized::NO;
  }

  //! Epilogue for all other types other for ExtendableBinary archives (except minimal types)
  /*! Finishes the node created in the prologue
      Minimal types do not start or finish nodes */
  template <class T, traits::EnableIf<!traits::has_minimal_base_class_serialization<T, traits::has_minimal_output_serialization, ExtendableBinaryOutputArchive>::value,
                                      !traits::has_minimal_output_serialization<T, ExtendableBinaryOutputArchive>::value,
                                      !is_extendablebinary_empty_prologue_and_epilogue1<T>::value,
                                      !is_extendablebinary_internal_prologue_and_epilogue1<T>::value> = traits::sfinae>
  inline void epilogue( ExtendableBinaryOutputArchive & ar, T const & )
  {
    ar.saveObjectEnd();
  }

  //! Epilogue for all other types other for ExtendableBinary archives
  template <class T, traits::EnableIf<!traits::has_minimal_base_class_serialization<T, traits::has_minimal_input_serialization, ExtendableBinaryInputArchive>::value,
                                      !traits::has_minimal_input_serialization<T, ExtendableBinaryInputArchive>::value,
                                      !is_extendablebinary_empty_prologue_and_epilogue1<T>::value,
                                      !is_extendablebinary_internal_prologue_and_epilogue1<T>::value> = traits::sfinae>
  inline void epilogue( ExtendableBinaryInputArchive & ar, T const & )
  {
    ar.loadEndOfObjectData();
  }
} // namespace cereal

// register archives for polymorphic support
CEREAL_REGISTER_ARCHIVE(cereal::ExtendableBinaryOutputArchive)
CEREAL_REGISTER_ARCHIVE(cereal::ExtendableBinaryInputArchive)

// tie input and output archives together
CEREAL_SETUP_ARCHIVE_TRAITS(cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive)

namespace cereal { namespace traits {

template<>
struct wrap_polymorphic_weak_ptr<cereal::ExtendableBinaryOutputArchive> : std::false_type
{ };

template<>
struct wrap_polymorphic_weak_ptr<cereal::ExtendableBinaryInputArchive> : std::false_type
{ };

}} // namespace cereal::traits

#endif // CEREAL_ARCHIVES_EXTENDABLE_BINARY_HPP_
