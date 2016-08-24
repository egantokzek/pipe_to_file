# pipe_to_file
Simple utility to overcome the logrotate copytruncate potential data loss.
<br>
<b>What is the copytruncate</b>
copytruncate allow log rotate operations on files that are continuously veing updated. The copytruncate removes the need for re-opening the log file after a rotate operation.
# copytruncate problems
However, the copytruncate may potientialy lose the data between the copy and the truncate operation. This can be solved by configuring logrotate to send a signal to the process that generates the log file, that process must re-open the log file upon receiving the signal.
<br>
Unfortunately it's not always possible to modify the process that generates the log.
<br>
# Solution
The pipe_to_file is a simpl solution that tries to overcome the condition just explaned.
<br>
It reads continuously from a pipe and updates a log file under logrotate control. It listens for HUP signals in order to re-open the log file while preserving data that is generated in between.
<br>
# Example
The following is an example of how to use pipe_to_file
<br>
1) Create a named pipe
<br><br>
mkfifo named_pipe
<br><br>
2) The generating process must be commanded to export the log to the created named pipe
<br><br>
3) Start pipe_to_file (the named pipe is automatically created if not exists)<br>
pipe_to_file -p pipe_to_file.pid named_pipe log_file.txt
<br><br>
2) Configure logrotate<br>
log_file.txt {<br>
daily<br>
rotate 7<br>
missingok<br>
ifempty<br>
compress<br>
compresscmd /usr/bin/bzip2<br>
uncompresscmd /usr/bin/bunzip2<br>
compressext .bz2<br>
dateext<br>
copytruncate<br>
postrotate<br>
/bin/kill -HUP `cat pipe_to_file.pid 2>/dev/null` 2>/dev/null || true<br>
endscript<br>
}
<br><br>
