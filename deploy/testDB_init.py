# -*- coding: utf-8 -*-
from node_lib import *
import json
import getpass

set_cli_wallet_url( "http://10.200.0.185:1227/rpc")
# set_cli_wallet_url( "http://127.0.0.1:1227/rpc")
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

jsonrpc_call["method"] = "is_new"
jsonrpc_call["params"] = []
cli_response = call_cli_wallet( jsonrpc_call )
cli_IsPasswordNew = cli_response["result"]

if cli_IsPasswordNew:
  jsonrpc_call["method"] = "set_password"
  jsonrpc_call["params"] = [cli_password]
  if is_cli_wallet_succeeded(jsonrpc_call):
    jsonrpc_call["method"] = "unlock"
    jsonrpc_call["params"] = [cli_password]
    if is_cli_wallet_succeeded(jsonrpc_call):
      print( "Your wallet was successfully initialized")
else:
    jsonrpc_call["method"] = "unlock"
    jsonrpc_call["params"] = [cli_password]
    if not is_cli_wallet_succeeded(jsonrpc_call):
      print( "Password is incorrect")
      exit(-1)
  
jsonrpc_call["method"] = "import_key"
jsonrpc_call["params"] = ["ptolemy", "5JcA4UqWn8s3ktBaXxeLNcD8CgmWdHHvd9CRkDWtopcCPQVwG8f"]
cli_response = call_cli_wallet( jsonrpc_call )
if cli_response["result"] == False :
  print( "cli_wallet: cannot import private key for account \'{0}\':".format( node_json["name"]) )
  exit(-4)

jsonrpc_call["method"] = "import_balance"
jsonrpc_call["params"] = ["ptolemy", ["5JcA4UqWn8s3ktBaXxeLNcD8CgmWdHHvd9CRkDWtopcCPQVwG8f"], True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "import_key"
jsonrpc_call["params"] = ["bank", "5KA96LxtCRmkcNwvWrB4QohaXKL3ysEUsddsEsVt5WEapn5Ckf5"]
cli_response = call_cli_wallet( jsonrpc_call )
 
jsonrpc_call["method"] = "import_balance"
jsonrpc_call["params"] = ["bank", ["5KA96LxtCRmkcNwvWrB4QohaXKL3ysEUsddsEsVt5WEapn5Ckf5"], True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "transfer"
jsonrpc_call["params"] = ["bank", "ptolemy", 100000, "SDRT", "here is some SDRs", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "upgrade_account"
jsonrpc_call["params"] = ["ptolemy", True]
cli_response = call_cli_wallet( jsonrpc_call )
 
jsonrpc_call["method"] = "register_account"
jsonrpc_call["params"] = ["bubbletone", "BTE7vEhUJxJE8WKC4GVEdee114WeHcfBfRA5B7NbheMNMqnAuHUwm", "BTE7vEhUJxJE8WKC4GVEdee114WeHcfBfRA5B7NbheMNMqnAuHUwm", 0, "ptolemy", True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "import_key"
jsonrpc_call["params"] = ["bubbletone", "5KG2Uao68bRN8QmPaei1bYRdNc9r8wQP1TR3xfx2Y1hiE38ukxV"]
cli_response = call_cli_wallet( jsonrpc_call )


jsonrpc_call["method"] = "transfer"
jsonrpc_call["params"] = ["ptolemy", "bubbletone", 1000000000, "BTE", "here is some umt", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "transfer"
jsonrpc_call["params"] = ["bank", "bubbletone", 100000, "SDRT", "here is some SDRs", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "upgrade_account"
jsonrpc_call["params"] = ["bubbletone", True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "register_account"
jsonrpc_call["params"] = ["t1node", "BTE6jdewg5KQ24T8tBCRnCtgbapEkJ2hBwXE1nXQR23VxLYbMo2cL", "BTE6jdewg5KQ24T8tBCRnCtgbapEkJ2hBwXE1nXQR23VxLYbMo2cL", 0, "bubbletone", True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "register_account"
jsonrpc_call["params"] = ["t2node", "BTE8JsK7hGpPSyaNgHhKQLQGwwFACyvksUUgyKZfKP1kCySEL6oLt", "BTE8JsK7hGpPSyaNgHhKQLQGwwFACyvksUUgyKZfKP1kCySEL6oLt", 0, "bubbletone", True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "register_account"
jsonrpc_call["params"] = ["t3node", "BTE7EJpNzzCJts1EJWfrzkzrWwB65TL7WA51BirngPiKEyqkkPj5q", "BTE7EJpNzzCJts1EJWfrzkzrWwB65TL7WA51BirngPiKEyqkkPj5q", 0, "bubbletone", True]
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

jsonrpc_call["method"] = "register_account"
jsonrpc_call["params"] = ["countrycom", "BTE8LEYPd6i2ucj2Q8Q21tgtqu5BrmAwTHpkbjpcc1HHjAnSqdXnF", "BTE8LEYPd6i2ucj2Q8Q21tgtqu5BrmAwTHpkbjpcc1HHjAnSqdXnF", 0, "bubbletone", True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "register_account"
jsonrpc_call["params"] = ["mds", "BTE5NS1nD7cWSkPRj45gqMciu52rYVC9EDXCBwh7PVFZsbMssMn8o", "BTE5NS1nD7cWSkPRj45gqMciu52rYVC9EDXCBwh7PVFZsbMssMn8o", 0, "bubbletone", True]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "register_account"
jsonrpc_call["params"] = ["nexign", "BTE8Grq28pmVgt8NgjjHFgktrTK2tsXkCBqkY2JfyhB8Di3NTRcWE", "BTE8Grq28pmVgt8NgjjHFgktrTK2tsXkCBqkY2JfyhB8Di3NTRcWE", 0, "bubbletone", True]
cli_response = call_cli_wallet( jsonrpc_call )


# jsonrpc_call["method"] = "create_asset"
# jsonrpc_call["params"] = ["bank", "SDR", 4, { "max_supply": "1000000000000000", "market_fee_percent": 0, "max_market_fee": "1000000000000000", "issuer_permissions": 64, "flags": 0, "whitelist_authorities": [], "blacklist_authorities": [], "whitelist_markets": [], "blacklist_markets": [], "description": "", "extensions": [] }, None, True]
# cli_response = call_cli_wallet( jsonrpc_call )

# jsonrpc_call["method"] = "issue_asset"
# jsonrpc_call["params"] = ["bank", "1000000000", "SDR", "first issue", True]
# cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "transfer"
jsonrpc_call["params"] = ["bank", "t1node", 10000, "SDRT", "here is some umt", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "transfer"
jsonrpc_call["params"] = ["bank", "t2node", 15000, "SDRT", "here is some umt", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "transfer"
jsonrpc_call["params"] = ["bank", "t3node", 50000, "SDRT", "here is some umt", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "transfer"
jsonrpc_call["params"] = ["bank", "countrycom", 10000, "SDRT", "here is some umt", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "transfer"
jsonrpc_call["params"] = ["bank", "nexign", 10000, "SDRT", "here is some umt", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "transfer"
jsonrpc_call["params"] = ["bank", "mds", 10000, "SDRT", "here is some umt", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "vote_for_committee_member"
jsonrpc_call["params"] = ["ptolemy", "init0", "true", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "vote_for_committee_member"
jsonrpc_call["params"] = ["ptolemy", "init1", "true", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "vote_for_committee_member"
jsonrpc_call["params"] = ["ptolemy", "init2", "true", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "vote_for_committee_member"
jsonrpc_call["params"] = ["ptolemy", "init3", "true", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "vote_for_committee_member"
jsonrpc_call["params"] = ["ptolemy", "init4", "true", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "vote_for_committee_member"
jsonrpc_call["params"] = ["ptolemy", "init5", "true", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "vote_for_committee_member"
jsonrpc_call["params"] = ["ptolemy", "init6", "true", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "vote_for_committee_member"
jsonrpc_call["params"] = ["ptolemy", "init7", "true", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "vote_for_committee_member"
jsonrpc_call["params"] = ["ptolemy", "init8", "true", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "vote_for_committee_member"
jsonrpc_call["params"] = ["ptolemy", "init9", "true", "true"]
cli_response = call_cli_wallet( jsonrpc_call )

jsonrpc_call["method"] = "vote_for_committee_member"
jsonrpc_call["params"] = ["ptolemy", "init10", "true", "true"]
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

