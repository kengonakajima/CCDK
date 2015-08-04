# Shinra MCS
Shinra MCS は、シンラ・システムのリモートレンダラの最小のエミュレーターです。

## <a name="InstallHowTo"></a> インストール手順
CCDKにおけるインストール手順は、 [セットアップ手順](../../docs/Setup.ja.md) を参照してください。


## <a name="SDS"></a> Shinra Development Station

Shinra Development Station (SDS) は、プロジェクトを作成したり操作をするためのGUIツールです。MCSに含まれるshinra.pyというPythonによるコマンドラインツールを内部で使用して、その機能を提供します。
より詳しい使用方法については、[ShinraDevelopmentStation](ShinraDevelopmentStation_ja.md)を参照してください。


## <a name="ShinraScript"></a> shinra.py スクリプト
MCSのpythonディレクトリの中には、 shinra.pyが格納されています。
shinra.pyスクリプトは、　ゲームプログラムをパッケージしたり、インストールしたり実行したりするために使います。　シンラパックファイルを生成したり、サーバーにデプロイしたりするための自動化ツールとして使うことができます。

以下は、引数無しで起動したときに表示される説明です。


```
usage: shinra.py [-h] [--log-level {DEBUG,INFO,WARN,ERROR}]
                 [--log-file LOG_FILE]
                 {package,install,project,run,setup,cleanup,configure} ...

Shinra MCS management script.

positional arguments:
  {package,install,project,run,setup,cleanup,configure}
                        command help.
    package             Create a Shinra package from a project.
    install             Install a game from a project.
    project             Build a project from a package.
    run                 Run an installed game.
    setup               Setup a game instance for manual execution. Restore
                        the user data from the archive and create proper
                        settings files.
    cleanup             Cleanup a game instance after a game execution.
                        Archive user data and cleanup temporary files.
    configure           Set default MCS settings values for current user.
                        Configuration file for current user is C:\Users\user
                        name\AppData\Roaming\Shinra\MCS\Settings.json.

optional arguments:
  -h, --help            show this help message and exit
  --log-level {DEBUG,INFO,WARN,ERROR}
  --log-file LOG_FILE
```

- ```package``` シンラパッケージをプロジェクトから作成します。
- ```install``` プロジェクトの設定に従ってゲームをインストールします。
- ```project``` パッケージファイルからプロジェクトを構築します。
- ```run``` インストールされたゲームを実行します。
- ```setup``` ゲームインスタンスを手動実行用にセットアップします。
- ```cleanup``` ゲームを実行した後に、消去します。
- ```configure``` 現在のユーザーのためにMCSのデフォルト設定を行います。設定ファイルは、 C:¥Users¥USERNAME¥AppData¥Roaming¥Shinra¥MCS¥Settings.json に格納されます。

### Package コマンド

シンラパッケージファイルをプロジェクトファイルの設定に基いて作成します。

```shinra.py package``` とだけ入力し、オプションを省略すると以下の説明が表示されます。

```
usage: shinra.py package [-h] project archive

positional arguments:
  project     Shinra project to use.
  archive     Shinra package to create.

optional arguments:
  -h, --help  show this help message and exit
```

引数projectは、使用するプロジェクトの名称を指定します。
引数archiveは、生成するパッケージファイルの名称を指定します。

以下の例では、 package.zipを "project.shinra"ファイルから作成します:

```
shinra.py package project.shinra package.zip
```

### Install コマンド

ゲームに必要なファイルをローカルマシンの必要な位置にコピーして、実行に備えます。
実行するには後述のrunコマンドを使用し、その際には実行時用の一時ファイルなどがさらに作成されます。


```shinra.py install```とだけ入力し、オプションを省略すると以下の説明が表示されます。

```
usage: shinra.py install [-h] [-games-install-dir GAMES_INSTALL_DIR]
                         [-mcs-path MCS_PATH] [-config CONFIGURATION] [-overwrite] [-no-overwrite]
                         [-cleanup] [-no-cleanup]
                         project

positional arguments:
  project               Shinra project to install.

optional arguments:
  -h, --help            show this help message and exit
  -games-install-dir GAMES_INSTALL_DIR
                        Directory where games will be installed for local
                        execution.
  -mcs-path MCS_PATH    Path where MCS is installed.
  -config CONFIGURATION Configuration (Debug or Release) to use.
  -overwrite            Force overwrite of all files on install.
  -no-overwrite         Only files with a older modification date, or a
                        different size than the source will be overwritten on
                        install.
  -cleanup              Force deletion of files in the installation directory
                        that are not part of the datapack.
  -no-cleanup           Leave files in the installation directory that are not
                        part of the datapack.
```

project引数は、ローカルマシンにインストールするプロジェクトの名称を指定します。

以下のオプションが利用可能です:

- ```-games-install-dir GAMES_INSTALL_DIR``` ローカルでゲームを実行するときにゲームをインストールするディレクトリ名を指定します。
- ```-mcs-path MCS_PATH```  インストールされているMCSの位置を指定します。このディレクトリから、DLLなどがコピーされます。
- ```-config CONFIGURATION```  DebugやReleaseなど、どのバージョンのDLLを使用するかを指定します。主にMCSの自体のデバッグ用です。
- ```-overwrite``` インストールするときに、ファイルを強制的に上書きします。
- ```-no-overwrite``` インストールするときに、日付が新しい場合や、ファイルサイズが異なる場合だけ上書きします。
- ```-cleanup``` インストールするときに、データパックに定義されていないファイルをすべて消します。クラッシュダンプファイルなどが消されます
- ```-no-cleanup``` インストールするときにファイルを自動的に消さないようにします。

以下の例では、 ```project.shinra``` プロジェクトの設定に従ってゲームをデフォルトのインストールディレクトリにインストールして、実行に備えます。

```
shinra.py install project.shinra
```

### <a name="RunCommand"></a> Run コマンド

インストール済みのゲームを実行します。

```
usage: shinra.py run [-h] [-games-install-dir GAMES_INSTALL_DIR]
                     [-store-path STORE_PATH] [-archive-path ARCHIVE_PATH]
                     [-show-window] [-no-show-window] [-display-statistics]
                     [-no-display-statistics]
                     [-encoder-threads ENCODER_THREADS]
                     [-max-encoding-frames MAX_ENCODING_FRAMES]
                     [-max-rendering-frames MAX_RENDERING_FRAMES]
                     projectid gameid userid gameport videoport

positional arguments:
  projectid             Project id of the game to run.
  gameid                Startup id to use to run the game.
  userid                User id to use to run the game.
  gameport              Port to use for game communications.
  videoport             Port to use for video communications.

optional arguments:
  -h, --help            show this help message and exit
  -games-install-dir GAMES_INSTALL_DIR
                        Directory where games will be installed for local
                        execution.
  -store-path STORE_PATH
                        Working directory where user data will be stored
                        during game execution.
  -archive-path ARCHIVE_PATH
                        Directory where user data will be archived between two
                        game executions.
  -show-window          Show renderer window.
  -no-show-window       Hide renderer window.
  -display-statistics   Display statistics in renderer window. Ineffective if
                        -no-show-window.
  -no-display-statistics
                        Hide statistics in renderer window. Ineffective if
                        -no-show-window.
  -encoder-threads ENCODER_THREADS
                        The number of threads dedicated to encoding the video
                        stream.
  -max-encoding-frames MAX_ENCODING_FRAMES
                        The maximum number of queued encoding frames requested
                        before the game starts to block.
  -max-rendering-frames MAX_RENDERING_FRAMES
                        The maximum number of queued rendering frames requested
                        before the game starts to block.
```



projectid引数は、実行するゲームのプロジェクトIDを指定します。これはプロジェクトファイル名とは異なります。プロジェクトの設定内容として格納されています。
gameid引数は、プロジェクトのStartup設定の名称を指定します。
userid引数は、ゲームを実行するときのユーザー名を指定します。
gameport引数は、ゲームの通信(映像以外)をするためのTCPポート番号を指定します。
videoport引数は、映像を通信するためのTCPポート番号を指定します。

以下のオプションが利用可能です:

- ```-h, --help``` ヘルプを表示します。
- ```-games-install-dir GAMES_INSTALL_DIR``` installコマンドと同様です。
- ```-store-path STORE_PATH``` ゲームが生成する一時的なユーザーデータの保存位置を指定します。
- ```-archive-path ARCHIVE_PATH``` ゲームが生成するユーザーデータをどこに永続化するかを指定します。
- ```-show-window``` レンダリング状況のモニタウインドウを表示させます。
- ```-no-show-window``` レンダリング状況のモニタウインドウを表示しないようにします。
- ```-display-statistics``` レンダリングにかかった時間やドローコールの数などの統計情報を表示します。 ```-no-show-window``` のときは無効になります。
- ```-no-display-statistics``` 統計情報を表示しません。
- ```-encoder-threads ENCODER_THREADS``` ビデオエンコーディングをするときに使用するスレッドの数を指定します。
- ```-max-encoding-frames MAX_ENCODING_FRAMES``` ゲームの実行がブロックしはじめるまでにキューされる最大のフレーム数を指定します。

以下の例では、 SSWプロジェクトの ssw_debugという名称のスタートアップ設定を、aerisユーザーで、ゲームポート55000,ビデオポート60000で起動します。

~~~
shinra.py run SSW ssw_debug aeris 55000 60000
~~~

ここで指定したポート番号が、正しく ShinraClientでも指定する必要があることに注意してください。 ([Shinra Client](#ShinraClient)を参照)


### Setup コマンド

Setupコマンドは、シンラのゲームインスタンスを実行するために必要なすべての一時ファイルを含むものの準備を行います。　ゲームの実行をしないrunコマンドのようなものです。

デバッガを用いてゲームを実行するときには便利です。

setupコマンドは、CloudPropertyファイル(実行時用の設定ファイル)を生成したり、MCSのユーザー設定データを抜き出したりして必要な設定をすべてそろえます。

setupしたゲームを実際に実行するためには、MCSによって自動的に行われるいくつかのことを考慮する必要があります:


- 複数のゲームを同時に実行するためには、 ```cloud-properties``` オプションを用いて個別に設定をする必要があります。 [複数ゲームの実行](#MultipleGames) を参照してください。
- ゲームの実行ファイルに与えるコマンドライン引数
- ゲームサーバーへの接続をするために、クライアントに対して、正しいTCPポート番号を引数として与えること。 ([Shinra Client](#ShinraClient) を参照)
- ゲームを実行したあと、 ```cleanup``` コマンドを実行すること。([Cleanupコマンド](#CleanupCommand) を参照)

オプション無しで起動すると以下の説明が表示されます:

```
usage: shinra.py setup [-h] [-cloud-properties CLOUD_PROPERTIES]
                       [-games-install-dir GAMES_INSTALL_DIR]
                       [-store-path STORE_PATH] [-archive-path ARCHIVE_PATH]
                       [-show-window] [-no-show-window] [-display-statistics]
                       [-no-display-statistics]
                       [-encoder-threads ENCODER_THREADS]
                       [-max-encoding-frames MAX_ENCODING_FRAMES]
                       [-max-rendering-frames MAX_RENDERING_FRAMES]
                       projectid gameid userid gameport videoport

positional arguments:
  projectid             Project id of the game to run.
  gameid                Startup id to use to run the game.
  userid                User id to use to run the game.
  gameport              Port to use for game communications.
  videoport             Port to use for video communications.

optional arguments:
  -h, --help            show this help message and exit
  -cloud-properties CLOUD_PROPERTIES
                        File name to use for creating the cloud property file.
  -games-install-dir GAMES_INSTALL_DIR
                        Directory where games will be installed for local
                        execution.
  -store-path STORE_PATH
                        Working directory where user data will be stored
                        during game execution.
  -archive-path ARCHIVE_PATH
                        Directory where user data will be archived between two
                        game executions.
  -show-window          Show renderer window.
  -no-show-window       Hide renderer window.
  -display-statistics   Display statistics in renderer window. Ineffective if
                        -no-show-window.
  -no-display-statistics
                        Hide statistics in renderer window. Ineffective if
                        -no-show-window.
  -encoder-threads ENCODER_THREADS
                        The number of threads dedicated to encoding the video
                        stream.
  -max-encoding-frames MAX_ENCODING_FRAMES
                        The maximum number of queued encoding frames requested
                        before the game starts to block.
  -max-rendering-frames MAX_RENDERING_FRAMES
                        The maximum number of queued rendering frames requested
                        before the game starts to block.
```

ほとんど ```install``` コマンドと同じです。異なっているのは次のひとつです:

- ```-cloud-properties CLOUD_PROPERTIES``` クラウドプロパティファイル(CloudProperties) として使用するファイルの名称を指定します。


### <a name="CleanupCommand"></a> Cleanup コマンド

cleanup コマンドは、ゲームを手動で実行した後に、ゲームが生成したユーザーデータをアーカイブし、不要なファイルを消します。
このコマンドは、setupコマンドを使用した手動実行の際だけ使います。
SDSを用いた自動実行のときは使いません。

オプション無しで起動すると以下の説明が表示されます:

```
usage: shinra.py cleanup [-h] [-cloud-properties CLOUD_PROPERTIES]
                         [-games-install-dir GAMES_INSTALL_DIR]
                         [-store-path STORE_PATH] [-archive-path ARCHIVE_PATH]
                         projectid userid gameid

positional arguments:
  projectid             Project id of the game to run.
  userid                User id to use to run the game.
  gameid                Startup id to use to run the game.

optional arguments:
  -h, --help            show this help message and exit
  -cloud-properties CLOUD_PROPERTIES
                        File name of the cloud property file to cleanup.
  -games-install-dir GAMES_INSTALL_DIR
                        Directory where games will be installed for local
                        execution.
  -store-path STORE_PATH
                        Working directory where user data will be stored
                        during game execution.
  -archive-path ARCHIVE_PATH
                        Directory where user data will be archived between two
                        game executions.
```


projectid引数, userid引数, gameid引数は、それぞれ runコマンドと同様です。
これらによって、何をアーカイブすべきかを特定します。

オプションは以下が利用可能です:

- ```-h, --help``` ヘルプを表示します
- ```-cloud-properties CLOUD_PROPERTIES``` setupコマンドと同じ
- ```-games-install-dir GAMES_INSTALL_DIR``` install/setupコマンドと同じ
- ```-store-path STORE_PATH``` install/setupコマンドと同じ
- ```-archive-path ARCHIVE_PATH``` install/setupコマンドと同じ
```

ゲームのユーザーデータをセーブするときのさらに詳しい動作については、[ファイルフック](#FileHooks) を参照してください。



### Project コマンド

ゲームのシンラパッケージファイルから、プロジェクトファイルと実際のゲームデータを取り出します。


オプション無しで起動すると以下のように説明が表示されます:

```
usage: shinra.py project [-h] archive dir project

positional arguments:
  archive     Shinra package to use as source.
  dir         Directory where to store project data.
  project     Name of Shinra project file to create.

optional arguments:
  -h, --help  show this help message and exit
```

archive引数は、プロジェクト情報を取り出す元となるアーカイブファイル(zipファイル)の位置を指定します。
dir引数は、取り出したプロジェクトファイルを保存するディレクトリを指定します。
projectは、作成するプロジェクトファイルの名称を指定します。

以下の例では、archive.zipから、  `C:\Path\to\data` で指定されるディレクトリ内に、 project.shinraというプロジェクトファイルと、データパックに含まれるデータと、スタートアップ設定を取り出します。

```
shinra.py project archive.zip C:\Path\to\data project.shinra
```

### Configure コマンド

shinra.pyの各コマンドは、利用者(Windowsのユーザ)ごとに保存されている設定値を利用して動作します。設定値は、 `%APPDATA%/Shinra/MCS/Settings.json` に保存されています。
SDSの設定画面によって、この値を変更できます。
ここに保存されている値が、shinra.pyを使うときのデフォルト設定として利用されます。

オプション無しで起動すると以下の説明が表示されます:

```
usage: shinra.py configure [-h] [-store-path STORE_PATH]
                           [-archive-path ARCHIVE_PATH]
                           [-games-install-dir GAMES_INSTALL_DIR]
                           [-show-window] [-no-show-window]
                           [-display-statistics] [-no-display-statistics]
                           [-encoder-threads ENCODER_THREADS]
                           [-max-encoding-frames MAX_ENCODING_FRAMES]
                           [-max-rendering-frames MAX_RENDERING_FRAMES]
                           [-mcs-path MCS_PATH] [-config CONFIGURATION]
                           [-overwrite] [-no-overwrite]
                           [-cleanup] [-no-cleanup]

optional arguments:
  -h, --help            show this help message and exit
  -store-path STORE_PATH
                        Working directory where user data will be stored
                        during game execution.
  -archive-path ARCHIVE_PATH
                        Directory where user data will be archived between two
                        game executions.
  -games-install-dir GAMES_INSTALL_DIR
                        Directory where games will be installed for local
                        execution.
  -show-window          Show renderer window.
  -no-show-window       Hide renderer window.
  -display-statistics   Display statistics in renderer window. Ineffective if
                        -no-show-window.
  -no-display-statistics
                        Hide statistics in renderer window. Ineffective if
                        -no-show-window.
  -encoder-threads ENCODER_THREADS
                        The number of threads dedicated to encoding the video
                        stream.
  -max-encoding-frames MAX_ENCODING_FRAMES
                        The maximum number of queued encoding frames requested
                        before the game starts to block.
  -max-rendering-frames MAX_RENDERING_FRAMES
                        The maximum number of queued rendering frames requested
                        before the game starts to block.
  -mcs-path MCS_PATH    Path where MCS is installed.
  -config CONFIGURATION Configuration (Debug or Release) to use.
  -overwrite            Force overwrite of all files on install.
  -no-overwrite         Only files with a older modification date, or a
                        different size than the source will be overwritten on
                        install.
  -cleanup              Force deletion of files in the installation directory
                        that are not part of the datapack.
  -no-cleanup           Leave files in the installation directory that are not
                        part of the datapack.
```

ほとんどすべてがinstallコマンドと共通です。installコマンドの説明を参照してください。

`Settings.json` ファイルは、単純なJSONファイルで、手動で変更することができます。
以下は、設定の例です。

```
{
  "MCSPath": "D:\\Shinra\\ShinraMCS",
  "Configuration": "Debug",
  "GamesInstallDir": "D:\\Shinra\\Games",
  "StorePath": "D:\\Shinra\\Local",
  "ArchivePath": "D:\\Shinra\\UserFiles",
  "DisplayStatistics": false,
  "EncoderThreads": 2,
  "MaxEncodingFrames": 5,
  "MaxRenderingFrames": 3,
  "ShowWindow": false,
  "OverwriteGameInstall": false,
  "CleanupOnInstall": false
}
```

不必要な設定が書かれていた場合は、MCSは無視します。


## <a name="ShinraClient"></a> Shinra Client

`ShinraClient.exe` は、以下の位置にあります:

`<MCS_INSTALL>/Shinra/x32/Release/ShinraClient.exe`

ShinraClient.exeは単純にクライアントとも呼びます。
これはプレイヤーが実際に操作するクライアントアプリケーションで、
ゲームの映像と音声を再生し、プレイヤーの入力をゲームサーバーに送信します。

ShinraClientを手動で実行して、異なるコンピュータで動作しているゲームインスタンスに接続することができます。その際は、サーバーIPアドレスとゲームTCPポート、ビデオTCPポート番号を正しく設定してください。

起動パラメータは以下の通りです:


- `-f` : フルスクリーンモード(デフォルト)
- `-w` : ウインドウモード
- `-sk` : クライアントの更新チェックをしない (MCSではデフォルト)
- `-sa <password>` : パスワードを設定する
- `-tg <timeout>` : ゲームサーバから切断するデフォルトタイムアウト値
- `-tr <timeout>` : レンダラから切断するデフォルトタイムアウト値

接続先の情報は、 URLの形式で指定します:

```
shinra://[<username>@]<game host>[:<game port>]/runMCS/<game id>[?VideoPort=<video port>]
```

- `<username>` プレイヤーのユーザー名。指定しなかった場合は起動時に入力を促します。
- `<game host>` ゲームが動作しているマシンアドレス。(例: `localhost`)
- `<game port>` ゲームポート番号。(デフォルトは55000).
- `<game id>` ゲームのID。MCSでは無視されます
- `<video port>` ビデオポート(デフォルトは60000).

以下の例では、ローカルマシンで動作しているゲームにウインドウモードで接続し、ゲームポート55000,ビデオポート60000で、クライアントサイドのタイムアウト無しで接続します。
サーバにアクセスするための合言葉は、起動時に入力します。

```
<MCS_INSTALL>/Shinra/x32/Release/ShinraClient.exe -w -tr 0.0 -tg 0.0 shinra://localhost/runMCS/mygame
```

## <a name="ShinraProject"></a> プロジェクトファイルのフォーマット

プロジェクトファイルは、JSONのテキストファイルで、以下の情報を含みます

- ゲームデータをパッケージするために必要な情報 (dataPacks)
- ゲームを実行するために必要な情報 (startups)

プロジェクトファイルを操作するためには、通常はSDS(ShinraDevelopmentStation)を使いますが、プロジェクトファイルは単純なので手動で編集できます。

以下は、datapackがひとつと startupがひとつ含まれる設定ファイルの例です:


```
{
    "projectId": "DX3",
    "projectVersion": "1",
    "contentId": "DX3Eidos_vffe",
    "apiKey": "eidos_0a0908b6c94c3c880203eb3a",
    "startups": [
        {
            "FileHooks": {
                "CloudFilteredPaths": [
                    "<RoamingAppData>\\Eidos Montreal\\**"
                ],
                "TempFilteredPaths": [
                    "<LocalAppData>\\dxhr\\**"
                ]
            },
            "arguments": "-archive",
            "dataPackId": "DX3",
            "executable": "DX3.exe",
            "id": "DX3",
            "workDir": ""
            "CustomProperties": {}
        }
    ],
    "dataPacks": [
        {
            "files": [
                {
                    "aliasPath": "",
                    "fileSystemPath": "D:\\Shinra\\import\\DX3\\DX3"
                }
            ],
            "id": "DX3",
            "version": "1"
        }
    ]
}
```

それぞれの項目の意味は以下の通りです:


- **projectId**: プロジェクトのID。 シンラプラットフォームで一意にプロジェクトを特定するために使います。
- **contentId**: (必須ではない) シンラ・システムによって指定されるコンテントIDです。これは、シンラのデータセンターにゲームをアップロードするときにゲームを特定するために必要です。
- **apiKey**: (必須ではない) シンラによって提供される秘密鍵です。シンラのデータセンターにゲームをアップロードしたり、オンラインサービスを利用するときに必要です。
- **projectVersion**: 追跡用に使われるバージョン番号
- **dataPacks[].id**: データパックのID。プロジェクト内でデータパックを特定するために必要です。プロジェクト内で重複していない必要があります。
- **dataPacks[].version**: 各データパックのバージョン。追跡のために使います
- **dataPacks[].files[].fileSystemPath**: ゲーム用にデプロイされるべきファイルのファイルシステム上のパス名。相対パスは、プロジェクトファイルからの相対になります。
- **dataPacks[].files[].aliasPath**: インストールするときに、既に存在するディレクトリ名の後ろに追加するディレクトリ名。
- **startups[].id** : スタートアップ設定のID。　プロジェクト内で重複していない必要があります。 実行するときのgameIDとして使われます。
- **startups[].dataPackId** : スタートアップ設定がどのデータパックを使うかをデータパックIDで指定します。
- **startups[].executable** : ゲームの実行ファイル(EXE)の位置。データパックのルートディレクトリからの相対です。
  datapack root.
- **startups[].arguments** : 実行ファイルに与えられるコマンドライン引数。([スタートアップ引数](#StartupArgs)を参照)
- **startups[].workDir** : 実行時に使われる作業ディレクトリ。データパックのルートディレクトリからの相対。
- **startups[].FileHooks.CloudFilteredPaths** : ファイルフックフィルタが使うパスのパターン設定。([ファイルフック](#FileHooks)を参照)
- **startups[].FileHooks.TempFilteredPaths** : ゲームの一時データをファイルフックに知らせるためのパスのパターン設定。([ファイルフック](#FileHooks)を参照)
- **startups[].CustomProperties** : カスタム設定項目。([カスタム設定](#CustomProperties)を参照)


## <a name="StartupArgs"></a> Startup引数

スタートアップ設定の```arguments```項目では、
ゲームインスタンスの起動時に与える文字列を指定するだけでなく、
自動的に置換される特別なパターンを付けます。


- ```{UserId}``` ゲームインスタンスを動作させるときのユーザー名を置換します。

以下は、UserIDの置換をさせる設定の例です:

```
--debug --username={UserId} --dbhost=192.137.16.50 --rthost=192.137.16.50
```

新しいゲームインスタンスを shinra.py ( [run command](#RunCommand) ) を使って実行するときは、それが以下のように置換されます:

```
--debug --username=aeris --dbhost=192.137.16.50 --rthost=192.137.16.50
```

## <a name="CustomProperties"></a> カスタム設定

カスタム設定( CustomProperties )は、 CloudProperties.json　に強制的に設定を追加するときに使います。
CloudProperties.jsonファイルの詳細については、 [CloudProperties設定ファイル](#CloudProperties) を参照してください。

次の例では、 `Cloud.Debug.BreakOnStartup`をTrueに設定し、 
`Cloud.Local.EncoderThreads`を3に設定しています。
大文字と小文字は区別されます。


```
"CustomProperties": {
  "Debug.BreakOnStartup": True,
  "Local.EncoderThreads": 3,
}
```


## <a name="FileHooks"></a> ファイルフック

プロジェクトでは、ゲーム用のファイルフックを定義できます。

ファイルフックは、ゲームアプリケーションによるファイルシステムアクセスを、
別の位置にリダイレクトするために使います。

リダイレクトは完全に透過的に行われ、ゲームアプリケーションからは見えません。

ファイルアクセスを自動的にリダイレクトする目的は、
2人以上のユーザーのゲームインスタンスを1つのWindowsユーザーを使って動作させることと、
それぞれのユーザーが他のユーザーのデータにアクセスすることを防ぐためです。

ファイルフックによるリダイレクトは、シンラのユーザーIDとゲームIDを使って排他されます。

ファイルフックによるリダイレクトは2種類あります:

- Temp hooks : ゲームの実行が終わったら削除されます。複数のゲームインスタンスが同じデータにアクセスしないようにするだけのためのものです。
- Cloud save hooks: ゲームの実行が終わったら永続化され、次に起動したときにまた同じデータを読み込むことができます。[クラウドセーブファイル](#CloudSave) を参照してください。

TempとCloud save の両方にマッチした場合はTempが優先され、ファイルはセーブされません。


### ファイルフックの記述形式

ファイルフックは、アプリケーションがアクセスするファイルのパス名にマッチするパターンとして設定します。
パターンを設定するために、あらかじめ設定されている変数とワイルドカードを使えます。

変数は以下の通りです:


- `<USERID>` : ゲームを開始するときに使用される、シンラのUserID。 
- `<EXEPATH>` : ゲームの実行ファイルが存在するパス
- `<USERNAME>` : Windowsのユーザー名
- `<Profile>` : Windowsユーザーのプロファイルフォルダ
- `<Documents>` : WindowsユーザーのDocumentフォルダ
- `<LocalAppData>` : Windowsユーザーの `AppData\Local` フォルダ
- `<LocalAppDataLow>` : Windowsユーザーの `AppData\LocalLow` フォルダ
- `<RoamingAppData>` : Windowsユーザーの ̀`AppData\Roaming` フォルダ

ワイルドカードは以下の通りです:

- `some\path\*` : アスタリスク('*')は、ディレクトリ内のすべてのファイルにマッチします。ただし、サブディレクトリにはマッチしません。
- `some\path\**` : 2重のアスタリスク('**') は、ディレクトリ内のすべてのファイルとサブディレクトリにマッチします。

以下の例では、 `C:\Users\aeris\AppData\Roaming\Eidos Montreal\` 以下のすべてのファイルにマッチします

```
<RoamingAppData>\Eidos Montreal\**
```

### ファイルフック　リダイレクション

アプリケーションがファイルフックにマッチするファイルにアクセスしようとすると、
そのファイルアクセスは異なるパスにリダイレクトされます。

リダイレクト先のパスは、以下の情報によって決まります:


- MCSの設定に含まれる `StorePath` 変数。
- ゲームインスタンスを開始するときに指定された `UserID`
- ゲームインスタンスを開始するときに指定された `GameID`
- ファイルフックのタイプ (`Temp` または `Cloud`)

ファイルは以下のパスにリダイレクトされます:


```
<StorePath>\UserFiles\<UserID>\<GameID>\<Type>\
```


### <a name="CloudSave"></a> クラウドセーブファイル

ゲームを終了した後にまた起動されるまでのあいだ、個別のゲームのユーザーデータは、
tar.gz アーカイブに保存されています。

このアーカイブのパスは、以下の情報によって決まります:

- MCSの設定に含まれる `ArchivePath` 変数
- ゲームインスタンスを開始するときに指定された `UserID`
- ゲームインスタンスを開始するときに指定された `GameID`

個別のユーザー用のセーブデータは以下のパスに保存されます:

```
<ArchivePath>/<UserID>/<GameID>.tar.gz
```

## <a name="RegistryRedirection"></a> レジストリのリダイレクション

ファイルフックによってそれぞれのユーザーによるファイルアクセスが衝突しないようになっているのと同じように、レジストリキーへのアクセスも同様にリダイレクトされます。

`HKEY_CURRENT_USER`に対するすべてのアクセスは、ローカルマシンのレジストリ保存場所にリダイレクトされます。この保存場所は、クラウドセーブファイルと同じように保存したり読み出されます。　レジストリのリダイレクシションは、自動的に行われるので、特に設定をする必要はありません。


## <a name="CloudProperties"></a>CloudProperties設定ファイル

MCSを利用しているゲームは、 CloudProperties.json ファイルから設定を読みます。

shinra.pyからゲームを実行するときは、 CloudPropertiesファイルはインスタンスごとに自動的に生成され、実行が終わると消されます。

shinra.pyの setupコマンドを用いてゲームを実行する場合は、 CloudProperties.jsonファイルの位置を、ゲームを実行する前に指定することが可能です。


### <a name="MultipleGames"></a> 複数のゲームインスタンス

MCSは、デフォルトではゲームの実行ファイルと同じディレクトリにある CloudProperties.jsonファイルを読みます。
この結果として、ゲームインスタンスをひとつしか立ち上げられません。

複数のインスタンスを起動するには、複数のCloudPropertiesファイルを作り、
それぞれのファイルでゲームポート、ビデオポート、ユーザーIDを個別に指定する必要があります。
ゲームを実行する際には、 'FLARE_CONFIG'環境変数を経由して、
どのCloudPropertiesファイルを読むのかを設定します。

次の例は、DX3を MyCloudProperties.jsonファイルを読むように設定します:

```
set FLARE_CONFIG=MyCloudProperties.json
DX3.exe -archive
```

### <a name="Properties"></a> 設定オプション

shinra.pyが生成する CloudProperties.jsonファイルを編集して、
さまざまな設定を試して、チューニングをすることができます。

オプションを設定するときの書き方は、以下のようにします:

```
Cloud.Section.Name: default
```

上記のように書くと、 CloudProperties.jsonの[JSON](http://json.org)の内容が以下のように追加されます:

```
{ "Cloud": { "Section": { "Name": default } } }
```

設定可能な項目は以下の通りです:

- Cloud.Performance.MaxFPS: 30

  1秒あたりの最大フレーム数を設定します。これを超えるとゲームはレンダラスレッドの内部で一時停止します。

* Cloud.Session.TokenTimeoutSec: 10

  ゲームサーバーが起動した後、新規の接続を受け入れなくなるまでのタイムアウトを秒で設定します。0にするとタイムアウトしません。10にすると10秒後からは新規接続が不可能になります。

* Cloud.Session.StreamTimeoutSec: 10

  通信が止まってしまった後、プレイヤーを切断するまでの秒数を指定します。0にすると切断しません。

* Cloud.Local.EncoderThreads: 1

  ビデオエンコードのために割り当てるスレッド数を指定します。

* Cloud.Local.MaxRenderingFrames: 3

  レンダラがゲームをブロックするまでにキューにためられる最大フレーム数。この枚数のフレームがエンコーダへのキューにたまった時点で、ゲームは停止します。

* Cloud.Local.MaxEncodingFrames: 5

  エンコーダーがゲームをブロックするためのキューにためられる最大フレーム数。　この枚数のフレームがストリームへのキューにたまった時点で、ゲームは停止します。


## FAQ

1. <a name="FAQ1"></a>MCSは、シンラのリモートレンダリング技術と同じですか？

   MCSは、リモートレンダラに対する描画コマンドを抽象化する部分では、シンラ・システムと同じ方法を用いています。　しかし実装は独立しており、単純化されています。ハードウェア・アクセラレーションや、1つのサーバーで多くのクライアントに対応するための部分などを含みません。
   また、シンラの最適化されたエンコーダではなく、単純なJPEGエンコーダを用いています。

2. <a name="FAQ2"></a>MCSを使うとゲームが遅くなります。　シンラのリモートレンダラでも同じなのでしょうか？

   [質問1](#FAQ1)でも述べたとおり、MCSはシンラのリモートレンダラの実装とは異なるものです。ハードウェアアクセラレーションとリソースの共有が実装されていないため、エンコーダはソフトウェアのみで実装され、また出力がJPEGであるためストリームの帯域幅も大きくなります。
   さらに、クライアントと同じ計算機で実行している場合は、ゲームサーバーとCPUやGPUを共有するため、さらに遅くなります。
   短く言うと、MCSはリモートレンダリングを模倣しているだけで、全くことなるものなので、ベンチマークとして使うことはできません。性能に関する参考にすることは避けてください。

3. <a name="FAQ3"></a>MCSで使われるポート番号は？

   MCSはデフォルトでは以下のポート番号を使います([CloudProperties](#CloudProperties)を参照)
   * ビデオポート (`Cloud.Network.VideoPort`): 60000
   * 音声、ゲーム操作ポート (`Cloud.Network.GamePort`): 55000

   これらのポート番号は、シンラクライアントでは、URLの形式で指定します:

   ```
   shinra://localhost:55000/runMCS/mygame?VideoPort=60000
   ```
