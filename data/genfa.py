#!/usr/bin/env python

import sys
import getopt
import numpy as np

random_seed = 0
num_reads = 10
avg_readlen = 35
sd_readlen = 5.0
name_prefix = "read"

def usage():
    global num_reads, avg_readlen, sd_readlen, name_prefix
    sys.stderr.write("Usage: {} [options]\n".format(sys.argv[0]))
    sys.stderr.write("Options:\n")
    sys.stderr.write("    -n INT   number of reads [{}]\n".format(num_reads))
    sys.stderr.write("    -l INT   average read length [{}]\n".format(avg_readlen))
    sys.stderr.write("    -u FLOAT std dev. of read length [{}]\n".format(sd_readlen))
    sys.stderr.write("    -N STR   read name prefix ['{}']\n".format(name_prefix))
    sys.stderr.write("    -s INT   rng seed [random]\n")
    sys.stderr.write("    -h       help message\n")
    sys.stderr.flush()
    return -1

def main(argc, argv):
    global num_reads, avg_readlen, sd_readlen, name_prefix

    try: opts, args = getopt.gnu_getopt(argv[1:], "n:l:u:N:s:h")
    except getopt.GetoptError as err:
        sys.stderr.write("error: {}\n".format(err))
        return usage()

    for o, a in opts:
        if o == "-h": return usage()
        elif o == "-n": num_reads = int(a)
        elif o == "-l": avg_readlen = int(a)
        elif o == "-u": sd_readlen = float(a)
        elif o == "-N": name_prefix = a
        elif o == "-s": np.random.seed(int(a))

    readlens = np.random.normal(avg_readlen, sd_readlen, num_reads).astype(int)

    lines = []

    for i, readlen in enumerate(readlens):
        read_name = "{}_{}".format(name_prefix,i+1)
        read_comment = "len={}".format(readlen)
        base_codes = np.random.randint(0, 4, readlen)
        read_seq = "".join("ACGT"[c] for c in base_codes)
        lines.append(">{}\t{}".format(read_name, read_comment))
        lines.append(read_seq)

    sys.stdout.write("\n".join(lines) + "\n")
    #  sys.stdout.write("\n".join(lines))
    sys.stdout.flush()

    return 0

if __name__ == "__main__":
    sys.exit(main(len(sys.argv), sys.argv))
