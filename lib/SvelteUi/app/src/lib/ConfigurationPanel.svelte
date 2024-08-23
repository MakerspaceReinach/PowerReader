<script>
    import { getConfiguration, configurationStore } from './ConfigurationStore'
    import { sysinfoStore } from './DataStores.js';
    import { wiki } from './Helpers.js';
    import Mask from './Mask.svelte'
    import Badge from './Badge.svelte';
    import HelpIcon from './HelpIcon.svelte';
    import CountrySelectOptions from './CountrySelectOptions.svelte';
    import { Link, navigate } from 'svelte-navigator';
    import SubnetOptions from './SubnetOptions.svelte';

    export let sysinfo = {}

    let uiElements = [{
        name: 'Bezug',
        key: 'i'
    },{
        name: 'Export',
        key: 'e'
    },{
        name: 'Sapannung',
        key: 'v'
    },{
        name: 'Strom',
        key: 'a'
    },{
        name: 'Blind/Scheinleistung',
        key: 'r'
    },
    {        
        name: 'Letzte Abrechnung',    // EHorvat: new last_counter_value.... also add "l" to config_ui.json
        key: 'l'
    },
    {
        name: 'Aktuelle Daten',
        key: 'c'
    },
    {
        name: 'Verbrauchsspitzen',
        key: 't'
    },
    // {
    //     name: 'Preis Vorschau',  //DGF removed
    //     key: 'p'
    // },
    {
        name: 'Minuten Plot',// EHorvat: new minute plot .... also add "q" to config_ui.json
        key: 'q'
    },
    {
        name: 'Stunden Plot',// EHorvat: new minute plot.... also add "h" to config_ui.json
        key: 'h'
    },
    {
        name: 'Tages Plot',
        key: 'd'
    },
    {
        name: 'Wochen Plot',  // DGF added weekly plot (for later use, not implemented, yet)
        key: 'w'
    },
    {
        name: 'Monats Plot',
        key: 'm'
    },
    {
        name: 'Frequenz',
        key: 'f'
    }];

    let loading = true;
    let saving = false;

    let configuration = {
        g: {
            t: '', h: '', s: 0, u: '', p: ''
        },
        m: {
            b: 2400, p: 11, i: false, d: 0, f: 0, r: 0,
            e: { e: false, k: '', a: '' },
            m: { e: false, w: false, v: false, a: false, c: false }
        },
        w: { s: '', p: '', w: 0.0, z: 255, a: true },
        n: {
            m: '', i: '', s: '', g: '', d1: '', d2: '', d: false, n1: '', n2: '', h: false
        },
        q: {
            h: '', p: 1883, u: '', a: '', b: '',
            s: { e: false, c: false, r: true, k: false }
        },
        o: {
            e: '',
            c: '',
            u1: '',
            u2: '',
            u3: ''
        },
        t: {
            t: [0,0,0,0,0,0,0,0,0,0], h: 1
        },
        p: {
            e: false, t: '', r: '', c: '', m: 1.0, n: ['','','','','','','',''], f: [null,null,null,null,null,null,null,null], i: null, y: 123456789, d:'', z: 123456789, g: '' //EHorvat added ", i: null" for fixed income refund and "y" "d" "z" "g" for last_countet, DGF tariffs added
        },
        d: {
            s: false, t: false, l: 5
        },
        u: {
            i: 0, e: 0, v: 0, a: 0, r: 0, l: 0, c: 0, t: 0, q: 0, h: 0, d: 0, w: 0, m: 0, f: 0 //EHorvat added , l: 0, for last_counter q: 0, h: 0, for minute plot and hour plot, f: for frequency, DGF added w (week plot) and removed p (price plot)
        },
        i: {
            h: { p: null, u: true },
            a: null,
            l: { p: null, i: false },
            r: { r: null, g: null, b: null, i: false },
            t: { d: null, a: null },
            v: { p: null, d: { v: null, g: null }, o: null, m: null, b: null }
        },
        h: {
            t: '', h: '', n: ''
        }
    };
    configurationStore.subscribe(update => {
        if(update.version) {
            configuration = update;
            loading = false;
        }
    });
    getConfiguration();

    let isFactoryReset = false;
    let isFactoryResetComplete = false;
    async function factoryReset() {
        if(confirm("Bist du sicher, dass du das Gerät auf die Werkeinstellungen zurücksetzen möchtest?")) {
            isFactoryReset = true;
            const data = new URLSearchParams();
            data.append("perform", "true");
            const response = await fetch('/reset', {
                method: 'POST',
                body: data
            });
            let res = (await response.json());
            isFactoryReset = false;
            isFactoryResetComplete = res.success;
        }
    }

    async function handleSubmit(e) {
        saving = true;
		const formData = new FormData(e.target);
		const data = new URLSearchParams();
		for (let field of formData) {
			const [key, value] = field
			data.append(key, value)
		}

        const response = await fetch('/save', {
            method: 'POST',
            body: data
        });
        let res = (await response.json())

        sysinfoStore.update(s => {
            s.booting = res.reboot;
            s.ui = configuration.u;
            return s;
        });

        saving = false;
        navigate("/");
	}

    async function reboot() {
      const response = await fetch('/reboot', {
            method: 'POST'
        });
        let res = (await response.json())
    }

    const askReboot = function() {
      if(confirm('Bist du sicher, dass du das Gerät neu starten möchtest?')) {
        sysinfoStore.update(s => {
            s.booting = true;
            return s;
        });
        reboot();
      }
    }

    const updateMqttPort = function() {
        if(configuration.q.s.e) {
            if(configuration.q.p == 1883) configuration.q.p = 8883;
        } else {
            if(configuration.q.p == 8883) configuration.q.p = 1883;
        }
    }

    let gpioMax = 44;
    $: {
        gpioMax = sysinfo.chip == 'esp8266' ? 16 : sysinfo.chip == 'esp32s2' ? 44 : 39;
    }
</script>

<form on:submit|preventDefault={handleSubmit} autocomplete="off">
    <div class="grid xl:grid-cols-4 lg:grid-cols-2 md:grid-cols-2">
        <div class="cnt">
            <strong class="text-sm">Allgemein</strong>
            <a href="{wiki('General-configuration')}" target="_blank" class="float-right"><HelpIcon/></a>
            <input type="hidden" name="g" value="true"/>
            <div class="my-1">
                <div class="flex">
                    <!--DGF moved to network section
                    <div>
                        Hostname<br/>
                        <input name="gh" bind:value={configuration.g.h} type="text" class="in-f w-full" pattern="[A-Za-z0-9-]+"/>
                    </div>
                -->
                    <div class="w-full">
                        Zeitzone<br/>
                        <select name="gt" bind:value={configuration.g.t} class="in-f w-full">
                            <CountrySelectOptions/>
                        </select>
                    </div>
                    <!--DGF removed until benefit is known
                    <div >
                    Preisregion<br/>
                    <select name="pr" bind:value={configuration.p.r} class="in-f w-full">
                        <optgroup label="Norwegen">
                            <option value="10YNO-1--------2">NO1</option>
                            <option value="10YNO-2--------T">NO2</option>
                            <option value="10YNO-3--------J">NO3</option>
                            <option value="10YNO-4--------9">NO4</option>
                            <option value="10Y1001A1001A48H">NO5</option>
                        </optgroup>
                        <optgroup label="Schweden">
                            <option value="10Y1001A1001A44P">SE1</option>
                            <option value="10Y1001A1001A45N">SE2</option>
                            <option value="10Y1001A1001A46L">SE3</option>
                            <option value="10Y1001A1001A47J">SE4</option>
                            </optgroup>
                        <optgroup label="Dänemark">
                            <option value="10YDK-1--------W">DK1</option>
                            <option value="10YDK-2--------M">DK2</option>
                        </optgroup>
                        <option value="10YAT-APG------L">Österreich</option>
                        <option value="10YBE----------2">Belgien</option>
                        <option value="10YCZ-CEPS-----N">Tschechien</option>
                        <option value="10Y1001A1001A39I">Estland</option>
                        <option value="10YFI-1--------U">Finnland</option>
                        <option value="10YFR-RTE------C">Frankreich</option>
                        <option value="10Y1001A1001A83F">Deutschland</option>
                        <option value="10YGB----------A">England</option>
                        <option value="10YLV-1001A00074">Lettland</option>
                        <option value="10YLT-1001A0008Q">Lithauen</option>
                        <option value="10YNL----------L">Niederlande</option>
                        <option value="10YPL-AREA-----S">Polen</option>
                        <option value="10YCH-SWISSGRIDZ">Schweiz</option>
                    </select>
                </div>
                -->
                <div>
                    Währung<br/>
                    <select name="pc" bind:value={configuration.p.c} class="in-l">
                        {#each ["NOK","SEK","DKK","EUR","CHF"] as c}
                        <option value={c}>{c}</option>
                        {/each}
                    </select>
                </div>
                </div>
            </div>
            <input type="hidden" name="p" value="true"/>
            <!-- DGF moved one line up
            <div class="my-1">
                <div class="flex">
                    
                <div class="w-full">
                    Preisregion<br/>
                    <select name="pr" bind:value={configuration.p.r} class="in-f w-full">
                        <optgroup label="Norwegen">
                            <option value="10YNO-1--------2">NO1</option>
                            <option value="10YNO-2--------T">NO2</option>
                            <option value="10YNO-3--------J">NO3</option>
                            <option value="10YNO-4--------9">NO4</option>
                            <option value="10Y1001A1001A48H">NO5</option>
                        </optgroup>
                        <optgroup label="Schweden">
                            <option value="10Y1001A1001A44P">SE1</option>
                            <option value="10Y1001A1001A45N">SE2</option>
                            <option value="10Y1001A1001A46L">SE3</option>
                            <option value="10Y1001A1001A47J">SE4</option>
                            </optgroup>
                        <optgroup label="Dänemark">
                            <option value="10YDK-1--------W">DK1</option>
                            <option value="10YDK-2--------M">DK2</option>
                        </optgroup>
                        <option value="10YAT-APG------L">Österreich</option>
                        <option value="10YBE----------2">Belgien</option>
                        <option value="10YCZ-CEPS-----N">Tschechien</option>
                        <option value="10Y1001A1001A39I">Estland</option>
                        <option value="10YFI-1--------U">Finnland</option>
                        <option value="10YFR-RTE------C">Frankreich</option>
                        <option value="10Y1001A1001A83F">Deutschland</option>
                        <option value="10YGB----------A">England</option>
                        <option value="10YLV-1001A00074">Lettland</option>
                        <option value="10YLT-1001A0008Q">Lithauen</option>
                        <option value="10YNL----------L">Niederlande</option>
                        <option value="10YPL-AREA-----S">Polen</option>
                        <option value="10YCH-SWISSGRIDZ">Schweiz</option>
                    </select>
                </div>
                <div>
                    Währung<br/>
                    <select name="pc" bind:value={configuration.p.c} class="in-l">
                        {#each ["NOK","SEK","DKK","EUR","CHF"] as c}
                        <option value={c}>{c}</option>
                        {/each}
                    </select>
                </div>
            </div>
        
        </div>
        -->
        <!--DGF Fixed price added by tariff-->
            <div class="my-1">
                <div class="flex">  
                    <span class="w-1/6">Tarif</span>
                    <span class="w-full">&nbsp;&nbsp;Name</span>
                    <span class="w-2/5">Kosten/kWh</span>
                </div>
            </div>
            {#each {length: 2} as _, i}
            <div class="my-1">
                <label class="flex">  
                        <span class="in-pre w-1/6">&nbsp;{i}</span>
                        <input name="pn{i}" bind:value={configuration.p.n[i]} type="text" maxlength="10" class="in-m w-full">
                        <input name="pf{i}" bind:value={configuration.p.f[i]} type="number" min="0.001" max="65" step="0.001" class="in-l tr w-2/5"/>
                    <!--DGF removed until benefit is known
                    <div class="w-1/2">
                        Faktor<br/>
                        <input name="pm" bind:value={configuration.p.m} type="number" min="0.001" max="1000" step="0.001" class="in-l tr w-full"/>
                    </div>
                    -->
                </label>
            </div>
            {/each}
            <!--DGF-->
            <!--DGF moved to separate line-->
             <div class="my-1">
                <div class="flex">
                    <div class="w-1/2">     <!-- EHorvat: Added new fixed income price / refund-->
                        Ertrag/kWh<br/>
                        <input name="pi" bind:value={configuration.p.i} type="number" min="0.001" max="65" step="0.001" class="in-s w-1/2"/>
                    </div>
                    <!--DGF removed until benefit is known
                    <div class="w-1/2">
                        Faktor<br/>
                        <input name="pm" bind:value={configuration.p.m} type="number" min="0.001" max="1000" step="0.001" class="in-l tr w-full"/>
                    </div>
                    -->
                </div>
            </div>
            <!--DGF-->
            <!-- DGF removed until benefit is known
            <div class="my-1">
                <label><input type="checkbox" name="pe" value="true" bind:checked={configuration.p.e} class="rounded mb-1"/> Enable price fetch from remote server</label>
                {#if configuration.p.e && sysinfo.chip != 'esp8266'}
                <br/><input name="pt" bind:value={configuration.p.t} type="text" class="in-s" placeholder="ENTSO-E API key, optional, read docs"/>
                {/if}
            </div>
            -->
            <div class="my-1">
                Sicherheit<br/>
                <select name="gs" bind:value={configuration.g.s} class="in-s">
                    <option value={0}>Keine</option>
                    <option value={1}>Nur Konfiguration</option>
                    <option value={2}>Alles</option>
                </select>
            </div>
            {#if configuration.g.s > 0}
            <div class="my-1">
                Benutzername<br/>
                <input name="gu" bind:value={configuration.g.u} type="text" class="in-s"/>
            </div>
            <div class="my-1">
                Passwort<br/>
                <input name="gp" bind:value={configuration.g.p} type="password" class="in-s"/>
            </div>
            {/if}
        </div>
        <div class="cnt">
            <strong class="text-sm">Zähler</strong>
            <a href="{wiki('Meter-configuration')}" target="_blank" class="float-right"><HelpIcon/></a>
            <input type="hidden" name="m" value="true"/>

            <div class="my-1">
                Spannung<br/>
                <select name="md" bind:value={configuration.m.d} class="in-s">
                    <option value={2}>400V (TN)</option>
                    <option value={1}>230V (IT/TT)</option>
                </select>
            </div>
            <div class="my-1 flex">
                <div class="mx-1">
                    Hauptsicherung<br/>
                    <label class="flex">
                        <input name="mf" bind:value={configuration.m.f} type="number" min="5" max="65535" class="in-f tr w-full"/>
                        <span class="in-post">A</span>
                    </label>
                </div>
                <div class="mx-1">
                    Produktion<br/>
                    <label class="flex">
                        <input name="mr" bind:value={configuration.m.r} type="number" min="0" max="65535" class="in-f tr w-full"/>
                        <span class="in-post">kWp</span>
                    </label>
                </div>
            </div>
            <div class="my-1">
            </div>
            
            <div class="my-1">
                <label><input type="checkbox" name="me" value="true" bind:checked={configuration.m.e.e} class="rounded mb-1"/> Zähler ist verschlüsselt</label>
                {#if configuration.m.e.e}
                <br/><input name="mek" bind:value={configuration.m.e.k} type="text" class="in-s"/>
                {/if}
            </div>

            <label><input type="checkbox" name="mm" value="true" bind:checked={configuration.m.m.e} class="rounded mb-1"/> Faktoren</label>
            {#if configuration.m.m.e}
            <div class="flex my-1">
                <div class="w-1/4">
                    Watt<br/>
                    <input name="mmw" bind:value={configuration.m.m.w} type="number" min="0.00" max="1000" step="0.001" class="in-f tr w-full"/>
                </div>
                <div class="w-1/4">
                    Volt<br/>
                    <input name="mmv" bind:value={configuration.m.m.v} type="number" min="0.00" max="1000" step="0.001" class="in-m tr w-full"/>
                </div>
                <div class="w-1/4">
                    Amp<br/>
                    <input name="mma" bind:value={configuration.m.m.a} type="number" min="0.00" max="1000" step="0.001" class="in-m tr w-full"/>
                </div>
                <div class="w-1/4">
                    kWh<br/>
                    <input name="mmc" bind:value={configuration.m.m.c} type="number" min="0.00" max="1000" step="0.001" class="in-l tr w-full"/>
                </div>
            </div>
            {/if}
        </div>
        <div class="cnt">
            <strong class="text-sm">WiFi</strong>
            <a href="{wiki('WiFi-configuration')}" target="_blank" class="float-right"><HelpIcon/></a>
            <input type="hidden" name="w" value="true"/>
            <div class="my-1">
                SSID<br/>
                <input name="ws" bind:value={configuration.w.s} type="text" maxlength="32" class="in-s"/> <!--DGF maxlength added-->
            </div>
            <div class="my-1">
                Passwort<br/>
                <input name="wp" bind:value={configuration.w.p} type="password" class="in-s"/>
            </div>
            <div class="my-1 flex">
                <div class="w-1/2">
                    Strom sparen<br/>
                    <select name="wz" bind:value={configuration.w.z} class="in-s">
                        <option value={255}>Standard</option>
                        <option value={0}>Aus</option>
                        <option value={1}>Minimum</option>
                        <option value={2}>Maximum</option>
                    </select>
                </div>
                <div class="ml-2 w-1/2">
                    Sendeleistung<br/>
                    <div class="flex">
                        <input name="ww" bind:value={configuration.w.w} type="number" min="0" max="20.5" step="0.5" class="in-f tr w-full"/>
                        <span class="in-post">dBm</span>
                    </div>
                </div>
            </div>
            <div class="my-3">
                <label><input type="checkbox" name="wa" value="true" bind:checked={configuration.w.a} class="rounded mb-1"/> Start a Wifi AP on home Wifi connection problem. Note: The AP is secured. But one can find the password on my github readme file. So if the AP can be publicly accessed you should think about setting some security using General sections Security settings</label>
            </div>
        </div>
        <div class="cnt">
            <strong class="text-sm">Netzwerk</strong>
            <a href="{wiki('Network-configuration')}" target="_blank" class="float-right"><HelpIcon/></a>
            <div class="my-1">
                IP<br/>
                <div class="flex">
                    <select name="nm" bind:value={configuration.n.m} class="in-f">
                        <option value="dhcp">DHCP</option>
                        <option value="static">Statisch</option>
                    </select>
                    <input name="ni" bind:value={configuration.n.i} type="text" class="in-m w-full" disabled={configuration.n.m == 'dhcp'} required={configuration.n.m == 'static'}/>
                    <select name="ns" bind:value={configuration.n.s} class="in-l" disabled={configuration.n.m == 'dhcp'} required={configuration.n.m == 'static'}>
                        <SubnetOptions/>
                    </select>
                </div>
            </div>
            {#if configuration.n.m == 'static'}
            <div class="my-1">
                Gateway<br/>
                <input name="ng" bind:value={configuration.n.g} type="text" class="in-s"/>
            </div>
            <div class="my-1">
                DNS Server<br/>
                <div class="flex">
                    <input name="nd1" bind:value={configuration.n.d1} type="text" class="in-f w-full"/>
                    <input name="nd2" bind:value={configuration.n.d2} type="text" class="in-l w-full"/>
                </div>
            </div>
            {/if}
            <div class="my-1">
                <label><input name="nd" value="true" bind:checked={configuration.n.d} type="checkbox" class="rounded mb-1"/> mDNS aktivieren</label>
            </div>
            <!--DGF moved from general section-->
             <div>
                Hostname<br/>
                <input name="gh" bind:value={configuration.g.h} type="text" class="in-f w-full" pattern="[A-Za-z0-9-]+"/>
            </div>
            <!--DGF-->
            <input type="hidden" name="ntp" value="true"/>
            <div class="my-1">
                NTP <label class="ml-4"><input name="ntpd" value="true" bind:checked={configuration.n.h} type="checkbox" class="rounded mb-1"/> von DHCP erhalten</label><br/>
                <div class="flex">
                    <input name="ntph" bind:value={configuration.n.n1} type="text" class="in-s"/>
                </div>
            </div>
        </div>
        <div class="cnt">
            <strong class="text-sm">MQTT</strong>
            <a href="{wiki('MQTT-configuration')}" target="_blank" class="float-right"><HelpIcon/></a>
            <input type="hidden" name="q" value="true"/>
            <div class="my-1">
                Server
                {#if sysinfo.chip != 'esp8266'}
                <label class="float-right mr-3"><input type="checkbox" name="qs" value="true" bind:checked={configuration.q.s.e} class="rounded mb-1" on:change={updateMqttPort}/> SSL</label>
                {/if}
                <br/>
                <div class="flex">
                    <input name="qh" bind:value={configuration.q.h} type="text" class="in-f w-3/4"/>
                    <input name="qp" bind:value={configuration.q.p} type="number" min="1024" max="65535" class="in-l tr w-1/4"/>
                </div>
            </div>
            {#if configuration.q.s.e}
            <div class="my-1">
                <div>
                    <Link to="/mqtt-ca">
                        {#if configuration.q.s.c}
                        <Badge color="green" text="CA OK" title="Click here to replace CA"/>
                        {:else}
                        <Badge color="blue" text="Upload CA" title="Click here to upload CA"/>
                        {/if}
                    </Link>

                    <Link to="/mqtt-cert">
                        {#if configuration.q.s.r}
                        <Badge color="green" text="Cert OK" title="Click here to replace certificate"/>
                        {:else}
                        <Badge color="blue" text="Upload cert" title="Click here to upload certificate"/>
                        {/if}
                    </Link>

                    <Link to="/mqtt-key">
                        {#if configuration.q.s.k}
                        <Badge color="green" text="Key OK" title="Click here to replace key"/>
                        {:else}
                        <Badge color="blue" text="Upload key" title="Click here to upload key"/>
                        {/if}
                    </Link>
                </div>
            </div>
            {/if}
            <div class="my-1">
                Benutzername<br/>
                <input name="qu" bind:value={configuration.q.u} type="text" class="in-s"/>
            </div>
            <div class="my-1">
                Passwort<br/>
                <input name="qa" bind:value={configuration.q.a} type="password" class="in-s"/>
            </div>
            <div class="my-1 flex">
                <div>
                    Client ID<br/>
                    <input name="qc" bind:value={configuration.q.c} type="text" class="in-f w-full"/>
                </div>
                <div>
                    Payload<br/>
                    <select name="qm" bind:value={configuration.q.m} class="in-l">
                        <option value={1}>Raw (10 Sek.)</option>
                        <option value={2}>Raw (2 Sek.)</option>
                        <option value={4}>HomeAssistant</option>
                    </select>
                </div>
            </div>
            <div class="my-1">
                Topic<br/>
                <input name="qb" bind:value={configuration.q.b} type="text" class="in-s"/>
            </div>
        </div>
        {#if configuration.q.m == 3}
        <div class="cnt">
            <strong class="text-sm">Domoticz</strong>
            <a href="{wiki('MQTT-configuration#domoticz')}" target="_blank" class="float-right"><HelpIcon/></a>
            <input type="hidden" name="o" value="true"/>
            <div class="my-1 flex">
                <div class="w-1/2">
                    Electricity IDX<br/>
                    <input name="oe" bind:value={configuration.o.e} type="text" class="in-f tr w-full"/>
                </div>
                <div class="w-1/2">
                    Current IDX<br/>
                    <input name="oc" bind:value={configuration.o.c} type="text" class="in-l tr w-full"/>
                </div>
            </div>
            <div class="my-1">
                Voltage IDX: L1, L2 & L3
                <div class="flex">
                    <input name="ou1" bind:value={configuration.o.u1} type="text" class="in-f tr w-1/3"/>
                    <input name="ou2" bind:value={configuration.o.u2} type="text" class="in-m tr w-1/3"/>
                    <input name="ou3" bind:value={configuration.o.u3} type="text" class="in-l tr w-1/3"/>
                </div>
            </div>
        </div>
        {/if}
        {#if configuration.q.m == 4}
        <div class="cnt">
            <strong class="text-sm">Home-Assistant</strong>
            <a href="{wiki('MQTT-configuration#home-assistant')}" target="_blank" class="float-right"><HelpIcon/></a>
            <input type="hidden" name="h" value="true"/>
            <div class="my-1">
                Discovery topic prefix<br/>
                <input name="ht" bind:value={configuration.h.t} type="text" class="in-s" placeholder="homeassistant"/>
            </div>
            <div class="my-1">
                Hostname for URL<br/>
                <input name="hh" bind:value={configuration.h.h} type="text" class="in-s" placeholder="{configuration.g.h}.local"/>
            </div>
            <div class="my-1">
                Name tag<br/>
                <input name="hn" bind:value={configuration.h.n} type="text" class="in-s"/>
            </div>
        </div>
        {/if}
        {#if configuration.p.r.startsWith("10YNO") || configuration.p.r == '10Y1001A1001A48H'}
        <div class="cnt">
            <strong class="text-sm">Tariff thresholds</strong>
            <a href="{wiki('Threshold-configuration')}" target="_blank" class="float-right"><HelpIcon/></a>
            <input type="hidden" name="t" value="true"/>
            <div class="flex flex-wrap my-1">
                {#each {length: 9} as _, i}
                <label class="flex w-40 m-1">
                    <span class="in-pre">{i+1}</span>
                    <input name="t{i}" bind:value={configuration.t.t[i]} type="number" min="0" max="65535" class="in-txt w-full"/>
                    <span class="in-post">kWh</span>
                </label>
                {/each}
            </div>
            <label class="flex m-1">
                <span class="in-pre">Average of</span>
                <input name="th" bind:value={configuration.t.h} type="number" min="0" max="255" class="in-txt tr w-full"/>
                <span class="in-post">hours</span>
            </label>
        </div>
        {/if}
        <div class="cnt">
            <strong class="text-sm">Anzeigen der Hautpseite</strong>
            <a href="{wiki('User-interface')}" target="_blank" class="float-right"><HelpIcon/></a>
            <input type="hidden" name="u" value="true"/>
            <div class="flex flex-wrap">
                {#each uiElements as el}
                    <div class="w-1/2">
                        {el.name}<br/>
                        <select name="u{el.key}" bind:value={configuration.u[el.key]} class="in-s">
                            <option value={0}>Verstecken</option>
                            <option value={1}>Anzeigen</option>
                            <option value={2}>Dynamisch</option>
                        </select>
                    </div>
                {/each}
            </div>
        </div>
        <div class="cnt"> <!-- EHorvat: Added new last_counter_value -->
            <strong class="text-sm">Letzte Abrechnung</strong>
            <div class="my-1"> 
                Z&auml;hlerstand [kWh] Import<br/>
                <input name="py" bind:value={configuration.p.y} type="number" min="0" max="2147483646" class="in-s"/>
            </div>
            <div class="my-1">
                Info zu Z&auml;hlerstand Imp. (max. 15 Zeichen)<br/>
                <input name="pd" bind:value={configuration.p.d} type="text" class="in-s" maxlength="15"> <!-- DGF: maxlength added -->
            </div>
            <div class="my-1">
                Z&auml;hlerstand [kWh] Export<br/>
                <input name="pz" bind:value={configuration.p.z} type="number" min="0" max="2147483646" class="in-s"/>
            </div>
            <div class="my-1">
                Info zu Z&auml;hlerstand Exp. (max. 15 Zeichen)<br/>
                <input name="pg" bind:value={configuration.p.g} type="text" class="in-s" maxlength="15"> <!-- DGF: maxlength added -->
            </div>         
        </div>
        <div class="cnt">
            <strong class="text-sm">Debugging</strong>
            <a href="https://amsleser.no/blog/post/24-telnet-debug" target="_blank" class="float-right"><HelpIcon/></a>
            <input type="hidden" name="d" value="true"/>
            <div class="mt-3">
                <label><input type="checkbox" name="ds" value="true" bind:checked={configuration.d.s} class="rounded mb-1"/> Debugging aktivieren</label>
            </div>
            {#if configuration.d.s}
            <div class="bd-red">Debug kann sporatische Neustarts veursachen. Bitte nicht eingeschaltet lassen!</div>
            <div class="my-1">
                <label><input type="checkbox" name="dt" value="true" bind:checked={configuration.d.t} class="rounded mb-1"/> Enable telnet</label>
            </div>
            {#if configuration.d.t}
            <div class="bd-red">Telnet is unsafe and should be off when not in use</div>
            {/if}
            <div class="my-1">
                <select name="dl" bind:value={configuration.d.l} class="in-s">
                    <option value={1}>Ausführlich</option>
                    <option value={2}>Debug</option>
                    <option value={3}>Info</option>
                    <option value={4}>Warnung</option>
                </select>
            </div>
            {/if}
        </div>
    </div>
    <div class="grid grid-cols-3">
        <div class="text-center">
            <button type="button" on:click={askReboot} class="py-2 px-4 rounded bg-yellow-500 text-white">Neustart</button>
        </div>
        <div class="text-right">
            <button type="submit" class="btn-pri">Speichern</button>
        </div>
    </div>
</form>

<Mask active={loading} message="Konfiguration wird geladen..."/>
<Mask active={saving} message="Konfiguration wird gespeichert..."/>
<Mask active={isFactoryReset} message="Gerät wird auf Werkeinstellungen zurückgesetzt..."/>
<Mask active={isFactoryResetComplete} message="Das Gerät wurde auf die Werkeinstellungen zurückgesetzt und in den Access Point Modus versetzt."/>
