# Apache Multitasking Architectures

## Overview

All Apache Multitasking Architectures are based on a task pool architecture.

**tasks**

- At Start-up
  - Apache creates a number of tasks(process/thread), most of them are idle.

- A request arrive
  - be processd by an idle task
  - therefore there’s no need to create a task for request processing

**master**

- a task controlling the task pool
  - either control the number of idle tasks
  - or just restart the server something goes wrong
- also offers the control interface for the administrator



## General Behavior

![image-20210715205220403](Apache-Multitasking-Architectures.assets\image-20210715205220403.png)

- First-time initialization:

  Allocate resources, read and check confifiguration, become a daemon.

- The restart loop

  (Re–)read configuration, create task pool by starting child server processes and enter the master server loop.

- The master server loop:

  Control the number of idle child server processes in the task pool.

- The request–response loop (Child server only):

  Wait to become leader, wait for a connection request, become worker and enter the keep–alive–loop.

- The keep–alive–loop (Child server only):

  Process HTTP requests

- Clean–up before deactivation (Master server and Child servers)

### Initialization & Restart Loop

 Three types of initializations:

- at the first activation of Apache
- every time the restart loop is run
- every time

#### Initialization 

- create static pools

- register information about prelinked modules

- read command line and set parameters

- read per–server confifiguration

- graceful_mode := false

- detach process:

  - create a clone of the current process using fork()

  - immediately stop the parent process

  - the clone detaches the input and output streams from the shell

  - the clone sets its process group ID to the group ID of process number 1 (init). It

    ’denies’ every relationship with its ’real’ parent process and from now on only

    depends on the system process init.

#### Restart Loop

- initialize and prepare resources for new child servers, read and process configuration files
-  create child servers
-  Running
  - Master server: observe child servers 
  - Child servers: Handle HTTP requests
-  kill child servers (graceful restart: kill idle child servers only)



### Inter–Process Communication (Signals and Pipe of Death)

#### Administrator controls the master server

- SIGTERM (shut down server)
- SIGHUP  (restart server)
- SIGUSR1 (restart server gracefully)

#### Master Server controls the child servers

- signal
- pipe of death



### Child Server(Worker)

- **Initialization, Confifiguration and Server restarts** 
  - establish access to resources
  - Re–initialize modules (ap_init_child_modules() )
  - Set up time–out handling
  - Within the loop, there are two initialization steps left
    - clear time–out
    - clear transient pool
  - set status := ready in the scoreboard
- **Accepting Connections** 



## MPM

![image-20210715213322823](Apache-Multitasking-Architectures.assets\image-20210715213322823.png)





## Prefork MPM

### The leader-followers pattern

**a pool of tasks, 3 roles**:

- wait for requests (listener)
- process a request (worker)
- queue in and wait to become the listener (idle worker)

![image-20210715203847529](Apache-Multitasking-Architectures.assets\image-20210715203847529.png)

### Arthitecture

![image-20210715204549523](Apache-Multitasking-Architectures.assets\image-20210715204549523.png)



## Worker MPM

![image-20210715213435421](Apache-Multitasking-Architectures.assets\image-20210715213435421.png)

### WinNT MPM

![image-20210715213611788](Apache-Multitasking-Architectures.assets\image-20210715213611788.png)

