from node_registration_request import * 
from node_do_register_account  import * 
from node_finish_configuration import * 

print( "Welcome to blockchain-in-telecom!" )
print( "This helper will guide you through the process of your new account registration." )
print( "Select your action:" )
print( "  1. I want to prepare data for my new account registration" )
print( "  2. I want to register a new account in favor of another user" )
print( "  3. My account was already registered in the blockchain. I want to finish configuration of my account" )

num_action = int( raw_input(">>>"))
if num_action < 1 or num_action > 3 :
  print("Invalid action")
  exit(-2)

if 1==num_action:
  prepare_account_for_registration()
elif 2==num_action:
  do_register_account()  
elif 3==num_action:
  finish_account_configuration()

