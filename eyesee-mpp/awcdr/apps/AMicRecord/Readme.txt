AMicRecord用途：
    保存采集的PCM音频数据。
    可以封装保存为wav文件，也可以直接保存为pcm数据。
    手动停止运行：ctrl+c

    提供配置文件AMicRecord.conf，用于配置运行参数。
    启动时，在命令行参数中给出配置文件的具体路径，AMicRecord会读取AMicRecord.conf，完成参数解析，
    然后按照参数运行。
    例如从命令行启动指令：
    ./AMicRecord -path ./AMicRecord.conf
    "-path ./AMicRecord.conf"指定了参数配置文件的路径。
    如果不给配置文件，则app按默认参数运行。

测试参数的说明：
(1)pcm_dir_path: 指定保存pcm数据的目录，例如/mnt/extsd/AMicRecord_Files。注意不要在最后加/。
(2)pcm_file_name: 指定保存的文件名，例如ai.cpm。
(3)pcm_sample_rate：指定采样率，例如16000
(4)pcm_channel_cnt：指定声道数，例如2
(5)pcm_bit_width：指定一个声道的sample的bit数，例如16
(6)pcm_cap_duration：指定采集时间，单位为秒，0表示无限。
(7)save_wav：指定是否保存为wav文件。1表示加wav头信息，保存为wav文件，0表示直接保存pcm数据。

