# -*- coding: utf-8 -*-
import json
from curl import *

NODE_FILE_PRIVATE = "node_private_keep_in_secret.json"
CLI_WALLET_URL = "http://127.0.0.1:1227/rpc"

def checknode0( jnode, item):
  if item in jnode:
    return True
  else:
    print( "JSON node: item not defined: \"{0}\" ".format( item) )
    exit(-1)
  return False  

def checknode( jnode, *itm):
  jn = jnode
  for i in itm :
    if checknode0(jn, i) :
      jn = jn[i]
    else:
      return False
  return True

def print_cli_wallet_result( jresponse, jcall):
  if( "result" in jresponse):
    print( "cli_wallet: \'{0}\' :".format( jcall["method"]) )
    print( "result:")
    print( json.dumps( jresponse["result"], indent=2) )
  print("")

def print_cli_wallet_error( jresponse, jcall):
  print( "cli_wallet: failed to call \'{0}\' :".format( jcall["method"]) )
  if "error" in jresponse:
    print( "error:")
    errout = jresponse["error"]
    if "data" in errout:
      if "stack" in errout["data"]:
        del errout["data"]["stack"]
    if not "call" in errout:
      errout["call"] = jcall  
  print( json.dumps( errout, indent=2) )

def print_cli_wallet_except( e, jcall):
  print( "cli_wallet: failed to call \'{0}\' :".format( jcall["method"]) )
  print( e)

def call_cli_wallet( jcall):
  try:
    cli_response_s = curl(CLI_WALLET_URL, req_type="POST", data=json.dumps(jcall))
    cli_response = json.loads(cli_response_s['content'].decode("utf-8"))
    if "error" in cli_response or not "result" in cli_response:
      print_cli_wallet_error( cli_response, jcall)
      exit(-3)
    print_cli_wallet_result(cli_response, jcall)  
  except Exception as e:
    print_cli_wallet_except(e, jcall)
    exit(-2)
  return cli_response

def is_cli_wallet_succeeded( jcall):
  try:
    cli_response_s = curl(CLI_WALLET_URL, req_type="POST", data=json.dumps(jcall))
    cli_response = json.loads(cli_response_s['content'].decode("utf-8"))
    if "error" in cli_response or not "result" in cli_response:
      return False
  except Exception as e:
    return False
  return True

def set_cli_wallet_url( url):
  global CLI_WALLET_URL
  CLI_WALLET_URL = url

