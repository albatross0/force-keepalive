# How to build

```
$ gcc -Wall -shared -fPIC -Wl,--no-as-needed -ldl -Wl,-soname=force-keepalive-connect.so -o force-keepalive-connect.so force-keepalive-connect.c
```

