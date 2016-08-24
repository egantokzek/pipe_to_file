# pipe_to_file
Simple utility to overcome the logrotate copytruncate potential data loss.
# What is the copytruncate
copytruncate allow log rotate operations on files that are continuously veing updated. The copytruncate removes the need for re-opening the log file after a rotate operation.
# copytruncate problems
However, the copytruncate may potientialy lose the data between the copy and the truncate operation. This can be solved by configuring logrotate to send a signal to the process that generates the log file, that process must re-open the log file upon receiving the signal.
It's not always possible to modify the process that generates the log
# Solution
The pipe_to_file is a simpl solution that tries to overcome the condition just explaned.
It reads continuously from a pipe and updates a log file under logrotate control. It listens for HUP signals in order to re-open the log file while preserving data that is generated in between
# Example
The following is an example of how to use pipe_to_file
1) Start pipe_to_file (the named pipe is automatically created if not exists)
pipe_to_file -p pipe_to_file.pid named_pipe log_file.txt
2) Configure logrotate
log_file.txt {
 daily
 rotate 7
 missingok
 ifempty
 compress
 compresscmd /usr/bin/bzip2
 uncompresscmd /usr/bin/bunzip2
 compressext .bz2
 dateext
copytruncate
postrotate
    /bin/kill -HUP `cat pipe_to_file.pid 2>/dev/null` 2>/dev/null || true
endscript
}
