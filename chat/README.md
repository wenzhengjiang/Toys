## Chat
A simple chat program implemented with poll(). 

### Commands

list : list all online users
conn #id : start talking to #id
EOF : exit

### Make and Run
```
> make
> ./chat
----- Chat Server -----
listening on port 9999
```
### test
```
> telnet 127.0.0.1 9999
list
0:localhost:59055
1:localhost:59054(me)
conn 0
start talking to localhost:59055
hello
>hello
88
>88
stop talking to localhost:59054
```

```
> telnet 127.0.0.1 9999
start talking to localhost:59054
>hello
hello
>88
88
stop talking to localhost:59055
```
