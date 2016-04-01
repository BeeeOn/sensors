#!/usr/bin/python

import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
	client.subscribe("BeeeOn/#")

client = mqtt.Client()
client.on_connect = on_connect

client.connect("localhost", 1883, 60)
client.loop_start()

command_topic = "BeeeOn/set_command"

print 'Type "quit" to exit.'
while (True):
	chyba = False
	msg = ""

	line = raw_input('PROMPT> ')
	line = line.replace(' ', '')

	for i in line:
		msg += str(ord(i)) + ','

	if len(msg) > 0:
		client.publish(command_topic, msg)
