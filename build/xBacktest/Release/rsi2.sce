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
          Data Feed 
         ============================================ -->
    <datafeed>
        <symbol name="dia" resolution="day" realtime="false">
            <source type="file" location="..\\DIA-2009.csv" format="csv" />
        </symbol>
        <symbol name="dia" resolution="day" realtime="false">
            <source type="file" location="..\\DIA-2010.csv" format="csv" />
        </symbol>
        <symbol name="dia" resolution="day" realtime="false">
            <source type="file" location="..\\DIA-2011.csv" format="csv" />
        </symbol>
        <symbol name="dia" resolution="day" realtime="false">
            <source type="file" location="..\\DIA-2012.csv" format="csv" />
        </symbol>
    </datafeed>
    <!-- ============================================
          Strategies 
         ============================================ -->
    <strategy>
        <name>rsi2</name>
        <description>RSI2 model</description>
        <author>zhuangshaobo</author>
        <entry>example.dll</entry>
        <mode>standalone</mode>
        <parameters>
            <param name="entrySmaPeriod" type="int" value="103">
<!--               <optimizing start="100" end="300" step="1" /> -->
            </param>
            <param name="exitSmaPeriod" type="int" value="7">
<!--               <optimizing start="5" end="15" step="1" /> -->
            </param>
            <param name="rsiPeriod" type="int" value="2">
<!--               <optimizing start="2" end="10" step="1" /> -->
            </param>
            <param name="overBoughtThreshold" type="int" value="90">
<!--                <optimizing start="5" end="25" step="1" /> -->
            </param>
            <param name="overSoldThreshold" type="int" value="10">
<!--                <optimizing start="75" end="95" step="1" /> -->
            </param>
            <param name="StopProfitPct" type="double" value="0.17">
                <optimizing start="0.01" end="0.99" step="0.01" />
            </param>
            <param name="DrawdownPct" type="double" value="0.01">
                <optimizing start="0.01" end="0.99" step="0.01" /> 
            </param>
        </parameters>
    </strategy>
</scenario>
