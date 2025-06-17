#!/bin/sh

## Watch VSPDriver activity by log stream
#filter="'VSPDriver|VSPUserClient|VSPSerialPort'"
filter="grep VSP"
sudo log stream --level debug --color always --style compact \
  --predicate 'process == "kernel"' | ${filter}
#'VSPDriver|VSPUserClient|VSPSerialPort'
