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

