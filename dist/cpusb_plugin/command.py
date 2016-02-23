from sys import argv

script, path, command = argv

print(path + "commands.log")
target = open(path + "commands.log", 'a')

target.write(command + "\n")

target.close()
