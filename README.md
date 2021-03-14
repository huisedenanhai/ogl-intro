# OpenGL简介

2021年春季学期北京大学本科生图形学课程的样例代码。

![screenshot](data/screenshot.png)

## Download

```
git clone https://github.com/huisedenanhai/ogl-intro.git
```

## Build

本项目利用[CMake](https://cmake.org/)配置。

### CLion

直接打开文件夹即可。

### VS Code

安装插件

+ [CMake Tools](https://vector-of-bool.github.io/docs/vscode-cmake-tools/)
+ [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)

然后打开文件夹即可。

### Visual Studio

1. 安装[CMake](https://cmake.org/)
2. 打开CMake
3. 选择项目文件夹和输出文件夹
4. 点击Configure
5. Configure结束后点击Generate
6. 打开输出文件夹中的ogl-intro.sln

### Xcode

1. 安装[CMake](https://cmake.org/)
2. 运行如下指令
```bash
cd ogl-intro
mkdir build
cd build
cmake .. -G Xcode
open ogl-intro.xcodeproj
```


