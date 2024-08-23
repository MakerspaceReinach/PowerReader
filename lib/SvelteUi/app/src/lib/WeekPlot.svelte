<script>
    import { zeropad } from './Helpers.js';
    import BarChart from './BarChart.svelte';

    export let json;
    export let currentTier; 
    let config = {};
    let max = 0;
    let min = 0;

    $: {
        let i = 0;
        let yTicks = [];
        let xTicks = [];
        let points = [];
        let cur = new Date();
        let lm = new Date();
        cur.setDate(lm.getDate() - 7);
        let currentWeekday = new Date().getDay();
        let weekDay;
        for(i = currentWeekday; i<=6; i++) 
        {
            let imp = json["i"+zeropad(i+1)];
            let exp = json["e"+zeropad(i+1)];
            if(imp === undefined) imp = 0;
            if(exp === undefined) exp = 0;
            switch (i)
            {
                case 0:
                    weekDay = "So";
                    break;
                case 1:
                    weekDay = "Mo";
                    break;
                case 2:
                    weekDay = "Di";
                    break;
                case 3:
                    weekDay = "Mi";
                    break;
                case 4:
                    weekDay = "Do";
                    break;
                case 5:
                    weekDay = "Fr";
                    break;           
                case 6:
                    weekDay = "Sa";
                    break;                
            }
            xTicks.push(
            {
                label: weekDay
            });
            points.push({
                label: imp.toFixed(imp < 10 ? 1 : 0), 
                title: imp.toFixed(2) + ' kWh',
                value: imp, 
                label2: exp.toFixed(exp < 10 ? 1 : 0), 
                title2: exp.toFixed(2) + ' kWh',
                value2: exp,
                color2: '#407038',
                color: '#7c3aed',
                colorTier: '#9770ff'  
            });
            min = Math.max(min, exp);
            max = Math.max(max, imp);
        }
        for(i = 0; i < currentWeekday; i++) 
        {
            let imp = json["i"+zeropad(i+1)];
            let exp = json["e"+zeropad(i+1)];
            if(imp === undefined) imp = 0;
            if(exp === undefined) exp = 0;
            switch (i)
            {
                case 0:
                    weekDay = "So";
                    break;
                case 1:
                    weekDay = "Mo";
                    break;
                case 2:
                    weekDay = "Di";
                    break;
                case 3:
                    weekDay = "Mi";
                    break;
                case 4:
                    weekDay = "Do";
                    break;
                case 5:
                    weekDay = "Fr";
                    break;           
                case 6:
                    weekDay = "Sa";
                    break;                
            }
            xTicks.push(
            {
                label: weekDay
            });
            points.push({
                label: imp.toFixed(imp < 10 ? 1 : 0), 
                title: imp.toFixed(2) + ' kWh', 
                value: imp, 
                label2: exp.toFixed(exp < 10 ? 1 : 0), 
                title2: exp.toFixed(2) + ' kWh', 
                value2: exp,
                color2: '#407038',
                color: '#7c3aed', 
                colorTier: '#9770ff'  
            });
            min = Math.max(min, exp);
            max = Math.max(max, imp);
        }

        let boundary = Math.ceil(Math.max(min, max)/10)*10;

        max = boundary;
        min = min == 0 ? 0 : boundary*-1;

        if(min < 0) 
        {
            let yTickDistDown = min/4;
            for(i = 0; i < 5; i++) {
                let val = (yTickDistDown*i);
                yTicks.push(
                {
                    value: val,
                    label: val.toFixed(0)
                });
            }
        }

        let yTickDistUp = max/4;
        for(i = 0; i < 5; i++) {
            let val = (yTickDistUp*i);
            yTicks.push({
                value: val,
                label: val.toFixed(0)
            });
        }

        config = {
            title: "Elektrische Energie letzte 7 Tage (kWh)",
            height: 226,
            width: 1520,
            padding: { top: 20, right: 15, bottom: 20, left: 35 },
            y: {
                min: min,
                max: max,
                ticks: yTicks
            },
            x: {
                ticks: xTicks
            },
            points: points
        };
    };

</script>
<BarChart config={config} currentTier={currentTier} /> <!--DGF current ier added -->
