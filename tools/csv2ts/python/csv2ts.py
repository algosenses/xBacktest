# coding: UTF-8
import sys, argparse, csv
import datetime
from teafiles import *

# command arguments
parser = argparse.ArgumentParser(description='csv to covert', fromfile_prefix_chars="@" )
parser.add_argument('infile', help='csv file to import', action='store')
parser.add_argument('outfile', help='ts file to save', action='store')

args = parser.parse_args()
csv_infile = args.infile
ts_outfile = args.outfile

# open csv file
with open(csv_infile, 'rb') as csvfile:
    csvfile.seek(0)

    reader = csv.reader(csvfile, delimiter=',')


    # create the file to store the received values
    with TeaFile.create(ts_outfile, "Time Open High Low Close Volume OpenInt", "qddddqq", 'rb1505') as tf:
        header = next(reader, None)

        for row in reader:
         #   t = DateTime.parse(row[0] + ' ' + row[1], '%m/%d/%Y %H:%M')
            t = DateTime.parse(row[0] + ' ' + row[1], '%Y-%m-%d %H:%M')
            open_ = float(row[2])
            high = float(row[3])
            low = float(row[4])
            close = float(row[5])
            volume = int(row[7])
            openint = int(row[8])

            tf.write(t, open_, high, low, close, volume, openint)
