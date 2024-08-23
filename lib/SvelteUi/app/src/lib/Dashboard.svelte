<script>
    import { dayPlotStore, weekPlotStore, monthPlotStore, minutePlotStore, hourPlotStore } from './DataStores.js'; //EHorvat added: , hourPlotStore, DGF added weekPlotStore, priceStore removed
    import { importcol, exportcol, metertype, uiVisibility } from './Helpers.js'; // importColor for other
    import PowerGauge from './PowerGauge.svelte';
    import FreqPlot from './FreqPlot.svelte';
    import VoltPlot from './VoltPlot.svelte';
    import AmpPlot from './AmpPlot.svelte';
    import AccountingData from './AccountingData.svelte';
    import Last_Counter_Value from './Last_Counter_Value.svelte';
    import ReactiveData from './ReactiveData.svelte';
    //import PricePlot from './PricePlot.svelte'; //DGF removed
    import DayPlot from './DayPlot.svelte';
    import WeekPlot from './WeekPlot.svelte';   // Added week plot 
    import MonthPlot from './MonthPlot.svelte';    
    import MinutePlot from './MinutePlot.svelte';//EHorvat Minute plot added line
    import HourPlot from './HourPlot.svelte';//EHorvat hour plot added line
//    import TemperaturePlot from './TemperaturePlot.svelte';
    import TariffPeakChart from './TariffPeakChart.svelte';
    import { writable } from 'svelte/store';

    export let data = {}
    export let sysinfo = {}
    //let prices = {} DGF removed
    let dayPlot = {}
    let weekPlot = {}  //DGF week plot added
    let monthPlot = {}
    let minutePlot = {} //EHorvat minute plot added line    
    let hourPlot = {} //EHorvat hour plot added line
//    let temperatures = {};
    //DGF removed
    // pricesStore.subscribe(update => {
    //     prices = update;
    // });
    //DGF
    dayPlotStore.subscribe(update => {
        dayPlot = update;
    });
    //DGF Weekplot added
    weekPlotStore.subscribe(update => {
        weekPlot = update;
    });  
    //DGF  
    monthPlotStore.subscribe(update => {
        monthPlot = update;
    });    
    minutePlotStore.subscribe(update => {  //EHorvat minute plot
        minutePlot = update;
    });    
    hourPlotStore.subscribe(update => {     //EHorvat hour plot
        hourPlot = update;
    });
//    temperaturesStore.subscribe(update => {
//        temperatures = update;
//    });
</script>

<div class="grid 2xl:grid-cols-6 xl:grid-cols-5 lg:grid-cols-4 md:grid-cols-3 sm:grid-cols-2">
    {#if uiVisibility(sysinfo.ui.i, data.i)}
        <!-- import power gauge-->
        <div class="cnt">
            <div class="grid grid-cols-2">
                <!--DGF current tariff added-->
                <div class="text-center col-span-2"><b>Bezug</b></div>
                <!--DGF-->
                <div class="col-span-2"><!-- EHorvat translated to german: Import to Bezug -->
                    <PowerGauge val={data.i ? data.i : 0} max={data.im ? data.im : 15000} unit="W" label="{data["tn"+data.trc]}" sub={data.trp} subunit={data.pc} color={importcol(data.trc)}/>
                </div>
                <!--DGF Text changed, font size changed, tiers added-->
                <div class="text-sm">Bezogen total</div>
                <div class="text-sm text-right">{data.ic ? (Math.trunc(data.ic*10)/10).toFixed(1) : '0'} kWh</div> <!--DGF Wrong rounding corrected-->
                {#if data.ti0 && data.ti0 > 0 && data.ti1 && data.ti1 > 0}
                    <div class="text-sm">{data.tn0}</div>
                    <div class="text-sm text-right">{(Math.trunc(data.ti0*10)/10).toFixed(1)} kWh</div> 
                {/if}
                {#if data.ti1 && data.ti1 > 0}
                    <div class="text-sm">{data.tn1}</div>
                    <div class="text-sm text-right">{(Math.trunc(data.ti1*10)/10).toFixed(1)} kWh</div>
                {/if}
                {#if data.ti2 && data.ti2 > 0}
                    <div class="text-sm">{data.tn2}</div>
                    <div class="text-sm text-right">{(Math.trunc(data.ti2*10)/10).toFixed(1)} kWh</div>
                {/if}
                {#if data.ti3 && data.ti3 > 0}
                    <div class="text-sm">{data.tn3}</div>
                    <div class="text-sm text-right">{(Math.trunc(data.ti3*10)/10).toFixed(1)} kWh</div>
                {/if}
                <!--DGF-->
            </div>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.e, data.om || data.e > 0)}
        <!-- export power gauge-->
        <div class="cnt">
            <div class="grid grid-cols-2">
                <div class="text-center col-span-2"><b>Export</b></div>
                <div class="col-span-2"> <!-- EHorvat next line: show fixed income refund ...added  sub={data.pi} subunit={data.pc} -->
                    <PowerGauge val={data.e ? data.e : 0} max={data.om ? data.om * 1000 : 10000} unit="W" label="" sub={data.pi} subunit={data.pc} color={exportcol}/>
                </div>
                <!--DGF Text added, font size changed-->
                
                    <div class="text-sm">Exportiert total</div>
                    <div class="text-sm text-right">{data.ec ? Math.trunc(data.ec*10)/10 : '0'} kWh</div> <!--DGF Wrong rounding corrected-->
                
                <!--DGF-->
            </div>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.f, data.frq)}
        <div class="cnt">
            <FreqPlot frq={data.frq}/>
         </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.v, data.u1 > 100 || data.u2 > 100 || data.u3 > 100)}
        <div class="cnt">
            <VoltPlot u1={data.u1} u2={data.u2} u3={data.u3} ds={data.ds}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.a, data.i1 > 0.01 || data.i2 > 0.01 || data.i3 > 0.01)}
        <div class="cnt">
            <AmpPlot u1={data.u1} u2={data.u2} u3={data.u3} i1={data.i1} i2={data.i2} i3={data.i3} max={data.mf ? data.mf : 32}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.c, data.ea)}
        <div class="cnt">
            <AccountingData sysinfo={sysinfo} data={data.ea} currency={data.pc} hasExport={data.om > 0 || data.e > 0}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.l, data.py)}
        <div class="cnt">
            <Last_Counter_Value last_Counter_Value={data.py} last_Counter_Info={data.pd} last_Counter_Costs={data.pl} last_Counter_Diff={data.pa} last_expCounter_Value={data.pz} last_expCounter_Info={data.pg} last_expCounter_Costs={data.pj} last_expCounter_Diff={data.pk} currency={data.pc} hasExport={uiVisibility(sysinfo.ui.e, data.om || data.e > 0)}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.r, data.ri > 0 || data.re > 0 || data.ric > 0 || data.rec > 0)}
        <div class="cnt">
            <ReactiveData importInstant={data.ri} exportInstant={data.re} importTotal={data.ric} exportTotal={data.rec} PowerFactorL1={data.f1} PowerFactorL2={data.f2} PowerFactorL3={data.f3} AparentPower={data.va} ReactivePower_Q1={data.rq1} ReactivePower_Q2={data.rq2} ReactivePower_Q3={data.rq3} ReactivePower_Q4={data.rq4}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.t, data.pr && (data.pr.startsWith("10YNO") || data.pr == '10Y1001A1001A48H'))}
        <div class="cnt h-64">
            <TariffPeakChart />
        </div>
    {/if}
    <!--DGF removed
    {#if uiVisibility(sysinfo.ui.p, data.pe && !Number.isNaN(data.p))}
        <div class="cnt gwf">
            <PricePlot json={prices}/>
        </div>
    {/if}
    -->
    {#if uiVisibility(sysinfo.ui.q, minutePlot)}
    <div class="cnt gwf">
        <MinutePlot  json={minutePlot} currentTier={data.trc}/> <!--DGF current tier added-->
    </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.h, hourPlot)}
    <div class="cnt gwf">
        <HourPlot json={hourPlot} currentTier={data.trc}/> <!--DGF current tier added-->
    </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.d, dayPlot)}
        <div class="cnt gwf">
            <DayPlot json={dayPlot} currentTier={data.trc}/> <!--DGF current tier added-->
        </div>
    {/if}
    <!--DGF week plot added-->
    {#if uiVisibility(sysinfo.ui.w, weekPlot)}
        <div class="cnt gwf">
            <WeekPlot json={weekPlot} currentTier={data.trc}/> <!--DGF current tier added-->
        </div>
    {/if}
    <!--DGF-->
    {#if uiVisibility(sysinfo.ui.m, monthPlot)}
        <div class="cnt gwf">
            <MonthPlot json={monthPlot} currentTier={data.trc}/> <!--DGF current tier added-->
        </div>
    {/if}
</div>