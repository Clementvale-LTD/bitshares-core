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

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/market_object.hpp>

#include <fc/uint128.hpp>

namespace graphene { namespace chain {

void database::cancel_order(const force_settlement_object& order, bool create_virtual_op)
{
   adjust_balance(order.owner, order.balance);

   if( create_virtual_op )
   {
      asset_settle_cancel_operation vop;
      vop.settlement = order.id;
      vop.account = order.owner;
      vop.amount = order.balance;
      push_applied_operation( vop );
   }
   remove(order);
}

void database::cancel_order( const limit_order_object& order, bool create_virtual_op  )
{
   auto refunded = order.amount_for_sale();
   auto refunded_umt = order.umt_fee;

   modify( order.seller(*this).statistics(*this),[&]( account_statistics_object& obj ){
      if( refunded.asset_id == asset_id_type() )
      {
         obj.total_core_in_orders -= refunded.amount;
      }
   });
   adjust_balance(order.seller, refunded);
   adjust_balance(order.seller, refunded_umt);
   adjust_balance(order.seller, order.deferred_fee);

   if( create_virtual_op )
   {
      limit_order_cancel_operation vop;
      vop.order = order.id;
      vop.fee_paying_account = order.seller;
      push_applied_operation( vop );
   }

   remove(order);
}

bool maybe_cull_small_order( database& db, const limit_order_object& order )
{
   /**
    *  There are times when the AMOUNT_FOR_SALE * SALE_PRICE == 0 which means that we
    *  have hit the limit where the seller is asking for nothing in return.  When this
    *  happens we must refund any balance back to the seller, it is too small to be
    *  sold at the sale price.
    *
    *  If the order is a taker order (as opposed to a maker order), so the price is
    *  set by the counterparty, this check is deferred until the order becomes unmatched
    *  (see #555) -- however, detecting this condition is the responsibility of the caller.
    */
   if( order.amount_to_receive().amount == 0 )
   {
      //ilog( "applied epsilon logic" );
      db.cancel_order(order);
      return true;
   }
   return false;
}


bool database::is_match_possible( const limit_order_object& bid, const limit_order_object& ask)
{
  if( bid.request_id.valid() || ask.request_id.valid() ){
    bool b_match_possible = false;
    if( bid.request_id.valid() && ask.request_id.valid() ){
      b_match_possible = ( *(bid.request_id) == *(ask.request_id) );
    }
    if( !b_match_possible)
      return false;
  }
  
  if( bid.user_id.valid() || ask.user_id.valid() ){
    bool b_match_possible = false;
    if( bid.user_id.valid() && ask.user_id.valid() ){
      b_match_possible = ( *(bid.user_id) == *(ask.user_id) );
    }
    if( !b_match_possible)
      return false;
  }

  if( bid.counterparty_id.valid()){
    const account_object& ask_seller = ask.seller(*this);
    bool b_match_possible = ( *(bid.counterparty_id) == ask_seller.get_id() );
    if( !b_match_possible)
      return false;
  }

  if( ask.counterparty_id.valid()){
    const account_object& bid_seller = bid.seller(*this);
    bool b_match_possible = ( *(ask.counterparty_id) == bid_seller.get_id() );
    if( !b_match_possible)
      return false;
  }

  return true;
}

bool database::is_match_possible( const limit_order_object& o, uint64_t request_id, uint64_t user_id, account_id_type accept_account_id, account_id_type request_account_id)
{
  if( !o.request_id.valid())
    return false;
  if( !o.user_id.valid())
    return false;
  if( !o.counterparty_id.valid())
    return false;

  {
    bool b_match_possible = ( *(o.request_id) == request_id );
    if( !b_match_possible)
      return false;
  }
  
  {
    bool b_match_possible = ( *(o.user_id) == user_id );
    if( !b_match_possible)
      return false;
  }

  {
    bool b_match_possible = ( *(o.counterparty_id) == accept_account_id );
    if( !b_match_possible)
      return false;
  }

  {
    const account_object& o_seller = o.seller(*this);
    bool b_match_possible = ( request_account_id == o_seller.get_id() );
    if( !b_match_possible)
      return false;
  }

  return true;
}

bool database::apply_order(const limit_order_object& new_order_object, bool allow_black_swan)
{
   auto order_id = new_order_object.id;

   const auto& limit_price_idx = get_index_type<limit_order_index>().indices().get<by_price>();

   // TODO: it should be possible to simply check the NEXT/PREV iterator after new_order_object to
   // determine whether or not this order has "changed the book" in a way that requires us to
   // check orders. For now I just lookup the lower bound and check for equality... this is log(n) vs
   // constant time check. Potential optimization.

   auto max_price = ~new_order_object.sell_price;
   auto limit_itr = limit_price_idx.lower_bound(max_price.max());
   auto limit_end = limit_price_idx.upper_bound(max_price);

   bool finished = false;
   while( !finished && limit_itr != limit_end )
   {
      auto old_limit_itr = limit_itr;
      ++limit_itr;
      // match returns 2 when only the old order was fully filled. In this case, we keep matching; otherwise, we stop.
      if( is_match_possible( new_order_object, *old_limit_itr) ){
        finished = (match(new_order_object, *old_limit_itr, old_limit_itr->sell_price) != 2);
      }
   }

   const limit_order_object* updated_order_object = find< limit_order_object >( order_id );
   if( updated_order_object == nullptr )
      return true;

   // before #555 we would have done maybe_cull_small_order() logic as a result of fill_order() being called by match() above
   // however after #555 we need to get rid of small orders -- #555 hardfork defers logic that was done too eagerly before, and
   // this is the point it's deferred to.
   return maybe_cull_small_order( *this, *updated_order_object );
}

/**
 *  Matches the two orders,
 *
 *  @return a bit field indicating which orders were filled (and thus removed)
 *
 *  0 - no orders were matched
 *  1 - bid was filled
 *  2 - ask was filled
 *  3 - both were filled
 */
template<typename OrderType>
int database::match( const limit_order_object& usd, const OrderType& core, const price& match_price )
{
   assert( usd.sell_price.quote.asset_id == core.sell_price.base.asset_id );
   assert( usd.sell_price.base.asset_id  == core.sell_price.quote.asset_id );
   assert( usd.for_sale > 0 && core.for_sale > 0 );

   auto usd_for_sale = usd.amount_for_sale();
   auto core_for_sale = core.amount_for_sale();

   asset usd_pays, usd_receives, core_pays, core_receives;

   if( usd_for_sale <= core_for_sale * match_price )
   {
      core_receives = usd_for_sale;
      usd_receives  = usd_for_sale * match_price;
   }
   else
   {
      //This line once read: assert( core_for_sale < usd_for_sale * match_price );
      //This assert is not always true -- see trade_amount_equals_zero in operation_tests.cpp
      //Although usd_for_sale is greater than core_for_sale * match_price, core_for_sale == usd_for_sale * match_price
      //Removing the assert seems to be safe -- apparently no asset is created or destroyed.
      usd_receives = core_for_sale;
      core_receives = core_for_sale * match_price;
   }

   core_pays = usd_receives;
   usd_pays  = core_receives;

   assert( usd_pays == usd.amount_for_sale() ||
           core_pays == core.amount_for_sale() );
   
   counterparty_info  usd_info;
   counterparty_info  core_info;

   usd_info.request_id = usd.request_id;
   usd_info.user_id = usd.user_id;
   usd_info.p_memo = usd.p_memo;

   core_info.request_id = core.request_id;
   core_info.user_id = core.user_id;
   core_info.p_memo = core.p_memo;

   int result = 0;
   result |= fill_order( usd, usd_pays, usd_receives, false, match_price, false, &core_info ); // although this function is a template,
                                                                                   // right now it only matches one limit order
                                                                                   // with another limit order,
                                                                                   // the first param is a new order, thus taker
   result |= fill_order( core, core_pays, core_receives, true, match_price, true, &usd_info ) << 1; // the second param is maker
   assert( result != 0 );
   return result;
}

int database::match( const limit_order_object& bid, const limit_order_object& ask, const price& match_price )
{
   return match<limit_order_object>( bid, ask, match_price );
}

asset database::sdr_amount_to_umt_fee_to_reserve( share_type sdr_amount )
{
  const auto& gpo = get_global_properties();
  const auto& chain_params = gpo.parameters;

  uint16_t umt_stakeholder_percent_fee = chain_params.umt_stakeholder_percent_fee;

  fc::uint128 r(sdr_amount.value);
  r *= umt_stakeholder_percent_fee;
  r /= GRAPHENE_100_PERCENT;

  asset umt_fee( share_type( r.to_uint64()), GRAPHENE_SDR_ASSET_ID );

  return umt_fee;
}

asset database::sdr_amount_to_umt_fee_to_pay( share_type sdr_amount_to_pay, share_type sdr_amount_in_order, asset sdr_amount_fee_reserved )
{
  FC_ASSERT( sdr_amount_to_pay <= sdr_amount_in_order );

  if( sdr_amount_to_pay == sdr_amount_in_order){
    return sdr_amount_fee_reserved; 
  }

  fc::uint128 r(sdr_amount_fee_reserved.amount.value);
  r *= sdr_amount_to_pay.value;
  r /= sdr_amount_in_order.value;

  asset umt_fee( share_type( r.to_uint64()), sdr_amount_fee_reserved.asset_id );

  return umt_fee;
}

bool database::fill_order( const limit_order_object& order, const asset& pays, const asset& receives, bool cull_if_small,
                           const price& fill_price, const bool is_maker, const counterparty_info* cparty_info )
{ try {

   FC_ASSERT( order.amount_for_sale().asset_id == pays.asset_id );
   FC_ASSERT( pays.asset_id != receives.asset_id );

   const account_object& seller = order.seller(*this);
   const asset_object& recv_asset = receives.asset_id(*this);

   asset umt_fee( 0, GRAPHENE_SDR_ASSET_ID );

   if( pays.asset_id == GRAPHENE_SDR_ASSET_ID) //SDR
    if( receives.asset_id != asset_id_type(0)) //not BTE
    {
      umt_fee = sdr_amount_to_umt_fee_to_pay( pays.amount, order.for_sale, order.umt_fee );
      adjust_balance(GRAPHENE_UMT_FEE_POOL_ACCOUNT, umt_fee);
    }

   auto issuer_fees = pay_market_fees( recv_asset, receives );
   pay_order( seller, receives - issuer_fees, pays );

   assert( pays.asset_id != receives.asset_id );
   push_applied_operation( fill_order_operation( order.id, order.seller, pays, receives, issuer_fees, fill_price, is_maker, cparty_info ));

   // conditional because cheap integer comparison may allow us to avoid two expensive modify() and object lookups
   if( order.deferred_fee > 0 )
   {
      modify( seller.statistics(*this), [&]( account_statistics_object& statistics )
      {
         statistics.pay_fee( order.deferred_fee, get_global_properties().parameters.cashback_vesting_threshold );
      } );
   }

   if( pays == order.amount_for_sale() )
   {
      remove( order );
      return true;
   }
   else
   {
      modify( order, [&]( limit_order_object& b ) {
                             b.for_sale -= pays.amount;
                             b.umt_fee -= umt_fee;
                             b.deferred_fee = 0;
                             if( NULL != cparty_info){
                               if( !b.p_accepted_memo.valid()){
                                 b.p_accepted_memo = cparty_info->p_memo;
                               }
                             }
                          });
      if( cull_if_small )
         return maybe_cull_small_order( *this, order );
      return false;
   }
} FC_CAPTURE_AND_RETHROW( (order)(pays)(receives) ) }

bool database::fill_order( const force_settlement_object& settle, const asset& pays, const asset& receives,
                           const price& fill_price, const bool is_maker )
{ try {
   bool filled = false;

   auto issuer_fees = pay_market_fees(get(receives.asset_id), receives);

   if( pays < settle.balance )
   {
      modify(settle, [&pays](force_settlement_object& s) {
         s.balance -= pays;
      });
      filled = false;
   } else {
      filled = true;
   }
   adjust_balance(settle.owner, receives - issuer_fees);

   assert( pays.asset_id != receives.asset_id );
   push_applied_operation( fill_order_operation( settle.id, settle.owner, pays, receives, issuer_fees, fill_price, is_maker ) );

   if (filled)
      remove(settle);

   return filled;
} FC_CAPTURE_AND_RETHROW( (settle)(pays)(receives) ) }

void database::pay_order( const account_object& receiver, const asset& receives, const asset& pays )
{
   const auto& balances = receiver.statistics(*this);
   modify( balances, [&]( account_statistics_object& b ){
         if( pays.asset_id == asset_id_type() )
         {
            b.total_core_in_orders -= pays.amount;
         }
   });
   adjust_balance(receiver.get_id(), receives);
}

asset database::calculate_market_fee( const asset_object& trade_asset, const asset& trade_amount )
{
   assert( trade_asset.id == trade_amount.asset_id );

   if( !trade_asset.charges_market_fees() )
      return trade_asset.amount(0);
   if( trade_asset.options.market_fee_percent == 0 )
      return trade_asset.amount(0);

   fc::uint128 a(trade_amount.amount.value);
   a *= trade_asset.options.market_fee_percent;
   a /= GRAPHENE_100_PERCENT;
   asset percent_fee = trade_asset.amount(a.to_uint64());

   if( percent_fee.amount > trade_asset.options.max_market_fee )
      percent_fee.amount = trade_asset.options.max_market_fee;

   return percent_fee;
}

asset database::pay_market_fees( const asset_object& recv_asset, const asset& receives )
{
   auto issuer_fees = calculate_market_fee( recv_asset, receives );
   assert(issuer_fees <= receives );

   //Don't dirty undo state if not actually collecting any fees
   if( issuer_fees.amount > 0 )
   {
      const auto& recv_dyn_data = recv_asset.dynamic_asset_data_id(*this);
      modify( recv_dyn_data, [&]( asset_dynamic_data_object& obj ){
                   //idump((issuer_fees));
         obj.accumulated_fees += issuer_fees.amount;
      });
   }

   return issuer_fees;
}

} }
