import glob
import json
import struct

regs = {"ax":0,"cx":1,"dx":2,"bx":3,"sp":4,"bp":5,"si":6,"di":7,"es":8,"cs":9,"ss":10,"ds":11,"flags":12,"ip":13}

def process():
    for filename in glob.glob("c:\\tests\\*.json"):
        with open(filename, 'r') as file:
            data = json.load(file)
            
        with open(filename.replace('.json', '.bin'), 'wb') as binfile:
            for entry in data:
                initial_regs = [0]*14
                    
                for reg in entry["initial"]["regs"]:
                    initial_regs[regs[reg]] = entry["initial"]["regs"][reg]
                    
                final_regs = initial_regs.copy()
                for reg in entry["final"]["regs"]:
                    final_regs[regs[reg]] = entry["final"]["regs"][reg]
                
                #bytedata = [len(entry["bytes"])]
                #bytedata.extend(entry["bytes"])
                
                initial_ram = [len(entry["initial"]["ram"])]
                for ram in entry["initial"]["ram"]:
                    initial_ram.extend(ram)
                final_ram = [len(entry["final"]["ram"])]
                for ram in entry["final"]["ram"]:
                    final_ram.extend(ram)
                
                #binfile.write(struct.pack(f'{len(bytedata)}B', *bytedata))
                binfile.write(struct.pack(f'{len(initial_regs)}H', *initial_regs))
                binfile.write(struct.pack(f'{len(initial_ram)}I', *initial_ram))
                binfile.write(struct.pack(f'{len(final_regs)}H', *final_regs))
                binfile.write(struct.pack(f'{len(final_ram)}I', *final_ram))

process()