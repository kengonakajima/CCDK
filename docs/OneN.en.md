Setting Up the 1:N Skeleton
====

The [CCDK Setup Procedure](./Setup.ja.md) outlines how to set-up the skeleton code for the 1:1 and N:N architectures. This document explains the setup procedure for 1:N, or one-to-many (one game instance to N players).

For details on the 1:N API, please refer to the [Shinra Game API](../mcs/Doc/API/ShinraGameAPI.md) documentation.


Pointers for 1:N Set-Up
====

1:N can be set by following the procedure below:

Unlike 1:1 or N:N, with the 1:N skeleton you *must* stream to actually play your game. Therefore, the process cannot be handled entirely within VisualStudio; You will need to modify the project using the [Shina Development Station](ShinraDevelopmentStation.md) as well:

1. Confirm that the 1:1 skeleton can be played via streaming
2. Build the 1:N skeleton in VisualStudio (for the first time)
3. Create a one_to_many project using Shinra Development Station.
4. Build the 1:N skeleton in VisualStudio again (for a second time)
5. Run a debug process on the 1:N skeleton from VisualStudio

<B>Step #1. Confirm that the 1:1 skeleton can be played via streaming</B>

This process has already been covered in the [CCDK Set-up Procedure](Setup.ja.md). If the set-up for 1:1 has already been done, you may skip this procedure.

<B>Step #2. Build the 1:N skeleton in VisualStudio</B>

(CCDK/CCDK.sln) includes a project named “one_to_many”. Although the building process for one_to_many itself should have been completed when the entire solution was built during setup for the 1:1 skeleton, please build this project by itself to make sure.

After the building process for the 1:N skeleton project, set the event as follows:

~~~
$(ProjectDir)..¥..¥mcs¥python¥shinra.py setup one_to_many debug aeris 55000 60000
~~~

However, in order to run this script properly, the game needs to be deployed at least once using the SDS (Step #3). For this first round of building, toggle “Use in building process” to “No”. If you set it to “Yes”, then it will cause a building error.

<B>Step #3. Create a one_to_many project using the Shinra Development Station</B>

Launch “ShinraDevelopmentStation.exe” (SDS) and create a new project. You can locate the project anywhere, but for learning purposes you should name it "one_to_many". By saving the project, you will create “one_to_many.shinra”.

For “DataPack”, designate the default directory “CCDK/Debug”. Debug directory is automatically created
when you built the program by Visual Studio with Debug configuration.

In addition to Debug directory, you have to add "assets" directory as well.
You can add it by simply clicking "New" button at the right of "Alias".
You browse your folders and choose "CCDK/skeletons/assets" and then the directory is automatically added to the top of your tree.
After that, you can see Debug and assets in content of the DataPack.

Then set “debug” as the ID for “Startup config” and set 4 as the number for MultiConnection (as shown in the diagram below):

<img src="images/sds_1n_setting.png"></img>

The executable is created through a debug-building process. Designate “Debug¥one_to_many.exe” as it is.

This completes the project creation process. Save and start a "debug”, and you should see the following window:

<img src="images/sds_1n_starting.png"></img>

It displays 4 lines, which means that up to 4 people can play the game simultaneously.

You will see "Start Game" on the very top line (the line also includes the username, which is "aeris" in the diagram). First, try the process without ticking the “Start Client” box. When you do that, a blank window should be be displayed just like in the “one_to_one” setup. This is the game server for 1:N.

Now press “Start Client” on the line for "aeris". The client connects to the game server and calls “Game::addPlayer”, which is located in “one_to_many.exe program(Game.cpp)”. After this, it displays rendering text before initializing the stream. You will know the operation check has succeeded when the counter on the screen starts increasing.

A new game server does not need to be launched for the second player (or any other players afterward). Simply initiate a new viewport by pressing “Start Client” sequentially, without changing the port number. The counter on the second and subsequent screens will increase, but you can see that the counter figure is different on each screen; this is how you can check that the game server process is successfully rendering different content for each render target.

<B>Step #4. Build the 1:N skeleton in VisualStudio (again)</B>

It is cumbersome to manipulate the GUI of Shinra Development Station every time you go through the debugging process. 

So, this time, re-enable the event “Use in building process” that we previously disabled in Step #2. With this setting enabled, the “shinra.py" script executes the setup command, then automatically reconstructs the “one_to_many” project after the building process, and finally replaces the previous version of the .exe with a new file created in the building process.

This script creates a directory named “one_to_many” inside the MCS game install directory. It stores all the required data here, including the fake D3DX DDL that drives streaming play and the necessary JSON files, etc. Now you can set this process up to activate your game via the start-up configuration named “debug”. The default values should be “aeris” for the username, 55000 for the game port, and 60000 for the video port.

For a detailed explanation of how to use “shinra.py”, please refer to [shinra.py reference] (../mcs/Doc/MCS_README.md).

<B>Step #5. Run a debug process on the 1:N skeleton in VisualStudio</B>

After following Step #4, the setup will be done for you every time, but you will still need to go through the bothersome GUI manipulation processes in VisualStudio or Explorer in order to execute a debug process (using the debugger). You can automate this process by setting the precise file to be debugged; the exact file you want to assign here is the .exe file created by the script in Step #4.

By default, the game will be installed in this location:

~~~
C:¥Shinra¥Games¥one_to_many
~~~

So in the VisualStudio debug settings, enter this as the target command:

~~~
C:¥Shinra¥Games¥one_to_many¥Debug¥Debug¥one_to_many.exe
~~~

This will make all processes using the debugger much easier.