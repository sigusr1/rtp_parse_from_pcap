#!/usr/bin/python
# -*- coding: UTF-8 -*-
import sys
import matplotlib.pyplot as plt

def get_timecost_list(filename):
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
                if seq != ((last_seq + 1) % 65535):
                    print("lost rtp:%s"  %(line))
                last_seq = seq
                
        if line.find("Frm_Interval") < 0:
            continue;
        
        timecost = int(line[-12:-4])
        if timecost >= 100000:     #cost too many time, more than 100ms
            print("Cost too many time:%s"  %(line))
        timecost_list.append(timecost)

    fo.close()

    return timecost_list


timecost_list = get_timecost_list(str(sys.argv[1]))


x = range(len(timecost_list))


plt.plot(x, timecost_list, c='b')


plt.xlabel("Frames") #X轴标签  
plt.ylabel("Frame interval(us)")  #Y轴标签  
plt.title("Network jitter") #图标题  
plt.grid(True)
plt.show()  #显示图
