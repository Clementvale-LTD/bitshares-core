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

   return new_service.id;

} FC_CAPTURE_AND_RETHROW( (o) ) }  

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void_result service_update_evaluator::do_evaluate(const service_update_evaluator::operation_type& o)
{ try {
   database& d = db();

   const service_object& so = o.service_to_update(d);
   
   service_to_update = &so;

   FC_ASSERT( o.owner == so.owner, "", ("o.owner", o.owner)("so.owner", so.owner) );
   
   const auto& chain_parameters = d.get_global_properties().parameters;
   FC_ASSERT( o.p_memo.gto.size() < chain_parameters.maximum_asset_whitelist_authorities );
   
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

object_id_type service_update_evaluator::do_apply(const service_update_evaluator::operation_type& o)
{ try {

   database& d = db();

   d.modify(*service_to_update, [&](service_object& so) {
      so.p_memo = o.p_memo;
   });

   return service_to_update->id;

} FC_CAPTURE_AND_RETHROW( (o) ) }  

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void_result bid_request_create_evaluator::do_evaluate(const bid_request_create_evaluator::operation_type& o)
{ try {
   database& d = db();

   FC_ASSERT( o.expiration >= d.head_block_time() );

   const auto& chain_parameters = d.get_global_properties().parameters;
   FC_ASSERT( o.p_memo.gto.size() < chain_parameters.maximum_asset_whitelist_authorities );

   for( auto a : o.assets ){
     const asset_object& ao = a(d);
     auto acc_itr = o.providers.find( ao.issuer );
     FC_ASSERT( acc_itr != o.providers.end() );     
   }
   
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

object_id_type bid_request_create_evaluator::do_apply(const bid_request_create_evaluator::operation_type& o)
{ try {
   database& d = db();

   auto next_bid_request_id = db().get_index_type<bid_request_index>().get_next_id();

   const bid_request_object& new_bid_request =
    d.create<bid_request_object>([&](bid_request_object& w) {
      w.owner = o.owner;
      w.name = o.name;
      w.assets = o.assets;
      w.providers = o.providers;
      w.p_memo = o.p_memo;
      w.expiration = o.expiration;
   });

   assert( new_bid_request.id == next_bid_request_id );

   return new_bid_request.id;

} FC_CAPTURE_AND_RETHROW( (o) ) }  

void_result bid_request_cancel_evaluator::do_evaluate(const bid_request_cancel_evaluator::operation_type& o)
{ try {
   database& d = db();

   _refobj = &o.bid_request_id(d);
   FC_ASSERT( _refobj->owner == o.fee_paying_account );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result bid_request_cancel_evaluator::do_apply(const bid_request_cancel_evaluator::operation_type& o)
{ try {
   database& d = db();

   d.remove( *_refobj);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void_result bid_create_evaluator::do_evaluate(const bid_create_evaluator::operation_type& o)
{ try {
   database& d = db();

   FC_ASSERT( o.expiration >= d.head_block_time() );

   const bid_request_object& bro = o.request(d);
   FC_ASSERT( bro.expiration >= d.head_block_time() );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

object_id_type bid_create_evaluator::do_apply(const bid_create_evaluator::operation_type& o)
{ try {
   database& d = db();

   auto next_bid_id = db().get_index_type<bid_index>().get_next_id();

   const bid_object& new_bid =
    d.create<bid_object>([&](bid_object& w) {
      w.owner = o.owner;
      w.name = o.name;
      w.request = o.request;
      w.p_memo = o.p_memo;
      w.expiration = o.expiration;
   });

   assert( new_bid.id == next_bid_id );

   return new_bid.id;

} FC_CAPTURE_AND_RETHROW( (o) ) }  

void_result bid_cancel_evaluator::do_evaluate(const bid_cancel_evaluator::operation_type& o)
{ try {
   database& d = db();

   _refobj = &o.bid_id(d);
   FC_ASSERT( _refobj->owner == o.fee_paying_account );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result bid_cancel_evaluator::do_apply(const bid_cancel_evaluator::operation_type& o)
{ try {
   database& d = db();

   d.remove( *_refobj);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }


} } // graphene::chain
