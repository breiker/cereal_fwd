/*! \file extendable_binary_details.hpp
    \brief Implementation details for extendable binary archive */
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
#ifndef CEREAL_DETAILS_EXTENDABLE_BINARY_DETAILS_HPP_
#define CEREAL_DETAILS_EXTENDABLE_BINARY_DETAILS_HPP_

#include <cereal/cereal.hpp>
#include <limits>
#include <sstream>
#include <queue>
#include <assert.h>

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
    /*! Max size if four bits, then four bits for field specific encoding */
    enum class FieldType
    {
        /*!< omitted field (0000)  */
            omitted_field = 0x0,
        /*!< int (0001 | ssss)  size in bytes
            Size is stored with one byte intervals up to 8 bytes, then full sizes.
            Max size 16 bytes */
            positive_integer = 0x1,
        /*!< int (0010 | ssss)  size in bytes
            Size is stored with one byte intervals up to 8 bytes, then full sizes.
            Max size 16 bytes */
            negative_integer = 0x2,
        /*!< float (0011 | size_of_floating_point) */
            floating_point = 0x3,
        /*!< int packed (0100) (optional) */
            integer_packed = 0x4,
        /*!< class (0101) */
            class_t = 0x5,
        /*!< pointer (0110)
             null
             shared - objectid follows
             virtual - typeid follows */
            pointer = 0x6,
        /*!< size for packed_array or containers (0111 | ssss) size in bytes
            size mapping @see positive_integer or @see getIntSizeFromTagSize() */
            size_tag = 0x7,
        /*!< packed array (1000 | size of elem)
            e.g. string */
            packed_array = 0x8,
        /*!< struct (1001) */
            packed_struct = 0x9,
        /*!< was last field in class (1010)
         (not optional fields, class id) */
            last_field = 0xa,
        LAST_RESERVED_UNUSED
    };

    //! Markers for FieldType::class_t
    /*! Max size is four bits */
    enum class ClassMarkers : std::uint8_t
    {
        /*!< For initialization or lack of properties */
            None = 0,
        /*!< Optimalization for class without empty fields
            After empty class FieldType::last_field is not saved to save some space.*/
            EmptyClass = 0x1 << 0,
        /*!< This class has version set to value bigger than 0
            After this tag version as varint is saved, so usually 1 byte. */
            HasVersion = 0x1 << 1,
    };

    ClassMarkers& operator|=(ClassMarkers& l, ClassMarkers r)
    {
      l = static_cast<ClassMarkers>(static_cast<std::uint8_t>(l) | static_cast<std::uint8_t>(r));
      return l;
    }

    std::uint8_t operator&(ClassMarkers l, ClassMarkers r)
    {
      return static_cast<std::uint8_t>(l) & static_cast<std::uint8_t>(r);
    }

    //! Markers for FieldType::pointer
    /*! Max size is four bits */
    enum class PointerMarkers : std::uint8_t
    {
        /*!< For initialization or lack of properties */
            None = 0,
        /*!< nullptr or object with the same object id was already saved */
            Empty = 0x1 << 0, // None (0) can be used
        /*!< Pointer is shared (shared or weak_ptr)
             After this tag, object id as varint will be saved */
            IsSharedPtr = 0x1 << 1,
        /*!< Pointer is polymorphic, casting will be needed
             After this tag, polymorphic id as 4 byte std::int32_t will be saved */
            IsPolymorphicPointer = 0x1 << 2
    };

    PointerMarkers& operator|=(PointerMarkers& l, PointerMarkers r)
    {
      l = static_cast<PointerMarkers>(static_cast<std::uint8_t>(l) | static_cast<std::uint8_t>(r));
      return l;
    }

    std::uint8_t operator&(PointerMarkers l, PointerMarkers r)
    {
      return static_cast<std::uint8_t>(l) & static_cast<std::uint8_t>(r);
    }

    //! max size of saved varint
    /*! enough to save uint64_t 8 */
    enum { maxVarintSize = 10 };

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

    //! Get number of bytes needed to save value.
    /*! TODO can be done quicker http://stackoverflow.com/questions/2274428/how-to-determine-how-many-bytes-an-integer-needs
       \param v value to check
       \return number of bytes needed to save this value */
    template <class T> inline
    std::uint8_t getHighestBit(T v)
    {
      // TODO fix for big endian
      std::uint8_t n = 0;
      while( v != 0 ) {
        v >>= 8;
        n++;
      }
      return n;
    }

    //! Get number of bytes needed to save value.
    /*! Separate function is needed because of error "shift count >= width of type [-Werror,-Wshift-count-overflow]" */
    inline std::uint8_t getHighestBit(std::uint8_t)
    {
      return 1;
    }

    template <class T> inline
    T getqNaN();

    template <> inline
    float getqNaN()
    {
      union { float f; std::uint32_t i; } f;
      f.i = 0x7fc00000;
      return f.f;
    }

    template <> inline
    double getqNaN()
    {
      union { double d; std::uint64_t i; } d;
      d.i = 0x7FF8000000000000ull;
      return d.d;
    }

    //! Get quiet not a number value for type
    template <class T> inline
    T getqNaN()
    {
      static_assert(traits::detail::delay_static_assert<T>::value, "unknown qNaN value");
    }

    //! Get size of floating point data for size identifier
    inline std::uint8_t getFloatSizeFromTagSize(std::uint8_t tagSize)
    {
      switch (tagSize) {
        case 1:
          return 4;
        case 2:
          return 8;
        default:
          throw Exception("Unsupported floating point size");
      }
    }

    //! Get size identifier for floating point type
    /*! @tparam T floating point type */
    template <class T> inline
    std::uint8_t getTagSizeFromFloatType()
    {
      static_assert(sizeof(T) == 4 || sizeof(T) == 8, "unsupported floating point type");
      static_assert(std::is_floating_point<T>::value, "expected floating point type");
      switch (sizeof(T)) {
        case 4:
          return 1;
        case 8:
          return 2;
      }
    }

    //! Get number of bytes to load based on value of identifier for integer tag
    inline std::uint8_t getIntSizeFromTagSize(std::uint8_t tagSize)
    {
      switch (tagSize) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
          return tagSize;
        case 9:
          return 16;
        case 10:
          return 32;
        default:
          throw Exception("Unsupported int size");
      }
    }


    //! Get identifier for size for integer tag based on number of bytes being stored
    inline std::uint8_t getIntSizeTagFromByteCount(unsigned int byteCount) {
      if(byteCount <= 8) {
        return byteCount;
      } else if(byteCount <= 16) {
        return 9;
      } else if(byteCount <= 32) {
        return 10;
      } else {
        throw Exception("Unsupported int size");
      }
    }

    //! Struct to keep position of start and end in stream
    struct StreamPos
    {
      std::streamoff start;
      std::streamoff end;
    };

    //! Class used as an adapter to queue of streams
    /*! Two streams are managed. Main stream can be used only for reading and reading position
        can move only forward. Position of reading for second stream can be moved freely.
        For second stream queue of start and end positions is kept. New StreamPos can be added
        when last StreamPos was not yet read fully. When StreamPos::end for top StreamPos is reached
        reading from next StreamPos is continued. If there are no StreamPos on the queue data
        is read from main stream.

        Additional functions for copying data from one stream to the other are provided (readToOtherStream()).
     */
    class StreamAdapter
    {
      public:
        //! Construct new object with main and secondary stream
        /*! @param stream main reading stream
            @param sharedObjectStream secondary stream for which new reading positions could be given
            @param maxBytesInSharedStream max size of data copied to other stream in readToOtherStream() method
         */
        StreamAdapter(std::istream & stream, std::stringstream & sharedObjectStream, std::size_t maxBytesInSharedStream)
            : nowReading(nullptr), bytesLeft(0), endOfWritingStream(0), startOfStream(sharedObjectStream.tellg()),
              mainStream(stream), backStream(sharedObjectStream), maxBytesSharedStream(maxBytesInSharedStream)
        {}

        //! Pushes new reading position on the stream
        /*! @param streamPos new reading pos, saves reference which has to be valid for whole object lifetime
            Throws if not enough bytes are read */
        void pushReadingPos(StreamPos & streamPos)
        {
          if (nowReading == nullptr) {
            // save backStream pos for restoration
            endOfWritingStream = backStream.tellg();
          } else {
            // save current position when we get back to this level
            backStreams.push(std::make_pair(backStream.tellg(), nowReading));
          }
          backStream.seekg(streamPos.start);
          nowReading = &streamPos;
          bytesLeft = streamPos.end - streamPos.start;
        }

        //! Reads binary data from stream which is on top
        /*! @param data address to read to
            @param size size of data to read
            Size has to be less or equal to data available at the current stream position. That is:
            - If using streamPos on queue - end of current streamPos
            - If using mainStream - end of mainStream
            Throws if not enough bytes are read. */
        inline std::size_t readBinary(void *const data, std::size_t size)
        {
          std::size_t readSize;
          if (nowReading == nullptr) {
            readSize = static_cast<std::size_t>( mainStream.rdbuf()->sgetn(reinterpret_cast<char *>( data ), size));
          } else {
            if (size > bytesLeft) {
              throw Exception("went to far reading skipped shared object stream");
            }
            readSize = static_cast<std::size_t>( backStream.rdbuf()->sgetn(reinterpret_cast<char *>( data ), size));
            bytesLeft -= readSize;
            if (0 == bytesLeft) {
              popStream();
            }
          }
          return readSize;
        }

        //! Discards size bytes from the input stream
        /*! @param size The number of bytes of data
            Throws if not enough bytes are read */
        inline void skipData(std::size_t size)
        {
          bool streamError;
          if (nowReading == nullptr) {
            mainStream.ignore(size);
            streamError = !mainStream; // we don't care about eof here
          } else {
            if (size > bytesLeft) {
              throw Exception("went to far reading skipped shared object stream");
            }
            backStream.ignore(size);
            streamError = !backStream; // we don't care about eof here
            bytesLeft -= size;
            if (0 == bytesLeft) {
              popStream();
            }
          }
          if (streamError)
            throw Exception("Failed to skip " + std::to_string(size) + " bytes from input stream!");
        }

        //! Check if by adding size bytes limit for max copied data is hit
        /*! @param size additional size bytes to read
            @param stream Stream used for data copying
            Throws Exception if limit would be hit */
        inline void checkIfMaxSize(std::size_t size, std::ostream& stream)
        {
          std::size_t posNow = static_cast<std::size_t>(stream.tellp());
          if (posNow - startOfStream + size > maxBytesSharedStream) {
            throw Exception("Shared obiect shared data limit hit");
          }
        }

        //! Reads size bytes from the input stream and copies them to other stream
        /*! @param size The number of bytes to read and copy
            @param stream Stream to copy data to
            Throws Exception if not enough bytes are read */
        inline void readToOtherStream(std::size_t size, std::ostream & stream)
        {
          checkIfMaxSize(size, stream);
          auto copyN = [](std::istream & from, std::size_t sizeToCopy, std::ostream & to) {
            auto start = std::istreambuf_iterator<char>(from);
            auto end = std::istreambuf_iterator<char>();
            auto dest = std::ostreambuf_iterator<char>(to);
            for (; sizeToCopy > 0 && start != end; ++start, ++dest, --sizeToCopy) {
              *dest = *start;
            }
            if (0 != sizeToCopy)
              throw Exception("Failed to skip data from input stream!");
          };

          if (nowReading == nullptr) {
            copyN(mainStream, size, stream);
          } else {
            // shouldn't be here, there is no point in copying data if we are reading from backStream
            assert(false);
            if (size > bytesLeft) {
              throw Exception("went to far reading skipped shared object stream");
            }
            copyN(mainStream, size, stream);
            bytesLeft -= size;
            if (0 == bytesLeft) {
              popStream();
            }
          }
        }

      private:

        //! Finish reading current StreamPos
        void popStream()
        {
          if (backStreams.empty()) {
            // move backStream back where it was for writing
            backStream.seekg(endOfWritingStream);
            nowReading = nullptr;
            bytesLeft = 0;
          } else {
            const auto & next = backStreams.back();
            backStream.seekg(next.first);
            // we read something before so next.first has to be used
            bytesLeft = next.second->end - next.first;
            nowReading = next.second;
            backStreams.pop();
          }
        }

      private:
        StreamPos *nowReading; //!< current reading stream position
        std::size_t bytesLeft; //!< how many bytes are left for nowReading position
        std::size_t endOfWritingStream; //!< end of writing backStream,
                                        //!< will be restored when reading from other position is done
        const std::size_t startOfStream; //!< start of stream at construction of object
        std::queue<std::pair<std::size_t, StreamPos *>> backStreams;
        std::istream & mainStream; //!< main reading stream
        std::istream & backStream; //!< stream to keep data from skipped shared pointers
        const std::size_t maxBytesSharedStream; //!< max allowed size of data copied to backStream
    };
  } // namespace extendable_binary_detail
} // namespace cereal
#endif
