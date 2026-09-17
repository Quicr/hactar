# ES8311 Register Map

Transcribed from `ES8311 DS.pdf`, revision 8.0 (May 2020). Fields not listed in a table are reserved or undocumented by the datasheet.

## 0x00 Reset (default `0x1F`)

- `CSM_ON` [7]: 0 = CSM power down (default); 1 = CSM power on.
- `MSC` [6]: 0 = serial port slave (default); 1 = serial port master.
- `SEQ_DIS` [5]: 0 = power-up sequence enabled (default); 1 = disabled.
- `RST_DIG` [4]: 0 = digital logic not reset; 1 = reset digital logic except control port (default).
- `RST_CMG` [3]: 0 = clock manager not reset; 1 = reset clock manager (default).
- `RST_MST` [2]: 0 = master block not reset; 1 = reset master block (default).
- `RST_ADC_DIG` [1]: 0 = ADC digital block not reset; 1 = reset ADC digital block (default).
- `RST_DAC_DIG` [0]: 0 = DAC digital block not reset; 1 = reset DAC digital block (default).

Remarks:
Chip must be reset at the start of each program. 0b0011'1111
given we are using ES8311 as master, then we should ensure that we select slave mode on bit 6

Hold in reset: 0b0011'1111
Running: 0b1100'0000

## 0x01 Clock Manager (default `0x00`)

- `MCLK_SEL` [7]: 0 = use MCLK pin (default); 1 = use BCLK pin.
- `MCLK_INV` [6]: 0 = normal MCLK (default); 1 = invert MCLK.
- `MCLK_ON` [5]: 0 = MCLK off (default); 1 = MCLK on.
- `BCLK_ON` [4]: 0 = BCLK off (default); 1 = BCLK on.
- `CLKADC_ON` [3]: 0 = ADC digital clock off (default); 1 = on.
- `CLKDAC_ON` [2]: 0 = DAC digital clock off (default); 1 = on.
- `ANACLKADC_ON` [1]: 0 = ADC analog clock off; 1 = on (default).
- `ANACLKDAC_ON` [0]: 0 = DAC analog clock off; 1 = on (default).

Remarks:
Using MCLK. Bit 5, MLK in control, enables/disables the mclk input pin. Since we are using our ES8311 in master, then we need sdp, adc, dac, analog adc and analog dac on

Set to: 0b0011'1111

## 0x02 Clock Manager (default `0x00`)

- `DIV_PRE` [7:5]: MCLK pre-divider; `mclk_prediv = mclk / (DIV_PRE + 1)`.
- `MULT_PRE` [4:3]: pre-multiplier; 0 = x1 (default), 1 = x2, 2 = x4, 3 = x8.
- `PATHSEL` [2]: 0 = no DFF path (default); 1 = DFF path.
- `DELYSEL` [1:0]: clock-doubler delay; 0 = 5 ns (default), 1 = 10 ns, 2 = 15 ns, 3 = 15 ns.

Remarks:
Digital clock aim is to be some defaulted value provided by the table on page 16 in the User Guide 
At MCLK 12Mhz 0x00 DIV1, MULT1 - 0b0000'0000, which doesn't make sense, because that gives an internal clock of 12Mhz, which
according to the User Guide, on page 14, that the clock divider needs to output <10Mhz. Their table is whack.
Roughly, I think given we are running at 3.0V, then we should be no higher than 8.9Mhz, by some crazy math, on the divider.

Therefore, to get the ratio integral ratio of 16 mentioned on pg 15 of the user guide, we can divide by 5, and multiply by 8
to get dig_mclk = 19.2Mhz.

This should be acceptable for 8Khz too.

For 19.2Mhz set register to 0x98

## 0x03 Clock Manager (default `0x10`)

- `ADC_FSMODE` [6]: 0 = single speed (default); 1 = double speed.
- `ADC_OSR` [5:0]: ADC delta-sigma oversampling rate. 0-14 unused; 15 = 60 x Fs; 16 = 64 x Fs single-speed or 32 x Fs double-speed (default); 31 = 124 x Fs or 62 x Fs; 32 = 128 x Fs or 64 x Fs; 63 = 252 x Fs or 126 x Fs.

Remarks:
Defaulting to table entry 12mhz 48khz 0x19 - 0b0001'1001
Not sure what the function of 

## 0x04 Clock Manager (default `0x10`)

- `DAC_OSR` [6:0]: DAC oversampling rate. 0-15 unused; 16 = 64 x Fs (default); 17 = 68 x Fs; 32 = 128 x Fs; 64 = 256 x Fs; 127 = 508 x Fs.

Remarks:
Defaulting to table entry 12mhz 48khz 0x19 - 0b0001'1001

## 0x05 Clock Manager (default `0x00`)

- `DIV_CLKADC` [7:4]: ADC MCLK divider; `adc_mclk = dig_mclk / (DIV_CLKADC + 1)`.
- `DIV_CLKDAC` [3:0]: DAC MCLK divider; `dac_mclk = dig_mclk / (DIV_CLKDAC + 1)`.

Remarks:
The ratio for 48khz with a dig_mclk = 19.2mhz is 400 % 16 = 0
The ratio for 8Khz with a dig_mclk = 19.2mhz = 2400 % 16 = 0
This can stay untouched at 0x00

## 0x06 Clock Manager (default `0x03`)

- `BCLK_CON` [6]: master BCLK output control; 0 = continuous (default); 1 = stop after transfer.
- `BCLK_INV` [5]: 0 = normal BCLK (default); 1 = inverted BCLK.
- `DIV_BCLK` [4:0]: master BCLK divider. 0-19 = MCLK / (`DIV_BCLK` + 1), default 3; 20 = /22; 21 = /24; 22 = /25; 23 = /30; 24 = /32; 25 = /33; 26 = /34; 27 = /36; 28 = /44; 29 = /48; 30 = /66; 31 = /72.

Remarks:
Since we are using a non-standard clock, we need to stop the BCLK early, so bit 6 should be a 1
Note, we are aiming to have a BCLK = LRCLK * 32 bits * 2 channels, given we are early stopping the bclk then 
we can need to have a bclk > than our minimum bclk in a perfect division.

Ex. 
48KHz * 32 * 2 = 3.072MHz
12Mhz / 3.072Mhz = 3.90625

So we need to divide by 3 so we still have a faster bclk

Therefore, DIV_BCLK = 2
Giving us 

BCLK = MCLK / DIV_BCLK+1 
BCLK = 12MHz / 2+1 = 4MHz

## 0x07 Clock Manager (default `0x00`)

- `TRI_BLRCK` [5]: 0 = normal BCLK/LRCK output (default); 1 = tri-state output.
- `TRI_ADCDAT` [4]: 0 = normal ADCDAT output (default); 1 = tri-state output.
- `DIV_LRCK[11:8]` [3:0]: upper four bits of the master LRCK divider; `LRCK = MCLK / (LRCK_DIV + 1)`.

Remarks:
48KHz DIV_LRCK=0
32KHz DIV_LRCK=1
8KHz DIV_LRCK=5

## 0x08 Clock Manager (default `0xFF`)

- `DIV_LRCK[7:0]` [7:0]: lower eight bits of the master LRCK divider; `LRCK = MCLK / (LRCK_DIV + 1)`. A divider of zero holds LRCK high.

Remarks:
48Khz DIV_LCK=249
32Khz DIV_LCK=118
8Khz DIV_LCK=219

## 0x09 SDP Input (default `0x00`)

- `SDP_IN_SEL` [7]: 0 = left input data to DAC (default); 1 = right input data to DAC.
- `SDP_IN_MUTE` [6]: 0 = unmute (default); 1 = mute.
- `SDP_IN_LRP` [5]: I2S/justified: 0 = normal L/R polarity (default), 1 = inverted. DSP/PCM: 0 = MSB on second BCLK rising edge after LRCK (default), 1 = first edge.
- `SDP_IN_WL` [4:2]: 0 = 24 bit (default); 1 = 20 bit; 2 = 18 bit; 3 = 16 bit; 4 = 32 bit.
- `SDP_IN_FMT` [1:0]: 0 = I2S (default); 1 = left justified; 2 = reserved; 3 = DSP/PCM.

Remarks: 
Soft mute exists in this register. 
We are doing 16 bit on 32 bit frames, could change if needed
I2S serial audio data format is what we normally use

In the STM32 settings it is set to I2S Phillips, and so should the ES8311,
but there is an error in the datasheet.

Correction!
bit[1:0] - 0 = Left justified, 1 = I2S Phillips, 2 = reserved 3 = DSP/PCM

Set to 0b0|0|0|1'00|00 (0x10)

## 0x0A SDP Output (default `0x00`)

- `SDP_OUT_MUTE` [6]: 0 = unmute (default); 1 = mute.
- `SDP_OUT_LRP` [5]: I2S/justified: 0 = normal L/R polarity (default), 1 = inverted. DSP/PCM: 0 = MSB on second BCLK rising edge after LRCK (default), 1 = first edge.
- `SDP_OUT_WL` [4:2]: 0 = 24 bit (default); 1 = 20 bit; 2 = 18 bit; 3 = 16 bit; 4 = 32 bit.
- `SDP_OUT_FMT` [1:0]: 0 = I2S (default); 1 = left justified; 2 = reserved; 3 = DSP/PCM.

Remarks:
Same as above

Set to 0b0|0|0|1'00|00 (0x10)

## 0x0B System (default `0x00`)

- `PWRUP_A` [7:3]: power-up stage A delay. At 8 kHz, 0-31 corresponds to 120 us-1392 ms.
- `PWRUP_B[3:1]` [2:0]: upper three bits of power-up stage B delay. At 8 kHz, 0-31 corresponds to 120 us-624 ms.

Remarks:
Leave at zero

## 0x0C System (default `0x20`)

- `PWRUP_B[0]` [7]: low bit of power-up stage B delay.
- `PWRUP_C` [6:0]: power-up stage C delay. At 8 kHz, 0-31 corresponds to 120 us-1401 ms.

Remarks:
Leave at zero

## 0x0D System (default `0xFC`)

- `PDN_ANA` [7]: 0 = enable analog circuits; 1 = power down (default).
- `PDN_IBIASGEN` [6]: 0 = enable analog bias; 1 = power down (default).
- `PDN_ADCBIASGEN` [5]: 0 = enable ADC bias; 1 = power down (default).
- `PDN_ADCVERFGEN` [4]: 0 = enable ADC reference; 1 = power down (default).
- `PDN_DACVREFGEN` [3]: 0 = enable DAC reference; 1 = power down (default).
- `PDN_VREF` [2]: 0 = disable internal reference; 1 = enable (default).
- `VMIDSEL` [1:0]: 0 = VMID power down (default); 1 = normal-speed startup; 2 = normal operation; 3 = fast startup.

Remarks:
0xFE

## 0x0E System (default `0x6A`)

- `PDN_PGA` [6]: 0 = enable analog PGA; 1 = power down (default).
- `PDN_MOD` [5]: 0 = enable analog ADC modulator; 1 = power down (default).
- `RST_MOD` [4]: 0 = normal (default); 1 = reset modulator.
- `VROI` [3]: 0 = normal impedance; 1 = low impedance (default).
- `LPVREFBUF` [2]: 0 = normal internal reference mode (default); 1 = low-power mode.

Remarks:
0b000'1010
0x0A

## 0x0F System (default `0x00`)

- `LPDAC` [7]: 0 = normal (default); 1 = DAC low-power mode.
- `LPPGA` [6]: 0 = normal (default); 1 = PGA low-power mode.
- `LPPGAOUT` [5]: 0 = normal (default); 1 = PGA output low-power mode.
- `LPVCMMOD` [4]: 0 = normal (default); 1 = ADC low-power mode.
- `LPADCVRP` [3]: 0 = normal (default); 1 = ADC reference low-power mode.
- `LPDACVRP` [2]: 0 = normal (default); 1 = DAC reference low-power mode.
- `LPFLASH` [1]: 0 = normal (default); 1 = ADC low-power mode.
- `LPINT1` [0]: 0 = normal (default); 1 = ADC low-power mode.

Remarks:
Only need to worry about this when we want to do low power

## 0x10 System (default `0x13`)

- `SYNCMODE` [7]: 0 = normal (default); 1 = sync mode.
- `VMIDLOW` [6:5]: 0 = VDDA/2 (default); 1 = VDDA/2 - 75 mV; 2 = VDDA/2 - 145 mV; 3 = VDDA/2 - 175 mV.
- `DAC_IBIAS_SW` [4]: 0 = normal DAC bias; 1 = higher DAC bias (default).
- `IBIAS_SW` [3:2]: 0 = bias level 0 (default); 1 = level 1; 2 = level 2; 3 = level 3.
- `VX2OFF` [1]: 0 = enable internal reference doubler; 1 = off (default).
- `VX1SEL` [0]: 0 = 1.45 V; 1 = 1.65 V (default).

- Remarks, not sure what sync mode means, but we can leave this for now

## 0x11 System (default `0x7C`)

- `VSEL` [6:0]: internal use.

## 0x12 System (default `0x02`)

- `PDN_DAC` [1]: 0 = enable DAC; 1 = power down (default).
- `ENREFR` [0]: 0 = disable DAC output reference (default); 1 = enable it.

Remarks:
0x01

## 0x13 System (default `0x40`)

- `HPSW` [4]: 0 = line-output drive (default); 1 = headphone drive.

Remarks:
We want headphones not line output
0x50

## 0x14 System (default `0x10`)

- `DMIC_ON` [6]: 0 = no DMIC; 1 = select DMIC and DMIC_SDA from MIC1P.
- `LINSEL` [4]: 0 = no input selection; 1 = select MIC1P-MIC1N.
- `PGAGAIN` [3:0]: ADC PGA gain: 0 = 0 dB through 10 = 30 dB in 3 dB steps.

Remarks:
We are using an analog mic 
0x10

## 0x15 ADC (default `0x00`)

- `ADC_RAMPRATE` [7:4]: ADC volume-control ramp: 0 = disabled; 1-15 = 0.25 dB per 4-65536 LRCKs.
- `DMIC_SENSE` [0]: 0 = latch DMIC on positive edge; 1 = negative edge.

Remarks:
We are not using DMIC 
Keep the ramp rate at default
0x00

## 0x16 ADC (default `0x04`)

- `ADC_SYNC` [5]: 0 = non-standard audio clock; 1 = standard audio clock.
- `ADC_INV` [4]: 0 = normal; 1 = inverted.
- `ADC_RAMCLR` [3]: clear ADC RAM when LRCK/ADC MCLK is active.
- `ADC_SCALE` [2:0]: ADC gain scale: 0 = 0 dB; 1 = 6 dB; 2 = 12 dB; 3 = 18 dB; 4 = 24 dB (default); 5 = 30 dB; 6 = 36 dB; 7 = 42 dB.

Remarks: 48khz and 8khz are standard audio clocks so we should keep ADC_SYNC=0

## 0x17 ADC Volume (default `0x00`)

- `ADC_VOLUME` [7:0]: 0x00 = -95.5 dB; 0x01 = -90.5 dB; then 0.5 dB/step; 0xBE = -0.5 dB; 0xBF = 0 dB; 0xC0 = +0.5 dB; 0xFF = +32 dB. With ALC enabled, this is `MAXGAIN`.

Remarks: Leave as default

## 0x18 ADC ALC (default `0x00`)

- `ALC_EN` [7]: 0 = disable (default); 1 = enable ADC automatic level control.
- `ADC_AUTOMUTE_EN` [6]: 0 = disable (default); 1 = enable ADC automute.
- `ALC_WINSIZE` [3:0]: ALC window: 0 = 0.25 dB/2 LRCK; 1-15 increase through 0.25 dB/65536 LRCK.

Remarks: Leave as default, we don't want the ALC yet

## 0x19 ADC ALC Level (default `0x00`)

- `ALC_MAXLEVEL` [7:4]: ALC maximum target: 0 = -30.1 dB through 15 = -6.0 dB.
- `ALC_MINLEVEL` [3:0]: ALC minimum target: 0 = -30.1 dB through 15 = -6.0 dB.

## 0x1A ADC Automute (default `0x00`)

- `ADC_AUTOMUTE_WS` [7:4]: detect window: samples = 2048 x (`winsize` + 1); 0 = 2048 samples / 42 ms, 15 = 32768 samples / 688 ms.
- `ADC_AUTOMUTE_NG` [3:0]: noise gate: 0 = -96 dB, increasing by 6 dB through 7 = -54 dB, then 3 dB through 15 = -30 dB.

## 0x1B ADC Automute/HPF (default `0x0C`)

- `ADC_AUTOMUTE_VOL` [7:5]: mute output gain; gain = value x -4 dB, 0 = 0 dB through 7 = -28 dB.
- `ADC_HPFS1` [4:0]: ADC HPF stage-1 coefficient.

## 0x1C ADC EQ/HPF (default `0x4C`)

- `ADC_EQBYPASS` [6]: 0 = normal; 1 = bypass (default).
- `ADC_HPF` [5]: 0 = freeze offset; 1 = dynamic HPF.
- `ADC_HPFS2` [4:0]: ADC HPF stage-2 coefficient.

## 0x1D ADCEQ B0[29:24] (default `0x00`

- `ADCEQ_B0[29:24]` [5:0]: upper six bits of the 30-bit ADCEQ B0 coefficient.

## 0x1E ADCEQ B0[23:16] (default `0x00`)

- `ADCEQ_B0[23:16]` [7:0]: ADCEQ B0 coefficient bits 23:16.

## 0x1F ADCEQ B0[15:8] (default `0x00`)

- `ADCEQ_B0[15:8]` [7:0]: ADCEQ B0 coefficient bits 15:8.

## 0x20 ADCEQ B0[7:0] (default `0x00`)

- `ADCEQ_B0[7:0]` [7:0]: ADCEQ B0 coefficient bits 7:0.

## 0x21 ADCEQ A1[29:24] (default `0x00`)

- `ADCEQ_A1[29:24]` [7:0]: upper byte of the 30-bit ADCEQ A1 coefficient, per datasheet.

## 0x22 ADCEQ A1[23:16] (default `0x00`)

- `ADCEQ_A1[23:16]` [7:0]: ADCEQ A1 coefficient bits 23:16.

## 0x23 ADCEQ A1[15:8] (default `0x00`)

- `ADCEQ_A1[15:8]` [7:0]: ADCEQ A1 coefficient bits 15:8.

## 0x24 ADCEQ A1[7:0] (default `0x00`)

- `ADCEQ_A1[7:0]` [7:0]: ADCEQ A1 coefficient bits 7:0.

## 0x25 ADCEQ A2[29:24] (default `0x00`)

- `ADCEQ_A2[29:24]` [7:0]: upper byte of the 30-bit ADCEQ A2 coefficient, per datasheet.

## 0x26 ADCEQ A2[23:16] (default `0x00`)

- `ADCEQ_A2[23:16]` [7:0]: ADCEQ A2 coefficient bits 23:16.

## 0x27 ADCEQ A2[15:8] (default `0x00`)

- `ADCEQ_A2[15:8]` [7:0]: ADCEQ A2 coefficient bits 15:8. The datasheet has a likely typo calling this B0.

## 0x28 ADCEQ A2[7:0] (default `0x00`)

- `ADCEQ_A2[7:0]` [7:0]: ADCEQ A2 coefficient bits 7:0.

## 0x29 ADCEQ B1[29:24] (default `0x00`)

- `ADCEQ_B1[29:24]` [7:0]: upper byte of the 30-bit ADCEQ B1 coefficient, per datasheet.

## 0x2A ADCEQ B1[23:16] (default `0x00`)

- `ADCEQ_B1[23:16]` [7:0]: ADCEQ B1 coefficient bits 23:16.

## 0x2B ADCEQ B1[15:8] (default `0x00`)

- `ADCEQ_B1[15:8]` [7:0]: ADCEQ B1 coefficient bits 15:8. The datasheet has a likely typo calling this B0.

## 0x2C ADCEQ B1[7:0] (default `0x00`)

- `ADCEQ_B1[7:0]` [7:0]: ADCEQ B1 coefficient bits 7:0.

## 0x2D ADCEQ B2[29:24] (default `0x00`)

- `ADCEQ_B2[29:24]` [7:0]: upper byte of the 30-bit ADCEQ B2 coefficient, per datasheet.

## 0x2E ADCEQ B2[23:16] (default `0x00`)

- `ADCEQ_B2[23:16]` [7:0]: ADCEQ B2 coefficient bits 23:16.

## 0x2F ADCEQ B2[15:8] (default `0x00`)

- `ADCEQ_B2[15:8]` [7:0]: ADCEQ B2 coefficient bits 15:8.

## 0x30 ADCEQ B2[7:0] (default `0x00`)

- `ADCEQ_B2[7:0]` [7:0]: ADCEQ B2 coefficient bits 7:0.

## 0x31 DAC (default `0x00`)

- `DAC_DSMMUTE_TO` [7]: 0 = mute to 8 (default); 1 = mute to 7/9.
- `DAC_DSMMUTE` [6]: 0 = unmute (default); 1 = mute.
- `DAC_DEMMUTE` [5]: 0 = unmute (default); 1 = mute.
- `DAC_INV` [4]: 0 = no phase inversion (default); 1 = 180 degree phase inversion.
- `DAC_RAMCLR` [3]: 0 = normal (default); 1 = clear DAC RAM when LRCK/DAC MCLK is active.
- `DAC_DSMDITH_OFF` [2]: 0 = dither on (default); 1 = dither off.

## 0x32 DAC Volume (default `0x00`)

- `DAC_VOLUME` [7:0]: 0x00 = -95.5 dB; 0x01 = -95.0 dB; then 0.5 dB/step; 0xBE = -0.5 dB; 0xBF = 0 dB; 0xC0 = +0.5 dB; 0xFF = +32 dB.

## 0x33 DAC Offset (default `0x00`)

- `DAC_OFFSET` [7:0]: DAC offset.

## 0x34 DAC DRC (default `0x00`)

- `DRC_EN` [7]: 0 = disable DRC (default); 1 = enable.
- `DRC_WINSIZE` [3:0]: DRC window: 0 = 0.25 dB/2 LRCK (default); 1-15 increase through 0.25 dB/65536 LRCK.

## 0x35 DAC DRC Level (default `0x00`)

- `DRC_MAXLEVEL` [7:4]: DRC maximum target: 0 = -30.1 dB through 15 = -6.0 dB.
- `DRC_MINLEVEL` [3:0]: DRC minimum target: 0 = -30.1 dB through 15 = -6.0 dB.

## 0x36 DAC (default `0x00`)

- No bit mappings are provided for this register in the supplied datasheet.

## 0x37 DAC (default `0x08`)

- `DAC_RAMPRATE` [7:4]: DAC VC/DRC ramp: 0 = disabled (default); 1-15 = 0.25 dB per 4-65536 LRCKs.
- `DAC_EQBYPASS` [3]: 0 = DACEQ enabled (default); 1 = bypass.

## 0x38 DACEQ B0[29:24] (default `0x00`)

- `DACEQ_B0[29:24]` [5:0]: upper six bits of the 30-bit DACEQ B0 coefficient.

## 0x39 DACEQ B0[23:16] (default `0x00`)

- `DACEQ_B0[23:16]` [7:0]: DACEQ B0 coefficient bits 23:16.

## 0x3A DACEQ B0[15:8] (default `0x00`)

- `DACEQ_B0[15:8]` [7:0]: DACEQ B0 coefficient bits 15:8.

## 0x3B DACEQ B0[7:0] (default `0x00`)

- `DACEQ_B0[7:0]` [7:0]: DACEQ B0 coefficient bits 7:0.

## 0x3C DACEQ B1[29:24] (default `0x00`)

- `DACEQ_B1[29:24]` [7:0]: upper byte of the 30-bit DACEQ B1 coefficient, per datasheet.

## 0x3D DACEQ B1[23:16] (default `0x00`)

- `DACEQ_B1[23:16]` [7:0]: DACEQ B1 coefficient bits 23:16.

## 0x3E DACEQ B1[15:8] (default `0x00`)

- `DACEQ_B1[15:8]` [7:0]: DACEQ B1 coefficient bits 15:8.

## 0x3F DACEQ B1[7:0] (default `0x00`)

- `DACEQ_B1[7:0]` [7:0]: DACEQ B1 coefficient bits 7:0.

## 0x40 DACEQ A1[29:24] (default `0x00`)

- `DACEQ_A1[29:24]` [7:0]: upper byte of the 30-bit DACEQ A1 coefficient, per datasheet.

## 0x41 DACEQ A1[23:16] (default `0x00`)

- `DACEQ_A1[23:16]` [7:0]: DACEQ A1 coefficient bits 23:16.

## 0x42 DACEQ A1[15:8] (default `0x00`)

- `DACEQ_A1[15:8]` [7:0]: DACEQ A1 coefficient bits 15:8.

## 0x43 DACEQ A1[7:0] (default `0x00`)

- `DACEQ_A1[7:0]` [7:0]: DACEQ A1 coefficient bits 7:0.

## 0x44 GPIO (default `0x00`)

- `ADC2DAC_SEL` [7]: 0 = disable ADC-to-DAC path (default); 1 = enable.
- `ADCDAT_SEL` [6:4]: ADCDAT output: 0 = ADC + ADC (default); 1 = ADC + 0; 2 = 0 + ADC; 3 = 0 + 0; 4 = DACL + ADC; 5 = ADC + DACR; 6 = DACL + DACR; 7 = N/A.
- `I2C_WL` [3]: internal use.
- `GPIO_SEL` [2:0]: internal use.

## 0x45 GP (default `0x00`)

- `FORCECSM` [7:4]: internal use.
- `ADC_DLY_SEL` [3]: internal use.
- `DAC_DLY_SEL` [2]: internal use.
- `DAC_AUTOCHN` [1]: internal use.
- `PULLUP_SE` [0]: BCLK/LRCK pull-up: 0 = on (default); 1 = off.

## 0xFA I2C (default `0x00`)

- `I2C_RETIME` [1]: internal use.
- `INI_REG` [0]: 0 = no reset (default); 1 = reset registers to defaults except this register.

## 0xFC Flag (default `0x00`, read-only fields)

- `FLAG_CSM_CHIP` [6:4]: CSM state: 0 = S0; 1 = S1; 2 = S2; 3 = S3; 6 = S6; 7 = S7; other values are dummy states.
- `FLAG_ADCAM` [1]: ADC automute flag.
- `FLAG_DACAM` [0]: internal use.

## 0xFD Chip ID 1 (default `0x83`)

- `CHIP_ID1` [7:0]: chip ID, `0x83`.

## 0xFE Chip ID 2 (default `0x11`)

- `CHIP_ID2` [7:0]: chip ID, `0x11`.

## 0xFF Chip Version (default `0x00`)

- `CHIP_VER` [7:0]: chip version information.

## Comments

- The datasheet states that MCLK, LRCK, and SCLK should be present before control-register configuration; otherwise reset the codec after the clocks are provided.
- Registers `0x21`, `0x25`, `0x29`, `0x2D`, `0x3C`, and `0x40` describe 30-bit coefficient upper portions as `[29:24]` while exposing eight bits. This is reproduced as documented.
- Register `0x36` has no bit definitions in the supplied revision.
- Do not write undocumented or internal-use fields unless validated by the vendor.
