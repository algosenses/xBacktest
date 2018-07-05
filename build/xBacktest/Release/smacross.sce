<!-- the scenario configuration file -->
<?xml version="1.0" encoding="UTF-8"?>
<scenario>
    <!-- ============================================
          Environment configuration
         ============================================ -->
    <environment>
        <core num="autodetect" />
        <optimizing mode="genetic" />
    </environment>
    <!-- ============================================
          Broker 
         ============================================ -->
    <broker type="backtesting">
        <cash>1000</cash>
        <iop>false</iop>
    </broker>
    <!-- ============================================
          Data Feed 
         ============================================ -->
    <datafeed>
        <symbol name="orcl" resolution="day" realtime="false">
            <source type="file" location="..\\orcl-2000.csv" format="csv" />
        </symbol>
    </datafeed>
    <!-- ============================================
          Strategies 
         ============================================ -->
    <strategy>
        <name>smacross</name>
        <description>SMA cross model</description>
        <author>zhuangshaobo</author>
        <entry>example.dll</entry>
        <mode>standalone</mode>
        <parameters>
            <param name="period" type="int" value="20">
<!--                <optimizing start="1" end="150" step="1" /> -->
            </param>
            <param name="StopLossPct" type="double" value="0.01">
<!--                <optimizing start="0.01" end="0.99" step="0.01" /> -->
            </param>
            <param name="StopProfitPct" type="double" value="0.17">
<!--                <optimizing start="0.01" end="0.99" step="0.01" /> -->
            </param>
            <param name="DrawdownPct" type="double" value="0.01">
<!--                <optimizing start="0.01" end="0.99" step="0.01" /> -->
            </param>
        </parameters>
    </strategy>
    
    <!-- ============================================
          Analysis report
         ============================================ -->
    <report>
        <summary>
          <file location="summary.txt" />
        </summary>
        <trade>
            <file location="trades.csv" />
        </trade>
        <return>
            <file location="returns.csv" />
        </return>
    </report>
</scenario>
