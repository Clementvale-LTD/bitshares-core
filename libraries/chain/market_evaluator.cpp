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
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/market_object.hpp>

#include <graphene/chain/market_evaluator.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

#include <graphene/chain/protocol/market.hpp>
#include <graphene/chain/worker_object.hpp>

#include <fc/uint128.hpp>
#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {
void_result limit_order_create_evaluator::do_evaluate(const limit_order_create_operation& op)
{ try {
   const database& d = db();

   FC_ASSERT( op.expiration >= d.head_block_time() );

   _seller        = this->fee_paying_account;
   _sell_asset    = &op.amount_to_sell.asset_id(d);
   _receive_asset = &op.min_to_receive.asset_id(d);

   if( _sell_asset->options.whitelist_markets.size() )
      FC_ASSERT( _sell_asset->options.whitelist_markets.find(_receive_asset->id) != _sell_asset->options.whitelist_markets.end() );
   if( _sell_asset->options.blacklist_markets.size() )
      FC_ASSERT( _sell_asset->options.blacklist_markets.find(_receive_asset->id) == _sell_asset->options.blacklist_markets.end() );

   if( op.bid_id.valid() ){
     const bid_object&  bo = (*(op.bid_id))(d);
     FC_ASSERT( bo.expiration >= d.head_block_time() );
   }

   FC_ASSERT( is_authorized_asset( d, *_seller, *_sell_asset ) );
   FC_ASSERT( is_authorized_asset( d, *_seller, *_receive_asset ) );

   FC_ASSERT( d.get_balance( *_seller, *_sell_asset ) >= op.amount_to_sell, "insufficient balance",
              ("balance",d.get_balance(*_seller,*_sell_asset))("amount_to_sell",op.amount_to_sell) );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void limit_order_create_evaluator::pay_fee()
{
   _deferred_fee = core_fee_paid;
}

object_id_type limit_order_create_evaluator::do_apply(const limit_order_create_operation& op)
{ try {
   const auto& seller_stats = _seller->statistics(db());
   db().modify(seller_stats, [&](account_statistics_object& bal) {
         if( op.amount_to_sell.asset_id == asset_id_type() )
         {
            bal.total_core_in_orders += op.amount_to_sell.amount;
         }
   });

   db().adjust_balance(op.seller, -op.amount_to_sell);

   asset umt_fee( 0, GRAPHENE_SDR_ASSET_ID );

   if( op.amount_to_sell.asset_id == GRAPHENE_SDR_ASSET_ID) //SDR
    if( op.min_to_receive.asset_id != asset_id_type(0)) //not BTE
    {
      umt_fee = db().sdr_amount_to_umt_fee_to_reserve( op.amount_to_sell.amount );
      db().adjust_balance(op.seller, -umt_fee);
    }

   const auto& new_order_object = db().create<limit_order_object>([&](limit_order_object& obj){
       obj.seller   = _seller->id;
       obj.for_sale = op.amount_to_sell.amount;
       obj.sell_price = op.get_price();
       obj.expiration = op.expiration;

       obj.umt_fee = umt_fee;

       obj.request_id =  op.request_id;
       obj.user_id = op.user_id;
       obj.counterparty_id = op.counterparty_id;
       obj.p_memo = op.p_memo;
       obj.bid_id = op.bid_id; 

       obj.deferred_fee = _deferred_fee;
   });
   limit_order_id_type order_id = new_order_object.id; // save this because we may remove the object by filling it
   bool filled = db().apply_order(new_order_object);

   FC_ASSERT( !op.fill_or_kill || filled );

   return order_id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result limit_order_accept_evaluator::do_evaluate(const limit_order_accept_operation& o)
{ try {
   database& d = db();

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result limit_order_accept_evaluator::do_apply(const limit_order_accept_operation& o)
{ try {
   database& d = db();

   const auto& limit_price_idx = d.get_index_type<limit_order_index>().indices().get<by_price>();

   auto limit_itr = limit_price_idx.lower_bound(price::max(o.asset_id_to_receive, o.asset_id_to_sell));
   auto limit_end = limit_price_idx.upper_bound(price::min(o.asset_id_to_receive, o.asset_id_to_sell));
   
   while( limit_itr != limit_end )
   {
      const limit_order_object& order = *limit_itr;
      ++limit_itr;
      // match returns 2 when only the old order was fully filled. In this case, we keep matching; otherwise, we stop.
      if( d.is_match_possible( order, o.request_id, o.user_id, o.seller, o.counterparty_id) ){
        limit_order_accepted_operation op_accepted;
        op_accepted.order_id = order.id;
        op_accepted.order_creator_account_id = o.counterparty_id;
        op_accepted.asset_id_to_sell = o.asset_id_to_sell;
        op_accepted.asset_id_to_receive = o.asset_id_to_receive;

        op_accepted.request_id = o.request_id;
        op_accepted.user_id = o.user_id;

        op_accepted.accepted_by_account_id = o.seller;
        op_accepted.p_accepted_memo = o.p_memo;

        d.push_applied_operation( op_accepted);

        d.modify( order, [&]( limit_order_object& b ) {
                                b.p_accepted_memo = o.p_memo;
                              });
        break;
      }
   }   

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result limit_order_cancel_evaluator::do_evaluate(const limit_order_cancel_operation& o)
{ try {
   database& d = db();

   _order = &o.order(d);
   FC_ASSERT( _order->seller == o.fee_paying_account );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

asset limit_order_cancel_evaluator::do_apply(const limit_order_cancel_operation& o)
{ try {
   database& d = db();

   auto refunded = _order->amount_for_sale();

   d.cancel_order(*_order, false /* don't create a virtual op*/);

   return refunded;
} FC_CAPTURE_AND_RETHROW( (o) ) }

} } // graphene::chain
