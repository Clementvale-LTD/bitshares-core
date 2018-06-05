/*
 * Copyright (c) 2017 Cryptonomex, Inc., and contributors.
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
#include <graphene/chain/market_object.hpp>

#include <graphene/app/database_api.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace graphene::chain;
using namespace graphene::chain::test;

namespace graphene { namespace chain {

struct swan_fixture : database_fixture {

    void standard_users() {
        set_expiration( db, trx );
        ACTORS((borrower)(borrower2)(feedproducer));
        _borrower = borrower_id;
        _borrower2 = borrower2_id;
        _feedproducer = feedproducer_id;

        transfer(committee_account, borrower_id, asset(init_balance));
        transfer(committee_account, borrower2_id, asset(init_balance));
    }

    void standard_asset() {
        set_expiration( db, trx );
        const auto& bitusd = create_bitasset("USDBIT", _feedproducer);
        _swan = bitusd.id;
        _back = asset_id_type();
        update_feed_producers(swan(), {_feedproducer});
    }

    #define ITWAS_HARDFORK_CORE_216_TIME (fc::time_point_sec( 1512747600 ))

    void set_feed(share_type usd, share_type core) {
        price_feed feed;
        feed.maintenance_collateral_ratio = 1750; // need to set this explicitly, testnet has a different default
        feed.settlement_price = swan().amount(usd) / back().amount(core);
        publish_feed(swan(), feedproducer(), feed);
    }

    void expire_feed() {
      generate_blocks(db.head_block_time() + GRAPHENE_DEFAULT_PRICE_FEED_LIFETIME);
      generate_block();
      FC_ASSERT( swan().bitasset_data(db).current_feed.settlement_price.is_null() );
    }

    void wait_for_hf_core_216() {
      generate_blocks( ITWAS_HARDFORK_CORE_216_TIME );
      generate_block();
    }

    void wait_for_maintenance() {
      generate_blocks( db.get_dynamic_global_properties().next_maintenance_time );
      generate_block();
    }

    const account_object& borrower() { return _borrower(db); }
    const account_object& borrower2() { return _borrower2(db); }
    const account_object& feedproducer() { return _feedproducer(db); }
    const asset_object& swan() { return _swan(db); }
    const asset_object& back() { return _back(db); }

    int64_t init_balance = 1000000;
    account_id_type _borrower, _borrower2, _feedproducer;
    asset_id_type _swan, _back;
};

}}

BOOST_FIXTURE_TEST_SUITE( swan_tests, swan_fixture )

BOOST_AUTO_TEST_SUITE_END()
