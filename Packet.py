#!/usr/bin/python

import time
import serial
import sys
import json
import argparse
from pprint import pprint
from colorama import Fore, Back, Style, init
from datetime import datetime

class Packet:
    def __init__(self):
        self.lenght = 0
        self.ptype = ''
        self.nid = ''
        self.src_type = ''
        self.src_address = ''
        self.dst_type = ''
        self.dst_address = ''
        self.join_device_type = ''
        self.link_payload = []
        self.xraw = []
        self.draw = []
        
		# prevede hexacislo na string 
    def to_hexstr(self, l):
        #print("To_hexstr:")
        #pom2 = ' '.join(map(lambda x : "%02X" % x , l))
        #print("%s" % pom2)
        return ' '.join(map(lambda x : "%02X" % x , l))

    def decode(self, data):
        # clear data
        self.__init__()  
				  
        self.xraw = map(lambda x : x[2:], data)
            
        # int() = vytiskne (vraci) integer
        # dva parametry = string a zaklad (prijimame hexa cisla)
        self.draw = map(lambda x : int(x, 16), data)
        data_int = self.draw
        #print("Po funkci int: %s" % data_int)
        # len() = vraci delku retezce
        self.lenght = len(data_int)
        #print("Po funkci len: %d" % self.lenght)
        # Vypise data na indexech: 1, 2, 3, 4
        self.nid = self.to_hexstr(data_int[1:5]) 
		
				# Horni dva bity adresy site (kvuli funkci gen_header())
        code = data_int[0] >> 6 
        # Ziskani hlavicky kontrolniho bytu
        header_payload = data_int[0] & 0x0F
        print("Code: %d") % code
        data_int
        print("Data_int: %d") % len(data_int)
        
        if header_payload == 0x03 or header_payload == 0x04 or header_payload == 0x05:
            #print("Join")
            self.ptype = 'JOIN'
        elif code == 0:
            self.ptype = 'DATA'
        elif code == 1:
            self.ptype = 'COMMIT'
        elif code == 2:
            self.ptype = 'ACK'
        elif code == 3:
            self.ptype = 'COMMIT_ACK'
            
        if self.ptype == 'JOIN':
            if header_payload == 0x03:
                self.ptype = 'JOIN REQUEST'
                #print("Join request")
                self.src_address = self.to_hexstr(data_int[6:10])
                #DEST ADDR???
                #self.dst_address = self.to_hexstr(data_int[4:5])
                #if data_int[9] == 0x00:
                self.join_device_type = 'END DEVICE'
                    #print("End device")
                #elif data_int[9] == 0x80:
                    #self.join_device_type = 'COORDINATOR'
                    #print("Coordinator")
                #else:
                    #self.join_device_type = 'UNKNOWN'
                    #print("Unknown")
                #self.link_payload = self.to_hexstr(data_int[10:])
                
            elif header_payload == 0x05:
                self.ptype = 'ACK JOIN REQUEST'
                self.join_device_type = 'END DEVICE'
                self.dst_address = self.to_hexstr(data_int[5:9])

            elif header_payload == 0x04:
                self.ptype = 'JOIN RESPONSE'
                #print("Join response")
                self.join_device_type = 'END DEVICE'
                self.dst_address = self.to_hexstr(data_int[5:9])
                #self.src_address = self.to_hexstr(data_int[9:10])
                self.nid = self.to_hexstr(data_int[20:24])
                #self.nid = self.to_hexstr(data_int[10:14])
                
			  # CIL
				# pokud je na 6. bitu jednicka, tak se jedna o ED, jinak COORD
        else:        
            if data_int[0] & 0x20: 
                self.dst_type = 'EDID'
                self.dst_address = self.to_hexstr(data_int[5:9])
            else:
                self.dst_type = 'CID'
                self.dst_address = self.to_hexstr(data_int[5:6])
        # ZDROJ   
				# pokud je na 5. bitu jednicka, tak se jedna o ED, jinak COORD
            if data_int[0] & 0x10:
                self.src_type = 'EDID'
                self.src_address = self.to_hexstr(data_int[6:10]) 
            else:
                self.src_type = 'CID'
                self.src_address = self.to_hexstr(data_int[9:10])
            self.link_payload = self.to_hexstr(data_int[10:])
        return

    def print_link_transfer(self):
        # return a string representing the date, controlled by an explicit  
				# format string
				# === VYPIS POD SEBOU ===
        if True:
				    # JOIN REQUEST 
            if self.ptype == 'JOIN REQUEST':
                print("Datum a cas: %s%s" % (Fore.YELLOW, datetime.now().strftime('%H:%M:%S.%f')))
                print("%sZDROJ%s: Typ a adresa zarizeni: %s%s: %s" % (Fore.CYAN, Fore.WHITE, Fore.CYAN, self.join_device_type, self.src_address))
                print("Typ paketu: %s%s" % (Fore.GREEN, self.ptype))
                print("Network ID: %s%s" % (Fore.GREEN, self.nid))
                #print("Payload: %s%s" % (Fore.MAGENTA, self.link_payload))
                # ACK JOIN REQUEST
            elif self.ptype == 'ACK JOIN REQUEST':
                print("Datum a cas: %s%s" % (Fore.YELLOW, datetime.now().strftime('%H:%M:%S.%f')))
                #print("%sZDROJ%s: Adresa zarizeni: %s%s" % (Fore.CYAN, Fore.WHITE, Fore.CYAN, self.src_address))
                print("%sCIL%s: Typ a adresa zarizeni: %s%s: %s" % (Fore.RED, Fore.WHITE, Fore.RED, self.join_device_type, self.dst_address))
                print("Typ paketu: %s%s" % (Fore.GREEN, self.ptype))
                print("Network ID: %s%s" % (Fore.GREEN, self.nid))
                # JOIN RESPONSE
            elif self.ptype == 'JOIN RESPONSE':
                print("Datum a cas: %s%s" % (Fore.YELLOW, datetime.now().strftime('%H:%M:%S.%f')))
                #print("%sZDROJ%s: Adresa zarizeni: %s%s" % (Fore.CYAN, Fore.WHITE, Fore.CYAN, self.src_address))
                print("%sCIL%s: Typ a adresa zarizeni: %s%s: %s" % (Fore.RED, Fore.WHITE, Fore.RED, self.join_device_type, self.dst_address))
                print("Typ paketu: %s%s" % (Fore.GREEN, self.ptype))
                print("Network ID: %s%s" % (Fore.GREEN, self.nid))
                # print("Payload: %s%s" % (Fore.MAGENTA, self.link_payload))
                # DATA, ACK, COMMIT, COMMIT_ACK
            else:
                print("Datum a cas: %s%s" % (Fore.YELLOW, datetime.now().strftime('%H:%M:%S.%f')))
                print("%sZDROJ%s: Typ a adresa zarizeni: %s%s: %s" % (Fore.CYAN, Fore.WHITE, Fore.CYAN, self.src_type, self.src_address))
                print("%sCIL%s: Typ a adresa zarizeni: %s%s: %s" % (Fore.RED, Fore.WHITE, Fore.RED, self.dst_type, self.dst_address))
                print("Typ paketu: %s%s" % (Fore.GREEN, self.ptype))
                print("Network ID: %s%s" % (Fore.GREEN, self.nid))
                if self.ptype == 'DATA':
                    print("Payload: %s%s" % (Fore.MAGENTA, self.link_payload))
        # === VYPIS VEDLE SEBE ===
        else:
            # JOIN REQUEST 
            if self.ptype == 'JOIN REQUEST':
                print ("%s%s %s%s: %s %s(%s) NID: %s" % (
                Fore.YELLOW,
								datetime.now().strftime('%H:%M:%S.%f'),
                Fore.CYAN,
                self.join_device_type,
                self.src_address,
                Fore.GREEN,
                self.ptype,
                self.nid
                ))
                # JOIN RESPONSE
            elif self.ptype == 'JOIN RESPONSE':
                print ("%s%s %s%s: %s %s(%s) NID: %s" % (
                Fore.YELLOW,
								datetime.now().strftime('%H:%M:%S.%f'),
                Fore.RED,
                self.join_device_type,                
                self.dst_address,
                Fore.GREEN,
                self.ptype,
                self.nid
                ))
                # DATA, ACK, COMMIT, COMMIT_ACK
            else:
                print ("%s%s %s(%s) %s %s=> %s(%s) %s %s(%s) NID: %s" % (
                Fore.YELLOW,
								datetime.now().strftime('%H:%M:%S.%f'),
                Fore.CYAN,
                self.src_type,
                self.src_address,
                Fore.WHITE,
                Fore.RED,
                self.dst_type,
                self.dst_address,
                Fore.GREEN,
                self.ptype,
                self.nid
                ))
                if self.ptype == 'DATA':
                    print("%sDATA: %s" % (Fore.MAGENTA, self.link_payload))
