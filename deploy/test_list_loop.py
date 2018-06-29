# -*- coding: utf-8 -*-
from node_lib import *
import json
import getpass

cli_password="smSecret"

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

jsonrpc_call["method"] = "unlock"
jsonrpc_call["params"] = [cli_password]
if not is_cli_wallet_succeeded(jsonrpc_call):
  print( "Password is incorrect")
  exit(-1)
  
while True:  
  jsonrpc_call["method"] = "list_assets"
  jsonrpc_call["params"] = ["", 0, 100]
  cli_response = call_cli_wallet( jsonrpc_call )

  jsonrpc_call["method"] = "list_account_balances"
  jsonrpc_call["params"] = ["t1node"]
  cli_response = call_cli_wallet( jsonrpc_call )

  jsonrpc_call["method"] = "list_services"
  jsonrpc_call["params"] = ["", 100]
  cli_response = call_cli_wallet( jsonrpc_call )

  jsonrpc_call["method"] = "list_bid_requests"
  jsonrpc_call["params"] = ["", None, 100]
  cli_response = call_cli_wallet( jsonrpc_call )

  jsonrpc_call["method"] = "list_bids"
  jsonrpc_call["params"] = ["", 100]
  cli_response = call_cli_wallet( jsonrpc_call )
