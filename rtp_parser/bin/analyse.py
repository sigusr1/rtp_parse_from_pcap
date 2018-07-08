#!/usr/bin/python
# -*- coding: UTF-8 -*-
import numpy as np
import matplotlib.pyplot as plt

def get_timecost_array(filename):
    timecost_list = []
    last_seq = -1;
    
    fo = open(filename, "r")

    for line in fo.readlines():
        if line.find("rtp") >= 0:
            seq_str = line[8:16]
            #print(seq_str)
            if (last_seq == -1):
                last_seq = int(seq_str)
            else:
                seq = int(seq_str)
                if (seq != last_seq + 1) and (last_seq != 65535):
                    print("lost rtp:%s"  %(line))
                last_seq = seq
                
        if line.find("Frm_Interval") < 0:
            continue;
        
        timecost_str = line[-12:-4]
        if int(timecost_str) >= 200000:
            print(line)
        timecost_list.append(timecost_str)

    fo.close()

    tmp = np.array(timecost_list)
    timecost_array = tmp.astype(np.float)
    return timecost_array


speed_array1 = get_timecost_array("src[192.168.42.1[554]]--dst[192.168.42.102[49108]].txt")
#speed_array2 = get_speed_array("iperf2.txt")
#speed_array3 = get_speed_array("iperf3.txt")

#x = range(max(len(speed_array1), len(speed_array2), len(speed_array3)))
x = range(len(speed_array1))

#print(x)

plt.plot(x, speed_array1, c='b')
#plt.plot(x, speed_array2, c='g')
#plt.plot(x, speed_array3, c='r')

plt.xlabel("Frames") #X轴标签  
plt.ylabel("Frame interval(us)")  #Y轴标签  
plt.title("Network jitter") #图标题  
plt.grid(True)
plt.show()  #显示图
