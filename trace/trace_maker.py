
type_write = 'W'
type_read = 'R'
trace_size = 1
trace_startTime = 0
trace_data = ['a','b','c','d','e','f','g','h',
              'i','g','k','l','m','n','o','p',
              'q','r','s','t','u','v','w','x',
              'y','z','A','B','C','D','E','F']

trace_file = "trace.txt"


open(trace_file, 'w').close()


# &type, &logical_address, &size, &start_time, &data

f = open(trace_file, "a")
for i in range(1000):
   line = type_write + "," + str(i) + "," + str(trace_size) + "," + str(trace_startTime) + "," + trace_data[0] + "\n"
   f.writelines(line)

f.close()


print("finish creating!")