#pragma once

#ifndef SOA_STD

#include <tuple>
#include <vector>
#include <array>
#include <type_traits>
#include <functional>
#include <cstdlib>
#include <memory.h>

namespace soa
{
    template <typename... Types>
    using tuple = std::tuple<Types...>;

    template <size_t Index, typename Tuple>
    using tuple_element_t = std::tuple_element_t<Index, Tuple>;

    template <typename T, typename Allocator>
    using container = std::vector<T, Allocator>;

    template <typename T, size_t Size>
    using array = std::array<T, Size>;

    template <size_t Size>
    using make_index_sequence = std::make_index_sequence<Size>;

    template <size_t... Vals>
    using index_sequence = std::index_sequence<Vals...>;

    template <typename T>
    using decay_t = std::decay_t<T>;

    template <typename T>
    using remove_const_t = std::remove_const_t<T>;

    using ptrdiff_t = std::ptrdiff_t;

    using std::apply;
    using std::get;
    using std::make_tuple;
    using std::forward_as_tuple;

#ifndef _WIN32
    using std::aligned_alloc;
    using std::free;
#endif
}

#endif

namespace soa
{
    template <typename MembersDesc, typename Allocator, typename... Types>
    class vector_base
    {
    public:
        using value_list = tuple<Types...>;
        using reference_list = tuple<Types&...>;
        using const_reference_list = tuple<const Types&...>;

        static constexpr size_t members_count{ sizeof...(Types) };
        static_assert(members_count == static_cast<size_t>(MembersDesc::Count), "The MembersDesc enum must match the number of types");

        template<MembersDesc... Members>
        using partial_ref_list = tuple<tuple_element_t<static_cast<size_t>(Members), value_list>&...>;

        template<MembersDesc... Members>
        using partial_const_ref_list = tuple<const tuple_element_t<static_cast<size_t>(Members), value_list>&...>;

    public:

        class const_iterator
        {
        public:
            using pointer = tuple<const Types*...>;
            using iterator_category = std::random_access_iterator_tag;
            using difference_type = ptrdiff_t;
            using reference = const_reference_list;
            using value_type = value_list;

            reference operator*() const
            {
                return apply([](auto... _obj) { return reference{ *(_obj)... }; }, m_ptr);
            }

            template<MembersDesc MemberIndex>
            const auto& value() const
            {
                return *get<MemberIndex>(m_ptr);
            }

            const_iterator& operator++()
            {
                apply([](auto*&... _obj) { (++_obj, ...); }, m_ptr);
                return *this;
            }

            const_iterator operator++(int)
            {
                iterator ret = *this;
                apply([](auto*&... _obj) { (_obj++, ...); }, m_ptr);
                return ret;
            }

            const_iterator& operator--()
            {
                apply([](auto*&... _obj) { (--_obj, ...); }, m_ptr);
                return *this;
            }

            const_iterator operator--(int)
            {
                iterator ret = *this;
                apply([](auto*&... _obj) { (_obj--, ...); }, m_ptr);
                return ret;
            }

            difference_type operator-(const const_iterator& _other) const
            {
                return get<0>(m_ptr) - get<0>(_other.m_ptr);
            }

            bool operator==(const const_iterator& _other) const
            {
                return m_ptr == _other.m_ptr;
            }

            bool operator!=(const const_iterator& _other) const
            {
                return m_ptr != _other.m_ptr;
            }

        protected:
            pointer m_ptr{};

            template<typename... Args>
            const_iterator(Args... _args)
                : m_ptr{ make_tuple(_args...) }
            {
            }

            friend class soa::vector_base<MembersDesc, Allocator, Types...>;
        };

        class iterator : public const_iterator
        {
        public:
            using pointer = tuple<Types*...>;
            using iterator_category = std::random_access_iterator_tag;
            using difference_type = ptrdiff_t;
            using reference = reference_list;
            using value_type = value_list;

            iterator() = default;

            reference_list operator*() const
            {
                return apply([](auto... _obj) { return reference_list{ *const_cast<remove_const_t<decltype(_obj)>>(_obj)...}; }, const_iterator::m_ptr);
            }

            template<MembersDesc MemberIndex>
            auto& value()
            {
                return *const_cast<tuple_element_t<static_cast<size_t>(MemberIndex), pointer>>(get<static_cast<size_t>(MemberIndex)>(const_iterator::m_ptr));
            }

            iterator& operator++()
            {
                const_iterator::operator++();
                return *this;
            }

            iterator operator++(int)
            {
                iterator ret = *this;
                const_iterator::operator++();
                return ret;
            }

            iterator& operator--()
            {
                const_iterator::operator--();
                return *this;
            }

            iterator operator--(int)
            {
                iterator ret = *this;
                const_iterator::operator--();
                return ret;
            }

        private:
            using const_iterator::const_iterator;

            friend class soa::vector_base<MembersDesc, Allocator, Types...>;
        };

        template<MembersDesc... Members>
        class partial_const_iterator
        {
        public:
            using pointer = tuple<const tuple_element_t<static_cast<size_t>(Members), value_list>*...>;
            using iterator_category = std::random_access_iterator_tag;
            using difference_type = ptrdiff_t;
            using reference = partial_const_ref_list<Members...>;
            using value_type = void;

            partial_const_ref_list<Members...> operator*() const
            {
                return apply([](auto... _obj) { return partial_const_ref_list<Members...>{ *(_obj)...}; }, m_ptr);
            }

            template<MembersDesc MemberIndex>
            const auto& value()
            {
                return *get<getIndex<MemberIndex>()>(m_ptr);
            }

            partial_const_iterator& operator++()
            {
                apply([](auto*&... _obj) { (++_obj, ...); }, m_ptr);
                return *this;
            }

            partial_const_iterator operator++(int)
            {
                partial_const_iterator ret = *this;
                apply([](auto*&... _obj) { (_obj++, ...); }, m_ptr);
                return ret;
            }

            partial_const_iterator& operator--()
            {
                apply([](auto*&... _obj) { (--_obj, ...); }, m_ptr);
                return *this;
            }

            partial_const_iterator operator--(int)
            {
                partial_const_iterator ret = *this;
                apply([](auto*&... _obj) { (_obj--, ...); }, m_ptr);
                return ret;
            }

            difference_type operator-(const partial_const_iterator& _other) const
            {
                return get<0>(m_ptr) - get<0>(_other.m_ptr);
            }

            bool operator==(const partial_const_iterator& _other) const
            {
                return m_ptr == _other.m_ptr;
            }

            bool operator!=(const partial_const_iterator& _other) const
            {
                return m_ptr != _other.m_ptr;
            }

        protected:
            static constexpr array<size_t, sizeof...(Members)> ms_mapping{ static_cast<size_t>(Members)... };

            pointer m_ptr{};

            template<typename... Args>
            partial_const_iterator(Args... _args)
                : m_ptr{ make_tuple(_args...) }
            {
            }

            friend class soa::vector_base<MembersDesc, Allocator, Types...>;

            template<MembersDesc MemberIndex, size_t Index = 0>
            static constexpr size_t getIndex()
            {
                if constexpr (ms_mapping[Index] == static_cast<size_t>(MemberIndex))
                    return Index;
                else
                    return getIndex<MemberIndex, Index + 1>();
            }
        };

        template<MembersDesc... Members>
        class partial_iterator : public partial_const_iterator<Members...>
        {
            using base_iterator = partial_const_iterator<Members...>;

            template<MembersDesc MemberIndex, size_t Index = 0>
            static constexpr size_t getIndex()
            {
                if constexpr (base_iterator::ms_mapping[Index] == static_cast<size_t>(MemberIndex))
                    return Index;
                else
                    return getIndex<MemberIndex, Index + 1>();
            }

        public:
            using pointer = tuple<tuple_element_t<static_cast<size_t>(Members), value_list>*...>;
            using iterator_category = std::random_access_iterator_tag;
            using difference_type = ptrdiff_t;
            using reference = partial_ref_list<Members...>;
            using value_type = void;

            partial_ref_list<Members...> operator*() const
            {
                return apply(
                    [](auto... _obj) {
                        return partial_ref_list<Members...>{ *(_obj)... };
                    },
                    base_iterator::m_ptr);
            }

            template<MembersDesc MemberIndex>
            auto& value()
            {
                return *const_cast<tuple_element_t<getIndex<MemberIndex>(), pointer>>(get<getIndex<MemberIndex>()>(base_iterator::m_ptr));
                //return *get<getIndex<MemberIndex>()>(base_iterator::m_ptr);
            }

            partial_iterator& operator++()
            {
                base_iterator::operator++();
                return *this;
            }

            partial_iterator operator++(int)
            {
                partial_iterator ret = *this;
                base_iterator::operator++();
                return ret;
            }

            partial_iterator& operator--()
            {
                base_iterator::operator--();
                return *this;
            }

            partial_iterator operator--(int)
            {
                partial_iterator ret = *this;
                base_iterator::operator--();
                return ret;
            }

        private:
            using base_iterator::partial_const_iterator;

            friend class soa::vector_base<MembersDesc, Allocator, Types...>;
        };

        vector_base() = default;

        explicit vector_base(Allocator _allocator)
            : m_soa{ container<Types, allocator_wrapper<Types>>{ std::move(_allocator) }... }
        {

        }

        size_t size() const
        {
            return get<0>(m_soa).size();
        }

        size_t capacity() const
        {
            return get<0>(m_soa).capacity();
        }

        bool empty() const
        {
            return get<0>(m_soa).empty();
        }

        iterator begin()
        {
            return begin_internal(make_index_sequence<members_count>{});
        }

        const_iterator begin() const
        {
            return begin_internal(make_index_sequence<members_count>{});
        }

        const_iterator cbegin() const
        {
            return begin_internal(make_index_sequence<members_count>{});
        }

        iterator end()
        {
            return end_internal(make_index_sequence<members_count>{});
        }

        const_iterator end() const
        {
            return end_internal(make_index_sequence<members_count>{});
        }

        const_iterator cend() const
        {
            return end_internal(make_index_sequence<members_count>{});
        }

        template<MembersDesc... Members>
        partial_iterator<Members...> begin()
        {
            return { get<static_cast<size_t>(Members)>(m_soa).data()... };
        }

        template<MembersDesc... Members>
        partial_iterator<Members...> end()
        {
            const size_t size{ this->size() };
            return { get<static_cast<size_t>(Members)>(m_soa).data() + size... };
        }

        template<MembersDesc... Members>
        partial_const_iterator<Members...> begin() const
        {
            return { get<static_cast<size_t>(Members)>(m_soa).data()... };
        }

        template<MembersDesc... Members>
        partial_const_iterator<Members...> end() const
        {
            const size_t size{ this->size() };
            return { get<static_cast<size_t>(Members)>(m_soa).data() + size... };
        }

        template<MembersDesc... Members>
        partial_const_iterator<Members...> cbegin() const
        {
            return { get<static_cast<size_t>(Members)>(m_soa).data()... };
        }

        template<MembersDesc... Members>
        partial_const_iterator<Members...> cend() const
        {
            const size_t size{ this->size() };
            return { get<static_cast<size_t>(Members)>(m_soa).data() + size... };
        }

        void reserve(size_t _capacity)
        {
            apply([_capacity](auto&&... _vec) { (_vec.reserve(_capacity), ...); }, m_soa);
        }

        void shrink_to_fit()
        {
            apply([](auto&&... _vec) { (_vec.shrink_to_fit(), ...); }, m_soa);
        }

        void clear()
        {
            apply([](auto&&... _vec) {	(_vec.clear(), ...); }, m_soa);
        }

        template<typename... Args>
        void push_back(Args&&... _args)
        {
            if constexpr (sizeof...(_args) == 1)
            {
                if constexpr (
                    std::is_same_v<decay_t<Args>..., reference_list> ||
                    std::is_same_v<decay_t<Args>..., const_reference_list> ||
                    std::is_same_v<decay_t<Args>..., value_list>)
                {
                    push_back_internal(std::forward<Args>(_args)..., make_index_sequence<members_count>{});
                }
                else
                {
                    push_back_internal(forward_as_tuple(std::forward<Args>(_args)...), make_index_sequence<members_count>{});
                }
            }
            else
            {
                push_back_internal(forward_as_tuple(std::forward<Args>(_args)...), make_index_sequence<members_count>{});
            }
        }

        void pop_back()
        {
            apply([](auto&&... _vec) {	(_vec.pop_back(), ...); }, m_soa);
        }

        void resize(size_t _size)
        {
            apply([_size](auto&&... _vec) { (_vec.resize(_size), ...); }, m_soa);
        }

        template<typename... Args>
        void resize(size_t _size, Args&&... _args)
        {
            if constexpr (sizeof...(_args) == 1)
            {
                if constexpr (
                    std::is_same_v<decay_t<Args>..., reference_list> ||
                    std::is_same_v<decay_t<Args>..., const_reference_list> ||
                    std::is_same_v<decay_t<Args>..., value_list>)
                {
                    resize_internal(_size, std::forward<Args>(_args)..., make_index_sequence<members_count>{});
                }
                else
                {
                    resize_internal(_size, forward_as_tuple(std::forward<Args>(_args)...), make_index_sequence<members_count>{});
                }
            }
            else
            {
                resize_internal(_size, forward_as_tuple(std::forward<Args>(_args)...), make_index_sequence<members_count>{});
            }
        }

        template<typename... Args>
        void insert(size_t _pos, Args&&... _args)
        {
            if constexpr (sizeof...(_args) == 1)
            {
                if constexpr (
                    std::is_same_v<decay_t<Args>..., reference_list> ||
                    std::is_same_v<decay_t<Args>..., const_reference_list> ||
                    std::is_same_v<decay_t<Args>..., value_list>)
                {
                    insert_internal(_pos, std::forward<Args>(_args)..., make_index_sequence<members_count>{});
                }
                else
                {
                    insert_internal(_pos, forward_as_tuple(std::forward<Args>(_args)...), make_index_sequence<members_count>{});
                }
            }
            else
            {
                insert_internal(_pos, forward_as_tuple(std::forward<Args>(_args)...), make_index_sequence<members_count>{});
            }
        }

        size_t erase(size_t _pos)
        {
            return erase(_pos, _pos + 1);
        }

        size_t erase(size_t _startPos, size_t _endPos)
        {
            return erase_internal(_startPos, _endPos, make_index_sequence<members_count>{});
        }

        template<MembersDesc I>
        auto& at(size_t _index)
        {
            return get<static_cast<size_t>(I)>(m_soa).at(_index);
        }

        template<MembersDesc I>
        const auto& at(size_t _index) const
        {
            return get<static_cast<size_t>(I)>(m_soa).at(_index);
        }

        value_list value_at(size_t _index) const
        {
            return at_internal<value_list>(_index, make_index_sequence<members_count>{});
        }

        reference_list ref_at(size_t _index)
        {
            return at_internal<reference_list>(_index, make_index_sequence<members_count>{});
        }

        const_reference_list ref_at(size_t _index) const
        {
            return at_internal<const_reference_list>(_index, make_index_sequence<members_count>{});
        }

        reference_list front()
        {
            return ref_at(0);
        }

        const_reference_list front() const
        {
            return ref_at(0);
        }

        reference_list back()
        {
            return ref_at(size() - 1);
        }

        const_reference_list back() const
        {
            return ref_at(size() - 1);
        }

        void sort()
        {
        }

    private:
        template<size_t... I>
        iterator begin_internal(index_sequence<I...>)
        {
            return { get<static_cast<size_t>(I)>(m_soa).data()... };
        }

        template<size_t... I>
        const_iterator begin_internal(index_sequence<I...>) const
        {
            return { get<static_cast<size_t>(I)>(m_soa).data()... };
        }

        template<size_t... I>
        iterator end_internal(index_sequence<I...>)
        {
            const size_t size{ this->size() };
            return { get<static_cast<size_t>(I)>(m_soa).data() + size... };
        }

        template<size_t... I>
        const_iterator end_internal(index_sequence<I...>) const
        {
            const size_t size{ this->size() };
            return { get<static_cast<size_t>(I)>(m_soa).data() + size... };
        }

        template<typename Tuple, size_t... I>
        void push_back_internal(Tuple&& _args, index_sequence<I...>)
        {
            (get<I>(m_soa).push_back(get<I>(std::forward<Tuple>(_args))), ...);
        }

        template<typename Tuple, size_t... I>
        void resize_internal(size_t _size, Tuple&& _args, index_sequence<I...>)
        {
            (get<I>(m_soa).resize(_size, get<I>(std::forward<Tuple>(_args))), ...);
        }

        template<typename Tuple, size_t... I>
        void insert_internal(size_t _pos, Tuple&& _args, index_sequence<I...>)
        {
            (get<I>(m_soa).insert(get<I>(m_soa).begin() + static_cast<ptrdiff_t>(_pos), get<I>(std::forward<Tuple>(_args))), ...);
        }

        template<size_t... I>
        size_t erase_internal(size_t _startPos, size_t _endPos, index_sequence<I...>)
        {
            (get<I>(m_soa).erase(get<I>(m_soa).begin() + static_cast<ptrdiff_t>(_startPos), get<I>(m_soa).begin() + static_cast<ptrdiff_t>(_endPos)), ...);
            return _startPos;
        }

        template<typename ReturnType, size_t... I>
        ReturnType at_internal(size_t _index, index_sequence<I...>)
        {
            return forward_as_tuple(get<I>(m_soa).at(_index) ...);
        }

        template<typename ReturnType, size_t... I>
        ReturnType at_internal(size_t _index, index_sequence<I...>) const
        {
            return forward_as_tuple(get<I>(m_soa).at(_index) ...);
        }

        template<typename T>
        class allocator_wrapper
        {
            Allocator m_allocator;

        public:
            using value_type = T;
            using pointer = T*;

            allocator_wrapper(Allocator&& _allocator)
                : m_allocator{ std::move(_allocator) }
            {
            }

            allocator_wrapper(const allocator_wrapper&) = default;

            template<typename U>
            friend class allocator_wrapper;

            template<typename U>
            allocator_wrapper(const allocator_wrapper<U>& _other)
                : m_allocator{ _other.m_allocator }
            {
            }

            allocator_wrapper& operator=(const allocator_wrapper&) = default;

            bool operator==(const allocator_wrapper& _other)
            {
                return this == &_other;
            }

            bool operator!=(const allocator_wrapper& _other)
            {
                return this != &_other;
            }

            [[nodiscard]] pointer allocate(size_t _count)
            {
                return m_allocator.template allocate<T>(_count);
            }

            void deallocate(pointer _ptr, size_t /*_count*/)
            {
                m_allocator.template free<T>(_ptr);
            }
        };

        tuple<container<Types, allocator_wrapper<Types>>...> m_soa{ container<Types, allocator_wrapper<Types>>{ Allocator{} }... };
    };

    struct std_allocator
    {
        template<typename T>
        static T* allocate(size_t _count)
        {
            constexpr size_t cache_line_size{ 64 };
            constexpr size_t alignment = alignof(T) > cache_line_size ? alignof(T) : cache_line_size;

#ifdef _WIN32
            return static_cast<T*>(_aligned_malloc(_count * sizeof(T), alignment));
#else
            return static_cast<T*>(aligned_alloc(alignment, _count * sizeof(T)));
#endif
        }

        template<typename T>
        static void free(T* _ptr)
        {
#ifdef _WIN32
            _aligned_free(_ptr);
#else
            free(_ptr);
#endif
    }
};

    template<typename MembersDesc, typename... Types>
    using vector = soa::vector_base<MembersDesc, soa::std_allocator, Types...>;
}
