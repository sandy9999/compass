import matplotlib.pyplot as plt
import numpy as np


def bvecs_read(fname):
    a = np.fromfile(fname, dtype=np.int32, count=1)
    b = np.fromfile(fname, dtype=np.uint8)
    d = a[0]
    return b.reshape(-1, d + 4)[:, 4:].copy()


def ivecs_read(fname):
    a = np.fromfile(fname, dtype='int32')
    d = a[0]
    return a.reshape(-1, d + 1)[:, 1:].copy()


def fvecs_read(fname):
    return ivecs_read(fname).view('float32')

# a = fvecs_read("./build/full_latency_laion.bin")


# Data
categories = ['A', 'B', 'C', 'D']
values = [20, 35, 30, 35]

# Example errors (standard deviation or uncertainty)
errors = [2, 3, 4, 1]

# Bar graph with error bars
plt.bar(categories, values, yerr=errors, capsize=5)

# Labels and title
plt.xlabel('Category')
plt.ylabel('Value')
plt.title('Bar Graph with Error Bars')

# Show plot
plt.savefig('latency_breakdown.pdf') 
