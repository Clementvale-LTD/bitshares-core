# -*- coding: utf-8 -*-
from node_lib import *
import json
import getpass

def finish_account_configuration():

  try:
    with open(NODE_FILE_PRIVATE, 'r') as f:
      node_data = f.read()
  except Exception as e:
    print( "Cannot read from \'{0}\' file:".format( NODE_FILE_PRIVATE) )
    print( e)
    exit(-1)

  try:
    node_json = json.loads(node_data)
  except Exception as e:
    print( "Incorrect JSON data in \'{0}\' file:".format( NODE_FILE_PRIVATE) )
    print( e)
    exit(-1)

  checknode(node_json, "name")
  checknode(node_json, "url")
  ## checknode(node_json, "wallet_password")
  checknode(node_json, "owner_key", "wif_priv_key" )
  checknode(node_json, "owner_key", "pub_key" )
  checknode(node_json, "witness_key", "wif_priv_key" )
  checknode(node_json, "witness_key", "pub_key" )

  # template for JSON call
  jsonrpc_call_s = '{"jsonrpc": "2.0", "method": "", "params": [], "id": 1}'

  # parsed JSON template
  jsonrpc_call = json.loads(jsonrpc_call_s)

  jsonrpc_call["method"] = "about"
  jsonrpc_call["params"] = []
  call_cli_wallet( jsonrpc_call )

  jsonrpc_call["method"] = "info"
  jsonrpc_call["params"] = []
  call_cli_wallet( jsonrpc_call )

  jsonrpc_call["method"] = "is_new"
  jsonrpc_call["params"] = []
  cli_response = call_cli_wallet( jsonrpc_call )
  cli_IsPasswordNew = cli_response["result"]

  if cli_IsPasswordNew:
    print( "You are about to create your new wallet.")
    print( "You will be asked to specify password.")
    print( "Remember this password and keep it in a safe place.")
    print( "It is highly recommended to use strong passwords:")
    print( " password length 12 charters or more, minimum is 5")
    print( " mixed lower and upper case charters")
    print( " with digits and special symbols")
    while True:
      print( "Specify password for your wallet")
      cli_password = getpass.getpass(prompt='>>>', stream=None)
      if length(cli_password) < 5:
        print( "Too short password")
        continue
      print( "Re-type your password")
      cli_password2 = getpass.getpass(prompt='>>>', stream=None)
      if cli_password == cli_password2:
        break
      else:
        print( "Passwords don’t match")
    jsonrpc_call["method"] = "set_password"
    jsonrpc_call["params"] = [cli_password]
    if is_cli_wallet_succeeded(jsonrpc_call):
      jsonrpc_call["method"] = "unlock"
      jsonrpc_call["params"] = [cli_password]
      if is_cli_wallet_succeeded(jsonrpc_call):
        print( "Your wallet was successfully initialized")
  else:
    while True:
      print( "Specify password for your wallet")
      cli_password = getpass.getpass(prompt='>>>', stream=None)
      jsonrpc_call["method"] = "unlock"
      jsonrpc_call["params"] = [cli_password]
      if is_cli_wallet_succeeded(jsonrpc_call):
        break
      print( "Password is incorrect")    

  jsonrpc_call["method"] = "get_account"
  jsonrpc_call["params"] = [node_json["name"]]
  call_cli_wallet( jsonrpc_call )

  jsonrpc_call["method"] = "unlock"
  ###[node_json["wallet_password"]]
  jsonrpc_call["params"] = [cli_password]
  call_cli_wallet( jsonrpc_call )
    
  jsonrpc_call["method"] = "import_key"
  jsonrpc_call["params"] = [node_json["name"], node_json["owner_key"]["wif_priv_key"]]
  cli_response = call_cli_wallet( jsonrpc_call )
  if cli_response["result"] == False :
    print( "cli_wallet: cannot import private key for account \'{0}\':".format( node_json["name"]) )
    exit(-4)

  jsonrpc_call["method"] = "upgrade_account"
  jsonrpc_call["params"] = [node_json["name"],True]
  cli_response = call_cli_wallet( jsonrpc_call )

  jsonrpc_call["method"] = "create_witness"
  jsonrpc_call["params"] = [node_json["name"], node_json["url"], True]
  cli_response = call_cli_wallet( jsonrpc_call )

  jsonrpc_call["method"] = "update_witness"
  jsonrpc_call["params"] = [node_json["name"], "", node_json["witness_key"]["pub_key"], True]
  cli_response = call_cli_wallet( jsonrpc_call )

  jsonrpc_call["method"] = "create_committee_member"
  jsonrpc_call["params"] = [node_json["name"], node_json["url"], True]
  cli_response = call_cli_wallet( jsonrpc_call )

  jsonrpc_call["method"] = "vote_for_witness"
  jsonrpc_call["params"] = [node_json["name"], node_json["name"], True, True]
  cli_response = call_cli_wallet( jsonrpc_call )

  jsonrpc_call["method"] = "vote_for_committee_member"
  jsonrpc_call["params"] = [node_json["name"], node_json["name"], True, True]
  cli_response = call_cli_wallet( jsonrpc_call )
