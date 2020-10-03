import numpy as np
import matplotlib.pyplot as plt
import os

os.system('./make.sh')
path_to_csv = "data.csv"
max_num = 16

outfile = open(path_to_csv, 'w')

for y in range(1, max_num):
    for x in range(1, max_num):
        stream = os.popen('./a.out ' + str(x) + ' ' + str(y))
        outfile.write(stream.read() + '; ')
    outfile.write('\n')

outfile.close()


data = np.genfromtxt(path_to_csv, delimiter=";")
plt.imshow(data, cmap='plasma', extent=[0.5,max_num+0.5,max_num-0.5,0.5])
plt.gca().invert_yaxis()
ax = plt.gca()
ax.set_xlim(0.5, max_num - 0.5)
plt.xlabel("External threads")
plt.ylabel("Internal threads")
plt.show()
