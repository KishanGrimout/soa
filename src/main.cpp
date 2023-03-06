
#include "soa/soa.h"

#include <algorithm>
#include <assert.h>
#include <string>
#include <utility>

// Demo purpose position type
struct alignas(16) vector3
{
    float x, y, z, w{};
};

inline bool operator==(const vector3& _lhs, const vector3& _rhs)
{
    return &_lhs == &_rhs;
}

// Small helper type to check copies or moves are performed on some operations
struct Checker
{
    Checker() : defaultCtor{ true } {}
    Checker(const Checker&) : copyCtor{ true } {}
    Checker(Checker&&) : moveCtor{ true } {}

    Checker& operator=(const Checker&)
    {
        copyAssign = true;
        return *this;
    }

    Checker& operator=(Checker&&)
    {
        moveAssign = true;
        return *this;
    }

    bool defaultCtor{ false };
    bool copyCtor{ false };
    bool moveCtor{ false };
    bool copyAssign{ false };
    bool moveAssign{ false };
};

inline bool operator==(const Checker& _lhs, const Checker& _rhs)
{
    return &_lhs == &_rhs;
}

// To declare a structure of arrays,
// you need strongly typed indices to access your members in the structure of arrays
enum class Example
{
    Position,
    NumItems,
    Life,
    Name,
    Checker,
    Count // The Count item is mandatory to validate the number of members vs. the number of types
};

// You create your structure of arrays, given your members description enum
// and the list of members types, that must match your enum
using ExampleArray = soa::vector<Example, vector3, int, float, std::string, Checker>;

class AllocatorInterface
{
public:
    virtual ~AllocatorInterface() = default;
    virtual void* allocate(size_t _size, size_t _align) = 0;
    virtual void free(void* _ptr) = 0;
};

class ActualAllocator : public AllocatorInterface
{
    void* allocate(size_t _size, size_t /*_align*/) override
    {
        return new char[_size];
    }

    void free(void* _ptr) override
    {
        delete[] static_cast<char*>(_ptr);
    }
};

// You can also create one with a custom allocator.
class PolymorphicAllocator
{
    AllocatorInterface& m_allocator;

public:
    PolymorphicAllocator(AllocatorInterface& _allocator)
        : m_allocator(_allocator)
    {

    }

    template<typename T>
    T* allocate(size_t _count)
    {
        return static_cast<T*>(m_allocator.allocate(_count * sizeof(T), alignof(T)));
    }

    template<typename T>
    void free(T* _ptr)
    {
        m_allocator.free(_ptr);
    }
};
using ExampleCustomAllocator = soa::vector_base<Example, PolymorphicAllocator, vector3, int, float, std::string, Checker>;

int main()
{
    // Create an empty SOA
    ExampleArray test;

    ActualAllocator allocator;
    ExampleCustomAllocator testCustom{ allocator };

    // Interface is similar to std::vector
    size_t size = test.size();
    assert(size == 0);

    bool empty = test.empty();
    assert(empty);

    // All operations will apply to all the vectors
    test.reserve(10);
    size_t capacity = test.capacity();
    assert(capacity == 10);

    // You can push_back a new element, consisting into a list of values matching your members list
    test.push_back(vector3{ 1.f, 2.f, 3.f }, 4, 5.f, std::string{ "test name" }, Checker{});
    assert(test.size() == 1);

    // And then access the member of an element of the array, from the enum value and array index
    const vector3& pos = std::as_const(test).at<Example::Position>(0);
    assert(pos.x == 1.f && pos.y == 2.f && pos.z == 3.f);

    int numItems = test.at<Example::NumItems>(0);
    assert(numItems == 4);

    float life = test.at<Example::Life>(0);
    assert(life == 5.f);

    const std::string& name = test.at<Example::Name>(0);
    assert(name == "test name");

    const Checker& checker = test.at<Example::Checker>(0);
    assert(checker.moveCtor);

    // You can get a tuple with copies of all members of a given array index
    ExampleArray::value_list values = test.value_at(0);
    std::get<static_cast<size_t>(Example::Position)>(values).x = 8.f;
    assert(std::get<static_cast<size_t>(Example::Checker)>(values).copyCtor);

    // Or a tuple of references
    ExampleArray::reference_list refs = test.ref_at(0);
    assert(refs == test.front());
    std::get<static_cast<size_t>(Example::Position)>(refs).x = 8.f;

    // Or a tuple of const references
    ExampleArray::const_reference_list constRefs = std::as_const(test).back();

    // You can also push a tuple with values
    test.push_back(values);
    assert(test.at<Example::Checker>(test.size() - 1).copyCtor);

    // Or references
    test.push_back(refs);
    assert(test.at<Example::Checker>(test.size() - 1).copyCtor);

    // Or const references
    test.push_back(constRefs);
    assert(test.at<Example::Checker>(test.size() - 1).copyCtor);

    // And the same for insertion
    test.insert(test.size(), values);
    test.insert(test.size(), refs);
    test.insert(test.size(), constRefs);

    // Resize is supported
    test.resize(3);
    assert(test.size() == 3);

    // As well as erase, based on indices
    size_t newIndex = test.erase(0);
    assert(newIndex == 0);
    assert(test.size() == 2);

    // Also with default value, from a list of default values for each member
    test.resize(4, vector3{ 6.f, 7.f, 8.f }, 9, 10.f, "other name", Checker{});
    assert(test.at<Example::Checker>(test.size() - 1).copyCtor);

    // Or with a tuple of default values
    test.resize(5, values);
    assert(test.at<Example::Checker>(test.size() - 1).copyCtor);

    test.pop_back();
    assert(test.size() == 4);

    // Insertion is only done from an index and not an iterator to simplify implementation
    test.insert(0, vector3{ 11.f, 12.f, 13.f }, 14, 15.f, "first name", Checker{});
    assert(test.at<Example::Checker>(0).moveCtor);

    // But there are iterators of the full structure
    {
        ExampleArray::iterator it = test.begin();
        auto end = test.end();
        assert(it != end);
        *it;
        static_assert(std::is_same<vector3&, decltype(it.value<Example::Position>())>::value, "iterator::value() returns a reference on the member");
        [[maybe_unused]] vector3& itPos = it.value<Example::Position>();
        assert(end - it == static_cast<ptrdiff_t>(test.size()));
    }

    // Const iterators
    {
        ExampleArray::const_iterator it = std::as_const(test).begin();
        *it;
        static_assert(std::is_same<const vector3&, decltype(it.value<Example::Position>())>::value, "const_iterator::value() returns a const reference on the member");
        [[maybe_unused]] const vector3& itPos = it.value<Example::Position>();
    }

    // Test compilation for partial_const_iterator
    {
        ExampleArray::partial_const_iterator<Example::Position, Example::Name> constIt = std::as_const(test).begin<Example::Position, Example::Name>();
        *constIt;
        static_assert(std::is_same<const vector3&, decltype(constIt.value<Example::Position>())>::value, "partial_const_iterator::value() returns a const reference on the member");
        [[maybe_unused]] const vector3& constItPos = constIt.value<Example::Position>();
    }

    // And also partial iterators, working on a subset of members, to iterate only on the members you need
    {
        ExampleArray::partial_iterator<Example::Position, Example::Name> it = test.begin<Example::Position, Example::Name>();
        auto end = test.end<Example::Position, Example::Name>();
        assert(end - it == static_cast<ptrdiff_t>(test.size()));

        for (; it != end; ++it)
        {
            // When dereferencing an iterator, you get a tuple of the members you selected.
            //ExampleArray::partial_ref_list<Example::Position, Example::Name> refIt = *it;

            //// ReferenceList is a tuple, so can be accessed like a regular tuple.
            //std::get<vector3&>(refIt).x = 1.f;

            //// Beware that the tuple indices are specific to that tuple, and you can't use the members description enum values.
            //std::get<1>(refIt) = "new value";

            //// But there are helper methods allowing to reuse the MembersDesc enum values and map to the correct tuple element.
            //[[maybe_unused]] vector3& itPos = it.value<Example::Position>();
            //[[maybe_unused]] std::string& itName = it.value<Example::Name>();
        }
    }

    //[[maybe_unused]] ExampleArray::partial_iterator<Example::Position, Example::Name> findItem = std::find_if(
    //    test.begin<Example::Position, Example::Name>(),
    //    test.end<Example::Position, Example::Name>(),
    //    [](const ExampleArray::partial_ref_list<Example::Position, Example::Name>& _element) {
    //        // Same remark here, the tuple elements are for the partial_ref_list, so you need to use appropriate parameter
    //        return !std::get<std::string&>(_element).empty();
    //    });

    // And... we are done!
    test.clear();
    assert(test.empty());

    {
        test.shrink_to_fit();
        ExampleArray::const_iterator itOnEmpty = test.cbegin();
        assert(itOnEmpty == test.cend());
    }

    return 0;
}
