How to Set up Space Sweeper 
====
Everything you need to build and play Space Sweeper is included inside the CCDK.
The files containing the source code, images, sound effects, and music are listed below.

~~~
CCDK/games/ss
~~~

### Setup
A backend server is required to play Space Sweeper in multiplayer mode. (To build the backend server, please refer to the “CCDK Setup” document (Setup.en.md), as the procedure is skipped here.)

Space Sweeper runs on MacOS X and Windows.

There are three steps to creating a build:

1.	Receive the external module from git
2.	Build the external module (moyai)
3.	Build Space Sweeper

The first three steps will allow you to play the game in single-player mode.
To play in multiplayer mode, three more steps are required:

4.	Start the Redis server
5.	Start the backend server
6.	Start Space Sweeper

Space Sweeper can be played via streaming by using the MCS (Minimum Cloud Set):

7.	Stream Space Sweeper over MCS


### 1. Receive the External Module from git

Space Sweeper was created with a simple rendering library called [moyai] ( https://github.com/kengonakajima/moyai ).
Moyai is required to build Space Sweeper.

If you’re using Github for Windows, moyai should automatically be downloaded when cloning the CCDK repository.
To get moyai via the command line, enter the below command from the CCDK directory after cloning it: 
~~~
git submodule update --init --recursive
~~~

This will retrieve moyai into the following location:

~~~
CCDK/externals/moyai
~~~


### 2. Build the External Module (moyai)

This information is contained in the README file in moyai (https://github.com/kengonakajima/moyai); however, the procedure for Windows is as follows.

Visual Studio 2012 or 2013 is required.

1. Open moyai/demowin/demowin.sln
2. Build "demowin" project
3. Start “demowin” project

In the Windows version of moyai, there are four build targets, which are DebugD3D, DebugOGL, ReleaseD3D, and ReleaseOGL.
DirectX is used for D3D, and OpenGL is used for OGL.
Please use DebugD3D or ReleaseD3D when using the CCDK.

The procedure for MacOS X is as follows.

~~~
$ cd externals/moyai
$ make
$ ./demo2d
$ ./demo3d
~~~

OpenGL can be used for MacOS. Debug information is valid in default mode.

With the above procedure, it succeeds if the screen which can be seen in github at moyai repository. //colin Any idea what this is?


### 3. Build Space Sweeper

The procedure for Windows is as follows.

1. Open CCDK/games/ss/sswin/sswin.sln
2. Build sswin project
3. Start sswin project (press F5)

In the default mode, Space Sweeper starts with the assumption that the backend server is on localhost.
If the game starts without a backend server, the message “FATAL ERROR” will appear.
The game will be unstable after this, and may crash.

You can play the game locally by adding the “—offline” parameter to the sswin runtime.
A build is successful when the FATAL ERROR message is displayed.

The procedure for MacOS X is as follows.

~~~
$ cd CCDK/games/ss
$ make depend
$ make
$ ./ss
~~~~

“./ss” at the tail end runs Space Sweeper game.
Offline play can be played by changing to  “./ss –offline”.

### 4. Start the Redis Server

To start Space Sweeper in multiplayer mode, a backend server is required.
The backend server saves information by using a Redis server. 
Please refer to [CCDK Setup] (Setup.en.md) for information on the Redis server and how to start the build.

### 5. Start the Backend Server

The procedure to start the backend server is explained in [CCDK Setup] (Setup.en.md). Additionally, database functionality is required for Space Sweeper, so add the “database” option when starting the backend project located at CCDK/CCDK.sln.

Start the backend server with these parameters:

~~~
realtime database --maxcon=10
~~~

The redis server should be activated at localhost for the backend server.


### 6. Start Space Sweeper

If both the Redis server and the backend server are running on the same machine, start Space Sweeper.
Start with the following parameters:

~~~
--dbhost=localhost --rthost=localhost --username=nobita --debug
~~~

The backend server address does not need to be specified if running on localhost, so the following parameters can be used in this instance:

~~~
--username=nobita --debug
~~~

After successfully starting, the character creation screen will appear.

![SpaceSweeperCharMake](images/ss_charmake.png)

The game is now ready to run!


List of Command Line Parameters
====

- ```--username=STRING ```  Set the user name. This name corresponds to save game data, and needs to be exclusively controlled at a higher directory.
- ```--dbhost=HOSTNAME```  Set the address to the backend server’s database.
Example: "localhost", "192.168.11.2"
- ```--rthost=HOSTNAME``` Set the address to the backend server’s realtime server (for game packet synchronization).
- ```--debug``` Activate debug commands, such as adding items or warping.
- ```--autoplay``` Play automatically. (Stress testing)
- ```--autocharge``` Automatically recharge player energy when playing.
- ```--speedwalk```  Move faster. For debugging.
- ```--infitems``` Infinite items. For debugging.
- ```--jsdebug```  Log the joystick at all times.
- ```--ncloc``` Log nearcast at all times.
- ```--skip-wm``` Skip the world map screen.
- ```--offline``` Play in single-player mode without the backend server.
- ```--fps=NUMBER``` Manually set the frame rate.
- ```--long-db-timeout``` Set the timeout for the database. This is required when playtesting over a slower network, like a wireless LAN.
- ```--save_max_concurrent=NUMBER``` Adjust maximum number of concurrent database saving request sent to backend server. Default value is 128 and this is suitable for SSD servers. Use 5~20 for HDD servers. This also depens on network speed/latency.


### 7. Stream Space Sweeper on the MCS (Minimum Cloud Set)
It’s easy to stream Space Sweeper over the MCS. Simply follow the instructions below.

1. Start mcs/ShinraDevelopmentStation.exe 
2. Create a new project
3. Add CCDK/games/ss to the Data Pack
4. Add Startup. Assign the Data Pack that was added in step 3, and assign sswin/ReleaseD3D/sswin.exe as the executable
5. Add any additional parameters (optional)
6. Start the game from the Startup setting

If Space Sweeper starts with the MCS’s default settings, the stream delay can be over 400 milliseconds, which is too much latency to be enjoyable from a gameplay perspective.
In this case, please try adjusting the following properties under the Startup setting.

-  Change the value (1~100) in ```Local.VideoQuality``` to a smaller number in the range of 5 to 10.
-  Increase the value (default of 2) in ```Local.EncoderThreads``` to 4. 

This may improve the stream delay; try different combinations of these values in your environment.