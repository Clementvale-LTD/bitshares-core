
1. 
copy 

cli_wallet.service
witness_node.service

into
/lib/systemd/system/

2.
mkdir /srv/bubbletone
mkdir /srv/bubbletone/programs
mkdir /srv/bubbletone/programs/witness_node
mkdir /srv/bubbletone/programs/cli_wallet
mkdir /srv/bubbletone/data
mkdir /srv/bubbletone/data/bubbletone-wallet

3. 
copy 

witness_node 
and 
cli_wallet 
into 

/srv/bubbletone/programs/witness_node
and 
/srv/bubbletone/programs/cli_wallet
correspondingly

4. 
copy 'bubbletone' blockchain directory containing config.ini into 
/srv/bubbletone/data

5.
Add services to autostart and start it:

systemctl enable witness_node
systemctl start witness_node

systemctl enable cli_wallet
systemctl start cli_wallet

6. view logs 
sudo journalctl -u witness_node
sudo journalctl -f -u witness_node

