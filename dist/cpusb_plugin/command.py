from sys import argv
import requests

script, path, id, unique, name, command = argv

proto = "http"
host = "soundboard.lan"
port = 3000

if (False):
	print(path + "commands.log")
	target = open(path + "commands.log", 'a')
	target.write(command + "\n")
	target.close()

def decode(data):
	return data.replace("%22", "\\\"").replace("%27", "\\\'")

id = decode(id)
unique = decode(unique)
name = decode(name)
command = decode(command);

payload = { 'client': { 'id': id, 'unique': unique, 'name': name }, 'data': command }
r = requests.post(proto + "://" + host + ":" + str(port), data={'json': command})
