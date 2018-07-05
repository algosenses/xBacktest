import os
import sys
import shutil

root_dir = 'x:/E/Workshop/YuanLang/Solution/xBacktest'
sdk_dir = 'x:/E/Workshop/YuanLang/Solution/xBacktest-SDK'

incl_files = [
    'Series/circular.h',
    'Series/BarSeries.h',
    'Series/DataSeries.h',
    'Broker/Order.h',
    'Strategy/Position.h',
    'Strategy/Strategy.h',
    'Core/Event.h',
    'Core/Bar.h',
    'Core/Defines.h',
    'Core/Simulator.h',
    'Core/Config.h',
    'Technical/Cross.h',
    'Technical/HighLow.h',
    'Technical/MA.h',
    'Technical/MACD.h',
    'Technical/RSI.h',
    'Technical/ATR.h',
    'Technical/KDJ.h',
    'Technical/Technical.h',
    'Utils/DateTime.h',
    'Utils/Date.h',
    'Utils/Export.h',
    'Utils/Utils.h',
    'Utils/Vector.h'
]

lib_file = 'xBacktest.lib'
bin_file = 'xBacktest.dll'
doc_file = 'xBacktest Programming Guide.pdf'

for f in incl_files:
    src = root_dir + '/Source/' + f
    dst = sdk_dir + '/Inc'
    shutil.copy2(src, dst)

shutil.copy2(root_dir + '/doc/' + doc_file, sdk_dir + '/doc')

debug = False

if len(sys.argv) == 2 and sys.argv[1] == 'debug':
    debug = True

if debug:
    shutil.copy2(root_dir + '/build/xBacktest/x64/Debug/' + lib_file, sdk_dir + '/Lib/xBacktest.lib')
    shutil.copy2(root_dir + '/build/xBacktest/x64/Debug/' + bin_file, sdk_dir + '/Bin/xBacktest.dll')
else:
    shutil.copy2(root_dir + '/build/xBacktest/x64/Release/' + lib_file, sdk_dir + '/Lib/xBacktest.lib')
    shutil.copy2(root_dir + '/build/xBacktest/x64/Release/' + bin_file, sdk_dir + '/Bin/xBacktest.dll')

