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
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>

namespace graphene { namespace chain { 

   /**
    *  @class limit_order_create_operation
    *  @brief instructs the blockchain to attempt to sell one asset for another
    *  @ingroup operations
    *
    *  The blockchain will atempt to sell amount_to_sell.asset_id for as
    *  much min_to_receive.asset_id as possible.  The fee will be paid by
    *  the seller's account.  Market fees will apply as specified by the
    *  issuer of both the selling asset and the receiving asset as
    *  a percentage of the amount exchanged.
    *
    *  If either the selling asset or the receiving asset is white list
    *  restricted, the order will only be created if the seller is on
    *  the white list of the restricted asset type.
    *
    *  Market orders are matched in the order they are included
    *  in the block chain.
    */
   struct limit_order_create_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 5 * GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset           fee;
      account_id_type seller;
      asset           amount_to_sell;
      asset           min_to_receive;

      /// The order will be removed from the books if not filled by expiration
      /// Upon expiration, all unsold asset will be returned to seller
      time_point_sec expiration = time_point_sec::maximum();

      /// If this flag is set the entire order must be filled or the operation is rejected
      bool fill_or_kill = false;

      /// SM: introduced to hold provider and operator specific information
      optional< uint64_t > request_id;
      optional< uint64_t > user_id;
      optional< account_id_type > counterparty_id;
      optional< memo_data > p_memo;
      optional< bid_id_type > bid_id;

      extensions_type   extensions;

      pair<asset_id_type,asset_id_type> get_market()const
      {
         return amount_to_sell.asset_id < min_to_receive.asset_id ?
                std::make_pair(amount_to_sell.asset_id, min_to_receive.asset_id) :
                std::make_pair(min_to_receive.asset_id, amount_to_sell.asset_id);
      }
      account_id_type fee_payer()const { return seller; }
      void            validate()const;
      price           get_price()const { return amount_to_sell / min_to_receive; }
   };

   struct limit_order_accept_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 5 * GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset           fee;
      account_id_type seller;
      asset_id_type asset_id_to_sell;
      asset_id_type asset_id_to_receive;

      /// SM: introduced to hold provider and operator specific information
      uint64_t request_id;
      uint64_t user_id;
      account_id_type counterparty_id;
      memo_data p_memo; //private encoded message

      extensions_type   extensions;

      pair<asset_id_type,asset_id_type> get_market()const
      {
         return asset_id_to_sell < asset_id_to_receive ?
                std::make_pair(asset_id_to_sell, asset_id_to_receive) :
                std::make_pair(asset_id_to_receive, asset_id_to_sell);
      }
      account_id_type fee_payer()const { return seller; }
      void            validate() const;
   };

   struct limit_order_accepted_operation : public base_operation
   {
      struct fee_parameters_type {};

      limit_order_accepted_operation() {}

      asset          fee;

      object_id_type      order_id;
      account_id_type     order_creator_account_id;
      asset_id_type       asset_id_to_sell;
      asset_id_type       asset_id_to_receive;

      uint64_t request_id;
      uint64_t user_id;

      account_id_type     accepted_by_account_id;
      memo_data p_accepted_memo;  //private encoded message

      account_id_type fee_payer()const { return order_creator_account_id; }
      void            validate()const { FC_ASSERT( !"virtual operation" ); }

      /// This is a virtual operation; there is no fee
      share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
   };
   
   /**
    *  @ingroup operations
    *  Used to cancel an existing limit order. Both fee_pay_account and the
    *  account to receive the proceeds must be the same as order->seller.
    *
    *  @return the amount actually refunded
    */
   struct limit_order_cancel_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 0; };

      asset               fee;
      limit_order_id_type order;
      /** must be order->seller */
      account_id_type     fee_paying_account;
      extensions_type   extensions;

      account_id_type fee_payer()const { return fee_paying_account; }
      void            validate()const;
   };

   struct counterparty_info
   {
     account_id_type     account;
     limit_order_id_type order;
     optional< uint64_t > request_id;
     optional< uint64_t > user_id;
     optional< memo_data > p_memo;
   };

   /**
    * @ingroup operations
    *
    * @note This is a virtual operation that is created while matching orders and
    * emitted for the purpose of accurately tracking account history, accelerating
    * a reindex.
    */
   struct fill_order_operation : public base_operation
   {
      struct fee_parameters_type {};

      fill_order_operation(){}
      fill_order_operation( object_id_type o, account_id_type a, asset p, asset r, price fp, bool m, const counterparty_info* cparty_info = NULL)
         :order_id(o),account_id(a),pays(p),receives(r),fill_price(fp),is_maker(m) {
           if( NULL != cparty_info){
             match_account_id = cparty_info->account;
             match_order_id = cparty_info->order;
             request_id = cparty_info->request_id;
             user_id = cparty_info->user_id;
             p_memo = cparty_info->p_memo;
           }
         }

      object_id_type      order_id;
      account_id_type     account_id;
      asset               pays;
      asset               receives;
      asset               fee; // paid by receiving account
      price               fill_price;
      bool                is_maker;

      account_id_type      match_account_id;    
      limit_order_id_type  match_order_id;    

      optional< uint64_t > request_id;
      optional< uint64_t > user_id;
      optional< memo_data > p_memo;

      pair<asset_id_type,asset_id_type> get_market()const
      {
         return pays.asset_id < receives.asset_id ?
                std::make_pair( pays.asset_id, receives.asset_id ) :
                std::make_pair( receives.asset_id, pays.asset_id );
      }
      account_id_type fee_payer()const { return account_id; }
      void            validate()const { FC_ASSERT( !"virtual operation" ); }

      /// This is a virtual operation; there is no fee
      share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
   };

   /**
    * @ingroup operations
    *
    * @note This is a virtual operation that is created while reviving a
    * bitasset from collateral bids.
    */
   struct execute_bid_operation : public base_operation
   {
      struct fee_parameters_type {};

      execute_bid_operation(){}
      execute_bid_operation( account_id_type a, asset d, asset c )
         : bidder(a), debt(d), collateral(c) {}

      account_id_type     bidder;
      asset               debt;
      asset               collateral;
      asset               fee;

      account_id_type fee_payer()const { return bidder; }
      void            validate()const { FC_ASSERT( !"virtual operation" ); }

      /// This is a virtual operation; there is no fee
      share_type      calculate_fee(const fee_parameters_type& k)const { return 0; }
   };
} } // graphene::chain

FC_REFLECT( graphene::chain::limit_order_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::limit_order_cancel_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::fill_order_operation::fee_parameters_type,  ) // VIRTUAL
FC_REFLECT( graphene::chain::execute_bid_operation::fee_parameters_type,  ) // VIRTUAL
FC_REFLECT( graphene::chain::limit_order_accept_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::limit_order_accepted_operation::fee_parameters_type, ) // VIRTUAL

FC_REFLECT( graphene::chain::limit_order_create_operation,(fee)(seller)(amount_to_sell)(min_to_receive)(expiration)(fill_or_kill)(request_id)(user_id)(counterparty_id)(p_memo)(bid_id)(extensions))
FC_REFLECT( graphene::chain::limit_order_cancel_operation,(fee)(fee_paying_account)(order)(extensions) )
FC_REFLECT( graphene::chain::fill_order_operation, (fee)(order_id)(account_id)(pays)(receives)(fill_price)(is_maker)(match_account_id)(match_order_id)(request_id)(user_id)(p_memo))
FC_REFLECT( graphene::chain::execute_bid_operation, (fee)(bidder)(debt)(collateral) )
FC_REFLECT( graphene::chain::limit_order_accept_operation,(fee)(seller)(asset_id_to_sell)(asset_id_to_receive)(request_id)(user_id)(counterparty_id)(p_memo)(extensions))
FC_REFLECT( graphene::chain::limit_order_accepted_operation,(fee)(order_id)(order_creator_account_id)(asset_id_to_sell)(asset_id_to_receive)(request_id)(user_id)(accepted_by_account_id)(p_accepted_memo))
