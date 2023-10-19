import os
import time
import json
import math
import sys
import threading
import subprocess

dest_mount_point = ['/mnt/nvme0n1p1/']
cpu_affinity = ["0-15,32-47"]
json_file_name = 'testwriter_conf.json'
hdw_file_name = 'testwriter_hardwaremap.txt'
src_file_name = 'rand_1G.bin'
dest_file_name = 'test'
trigger_count = 1
thread_count = 1
frame_sizes = [2147483648] # limited to 4294967295 bytes for char datatype
block_sizes = [524288, 1048576, 2097152, 4194304, 8388608, 16777216, 33554432]
header_size = 72
element_count = 10

test_to_run = 'HDF5LIBS_write_partframe_test'
output_results_file_name = 'converter_results_part-framewrite_old_no-pre-allocate-flush.txt'

trim_beetween_tests = True
delete_output_file = True

frame_sizes = [ f - header_size for f in frame_sizes]

if len(sys.argv) != 1:
    print('Error: Invalid number of arguments')
    exit()

# Clean and trim
print('Cleaning and trimming...')
for dest in dest_mount_point:
    if delete_output_file:
        os.system('rm -f ' + dest + '*.hdf5 && sync')
        os.system('rm -f ' + dest + '*.hdf5.writing && sync')
        if trim_beetween_tests:
            os.system('sudo fstrim -v '+ dest)

print('Waiting 30s before start...')
time.sleep(30)

for frame_size in frame_sizes:
    for block_size in block_sizes:
        
        print('frame_size: ' + str(frame_size))
        print('element_count: ' + str(element_count))
        print('trigger_count: ' + str(trigger_count))
        print('data_size: ' + str(frame_size * element_count * trigger_count))

        # Update json config file
        with open(json_file_name, 'r+') as f:
            data = json.load(f)
            data['trigger_count'] = trigger_count
            data['element_count'] = element_count
            data['data_size'] = frame_size
            f.seek(0)        # <--- should reset file position to the beginning.
            json.dump(data, f, indent=4)
            f.truncate()     # remove remaining part

        # Measure current memory usage
        print('Measuring current memory usage...')
        free_memory = []
        cpu_usage = []
        free_memory.append(int(subprocess.check_output("free -m | grep Mem | tr -s ' ' | cut -d ' ' -f 4", shell=True)))
        cpu_usage.append(float(subprocess.check_output("mpstat | grep all | tr -s ' ' | cut -d ' ' -f 4", shell=True)))


        # runing tests thread
        threads = []
        for i, dest in enumerate(dest_mount_point):
            for num in range(int(thread_count / len(dest_mount_point))):
                print('Running thread ' + str(num) + ' on ' + dest)
                out_file_name = dest + dest_file_name + "_" + str(num) + ".hdf5"
                
                # runing thread
                t = threading.Thread(target=os.system, args=("taskset --all-tasks -c "+ cpu_affinity[i] +" " + test_to_run + " " + json_file_name + " " + hdw_file_name + " " + dest + src_file_name + " " + out_file_name + " " + str(block_size) +  " >> " + output_results_file_name, ))
                threads.append(t)
                t.start()

        # join threads
        while threads  != []:
            for t in threads:
                if not t.is_alive():
                    threads.remove(t)
            time.sleep(1)
            free_memory.append(int (subprocess.check_output("free -m | grep Mem | tr -s ' ' | cut -d ' ' -f 4", shell=True)))
            cpu_usage.append(float(subprocess.check_output("mpstat | grep all | tr -s ' ' | cut -d ' ' -f 4", shell=True)))

            
        
        print('Test finished. Waiting for 30 seconds...')
        
        # append recorded data to output file
        f = open(output_results_file_name, 'a')

        f.write('Min free memory: ' + str(min(free_memory)) + ' MiB\n')
        f.write('Max memory usage: ' + str(free_memory[0] - min(free_memory)) + ' MiB\n')
        f.write('Mean memory usage: ' + str(free_memory[0] - (sum(free_memory[1:]) / len(free_memory[1:]))) + ' MiB\n')

        f.write('Threads count: ' + str(thread_count) + '\n')
        f.write('Mean CPU usage: ' + str((sum(cpu_usage) / len(cpu_usage))) + '\n')
        
        f.close()

        time.sleep(30)

        # Clean and trim
        print('Cleaning and trimming...')
        for dest in dest_mount_point:
            if delete_output_file:
                os.system('rm -f ' + dest + '*.hdf5 && sync')
                os.system('rm -f ' + dest + '*.hdf5.writing && sync')
                if trim_beetween_tests:
                    os.system('sudo fstrim -v '+ dest)
        
        # wait for a while
        print('Cleaning finished. Waiting for 30 seconds...')
        time.sleep(30)
