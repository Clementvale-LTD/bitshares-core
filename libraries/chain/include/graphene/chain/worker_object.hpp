/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {

  class service_object : public abstract_object<service_object>
  {
  public:
    static const uint8_t space_id = protocol_ids;
    static const uint8_t type_id = service_object_type;

    /// ID of the account which owns this worker
    account_id_type owner;

    /// Human-readable name for the worker
    string name;

    memo_group p_memo;

    service_id_type get_id()const { return id; }
  };

  struct by_name;
  struct by_owner;

  typedef multi_index_container<
      service_object,
      indexed_by<
          ordered_unique<tag<by_id>, member<object, object_id_type, &object::id>>,
          ordered_unique<tag<by_name>, member<service_object, string, &service_object::name>>,
          ordered_non_unique<tag<by_owner>, member<service_object, account_id_type, &service_object::owner>>>>
      service_object_multi_index_type;
  typedef generic_index<service_object, service_object_multi_index_type> service_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::service_object, (graphene::db::object),
                    (owner)
                    (name)
                    (p_memo)
                  )

