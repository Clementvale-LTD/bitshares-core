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
###[node_json["wallet_password"]]
jsonrpc_call["params"] = [cli_password]
call_cli_wallet( jsonrpc_call )
  
jsonrpc_call["method"] = "import_key"
jsonrpc_call["params"] = ["ptolemy", "5JcA4UqWn8s3ktBaXxeLNcD8CgmWdHHvd9CRkDWtopcCPQVwG8f"]
cli_response = call_cli_wallet( jsonrpc_call )
if cli_response["result"] == False :
  print( "cli_wallet: cannot import private key for account \'{0}\':".format( node_json["name"]) )
  exit(-4)

jsonrpc_call["method"] = "import_balance"
jsonrpc_call["params"] = ["ptolemy", ["5JcA4UqWn8s3ktBaXxeLNcD8CgmWdHHvd9CRkDWtopcCPQVwG8f"], True]
cli_response = call_cli_wallet( jsonrpc_call )


jsonrpc_call["method"] = "upgrade_account"
jsonrpc_call["params"] = ["ptolemy", True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "register_account"
jsonrpc_call["params"] = ["bubbletone", "BTS7vEhUJxJE8WKC4GVEdee114WeHcfBfRA5B7NbheMNMqnAuHUwm", "BTS7vEhUJxJE8WKC4GVEdee114WeHcfBfRA5B7NbheMNMqnAuHUwm", "ptolemy", "ptolemy", 0, True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "import_key"
jsonrpc_call["params"] = ["bubbletone", "5KG2Uao68bRN8QmPaei1bYRdNc9r8wQP1TR3xfx2Y1hiE38ukxV"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "upgrade_account"
jsonrpc_call["params"] = ["bubbletone", True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "register_account"
jsonrpc_call["params"] = ["bank", "BTS73hk1bKb4JaNPmBqMWKHWMxF1FiBdq3DyeNpH4gaM1mxY13Lez", "BTS73hk1bKb4JaNPmBqMWKHWMxF1FiBdq3DyeNpH4gaM1mxY13Lez", "bubbletone", "bubbletone", 0, True]
cli_response = call_cli_wallet( jsonrpc_call )


jsonrpc_call["method"] = "register_account"
jsonrpc_call["params"] = ["t1node", "BTS6jdewg5KQ24T8tBCRnCtgbapEkJ2hBwXE1nXQR23VxLYbMo2cL", "BTS6jdewg5KQ24T8tBCRnCtgbapEkJ2hBwXE1nXQR23VxLYbMo2cL", "bubbletone", "bubbletone", 0, True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "register_account"
jsonrpc_call["params"] = ["t2node", "BTS8JsK7hGpPSyaNgHhKQLQGwwFACyvksUUgyKZfKP1kCySEL6oLt", "BTS8JsK7hGpPSyaNgHhKQLQGwwFACyvksUUgyKZfKP1kCySEL6oLt", "bubbletone", "bubbletone", 0, True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "register_account"
jsonrpc_call["params"] = ["t3node", "BTS7EJpNzzCJts1EJWfrzkzrWwB65TL7WA51BirngPiKEyqkkPj5q", "BTS7EJpNzzCJts1EJWfrzkzrWwB65TL7WA51BirngPiKEyqkkPj5q", "bubbletone", "bubbletone", 0, True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "import_key"
jsonrpc_call["params"] = ["bank", "5KA96LxtCRmkcNwvWrB4QohaXKL3ysEUsddsEsVt5WEapn5Ckf5"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "import_key"
jsonrpc_call["params"] = ["t1node", "5Jd6bPHZsxZvGcSCxG5AKk8ehb2F8FDPMqGccy2xji7XUcxMba4"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "import_key"
jsonrpc_call["params"] = ["t2node", "5KayRqbWQivnksDUFHFn7DQEnBeRFfmhf5H5dWaQTKm4C11pMKx"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "import_key"
jsonrpc_call["params"] = ["t3node", "5J2rJgVbj4VTjo4UAnpshDGYiSGxgMhdVFG32BiZ8snuSf6u6X8"]
cli_response = call_cli_wallet( jsonrpc_call )


jsonrpc_call["method"] = "create_asset"
jsonrpc_call["params"] = ["bank", "SDR", 4, { "max_supply": "1000000000000000", "market_fee_percent": 0, "max_market_fee": "1000000000000000", "issuer_permissions": 64, "flags": 0, "core_exchange_rate": {"base": {"amount": 1, "asset_id": "1.3.0"}, "quote": {"amount": 1,"asset_id": "1.3.1"} }, "whitelist_authorities": [], "blacklist_authorities": [], "whitelist_markets": [], "blacklist_markets": [], "description": "", "extensions": [] }, None, True]
cli_response = call_cli_wallet( jsonrpc_call )


jsonrpc_call["method"] = "issue_asset"
jsonrpc_call["params"] = ["bank", "1000000000", "SDR", "first issue", True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "transfer"
jsonrpc_call["params"] = ["bank", "t1node", 10000, "SDR", "here is some umt", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "transfer"
jsonrpc_call["params"] = ["bank", "t2node", 15000, "SDR", "here is some umt", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "transfer"
jsonrpc_call["params"] = ["bank", "t3node", 50000, "SDR", "here is some umt", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

# jsonrpc_call["method"] = "transfer"
# jsonrpc_call["params"] = ["ptolemy", "bank", 10000000, "UMT", "here is some umt", "true"]
# cli_response = call_cli_wallet( jsonrpc_call )

# jsonrpc_call["method"] = "transfer"
# jsonrpc_call["params"] = ["ptolemy", "t1node", 5000000, "UMT", "here is some umt", "true"]
# cli_response = call_cli_wallet( jsonrpc_call )

# jsonrpc_call["method"] = "transfer"
# jsonrpc_call["params"] = ["ptolemy", "t2node", 2000000, "UMT", "here is some umt", "true"]
# cli_response = call_cli_wallet( jsonrpc_call )

# jsonrpc_call["method"] = "transfer"
# jsonrpc_call["params"] = ["ptolemy", "t3node", 1500000, "UMT", "here is some umt", "true"]
# cli_response = call_cli_wallet( jsonrpc_call )

