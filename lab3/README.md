# How to run our code

## Use language 
- python
## Environment configuration
- According to the lab's requirements, we only need to use the command ` python kvstore2pcsystem.py --config [conf_file] `to open coordinator and participant service
## Test
- We offer a src called `Client.py`. Run this file with command `python Client.py`, then u can type SET,DEL and GET to interact our 2pc kV_ Store！
## Demo test
![avatar](demo_test.png


启动协调者
python3 kvstore2pcsystem.py --config coordinator.conf 

启动参与者1
python3 kvstore2pcsystem.py --config participant1.conf

启动参与者2
python3 kvstore2pcsystem.py --config participant2.conf

启动客户端
python3 Client.py