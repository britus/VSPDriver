sudo log erase ; sleep 2 ; \
sudo log stream --style compact --color always \
	--backtrace --debug --info --process 0 | egrep 'error|timeout|VSP|IOUser'
