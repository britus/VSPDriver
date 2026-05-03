# VSPDriver - Virtual Serial Port Driver

This is a macOS DriverKit driver implementation including a user interface App.

## Get Started

- [The app in depth](https://github.com/britus/VSPDriver/blob/master/README_usage_en.md)

## Beta / Release Archive

- [All releases](https://github.com/britus/VSPClient_Releases)

## Log Files and Tester

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

## Serial Port List
![Serial Port List](https://github.com/britus/VSPDriver/blob/master/Screens/screen_1.png)
## Port Link List
![Port Link List](https://github.com/britus/VSPDriver/blob/master/Screens/screen_2.png)
## Port Link List (multi target S<>L+L)
![Port Link List (multi target S<>L+L)](https://github.com/britus/VSPDriver/blob/master/Screens/screen_2a.png)
## Port Link List (multi target S<>L<>L)
![Port Link List (multi target S<>L<>L)](https://github.com/britus/VSPDriver/blob/master/Screens/screen_2b.png)
## Driver Options
![Driver Options](https://github.com/britus/VSPDriver/blob/master/Screens/screen_3.png)
## Test Terminal
![Test Terminal](https://github.com/britus/VSPDriver/blob/master/Screens/screen_4.png)
## Test Terminal with scripting
![Test Terminal with scripting](https://github.com/britus/VSPDriver/blob/master/Screens/screen_5.png)
## Test Loop with CoolTerm (multi target)
![Loop with CoolTerm](https://github.com/britus/VSPDriver/blob/master/Screens/screen_6.png)
