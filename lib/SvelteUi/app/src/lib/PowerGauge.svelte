<script>
    import PowerGaugeSvg from './PowerGaugeSvg.svelte';

    export let val;
    export let max;
    export let unit;
    export let label;
    export let sub = "";
    export let subunit = "";
    export let color; // DGF ColorFn replaced with color
    //export let colorFn;
    let pct = 0;
    $: {
        pct = (Math.min(val,max)/max) * 100
    }
</script>

<div class="pl-root">
    <!--<PowerGaugeSvg pct={pct} color={color ? color : colorFn(pct)}/>  DGF ColorFn replaced with color-->
    <PowerGaugeSvg pct={pct} color={color}/>  <!--DGF ColorFn replaced with color-->
    <span class="pl-ov">
        <span class="pl-lab">{label}</span>
        <br/>
        <span class="pl-val">{val}</span> 
        <span class="pl-unt">{unit}</span>
        {#if sub}
        <br/>
        <span class="pl-sub">{sub}</span>
        <span class="pl-snt">{subunit}/kWh</span>
        {/if}
    </span>
</div>
