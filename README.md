# BuildPushKill

## 简介
BuildPushKill由三个部分组成：会自动扫描已修改文件，编译受影响的模块（build）；push编译的模块到手机中（push）；杀死受影响的进程（kill.如果在配置文件中有设置需要重启的apk，也会自动重启apk）  

## 使用说明
1.下载安装包 [release](https://github.com/Onelzx/bpk/releases)，点击安装（或使用dpkg安装：`sudo dpkg -i BuildPushKill.deb`）  
也可以下载源码自己编译安装：在项目根目录运行`./build.sh -b`，就会在`out`目录下生成可执行文件`bpk`和安装包`BuildPushKill.deb`   
  
2.在**安卓项目根目录及其子目录**下运行`bpk`，BPK接受4种参数：   
    `无参数`：`bpk`，自动扫描**当前运行目录**下被修改的文件（从上一次在当前项目运行BPK之后修改的文件，如果在当前项目是第一次运行，则从当前时间减30分钟算起），并编译、push被影响的模块，然后重启系统服务   
    `模块名`：`bpk ModuleName`，编译、push指定的模块名（模块名无需带上后缀名），然后重启系统服务，例：`bpk libcom.mytest`  
    `-r`：`bpk -r`，重置用于统计哪些文件被修改的起始时间，将其修改为当前时间，建议在第一次运行BPK的安卓项目下运行一次`bpk -r`  
    `-c`：`bpk -c`，清空BPK缓存文件  

## Tip & Bug
1.添加新模块后BPK可能会检测不到，需要指定新加的模块名来编译`bpk ModuleName`（模块名无需带上后缀名）  
2.在机械硬盘上，尽量在较小的目录下运行BPK，以节省扫描文件的时间。以chi-cdk目录为例，机械硬盘扫描需要约700ms（第一次可能时间更长），固态硬盘只需要300ms左右  
3.在某个安卓项目下第一次使用BPK时或者刚更新完代码后，建议都重置一下BPK时间戳`bpk -r`，以防止没用的文件修改也会被扫描进来  

## BPK配置文件说明
BPK提供了一些配置项，配置文件路径是`~/.BPKTemp/config.json`：  
"TargetsOutDirScan"：扫描`out`目录下的哪些子目录。填写模块在哪些目录下生成，如`vendor`、`system`  
"KillProcess"：push完需要关闭进程的进程名  
    "AccurateSearch"：是否是精确搜索，填写`true`或`false`。如果是需要完全匹配进程名则填`true`；如果是进程名带有某个字符串则填`false`  
    "ProcessName"：进程名，如`cameraserver`  
"RestartAPK"：需要重启的apk  
    "PackageName"：apk包名，如`com.android.camera`  
    "ActivityName"：需要启动的activity名，如`Camera`  
    "RestartAPKInterval"：重启apk的等待时间，单位毫秒  
"IgnoreModifiedFileDir"：扫描被修改的文件时需要被忽略的目录。该目录是从安卓项目根目录起的相对路径  
"ExtAction"：额外的扩展行为。当扫描到某个目录下的文件被修改时会触发一些额外的行为  
    "Dir"：触发额外行为的目录。该目录是从安卓项目根目录起的相对路径  
    "RelatedModule"：如果该目录下的文件被修改了则编译某个模块（只写模块名，不需要带有后缀名），如`libcom.mytest`  
    "ShellBeforeBuild"：在编译前调用的shell脚本文件名。脚本需要放在`~/.BPKTemp/script`下  
    "ShellAfterBuild"：在编译后调用的shell脚本文件名。脚本需要放在`~/.BPKTemp/script`下  

