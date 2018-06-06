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

#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( uia_tests, database_fixture )

BOOST_AUTO_TEST_CASE( create_advanced_uia )
{
   try {
      asset_id_type test_asset_id = db.get_index<asset_object>().get_next_id();
      asset_create_operation creator;
      creator.issuer = account_id_type();
      creator.fee = asset();
      creator.symbol = "ADVANCED";
      creator.common_options.max_supply = 100000000;
      creator.precision = 2;
      creator.common_options.issuer_permissions = white_list|override_authority|transfer_restricted|disable_confidential;
      creator.common_options.flags = white_list|override_authority|disable_confidential;
      creator.common_options.whitelist_authorities = creator.common_options.blacklist_authorities = {account_id_type()};
      trx.operations.push_back(std::move(creator));
      PUSH_TX( db, trx, ~0 );

      const asset_object& test_asset = test_asset_id(db);
      BOOST_CHECK(test_asset.symbol == "ADVANCED");
      BOOST_CHECK(test_asset.options.flags & white_list);
      BOOST_CHECK(test_asset.options.max_supply == 100000000);

      const asset_dynamic_data_object& test_asset_dynamic_data = test_asset.dynamic_asset_data_id(db);
      BOOST_CHECK(test_asset_dynamic_data.current_supply == 0);
   } catch(fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_CASE( override_transfer_test )
{ try {
   ACTORS( (dan)(eric)(sam) );
   const asset_object& advanced = create_user_issued_asset( "ADVANCED", sam, override_authority );
   BOOST_TEST_MESSAGE( "Issuing 1000 ADVANCED to dan" );
   issue_uia( dan, advanced.amount( 1000 ) );
   BOOST_TEST_MESSAGE( "Checking dan's balance" );
   BOOST_REQUIRE_EQUAL( get_balance( dan, advanced ), 1000 );

   override_transfer_operation otrans;
   otrans.issuer = advanced.issuer;
   otrans.from = dan.id;
   otrans.to   = eric.id;
   otrans.amount = advanced.amount(100);
   trx.operations.push_back(otrans);

   BOOST_TEST_MESSAGE( "Require throwing without signature" );
   GRAPHENE_REQUIRE_THROW( PUSH_TX( db, trx, 0 ), tx_missing_active_auth );
   BOOST_TEST_MESSAGE( "Require throwing with dan's signature" );
   sign( trx,  dan_private_key  );
   GRAPHENE_REQUIRE_THROW( PUSH_TX( db, trx, 0 ), tx_missing_active_auth );
   BOOST_TEST_MESSAGE( "Pass with issuer's signature" );
   trx.signatures.clear();
   sign( trx,  sam_private_key  );
   PUSH_TX( db, trx, 0 );

   BOOST_REQUIRE_EQUAL( get_balance( dan, advanced ), 900 );
   BOOST_REQUIRE_EQUAL( get_balance( eric, advanced ), 100 );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( override_transfer_test2 )
{ try {
   ACTORS( (dan)(eric)(sam) );
   const asset_object& advanced = create_user_issued_asset( "ADVANCED", sam, 0 );
   issue_uia( dan, advanced.amount( 1000 ) );
   BOOST_REQUIRE_EQUAL( get_balance( dan, advanced ), 1000 );

   trx.operations.clear();
   override_transfer_operation otrans;
   otrans.issuer = advanced.issuer;
   otrans.from = dan.id;
   otrans.to   = eric.id;
   otrans.amount = advanced.amount(100);
   trx.operations.push_back(otrans);

   BOOST_TEST_MESSAGE( "Require throwing without signature" );
   GRAPHENE_REQUIRE_THROW( PUSH_TX( db, trx, 0 ), fc::exception);
   BOOST_TEST_MESSAGE( "Require throwing with dan's signature" );
   sign( trx,  dan_private_key  );
   GRAPHENE_REQUIRE_THROW( PUSH_TX( db, trx, 0 ), fc::exception);
   BOOST_TEST_MESSAGE( "Fail because overide_authority flag is not set" );
   trx.signatures.clear();
   sign( trx,  sam_private_key  );
   GRAPHENE_REQUIRE_THROW( PUSH_TX( db, trx, 0 ), fc::exception );

   BOOST_REQUIRE_EQUAL( get_balance( dan, advanced ), 1000 );
   BOOST_REQUIRE_EQUAL( get_balance( eric, advanced ), 0 );
} FC_LOG_AND_RETHROW() }

/**
 * verify that issuers can halt transfers
 */
BOOST_AUTO_TEST_CASE( transfer_restricted_test )
{
   try
   {
      ACTORS( (sam)(alice)(bob) );

      BOOST_TEST_MESSAGE( "Issuing 1000 UIA to Alice" );

      auto _issue_uia = [&]( const account_object& recipient, asset amount )
      {
         asset_issue_operation op;
         op.issuer = amount.asset_id(db).issuer;
         op.asset_to_issue = amount;
         op.issue_to_account = recipient.id;
         transaction tx;
         tx.operations.push_back( op );
         set_expiration( db, tx );
         PUSH_TX( db, tx, database::skip_authority_check | database::skip_tapos_check | database::skip_transaction_signatures );
      } ;

      const asset_object& uia = create_user_issued_asset( "TXRX", sam, transfer_restricted );
      _issue_uia( alice, uia.amount( 1000 ) );

      auto _restrict_xfer = [&]( bool xfer_flag )
      {
         asset_update_operation op;
         op.issuer = sam_id;
         op.asset_to_update = uia.id;
         op.new_options = uia.options;
         if( xfer_flag )
            op.new_options.flags |= transfer_restricted;
         else
            op.new_options.flags &= ~transfer_restricted;
         transaction tx;
         tx.operations.push_back( op );
         set_expiration( db, tx );
         PUSH_TX( db, tx, database::skip_authority_check | database::skip_tapos_check | database::skip_transaction_signatures );
      } ;

      BOOST_TEST_MESSAGE( "Enable transfer_restricted, send fails" );

      transfer_operation xfer_op;
      xfer_op.from = alice_id;
      xfer_op.to = bob_id;
      xfer_op.amount = uia.amount(100);
      signed_transaction xfer_tx;
      xfer_tx.operations.push_back( xfer_op );
      set_expiration( db, xfer_tx );
      sign( xfer_tx, alice_private_key );

      _restrict_xfer( true );
      GRAPHENE_REQUIRE_THROW( PUSH_TX( db, xfer_tx ), transfer_restricted_transfer_asset );

      BOOST_TEST_MESSAGE( "Disable transfer_restricted, send succeeds" );

      _restrict_xfer( false );
      PUSH_TX( db, xfer_tx );

      xfer_op.amount = uia.amount(101);

   }
   catch(fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}

#define ITWAS_HARDFORK_385_TIME (fc::time_point_sec( 1445558400 ))
#define ITWAS_HARDFORK_409_TIME (fc::time_point_sec( 1446652800 ))

BOOST_AUTO_TEST_CASE( asset_name_test )
{
   try
   {
      ACTORS( (alice)(bob) );

      auto has_asset = [&]( std::string symbol ) -> bool
      {
         const auto& assets_by_symbol = db.get_index_type<asset_index>().indices().get<by_symbol>();
         return assets_by_symbol.find( symbol ) != assets_by_symbol.end();
      };

      // Alice creates asset "ALPHA"
      BOOST_CHECK( !has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );
      create_user_issued_asset( "ALPHA", alice_id(db), 0 );
      BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );

      // Nobody can create another asset named ALPHA
      GRAPHENE_REQUIRE_THROW( create_user_issued_asset( "ALPHA",   bob_id(db), 0 ), fc::exception );
      BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );
      GRAPHENE_REQUIRE_THROW( create_user_issued_asset( "ALPHA", alice_id(db), 0 ), fc::exception );
      BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );

      generate_blocks( ITWAS_HARDFORK_385_TIME );
      generate_block();

      // Bob can't create ALPHA.ONE
      GRAPHENE_REQUIRE_THROW( create_user_issued_asset( "ALPHA.ONE", bob_id(db), 0 ), fc::exception );
      BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );
      if( db.head_block_time() <= ITWAS_HARDFORK_409_TIME )
      {
         // Alice can't create ALPHA.ONE before hardfork
         GRAPHENE_REQUIRE_THROW( create_user_issued_asset( "ALPHA.ONE", alice_id(db), 0 ), fc::exception );
         BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );
         generate_blocks( ITWAS_HARDFORK_409_TIME );
         generate_block();
         // Bob can't create ALPHA.ONE after hardfork
         GRAPHENE_REQUIRE_THROW( create_user_issued_asset( "ALPHA.ONE", bob_id(db), 0 ), fc::exception );
         BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( !has_asset("ALPHA.ONE") );
      }
      // Alice can create it
      create_user_issued_asset( "ALPHA.ONE", alice_id(db), 0 );
      BOOST_CHECK(  has_asset("ALPHA") );    BOOST_CHECK( has_asset("ALPHA.ONE") );
   }
   catch(fc::exception& e)
   {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_SUITE_END()
