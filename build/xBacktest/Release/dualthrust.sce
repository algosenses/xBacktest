<!-- the scenario configuration file -->
<?xml version="1.0" encoding="UTF-8"?>
<scenario>
    <!-- ============================================
          Machine configuration
         ============================================ -->
    <machine>
        <core num="autodetect" />
    </machine>
    <!-- ============================================
          Broker 
         ============================================ -->
    <broker type="backtesting">
        <cash>1000000</cash>
    </broker>
    <!-- ============================================
          Data feed, the first symbol is the default
          instrument use to feed strategy
         ============================================ -->
    <datafeed>
        <symbol name="IF888" resolution="minute" realtime="false">
            <source type="file" location="..\\IF888.ts" format="bin" />
        </symbol>        
        <symbol name="IF888" resolution="day" realtime="false">
            <source type="file" location="..\\IF888_Day.ts" format="bin" />
        </symbol>
    </datafeed>
    <!-- ============================================
          Strategies 
         ============================================ -->
    <strategy>
        <name>DualThrust</name>
        <description>Dual Thrust model</description>
        <author>zhuangshaobo</author>
        <entry>example.dll</entry>
        <mode>standalone</mode>
        <parameters>
            <param name="PastNDays" type="int" value="5">
            </param>
            <param name="Ks" type="double" value="0.3">
            </param>
            <param name="Kx" type="double" value="0.3">
            </param>
        </parameters>
    </strategy>
</scenario>
