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
#include <graphene/chain/hardfork.hpp>

#include <graphene/chain/balance_object.hpp>
#include <graphene/chain/budget_record_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/market_object.hpp>
#include <graphene/chain/withdraw_permission_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/worker_object.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( operation_tests, database_fixture )

/***
 * A descriptor of a particular withdrawal period
 */
struct withdrawal_period_descriptor {
   withdrawal_period_descriptor(const time_point_sec start, const time_point_sec end, const asset available, const asset claimed)
      : period_start_time(start), period_end_time(end), available_this_period(available), claimed_this_period(claimed) {}

   // Start of period
   time_point_sec period_start_time;

   // End of period
   time_point_sec period_end_time;

   // Quantify how much is still available to be withdrawn during this period
   asset available_this_period;

   // Quantify how much has already been claimed during this period
   asset claimed_this_period;

   string const to_string() const {
       string asset_id = fc::to_string(available_this_period.asset_id.space_id)
                         + "." + fc::to_string(available_this_period.asset_id.type_id)
                         + "." + fc::to_string(available_this_period.asset_id.instance.value);
       string text = fc::to_string(available_this_period.amount.value)
                     + " " + asset_id
                     + " is available from " + period_start_time.to_iso_string()
                     + " to " + period_end_time.to_iso_string();
       return text;
   }
};


/***
 * Get a description of the current withdrawal period
 * @param current_time   Current time
 * @return A description of the current period
 */
withdrawal_period_descriptor current_period(const withdraw_permission_object& permit, fc::time_point_sec current_time) {
   // @todo [6] Is there a potential race condition where a call to available_this_period might become out of sync with this function's later use of period start time?
   asset available = permit.available_this_period(current_time);
   asset claimed = asset(permit.withdrawal_limit.amount - available.amount, permit.withdrawal_limit.asset_id);
   auto periods = (current_time - permit.period_start_time).to_seconds() / permit.withdrawal_period_sec;
   time_point_sec current_period_start = permit.period_start_time + (periods * permit.withdrawal_period_sec);
   time_point_sec current_period_end = current_period_start + permit.withdrawal_period_sec;
   withdrawal_period_descriptor descriptor = withdrawal_period_descriptor(current_period_start, current_period_end, available, claimed);

   return descriptor;
}

/**
 * This auxiliary test is used for two purposes:
 * (a) it checks the creation of withdrawal claims,
 * (b) it is used as a precursor for tests that evaluate withdrawal claims.
 *
 * NOTE: This test verifies proper withdrawal claim behavior
 * as it occurred before (for backward compatibility)
 * Issue #23 was addressed.
 * That issue is concerned with ensuring that the first claim
 * can occur before the first withdrawal period.
 */
BOOST_AUTO_TEST_CASE( withdraw_permission_create_before_hardfork_23 )
{ try {
   auto nathan_private_key = generate_private_key("nathan");
   auto dan_private_key = generate_private_key("dan");
   account_id_type nathan_id = create_account("nathan", nathan_private_key.get_public_key()).id;
   account_id_type dan_id = create_account("dan", dan_private_key.get_public_key()).id;

   transfer(account_id_type(), nathan_id, asset(1000));
   generate_block();
   set_expiration( db, trx );

   {
      withdraw_permission_create_operation op;
      op.authorized_account = dan_id;
      op.withdraw_from_account = nathan_id;
      op.withdrawal_limit = asset(5);
      op.withdrawal_period_sec = fc::hours(1).to_seconds();
      op.periods_until_expiration = 5;
      op.period_start_time = db.head_block_time() + db.get_global_properties().parameters.block_interval*5; // 5 blocks after current blockchain time
      trx.operations.push_back(op);
      REQUIRE_OP_VALIDATION_FAILURE(op, withdrawal_limit, asset());
      REQUIRE_OP_VALIDATION_FAILURE(op, periods_until_expiration, 0);
      REQUIRE_OP_VALIDATION_FAILURE(op, withdraw_from_account, dan_id);
      REQUIRE_OP_VALIDATION_FAILURE(op, withdrawal_period_sec, 0);
      REQUIRE_THROW_WITH_VALUE(op, withdrawal_limit, asset(10, asset_id_type(10)));
      REQUIRE_THROW_WITH_VALUE(op, authorized_account, account_id_type(1000));
      REQUIRE_THROW_WITH_VALUE(op, period_start_time, fc::time_point_sec(10000));
      REQUIRE_THROW_WITH_VALUE(op, withdrawal_period_sec, 1);
      trx.operations.back() = op;
   }
   sign( trx, nathan_private_key );
   db.push_transaction( trx );
   trx.clear();
} FC_LOG_AND_RETHROW() }

/**
 * This auxiliary test is used for two purposes:
 * (a) it checks the creation of withdrawal claims,
 * (b) it is used as a precursor for tests that evaluate withdrawal claims.
 *
 * NOTE: This test verifies proper withdrawal claim behavior
 * as it should occur after Issue #23 is addressed.
 * That issue is concerned with ensuring that the first claim
 * can occur before the first withdrawal period.
 */
#define ITWAS_HARDFORK_23_TIME (fc::time_point_sec( 1512747600 ))
BOOST_AUTO_TEST_CASE( withdraw_permission_create_after_hardfork_23 )
{ try {
   auto nathan_private_key = generate_private_key("nathan");
   auto dan_private_key = generate_private_key("dan");
   account_id_type nathan_id = create_account("nathan", nathan_private_key.get_public_key()).id;
   account_id_type dan_id = create_account("dan", dan_private_key.get_public_key()).id;

   transfer(account_id_type(), nathan_id, asset(1000));
   generate_block();
   set_expiration( db, trx );

   {
      withdraw_permission_create_operation op;
      op.authorized_account = dan_id;
      op.withdraw_from_account = nathan_id;
      op.withdrawal_limit = asset(5);
      op.withdrawal_period_sec = fc::hours(1).to_seconds();
      op.periods_until_expiration = 5;
      op.period_start_time = ITWAS_HARDFORK_23_TIME + db.get_global_properties().parameters.block_interval*5; // 5 blocks after fork time
      trx.operations.push_back(op);
      REQUIRE_OP_VALIDATION_FAILURE(op, withdrawal_limit, asset());
      REQUIRE_OP_VALIDATION_FAILURE(op, periods_until_expiration, 0);
      REQUIRE_OP_VALIDATION_FAILURE(op, withdraw_from_account, dan_id);
      REQUIRE_OP_VALIDATION_FAILURE(op, withdrawal_period_sec, 0);
      REQUIRE_THROW_WITH_VALUE(op, withdrawal_limit, asset(10, asset_id_type(10)));
      REQUIRE_THROW_WITH_VALUE(op, authorized_account, account_id_type(1000));
      REQUIRE_THROW_WITH_VALUE(op, period_start_time, fc::time_point_sec(10000));
      REQUIRE_THROW_WITH_VALUE(op, withdrawal_period_sec, 1);
      trx.operations.back() = op;
   }
   sign( trx, nathan_private_key );
   db.push_transaction( trx );
   trx.clear();
} FC_LOG_AND_RETHROW() }

/**
 * Test the claims of withdrawals both before and during
 * authorized withdrawal periods.
 * NOTE: The simulated elapse of blockchain time through the use of
 * generate_blocks() must be carefully used in order to simulate
 * this test.
 * NOTE: This test verifies proper withdrawal claim behavior
 * as it occurred before (for backward compatibility)
 * Issue #23 was addressed.
 * That issue is concerned with ensuring that the first claim
 * can occur before the first withdrawal period.
 */
BOOST_AUTO_TEST_CASE( withdraw_permission_test_before_hardfork_23 )
{ try {
      INVOKE(withdraw_permission_create_before_hardfork_23);

      auto nathan_private_key = generate_private_key("nathan");
      auto dan_private_key = generate_private_key("dan");
      account_id_type nathan_id = get_account("nathan").id;
      account_id_type dan_id = get_account("dan").id;
      withdraw_permission_id_type permit;
      set_expiration( db, trx );

      fc::time_point_sec first_start_time;
      {
          const withdraw_permission_object& permit_object = permit(db);
          BOOST_CHECK(permit_object.authorized_account == dan_id);
          BOOST_CHECK(permit_object.withdraw_from_account == nathan_id);
          BOOST_CHECK(permit_object.period_start_time > db.head_block_time());
          first_start_time = permit_object.period_start_time;
          BOOST_CHECK(permit_object.withdrawal_limit == asset(5));
          BOOST_CHECK(permit_object.withdrawal_period_sec == fc::hours(1).to_seconds());
          BOOST_CHECK(permit_object.expiration == first_start_time + permit_object.withdrawal_period_sec*5 );
      }

   generate_blocks(2); // Still before the first period, but BEFORE the real time during which "early" claims are checked

      {
          withdraw_permission_claim_operation op;
          op.withdraw_permission = permit;
          op.withdraw_from_account = nathan_id;
          op.withdraw_to_account = dan_id;
          op.amount_to_withdraw = asset(1);
          set_expiration( db, trx );

          trx.operations.push_back(op);
          sign( trx, dan_private_key ); // Transaction should be signed to be valid

          // This operation/transaction will be pushed early/before the first
          // withdrawal period
          // However, this will not cause an exception prior to HARDFORK_23_TIME
          // because withdrawaing before that the first period was acceptable
          // before the fix
          PUSH_TX( db, trx ); // <-- Claim #1


          //Get to the actual withdrawal period
          bool miss_intermediate_blocks = false; // Required to have generate_blocks() elapse flush to the time of interest
          generate_blocks(first_start_time, miss_intermediate_blocks);
          set_expiration( db, trx );

          REQUIRE_THROW_WITH_VALUE(op, withdraw_permission, withdraw_permission_id_type(5));
          REQUIRE_THROW_WITH_VALUE(op, withdraw_from_account, dan_id);
          REQUIRE_THROW_WITH_VALUE(op, withdraw_from_account, account_id_type());
          REQUIRE_THROW_WITH_VALUE(op, withdraw_to_account, nathan_id);
          REQUIRE_THROW_WITH_VALUE(op, withdraw_to_account, account_id_type());
          REQUIRE_THROW_WITH_VALUE(op, amount_to_withdraw, asset(10));
          REQUIRE_THROW_WITH_VALUE(op, amount_to_withdraw, asset(6));
          set_expiration( db, trx );
          trx.clear();
          trx.operations.push_back(op);
          sign( trx, dan_private_key );
          PUSH_TX( db, trx ); // <-- Claim #2

          // would be legal on its own, but doesn't work because trx already withdrew
          REQUIRE_THROW_WITH_VALUE(op, amount_to_withdraw, asset(5));

          // Make sure we can withdraw again this period, as long as we're not exceeding the periodic limit
          trx.clear();
          // withdraw 1
          trx.operations = {op};
          // make it different from previous trx so it's non-duplicate
          trx.expiration += fc::seconds(1);
          sign( trx, dan_private_key );
          PUSH_TX( db, trx ); // <-- Claim #3
          trx.clear();
      }

      // Account for three (3) claims of one (1) unit
      BOOST_CHECK_EQUAL(get_balance(nathan_id, asset_id_type()), 997);
      BOOST_CHECK_EQUAL(get_balance(dan_id, asset_id_type()), 3);

      {
          const withdraw_permission_object& permit_object = permit(db);
          BOOST_CHECK(permit_object.authorized_account == dan_id);
          BOOST_CHECK(permit_object.withdraw_from_account == nathan_id);
          BOOST_CHECK(permit_object.period_start_time == first_start_time);
          BOOST_CHECK(permit_object.withdrawal_limit == asset(5));
          BOOST_CHECK(permit_object.withdrawal_period_sec == fc::hours(1).to_seconds());
          BOOST_CHECK_EQUAL(permit_object.claimed_this_period.value, 3 ); // <-- Account for three (3) claims of one (1) unit
          BOOST_CHECK(permit_object.expiration == first_start_time + 5*permit_object.withdrawal_period_sec);
          generate_blocks(first_start_time + permit_object.withdrawal_period_sec);
          // lazy update:  verify period_start_time isn't updated until new trx occurs
          BOOST_CHECK(permit_object.period_start_time == first_start_time);
      }

      {
          // Leave Nathan with one unit
          transfer(nathan_id, dan_id, asset(996));

          // Attempt a withdrawal claim for units than available
          withdraw_permission_claim_operation op;
          op.withdraw_permission = permit;
          op.withdraw_from_account = nathan_id;
          op.withdraw_to_account = dan_id;
          op.amount_to_withdraw = asset(5);
          trx.operations.push_back(op);
          set_expiration( db, trx );
          sign( trx, dan_private_key );
          //Throws because nathan doesn't have the money
          GRAPHENE_CHECK_THROW(PUSH_TX( db, trx ), fc::exception);

          // Attempt a withdrawal claim for which nathan does have sufficient units
          op.amount_to_withdraw = asset(1);
          trx.clear();
          trx.operations = {op};
          set_expiration( db, trx );
          sign( trx, dan_private_key );
          PUSH_TX( db, trx );
      }

      BOOST_CHECK_EQUAL(get_balance(nathan_id, asset_id_type()), 0);
      BOOST_CHECK_EQUAL(get_balance(dan_id, asset_id_type()), 1000);
      trx.clear();
      transfer(dan_id, nathan_id, asset(1000));

      {
          const withdraw_permission_object& permit_object = permit(db);
          BOOST_CHECK(permit_object.authorized_account == dan_id);
          BOOST_CHECK(permit_object.withdraw_from_account == nathan_id);
          BOOST_CHECK(permit_object.period_start_time == first_start_time + permit_object.withdrawal_period_sec);
          BOOST_CHECK(permit_object.expiration == first_start_time + 5*permit_object.withdrawal_period_sec);
          BOOST_CHECK(permit_object.withdrawal_limit == asset(5));
          BOOST_CHECK(permit_object.withdrawal_period_sec == fc::hours(1).to_seconds());
          generate_blocks(permit_object.expiration);
      }
      // Ensure the permit object has been garbage collected
      BOOST_CHECK(db.find_object(permit) == nullptr);

      {
          withdraw_permission_claim_operation op;
          op.withdraw_permission = permit;
          op.withdraw_from_account = nathan_id;
          op.withdraw_to_account = dan_id;
          op.amount_to_withdraw = asset(5);
          trx.operations.push_back(op);
          set_expiration( db, trx );
          sign( trx, dan_private_key );
          //Throws because the permission has expired
          GRAPHENE_CHECK_THROW(PUSH_TX( db, trx ), fc::exception);
      }
  } FC_LOG_AND_RETHROW() }

/**
 * Test the claims of withdrawals both before and during
 * authorized withdrawal periods.
 * NOTE: The simulated elapse of blockchain time through the use of
 * generate_blocks() must be carefully used in order to simulate
 * this test.
 * NOTE: This test verifies proper withdrawal claim behavior
 * as it should occur after Issue #23 is addressed.
 * That issue is concerned with ensuring that the first claim
 * can occur before the first withdrawal period.
 */
BOOST_AUTO_TEST_CASE( withdraw_permission_test_after_hardfork_23 )
{ try {
   INVOKE(withdraw_permission_create_after_hardfork_23);

   auto nathan_private_key = generate_private_key("nathan");
   auto dan_private_key = generate_private_key("dan");
   account_id_type nathan_id = get_account("nathan").id;
   account_id_type dan_id = get_account("dan").id;
   withdraw_permission_id_type permit;
   set_expiration( db, trx );

   fc::time_point_sec first_start_time;
   {
      const withdraw_permission_object& permit_object = permit(db);
      BOOST_CHECK(permit_object.authorized_account == dan_id);
      BOOST_CHECK(permit_object.withdraw_from_account == nathan_id);
      BOOST_CHECK(permit_object.period_start_time > db.head_block_time());
      first_start_time = permit_object.period_start_time;
      BOOST_CHECK(permit_object.withdrawal_limit == asset(5));
      BOOST_CHECK(permit_object.withdrawal_period_sec == fc::hours(1).to_seconds());
      BOOST_CHECK(permit_object.expiration == first_start_time + permit_object.withdrawal_period_sec*5 );
   }

   generate_blocks( ITWAS_HARDFORK_23_TIME); // Still before the first period, but DURING the real time during which "early" claims are checked

   {
      withdraw_permission_claim_operation op;
      op.withdraw_permission = permit;
      op.withdraw_from_account = nathan_id;
      op.withdraw_to_account = dan_id;
      op.amount_to_withdraw = asset(1);
      set_expiration( db, trx );

      trx.operations.push_back(op);
      sign( trx, dan_private_key ); // Transaction should be signed to be valid
      //Throws because we haven't entered the first withdrawal period yet.
      GRAPHENE_REQUIRE_THROW(PUSH_TX( db, trx ), fc::exception);
      //Get to the actual withdrawal period
      bool miss_intermediate_blocks = false; // Required to have generate_blocks() elapse flush to the time of interest
      generate_blocks(first_start_time, miss_intermediate_blocks);
          set_expiration( db, trx );

      REQUIRE_THROW_WITH_VALUE(op, withdraw_permission, withdraw_permission_id_type(5));
      REQUIRE_THROW_WITH_VALUE(op, withdraw_from_account, dan_id);
      REQUIRE_THROW_WITH_VALUE(op, withdraw_from_account, account_id_type());
      REQUIRE_THROW_WITH_VALUE(op, withdraw_to_account, nathan_id);
      REQUIRE_THROW_WITH_VALUE(op, withdraw_to_account, account_id_type());
      REQUIRE_THROW_WITH_VALUE(op, amount_to_withdraw, asset(10));
      REQUIRE_THROW_WITH_VALUE(op, amount_to_withdraw, asset(6));
      set_expiration( db, trx );
      trx.clear();
      trx.operations.push_back(op);
      sign( trx, dan_private_key );
      PUSH_TX( db, trx ); // <-- Claim #1

      // would be legal on its own, but doesn't work because trx already withdrew
      REQUIRE_THROW_WITH_VALUE(op, amount_to_withdraw, asset(5));

      // Make sure we can withdraw again this period, as long as we're not exceeding the periodic limit
      trx.clear();
      // withdraw 1
      trx.operations = {op};
      // make it different from previous trx so it's non-duplicate
      trx.expiration += fc::seconds(1);
      sign( trx, dan_private_key );
      PUSH_TX( db, trx ); // <-- Claim #2
      trx.clear();
   }

   // Account for two (2) claims of one (1) unit
   BOOST_CHECK_EQUAL(get_balance(nathan_id, asset_id_type()), 998);
   BOOST_CHECK_EQUAL(get_balance(dan_id, asset_id_type()), 2);

   {
      const withdraw_permission_object& permit_object = permit(db);
      BOOST_CHECK(permit_object.authorized_account == dan_id);
      BOOST_CHECK(permit_object.withdraw_from_account == nathan_id);
      BOOST_CHECK(permit_object.period_start_time == first_start_time);
      BOOST_CHECK(permit_object.withdrawal_limit == asset(5));
      BOOST_CHECK(permit_object.withdrawal_period_sec == fc::hours(1).to_seconds());
      BOOST_CHECK_EQUAL(permit_object.claimed_this_period.value, 2 ); // <-- Account for two (2) claims of one (1) unit
      BOOST_CHECK(permit_object.expiration == first_start_time + 5*permit_object.withdrawal_period_sec);
      generate_blocks(first_start_time + permit_object.withdrawal_period_sec);
      // lazy update:  verify period_start_time isn't updated until new trx occurs
      BOOST_CHECK(permit_object.period_start_time == first_start_time);
   }

   {
      // Leave Nathan with one unit
      transfer(nathan_id, dan_id, asset(997));

      // Attempt a withdrawal claim for units than available
      withdraw_permission_claim_operation op;
      op.withdraw_permission = permit;
      op.withdraw_from_account = nathan_id;
      op.withdraw_to_account = dan_id;
      op.amount_to_withdraw = asset(5);
      trx.operations.push_back(op);
      set_expiration( db, trx );
      sign( trx, dan_private_key );
      //Throws because nathan doesn't have the money
      GRAPHENE_CHECK_THROW(PUSH_TX( db, trx ), fc::exception);

      // Attempt a withdrawal claim for which nathan does have sufficient units
      op.amount_to_withdraw = asset(1);
      trx.clear();
      trx.operations = {op};
      set_expiration( db, trx );
      sign( trx, dan_private_key );
      PUSH_TX( db, trx );
   }

   BOOST_CHECK_EQUAL(get_balance(nathan_id, asset_id_type()), 0);
   BOOST_CHECK_EQUAL(get_balance(dan_id, asset_id_type()), 1000);
   trx.clear();
   transfer(dan_id, nathan_id, asset(1000));

   {
      const withdraw_permission_object& permit_object = permit(db);
      BOOST_CHECK(permit_object.authorized_account == dan_id);
      BOOST_CHECK(permit_object.withdraw_from_account == nathan_id);
      BOOST_CHECK(permit_object.period_start_time == first_start_time + permit_object.withdrawal_period_sec);
      BOOST_CHECK(permit_object.expiration == first_start_time + 5*permit_object.withdrawal_period_sec);
      BOOST_CHECK(permit_object.withdrawal_limit == asset(5));
      BOOST_CHECK(permit_object.withdrawal_period_sec == fc::hours(1).to_seconds());
      generate_blocks(permit_object.expiration);
   }
   // Ensure the permit object has been garbage collected
   BOOST_CHECK(db.find_object(permit) == nullptr);

   {
      withdraw_permission_claim_operation op;
      op.withdraw_permission = permit;
      op.withdraw_from_account = nathan_id;
      op.withdraw_to_account = dan_id;
      op.amount_to_withdraw = asset(5);
      trx.operations.push_back(op);
      set_expiration( db, trx );
      sign( trx, dan_private_key );
      //Throws because the permission has expired
      GRAPHENE_CHECK_THROW(PUSH_TX( db, trx ), fc::exception);
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( withdraw_permission_nominal_case )
{ try {
   INVOKE(withdraw_permission_create_before_hardfork_23);

   auto nathan_private_key = generate_private_key("nathan");
   auto dan_private_key = generate_private_key("dan");
   account_id_type nathan_id = get_account("nathan").id;
   account_id_type dan_id = get_account("dan").id;
   withdraw_permission_id_type permit;

   // Wait until the permission period's start time
   const withdraw_permission_object& first_permit_object = permit(db);
   generate_blocks(
           first_permit_object.period_start_time);

   // Loop through the withdrawal periods and claim a withdrawal
   while(true)
   {
      const withdraw_permission_object& permit_object = permit(db);
      //wdump( (permit_object) );
      withdraw_permission_claim_operation op;
      op.withdraw_permission = permit;
      op.withdraw_from_account = nathan_id;
      op.withdraw_to_account = dan_id;
      op.amount_to_withdraw = asset(5);
      trx.operations.push_back(op);
      set_expiration( db, trx );
      sign( trx, dan_private_key );
      PUSH_TX( db, trx );
      // tx's involving withdraw_permissions can't delete it even
      // if no further withdrawals are possible
      BOOST_CHECK(db.find_object(permit) != nullptr);
      BOOST_CHECK( permit_object.claimed_this_period == 5 );
      BOOST_CHECK_EQUAL( permit_object.available_this_period(db.head_block_time()).amount.value, 0 );
      BOOST_CHECK_EQUAL( current_period(permit_object, db.head_block_time()).available_this_period.amount.value, 0 );
      trx.clear();
      generate_blocks(
           permit_object.period_start_time
         + permit_object.withdrawal_period_sec );
      if( db.find_object(permit) == nullptr )
         break;
   }

   BOOST_CHECK_EQUAL(get_balance(nathan_id, asset_id_type()), 975);
   BOOST_CHECK_EQUAL(get_balance(dan_id, asset_id_type()), 25);
} FC_LOG_AND_RETHROW() }


/**
 * This case checks to see whether the amount claimed within any particular withdrawal period
 * is properly reflected within the permission object.
 * The maximum withdrawal per period will be limited to 5 units.
 * There are a total of 5 withdrawal periods that are permitted.
 * The test will evaluate the following withdrawal pattern:
 * (1) during Period 1, a withdrawal of 4 units,
 * (2) during Period 2, a withdrawal of 1 units,
 * (3) during Period 3, a withdrawal of 0 units,
 * (4) during Period 4, a withdrawal of 5 units,
 * (5) during Period 5, a withdrawal of 3 units.
 *
 * Total withdrawal will be 13 units.
 */
BOOST_AUTO_TEST_CASE( withdraw_permission_incremental_case )
{ try {
    INVOKE(withdraw_permission_create_after_hardfork_23);
    time_point_sec expected_first_period_start_time = ITWAS_HARDFORK_23_TIME + db.get_global_properties().parameters.block_interval*5; // Hard-coded to synchronize with withdraw_permission_create_after_hardfork_23()
    uint64_t expected_period_duration_seconds = fc::hours(1).to_seconds(); // Hard-coded to synchronize with withdraw_permission_create_after_hardfork_23()

    auto nathan_private_key = generate_private_key("nathan");
    auto dan_private_key = generate_private_key("dan");
    account_id_type nathan_id = get_account("nathan").id;
    account_id_type dan_id = get_account("dan").id;
    withdraw_permission_id_type permit;

    // Wait until the permission period's start time
    {
        const withdraw_permission_object &before_first_permit_object = permit(db);
        BOOST_CHECK_EQUAL(before_first_permit_object.period_start_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch());
        generate_blocks(
                before_first_permit_object.period_start_time);
    }
    // Before withdrawing, check the period description
    const withdraw_permission_object &first_permit_object = permit(db);
    const withdrawal_period_descriptor first_period = current_period(first_permit_object, db.head_block_time());
    BOOST_CHECK_EQUAL(first_period.period_start_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch());
    BOOST_CHECK_EQUAL(first_period.period_end_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + expected_period_duration_seconds);
    BOOST_CHECK_EQUAL(first_period.available_this_period.amount.value, 5);

    // Period 1: Withdraw 4 units
    {
        // Before claiming, check the period description
        const withdraw_permission_object& permit_object = permit(db);
        BOOST_CHECK(db.find_object(permit) != nullptr);
        withdrawal_period_descriptor period_descriptor = current_period(permit_object, db.head_block_time());
        BOOST_CHECK_EQUAL(period_descriptor.available_this_period.amount.value, 5);
        BOOST_CHECK_EQUAL(period_descriptor.period_start_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 0));
        BOOST_CHECK_EQUAL(period_descriptor.period_end_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 1));

        // Claim
        withdraw_permission_claim_operation op;
        op.withdraw_permission = permit;
        op.withdraw_from_account = nathan_id;
        op.withdraw_to_account = dan_id;
        op.amount_to_withdraw = asset(4);
        trx.operations.push_back(op);
        set_expiration( db, trx );
        sign( trx, dan_private_key );
        PUSH_TX( db, trx );

        // After claiming, check the period description
        BOOST_CHECK(db.find_object(permit) != nullptr);
        BOOST_CHECK( permit_object.claimed_this_period == 4 );
        BOOST_CHECK_EQUAL( permit_object.claimed_this_period.value, 4 );
        period_descriptor = current_period(permit_object, db.head_block_time());
        BOOST_CHECK_EQUAL(period_descriptor.available_this_period.amount.value, 1);
        BOOST_CHECK_EQUAL(period_descriptor.period_start_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 0));
        BOOST_CHECK_EQUAL(period_descriptor.period_end_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 1));

        // Advance to next period
        trx.clear();
        generate_blocks(
                permit_object.period_start_time
                + permit_object.withdrawal_period_sec );
    }

    // Period 2: Withdraw 1 units
    {
        // Before claiming, check the period description
        const withdraw_permission_object& permit_object = permit(db);
        BOOST_CHECK(db.find_object(permit) != nullptr);
        withdrawal_period_descriptor period_descriptor = current_period(permit_object, db.head_block_time());
        BOOST_CHECK_EQUAL(period_descriptor.available_this_period.amount.value, 5);
        BOOST_CHECK_EQUAL(period_descriptor.period_start_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 1));
        BOOST_CHECK_EQUAL(period_descriptor.period_end_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 2));

        // Claim
        withdraw_permission_claim_operation op;
        op.withdraw_permission = permit;
        op.withdraw_from_account = nathan_id;
        op.withdraw_to_account = dan_id;
        op.amount_to_withdraw = asset(1);
        trx.operations.push_back(op);
        set_expiration( db, trx );
        sign( trx, dan_private_key );
        PUSH_TX( db, trx );

        // After claiming, check the period description
        BOOST_CHECK(db.find_object(permit) != nullptr);
        BOOST_CHECK( permit_object.claimed_this_period == 1 );
        BOOST_CHECK_EQUAL( permit_object.claimed_this_period.value, 1 );
        period_descriptor = current_period(permit_object, db.head_block_time());
        BOOST_CHECK_EQUAL(period_descriptor.available_this_period.amount.value, 4);
        BOOST_CHECK_EQUAL(period_descriptor.period_start_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 1));
        BOOST_CHECK_EQUAL(period_descriptor.period_end_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 2));

        // Advance to next period
        trx.clear();
        generate_blocks(
                permit_object.period_start_time
                + permit_object.withdrawal_period_sec );
    }

    // Period 3: Withdraw 0 units
    {
        // Before claiming, check the period description
        const withdraw_permission_object& permit_object = permit(db);
        BOOST_CHECK(db.find_object(permit) != nullptr);
        withdrawal_period_descriptor period_descriptor = current_period(permit_object, db.head_block_time());
        BOOST_CHECK_EQUAL(period_descriptor.available_this_period.amount.value, 5);
        BOOST_CHECK_EQUAL(period_descriptor.period_start_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 2));
        BOOST_CHECK_EQUAL(period_descriptor.period_end_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 3));

        // No claim

        // After doing nothing, check the period description
        period_descriptor = current_period(permit_object, db.head_block_time());
        BOOST_CHECK_EQUAL(period_descriptor.available_this_period.amount.value, 5);
        BOOST_CHECK_EQUAL(period_descriptor.period_start_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 2));
        BOOST_CHECK_EQUAL(period_descriptor.period_end_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 3));

        // Advance to end of Period 3
        time_point_sec period_end_time = period_descriptor.period_end_time;
        generate_blocks(period_end_time);
    }

    // Period 4: Withdraw 5 units
    {
        // Before claiming, check the period description
        const withdraw_permission_object& permit_object = permit(db);
        BOOST_CHECK(db.find_object(permit) != nullptr);
        withdrawal_period_descriptor period_descriptor = current_period(permit_object, db.head_block_time());
        BOOST_CHECK_EQUAL(period_descriptor.available_this_period.amount.value, 5);
        BOOST_CHECK_EQUAL(period_descriptor.period_start_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 3));
        BOOST_CHECK_EQUAL(period_descriptor.period_end_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 4));

        // Claim
        withdraw_permission_claim_operation op;
        op.withdraw_permission = permit;
        op.withdraw_from_account = nathan_id;
        op.withdraw_to_account = dan_id;
        op.amount_to_withdraw = asset(5);
        trx.operations.push_back(op);
        set_expiration( db, trx );
        sign( trx, dan_private_key );
        PUSH_TX( db, trx );

        // After claiming, check the period description
        BOOST_CHECK(db.find_object(permit) != nullptr);
        BOOST_CHECK( permit_object.claimed_this_period == 5 );
        BOOST_CHECK_EQUAL( permit_object.claimed_this_period.value, 5 );
        period_descriptor = current_period(permit_object, db.head_block_time());
        BOOST_CHECK_EQUAL(period_descriptor.available_this_period.amount.value, 0);
        BOOST_CHECK_EQUAL(period_descriptor.period_start_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 3));
        BOOST_CHECK_EQUAL(period_descriptor.period_end_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 4));

        // Advance to next period
        trx.clear();
        generate_blocks(
                permit_object.period_start_time
                + permit_object.withdrawal_period_sec );
    }

    // Period 5: Withdraw 3 units
    {
        // Before claiming, check the period description
        const withdraw_permission_object& permit_object = permit(db);
        BOOST_CHECK(db.find_object(permit) != nullptr);
        withdrawal_period_descriptor period_descriptor = current_period(permit_object, db.head_block_time());
        BOOST_CHECK_EQUAL(period_descriptor.available_this_period.amount.value, 5);
        BOOST_CHECK_EQUAL(period_descriptor.period_start_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 4));
        BOOST_CHECK_EQUAL(period_descriptor.period_end_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 5));

        // Claim
        withdraw_permission_claim_operation op;
        op.withdraw_permission = permit;
        op.withdraw_from_account = nathan_id;
        op.withdraw_to_account = dan_id;
        op.amount_to_withdraw = asset(3);
        trx.operations.push_back(op);
        set_expiration( db, trx );
        sign( trx, dan_private_key );
        PUSH_TX( db, trx );

        // After claiming, check the period description
        BOOST_CHECK(db.find_object(permit) != nullptr);
        BOOST_CHECK( permit_object.claimed_this_period == 3 );
        BOOST_CHECK_EQUAL( permit_object.claimed_this_period.value, 3 );
        period_descriptor = current_period(permit_object, db.head_block_time());
        BOOST_CHECK_EQUAL(period_descriptor.available_this_period.amount.value, 2);
        BOOST_CHECK_EQUAL(period_descriptor.period_start_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 4));
        BOOST_CHECK_EQUAL(period_descriptor.period_end_time.sec_since_epoch(), expected_first_period_start_time.sec_since_epoch() + (expected_period_duration_seconds * 5));

        // Advance to next period
        trx.clear();
        generate_blocks(
                permit_object.period_start_time
                + permit_object.withdrawal_period_sec );
    }

    // Withdrawal periods completed
    BOOST_CHECK(db.find_object(permit) == nullptr);

    BOOST_CHECK_EQUAL(get_balance(nathan_id, asset_id_type()), 987);
    BOOST_CHECK_EQUAL(get_balance(dan_id, asset_id_type()), 13);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( withdraw_permission_update )
{ try {
   INVOKE(withdraw_permission_create_before_hardfork_23);

   auto nathan_private_key = generate_private_key("nathan");
   account_id_type nathan_id = get_account("nathan").id;
   account_id_type dan_id = get_account("dan").id;
   withdraw_permission_id_type permit;
   set_expiration( db, trx );

   {
      withdraw_permission_update_operation op;
      op.permission_to_update = permit;
      op.authorized_account = dan_id;
      op.withdraw_from_account = nathan_id;
      op.periods_until_expiration = 2;
      op.period_start_time = db.head_block_time() + 10;
      op.withdrawal_period_sec = 10;
      op.withdrawal_limit = asset(12);
      trx.operations.push_back(op);
      REQUIRE_THROW_WITH_VALUE(op, periods_until_expiration, 0);
      REQUIRE_THROW_WITH_VALUE(op, withdrawal_period_sec, 0);
      REQUIRE_THROW_WITH_VALUE(op, withdrawal_limit, asset(1, asset_id_type(12)));
      REQUIRE_THROW_WITH_VALUE(op, withdrawal_limit, asset(0));
      REQUIRE_THROW_WITH_VALUE(op, withdraw_from_account, account_id_type(0));
      REQUIRE_THROW_WITH_VALUE(op, authorized_account, account_id_type(0));
      REQUIRE_THROW_WITH_VALUE(op, period_start_time, db.head_block_time() - 50);
      trx.operations.back() = op;
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
   }

   {
      const withdraw_permission_object& permit_object = db.get(permit);
      BOOST_CHECK(permit_object.authorized_account == dan_id);
      BOOST_CHECK(permit_object.withdraw_from_account == nathan_id);
      BOOST_CHECK(permit_object.period_start_time == db.head_block_time() + 10);
      BOOST_CHECK(permit_object.withdrawal_limit == asset(12));
      BOOST_CHECK(permit_object.withdrawal_period_sec == 10);
      // BOOST_CHECK(permit_object.remaining_periods == 2);
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( withdraw_permission_delete )
{ try {
   INVOKE(withdraw_permission_update);

   withdraw_permission_delete_operation op;
   op.authorized_account = get_account("dan").id;
   op.withdraw_from_account = get_account("nathan").id;
   set_expiration( db, trx );
   trx.operations.push_back(op);
   sign( trx, generate_private_key("nathan" ));
   PUSH_TX( db, trx );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( witness_create )
{ try {
   ACTOR(nathan);
   upgrade_to_lifetime_member(nathan_id);
   trx.clear();
   witness_id_type nathan_witness_id = create_witness(nathan_id, nathan_private_key).id;
   // Give nathan some voting stake
   transfer(committee_account, nathan_id, asset(10000000));
   generate_block();
   set_expiration( db, trx );

   {
      account_update_operation op;
      op.account = nathan_id;
      op.new_options = nathan_id(db).options;
      op.new_options->votes.insert(nathan_witness_id(db).vote_id);
      op.new_options->num_witness = std::count_if(op.new_options->votes.begin(), op.new_options->votes.end(),
                                                  [](vote_id_type id) { return id.type() == vote_id_type::witness; });
      op.new_options->num_committee = std::count_if(op.new_options->votes.begin(), op.new_options->votes.end(),
                                                    [](vote_id_type id) { return id.type() == vote_id_type::committee; });
      trx.operations.push_back(op);
      sign( trx, nathan_private_key );
      PUSH_TX( db, trx );
      trx.clear();
   }

   generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
   const auto& witnesses = db.get_global_properties().active_witnesses;

   // make sure we're in active_witnesses
   auto itr = std::find(witnesses.begin(), witnesses.end(), nathan_witness_id);
   BOOST_CHECK(itr != witnesses.end());

   // generate blocks until we are at the beginning of a round
   while( ((db.get_dynamic_global_properties().current_aslot + 1) % witnesses.size()) != 0 )
      generate_block();

   int produced = 0;
   // Make sure we get scheduled at least once in witnesses.size()*2 blocks
   // may take this many unless we measure where in the scheduling round we are
   // TODO:  intense_test that repeats this loop many times
   for( size_t i=0, n=witnesses.size()*2; i<n; i++ )
   {
      signed_block block = generate_block();
      if( block.witness == nathan_witness_id )
         produced++;
   }
   BOOST_CHECK_GE( produced, 1 );
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE( assert_op_test )
{
   try {
   // create some objects
   auto nathan_private_key = generate_private_key("nathan");
   public_key_type nathan_public_key = nathan_private_key.get_public_key();
   account_id_type nathan_id = create_account("nathan", nathan_public_key).id;

   assert_operation op;

   // nathan checks that his public key is equal to the given value.
   op.fee_paying_account = nathan_id;
   op.predicates.emplace_back(account_name_eq_lit_predicate{ nathan_id, "nathan" });
   trx.operations.push_back(op);
   sign( trx, nathan_private_key );
   PUSH_TX( db, trx );

   // nathan checks that his public key is not equal to the given value (fail)
   trx.clear();
   op.predicates.emplace_back(account_name_eq_lit_predicate{ nathan_id, "dan" });
   trx.operations.push_back(op);
   sign( trx, nathan_private_key );
   GRAPHENE_CHECK_THROW( PUSH_TX( db, trx ), fc::exception );
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( balance_object_test )
{ try {
   // Intentionally overriding the fixture's db; I need to control genesis on this one.
   database db;
   const uint32_t skip_flags = database::skip_undo_history_check;
   fc::temp_directory td( graphene::utilities::temp_directory_path() );
   genesis_state.initial_balances.push_back({generate_private_key("n").get_public_key(), GRAPHENE_SYMBOL, 1});
   genesis_state.initial_balances.push_back({generate_private_key("x").get_public_key(), GRAPHENE_SYMBOL, 1});
   fc::time_point_sec starting_time = genesis_state.initial_timestamp + 3000;

   auto n_key = generate_private_key("n");
   auto x_key = generate_private_key("x");
   auto v1_key = generate_private_key("v1");
   auto v2_key = generate_private_key("v2");

   genesis_state_type::initial_vesting_balance_type vest;
   vest.owner = v1_key.get_public_key();
   vest.asset_symbol = GRAPHENE_SYMBOL;
   vest.amount = 500;
   vest.begin_balance = vest.amount;
   vest.begin_timestamp = starting_time;
   vest.vesting_duration_seconds = 60;
   genesis_state.initial_vesting_balances.push_back(vest);
   vest.owner = v2_key.get_public_key();
   vest.begin_timestamp -= fc::seconds(30);
   vest.amount = 400;
   genesis_state.initial_vesting_balances.push_back(vest);

   genesis_state.initial_accounts.emplace_back("n", n_key.get_public_key());

   auto _sign = [&]( signed_transaction& tx, const private_key_type& key )
   {  tx.sign( key, db.get_chain_id() );   };

   db.open(td.path(), [this]{return genesis_state;}, "TEST");
   const balance_object& balance = balance_id_type()(db);
   BOOST_CHECK_EQUAL(balance.balance.amount.value, 1);
   BOOST_CHECK_EQUAL(balance_id_type(1)(db).balance.amount.value, 1);

   balance_claim_operation op;
   op.deposit_to_account = db.get_index_type<account_index>().indices().get<by_name>().find("n")->get_id();
   op.total_claimed = asset(1);
   op.balance_to_claim = balance_id_type(1);
   op.balance_owner_key = x_key.get_public_key();
   trx.operations = {op};
   _sign( trx, n_key );
   // Fail because I'm claiming from an address which hasn't signed
   GRAPHENE_CHECK_THROW(db.push_transaction(trx), tx_missing_other_auth);
   trx.clear();
   op.balance_to_claim = balance_id_type();
   op.balance_owner_key = n_key.get_public_key();
   trx.operations = {op};
   _sign( trx, n_key );
   db.push_transaction(trx);

   // Not using fixture's get_balance() here because it uses fixture's db, not my override
   BOOST_CHECK_EQUAL(db.get_balance(op.deposit_to_account, asset_id_type()).amount.value, 1);
   BOOST_CHECK(db.find_object(balance_id_type()) == nullptr);
   BOOST_CHECK(db.find_object(balance_id_type(1)) != nullptr);

   auto slot = db.get_slot_at_time(starting_time);
   db.generate_block(starting_time, db.get_scheduled_witness(slot), init_account_priv_key, skip_flags);
   set_expiration( db, trx );

   const balance_object& vesting_balance_1 = balance_id_type(2)(db);
   const balance_object& vesting_balance_2 = balance_id_type(3)(db);
   BOOST_CHECK(vesting_balance_1.is_vesting_balance());
   BOOST_CHECK_EQUAL(vesting_balance_1.balance.amount.value, 500);
   BOOST_CHECK_EQUAL(vesting_balance_1.available(db.head_block_time()).amount.value, 0);
   BOOST_CHECK(vesting_balance_2.is_vesting_balance());
   BOOST_CHECK_EQUAL(vesting_balance_2.balance.amount.value, 400);
   BOOST_CHECK_EQUAL(vesting_balance_2.available(db.head_block_time()).amount.value, 150);

   op.balance_to_claim = vesting_balance_1.id;
   op.total_claimed = asset(1);
   op.balance_owner_key = v1_key.get_public_key();
   trx.clear();
   trx.operations = {op};
   _sign( trx, n_key );
   _sign( trx, v1_key );
   // Attempting to claim 1 from a balance with 0 available
   GRAPHENE_CHECK_THROW(db.push_transaction(trx), balance_claim_invalid_claim_amount);

   op.balance_to_claim = vesting_balance_2.id;
   op.total_claimed.amount = 151;
   op.balance_owner_key = v2_key.get_public_key();
   trx.operations = {op};
   trx.signatures.clear();
   _sign( trx, n_key );
   _sign( trx, v2_key );
   // Attempting to claim 151 from a balance with 150 available
   GRAPHENE_CHECK_THROW(db.push_transaction(trx), balance_claim_invalid_claim_amount);

   op.balance_to_claim = vesting_balance_2.id;
   op.total_claimed.amount = 100;
   op.balance_owner_key = v2_key.get_public_key();
   trx.operations = {op};
   trx.signatures.clear();
   _sign( trx, n_key );
   _sign( trx, v2_key );
   db.push_transaction(trx);
   BOOST_CHECK_EQUAL(db.get_balance(op.deposit_to_account, asset_id_type()).amount.value, 101);
   BOOST_CHECK_EQUAL(vesting_balance_2.balance.amount.value, 300);

   op.total_claimed.amount = 10;
   trx.operations = {op};
   trx.signatures.clear();
   _sign( trx, n_key );
   _sign( trx, v2_key );
   // Attempting to claim twice within a day
   GRAPHENE_CHECK_THROW(db.push_transaction(trx), balance_claim_claimed_too_often);

   db.generate_block(db.get_slot_time(1), db.get_scheduled_witness(1), init_account_priv_key, skip_flags);
   slot = db.get_slot_at_time(vesting_balance_1.vesting_policy->begin_timestamp + 60);
   db.generate_block(db.get_slot_time(slot), db.get_scheduled_witness(slot), init_account_priv_key, skip_flags);
   set_expiration( db, trx );

   op.balance_to_claim = vesting_balance_1.id;
   op.total_claimed.amount = 500;
   op.balance_owner_key = v1_key.get_public_key();
   trx.operations = {op};
   trx.signatures.clear();
   _sign( trx, n_key );
   _sign( trx, v1_key );
   db.push_transaction(trx);
   BOOST_CHECK(db.find_object(op.balance_to_claim) == nullptr);
   BOOST_CHECK_EQUAL(db.get_balance(op.deposit_to_account, asset_id_type()).amount.value, 601);

   op.balance_to_claim = vesting_balance_2.id;
   op.balance_owner_key = v2_key.get_public_key();
   op.total_claimed.amount = 10;
   trx.operations = {op};
   trx.signatures.clear();
   _sign( trx, n_key );
   _sign( trx, v2_key );
   // Attempting to claim twice within a day
   GRAPHENE_CHECK_THROW(db.push_transaction(trx), balance_claim_claimed_too_often);

   db.generate_block(db.get_slot_time(1), db.get_scheduled_witness(1), init_account_priv_key, skip_flags);
   slot = db.get_slot_at_time(db.head_block_time() + fc::days(1));
   db.generate_block(db.get_slot_time(slot), db.get_scheduled_witness(slot), init_account_priv_key, skip_flags);
   set_expiration( db, trx );

   op.total_claimed = vesting_balance_2.balance;
   trx.operations = {op};
   trx.signatures.clear();
   _sign( trx, n_key );
   _sign( trx, v2_key );
   db.push_transaction(trx);
   BOOST_CHECK(db.find_object(op.balance_to_claim) == nullptr);
   BOOST_CHECK_EQUAL(db.get_balance(op.deposit_to_account, asset_id_type()).amount.value, 901);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(transfer_with_memo) {
   try {
      ACTOR(alice);
      ACTOR(bob);
      transfer(account_id_type(), alice_id, asset(1000));
      BOOST_CHECK_EQUAL(get_balance(alice_id, asset_id_type()), 1000);

      transfer_operation op;
      op.from = alice_id;
      op.to = bob_id;
      op.amount = asset(500);
      op.memo = memo_data();
      op.memo->set_message(alice_private_key, bob_public_key, "Dear Bob,\n\nMoney!\n\nLove, Alice");
      trx.operations = {op};
      trx.sign(alice_private_key, db.get_chain_id());
      db.push_transaction(trx);

      BOOST_CHECK_EQUAL(get_balance(alice_id, asset_id_type()), 500);
      BOOST_CHECK_EQUAL(get_balance(bob_id, asset_id_type()), 500);

      auto memo = db.get_recent_transaction(trx.id()).operations.front().get<transfer_operation>().memo;
      BOOST_CHECK(memo);
      BOOST_CHECK_EQUAL(memo->get_message(bob_private_key, alice_public_key), "Dear Bob,\n\nMoney!\n\nLove, Alice");
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(zero_second_vbo)
{
   try
   {
      ACTOR(alice);
      // don't pay witnesses so we have some worker budget to work with

      transfer(account_id_type(), alice_id, asset(int64_t(100000) * 1100 * 1000 * 1000));
      {
         asset_reserve_operation op;
         op.payer = alice_id;
         op.amount_to_reserve = asset(int64_t(100000) * 1000 * 1000 * 1000);
         transaction tx;
         tx.operations.push_back( op );
         set_expiration( db, tx );
         db.push_transaction( tx, database::skip_authority_check | database::skip_tapos_check | database::skip_transaction_signatures );
      }
      enable_fees();
      upgrade_to_lifetime_member(alice_id);
      generate_block();

      // Wait for a maintenance interval to ensure we have a full day's budget to work with.
      // Otherwise we may not have enough to feed the witnesses and the worker will end up starved if we start late in the day.
      generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
      generate_block();

      auto check_vesting_1b = [&](vesting_balance_id_type vbid)
      {
         // this function checks that Alice can't draw any right now,
         // but one block later, she can withdraw it all.

         vesting_balance_withdraw_operation withdraw_op;
         withdraw_op.vesting_balance = vbid;
         withdraw_op.owner = alice_id;
         withdraw_op.amount = asset(1);

         signed_transaction withdraw_tx;
         withdraw_tx.operations.push_back( withdraw_op );
         sign(withdraw_tx, alice_private_key);
         GRAPHENE_REQUIRE_THROW( PUSH_TX( db, withdraw_tx ), fc::exception );

         generate_block();
         withdraw_tx = signed_transaction();
         withdraw_op.amount = asset(500);
         withdraw_tx.operations.push_back( withdraw_op );
         set_expiration( db, withdraw_tx );
         sign(withdraw_tx, alice_private_key);
         PUSH_TX( db, withdraw_tx );
      };

      // This block creates a zero-second VBO with a vesting_balance_create_operation.
      {
         cdd_vesting_policy_initializer pinit;
         pinit.vesting_seconds = 0;

         vesting_balance_create_operation create_op;
         create_op.creator = alice_id;
         create_op.owner = alice_id;
         create_op.amount = asset(500);
         create_op.policy = pinit;

         signed_transaction create_tx;
         create_tx.operations.push_back( create_op );
         set_expiration( db, create_tx );
         sign(create_tx, alice_private_key);

         processed_transaction ptx = PUSH_TX( db, create_tx );
         vesting_balance_id_type vbid = ptx.operation_results[0].get<object_id_type>();
         check_vesting_1b( vbid );
      }
   } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( vbo_withdraw_different )
{
   try
   {
      ACTORS((alice)(izzy));
      // don't pay witnesses so we have some worker budget to work with

      // transfer(account_id_type(), alice_id, asset(1000));

      asset_id_type stuff_id = create_user_issued_asset( "STUFF", izzy_id(db), 0 ).id;
      issue_uia( alice_id, asset( 1000, stuff_id ) );

      // deposit STUFF with linear vesting policy
      vesting_balance_id_type vbid;
      {
         linear_vesting_policy_initializer pinit;
         pinit.begin_timestamp = db.head_block_time();
         pinit.vesting_cliff_seconds    = 30;
         pinit.vesting_duration_seconds = 30;

         vesting_balance_create_operation create_op;
         create_op.creator = alice_id;
         create_op.owner = alice_id;
         create_op.amount = asset(100, stuff_id);
         create_op.policy = pinit;

         signed_transaction create_tx;
         create_tx.operations.push_back( create_op );
         set_expiration( db, create_tx );
         sign(create_tx, alice_private_key);

         processed_transaction ptx = PUSH_TX( db, create_tx );
         vbid = ptx.operation_results[0].get<object_id_type>();
      }

      // wait for VB to mature
      generate_blocks( 30 );

      BOOST_CHECK( vbid(db).get_allowed_withdraw( db.head_block_time() ) == asset(100, stuff_id) );

      // bad withdrawal op (wrong asset)
      {
         vesting_balance_withdraw_operation op;

         op.vesting_balance = vbid;
         op.amount = asset(100);
         op.owner = alice_id;

         signed_transaction withdraw_tx;
         withdraw_tx.operations.push_back(op);
         set_expiration( db, withdraw_tx );
         sign( withdraw_tx, alice_private_key );
         GRAPHENE_CHECK_THROW( PUSH_TX( db, withdraw_tx ), fc::exception );
      }

      // good withdrawal op
      {
         vesting_balance_withdraw_operation op;

         op.vesting_balance = vbid;
         op.amount = asset(100, stuff_id);
         op.owner = alice_id;

         signed_transaction withdraw_tx;
         withdraw_tx.operations.push_back(op);
         set_expiration( db, withdraw_tx );
         sign( withdraw_tx, alice_private_key );
         PUSH_TX( db, withdraw_tx );
      }
   }
   FC_LOG_AND_RETHROW()
}

#define ITWAS_HARDFORK_516_TIME (fc::time_point_sec( 1456250400 ))
#define ITWAS_HARDFORK_599_TIME (fc::time_point_sec( 1459789200 ))

// TODO:  Write linear VBO tests

BOOST_AUTO_TEST_CASE( top_n_special )
{
   ACTORS( (alice)(bob)(chloe)(dan)(izzy)(stan) );

   generate_blocks( ITWAS_HARDFORK_516_TIME );
   generate_blocks( ITWAS_HARDFORK_599_TIME );

   try
   {
      {
         //
         // Izzy (issuer)
         // Stan (special authority)
         // Alice, Bob, Chloe, Dan (ABCD)
         //

         asset_id_type topn_id = create_user_issued_asset( "TOPN", izzy_id(db), 0 ).id;
         authority stan_owner_auth = stan_id(db).owner;
         authority stan_active_auth = stan_id(db).active;

         // set SA, wait for maint interval
         // TODO:  account_create_operation
         // TODO:  multiple accounts with different n for same asset

         {
            top_holders_special_authority top2, top3;

            top2.num_top_holders = 2;
            top2.asset = topn_id;

            top3.num_top_holders = 3;
            top3.asset = topn_id;

            account_update_operation op;
            op.account = stan_id;
            op.extensions.value.active_special_authority = top3;
            op.extensions.value.owner_special_authority = top2;

            signed_transaction tx;
            tx.operations.push_back( op );

            set_expiration( db, tx );
            sign( tx, stan_private_key );

            PUSH_TX( db, tx );

            // TODO:  Check special_authority is properly set
            // TODO:  Do it in steps
         }

         // wait for maint interval
         // make sure we don't have any authority as account hasn't gotten distributed yet
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         BOOST_CHECK( stan_id(db).owner  == stan_owner_auth );
         BOOST_CHECK( stan_id(db).active == stan_active_auth );

         // issue some to Alice, make sure she gets control of Stan

         // we need to set_expiration() before issue_uia() because the latter doens't call it #11
         set_expiration( db, trx );  // #11
         issue_uia( alice_id, asset( 1000, topn_id ) );

         BOOST_CHECK( stan_id(db).owner  == stan_owner_auth );
         BOOST_CHECK( stan_id(db).active == stan_active_auth );

         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         /*  NOTE - this was an old check from an earlier implementation that only allowed SA for LTM's
         // no boost yet, we need to upgrade to LTM before mechanics apply to Stan
         BOOST_CHECK( stan_id(db).owner  == stan_owner_auth );
         BOOST_CHECK( stan_id(db).active == stan_active_auth );

         set_expiration( db, trx );  // #11
         upgrade_to_lifetime_member(stan_id);
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);
         */

         BOOST_CHECK( stan_id(db).owner  == authority(  501, alice_id, 1000 ) );
         BOOST_CHECK( stan_id(db).active == authority(  501, alice_id, 1000 ) );

         // give asset to Stan, make sure owner doesn't change at all
         set_expiration( db, trx );  // #11
         transfer( alice_id, stan_id, asset( 1000, topn_id ) );

         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         BOOST_CHECK( stan_id(db).owner  == authority(  501, alice_id, 1000 ) );
         BOOST_CHECK( stan_id(db).active == authority(  501, alice_id, 1000 ) );

         set_expiration( db, trx );  // #11
         issue_uia( chloe_id, asset( 131000, topn_id ) );

         // now Chloe has 131,000 and Stan has 1k.  Make sure change occurs at next maintenance interval.
         // NB, 131072 is a power of 2; the number 131000 was chosen so that we need a bitshift, but
         // if we put the 1000 from Stan's balance back into play, we need a different bitshift.

         // we use Chloe so she can be displaced by Bob later (showing the tiebreaking logic).

         // Check Alice is still in control, because we're deferred to next maintenance interval
         BOOST_CHECK( stan_id(db).owner  == authority(  501, alice_id, 1000 ) );
         BOOST_CHECK( stan_id(db).active == authority(  501, alice_id, 1000 ) );

         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         BOOST_CHECK( stan_id(db).owner  == authority( 32751, chloe_id, 65500 ) );
         BOOST_CHECK( stan_id(db).active == authority( 32751, chloe_id, 65500 ) );

         // put Alice's stake back in play
         set_expiration( db, trx );  // #11
         transfer( stan_id, alice_id, asset( 1000, topn_id ) );

         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         BOOST_CHECK( stan_id(db).owner  == authority( 33001, alice_id, 500, chloe_id, 65500 ) );
         BOOST_CHECK( stan_id(db).active == authority( 33001, alice_id, 500, chloe_id, 65500 ) );

         // issue 200,000 to Dan to cause another bitshift.
         set_expiration( db, trx );  // #11
         issue_uia( dan_id, asset( 200000, topn_id ) );
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         // 200000 Dan
         // 131000 Chloe
         // 1000 Alice

         BOOST_CHECK( stan_id(db).owner  == authority( 41376,                chloe_id, 32750, dan_id, 50000 ) );
         BOOST_CHECK( stan_id(db).active == authority( 41501, alice_id, 250, chloe_id, 32750, dan_id, 50000 ) );

         // have Alice send all but 1 back to Stan, verify that we clamp Alice at one vote
         set_expiration( db, trx );  // #11
         transfer( alice_id, stan_id, asset( 999, topn_id ) );
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         BOOST_CHECK( stan_id(db).owner  == authority( 41376,                chloe_id, 32750, dan_id, 50000 ) );
         BOOST_CHECK( stan_id(db).active == authority( 41376, alice_id,   1, chloe_id, 32750, dan_id, 50000 ) );

         // send 131k to Bob so he's tied with Chloe, verify he displaces Chloe in top2
         set_expiration( db, trx );  // #11
         issue_uia( bob_id, asset( 131000, topn_id ) );
         generate_blocks(db.get_dynamic_global_properties().next_maintenance_time);

         BOOST_CHECK( stan_id(db).owner  == authority( 41376, bob_id, 32750,                  dan_id, 50000 ) );
         BOOST_CHECK( stan_id(db).active == authority( 57751, bob_id, 32750, chloe_id, 32750, dan_id, 50000 ) );

         // TODO more rounding checks
      }

   } FC_LOG_AND_RETHROW()
}

#define ITWAS_HARDFORK_538_TIME (fc::time_point_sec( 1456250400 ))
#define ITWAS_HARDFORK_555_TIME (fc::time_point_sec( 1456250400 ))

BOOST_AUTO_TEST_SUITE_END()
