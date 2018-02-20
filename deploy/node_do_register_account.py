from __future__ import print_function
from node_lib import *
import json
import getpass
import os.path


NODE_FILE_PRIVATE = "node_private_keep_in_secret.json"
NODE_FILE_PUBLIC  = "node_puplic.json"

def do_register_account():

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
  print( "This tool will register a new account in the blockchain in favor of other user")
  print( "Follow instruction and supply necessary information")
  print( "")

  node_pub = NODE_FILE_PUBLIC

  if os.path.isfile( NODE_FILE_PUBLIC):
    print( "\'", node_pub,  "\' file is found", sep="")
    print( "Do you want to use another file?[Y/N]")
    user_confirm = raw_input(">>>")
    if ('Y'==user_confirm[0] or 'y'==user_confirm[0]):
      node_pub = ""
  else:
    print( "Cannot find \'", node_pub, "\' file", sep="")
    node_pub = ""

  if node_pub == "":
    print( "Specify path to the file containing registration data of new account:")
    node_pub = raw_input(">>>")
    if not os.path.isfile( node_pub):
      print( "Cannot find \'", node_pub, "\' file", sep="")
      exit(-1)

  try:
    with open(node_pub, 'r') as f:
      node_pub_data = f.read()
  except Exception as e:
    print( "Cannot read from \'{0}\' file:".format( node_pub) )
    print( e)
    exit(-1)

  try:
    node_json = json.loads(node_pub_data)
  except Exception as e:
    print( "Incorrect JSON data in \'{0}\' file:".format( node_pub) )
    print( e)
    exit(-1)

  checknode(node_json, "name")
  checknode(node_json, "url")
  checknode(node_json, "owner_key",   "pub_key" )
  checknode(node_json, "witness_key", "pub_key" )

  print( "" )
  print( "You are about to register the following new account:" )
  print( json.dumps( node_json, indent=2) )
  print( "Do you wish to continue? [Y/N]" )
  user_confirm = raw_input(">>>")
  if ('Y'==user_confirm[0] or 'y'==user_confirm[0]):
    node_pub = ""

  # template for JSON call
  jsonrpc_call_s = '{"jsonrpc": "2.0", "method": "", "params": [], "id": 1}'

  # parsed JSON template
  jsonrpc_call = json.loads(jsonrpc_call_s)

  while True:
    print( "")
    print( "Specify password for your wallet")
    cli_password = getpass.getpass(prompt='>>>', stream=None)
    jsonrpc_call["method"] = "unlock"
    jsonrpc_call["params"] = [cli_password]
    if is_cli_wallet_succeeded(jsonrpc_call):
      break
    print( "Password is incorrect")    

  jsonrpc_call["method"] = "list_my_accounts"
  jsonrpc_call["params"] = [cli_password]
  cli_response = call_cli_wallet(jsonrpc_call)
  list_my_accounts = cli_response["result"]
  if not isinstance(list_my_accounts, list):
    print( "Something wrong: cannot get your account")    
    exit(-2)

  if 0==len(list_my_accounts) :
    print( "No accounts in your wallet")
    exit(-2)

  my_acc = list_my_accounts[0]

  if len(list_my_accounts) > 1:
    print( "Select account to use as registrar:")
    num_registrar = 1 
    for acc in list_my_accounts:
      print(num_registrar, ":", acc["name"])
      num_registrar += 1
    num_registrar = int( raw_input(">>>"))
    if num_registrar < 1 or num_registrar > len(list_my_accounts):
      print("Error")
      exit(-2)
    num_registrar -= 1  
    my_acc = list_my_accounts[num_registrar]

  jsonrpc_call["method"] = "register_account"
  jsonrpc_call["params"] = [node_json["name"], node_json["owner_key"]["pub_key"], node_json["owner_key"]["pub_key"], my_acc["name"], my_acc["name"], 0, True ]
  call_cli_wallet( jsonrpc_call )

  print( "" )
  print( "New account was successfully registered" )
  print( "Notify account owner" )

  jsonrpc_call["method"] = "lock"
  jsonrpc_call["params"] = []
  is_cli_wallet_succeeded(jsonrpc_call)
  


