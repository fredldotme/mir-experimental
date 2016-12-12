/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */
#ifndef MIR_TEST_INPUT_CONFIG_MATCHERS_H
#define MIR_TEST_INPUT_CONFIG_MATCHERS_H

#include "mir/input/input_configuration.h"
#include <gmock/gmock.h>

namespace testing
{
namespace internal
{

class InputConfigElementsMatcher
    : public MatcherInterface<mir::input::InputConfiguration const&>,
      public UnorderedElementsAreMatcherImplBase
{
public:
    /*typedef InputConfiguration RawContainer;
    //typedef internal::StlContainerView<RawContainer> View;
    typedef typename View::type StlContainer;
    typedef typename View::const_reference StlContainerReference;
    typedef typename StlContainer::const_iterator StlContainerConstIterator;
    typedef typename RawContainer::value_type Element;*/
    typedef mir::input::DeviceConfiguration Element;

    // Constructs the matcher from a sequence of element values or
    // element matchers.
    template <typename InputIter>
    InputConfigElementsMatcher(InputIter first, InputIter last)
    {
        for (; first != last; ++first)
        {
            matchers_.push_back(MatcherCast<const Element&>(*first));
            matcher_describers().push_back(matchers_.back().GetDescriber());
        }
    }

    // Describes what this matcher does.
    virtual void DescribeTo(::std::ostream* os) const
    {
        return UnorderedElementsAreMatcherImplBase::DescribeToImpl(os);
    }

    // Describes what the negation of this matcher does.
    virtual void DescribeNegationTo(::std::ostream* os) const
    {
        return UnorderedElementsAreMatcherImplBase::DescribeNegationToImpl(os);
    }

    virtual bool MatchAndExplain(mir::input::InputConfiguration const& container, MatchResultListener* listener) const
    {
        ::std::vector<string> element_printouts;
        MatchMatrix matrix = AnalyzeElements(container, &element_printouts, listener);

        const size_t actual_count = matrix.LhsSize();
        if (actual_count == 0 && matchers_.empty())
        {
            return true;
        }
        if (actual_count != matchers_.size())
        {
            // The element count doesn't match.  If the container is empty,
            // there's no need to explain anything as Google Mock already
            // prints the empty container. Otherwise we just need to show
            // how many elements there actually are.
            if (actual_count != 0 && listener->IsInterested())
            {
                *listener << "which has " << Elements(actual_count);
            }
            return false;
        }

        return VerifyAllElementsAndMatchersAreMatched(element_printouts, matrix, listener) &&
               FindPairing(matrix, listener);
    }

private:
    typedef ::std::vector<Matcher<const Element&>> MatcherVec;

    MatchMatrix AnalyzeElements(mir::input::InputConfiguration const& config,
                                ::std::vector<string>* element_printouts,
                                MatchResultListener* listener) const
    {
        element_printouts->clear();
        ::std::vector<char> did_match;
        size_t num_elements = config.size();
        config.for_each(
            [&](mir::input::DeviceConfiguration const& element)
            {
                if (listener->IsInterested())
                    element_printouts->push_back(PrintToString(element));
                for (size_t irhs = 0; irhs != matchers_.size(); ++irhs)
                    did_match.push_back(Matches(matchers_[irhs])(element));
            });

        MatchMatrix matrix(num_elements, matchers_.size());
        ::std::vector<char>::const_iterator did_match_iter = did_match.begin();
        for (size_t ilhs = 0; ilhs != num_elements; ++ilhs)
        {
            for (size_t irhs = 0; irhs != matchers_.size(); ++irhs)
            {
                matrix.SetEdge(ilhs, irhs, *did_match_iter++ != 0);
            }
        }
        return matrix;
    }

    MatcherVec matchers_;

    GTEST_DISALLOW_ASSIGN_(InputConfigElementsMatcher);
};

template <>
class UnorderedElementsAreMatcherImpl<mir::input::InputConfiguration const&>
    : public InputConfigElementsMatcher
{
public:
    using InputConfigElementsMatcher::InputConfigElementsMatcher;
    //template <typename InputIter>
    //InputConfigElementsMatcher(InputIter first, InputIter last)
};

template <>
class UnorderedElementsAreMatcherImpl<mir::input::InputConfiguration&>
    : public InputConfigElementsMatcher
{
};

template <>
class UnorderedElementsAreMatcherImpl<mir::input::InputConfiguration>
    : public InputConfigElementsMatcher
{
};

template <>
class UnorderedElementsAreMatcherImpl<mir::input::InputConfiguration const>
    : public InputConfigElementsMatcher
{
};

}
}

#endif
