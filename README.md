Blockchain Core for Blockchain-Telecom
==============
* [Getting Started](#getting-started)
* [Using the API](#using-the-api)
* [License](#license)

Blockchain Core is the blockchain implementation and command-line interface for decentralized telecom ecosystem.

Visit [wiki.blockchaintele.com](https://wiki.blockchaintele.com) to learn about Blockchain Telecom ecosystem.


Getting Started
---------------
Guides, documentation, use cases and API  specifications are available in the
[wiki](https://wiki.blockchaintele.com).

We recommend building on Ubuntu 16.04 LTS, and the build dependencies may be installed with:

    sudo apt-get update
    sudo apt-get install autoconf cmake git libboost-all-dev libssl-dev g++ libcurl4-openssl-dev

To build after all dependencies are installed:

    git clone https://github.com/Clementvale-LTD/blockchain-telecom.graphene-core.git
    cd blockchain-telecom.graphene-core
    git checkout <LATEST_RELEASE_TAG>
    git submodule update --init --recursive
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .
    make


After building, the witness node can be launched with:

    ./programs/witness_node/witness_node

The node will automatically create a data directory including a config file. 

After syncing, you can exit the node using Ctrl+C and setup the command-line wallet by editing
`witness_node_data_dir/config.ini` as follows:

    rpc-endpoint = 127.0.0.1:7212

After starting the witness node again, in a separate terminal you can run:

    ./programs/cli_wallet/cli_wallet

Set your inital password:

    >>> set_password <PASSWORD>
    >>> unlock <PASSWORD>

`rpc-endpoint` should be bound to localhost for security.

Use `help` to see all available wallet commands. Source definition and listing of all commands is available
[here](https://wiki.blockchaintele.com/index.php/Blockchain_API).


Using the API
-------------

We provide several different API's.  Each API has its own ID.
When running `witness_node`, initially two API's are available:
API 0 provides read-only access to the database, while API 1 is
used to login and gain access to additional, restricted API's.

Here is an example using `curl` HTTP client for API's which do not require login or other session state:

    $ curl --data '{"jsonrpc": "2.0", "method": "call", "params": [0, "get_accounts", [["1.2.0"]]], "id": 1}' http://127.0.0.1:7212/rpc
    {"id":1,"result":[{"id":"1.2.0","annotations":[],"membership_expiration_date":"1969-12-31T23:59:59","registrar":"1.2.0","referrer":"1.2.0","lifetime_referrer":"1.2.0","network_fee_percentage":2000,"lifetime_referrer_fee_percentage":8000,"referrer_rewards_percentage":0,"name":"committee-account","owner":{"weight_threshold":1,"account_auths":[],"key_auths":[],"address_auths":[]},"active":{"weight_threshold":6,"account_auths":[["1.2.5",1],["1.2.6",1],["1.2.7",1],["1.2.8",1],["1.2.9",1],["1.2.10",1],["1.2.11",1],["1.2.12",1],["1.2.13",1],["1.2.14",1]],"key_auths":[],"address_auths":[]},"options":{"memo_key":"GPH1111111111111111111111111111111114T1Anm","voting_account":"1.2.0","num_witness":0,"num_committee":0,"votes":[],"extensions":[]},"statistics":"2.7.0","whitelisting_accounts":[],"blacklisting_accounts":[]}]}

API 0 is accessible using regular JSON-RPC:

    $ curl --data '{"jsonrpc": "2.0", "method": "get_accounts", "params": [["1.2.0"]], "id": 1}' http://127.0.0.1:7212/rpc

To access the restricted API you must have private key corresponded to 
the account registered in the blockchain.  To start using restricted API 
you must run cli_wallet (for security reasons it is recommended to run it 
locally on your workstation) and import private key into your wallet. See 
[API guide](https://wiki.blockchaintele.com/index.php/Blockchain_API) 
for details.

 License
-------
Blockchain Core for Blockchain-Telecom is under the MIT license. See [LICENSE](https://github.com/Clementvale-LTD/blockchain-telecom.graphene-core/blob/bubbletone/LICENSE.txt)
for more information.
