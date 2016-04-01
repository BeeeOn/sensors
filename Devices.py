#!/usr/bin/python

##
# @file Devices.py
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
import re
from copy import deepcopy

class Device(Thread):
	_id = 0
	_pid = 0
	_group = []
	_rssi = 0
	_cmd = 0
	_edid = [0,0,0,0]
	_cid = 0
	_nid = [0,0,0,0]
	_channel = 0
	_bitrate = 0
	_band = 0
	_refresh_time = 15
	_pcid = 0 #parent cid

	signal_stop = True
	configTopic = "BeeeOn/config"
	dataToTopic = "BeeeOn/data_to"
	dataFromTopic = "BeeeOn/data_from"
	log_directory = "/tmp/"

	_command = []

	#name/id_value/length_value
	table = {
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

	def __init__(self, mqtt):
		Thread.__init__(self)
		self.mqtt = mqtt #mqtt object

	@property
	def id(self):
		return self._id
	@id.setter
	def id(self, id):
		self._id = id

	@property
	def pid(self):
		return self._pid
	@pid.setter
	def pid(self, pid):
		self._pid = pid

	@property
	def group(self):
		return self._group
	@group.setter
	def group(self, group):
		self._group = group

	@property
	def rssi(self):
		return self._rssi
	@rssi.setter
	def rssi(self, rssi):
		self._rssi = rssi

	@property
	def cmd(self):
		return self._cmd
	@cmd.setter
	def cmd(self,cmd):
		self._cmd = cmd

	@property
	def nid(self):
		return self._nid
	@nid.setter
	def nid(self, nid):
		self._nid = nid

	@property
	def edid(self):
		return self._edid
	@edid.setter
	def edid(self, edid):
		self._edid = edid

	@property
	def cid(self):
		return self._cid
	@cid.setter
	def cid(self, cid):
		self._cid = cid

	@property
	def channel(self):
		return self._channel
	@channel.setter
	def channel(self, channel):
		self._channel = channel

	@property
	def command(self):
		return self._command
	@command.setter
	def command(self, command):
		self._command = command

	@property
	def bitrate(self):
		return self._bitrate
	@bitrate.setter
	def bitrate(self, bitrate):
		self._bitrate = bitrate

	@property
	def band(self):
		return self._band
	@band.setter
	def band(self, band):
		self._band = band

	@property
	def refresh_time(self):
		return self._refresh_time
	@refresh_time.setter
	def refresh_time(self, refresh_time):
		self._refresh_time = refresh_time

	@property
	def pcid(self):
		return self._pcid
	@pcid.setter
	def pcid(self, pcid):
		self._pcid = pcid

	def print_properties(self):
		print self.id
		print self.pid
		print self.group
		print self.rssi
		print self.cmd
		print self.edid
		print self.cid
		print self.nid
		print self.channel
		print self.bitrate
		print self.band
		print self.refresh_time
		print self.pcid

	#stopping loop
	def stop(self):
		self.signal_stop = False

	def command_value(self, item):
		match = re.search(r'=(.*?)[;\)]',item)
		return float(match.group(1))

	#string to list
	def stringTolist(self, value):
		return value.split('-')

	#list of integer to string with comma
	#[0,1,2] >> "0,1,2,"
	def listToMqttStr(self, listValue):
		return ','.join(str(i) for i in listValue)

	#convert PID to 4B string with comma
	def pidToStr(self):
		msg = [(self.pid >> i & 0xff) for i in (24,16,8,0)]
		return str(self.listToMqttStr(msg))

	#length of sending data
	def msgLen(self, data):
		items = data.split(',')
		items.pop() #remove last value - is empty
		return len(items)*2 #count of data + comma

	#head of message
	#PID,CHANNEL,BITRATE,BAND,RSSI
	def createHead(self):
		msg = self.pidToStr() + ','
		msg += str(self.channel) + ','
		msg += str(self.bitrate) + ','
		msg += str(self.band) + ','
		msg += str(self.rssi) + ','
		return msg

	#sending message to mosquitto
	#HEAD,PACKET_LEN,PACKET_DATA
	def sendData(self, topic, data):
		msg = self.createHead()
		msg += str(data)
		self.mqtt.publish(topic, msg)

	#resend message to mosquitto
	def reSendMsg(self, msgList):
		msgList = deepcopy(msgList)
		del msgList[0:8] #remove PID - sender

		#added PID - recipient
		msg = self.createHead()
		msg += ','.join(str(i) for i in msgList)
		self.mqtt.publish(self.dataToTopic, msg)

	#config string
	def createMsg(self, name, value):
		msg = ""
		msg += str(self.table[name][1]) + ','

		#hack pre velkost dat, aby sa poslala naozaj skutocna velkost,
		#osetrenie kvoli preteceniu na zariadeniach
		if (name != 'data'):
			msg += str(self.table[name][2]) + ','
		else:
			msg += str(len(str(value).split(','))) + ',';

		msg += str(value) +','
		return msg

	#config data from device
	def get_config(self):
		msg = self.createMsg('cmd', 6)
		self.sendData(self.configTopic, msg)
		print "<<<get config from device\n"

	#send message to device
	def fitp_send(self, tocoord, toed, data, data_len):
		msg = self.createMsg('tocoord', tocoord)
		msg += self.createMsg('toed', self.listToMqttStr(toed))
		msg += self.createMsg('data_len', data_len)
		msg += self.createMsg('data', self.listToMqttStr(data))
		msg += self.createMsg('cmd', 0)

		self.sendData(self.configTopic, msg)
		print "<<<fitp_send: ", msg

	def set_edid(self, value):
		msg = self.createMsg('edid', self.listToMqttStr(value))
		self.sendData(self.configTopic, msg)
		print "<<<set edid"

	def set_cid(self, value):
		msg = self.createMsg('cid', int(value))
		self.sendData(self.configTopic, msg)
		print "<<<set cid"

	def set_nid(self, value):
		msg = self.createMsg('nid', self.listToMqttStr(value))
		self.sendData(self.configTopic, msg)
		print "<<<set nid"

	def set_channel(self, value):
		msg = self.createMsg('channel', int(value));
		self.sendData(self.configTopic, msg)
		print "<<<set channel"

	def set_bitrate(self, value):
		msg = self.createMsg('bitrate', int(value));
		self.sendData(self.configTopic, msg)
		print "<<<set bitrate"

	def set_band(self, value):
		msg = self.createMsg('band', int(value));
		self.sendData(self.configTopic, msg)
		print "<<<set band"

	def set_rssi(self, value):
		msg = self.createMsg('rssi', int(value));
		self.sendData(self.configTopic, msg)
		print "<<<set rssi"

	def set_refresh_time(self, value):
		msg = self.createMsg('refresh_time', int(value))
		self.sendData(self.configTopic, msg)
		print "<<<set refresh time"

	def set_pcid(self, value):
		msg = self.createMsg('pcid', int(value))
		self.sendData('pcid', int(value))
		print "<<<set parent cid - pcid"

	def set_sleep(self, value):
		time.sleep(float(value))
		print "<<<set sleep"

	#basic config of device
	def setConfig(self, listValue):
		data = ""
		for value in listValue:
			if (value == 'edid'):
				self.set_edid(self.edid)
			elif (value == 'cid'):
				self.set_cid(self.cid)
			elif (value == 'nid'):
				self.set_nid(self.nid)
			elif (value == 'channel'):
				self.set_channel(self.channel)
			elif (value == 'bitrate'):
				self.set_bitrate(self.bitrate)
			elif (value == 'band'):
				self.set_band(self.band)
			elif (value == 'rssi'):
				self.set_rssi(self.rssi)
			elif (value == 'refresh_time'):
				self.set_refresh_time(self.refresh_time)
			elif (value == 'pcid'):
				self.set_pcid(self.pcid)
			else:
				print "Unknown argument."
			time.sleep(0.01)

	def unsupportedCommand(self):
		print "Unsupported command.";

############
#HexaSen
############
class HexaSen(Device):
	def __init__(self, mqtt):
		super(HexaSen, self).__init__(mqtt)
		time.sleep(0.5)

		#f = open(self.log_directory + "ed" + str(self.id), 'a')

		#make and run
		if not os.path.exists("./build/HexaSen"):
			subprocess.call(["./build.sh", "-ted", "-px86", "--empty"])

		process = subprocess.Popen('./build/hexa_sen')
		self.pid = process.pid

	#send join message
	def fitp_join(self):
		msg = self.createMsg('cmd', 1)
		self.sendData(self.configTopic, msg)
		print "<<<fitp join"

	#body of program
	def run(self):
		time.sleep(0.1)
		self.setConfig(['edid', 'pcid', 'nid', 'channel', 'bitrate', 'band', 'rssi', 'refresh_time'])

		while self.signal_stop:
			for item in self.command:
				#fitp_send - 0
				if "fitp_send(" in str(item):
					args = re.findall(r'=(.*?)[;\)]',item)
					self.fitp_send(args[0], self.stringTolist(args[1]), self.stringTolist(args[2]), args[3])

				#fitp_join - 1
				elif str(item) == "fitp_join":
					self.fitp_join()

				#reset - 5
				elif str(item) == "reset":
					for x in self.command:
						if "reset" != x:
							self.command.pop(0)
						else:
							self.command.pop(0)
						print "reset"
					break

				#get_config - 6
				elif str(item) == "get_config":
					self.get_config();

				#sleep - 7
				elif "sleep(" in str(item):
					value = self.command_value(item)
					print "sleep", value
					time.sleep(value)

				#set_edid - 1
				elif "set_edid(" in str(item):
					args = re.findall(r'=(.*?)[;\)]',item)
					self.set_edid(self.stringTolist(args[0]))

				#set_nid - 3
				elif "set_nid(" in str(item):
					args = re.findall(r'=(.*?)[;\)]',item)
					self.set_nid(self.stringTolist(args[0]))

				#set_channel - 4
				elif "set_channel(" in str(item):
					value = self.command_value(item)
					self.set_channel(value)

				#set_bitrate - 5
				elif "set_bitrate(" in str(item):
					value = self.command_value(item)
					self.set_bitrate(value)

				#set_band - 6
				elif "set_band(" in str(item):
					value = self.command_value(item)
					self.set_band(value)

				#set_rssi - 7
				elif "set_rssi(" in str(item):
					value = self.command_value(item)
					print item
					self.set_rssi(value)

				#set_refresh_time - 12
				elif "set_refresh_time(" in str(item):
					value = self.command_value(item)
					self.set_refresh_time(value)

				#set_pcid - 13
				elif "set_pcid(" in str(item):
					value = self.command_value(item)
					self.set_pcid(value)

				else:
					self.unsupportedCommand()
					self.command = []
				time.sleep(0.1)
			time.sleep(0.2)

############
#Coord
############
class Coord(Device):
	def __init__(self, mqtt):
		super(Coord, self).__init__(mqtt)
		time.sleep(0.5)

		#f = open(self.log_directory + "ed" + str(self.id), 'a')

		#make and run
		if not os.path.exists("./build/coord"):
			subprocess.call(["./build.sh", "-tcoord", "-px86", "--empty"])

		process = subprocess.Popen('./build/coord')
		self.pid = process.pid

	#send joining message
	def fitp_join(self):
		msg = self.createMsg('cmd', 1)
		self.sendData(self.configTopic, msg)
		print "<<<fitp_join"

	def fitp_joined(self):
		self.sendData("")

	#body of program
	def run(self):
		time.sleep(0.1)
		self.setConfig(['edid', 'cid', 'nid', 'channel', 'bitrate', 'band', 'rssi', 'pcid'])

		while self.signal_stop:
			for item in self.command:
				#fitp_send - 0
				if "fitp_send(" in str(item):
					args = re.findall(r'=(.*?)[;\)]',item)
					self.fitp_send(args[0], self.stringTolist(args[1]), self.stringTolist(args[2]), args[3])

				#fitp_join - 1
				elif str(item) == "fitp_join":
					self.fitp_join()

				#reset - 5
				elif str(item) == "reset":
					for x in self.command:
						if "reset" != x:
							self.command.pop(0)
						else:
							self.command.pop(0)
						print "reset"
					break

				#get_config - 6
				elif str(item) == "get_config":
					self.get_config();

				#sleep - 7
				elif "sleep(" in str(item):
					value = self.command_value(item)
					print "sleep", value
					time.sleep(value)

				#set_edid - 1
				elif "set_edid(" in str(item):
					args = re.findall(r'=(.*?)[;\)]',item)
					self.set_edid(self.stringTolist(args[0]))

				#set_cid - 2
				elif "set_cid(" in str(item):
					value = self.command_value(item)
					self.set_cid(value)

				#set_nid - 3
				elif "set_nid(" in str(item):
					args = re.findall(r'=(.*?)[;\)]',item)
					self.set_nid(self.stringTolist(args[0]))

				#set_channel - 4
				elif "set_channel(" in str(item):
					value = self.command_value(item)
					self.set_channel(value)

				#set_bitrate - 5
				elif "set_bitrate(" in str(item):
					value = self.command_value(item)
					self.set_bitrate(value)

				#set_band - 6
				elif "set_band(" in str(item):
					value = self.command_value(item)
					self.set_band(value)

				#set_rssi - 7
				elif "set_rssi(" in str(item):
					value = self.command_value(item)
					print item
					self.set_rssi(value)

				#set_pcid - 13
				elif "set_pcid(" in str(item):
					value = self.command_value(item)
					self.set_pcid(value)

				else:
					self.unsupportedCommand()
					self.command = []
				time.sleep(0.1)
			time.sleep(0.2)

############
#Pan
############
class Pan(Device):
	def __init__(self, mqtt):
		super(Pan, self).__init__(mqtt)
		time.sleep(0.5)
		#f = open(self.log_directory + "pan" + str(self.id), 'a')

		#make and run
		if not os.path.exists("./build/Pan"):
			subprocess.call(["./build.sh", "-tpan", "-px86", "--empty"])

		process = subprocess.Popen('./build/pan')
		self._pid = process.pid

	#start joining timer
	def start_joining(self):
		msg = self.createMsg('cmd', 3)
		self.sendData(self.configTopic, msg)
		print "<<<start joining timer", "id", self.id

	#body of program
	def run(self):
		time.sleep(0.1)
		self.setConfig(['nid','channel', 'bitrate', 'band', 'rssi'])

		while self.signal_stop:
			for item in self.command:
				#fitp_send - 0
				if "fitp_send(" in str(item):
					args = re.findall(r'=(.*?)[;\)]',item)
					self.fitp_send(args[0], self.stringTolist(args[1]), self.stringTolist(args[2]), args[3])

				#start_joining - 3
				elif str(item) == "start_joining":
					self.start_joining()

				#reset - 5
				elif str(item) == "reset":
					for x in self.command:
						if "reset" != x:
							self.command.pop(0)
						else:
							self.command.pop(0)
						print "reset"
					break

				#get_config - 6
				elif str(item) == "get_config":
					self.get_config();

				#set_nid - 3
				elif "set_nid(" in str(item):
					args = re.findall(r'=(.*?)[;\)]',item)
					self.set_nid(self.stringTolist(args[0]))

				#set_channel - 4
				elif "set_channel(" in str(item):
					value = self.command_value(item)
					self.set_channel(value)

				#set_bitrate - 5
				elif "set_bitrate(" in str(item):
					value = self.command_value(item)
					self.set_bitrate(value)

				#set_band - 6
				elif "set_band(" in str(item):
					value = self.command_value(item)
					self.set_band(value)

				#set_rssi - 7
				elif "set_rssi(" in str(item):
					value = self.command_value(item)
					print item
					self.set_rssi(value)

				else:
					self.unsupportedCommand()
					self.command = []
				time.sleep(0.1)
			time.sleep(0.2)
