import argprase
import matplotlib.pyplot as plt
import csv
  
x = []
y = []

parser = argprase.ArgumentParser(description="Plots from a CSV file")
parser.add_argument('file', help="Full path of CSV file", required=True)
parser.add_argument('col_name', help="Column name that needs to be plotted", required=True)
args = parser.parse_args()
  
with open(args.file,'r') as csvfile:
    plots = csv.reader(csvfile, delimiter = ',')
      
    for row in plots:
        x.append(row[0])
        y.append(int(row[2]))
  
plt.bar(x, y, color = 'g', width = 0.72, label = args.col_name)
# plt.xlabel('Workload')
plt.ylabel(args.col_name)
plt.legend()
plt.show()