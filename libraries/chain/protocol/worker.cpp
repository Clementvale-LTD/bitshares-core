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
#include <graphene/chain/protocol/worker.hpp>

namespace graphene { namespace chain {

bool is_valid_srv_name( const string& sname )
{
    if( sname.size() < GRAPHENE_MIN_ASSET_SYMBOL_LENGTH )
        return false;

    if( sname.size() > GRAPHENE_MAX_ASSET_SYMBOL_LENGTH )
        return false;

    if( !isalpha( sname.front() ) )
        return false;

    for( const auto c : sname )
    {
        if( (isalpha( c ) && islower( c )) || isdigit(c) )
            continue;

        if( c == '.' || c == '-' || c == '_'  )
            continue;

        return false;
    }

    return true;
}
  

void  service_create_operation::validate()const
{
   FC_ASSERT( is_valid_srv_name( name) );
   FC_ASSERT( fee.amount >= 0 );
}
  
void  service_update_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}

void  bid_request_create_operation::validate()const
{
   FC_ASSERT( is_valid_srv_name( name) );
   FC_ASSERT( fee.amount >= 0 );
}

void  bid_request_cancel_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}

void  bid_create_operation::validate()const
{
   FC_ASSERT( is_valid_srv_name( name) );
   FC_ASSERT( fee.amount >= 0 );
}

void  bid_cancel_operation::validate()const
{
   FC_ASSERT( fee.amount >= 0 );
}

} }
