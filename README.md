# soa
A vector-like class to create structures of arrays.

See src/main.cpp for a detailed usage sample.

## ChatGPT explanation

### Description

`vector_base` is a class template that provides a basic implementation of a vector-like container using the Structure of Arrays (SoA) data layout. The container stores its elements in separate arrays for each data member, rather than storing each element as a struct or object with all data members together.

The `vector_base` class provides a number of member functions for manipulating the container, such as `size()`, `capacity()`, `empty()`, `reserve()`, `shrink_to_fit()`, `clear()`, `push_back()`, `pop_back()`, and `resize()`. It also provides a number of iterator classes: `const_iterator`, `iterator`, `partial_const_iterator`, and `partial_iterator`.

The template parameters of `vector_base` are:

    MembersDesc: an enum type that lists the data members of the container.
    Allocator: the allocator type to use for allocating and deallocating the memory used by the container.
    Types...: the types of the data members of the container.

The data members are specified by `MembersDesc`, which should be an enum whose values correspond to the indices of the `Types...` parameter pack. For example, if `Types...` is `int`, `float`, `bool`, then `MembersDesc` might be:

    enum class MembersDesc
    {
        Int = 0,
        Float = 1,
        Bool = 2,
        Count = 3
    };

The `value_list`, `reference_list`, and `const_reference_list` typedefs are tuples of the data members, data members references, and const data member references, respectively.

The `partial_ref_list` and `partial_const_ref_list` templates are used to create partial views of the container, where only a subset of the data members are exposed. These templates take a list of member indices as template parameters and produce tuples of references to those members.

Overall, `vector_base` provides a flexible, low-level building block for constructing data structures using the SoA layout, allowing for efficient data access and manipulation.

### partial_iterator test
    // Tests the partial iterators of a vector_base
    TEST(VectorBaseTest, PartialIterator)
    {
        using VectorBase = vector_base<MembersDesc, Allocator, int, float, double>;
        VectorBase vector;
        vector.push_back(1, 2.0f, 3.0);

        // Test partial iteration over int and double elements
        auto it = vector.begin<MembersDesc::Int, MembersDesc::Double>();
        EXPECT_EQ(std::get<0>(*it), 1);
        EXPECT_EQ(std::get<1>(*it), 3.0);
        ++it;
        EXPECT_EQ(it, vector.end<MembersDesc::Int, MembersDesc::Double>());
    }