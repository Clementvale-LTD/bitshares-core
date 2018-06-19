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
#include <graphene/chain/database.hpp>
#include <graphene/chain/worker_evaluator.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/worker_object.hpp>

#include <graphene/chain/protocol/vote.hpp>

namespace graphene { namespace chain {

void_result service_create_evaluator::do_evaluate(const service_create_evaluator::operation_type& o)
{ try {
   database& d = db();

   const auto& chain_parameters = d.get_global_properties().parameters;
   FC_ASSERT( o.p_memo.gto.size() < chain_parameters.maximum_asset_whitelist_authorities );
   
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

object_id_type service_create_evaluator::do_apply(const service_create_evaluator::operation_type& o)
{ try {
   database& d = db();

   auto next_service_id = db().get_index_type<service_index>().get_next_id();

   const service_object& new_service =
    d.create<service_object>([&](service_object& w) {
      w.owner = o.owner;
      w.name = o.name;
      w.p_memo = o.p_memo;
   });

   assert( new_service.id == next_service_id );

} FC_CAPTURE_AND_RETHROW( (o) ) }  

} } // graphene::chain
