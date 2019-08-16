# codec_single_thread

## 简介

阻塞式接收UDP包并对其进行编码转换。可以解析并发送RTP格式的数据包与SC2格式的数据包。

* 依赖
  * c++11
  * jsoncpp （代码内含）
  * pjsip

## 文件说明

* codec.h/ codec.cpp 编解码转换的模块，编码的输入和解码输出为8KHz，16bit采样，线性量化的PCM。
* jsoncpp jsoncpp库的代码， 需要C++11，如果无法使用C++11，请使用低版本的jsoncpp库
* rtp： rtp和rtcp发送与解析，注意，当前没有SSRC冲突检测与处理。所以需要保证每路对话使用不同的端口
* session 对话处理，处理RTP，SC2并调用编码器进行编解码转换
* udp： 对socket的封装，仅适用于Linux
* logger：出错时的打印信息，默认打印到标准错误输出
* utils： 实体。包括json文件安全读取等函数。