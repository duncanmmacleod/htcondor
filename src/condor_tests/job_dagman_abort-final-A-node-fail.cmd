executable     	= x_dagman_retry-monitor.pl
arguments		= 5
universe        = vanilla
notification   	= NEVER
getenv         	= true
output			= job_dagman_abort-final-A-node.$(nodename).out
error			= job_dagman_abort-final-A-node.$(nodename).err
queue

