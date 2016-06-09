from sys import argv
import requests

script, path, command = argv

proto = "http"
host = "soundboard.lan"
port = 3000

if (False):
	print(path + "commands.log")
	target = open(path + "commands.log", 'a')
	target.write(command + "\n")
	target.close()

r = requests.post(proto + "://" + host + ":" + str(port), data={'json': command})
