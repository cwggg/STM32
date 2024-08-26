#!/bin/bash

from tkinter import filedialog
import serial
import time
import os
import binascii

#byte0 byte1: identifier->0x55AA
#byte2-5: dataLen
#byte6-9: crc32
header_data = [0xAA,0x55,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
             0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
             0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
             0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00]

def print_progress_bar(percent):
    bar_length = 20
    filled_length = int(bar_length * percent / 100)
    bar = '#' * filled_length + '-' * (bar_length - filled_length)
    print(f'Progress: [{bar}] {percent}%')

def crc32(data_list):
    data_bytes = b''.join([bytes([x]) for x in data_list])
    return binascii.crc32(data_bytes) & 0xffffffff
    
ser = serial.Serial(port= 'COM36', baudrate=115200)
# file_name = "ff.bin"
file_name = filedialog.askopenfilename(filetypes = [('binary','.bin')],initialdir = r"C:\\",multiple = False)
try:
    with open(file_name,"rb") as file:
        dataPackSize = 64
        file_data = file.read()
        file_data_trans = list(file_data)
        file_size = os.path.getsize(file_name)
        # print(file_size)
        if file_size%dataPackSize == 0:
            transmit_package = file_size//dataPackSize
        else:
            transmit_package = file_size//dataPackSize + 1
            zero_add_cnt = dataPackSize-file_size%dataPackSize
            file_data_trans.extend([0]*zero_add_cnt)
            print(len(file_data_trans))
        #package len
        header_data[2] = len(file_data_trans)&0xff
        header_data[3] = (len(file_data_trans)>>8)&0xff
        header_data[4] = (len(file_data_trans)>>16)&0xff
        header_data[5] = (len(file_data_trans)>>24)&0xff
        #package crc32
        crc32data = crc32(file_data_trans)
        header_data[6] = crc32data&0xff
        header_data[7] = (crc32data>>8)&0xff
        header_data[8] = (crc32data>>16)&0xff
        header_data[9] = (crc32data>>24)&0xff
        print("CRC32 value is %X \r\n",hex(crc32data))
        ser.write(header_data)
        time.sleep(0.1)
        for i in range(transmit_package):
           ser.write(file_data_trans[(i*dataPackSize):(i*dataPackSize+dataPackSize)]) 
           percent = round(i*100/transmit_package,1)
           print_progress_bar(percent)
           time.sleep(0.1)
           if i == (transmit_package-1):
                print_progress_bar(100)
except:
    print("File open error!")

print(ser.readline().decode('utf-8'))
ser.close()