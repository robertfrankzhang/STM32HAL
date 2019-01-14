#!/usr/bin/python3
import serial
import time
import sys

class PillBottleDock:
    def __init__(self):
        self.Timeout = 0
        self.Hello = 1
        self.Ready = 2
        self.Done = 3
        self.AllReceived = 4
        self.Full = 5
        self.Timestamp = 6
        self.RealTimeDate = 7
        self.PillCount = 8
        self.LockoutPeriod = 9
        self.TypeStr = ['Timeout','Hello','Ready','Done','AllReceived','Full','Timestamp','RealTimeDate','PillCount','LockPeriod']

    def connect(self,devId):
        dev = '/dev/ttyACM{}'.format(devId)
        return serial.Serial(dev, baudrate=115200, timeout=1.0)

    def rxSession(self,port):
        '''receve timestamps'''
        print('session RX')
        while True:
            rxbuf = self.getPacket(port)
            if rxbuf[0] == self.Hello:
                break
        self.sndPacket(port,[self.Ready])
        while True:
            packet = self.getPacket(port)
            if packet[0] == self.Done:
                break
            # process records
        self.sndPacket(port,[self.AllReceived])
        return True

    def txSession(self,port):
        '''send proscription data'''
        print('session TX')
        while True:
            self.sndPacket(port,[self.Hello])
            packet = self.getPacket(port)
            if packet[0] == self.Ready:
                break

        while True:
            for i in range(6):
                self.sndPacket(port,[self.PillCount, 10])
                self.sndPacket(port,[self.LockoutPeriod, 20])
                self.sndPacket(port,[self.RealTimeDate,19,1,15,8,0,0])
                self.sndPacket(port,[self.Done])
                packet = self.getPacket(port)
                if packet[0] == self.AllReceived:
                    return True
        return True
        
            
    def getPacket(self,port):
        while True:
            try:
                sync = port.read(1)
                if sync[0] != 0xaa:
                    print('out sync0')
                    continue
                sync = port.read(1)
                if sync[0] != 0xbb:
                    print('out sync0')
                    continue
            except:
                return [0] # timeout
            length = port.read(1)
            packet = list(port.read(length[0]))
            print('rcvd {}'.format(self.TypeStr[packet[0]]),packet)
            return packet

    def sndPacket(self,port,data):
        #time.sleep(0.05)
        print('send {}'.format(self.TypeStr[data[0]]),data)
        header = [0xaa,0xbb,len(data)]
        port.write(bytearray(header+data))

if __name__ == '__main__':
    dock = PillBottleDock()
    port = dock.connect(0)
    if port is not None:
        print('port opened')

    if len(sys.argv) > 1:
        if sys.argv[1] == 'tx':
            dock.txSession(port)
        else:
            dock.rxSession(port)
    else:
        dock.rxSession(port)
                
    #while True:
    #    line = port.readline().decode("utf-8") 
    #    print(line)
    #    port.write('Say something1'.encode())
    #    port.write('Say something2'.encode())
