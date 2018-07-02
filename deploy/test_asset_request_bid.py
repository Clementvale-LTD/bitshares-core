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

'''  
jsonrpc_call["method"] = "create_asset"
jsonrpc_call["params"] = ["t1node", "T1PUB", 0, { "max_supply": "1000000000000", "whitelist_authorities": [], "blacklist_authorities": [], "memo": "<params>...XML descryption PUBLIC ...</params>" }, None, True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "create_asset"
jsonrpc_call["params"] = ["t1node", "T1PRIVA", 0, { "max_supply": "1000000000000", "whitelist_authorities": ["t1node", "t2node", "t3node"], "blacklist_authorities": [], "memo": "<params>...XML descryption PRIVATE A ...</params>" }, None, True]
cli_response = call_cli_wallet( jsonrpc_call )


jsonrpc_call["method"] = "issue_asset"
jsonrpc_call["params"] = ["t1node", "1000000000", "T1PUB", "First issue", True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "issue_asset"
jsonrpc_call["params"] = ["t1node", "1000000000", "T1PRIVA", "First issue", True]
cli_response = call_cli_wallet( jsonrpc_call )
'''

jsonrpc_call["method"] = "create_bid_request"
jsonrpc_call["params"] = ["t1node", "t1node_br_1", "test bid request 1", ["T1PUB", "T1PRIVA"], 60, True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "create_bid_request"
jsonrpc_call["params"] = ["t1node", "t1node_br_2", "test bid request 2", ["T1PUB", "T1PRIVA"], 60, True]
cli_response = call_cli_wallet( jsonrpc_call )


jsonrpc_call["method"] = "create_bid"
jsonrpc_call["params"] = ["t1node", "t1node_bid_1", "t1node_br_1", "test bid 1", 300, True]
cli_response = call_cli_wallet( jsonrpc_call )


