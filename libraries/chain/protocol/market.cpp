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
#include <graphene/chain/protocol/market.hpp>

namespace graphene { namespace chain {

share_type cut_fee(share_type a, uint16_t p)
{
   if( a == 0 || p == 0 )
      return 0;
   if( p == GRAPHENE_100_PERCENT )
      return a;

   fc::uint128 r(a.value);
   r *= p;
   //rounding to bigger value
   r += GRAPHENE_100_PERCENT - 1;
   r /= GRAPHENE_100_PERCENT;
   return r.to_uint64();
}

void limit_order_create_operation::validate()const
{
   FC_ASSERT( amount_to_sell.asset_id != min_to_receive.asset_id );
   FC_ASSERT( fee.amount >= 0 );
   FC_ASSERT( amount_to_sell.amount > 0 );
   FC_ASSERT( min_to_receive.amount > 0 );
}

void limit_order_accept_operation::validate()const
{
   FC_ASSERT( asset_id_to_sell != asset_id_to_receive );
   FC_ASSERT( fee.amount >= 0 );
}

void limit_order_cancel_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}

uint64_t  limit_order_create_operation::get_sales_ufee_percent(const fee_parameters_type& k)const
{
  uint64_t _ufee_percent = 0;

  auto use_feelevel = feelevel;
  if ( k.accufee.find(use_feelevel) == k.accufee.end() ) {  // not found
    use_feelevel = 0;
  }

  if ( k.accufee.find(use_feelevel) != k.accufee.end() ) {  // found
    auto usefee  = k.accufee.at(use_feelevel);
    if( amount_to_sell.asset_id == GRAPHENE_SDR_ASSET_ID){
      _ufee_percent = usefee.ubuy;
    }else if( min_to_receive.asset_id == GRAPHENE_SDR_ASSET_ID){
      _ufee_percent = usefee.usell; 
    }
  }

  return _ufee_percent;
}

share_type limit_order_create_operation::calculate_reserve_ufee(const fee_parameters_type& k )const
{
  if( amount_to_sell.asset_id == GRAPHENE_SDR_ASSET_ID){
    // we will reserve SDRt only if we buy smth for SDRt; otherwise all SDRt fee is paid from received SDRt amount
    auto _ufee_percent = get_sales_ufee_percent(k);
    if( _ufee_percent > 0){
      share_type ufee_sales = cut_fee( amount_to_sell.amount, _ufee_percent);
      return ufee_sales;
    }
  }

  return 0;
}

dualfee limit_order_create_operation::calculate_fee(const fee_parameters_type& k )const
{
  //calculates just transaction part of the fee
  share_type ufee_required = k.ufee + calculate_data_fee( fc::raw::pack_size(p_memo), k.ufee_pkb );
  return dualfee{k.fee, ufee_required};
}

} } // graphene::chain
