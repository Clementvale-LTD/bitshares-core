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
#include <graphene/chain/protocol/memo.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>

namespace graphene { namespace chain {

void memo_data::set_message(const fc::ecc::private_key& priv, const fc::ecc::public_key& pub,
                            const string& msg, uint64_t custom_nonce)
{
   if( priv != fc::ecc::private_key() && public_key_type(pub) != public_key_type() )
   {
      from = priv.get_public_key();
      to = pub;
      if( custom_nonce == 0 )
      {
         uint64_t entropy = fc::sha224::hash(fc::ecc::private_key::generate())._hash[0];
         entropy <<= 32;
         entropy                                                     &= 0xff00000000000000;
         nonce = (fc::time_point::now().time_since_epoch().count()   &  0x00ffffffffffffff) | entropy;
      } else
         nonce = custom_nonce;
      auto secret = priv.get_shared_secret(pub);
      auto nonce_plus_secret = fc::sha512::hash(fc::to_string(nonce) + secret.str());
      string text = memo_message(digest_type::hash(msg)._hash[0], msg).serialize();
      message = fc::aes_encrypt( nonce_plus_secret, vector<char>(text.begin(), text.end()) );
   }
   else
   {
      auto text = memo_message(0, msg).serialize();
      message = vector<char>(text.begin(), text.end());
   }
}

string memo_data::get_message(const fc::ecc::private_key& priv,
                              const fc::ecc::public_key& pub)const
{
   if( from != public_key_type() )
   {
      auto secret = priv.get_shared_secret(pub);
      auto nonce_plus_secret = fc::sha512::hash(fc::to_string(nonce) + secret.str());
      auto plain_text = fc::aes_decrypt( nonce_plus_secret, message );
      auto result = memo_message::deserialize(string(plain_text.begin(), plain_text.end()));
      FC_ASSERT( result.checksum == uint32_t(digest_type::hash(result.text)._hash[0]) );
      return result.text;
   }
   else
   {
      return memo_message::deserialize(string(message.begin(), message.end())).text;
   }
}

string memo_message::serialize() const
{
   auto serial_checksum = string(sizeof(checksum), ' ');
   (uint32_t&)(*serial_checksum.data()) = checksum;
   return serial_checksum + text;
}

memo_message memo_message::deserialize(const string& serial)
{
   memo_message result;
   FC_ASSERT( serial.size() >= sizeof(result.checksum) );
   result.checksum = ((uint32_t&)(*serial.data()));
   result.text = serial.substr(sizeof(result.checksum));
   return result;
}

void memo_group::set_message(const fc::ecc::private_key& priv, const vector<fc::ecc::public_key>& pubs,
                             const string& msg, uint64_t custom_nonce)
{
   if( priv != fc::ecc::private_key() && !pubs.empty() )
   {
      from = priv.get_public_key();
      if( custom_nonce == 0 )
      {
         uint64_t entropy = fc::sha224::hash(fc::ecc::private_key::generate())._hash[0];
         entropy <<= 32;
         entropy                                                     &= 0xff00000000000000;
         nonce = (fc::time_point::now().time_since_epoch().count()   &  0x00ffffffffffffff) | entropy;
      } else
         nonce = custom_nonce;

      auto s1 = fc::ecc::private_key::generate();
      auto s2 = fc::time_point::now().time_since_epoch().count();
      auto s3 = fc::ecc::private_key::generate();

      auto aes_secret = fc::sha512::hash( s1.get_secret().str() + fc::to_string(s2) + s3.get_secret().str() );

      string text = memo_message(digest_type::hash(msg)._hash[0], msg).serialize();
      message = fc::aes_encrypt( aes_secret, vector<char>(text.begin(), text.end()) );
      
      for( const auto& pub : pubs ){
        auto secret = priv.get_shared_secret(pub);
        auto nonce_plus_secret = fc::sha512::hash(fc::to_string(nonce) + secret.str());
        to_ekey r;
        r.to = pub;
        r.ekey = fc::aes_encrypt( nonce_plus_secret, vector<char>( aes_secret.data(), aes_secret.data() + aes_secret.data_size() ) );
        gto.push_back( r);
     }
   }
   else
   {
      auto text = memo_message(0, msg).serialize();
      message = vector<char>(text.begin(), text.end());
   }
}

bool memo_group::try_get_message( const fc::ecc::private_key& priv, std::string& outmsg) const
{
   if( from != public_key_type() )
   {
      try{

        if( priv == fc::ecc::private_key() )
          return false;

        fc::ecc::public_key mypub = priv.get_public_key();

        to_ekey euse;
        if( from == mypub){
          euse = gto[0];
        }else{
          for( const to_ekey& r : gto ){
            if( r.to == mypub){
              euse = r;
              break;
            }
          }
        }

        if( euse.to != public_key_type() ){
          auto secret = priv.get_shared_secret( euse.to);
          auto nonce_plus_secret = fc::sha512::hash(fc::to_string(nonce) + secret.str());
          vector<char> aes_secret_raw = fc::aes_decrypt( nonce_plus_secret, euse.ekey );
          fc::sha512 aes_secret( aes_secret_raw.data(), aes_secret_raw.size() );

          auto plain_text = fc::aes_decrypt( aes_secret, message );
          auto result = memo_message::deserialize(string(plain_text.begin(), plain_text.end()));
          FC_ASSERT( result.checksum == uint32_t(digest_type::hash(result.text)._hash[0]) );
          outmsg = result.text;
          return true;
        }
      }
      catch (const fc::exception&)
      {}
      catch (const std::exception&)
      {}
 
      return false;
   } else {
      outmsg = memo_message::deserialize(string(message.begin(), message.end())).text;
      return true;
   }
}

} } // graphene::chain
