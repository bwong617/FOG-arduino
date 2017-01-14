\import fileinput

print ("[START]")

line_counter = 0

filename = 'S01R01'

f1 = open(filename + '.txt', 'r')
f2 = open(filename + '_exp.txt', 'w')
f3 = open(filename + '_normal.txt', 'w')
f4 = open(filename + '_freeze.txt', 'w')         

for line in f1:

    line_data = line.split()

    if line_data[10] != "0":
        f2.write(line_data[2] + ",")
    if line_data[10] == "1":
        f3.write(line_data[2] + ",")
    if line_data[10] == "2":
        f4.write(line_data[2] + ",")
    
f1.close()
f2.close()
f3.close()
f4.close()

print ("[END]")
