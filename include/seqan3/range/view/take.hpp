// -----------------------------------------------------------------------------------------------------
// Copyright (c) 2006-2019, Knut Reinert & Freie Universität Berlin
// Copyright (c) 2016-2019, Knut Reinert & MPI für molekulare Genetik
// This file may be used, modified and/or redistributed under the terms of the 3-clause BSD-License
// shipped with this file and also available at: https://github.com/seqan/seqan3/blob/master/LICENSE.md
// -----------------------------------------------------------------------------------------------------

/*!\file
 * \author Hannes Hauswedell <hannes.hauswedell AT fu-berlin.de>
 * \brief Provides seqan3::view::take.
 */

#pragma once

#include <range/v3/algorithm/copy.hpp>

#include <seqan3/core/type_traits/iterator.hpp>
#include <seqan3/core/type_traits/range.hpp>
#include <seqan3/core/type_traits/template_inspection.hpp>
#include <seqan3/core/type_traits/transformation_trait_or.hpp>
#include <seqan3/io/exception.hpp>
#include <seqan3/range/concept.hpp>
#include <seqan3/range/shortcuts.hpp>
#include <seqan3/range/container/concept.hpp>
#include <seqan3/range/view/detail.hpp>
#include <seqan3/range/detail/inherited_iterator_base.hpp>
#include <seqan3/std/algorithm>
#include <seqan3/std/concepts>
#include <seqan3/std/iterator>
#include <seqan3/std/ranges>
#include <seqan3/std/span>
#include <seqan3/std/type_traits>

namespace seqan3::detail
{

// ============================================================================
//  view_take
// ============================================================================

/*!\brief The type returned by seqan3::view::take and seqan3::view::take_or_throw.
 * \tparam urng_t    The type of the underlying ranges, must satisfy seqan3::view::concept.
 * \tparam exactly   Whether to expose sized'ness on the view.
 * \tparam or_throw  Whether to throw an exception when the input is exhausted before the end of line is reached.
 * \implements std::ranges::View
 * \implements std::ranges::RandomAccessRange
 * \implements std::ranges::SizedRange
 * \ingroup view
 *
 * \details
 *
 * Note that most members of this class are generated by ranges::view_interface which is not yet documented here.
 */
template <std::ranges::View urng_t, bool exactly, bool or_throw>
class view_take : public ranges::view_interface<view_take<urng_t, exactly, or_throw>>
{
private:
    //!\brief The underlying range.
    urng_t urange;

    //!\brief The desired target_size.
    size_t target_size;

    //!\brief The iterator type inherits from the underlying type, but overwrites several operators.
    //!\tparam rng_t Should be `urng_t` for defining #iterator and `urng_t const` for defining #const_iterator.
    template <typename rng_t>
    class iterator_type : public inherited_iterator_base<iterator_type<rng_t>, std::ranges::iterator_t<rng_t>>
    {
    private:
        //!\brief The iterator type of the underlying range.
        using base_base_t = std::ranges::iterator_t<rng_t>;
        //!\brief The CRTP wrapper type.
        using base_t      = inherited_iterator_base<iterator_type, std::ranges::iterator_t<rng_t>>;

        //!\brief The sentinel type is identical to that of the underlying range.
        using sentinel_type = std::ranges::sentinel_t<urng_t>;

        //!\brief The current position.
        size_t pos{};

        //!\brief The size parameter to the view.
        size_t max_pos{};

        //!\brief A pointer to host, s.t. the size of the view can shrink on pure input ranges.
        std::conditional_t<exactly && !std::ForwardIterator<base_base_t>, view_take *, detail::ignore_t> host_ptr;

    public:
        /*!\name Constructors, destructor and assignment
         * \brief Exceptions specification is implicitly inherited.
         * \{
         */
        constexpr iterator_type()                                      = default; //!< Defaulted.
        constexpr iterator_type(iterator_type const & rhs)             = default; //!< Defaulted.
        constexpr iterator_type(iterator_type && rhs)                  = default; //!< Defaulted.
        constexpr iterator_type & operator=(iterator_type const & rhs) = default; //!< Defaulted.
        constexpr iterator_type & operator=(iterator_type && rhs)      = default; //!< Defaulted.
        ~iterator_type()                                               = default; //!< Defaulted.

        //!\brief Constructor that delegates to the CRTP layer.
        constexpr iterator_type(base_base_t const & it) noexcept(noexcept(base_t{it})) :
            base_t{std::move(it)}
        {}

        //!\brief Constructor that delegates to the CRTP layer and initialises the members.
        constexpr iterator_type(base_base_t it, size_t const _pos, size_t const _max_pos, view_take * host = nullptr) noexcept(noexcept(base_t{it})) :
            base_t{std::move(it)}, pos{_pos}, max_pos(_max_pos)
        {
            host_ptr = host;
        }
        //!\}

        /*!\name Associated types
         * \brief All are derived from the base_base_t.
         * \{
         */

        //!\brief The difference type.
        using difference_type       = typename std::iterator_traits<base_base_t>::difference_type;
        //!\brief The value type.
        using value_type            = typename std::iterator_traits<base_base_t>::value_type;
        //!\brief The reference type.
        using reference             = typename std::iterator_traits<base_base_t>::reference;
        //!\brief The pointer type.
        using pointer               = typename std::iterator_traits<base_base_t>::pointer;
        //!\brief The iterator category tag.
        using iterator_category     = iterator_tag_t<base_base_t>;
        //!\}

        /*!\name Arithmetic operators
         * \brief seqan3::detail::inherited_iterator_base operators are used unless specialised here.
         * \{
         */

        //!\brief Increments the iterator by one.
        constexpr iterator_type & operator++() noexcept(noexcept(++std::declval<base_t &>()))
        {
            base_t::operator++();
            ++pos;
            if constexpr (exactly && !std::ForwardIterator<base_base_t>)
                --host_ptr->target_size;
            return *this;
        }

        //!\brief Returns an iterator incremented by one.
        constexpr iterator_type operator++(int) noexcept(noexcept(++std::declval<iterator_type &>()) &&
                                               std::is_nothrow_copy_constructible_v<iterator_type>)
        {
            iterator_type cpy{*this};
            ++(*this);
            return cpy;
        }

        //!\brief Decrements the iterator by one.
        constexpr iterator_type & operator--() noexcept(noexcept(--std::declval<base_base_t &>()))
        //!\cond
            requires std::BidirectionalIterator<base_base_t>
        //!\endcond
        {
            base_t::operator--();
            --pos;
            return *this;
        }

        //!\brief Returns an iterator decremented by one.
        constexpr iterator_type operator--(int) noexcept(noexcept(--std::declval<iterator_type &>()) &&
                                               std::is_nothrow_copy_constructible_v<iterator_type>)
        //!\cond
            requires std::BidirectionalIterator<base_base_t>
        //!\endcond
        {
            iterator_type cpy{*this};
            --(*this);
            return cpy;
        }

        //!\brief Advances the iterator by skip positions.
        constexpr iterator_type & operator+=(difference_type const skip) noexcept(noexcept(std::declval<base_t &>() += skip))
        //!\cond
            requires std::RandomAccessIterator<base_base_t>
        //!\endcond
        {
            base_t::operator+=(skip);
            pos += skip;
            return *this;
        }

        //!\brief Advances the iterator by -skip positions.
        constexpr iterator_type & operator-=(difference_type const skip) noexcept(noexcept(std::declval<base_t &>() -= skip))
        //!\cond
            requires std::RandomAccessIterator<base_base_t>
        //!\endcond
        {
            base_t::operator-=(skip);
            pos -= skip;
            return *this;
        }
        //!\}

        /*!\name Comparison operators
         * \brief We define comparison against self and against the sentinel.
         * \{
         */

        //!\brief Checks whether `*this` is equal to `rhs`.
        constexpr bool operator==(iterator_type const & rhs) const
            noexcept(!or_throw && noexcept(std::declval<base_base_t &>() == std::declval<base_base_t &>()))
        //!\cond
            requires std::ForwardIterator<base_base_t>
        //!\endcond
        {
            return *base_t::this_to_base() == *rhs.this_to_base();
        }

        //!\copydoc operator==()
        constexpr bool operator==(sentinel_type const & rhs) const
            noexcept(!or_throw && noexcept(std::declval<base_base_t const &>() == std::declval<sentinel_type const &>()))
        {
            if (pos >= max_pos)
                return true;

            if (*base_t::this_to_base() == rhs)
            {
                if constexpr (or_throw)
                    throw unexpected_end_of_input{"Reached end of input before designated size."};

                return true;
            }
            else
            {
                return false;
            }
        }

        //!\brief Checks whether `lhs` is equal to `rhs`.
        constexpr friend bool operator==(sentinel_type const & lhs, iterator_type const & rhs) noexcept(noexcept(rhs == lhs))
        {
            return rhs == lhs;
        }

        //!\brief Checks whether `*this` is not equal to `rhs`.
        constexpr bool operator!=(sentinel_type const & rhs) const
            noexcept(noexcept(std::declval<iterator_type &>() == rhs))
        {
            return !(*this == rhs);
        }

        //!\copydoc operator!=()
        constexpr bool operator!=(iterator_type const & rhs) const
            noexcept(noexcept(std::declval<iterator_type &>() == rhs))
        //!\cond
            requires std::ForwardIterator<base_base_t>
        //!\endcond
        {
            return !(*this == rhs);
        }

        //!\brief Checks whether `lhs` is not equal to `rhs`.
        constexpr friend bool operator!=(sentinel_type const & lhs, iterator_type const & rhs) noexcept(noexcept(rhs != lhs))
        {
            return rhs != lhs;
        }
        //!\}

        /*!\name Reference/Dereference operators
         * \brief seqan3::detail::inherited_iterator_base operators are used unless specialised here.
         * \{
         */

        /*!\brief Accesses an element by index.
         * \param n Position relative to current location.
         * \return A reference to the element at relative location.
         */
        constexpr reference operator[](std::make_unsigned_t<difference_type> const n) const
            noexcept(noexcept(std::declval<base_base_t &>()[0]))
        //!\cond
            requires std::RandomAccessIterator<base_base_t>
        //!\endcond
        {
            return base_base_t::operator[](n);
        }
        //!\}
    }; // class iterator_type

public:
    /*!\name Associated types
     * \{
     */
    //!\brief The reference_type.
    using reference         = reference_t<urng_t>;
    //!\brief The const_reference type is equal to the reference type if the underlying range is const-iterable.
    using const_reference   = detail::transformation_trait_or_t<seqan3::reference<urng_t const>, void>;
    //!\brief The value_type (which equals the reference_type with any references removed).
    using value_type        = value_type_t<urng_t>;
    //!\brief The size_type is `size_t` if the view is exact, otherwise `void`.
    using size_type         = std::conditional_t<exactly || std::ranges::SizedRange<urng_t>,
                                                 transformation_trait_or_t<seqan3::size_type<urng_t>, size_t>,
                                                 void>;
    //!\brief A signed integer type, usually std::ptrdiff_t.
    using difference_type   = difference_type_t<urng_t>;
    //!\brief The iterator type of this view (a random access iterator).
    using iterator          = iterator_type<urng_t>;
    //!\brief The const_iterator type is equal to the iterator type if the underlying range is const-iterable.
    using const_iterator    = detail::transformation_trait_or_t<std::type_identity<iterator_type<urng_t const>>, void>;
    //!\}

    /*!\name Constructors, destructor and assignment
     * \{
     */
    constexpr view_take()                                  = default; //!< Defaulted.
    constexpr view_take(view_take const & rhs)             = default; //!< Defaulted.
    constexpr view_take(view_take && rhs)                  = default; //!< Defaulted.
    constexpr view_take & operator=(view_take const & rhs) = default; //!< Defaulted.
    constexpr view_take & operator=(view_take && rhs)      = default; //!< Defaulted.
    ~view_take()                                           = default; //!< Defaulted.

    /*!\brief Construct from another View.
     * \param[in] _urange The underlying range.
     * \param[in] _size   The desired size (after which to stop returning elements).
     * \throws unexpected_end_of_input If `exactly && or_throw && seqan3::SizedRange<urng_t>`.
     */
    constexpr view_take(urng_t _urange, size_t const _size)
        : urange{std::move(_urange)}, target_size{_size}
    {
        if constexpr (exactly && or_throw && std::ranges::SizedRange<urng_t>)
        {
            if (seqan3::size(_urange) < _size)
            {
                throw std::invalid_argument{
                    "You are trying to construct a view::take_exactly_or_throw from a range that is strictly smaller."};
            }
        }
    }

    /*!\brief Construct from another ViewableRange.
     * \tparam rng_t      Type of the passed range; `urng_t` must be constructible from this.
     * \param[in] _urange The underlying range.
     * \param[in] _size   The desired size (after which to stop returning elements).
     * \throws unexpected_end_of_input If `exactly && or_throw && seqan3::SizedRange<urng_t>`.
     */
    template<std::ranges::ViewableRange rng_t>
    //!\cond
        requires std::Constructible<rng_t, std::ranges::all_view<rng_t>>
    //!\endcond
    constexpr view_take(rng_t && _urange, size_t const _size)
        : view_take{std::view::all(std::forward<rng_t>(_urange)), _size}
    {}
    //!\}

    /*!\name Iterators
     * \{
     */
    /*!\brief Returns an iterator to the first element of the container.
     * \returns Iterator to the first element.
     *
     * If the container is empty, the returned iterator will be equal to end().
     *
     * ### Complexity
     *
     * Constant.
     *
     * ### Exceptions
     *
     * No-throw guarantee.
     */
    constexpr iterator begin() noexcept
    {
        return {seqan3::begin(urange), 0, target_size, &(*this)};
    }

    //!\copydoc begin()
    constexpr const_iterator begin() const noexcept
        requires ConstIterableRange<urng_t>
    {
        return {seqan3::cbegin(urange), 0, target_size};
    }

    //!\copydoc begin()
    constexpr const_iterator cbegin() const noexcept
        requires ConstIterableRange<urng_t>
    {
        return {seqan3::cbegin(urange), 0, target_size};
    }

    /*!\brief Returns an iterator to the element following the last element of the range.
     * \returns Iterator to the end.
     *
     * This element acts as a placeholder; attempting to dereference it results in undefined behaviour.
     *
     * ### Complexity
     *
     * Constant.
     *
     * ### Exceptions
     *
     * No-throw guarantee.
     */
    constexpr auto end() noexcept
    {
        return seqan3::end(urange);
    }

    //!\copydoc end()
    constexpr auto end() const noexcept
        requires ConstIterableRange<urng_t>
    {
        return seqan3::cend(urange);
    }

    //!\copydoc end()
    constexpr auto cend() const noexcept
        requires ConstIterableRange<urng_t>
    {
        return seqan3::cend(urange);
    }
    //!\}

    /*!\brief Returns the number of elements in the view.
     * \returns The number of elements in the view.
     *
     * This overload is only available if the underlying range models std::ranges::SizedRange or for
     * specialisation that have the `exactly` template parameter set.
     *
     * ### Complexity
     *
     * Constant.
     *
     * ### Exceptions
     *
     * No-throw guarantee.
     */
    constexpr size_type size() const noexcept
        requires exactly
    {
        return target_size;
    }

    //!\overload
    constexpr size_type size() noexcept
        requires (!exactly) && std::ranges::SizedRange<urng_t>
    {
        return std::min<size_type>(target_size, std::ranges::size(urange));
    }

    //!\overload
    constexpr size_type size() const noexcept
        requires (!exactly) && std::ranges::SizedRange<urng_t const>
    {
        return std::min<size_type>(target_size, std::ranges::size(urange));
    }
};

//!\brief Template argument type deduction guide that strips references.
//!\relates seqan3::detail::view_take
template <typename urng_t,
          bool exactly = false,
          bool or_throw = false>
view_take(urng_t && , size_t) -> view_take<std::ranges::all_view<urng_t>, exactly, or_throw>;

// ============================================================================
//  take_fn (adaptor definition)
// ============================================================================

/*!\brief View adaptor definition for view::take and view::take_or_throw.
 * \tparam or_throw Whether to throw an exception when the input is exhausted before the end is reached.
 */
template <bool exactly, bool or_throw>
struct take_fn
{
    //!\brief Store the arguments and return a range adaptor closure object.
    constexpr auto operator()(size_t const size) const
    {
        return adaptor_from_functor{*this, size};
    }

    /*!\brief Type erase if possible and return view_take if not.
     * \returns An instance of std::span, std::basic_string_view, std::ranges::subrange or seqan3::detail::view_take.
     */
    template <std::ranges::Range urng_t>
    constexpr auto operator()(urng_t && urange, size_t target_size) const
    {
        static_assert(std::ranges::ViewableRange<urng_t>,
                      "The view::take adaptor can only be passed ViewableRanges, i.e. Views or &-to-non-View.");

        // safeguard against wrong size
        if constexpr (std::ranges::SizedRange<urng_t>)
        {
            if constexpr (or_throw)
            {
                if (target_size > std::ranges::size(urange))
                {
                    throw std::invalid_argument{"You are trying to construct a view::take_exactly_or_throw from a "
                                                "range that is strictly smaller."};
                }
            }
            else
            {
                target_size = std::min(target_size, std::ranges::size(urange));
            }
        }

        // string_view
        if constexpr (is_type_specialisation_of_v<remove_cvref_t<urng_t>, std::basic_string_view>)
        {
            return urange.substr(0, target_size);
        }
        // string const &
        else if constexpr (is_type_specialisation_of_v<remove_cvref_t<urng_t>, std::basic_string> &&
                           std::is_const_v<std::remove_reference_t<urng_t>>)
        {
            return std::basic_string_view{std::ranges::data(urange), target_size};
        }
        // contiguous
        else if constexpr (ForwardingRange<urng_t> &&
                           std::ranges::ContiguousRange<urng_t> &&
                           std::ranges::SizedRange<urng_t>)
        {
            return std::span{std::ranges::data(urange), target_size};
        }
        // random_access
        else if constexpr (ForwardingRange<urng_t> &&
                           std::ranges::RandomAccessRange<urng_t> &&
                           std::ranges::SizedRange<urng_t>)
        {
            return std::ranges::subrange<std::ranges::iterator_t<urng_t>, std::ranges::iterator_t<urng_t>>
            {
                std::ranges::begin(urange),
                std::ranges::begin(urange) + target_size,
                target_size
            };
        }
        // our type
        else
        {
            return view_take<std::ranges::all_view<urng_t>, exactly, or_throw>
            {
                std::forward<urng_t>(urange),
                target_size
            };
        }
    }
};

} // namespace seqan3::detail

// ============================================================================
//  view::take (adaptor instance definition)
// ============================================================================

namespace seqan3::view
{

/*!\name General purpose views
 * \{
 */

/*!\brief               A view adaptor that returns the first `size` elements from the underlying range (or less if the
 *                      underlying range is shorter).
 * \tparam urng_t       The type of the range being processed. See below for requirements. [template parameter is
 *                      omitted in pipe notation]
 * \param[in] urange    The range being processed. [parameter is omitted in pipe notation]
 * \param[in] size      The target size of the view.
 * \returns             Up to `size` elements of the underlying range.
 * \ingroup view
 *
 * \details
 *
 * **Header**
 * ```cpp
 *      #include <seqan3/range/view/take.hpp>
 * ```
 *
 * ### View properties
 *
 * | range concepts and reference_t  | `urng_t` (underlying range type)   | `rrng_t` (returned range type)   |
 * |---------------------------------|:----------------------------------:|:--------------------------------:|
 * | std::ranges::InputRange         | *required*                         | *preserved*                      |
 * | std::ranges::ForwardRange       |                                    | *preserved*                      |
 * | std::ranges::BidirectionalRange |                                    | *preserved*                      |
 * | std::ranges::RandomAccessRange  |                                    | *preserved*                      |
 * | std::ranges::ContiguousRange    |                                    | *preserved*                      |
 * |                                 |                                    |                                  |
 * | std::ranges::ViewableRange      | *required*                         | *guaranteed*                     |
 * | std::ranges::View               |                                    | *guaranteed*                     |
 * | std::ranges::SizedRange         |                                    | *preserved*                      |
 * | std::ranges::CommonRange        |                                    | *preserved*                      |
 * | std::ranges::OutputRange        |                                    | *preserved*                      |
 * | seqan3::ConstIterableRange      |                                    | *preserved*                      |
 * |                                 |                                    |                                  |
 * | seqan3::reference_t             |                                    | seqan3::reference_t<urng_t>      |
 *
 * See the \link view view submodule documentation \endlink for detailed descriptions of the view properties.
 *
 * ### Return type
 *
 * | `urng_t` (underlying range type)                                                       | `rrng_t` (returned range type)  |
 * |:--------------------------------------------------------------------------------------:|:-------------------------------:|
 * | `std::basic_string const &` *or* `std::basic_string_view`                              | `std::basic_string_view`        |
 * | `seqan3::ForwardingRange && std::ranges::SizedRange && std::ranges::ContiguousRange`   | `std::span`                     |
 * | `seqan3::ForwardingRange && std::ranges::SizedRange && std::ranges::RandomAccessRange` | `std::ranges::subrange`         |
 * | *else*                                                                                 | *implementation defined type*   |
 *
 * This adaptor is different from std::view::take in that it performs type erasure for some underlying ranges.
 * It returns exactly the type specified above.
 *
 * Some benchmarks have shown that it is also faster than std::view::take for pure forward and input ranges.
 *
 * ### Example
 *
 * \snippet test/snippet/range/view/take.cpp usage
 *
 * \hideinitializer
 */
inline auto constexpr take = detail::take_fn<false, false>{};

//!\}

} // namespace seqan3::view
