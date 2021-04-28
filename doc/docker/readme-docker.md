```sh
# Docker engine install
$ sudo apt update
$ sudo apt-get remove docker docker-engine docker.io containerd runc
$ sudo apt-get install \
    apt-transport-https \
    ca-certificates \
    curl \
    gnupg \
    lsb-release
$ curl -fsSL https://download.docker.com/linux/debian/gpg | sudo gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
$ echo \
  "deb [arch=amd64 signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/debian \
  $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
$ sudo apt-get update
$ sudo apt-get install docker-ce docker-ce-cli containerd.io
$ sudo usermod -a -G docker poc

# Docker compose install
$ sudo curl -L "https://github.com/docker/compose/releases/download/1.29.1/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
$ sudo chmod +x /usr/local/bin/docker-compose
$ sudo ln -s /usr/local/bin/docker-compose /usr/bin/docker-compose
$ sudo nano /etc/docker/daemon.json
# { "insecure-registries":["bee40b7e7c68.sn.mynetname.net:5000"] }

# Firewall
$ sudo apt install ufw
$ sudo ufw enable
$ sudo ufw allow ssh
$ sudo ufw allow http
$ sudo ufw allow https

# Settings
$ echo "alias dc='docker-compose'" >> ~/.bashrc
$ . ~/.bashrc

# For launch create dirs and put conf
$ mkdir -p ~/docker/data/certbot
$ mkdir -p ~/docker/data/pocketcoin
$ sudo chown 10000:10000 ~/docker/data/pocketcoin/ -R
$ sudo chmod 700 ~/docker/data/pocketcoin/ -R

# .. copy nginx.conf
$ curl -L https://raw.githubusercontent.com/dancheskus/nginx-docker-ssl/master/init-letsencrypt.sh > init-letsencrypt.sh

# .. open and change
# .. example.com -> domain
# .. email
# .. paths
# .. temp set staging=1 for test
$ chmod +x init-letsencrypt.sh
$ sudo ./init-letsencrypt.sh

# Go
$ docker-compose up -d


```