# Server-Client
Built a client and a server that implemented a task management protocol.


To run:
run the makefile 
run ./jobserver
run ./jobclient 127.0.0.1

now you can use some command on the client which will be sent to the server to be executed.

run {filename} (arg1, arg2,....} .  - runs the file in the folder jobs

jobs - displays a list of jobs currently running 

kill {pid} - kills the running pid process

watch {pid} - for another client to get updates about the pid running, if you type watch {pid} for a second time then it will stop getting updates fromt the pid

exit - this just ends the client process and will notify the server.

if the server pid is killed then it will exit cleanly (free all memory and and kill any jobs it was running).


