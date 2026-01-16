#!/bin/sh

## Watch VSPDriver activity by log stream
filter="VSPDriver|VSPUserClient|VSPSerialPort"

sudo log stream --level debug \
	--color always \
	--style compact \
	--predicate 'process == "kernel"' | egrep ${filter}
