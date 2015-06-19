# Shinra Development Station
The Shinra Development Station (SDS) is a graphical interface tool intended to ease:
- Shinra project creation and edition
- Execution of games using the Minimal Cloud Set (MCS)
- Project deployment and execution with the Shinra Command Center (SCC).

## SDS
The interface is composed of three main elements:
- **The menu bar** on the top gives access to most of the important actions like project loading and saving, or configuration settings of SDS, MCS and SCC connection.
- **The project tree** on the left gives a representation of the different elements of the project including the data pack and the startup configurations. Those two concepts are detailed in the "Shinra Project Structure" section below. You can select individual element in the tree to have access to its properties and edit them.
- **The contextual properties** on the right will display a set of properties you can edit. Those properties will vary depending on the selected item in the project tree.

### SDS Configuration
SDS is mostly a front end for different underlying tools, but the application itself
requires some settings to access these tools. You can edit the SDS settings by clicking "Settings" and "SDS Configuration" in the main menu bar.

**Python executable** is the path to python.exe used to execute the MCS Shinra script.
Python 3 is required.

**Shinra script path** is the path to the shinra.py script used for packaging, game
installation and execution. By default the path is deduced from a relative path from
the SDS executable.

**Default game user id** is the user id that will be used by default when creating new
game instances.

The SDS configuration is saved in the user's settings in a json format: %APPDATA%/Shinra/SDS/Settings.json

### Project edition
The main aspect of SDS is to create and edit project files.

#### Shinra Project structure
A Shinra project is composed of two main parts: the **data pack** and the **startup configurations**, and an optional **reporting system**.
A data pack allows to define a directory containing the data that will be included in
the game deployment. A startup configuration allows to define how a game has to be
executed, and how MCS has to be configured for this execution. The paths for executable
and work directory are relative to the root of the a data pack, so your need to specify
which data pack to use for each startup configuration. The normal flow of configuration
is to first create a data pack, and then to crate a startup configuration using an executable contained in this data pack.
The file hooks configuration is specific to MCS. You can check the "File hook" section in the MCS_REAMDE documentation for more details.

See ["Shinra project file format" in MCS_README](MCS_README.html#ShinraProject) for more information on the shinra project file format.

#### Create a data pack
You can create a new Data pack by three different means:
- Click "Add data pack" in the Project menu.
- Right click on the "Data packs" section in the project tree and select "Add data pack".
- Drag and drop a directory containing the data on the project tree.

Once you have your Data pack created, you can configure the different properties:
- **Id** is string used to identify this data pack from the others. It can be configured to your liking.
- **Version** is a string used only for tracking purpose.
- **Directory** is the path where the data you want to include is located.
- **Alias** is the name of the directory under which the data will be deployed. By default it is the same as the original directory name, but it can be changed for convenience.

#### Create a startup configuration
You can create a new Startup configuration by three different means:
- Click "Add Startup configuration" in the Project menu.
- Right click on the "Startup configurations" section in the project tree and select "Add Startup configuration".
- For a given Data pack, right click on an executable file you want to start and select "Add startup configuration".

Once you have your Startup configuration created, you can configure the different properties:
- **Id** is a string used to identify this startup configuration form the others.
- **Executable** is the path to the executable inside the data pack.
- **Arguments** will be used when running the executable.
- **Work directory** will be used as the current work directory for the execution. The path is relative to Data pack itself.
- **Data pack** will be used as the reference for path definition.
- **Save data** is a list of Path filter expressions (or file hooks) used to define what game file needs to be redirected and saved on the end of a game execution. The data will be restored on the next execution of the same game for the same user. See ["File hooks" section in MCS_README](MCS_README.html#FileHooks) for more information on the file hooks configuration and inner workings.
- **Temp data** is a list of Path filter expressions (or file hooks) used to define what game file needs to be redirected and discarded on the end of a game execution. See ["File hooks" section in MCS_README](MCS_README.html#FileHooks) for more information on the file hooks configuration and inner workings.
- **Custom properties** allows to define specific values for any property in the CloudProperty file of a game instance. This is mostly used as a workaround to set some specific options required for some legacy games. See ["CloudProperties configuration file" section in MCS_README](MCS_README.html#CloudProperties) for more information on the property file, and its possible options.

Note you can drag the files from the data pack preview window to the text fields.
You can also have multiple startup configurations pointing at the same executable in the same data pack and using different arguments.

### Project packaging
SDS allows to package a project in a zip file that will contain the files from the datapack, and the required information to deploy the project on the Shinra servers. Note it is also possible to deploy a Shinra pack or on another workstation. 
You can create a ShinraPack in two different ways:
- In the Project menu select "Build ShinraPack".
- Right click on the project item in the project tree and select "Start game".

### Project import
A packaged project can be re-imported to create a new Shinra project. To import a project click File and Import project. This will open the import dialog box. You need to specify:
- Import from package: The zip file containing the packaged project.
- Store data in: A directory where the project data will be extracted to.
- Project file: The name of the new project file to be created.

### Reporting
SDS can be configured to interact with a reporting system to create issues. The Reporting configuration is composed in two parts:
- The SDS configuration, where you can define Report Commands that will be used to create issued. For each Report command you can also define a set of variables that have to be defined on the project level.
- The Project configuration, where you define which Report Commands to use, and define project specific variables required by the selected command.

#### Report commands configuration
To add a reporting system you need to define a ReportCommands section in the file %AppData%/Shinra/SDS/Settings.json file. Below is an example definition for github.
For the moment SDS doesn't provide a UI to edit those values, so you'll have to edit 
the settings file manually.

```
{
  "ReportCommands": {
    "github": {
      "Command": "{ProjectURL}/issues/new?title={Title}&body={Body}",
      "Type": "url",
      "Variables": {
        "ProjectURL": {
          "Default": "https://github.com/user/project",
          "UrlEscape": false,
          "Multiline": false
        },
        "Title": {
          "Default": "[{GameId}]",
          "UrlEscape": true,
          "Multiline": false
        },
        "Body": {
          "Default": "GameId={GameId}\nUserId={UserId}\nVideoPort={VideoPort}\nGamePort={GamePort}\n",
          "UrlEscape": true,
          "Multiline": true
        }
      }
    }
  }
}
```

- **ReportCommands**: Root property for the report commands definitions.
- **ReportCommands.[commandId]**: Each command is identified by a unique id. This id will be used in the project configuration to specify.
- **ReportCommands.[commandId].Command**: This defines the command line that will be executed to create a issue. This command can be a URL that will be opened in the default web browser.
- **ReportCommands.[commandId].Type**: Specify the type of command. If set to "url", the command will treated as a URL and opened in the default web browser. If set to "command", the command will be executed as a command line. Not when running as a "command" the different Command variables, and [Game instance variables](#GameInstanceVariable) will be defined as environment variable and accessible to you script or executable.
- **ReportCommands.[commandId].Variables**: Root property for the list of variables required by the command. Those variables will be defined for each project.
- **ReportCommands.[commandId].Variables.[variableName].Default**: Default value for the variable.
- **ReportCommands.[commandId].Variables.[variableName].UrlEscape**: Boolean that defines if the variable content should be escaped before being set in the command line, or in the environment.
- **ReportCommands.[commandId].Variables.[variableName].Multiline**: Boolean that defines if variable edition in the project UI should be single-line or multi-line.

#### Project report configuration
On the project side, you can use SDS UI to specify which Report command to
use for the project, and define the values for each variable required by this command.
You have access to the [Game instance variables](#GameInstanceVariable) when defining the
variable values. Report variables cannot refer to each other.
Below is an example of a report configuration section for a project file.

```
{
  "reportConfig": {
    "Id": "github",
    "Variables": {
      "ProjectURL": "https://github.com/aterrienne-eidosmontreal/testing",
      "Title": "[{GameId}]",
      "Body": "GameId={GameId}\nUserId={UserId}\nVideoPort={VideoPort}\nGamePort={GamePort}\n"
    }
  }
}
```

### <a name="GameInstanceVariable"></a> Game instance variables
When defining a report command line, or a report variable value, you have access to
a set of game instance variables:
- {GameId} : Id of the game instance.
- {UserId} : Id of the user playing the game.
- {VideoPort} : Video port used for this game instance.
- {GamePort} : Game port used for this game instance.
- {GameDumpPath} : Path to the minidump file of the game (.mdmp). May be empty id game didn't crashed.
- {UserDataPath} : Path the the user game data archive.
- {UserLogsPath} : Path to the user error log file. Not this file is user specific, but not game specific.
- {ExitCode} : Error code returned by the game executable.


## MCS
MCS is the underlying tool used to run games locally. SDS provides a convenient interface to configure MCS, install and run games from Shinra projects.

### MCS Configuration
MCS has a set of configuration that can be edited using SDS interface.
You can check the MCS configuration by clicking "MCS Configuration" in the Settings menu.

**Shinra MCS path** must be set to the directory where the MCS package was unzipped. This directory must contain the various executables and Dlls required to run a game in MCS environment. SDS will automatically create links and configuration files to run the game for you.

**User data work dir** will be used as the root directory where the user data will be stored during game execution. For more information on how MCS is handling user data see ["File hooks" section in MCS_README](MCS_README.html#FileHooks) documentation. Make sure it points to a directory where you have proper write access, and enough disk space to receive the files from the data packs.

**User data archive dir** will be used as the root directory where the user data will be archived between two game executions. For more information on how MCS is handling user data see ["File hooks" section in MCS_README](MCS_README.html#FileHooks) documentation. Make sure it points to a directory where you have proper write access, and enough disk space to receive the files from the data packs.

**Games installation dir** will be used as a root directory to deploy the game files locally and perform execution. Make sure it points to a directory where you have proper write access, and enough disk space to receive the files from the data packs.

When **Force overwriting of game data on install** is unchecked, and re-installing a game, only the data with a more recent changed date, or with a different file size will be overwritten. This enables to avoid unnecessary copy of big files when re-installing a game. You can disable this feature and force overwriting by checking this option.

When **Cleanup unused files on install** is checked, the installation of a game will delete all files in the destination directory that is not part of the datapack being installed.

The **Local execution options** are settings that allow to tune the performance for running the game on MCS. See ["Properties options" section in MCS_README](MCS_README.html#Properties) documentation for more information on these options. Note those settings are for MCS execution only and won't be used on the Cloud server deployment.   

The MCS configuration is saved in json format at %APPDATA%/Shinra/SDS/Settings.json.

### Game execution
You can start a MCS game using SDS in two different ways:
- In the Project menu select "Start game", then select the startup configuration you want to run.
- Right click on a given Startup configuration and select "Start game".

When starting a game, the game data will be deployed in the directory specified under the "MCS configuration", and then executed there. Once the installation is over you will be prompted the Game running window.

### Game running window
When running a game you are presented with a window enabling to start several instances of the game. Each instance is composed of:
- **User id** to be used for this particular instance of the game. Note we cannot have multiple instances of the same game with the same user id. Each instance has to have a specific user id.
- **Game port** to be used for this particular instance of the game. The port must be available and not conflict with the other ports of other game instances.
- **Video port** to be used for this particular instance of the game. The same restrictions than for the Game port applies here.
- **Game** start button. This will start the game instance itself with the specified user id, game port and video port. The "start client" check-box defines if the ShinraClient will be automatically started at the same time than the game instance. Uncheck this option if you want to run the client manually.
- **Create issue** button. This button is enabled only if the project has a reporting configuration. Click on this button to execute the report command associated to your project. The report variables will be set for the particular game instance. The "create on game error" check-box allows to automatically run the Report command in case the game quits with an error status.

You can add a new game instance by right clicking in the list and select "Add game instance". You can remove a game instance by clicking on a instance and hit delete key.


## SCC
The Shinra Command Center is a http rest service that serves as the interface to deploy
and manage games on the Shinra cloud services.
SDS allows to connect to SCC and perform those tasks easily.

### SCC Configuration

**Server URL** is the url to the SCC service to use.

**Proxy URL** is an optional Url SDS will use as proxy for http requests. By default SDS uses the proxy settings of the system, but in case you need specific proxy setting.

**Api Key** is a key provided to you by the Shinra team. It is private to your development team. This key is required by SCC to allow requests execution.

**Refresh time** is the time between to refresh of the VM status. SDS UI will automatically request the status for the project VM. Set this to 0 to disable the automatic refresh.

### Upload a project
To upload a project, you first need a complete project and a valid SCC
configuration including an Api Key. You are encouraged to first test your game with MCS
and make sure everything works and the configuration is correct before attempting
an upload. Note the upload process can be very long.
The upload process is composed of three steps, each can be time consuming depending on the size of the project:
- **Package building.** A new package is created from the project for upload.
- **Package upload.** The package is uploaded to SCC using http.
- **Package installation.** After SCC receives the package, the installation process starts. A new VM is created and the game data are installed on it. You will see the status of the VM going to "installing". When the installation ends. 

### Start a VM
Once a project has been uploaded and installed, the VM is in state "ready".
From this point you can start the VM using "Start" button.
The "Clients password" field enables to define a password the clients will have to
use to connect to the game. By default, any client with the proper connection information (ip and port) can connect to a game. You can restrict the access to only
client using the password.
This password can only be set when starting the VM. If you want to change the password you will have to stop the VM first and re-start it with the new password.

When you click on Start, the VM goes in "starting" status while the system is booting. When the VM ends its boot process it will go to "running" status. You will also be
displayed to the connection information for your game (IP address and ). From this point you can start clients to play the game.

### Stop a VM
When the VM is in state "running" you can stop it using the "Stop" button. This will shut the VM down and stop all the current game instances. Once the stopping process is
done the VM is back in "ready" state. You will also have access to a link to download
a zip file containing the VM logs.

### Run client manually
When a VM is running you have access to the game connection information composed of:
- game server IP
- game server port
You can  use these information to manually run a client. Note if the server uses a
simple authentication password, you will also need to specify it in command line.
Following is an example connecting to a server on Ip 192.34.56.88 and port 16666, using password MYPASS:

```
ShinraClent.exe -l 192.34.56.88:16666 -sa MYPASS
```
