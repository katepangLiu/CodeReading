# pion-webrtc

[pion-webrtc](https://github.com/pion/webrtc) 是一个纯golang的webrtc实现，
最近正在读 [webrtc for curious](https://github.com/webrtc-for-the-curious/webrtc-for-the-curious)
二者的主要作者都是[Sean-Der](https://github.com/Sean-Der)
近期打算结合书和源码实现，更好地理解webrtc.

## Overview

pion-webrtc 提供了2套接口，一套用于原生客户端开发，一套用于js开发(也支持wasm)

需要区分编译的源文件，通过go编译选项进行管理:

- +build !js
- +build js,wasm
- +build js,!go1.14 
- +build js,go1.14  

## WebRTC Agent

WebRTC Agent 整体上看就是各种协议Agent的集合体，API上 由 PeerConnection 来表示

![webrtc-agent](https://webrtcforthecurious.com/docs/images/01-webrtc-agent.png)

