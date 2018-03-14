# jms
Second project for syspro class, spring semester, 2017
This is a job submitting management system (jms), implemented for the syspro class, spring semester 2017, di-uoa. Assignment file (greek) to be included.

The whole of these source files implement a job management system described in the assigmnmet file. In short, it is like linux bash:a user can submit jobs (unix commands/programs), query their status, change said status (suspend a process, activate anew a suspended process, shutdown the whole jms etc).

Basic source files are jms_console.c and jms_coord.c. There are also pool.c, jobsCatalogue.c and poolList.c. The first three source files implement the console, coordinator and pool logic respectively, while the latter two implement useful data structures. 

Coordinator:
Handles all the jobs submitted by the user. Said handling includes submitting the jobs and getting information about their status (whether they are still active or finished for example). The Coordinator does not manipulate the jobs by itself; it rather uses any number (dynamically generated, according to the program's needs) of job pools, with each pool being able to handle up to a number of jobs (the user gives this number a input. Once a pool has reached its max number of jobs, and these jobs have finished, hands statistic info to the Coordinator and exits.

Console:
User interface program. Allows the user to submit the commands they want, and communicates that input to coordinator through named pipes.

In a unix machine, you need to launch jms_console and then, in another terminal, jms_coord. The commads are:
unixprompt>./jms_console -w <jms_in> -r <jms_out> -o <operations_file>   (the last argument is optional)]
unixprompt>./jms_coord -l <path> -n <jobs_pool> -r <jms_in> -w <jms_out>
  
Arguments explanation:
<path>:jms maps each process' stdout and stderr to output files, in a separate directory for each process. The path argument indicates the path for these directories.
<jobs_pool>: the max number of jobs that can be submitted to a pool
<jms_in>: the name of the named pipe for the console to write data to the coord
<jms_out>: the name of the named pipe for the coord to write data to the console
  
Commands Repertoire:

1.submit <job>

2.status <jobID>

3.status-all

4.show-active

5.show-pools

6.show-finished

7.suspend <jobID>
  
8.resume <jobID>
  
9.shutdown

jobsCatalogue: A skip list of all the jobs submitted.
poolList: A list of all the pools. Insertion occurs at the beginning of the list for efficiency; since the newest pools are expected to be queried more often.










