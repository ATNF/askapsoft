snoop_utility = "tVisSourceSnoop.sh"
default_path = "/group/askap/vor010/ASKAPsdp_ingest/Code/Components/Services/ingest/current"

# mapping between block, ingest machine and port corresponding to the first card
current_ingest_config = {7:(2,16456), 6:(14,16444), 5:(13,16432), 4:(10,16420), 3:(9,16408)}

import os,  sys

path2snoop = None
if os.path.exists(snoop_utility):
   path2snoop = snoop_utility
elif os.path.exists(os.path.join("apps",snoop_utility)):
   path2snoop = os.path.join("apps",snoop_utility)
else:
    path2snoop = os.path.join(default_path, "apps",snoop_utility)

#if not os.path.exists(path2snoop):
#   raise RuntimeError, "Unable to find snoop utility: %s" % (path2snoop,)

if len(sys.argv) not in [1,3,5]:
   raise RuntimeError, "Usage: %s [-b block] [-c card]" % (sys.argv[0],)

block = None
card = None

if len(sys.argv) > 1:
   if sys.argv[1] == "-b":
      block = int(sys.argv[2])
   elif sys.argv[1] == "-c":
      card = int(sys.argv[2])
   else:
      raise RuntimeError, "Unknown option %s" % (sys.argv[1],)

if len(sys.argv) == 5:
   if sys.argv[3] == "-b":
      if block != None:
         raise RuntimeError, "duplicated -b option"
      block = int(sys.argv[4])
   elif sys.argv[3] == "-c":
      if card != None:
         raise RuntimeError, "duplicated -c option"
      card = int(sys.argv[4])
   else:
      raise RuntimeError, "Unknown option %s" % (sys.argv[3],)

if card != None:
   if card < 1 or card > 12:
      raise RuntimeError, "There are 12 correlator cards per correlator block, you have card = %i" % (card,)

#print block, card

def runSnoop(block, card, count = 10, verbose = True):
   """
      make parset and run tVisSourceSnoop
      the output is redirected into out.dat for additional analysis
   """
   if block == None:
      raise RuntimeError, "Block has to be defined!"
   if block not in current_ingest_config:
      raise RuntimeError, "Configuration for block %i has not been defined" % (block,)
   tmp_parset_file = os.path.join(os.path.abspath("./"),".tmp.tVisSourceSnoop.in")
   tmp_result_file = os.path.join(os.path.abspath("./"),"out.dat")
   tmp_hosts_file = os.path.join(os.path.abspath("./"),".tmp.tVisSourceSnoop.hosts")
   
   if len(current_ingest_config[block]) != 2:
      raise RuntimeError, "Expect 2-element tuple in current_ingest_config, you have %s" % (current_ingest_config,)
   port = current_ingest_config[block][1]
   nranks = 12
   if card != None:
      port += card - 1
      nranks = 1
   with open(tmp_parset_file,"w") as f:
      f.write("buffer_size = 124416\n")
      f.write("vis_source.max_beamid = 36\n")
      f.write("vis_source.max_slice = 3\n")
      f.write("vis_source.port = %i\n" % (port,))
      f.write("vis_source.receive_buffer_size = 67108864\n")
      f.write("count = %i\n" % (count,))
   with open(tmp_hosts_file,"w") as f:
      f.write("# automatically generated hosts file\n")
      f.write("10.10.101.%i:%i\n" % (200+current_ingest_config[block][0],nranks))
   if verbose:
      os.system("mpirun -np %i -f %s %s -c %s | tee %s" % (nranks, tmp_hosts_file, path2snoop, tmp_parset_file, tmp_result_file))
   else:
      os.system("mpirun -np %i -f %s %s -c %s > %s" % (nranks, tmp_hosts_file, path2snoop, tmp_parset_file, tmp_result_file))
   # parse output to present summary (and leave the file for future investigation)
   with open(tmp_result_file) as f:
      ranksSighted = [False] * nranks
      expected = 31104
      for line in f:
          if "datagrams for Epoch:" in line:
             pos = line.find("- rank")
             if pos == -1:
                raise RuntimeError, "Unable to parse the log: %s" % (line,)
             parts=line[pos:].split()
             if len(parts)<9:
                raise RuntimeError, "Unable to parse the log: %s" % (parts,)
             received = int(parts[4])
             rank = int(parts[2])
             if rank >= len(ranksSighted):
                raise RuntimeError, "Unexpected rank in the log: rank=" % (rank,)
             # ignoring the first one which is always smaller than expected
             if not ranksSighted[rank]:
                ranksSighted[rank] = True
                continue
             if received > expected:
                raise RuntimeError, "Received %i datagrams! This shouldn't happen" % (received,)
             if received < expected:
                curCard = card
                if curCard == None:
                   curCard = rank + 1
                print "Missed %i datagrams from card %i of block %i at epoch %s" % (expected-received, curCard, block, parts[8])
             
   
if block == None:
   for block in current_ingest_config:
       runSnoop(block,card, count = 10, verbose = False)
else:
   runSnoop(block,card)
