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
        <symbol name="RU000" resolution="minute" realtime="false">
            <source type="file" location="..\\RU000.ts" format="bin" />
            <contract>
                <multiple>10</multiple>
                <pricetick>5</pricetick>
                <commission type="percentage" value="0.0002" />
            </contract>
        </symbol>
    </datafeed>
    <!-- ============================================
          Strategies 
         ============================================ -->
    <strategy>
        <name>maxd</name>
        <description>MA Cross Delay model</description>
        <author>zhuangshaobo</author>
        <entry>example.dll</entry>
        <mode>standalone</mode>
        <parameters>
            <param name="MA1Period" type="int" value="15">
            </param>
            <param name="MA2Period" type="int" value="40">
            </param>
            <param name="Delay" type="int" value="185">
            </param>
        </parameters>
    </strategy>
    <report>
        <summary>
          <file location="summary.txt" />
        </summary>
        <trade>
            <file location="trades.csv" />
        </trade>
        <!--
        <return>
            <file location="returns.csv" />
        </return>
        -->
    </report>
</scenario>
