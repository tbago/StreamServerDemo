## 基于阻塞模式的HLS Server demo

1、首先需要调用ffmpeg命令行生成m3u8，和分片的ts文件，具体命令参考如下：

````shell
./ffmpeg -i ./media_resource/00000.m2ts -vf scale=640:480 -c:v libx264 -c:a copy -f hls  -hls_list_size 0  ./test/index.m3u8
````

2、播放需要调用ffplay，具体命令如下：

```shell
fplay -i http://127.0.0.1:8088/index.m3u8
```



