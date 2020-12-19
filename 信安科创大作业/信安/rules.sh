#!/bin/sh

#清空规则
sudo iptables -F
sudo iptables -t nat -F

#设置默认规则
sudo iptables -P INPUT DROP
sudo iptables -P OUTPUT ACCEPT
sudo iptables -P FORWARD DROP

#允许建立通信
sudo iptables -A INPUT -i ens38 -j ACCEPT
sudo iptables -A FORWARD -i ens38 -o ens33 -j ACCEPT
sudo iptables -I INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
sudo iptables -I FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT

#隐藏拓扑结构
sudo iptables -t nat -A POSTROUTING -s 192.168.33.0/24 -j SNAT --to 192.168.205.128

#开启http以及ftp服务
sudo iptables -A FORWARD -i ens33 -o ens38 -p tcp --dport 20 -j DNAT --to 192.168.33.11:20
sudo iptables -A FORWARD -i ens33 -o ens38 -p tcp --dport 21 -j DNAT --to 192.168.33.11:21
sudo iptables -A FORWARD -i ens33 -o ens38 -p tcp --dport 80 -j DNAT --to 192.168.33.11:80
sudo iptables -A FORWARD -i ens33 -o ens38 -p tcp --dport 1997:7991 -j DNAT --to 192.168.33.11:1997-7991
sudo iptables -t nat -A PREROUTING -i ens33 -p tcp --dport 20 -j DNAT --to 192.168.33.11:20
sudo iptables -t nat -A PREROUTING -i ens33 -p tcp --dport 21 -j DNAT --to 192.168.33.11:21
sudo iptables -t nat -A PREROUTING -i ens33 -p tcp --dport 80 -j DNAT --to 192.168.33.11:80
sudo iptables -t nat -A PREROUTING -p tcp --dport 1997:7991 -j DNAT --to 192.168.33.11:1997-7991

