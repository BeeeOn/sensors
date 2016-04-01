#!/usr/bin/python

##
# @file simulator.py
# @author BeeeOn team - Peter Tisovcik (xtisov00@fit.vutbr.cz)
# @data 2016
# @brief

import paho.mqtt.client as mqtt
import subprocess
import argparse
import sys, os, signal
from threading import Thread
import time
import lxml.etree as ET
from Devices import *
from Packet import *
import threading

debugFile="/dev/null"
configFile = "xml.xml"

#handler for CTRL+C
def sigint_handler(signum, frame):
	print 'Close CTRL+C!'
	for i in devices:
		i.stop()
		os.kill(i.pid, signal.SIGKILL)
	sys.exit(0)

#table of message type
def cmdTable():
	table = {
		'group':('group',100,1),
		'cmd':('cmd',0,1),
		'edid':('edid',1,4),
		'cid':('cid',2,1),
		'nid':('nid',3,4),
		'channel':('channel',4,1),
		'bitrate':('bitrate',5,1),
		'band':('band',6,1),
		'rssi':('rssi',7,1),
		'tocoord':('tocoord',8,1),
		'toed':('toed',9,4),
		'data_len':('data_len',10,1),
		'data':('data',11,255),
		'refresh_time':('refresh_time',12,1),
		'pcid':('pcid',13,1)
	}
	return table

def on_connect(client, userdata, flags, rc):
	client.subscribe("BeeeOn/#")

def on_message(client, userdata, msg):
	#data fromm device
	if (str(msg.topic) == "BeeeOn/data_from"):
		print(">>>>>>"+msg.topic+" "+str(msg.payload))

		split_msg = msg.payload.split(',')
		device_channel = 0

		pid_from_device = 0
		for i in range(0,4):
			pid_from_device = pid_from_device << 8 | int(split_msg[i])

		send_list = []
		groups = []
		for dev in devices:
			#finding from whom came the message
			if dev.pid == pid_from_device:
				groups = dev.group
				device_channel = dev.channel

		for dev in devices:
			if dev.pid == pid_from_device:
				continue

			#select of equipment in the same channel
			for g in groups:
				if g in dev.group:
					if (int(dev.channel) == int(device_channel)):
						send_list.append(dev.id)

		#avoiding the duplication
		send_list = list(set(send_list))

		#send message
		for device in devices:
			if device.id in send_list:
				device.reSendMsg(split_msg)

		print "-----------------------------------"
		init(autoreset=True)
		print "SNIFFER"
		print msg.payload
		split_msg = msg.payload.split(',')
		temp = split_msg[9:]
		temp.pop()
		packet = "{\"raw\": ["

		#first value
		packet += "\"0x%02X\"" % int(split_msg[8])

		for item in temp:
			packet += ",\"0x%02X\"" % int(item)
		packet += "]}\n"

		p = Packet()

		try:
			# Convert JSON string into Python nested dictionary/list.
			data = json.loads(packet)
		except Exception, e:
			print("Error")
		p.decode(data['raw'])
		p.print_link_transfer()
		print "-----------------------------------"

	#config data from device
	elif(str(msg.topic) == "BeeeOn/config_from"):
		split_msg = msg.payload.split(',')

		pid_from_device = 0
		for i in range(0,4):
			pid_from_device = pid_from_device << 8 | int(split_msg[i])

		exists = False
		for x in devices:
			if x.pid == pid_from_device:
				exists = True
				print "ID device: " + str(x.id)
				break

		if exists:
			msg2 = split_msg[8:]
			msg2.pop() #remove last value
			table = cmdTable()

			print "---------------"
			actual = 0;
			table = cmdTable()
			i =0
			while i <len(msg2):
				for item in table.keys():
					if str(table[item][1]) == str(msg2[i]):
						i = i + 1

						limit = msg2[i]
						i = i + 1
						for x in range(0, int(limit)):
							print item, msg2[i+x]
						i = i + int(limit)
						break

			print "---------------"

	#config messages
	elif(str(msg.topic) == "BeeeOn/set_command"):
		sprava = msg.payload.split(',')
		sprava.pop() #remove last empty value

		new_msg = ""
		for i in sprava:
			 new_msg += str(chr(int(i)))

		try:
			new_msg = new_msg.split('xx')
		except:
			new_msg = new_msg

		for item in new_msg:
			if item == ',' or item == '':
				continue;
			item_split = item.split(',')
			item_split = [i for i in item_split if i]
			commands = []

			for device in devices:
				if str(device.id) == str(item_split[0]):
					for i in item_split[1:]:
						commands.append(i)
					device.command = commands
					break;

			commands = []

	#save debug data and data from devices to file
	if(str(msg.topic) == "BeeeOn/debug" or str(msg.topic) == "BeeeOn/data_from"):
		split_msg = msg.payload.split(',')

		new_line = ""
		if (str(msg.topic) == "BeeeOn/data_from"):
			new_line = "\n"

		msg2 = split_msg[4:]
		with open(debugFile, 'a') as f:
			f.write(new_line + str(msg.topic).ljust(17, ' ') + ','.join(msg2)+ "\n")

#load config data for devices from file
def load_config(device_type):
	config = [] #list of devices

	tree = ET.parse(configFile)
	root = tree.getroot()
	root = root.find(device_type)

	table = cmdTable()

	if root is None:
		return config

	for child in root:
		#device type
		if (device_type == 'ed'):
			configData = HexaSen(client)
		elif (device_type == 'pan'):
			configData = Pan(client)
		elif (device_type == 'coord'):
			configData = Coord(client)
		else:
			return config

		configData.id = child.attrib['id']

		for data in child:
			if data.tag == table['group'][0]:
				configData.group = data.text.split(',')
			elif data.tag == table['edid'][0]:
				configData.edid = data.text.split(',')
			elif data.tag == table['cid'][0]:
				configData.cid = data.text
			elif data.tag == table['nid'][0]:
				configData.nid = data.text.split(',')
			elif data.tag == table['channel'][0]:
				configData.channel = data.text
			elif data.tag == table['bitrate'][0]:
				configData.bitrate = data.text
			elif data.tag == table['band'][0]:
				configData.band = data.text
			elif data.tag == table['rssi'][0]:
				configData.rssi = data.text
			elif data.tag == 'members':
				configData.add_members(data.text.split(','), child.attrib['id'] )

		#configData.print_properties()
		configData.start()
		time.sleep(0.1)
		config.append(configData)
	return config

###########################
if __name__ == "__main__":
	signal.signal(signal.SIGINT, sigint_handler)

	client = mqtt.Client()
	client.on_connect = on_connect
	client.on_message = on_message

	try:
		client.connect("localhost", 1883, 60)
	except:
		print "Mosquitto service isn't started - mosquitto -d."
		sys.exit(0)

	#process for mosquitto
	client.loop_start()

	devices = []

	with open(debugFile, 'w') as f:
		f.write('')

	#load ED devices
	ed = load_config('ed')
	devices.extend(ed)
	time.sleep(0.1)

	#load COORD devices
	coord = load_config('coord')
	devices.extend(coord)
	time.sleep(0.1)

	#load PAN device
	pan = load_config('pan')
	devices.extend(pan)

	while True:
		time.sleep(5)
