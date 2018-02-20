# -*- coding: utf-8 -*-
from __future__ import print_function
from node_lib import *
import json
import os.path

NODE_FILE_PRIVATE = "node_private_keep_in_secret.json"
NODE_FILE_PUBLIC  = "node_puplic.json"

def prepare_account_for_registration():

  node_data_private = \
  {
    "name": "",
    "url": "",
    "owner_key": {
      "brain_priv_key": "",
      "wif_priv_key": "",
      "pub_key": ""
    },
    "witness_key": {
      "brain_priv_key": "",
      "wif_priv_key": "",
      "pub_key": ""
    }
  }

  # template for JSON call
  jsonrpc_call_s = '{"jsonrpc": "2.0", "method": "", "params": [], "id": 1}'

  # parsed JSON template
  jsonrpc_call = json.loads(jsonrpc_call_s)

  print( "Checking cli_wallet...")
  jsonrpc_call["method"] = "about"
  jsonrpc_call["params"] = []
  call_cli_wallet( jsonrpc_call )

  print( "cli_wallet is present and running")
  print( "")
  print( "This tool will prepare public and private files necessary to register your new account")
  print( "Follow instruction and supply necessary information")
  print( "")

  if os.path.isfile( NODE_FILE_PRIVATE):
    print( "\'", NODE_FILE_PRIVATE,  "\' file already exists.", sep="")
    print( "This file contains essential account information, such as private and public keys.")
    print( "Ensure that you backup this file for all your actual accounts.")
    print( "If you continue this file will be overwritten and you may lose access to your account.")
    print( "Do you wish to continue? [Y/N]")
    user_confirm = raw_input(">>>")
    if not ('Y'==user_confirm[0] or 'y'==user_confirm[0]):
      exit(0)

  while True:
    print( "")
    print( "Type in your account name")
    print( "  A valid name consists of a dot-separated sequence" )
    print( "  of one or more labels consisting of the following rules:" )
    print( "   - Each label is three characters or more" )
    print( "   - Each label begins with a letter" )
    print( "   - Each label ends with a letter or digit" )
    print( "   - Each label contains only letters, digits or hyphens" )
    print( "   - All letters are lowercase" )
    print( "   - Length is between (inclusive) GRAPHENE_MIN_ACCOUNT_NAME_LENGTH and GRAPHENE_MAX_ACCOUNT_NAME_LENGTH" )
    in_name = raw_input(">>>")

    print( "")
    print( "Type in web site associated with account")
    in_url = raw_input(">>>")

    print( "")
    print( "Check data you typed:")
    print( "account name    :", in_name)
    print( "account web site:", in_url)
    print( "")

    print( "Is information above correct? [Y/N]")
    user_confirm = raw_input(">>>")
    if 'Y'==user_confirm[0] or 'y'==user_confirm[0]:
      break

  node_data_private["name"] = in_name
  node_data_private["url"]  = in_url

  print( "")
  print( "Creating private and public keys for your account...")
  print( "")

  jsonrpc_call["method"] = "suggest_brain_key"
  jsonrpc_call["params"] = []
  cli_response = call_cli_wallet( jsonrpc_call )
  node_data_private["owner_key"] = cli_response["result"]

  jsonrpc_call["method"] = "suggest_brain_key"
  jsonrpc_call["params"] = []
  cli_response = call_cli_wallet( jsonrpc_call )
  node_data_private["witness_key"] = cli_response["result"]

  with open(NODE_FILE_PRIVATE, 'w') as f:
    json.dump(node_data_private, f, indent=2)

  node_data_public = \
  {
    "name": node_data_private["name"],
    "url": node_data_private["url"],
    "owner_key": {
      "pub_key": node_data_private["owner_key"]["pub_key"]
    },
    "witness_key": {
      "pub_key": node_data_private["witness_key"]["pub_key"]
    }
  }

  with open(NODE_FILE_PUBLIC, 'w') as f:
    json.dump(node_data_public, f, indent=2)

  print( "")
  print( "Files")
  print( "\'", NODE_FILE_PRIVATE, "\'", sep="")
  print( "\'", NODE_FILE_PUBLIC, "\'", sep="")
  print( "were successfully created")
  print( "\'", NODE_FILE_PRIVATE, "\'", "contains sensitive account information, including your private keys.", sep="")
  print( "Backup this file and keep it in a secured safe place.")
  print( "Loosing or leaking information contained in this file leads to lose control over your account." )
  print( "\'", NODE_FILE_PUBLIC, "\'", "contains public data which can be safely distributed.", sep="")
  print( "Send this file to the authenticated registrar to register your account in the blockchain.")
  print( "")

###
# prepare_account_for_registration()

  



