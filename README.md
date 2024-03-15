# unimrcp_with_huaweicloud_asr

#### 介绍
本仓库是基于unimrcp集成华为云实时语音识别开源代码，旨在帮助社区用户快速搭建外呼系统。本仓库提供了两种安装方法，一是源码安装，二是直接采用release出来的二进制库。


#### 安装教程

本代码仓提供两种安装方式供用户选择。

##### 方法一： 源码安装

1. 先下载安装华为云实时语音识别SDK，安装指南参考https://bbs.huaweicloud.com/blogs/392949

2. 将安装的华为云ASR SDK放入**plugins/huawei_recog/huaweicloud_asr**

3. 进入根目录/unimcrp-deps-1.6.0，安装安装unimrcp的依赖库

```
./build-dep-libs.sh
```

4. 进入根目录，安装unimrcp

```
./bootstrap
./configure
make
make install

```

5. unimrcp默认安装在/usr/local/unimrcp中

##### 方法二：直接使用release库

```
wget https://gitee.com/computervisionlearner/unimrcp_with_huaweicloud_asr/releases/download/v0.0.1/unimrcp-1.8.0_unimrcp-deps-1.6.0_binary.tar.gz
tar -xzvf unimrcp-1.8.0_unimrcp-deps-1.6.0_binary.tar.gz
```

#### 使用说明

当前本仓库仅集成了实时语音识别功能，语音合成暂未实现，请读者知悉。更详细的说明请移步博客：https://bbs.huaweicloud.com/blogs/423604

**欢迎社区各位C++、语音专家多多指点，丰富和完善本代码仓功能**


