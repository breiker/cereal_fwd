Preface
=======

Library is under active developement and parts of it may change.

This documentation is intended as an extension to official cereal
documentation available at <http://uscilab.github.io/cereal/index.html>.

Doxygen documentation is available at <https://breiker.github.io/cereal_fwd/doxygen/index.html>.

Introduction
============

This documents describes modification made to cereal library to support
forward compatibility and portability between different platforms.\
Modifications were focused on binary format for which new ExtendableBinary archive
was made.\
Unless specified otherwise information applies to ExtendableBinary
archive, not necessarily to other archives.\

Main goals
----------

-   Support backward[^1] and forward[^2] compatibility.

-   Minimal size of saved data without hindering ability to evolve
    structure of serialized data.

-   Support streaming when saving and loading.

-   Minimize allocations during saving and loading process.

-   Support portability between different platforms.

-   Don’t break compatibility for existing archives in library.

-   Loading data from unknown source cannot result in undefined behavior
    (e.g. crash).

Backward and forward compatibility
----------------------------------

Backward compatibility is already supported for all archives in cereal
(see “Explicit versioning” in
<http://uscilab.github.io/cereal/serialization_functions.html>). Forward
compatibility support was added to ExtendableBinary archive.

Portability
-----------

There are two main problems with making archive portable between
platforms — endianness and size of integers.

ExtendableBinary archive, similarly to PortableBinary archive, saves
byte order at the beginning of stream. If platform used for reading has
the same byte order data is read as is. Otherwise, archive swaps bytes
for each primitive type. This results in slight run-time overhead.

Additionally, size of integer for not fixed size integers may differ
between platforms. Saving types such as *std::size\_t* or *long* may
produce data which may not be readable on other platforms. In
ExtendableBinary archive all integers used as fields are saved with size
information connected to them. This makes it possible to load integers
which have different size on writing and reading side. Loading integer
that cannot fit into type used for loading will result in exception. For
space saving purposes size of saved integer is determined as minimal
number of bytes needed to represent stored number.\
Note that above optimizations are not used when handing arrays of binary
data (e.g. vector or array of integers). Fixed size integers should be
preferred for types used in such situations.

Added features
==============

Omit field tag
--------------

During class evolution it may be decided that some fields are no longer
used and don’t have to be saved or loaded. Saving them would make
unnecessary space and run-time overhead. Some archives requires
preservation of serialized field order to work. That is why simply
deleting serialization of field may not be the best solution. For that
situation *OmittedFieldTag* was introduced. It is intended to be used as
indication that, in this position, there used to be a field but will not
be saved and loaded anymore.

Loading field which was saved with *OmittedFieldTag* doesn’t result in
exception. Field is not loaded and serialize function is not called.\
Field which was serialized but is loaded with *OmittedFieldTag* makes
archive skip saved data for that field and prepares archive for loading
next field.

Use of *OmittedFieldTag* makes saving fields optional. To allow checking
if field was saved *Archive::wasSerialied* method is introduced. Return
value of that method indicates if field was saved to archive or
*OmittedFieldTag* was used.

Possible return value can be seen in table below:\
T - type is written/read\
O - OmittedFieldTag is used


  |Writing side   |Reading side   |Return value|
  |-------------- |-------------- |------------|
  |T              |T              |true        |
  |O              |O              |false       |
  |T              |O              |true        |
  |O              |T              |false       |

As an example struct *A* seen below has field *b* which is not longer
needed. In new version it is possible to mark field serialized as second
with *OmittedFieldTag*.

    // old version
      struct A {
        int a;
        B b; // type is irrelevant here
        float c;
        template <class Archive>
        void serialize(Archive& ar, std::uint32_t version)
        {
          ar(a);
          ar(b);
          ar(c);
        }
      };
    // new version
      struct A {
        int a;
        float c;
        template <class Archive>
        void serialize(Archive& ar, std::uint32_t version)
        {
          ar(a);
          // b field is no longer used
          ar(cereal::OmittedFieldTag());
          ar(c);
        }
      };

Archive’s *wasSerialized* method can be used to check if field *b* was
serialized.

    template<class Archive>
    void serialize(Archive& ar, std::uint32_t version)
    {
      ar(a);
      ar(b);
      if(ar.wasSerialized())
        std::cout << "field b was serialized\n";
      else
        std::cout << "field b was not serialized\n";
      ar(c);
    }

Shared pointer reading
----------------------

Data for shared pointers pointing to the same address is saved only once
(<http://uscilab.github.io/cereal/pointers.html>). Supporting forward
compatibility requires ability to skip loading of fields which are not
known or not used in that version. As a result, data which is defined
once but not used has to available for later usage in case same shared
pointer may be loaded later.\
In ExtendableBinary archive data for skipped shared pointers is copied
into memory buffer for later usage. Moving back in stream is not an
option since that would break streaming support. Please note that in
worst case scenario size of in memory buffer can be close to original
size of archive.
Maximum size of memory buffer can be limited by passing modified 
*Options* to *ExtendableBinaryInputArchive* constructor. Size can be
changed with Options::maxSharedBufferSize method. If maximum size is
exceeded exception will be thrown.

Unknown polymorphic pointers
----------------------------

Trying to read unknown or not registered polymorphic pointer would
normally result in exception being thrown. That means that introducing
new derived class would have to be done by adding new field and not using
existing base class pointer. In ExtendableBinaryArchive exception is not
thrown when reading unknown polymorphic pointer, field is just set to
nullptr. This behavior can be disabled with *ignoreUnknownPolymorphicTypes*
method in archive's *Options* class.

Class evolution
===============

Example
-------

    class A {
      int a;
      float b;

      template<class Archive>
      void serialize(Archive& ar, std::uint32_t version)
      {
        ar(a, b);
      }
    };

    class A {
      int a;
      float b;
      C c;

      template<class Archive>
      void serialize(Archive& ar, std::uint32_t version)
      {
        ar(a, b);
        if(version >= 1)
          ar(c);
      }
    };
    CEREAL_CLASS_VERSION( A, 1 );

Example above shows class *A* which in old version has two fields, *a*
and *b*. After some time new field, *c*, is added. Adding new field to
serialization requires:

-   Updating class version with CEREAL\_CLASS\_VERSION.

-   Adding field to serialization function conditionally on version
    passed in parameter.

Conditionally loading field guarantees that field will be loaded only
when it was actually saved.\
Loading new version with code supporting only old version will result in
loading *a* and *b* fields. Unknown fields at the end of class will be
skipped, in this situation *c* field.

Permitted changes
-----------------

-   Not loading fields from the end of class/struct is permitted.

-   Adding new fields at the end of serialization code. New fields have
    to be loaded conditionally on class version saved to archive.

-   Changing size of integers; if values saved with older version were
    never bigger than can be stored in new size. Loading integer bigger
    than can be stored in field used for loading results in exception.

-   Swapping structs or classes to different types serialized in the
    same way.

    -   Field cannot be saved as shared pointer.

    -   Field cannot be saved as pointer to base class.

-   Changing type of serialized field to *OmittedFieldTag*.

    -   Changing field to OmittedFieldTag can be done even without
        changing class version and saving it conditionally on class
        version.

-   Changing serialize function from variant without *version* parameter
    to variant with one. (Contrary to “Explicit versioning” -
    <http://uscilab.github.io/cereal/serialization_functions.html>)

Forbidden changes
-----------------

-   Trying to load more fields than were saved will result in exception.

-   Changing sign of integer and loading number that doesn’t fit into
    new type e.g. loading negative number to unsigned type.

-   Changing size of floating point types (float, double).

-   Changing order in which fields are serialized.

-   Adding new field for serialization without updating class version
    and loading it without checking version.

-   Adding new field for serialization not as the last field.

Basic usage
============

Library is header only. To use it *include* folder has to be added to include directories.

Sample usage can be found below:

    #include <memory>
    #include <fstream>
    #include <cassert>
    
    #include <cereal/types/polymorphic.hpp> // for polymorphic pointers
    #include <cereal/types/string.hpp> // for string
    #include <cereal/archives/extendable_binary.hpp>

    class B {
    public:
        virtual ~B() {}
        int b = 3;
        template <class Archive>
        void serialize(Archive & ar, const std::uint32_t version) {
            ar(b);
        }
    };
    class A : public B {
    public:
        std::string a = "char";
        template <class Archive>
        void serialize(Archive & ar, const std::uint32_t version) {
            ar(cereal::base_class<B>(this));
            ar(a);
        }
    };
    CEREAL_REGISTER_TYPE(A)

    int main() {
        std::unique_ptr<B> ptr_o = std::make_unique<A>();
        {
        std::ofstream ofs("filename", std::ios::binary);
        cereal::ExtendableBinaryOutputArchive oa(ofs);
        oa(ptr_o);
        }

        std::unique_ptr<B> ptr_i;
        std::ifstream ifs("filename", std::ios::binary);
        cereal::ExtendableBinaryInputArchive ia(ifs);
        ia(ptr_i);
        assert(ptr_i != nullptr && ptr_o->b == ptr_i->b);
        assert(typeid(ptr_i) == typeid(ptr_o));
    }


Planned changes
===============

-   Saving vectors with arithmetic types and strings can be handled in
    more efficient way.

-   More testing on big endian platform.

[^1]: <https://en.wikipedia.org/wiki/Backward_compatibility>

[^2]: <https://en.wikipedia.org/wiki/Forward_compatibility>
