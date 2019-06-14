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
#include <graphene/chain/protocol/buyback.hpp>
#include <graphene/chain/protocol/ext.hpp>
#include <graphene/chain/protocol/special_authority.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/protocol/vote.hpp>

namespace graphene { namespace chain {

   bool is_valid_name( const string& s );
   bool is_cheap_name( const string& n );

   /// These are the fields which can be updated by the active authority.
   struct account_options
   {
      /// The memo key is the key this account will typically use to encrypt/sign transaction memos and other non-
      /// validated account activities. This field is here to prevent confusion if the active authority has zero or
      /// multiple keys in it.
      public_key_type  memo_key;
      /// If this field is set to an account ID other than GRAPHENE_PROXY_TO_SELF_ACCOUNT,
      /// then this account's votes will be ignored; its stake
      /// will be counted as voting for the referenced account's selected votes instead.
      account_id_type voting_account = GRAPHENE_PROXY_TO_SELF_ACCOUNT;

      /// The number of active witnesses this account votes the blockchain should appoint
      /// Must not exceed the actual number of witnesses voted for in @ref votes
      uint16_t num_witness = 0;
      /// The number of active committee members this account votes the blockchain should appoint
      /// Must not exceed the actual number of committee members voted for in @ref votes
      uint16_t num_committee = 0;
      /// Fee level of the account in limit_order_create_operation; the greater feelevel the smaller fee
      uint16_t feelevel = 0;
      /// This is the list of vote IDs this account votes for. The weight of these votes is determined by this
      /// account's balance of core asset.
      flat_set<vote_id_type> votes;
      extensions_type        extensions;

      void validate()const;
   };

   /**
    *  @ingroup operations
    */
   struct account_create_operation : public base_operation
   {
      struct ext
      {
         optional< void_t >            null_ext;
         optional< special_authority > owner_special_authority;
         optional< special_authority > active_special_authority;
      };

      struct fee_parameters_type
      {
         uint64_t basic_fee      = 5*GRAPHENE_BLOCKCHAIN_PRECISION; ///< the cost to register the cheapest non-free account
         uint64_t premium_fee    = 2000*GRAPHENE_BLOCKCHAIN_PRECISION; ///< the cost to register the cheapest non-free account
         uint32_t price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t ufee = 0;  //SDR fee per operation
      };

      asset           fee;
      asset           ufee = asset( 0, GRAPHENE_SDR_ASSET_ID );  //fee in sdr

      /// This account pays the fee. Must be a lifetime member.
      account_id_type registrar;

      string          name;
      authority       owner;
      authority       active;

      account_options options;
      extension< ext > extensions;

      account_id_type fee_payer()const { return registrar; }
      void            validate()const;
      dualfee      calculate_fee(const fee_parameters_type& )const;

      void get_required_active_authorities( flat_set<account_id_type>& a )const
      {
         // registrar should be required anyway as it is the fee_payer(), but we insert it here just to be sure
         a.insert( registrar );
      }
   };

   /**
    * @ingroup operations
    * @brief Update an existing account
    *
    * This operation is used to update an existing account. It can be used to update the authorities, or adjust the options on the account.
    * See @ref account_object::options_type for the options which may be updated.
    */
   struct account_update_operation : public base_operation
   {
      struct ext
      {
         optional< void_t >            null_ext;
         optional< special_authority > owner_special_authority;
         optional< special_authority > active_special_authority;
      };

      struct fee_parameters_type
      {
         share_type fee             = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t   price_per_kbyte = GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t ufee = 0;  //SDR fee per operation
      };

      asset      fee;
      asset      ufee = asset( 0, GRAPHENE_SDR_ASSET_ID );  //fee in sdr

      /// The account to update
      account_id_type account;

      /// New owner authority. If set, this operation requires owner authority to execute.
      optional<authority> owner;
      /// New active authority. This can be updated by the current active authority.
      optional<authority> active;

      /// New account options
      optional<account_options> new_options;
      extension< ext > extensions;

      account_id_type fee_payer()const { return account; }
      void       validate()const;
      dualfee calculate_fee( const fee_parameters_type& k )const;

      bool is_owner_update()const
      { return owner || extensions.value.owner_special_authority.valid(); }

      void get_required_owner_authorities( flat_set<account_id_type>& a )const
      { if( is_owner_update() ) a.insert( account ); }

      void get_required_active_authorities( flat_set<account_id_type>& a )const
      { if( !is_owner_update() ) a.insert( account ); }
   };

   /**
    * @brief Manage an account's membership status
    * @ingroup operations
    *
    * This operation is used to upgrade an account to a member, or renew its subscription. If an account which is an
    * unexpired annual subscription member publishes this operation with @ref upgrade_to_lifetime_member set to false,
    * the account's membership expiration date will be pushed backward one year. If a basic account publishes it with
    * @ref upgrade_to_lifetime_member set to false, the account will be upgraded to a subscription member with an
    * expiration date one year after the processing time of this operation.
    *
    * Any account may use this operation to become a lifetime member by setting @ref upgrade_to_lifetime_member to
    * true. Once an account has become a lifetime member, it may not use this operation anymore.
    */
   struct account_upgrade_operation : public base_operation
   {
      struct fee_parameters_type { 
         uint64_t membership_annual_fee   =  2000 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint64_t membership_lifetime_fee = 10000 * GRAPHENE_BLOCKCHAIN_PRECISION; ///< the cost to upgrade to a lifetime member
         uint64_t ufee = 0;  //SDR fee per operation
      };

      asset             fee;
      asset             ufee = asset( 0, GRAPHENE_SDR_ASSET_ID );  //fee in sdr
      /// The account to upgrade; must not already be a lifetime member
      account_id_type   account_to_upgrade;
      /// If true, the account will be upgraded to a lifetime member; otherwise, it will add a year to the subscription
      bool              upgrade_to_lifetime_member = false;
      extensions_type   extensions;

      account_id_type fee_payer()const { return account_to_upgrade; }
      void       validate()const;
      dualfee calculate_fee( const fee_parameters_type& k )const;
   };

} } // graphene::chain

FC_REFLECT(graphene::chain::account_options, (memo_key)(voting_account)(num_witness)(num_committee)(feelevel)(votes)(extensions))

FC_REFLECT(graphene::chain::account_create_operation::ext, (null_ext)(owner_special_authority)(active_special_authority) )
FC_REFLECT( graphene::chain::account_create_operation,
            (fee)(ufee)(registrar)
            (name)(owner)(active)(options)(extensions)
          )

FC_REFLECT(graphene::chain::account_update_operation::ext, (null_ext)(owner_special_authority)(active_special_authority) )
FC_REFLECT( graphene::chain::account_update_operation,
            (fee)(ufee)(account)(owner)(active)(new_options)(extensions)
          )

FC_REFLECT( graphene::chain::account_upgrade_operation,
            (fee)(ufee)(account_to_upgrade)(upgrade_to_lifetime_member)(extensions) )

FC_REFLECT( graphene::chain::account_create_operation::fee_parameters_type, (basic_fee)(premium_fee)(price_per_kbyte)(ufee) )
FC_REFLECT( graphene::chain::account_update_operation::fee_parameters_type, (fee)(price_per_kbyte)(ufee) )
FC_REFLECT( graphene::chain::account_upgrade_operation::fee_parameters_type, (membership_annual_fee)(membership_lifetime_fee)(ufee) )
