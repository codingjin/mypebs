import pandas as pd
import matplotlib.pylab as plt
import csv
import os
import sys
import ast

argvlen = len(sys.argv)
if argvlen != 2:
	print("Usage: python stat.py profile_filename")
	exit(0)

#df_address = pd.read_csv("profiling_result")
df_address = pd.read_csv(sys.argv[1])

#print(df_address)

df_address_list = df_address['Address'].values.tolist()

#print(df_address_list)

new_addresses = []
for tmp in df_address_list:
	addr = ast.literal_eval(str(tmp))
	new_addresses.append(addr)

	#if addr>=34186810545 and addr<=34226810545:
	#	new_addresses.append(addr)
	#else:
	#	print(addr, "Not appended")
print(len(new_addresses))

min_address = min(new_addresses)
max_address = max(new_addresses)
print(min_address)
print(max_address)

#plt.hist(new_addresses, range=[1.39813468440448e+14, 1.40728425785584e+14], bins=500)
#plt.hist(new_addresses, range=[139105922211856, 139761282211848], bins=500)
plt.hist(new_addresses, log=True, range=[min_address, max_address])

plt.xlabel('Virtual Address')
plt.ylabel('Freq')
plt.title('Store Freq of Virtual Addr')

plt.show()

