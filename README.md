# How to build

```
$ gcc -Wall -shared -fPIC -Wl,--no-as-needed -ldl -Wl,-soname=force-keepalive-connect.so -o force-keepalive-connect.so force-keepalive-connect.c
```

# Example

Nginx can set the TCP keep-alive option to the listening socket.
http://nginx.org/en/docs/http/ngx_http_core_module.html#listen

However, it cannot be set to upstream connections.

By using LD_PRELOAD to hook the connect call as follows, the TCP keep-alive is also enabled for upstream connection.
[(nginx config file)](./README.md#nginxconf)

```
## Run nginx with LD_PRELOAD
# LD_PRELOAD=/usr/local/lib/force-keepalive-connect.so nginx

## Trace specific system calls
# strace -ftty -e trace=connect,setsockopt $(pgrep nginx | sed 's/^/-p /')

## Capture packets for upstream connection
# tshark -nn -i lo port 8080

## Make a request to the nginx
# curl localhost

## output of strace
[pid 10710] 11:02:21.167361 setsockopt(12, SOL_SOCKET, SO_KEEPALIVE, [1], 4) = 0
[pid 10710] 11:02:21.167551 setsockopt(12, SOL_TCP, TCP_KEEPIDLE, [30], 4) = 0
[pid 10710] 11:02:21.167753 setsockopt(12, SOL_TCP, TCP_KEEPINTVL, [5], 4) = 0
[pid 10710] 11:02:21.167888 setsockopt(12, SOL_TCP, TCP_KEEPCNT, [2], 4) = 0
[pid 10710] 11:02:21.168036 connect(12, {sa_family=AF_INET, sin_port=htons(8080), sin_addr=inet_addr("127.0.0.1")}, 16) = -1 EINPROGRESS (Operation now in progress)
[pid 10710] 11:02:21.168566 setsockopt(13, SOL_TCP, TCP_NODELAY, [1], 4) = 0
[pid 10710] 11:02:21.168805 setsockopt(3, SOL_TCP, TCP_NODELAY, [1], 4) = 0

## output of tshark
  1 0.000000000    127.0.0.1 -> 127.0.0.1    TCP 74 42666 > 8080 [SYN] Seq=0 Win=43690 Len=0 MSS=65495 SACK_PERM=1 TSval=2461569836 TSecr=0 WS=128
  2 0.000025033    127.0.0.1 -> 127.0.0.1    TCP 74 8080 > 42666 [SYN, ACK] Seq=0 Ack=1 Win=43690 Len=0 MSS=65495 SACK_PERM=1 TSval=2461569836 TSecr=2461569836 WS=128
  3 0.000035361    127.0.0.1 -> 127.0.0.1    TCP 66 42666 > 8080 [ACK] Seq=1 Ack=1 Win=43776 Len=0 TSval=2461569836 TSecr=2461569836
  4 0.000201735    127.0.0.1 -> 127.0.0.1    HTTP 137 GET / HTTP/1.1
  5 0.000207507    127.0.0.1 -> 127.0.0.1    TCP 66 8080 > 42666 [ACK] Seq=1 Ack=72 Win=43776 Len=0 TSval=2461569836 TSecr=2461569836
  6 0.000375293    127.0.0.1 -> 127.0.0.1    HTTP 916 HTTP/1.1 200 OK  (text/html)
  7 0.000379483    127.0.0.1 -> 127.0.0.1    TCP 66 42666 > 8080 [ACK] Seq=72 Ack=851 Win=45440 Len=0 TSval=2461569836 TSecr=2461569836
  8 30.462472088    127.0.0.1 -> 127.0.0.1    TCP 66 [TCP Keep-Alive] 42666 > 8080 [ACK] Seq=71 Ack=851 Win=45440 Len=0 TSval=2461569836 TSecr=2461569836
  9 30.462510745    127.0.0.1 -> 127.0.0.1    TCP 66 [TCP Keep-Alive ACK] 8080 > 42666 [ACK] Seq=851 Ack=72 Win=43776 Len=0 TSval=2461600298 TSecr=2461569836
 10 60.670574861    127.0.0.1 -> 127.0.0.1    TCP 66 [TCP Keep-Alive] 42666 > 8080 [ACK] Seq=71 Ack=851 Win=45440 Len=0 TSval=2461600298 TSecr=2461600298
 11 60.670733263    127.0.0.1 -> 127.0.0.1    TCP 66 [TCP Keep-Alive ACK] 8080 > 42666 [ACK] Seq=851 Ack=72 Win=43776 Len=0 TSval=2461630505 TSecr=2461569836
 12 90.878471652    127.0.0.1 -> 127.0.0.1    TCP 66 [TCP Keep-Alive] 42666 > 8080 [ACK] Seq=71 Ack=851 Win=45440 Len=0 TSval=2461630505 TSecr=2461630505
 13 90.878490252    127.0.0.1 -> 127.0.0.1    TCP 66 [TCP Keep-Alive ACK] 8080 > 42666 [ACK] Seq=851 Ack=72 Win=43776 Len=0 TSval=2461660713 TSecr=2461569836
 14 100.084545506    127.0.0.1 -> 127.0.0.1    TCP 66 8080 > 42666 [FIN, ACK] Seq=851 Ack=72 Win=43776 Len=0 TSval=2461669918 TSecr=2461569836
 15 100.084651922    127.0.0.1 -> 127.0.0.1    TCP 66 42666 > 8080 [FIN, ACK] Seq=72 Ack=852 Win=45440 Len=0 TSval=2461669919 TSecr=2461669918
 16 100.084660640    127.0.0.1 -> 127.0.0.1    TCP 66 8080 > 42666 [ACK] Seq=852 Ack=73 Win=43776 Len=0 TSval=2461669919 TSecr=2461669919
```

### nginx.conf

```
user  nginx;
worker_processes  1;

error_log  /var/log/nginx/error.log info;
pid        /var/run/nginx.pid;

env FORCE_KEEPALIVE_IDLE=30;
env FORCE_KEEPALIVE_INTVL=5;
env FORCE_KEEPALIVE_CNT=2;

events {
    worker_connections  1024;
}

http {
    upstream backend {
        keepalive  1000;
        server     localhost:8080;
    }

    client_header_timeout  100s;
    client_body_timeout    100s;
    keepalive_timeout      100s;
    keepalive_requests     1000;
    proxy_read_timeout     100s;
    proxy_send_timeout     100s;

    server {
        listen       80 so_keepalive=60s:5:2;
        server_name  localhost;
        location / {
            proxy_http_version  1.1;
            proxy_set_header    Connection "";
            proxy_pass          http://backend;

        }
    }

    server {
        listen       8080;
        server_name  localhost;
        location / {
            root  /usr/share/nginx/html;
        }
    }
}
```

