/*! \file extendable_binary.hpp
    \brief Binary input and output archives with bacward and forward compatiblity */
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

namespace cereal
{
  namespace extendable_binary_detail
  {
    //! Returns true if the current machine is little endian
    /*! @ingroup Internal */
    inline std::uint8_t is_little_endian()
    {
      static std::int32_t test = 1;
      return *reinterpret_cast<std::int8_t*>( &test ) == 1;
    }

    //! Swaps the order of bytes for some chunk of memory
    /*! @param data The data as a uint8_t pointer
        @tparam DataSize The true size of the data
        @ingroup Internal */
    template <std::size_t DataSize>
    inline void swap_bytes( std::uint8_t * data )
    {
      for( std::size_t i = 0, end = DataSize / 2; i < end; ++i )
        std::swap( data[i], data[DataSize - i - 1] );
    }

    //! Describes type of saved field
    enum class FieldType {
      /*! int (0000 | ssss)  size in bytes
          need to store up to 256 bytes - 0,1,2,4,8,16,32 - 7 values
          | 0000 - 0 | 0001:  1 | 0010:  2 | 0011:  3 | 0100:  4 | 0101:  5 | 0110:  6 | 0111:  7 |
          |          | 1001: -1 | 1010: -2 | 1011: -3 | 1100: -4 | 1101: -5 | 1110: -6 | 1111: -7 |
          Reduce to 128 bytes and add bit? 256 bytes? */
      positive_integer = 0x0,
      /*! int (0000 | ssss)  size in bytes
          need to store up to 256 bytes - 0,1,2,4,8,16,32 - 7 values
          | 0000 - 0 | 0001:  1 | 0010:  2 | 0011:  3 | 0100:  4 | 0101:  5 | 0110:  6 | 0111:  7 |
          |          | 1001: -1 | 1010: -2 | 1011: -3 | 1100: -4 | 1101: -5 | 1110: -6 | 1111: -7 |
          Reduce to 128 bytes and add bit? 256 bytes? */
      negative_integer = 0x1,
      //! float (0001 | ????)
      floating_point = 0x2,
      //! int packed (0010) (optional)
      integer_packed = 0x3,
      /*! class (0011)
            pointer
            virtual - typeid follows */
      class_t = 0x4,
      /*! packed array (0100 | size of elem)
          e.g. string */
      packed_array = 0x5,
      //! struct (0101)
      packed_struct = 0x6,
      //! omitted field (0110)
      ommited_field = 0x7,
      /*! was last field in class (0111)
       (not optional fields, class id) */
      last_field = 0x8,
      LAST_RESERVED_UNUSED
    };

    //! max size of saved varint
    /*! enough to save uint64_t 8 */
    constexpr std::size_t maxVarintSize = 10;

    inline std::uint8_t writeType(FieldType fieldType, std::uint8_t other) {
      return (static_cast<std::uint8_t>(fieldType) << 4) | (0xf & other);
    }

    inline std::pair<FieldType, std::uint8_t> readType(std::uint8_t input) {
      std::uint8_t fieldType = (input >> 4);
      if(static_cast<std::uint8_t>(fieldType) >= static_cast<std::uint8_t>(FieldType::LAST_RESERVED_UNUSED)) {
        throw Exception("FieldType has unknown value " + std::to_string(fieldType) + " all:" + std::to_string(input));
      }
      return std::make_pair(static_cast<FieldType>(fieldType), (0xf & input));
    }

    //!
    /*! TODO can be done quicker http://stackoverflow.com/questions/2274428/how-to-determine-how-many-bytes-an-integer-needs
       \param v
       \return */
    template <class T> inline
    std::uint8_t getHighestBit(T v) {
      // TODO fix for big endian
      std::uint8_t n = 0;
      while( v != 0 ) {
        v >>= 8;
        n++;
      }
      return n;
    }
    //! shift count >= width of type [-Werror,-Wshift-count-overflow]
    inline std::uint8_t getHighestBit(std::uint8_t) {
      return 1;
    }

    inline std::uint8_t getFloatSizeFromTagSize(std::uint8_t tagSize) {
      switch (tagSize) {
        case 1:
          return 4;
        case 2:
          return 8;
        case 3:
          return 16;
        default:
          throw Exception("Unsupported floating point size");
      }
    }

    template <class T>
    inline std::uint8_t getTagSizeFromFloatType() {
      static_assert(sizeof(T) == 4 || sizeof(T) == 8 || sizeof(T) == 16, "unsupported floating point type");
      static_assert(std::is_floating_point<T>::value, "expected floating point type");
      switch (sizeof(T)) {
        case 4:
          return 1;
        case 8:
          return 2;
        case 16:
          return 3;
      }
    }
  } // end namespace extendable_binary_detail

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
  class ExtendableBinaryOutputArchive : public OutputArchive<ExtendableBinaryOutputArchive, AllowEmptyClassElision>
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
        OutputArchive<ExtendableBinaryOutputArchive, AllowEmptyClassElision>(this),
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

    private:
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
  class ExtendableBinaryInputArchive : public InputArchive<ExtendableBinaryInputArchive, AllowEmptyClassElision>
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
        InputArchive<ExtendableBinaryInputArchive, AllowEmptyClassElision>(this),
        itsStream(stream),
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
        auto const readSize = static_cast<std::size_t>( itsStream.rdbuf()->sgetn( reinterpret_cast<char*>( data ), size ) );

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
      inline void skipData( std::size_t size )
      {
        itsStream.ignore( size );
        // TODO consider switching to seekg, not sure if supported for all streams
        if(itsStream.bad())
          throw Exception("Failed to skip " + std::to_string(size) + " bytes from input stream!");
      }

      inline auto loadType() -> decltype(extendable_binary_detail::readType(std::uint8_t{})) {
        std::uint8_t v;
        loadBinary<sizeof(std::uint8_t)>(&v, sizeof(std::uint8_t));
        return extendable_binary_detail::readType(v);
      }

       //! Load type tag from input stream
      /*! @tparam expected_type type which is expected to be loaded
          Throws if type is not expected or ommited */
      template <extendable_binary_detail::FieldType expected_type>
      inline auto loadTypeTagSkip() -> decltype(extendable_binary_detail::readType(std::uint8_t{})) {
        using namespace extendable_binary_detail;
        static_assert(expected_type != FieldType::last_field, "should go to loadEndOfClass");
        std::uint8_t v;
        loadBinary<sizeof(std::uint8_t)>(&v, sizeof(std::uint8_t));
        auto type = extendable_binary_detail::readType(v);
        if(type.first != expected_type && type.first != FieldType::ommited_field)
          throw Exception("Loaded wrong type, expected: " + std::to_string(static_cast<int>(expected_type))
                          + " got: " + std::to_string(static_cast<int>(type.first)) );
        return type;
      }
      // TODO varargs
      //! Load type tag from input stream
      /*! @tparam expected_type type which is expected to be loaded
          Throws if type is not expected or ommited */
      template <extendable_binary_detail::FieldType expected_type>
      inline auto loadTypeTagSkipNoError() -> decltype(extendable_binary_detail::readType(std::uint8_t{})) {
        using namespace extendable_binary_detail;
        static_assert(expected_type != FieldType::last_field, "should go to loadEndOfClass");
        std::uint8_t v;
        loadBinary<sizeof(std::uint8_t)>(&v, sizeof(std::uint8_t));
        auto type = extendable_binary_detail::readType(v);
        return type;
      }

      //! Skip data until end of class is reached
      /*! @tparam expected_type is last_field indicating after last field in class
          Consider checking if types have expected sizes
          Note: specialization of loadTypeSkip would be preferable but specialization of member functions without
            specializing class is not possible */
      inline void loadEndOfClass() {
        using namespace extendable_binary_detail;
        using return_type = decltype(extendable_binary_detail::readType(std::uint8_t{}));
        std::uint8_t v;
        return_type type;
        int class_depth = 1; // we are in an object
        do {
          loadBinary<sizeof(std::uint8_t)>(&v, sizeof(std::uint8_t));
          type = readType(v);
          switch (type.first) {
            case FieldType::positive_integer:
            case FieldType::negative_integer: {
              skipData(type.second);
              break;
            }
            case FieldType::floating_point: {
              skipData(getFloatSizeFromTagSize(type.second));
              break;
            }
            case FieldType::integer_packed:
              break; // one byte
            case FieldType::ommited_field:
              break; // one byte
            case FieldType::last_field: {
              /* what we expected, but only if we didn't go into next class field */
              --class_depth;
              break; // one byte
            }
            case FieldType::class_t: {
              ++class_depth;
              // TODO rest of field interpretation
              break;
            }
            case FieldType::packed_array: {
              std::size_t size;
              loadVarint(size);
              skipData(type.second * size);
              break;
              // TODO fix size is saved two times
            }
            case FieldType::packed_struct: {
              throw Exception("packed_struct is not supported yet");
            }
            default:
              throw Exception("Unknown type of field: " + std::to_string(static_cast<int>(type.first)));
              // delete, it's already present in readType function
          }
        } while ( type.first != FieldType::last_field || class_depth != 0);
      }

      //! Load varint to the stream
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
    private:
      std::istream & itsStream;
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
    const auto type = ar.loadTypeTagSkip<FieldType::integer_packed>();
    if( type.first == FieldType::ommited_field )
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
    using unsinged_type = typename std::make_unsigned<T>::type;
    // can be stored in the same byte as type
    if( t < 0xf && t >= 0 ) {
      std::uint8_t v = writeType(FieldType::integer_packed, t);
      ar.template saveBinary<sizeof(std::uint8_t)>(&v, sizeof(v));
    } else {
      // note that abs of minimal value for signed type may not to stored the same signed type
      // http://stackoverflow.com/questions/17313579/is-there-a-safe-way-to-get-the-unsigned-absolute-value-of-a-signed-integer-with
      const unsinged_type absolute = t >= 0 ? t : -static_cast<unsinged_type>(t);
      const auto highestBit = getHighestBit(absolute);
      const auto fieldType = t >= 0 ? FieldType::positive_integer : FieldType::negative_integer;
      std::uint8_t v = writeType(fieldType, highestBit);
      ar.template saveBinary<sizeof(std::uint8_t)>(&v, sizeof(v));
      // TODO fix for big endian
      ar.saveBinary<sizeof(T)>(std::addressof(absolute), highestBit);
    }
  }

  //! Loading for integer types from extendable binary
  template<class T> inline
  typename std::enable_if<std::is_integral<T>::value &&
      !std::is_same<T, bool>::value, void>::type
  CEREAL_LOAD_FUNCTION_NAME(ExtendableBinaryInputArchive & ar, T & t)
  {
    using namespace extendable_binary_detail;
    auto type = ar.loadTypeTagSkipNoError<FieldType::integer_packed>();
    switch(type.first) {
      case FieldType::ommited_field:
        return;
      case FieldType::integer_packed: {
        t = type.second;
        break;
      }
      case FieldType::positive_integer: {
        if(type.second > sizeof(T)) {
          throw Exception("Integer is to big to be loaded");
        }
        t = 0;
        // TODO fix big endian, size of elem i wrong
        ar.template loadBinary<sizeof(T)>(std::addressof(t), type.second);
        break;
      }
      case FieldType::negative_integer: {
        if(type.second > sizeof(T)) {
          throw Exception("Integer is to big to be loaded");
        }
        if(std::is_unsigned<T>::value) {
          throw Exception("Negative value cannot be loaded to unsigned type");
        }
        t = 0;
        // TODO fix big endian, size of elem is wrong
        ar.template loadBinary<sizeof(T)>(std::addressof(t), type.second);
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
    auto type = ar.loadTypeTagSkip<FieldType::floating_point>();
    if(type.first == FieldType::ommited_field)
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

  //! Serializing SizeTags to extendable binary
  template <class Archive, class T> inline
  CEREAL_ARCHIVE_RESTRICT(ExtendableBinaryInputArchive, ExtendableBinaryOutputArchive)
  CEREAL_SERIALIZE_FUNCTION_NAME( Archive & ar, SizeTag<T> & t )
  {
    ar( t.size ); // TODO implement
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
    // TODO size is saved twice, once before in size tag @see types/string.hpp
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
    const auto type = ar.loadTypeTagSkip<FieldType::packed_array>();
    if(type.first == FieldType::ommited_field)
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
  // ######################################################################
  // ExtendableBinaryArchive prologue and epilogue functions
  // ######################################################################

  //! Types which has empty prologue and epilogoue (version with one template argument
  /*! For most of this types handling is already done in serialize function */
  template<class T>
  struct is_extendablebinary_empty_prologue_and_epilogue1 :
      traits::detail::meta_bool_or<
          std::is_arithmetic<T>::value,
          std::is_same<T, std::nullptr_t>::value
      > {};
  template <class T>
  struct is_extendablebinary_empty_prologue_and_epilogue1<NameValuePair<T>> : std::true_type {};
  template <class T>
  struct is_extendablebinary_empty_prologue_and_epilogue1<SizeTag<T>> : std::true_type {};
  template <class T, std::size_t V>
  struct is_extendablebinary_empty_prologue_and_epilogue1<std::array<T, V>> : std::true_type {};
  template <class T>
  struct is_extendablebinary_empty_prologue_and_epilogue1<BinaryData<T>> : std::true_type {};
  // TODO for input archive cache size, and store it specially for BinaryData
  // TODO move to traits namespace
  // TODO change for pointers

  // ######################################################################
  //! Prologue for arithmetic types for ExtendableBinary archives
  template <class T, traits::EnableIf<is_extendablebinary_empty_prologue_and_epilogue1<T>::value> = traits::sfinae> inline
  void prologue( ExtendableBinaryOutputArchive &, T const & )
  {
    std::cout << "EMPTY prologue:" << typeid(T).name() << "\n";
  }

  //! Prologue for arithmetic types for ExtendableBinary archives
  template <class T, traits::EnableIf<is_extendablebinary_empty_prologue_and_epilogue1<T>::value> = traits::sfinae> inline
  void prologue( ExtendableBinaryInputArchive &, T const & )
  { }

  // ######################################################################
  //! Epilogue for arithmetic types for ExtendableBinary archives
  template <class T, traits::EnableIf<is_extendablebinary_empty_prologue_and_epilogue1<T>::value> = traits::sfinae> inline
  void epilogue( ExtendableBinaryOutputArchive &, T const & )
  { }

  //! Epilogue for arithmetic types for ExtendableBinary archives
  template <class T, traits::EnableIf<is_extendablebinary_empty_prologue_and_epilogue1<T>::value> = traits::sfinae> inline
  void epilogue( ExtendableBinaryInputArchive &, T const & )
  { }

  // ####string############################################
  //! Prologue for strings for ExtendableBinary archives
  template<class CharT, class Traits, class Alloc> inline
  void prologue(ExtendableBinaryOutputArchive &, std::basic_string<CharT, Traits, Alloc> const &)
  { }

  //! Prologue for strings for ExtendableBinary archives
  template<class CharT, class Traits, class Alloc> inline
  void prologue(ExtendableBinaryInputArchive &, std::basic_string<CharT, Traits, Alloc> const &)
  { }

  //! Epilogue for strings for ExtendableBinary archives
  template<class CharT, class Traits, class Alloc> inline
  void epilogue(ExtendableBinaryOutputArchive &, std::basic_string<CharT, Traits, Alloc> const &)
  { }

  //! Epilogue for strings for ExtendableBinary archives
  template<class CharT, class Traits, class Alloc> inline
  void epilogue(ExtendableBinaryInputArchive &, std::basic_string<CharT, Traits, Alloc> const &)
  { }


  // ######################################################################
  //! Prologue for all other types for ExtendableBinary archives (except minimal types)
  /*! Starts a new node, named either automatically or by some NVP,
      that may be given data by the type about to be archived

      Minimal types do not start or finish nodes */
  template <class T, traits::EnableIf<!std::is_arithmetic<T>::value,
                                      !traits::has_minimal_base_class_serialization<T, traits::has_minimal_output_serialization, ExtendableBinaryOutputArchive>::value,
                                      !traits::has_minimal_output_serialization<T, ExtendableBinaryOutputArchive>::value,
                                      !is_extendablebinary_empty_prologue_and_epilogue1<T>::value> = traits::sfinae>
  inline void prologue( ExtendableBinaryOutputArchive & ar, T const & )
  {
    using namespace extendable_binary_detail;
    std::uint8_t t = writeType(FieldType::class_t, 0);
    ar.template saveBinary<sizeof(std::uint8_t)>(&t, sizeof(std::uint8_t));
    std::cout << "prologue:" << typeid(T).name() << "\n";
  }

  //! Prologue for all other types for ExtendableBinary archives
  template <class T, traits::EnableIf<!std::is_arithmetic<T>::value,
                                      !traits::has_minimal_base_class_serialization<T, traits::has_minimal_input_serialization, ExtendableBinaryInputArchive>::value,
                                      !traits::has_minimal_input_serialization<T, ExtendableBinaryInputArchive>::value,
                                      !is_extendablebinary_empty_prologue_and_epilogue1<T>::value> = traits::sfinae>
  inline void prologue( ExtendableBinaryInputArchive & ar, T const & )
  {
    using namespace extendable_binary_detail;
    auto type = ar.loadTypeTagSkip<FieldType::class_t>();
    if(type.first == FieldType::ommited_field) {
      throw Exception("TODO ommited class");
    }
    // TODO block serialize
    // TODO object attributes
  }

  // ######################################################################
  //! Epilogue for all other types other for ExtendableBinary archives (except minimal types)
  /*! Finishes the node created in the prologue

      Minimal types do not start or finish nodes */
  template <class T, traits::EnableIf<!std::is_arithmetic<T>::value,
                                      !traits::has_minimal_base_class_serialization<T, traits::has_minimal_output_serialization, ExtendableBinaryOutputArchive>::value,
                                      !traits::has_minimal_output_serialization<T, ExtendableBinaryOutputArchive>::value,
                                      !is_extendablebinary_empty_prologue_and_epilogue1<T>::value> = traits::sfinae>
  inline void epilogue( ExtendableBinaryOutputArchive & ar, T const & )
  {
    using namespace extendable_binary_detail;
    std::uint8_t t = writeType(FieldType::last_field, 0);
    ar.template saveBinary<sizeof(std::uint8_t)>(&t, sizeof(std::uint8_t));
    std::cout << "epilogue:" << typeid(T).name() << "\n";
  }

  //! Epilogue for all other types other for ExtendableBinary archives
  template <class T, traits::EnableIf<!std::is_arithmetic<T>::value,
                                      !traits::has_minimal_base_class_serialization<T, traits::has_minimal_input_serialization, ExtendableBinaryInputArchive>::value,
                                      !traits::has_minimal_input_serialization<T, ExtendableBinaryInputArchive>::value,
                                      !is_extendablebinary_empty_prologue_and_epilogue1<T>::value> = traits::sfinae>
  inline void epilogue( ExtendableBinaryInputArchive & ar, T const & )
  {
    using namespace extendable_binary_detail;
    ar.loadEndOfClass();
  }
} // namespace cereal

// register archives for polymorphic support
CEREAL_REGISTER_ARCHIVE(cereal::ExtendableBinaryOutputArchive)
CEREAL_REGISTER_ARCHIVE(cereal::ExtendableBinaryInputArchive)

// tie input and output archives together
CEREAL_SETUP_ARCHIVE_TRAITS(cereal::ExtendableBinaryInputArchive, cereal::ExtendableBinaryOutputArchive)

#endif // CEREAL_ARCHIVES_EXTENDABLE_BINARY_HPP_
