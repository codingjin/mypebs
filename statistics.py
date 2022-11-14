import pandas as pd
import matplotlib.pylab as plt
import csv
import os
import sys
import ast



df_address = pd.read_csv("profiling_result")

#print(df_address)

df_address_list = df_address['Address'].values.tolist()

#print(df_address_list)

new_addresses = []
for addr in df_address_list:
	new_addresses.append(ast.literal_eval(str(addr)))
print(len(new_addresses))

'''
count = 0

for add in new_addresses:
	if add >= 139228521369616 and add <= 139883881369608:
		count += 1

print(count)
print(len(new_addresses))
print(count/len(new_addresses))
'''

min_address = min(new_addresses)
max_address = max(new_addresses)

print(min_address)
print(max_address)


plt.hist(new_addresses, range=[1.39813468440448e+14, 1.40728425785584e+14], bins=500)
#plt.hist(new_addresses, range=[139105922211856, 139761282211848], bins=500)

plt.xlabel('Virtual Address')
plt.ylabel('Freq')
plt.title('Store Freq of Virtual Addr')

plt.show()

