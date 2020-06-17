sudo docker rm -f $(sudo docker ps -a -q)
sudo docker rm -f $(sudo docker container ls -a -q)
sudo docker network rm sock-network
sudo docker network create --subnet 172.20.0.0/16 --ip-range 172.20.240.0/24 sock-network
sleep 5
sudo docker run -d --network sock-network --ip=172.20.240.19 --name=session-db  isalb/redis
sudo docker run -d --network sock-network --ip=172.20.240.2 isalb/shipping
sudo docker run -d --network sock-network --ip=172.20.240.9 --name=user-db  mongo:3.4
sudo docker run -d --network sock-network --ip=172.20.240.3 -e MONGO_HOST=user-db:27017  isalb/user
sudo docker run -d --network sock-network --ip=172.20.240.8 --name=carts-db  mongo:3.4
sudo docker run -d --network sock-network --ip=172.20.240.5 -e MONGO_HOST=cart-db:27017  isalb/carts
sudo docker run -d --network sock-network --ip=172.20.240.6  isalb/payment
sudo docker run -d --network sock-network --ip=172.20.240.10 --name=orders-db  mongo:3.4
sudo docker run -d --network sock-network --ip=172.20.240.4 -e MONGO_HOST=orders-db:27017 isalb/orders
sudo docker run -d --network sock-network --ip=172.20.240.11  isalb/cataloguedb
sudo docker run -d --network sock-network --ip=172.20.240.7  isalb/catalogue
sudo docker run -d --network sock-network --ip=172.20.240.200 -v /a0.conf:/etc/nginx/conf.d/a.conf -v /public:/public -p 8080:8080 nginx
sudo docker run -d --network sock-network --ip=172.20.240.1  -p 8079:8079 -e SESSION_REDIS=1  isalb/frontend
sudo docker run -d --network sock-network --ip=172.20.240.20  -p 10020:10020 -e PORT=10020 -e SESSION_REDIS=1  isalb/frontend
sudo docker run -d --network sock-network --ip=172.20.240.30  -p 10030:10030 -e PORT=10030 -e SESSION_REDIS=1  isalb/frontend
sudo docker run -d --network sock-network --ip=172.20.240.40  -p 10040:10040 -e PORT=10040 -e SESSION_REDIS=1  isalb/frontend
sudo docker run -d --network sock-network --ip=172.20.240.50  -p 10050:10050 -e PORT=10050 -e SESSION_REDIS=1  isalb/frontend
sudo docker run -d --network sock-network --ip=172.20.240.60  -p 10060:10060 -e PORT=10060 -e SESSION_REDIS=1  isalb/frontend
sudo docker run -d --network sock-network --ip=172.20.240.70  -p 10070:10070 -e PORT=10070 -e SESSION_REDIS=1  isalb/frontend
sudo docker run -d --network sock-network --ip=172.20.240.80  -p 10080:10080 -e PORT=10080 -e SESSION_REDIS=1  isalb/frontend
sudo docker run -d --network sock-network --ip=172.20.240.90  -p 10090:10090 -e PORT=10090 -e SESSION_REDIS=1  isalb/frontend
sudo docker run -d --network sock-network --ip=172.20.240.100  -p 10100:10100 -e PORT=10100 -e SESSION_REDIS=1  isalb/frontend
