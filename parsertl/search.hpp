// search.hpp
// Copyright (c) 2017 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PARSERTL_SEARCH_HPP
#define PARSERTL_SEARCH_HPP

#include "match_results.hpp"
#include "parse.hpp"
#include <set>
#include "token.hpp"

namespace parsertl
{
// Forward declaration:
namespace details
{
template<typename iterator>
void next(const state_machine &sm_, iterator &iter_, match_results &results_,
    iterator &last_eoi_);
}

// Equivalent of std::search().
template<typename iterator>
bool search(const state_machine &sm_, iterator &iter_, iterator &end_,
    match_results &results_, const std::set<std::size_t> &productions_)
{
    bool hit_ = false;
    iterator curr_ = iter_;
    iterator last_eoi_;

    end_ = iterator();

    while (curr_ != end_)
    {
        results_.reset(curr_->id, sm_);

        while (results_.entry.action != accept &&
            results_.entry.action != error)
        {
            if (results_.entry.action == reduce &&
                productions_.find(results_.entry.param) != productions_.end())
            {
                hit_ = true;
            }

            details::next(sm_, curr_, results_, last_eoi_);
        }

        if (results_.entry.action == accept)
        {
            end_ = curr_;
            hit_ |= productions_.empty();
            break;
        }
        else if (last_eoi_->id != 0)
        {
            end_ = last_eoi_;
            hit_ |= productions_.empty();
            break;
        }
        else
        {
            ++iter_;
            curr_ = iter_;
            hit_ = false;
        }
    }

    return hit_;
}

namespace details
{
template<typename iterator>
void next(const state_machine &sm_, iterator &iter_, match_results &results_,
    iterator &last_eoi_)
{
    switch (results_.entry.action)
    {
    case error:
        break;
    case shift:
    {
        const auto *ptr_ = &sm_._table[results_.entry.param * sm_._columns];

        results_.stack.push_back(results_.entry.param);

        if (results_.token_id != 0)
        {
            ++iter_;
        }

        results_.token_id = iter_->id;

        if (results_.token_id == iterator::value_type::npos())
        {
            results_.entry.action = error;
            results_.entry.param = unknown_token;
        }
        else
        {
            results_.entry = ptr_[results_.token_id];
        }

        if (ptr_->action != error)
        {
            match_results temp_ = results_;
            iterator i_;

            temp_.token_id = 0;
            temp_.entry = *ptr_;

            if (parse(sm_, i_, temp_))
            {
                last_eoi_ = iter_;
            }
        }

        break;
    }
    case reduce:
    {
        const std::size_t size_ =
            sm_._rules[results_.entry.param].second.size();
        token<iterator> token_;

        if (size_)
        {
            results_.stack.resize(results_.stack.size() - size_);
        }
        else
        {
            token_.first = token_.second = iter_->first;
        }

        results_.token_id = sm_._rules[results_.entry.param].first;
        results_.entry = sm_._table[results_.stack.back() * sm_._columns +
            results_.token_id];
        token_.id = results_.token_id;
        break;
    }
    case go_to:
        results_.stack.push_back(results_.entry.param);
        results_.token_id = iter_->id;
        results_.entry = sm_._table[results_.stack.back() * sm_._columns +
            results_.token_id];
        break;
    case accept:
    {
        const std::size_t size_ =
            sm_._rules[results_.entry.param].second.size();

        if (size_)
        {
            results_.stack.resize(results_.stack.size() - size_);
        }

        break;
    }
    }
}
}
}

#endif
