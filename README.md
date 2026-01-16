## VSPDriver - Virtual Serial Port Driver

This is a macOS DriverKit driver implementation including a user interface App.
 
### Release

- [All releases](https://github.com/britus/VSPClient_Releases)

### Log Files and Tester

Create a small script which filter all VSPxxx messages.

```
#!/bin/sh

## Watch VSPDriver activity by log stream
filter="VSPDriver|VSPUserClient|VSPSerialPort"

sudo log stream --level debug \
    --color always \
    --style compact \
    --predicate 'process == "kernel"' | egrep ${filter}
```

