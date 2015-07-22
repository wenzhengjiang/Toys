## Echo

Echo server implemented with poll().

### Make and Run
```
make
./echo
```

### Test
```
### simple data test
telnet localhost 9999

### large data test
./cp1_check 127.0.0.1 9999 1000 10 2048 500

```
