<?php
$g_IOT=array(
239 => [ 'idx'=>239, 'title'=>"Ignition"], //
240 => [ 'idx'=>240, 'title'=>"Movement"], // 
80 => [ 'idx'=>80, 'title'=>"Data Mode"], // 
21 => [ 'idx'=>21, 'title'=>"GSM Signal"], // 
200 => [ 'idx'=>200, 'title'=>"Sleep Mode"], // 
69 => [ 'idx'=>69, 'title'=>"GNSS Status"], // 
181 => [ 'idx'=>181, 'title'=>"GNSS PDOP", 'scale'=>0.1, 'unit' =>""], // Coefficient, <a href="/view/FMB120_Status_info#GNSS_Info" 'title'="FMB120 Status info">calculation formula</a>
182 => [ 'idx'=>182, 'title'=>"GNSS HDOP", 'scale'=>0.1, 'unit' =>""], // Coefficient, <a href="/view/FMB120_Status_info#GNSS_Info" 'title'="FMB120 Status info">calculation formula</a>
66 => [ 'idx'=>66, 'title'=>"External Voltage", 'scale'=>0.001, 'unit' =>"V"], // 
24 => [ 'idx'=>24, 'title'=>"Speed", 'unit' =>"km/h"], // 
205 => [ 'idx'=>205, 'title'=>"GSM Cell ID"], // 
206 => [ 'idx'=>206, 'title'=>"GSM Area Code"], // 
67 => [ 'idx'=>67, 'title'=>"Battery Voltage", 'scale'=>0.001, 'unit' =>"V"], // 
68 => [ 'idx'=>68, 'title'=>"Battery Current", 'scale'=>0.001, 'unit' =>"A"], // 
241 => [ 'idx'=>241, 'title'=>"Active GSM Operator"], // 
199 => [ 'idx'=>199, 'title'=>"Trip Odometer", 'unit' =>"m"], // 
16 => [ 'idx'=>16, 'title'=>"Total Odometer"], // 
1 => [ 'idx'=>1, 'title'=>"Digital Input 1"], // 
9 => [ 'idx'=>9, 'title'=>"Analog Input 1", 'scale'=>0.001, 'unit' =>"mV"], // 
179 => [ 'idx'=>179, 'title'=>"Digital Output 1"], // 
12 => [ 'idx'=>12, 'title'=>"Fuel Used GPS", 'scale'=>0.001, 'unit' =>"l"], // 
13 => [ 'idx'=>13, 'title'=>"Fuel Rate GPS", 'scale'=>0.01, 'unit' =>"l/100km"], // 
17 => [ 'idx'=>17, 'title'=>"Axis X", 'unit' =>"mG"], // 
18 => [ 'idx'=>18, 'title'=>"Axis Y", 'unit' =>"mG"], // 
19 => [ 'idx'=>19, 'title'=>"Axis Z", 'unit' =>"mG"], // 
11 => [ 'idx'=>11, 'title'=>"ICCID1"], // 
10 => [ 'idx'=>10, 'title'=>"SD Status"], // 
2 => [ 'idx'=>2, 'title'=>"Digital Input 2"], // 
3 => [ 'idx'=>3, 'title'=>"Digital Input 3"], // 
6 => [ 'idx'=>6, 'title'=>"Analog Input 2", 'scale'=>0.001, 'unit' =>"mV"], // 
180 => [ 'idx'=>180, 'title'=>"Digital Output 2"], // Logic 0/1
72 => [ 'idx'=>72, 'title'=>"Dallas Temperature 1", 'scale'=>0.1, 'unit' =>"&deg;C"], // Degrees ( &deg;C ), -55 - +115,
73 => [ 'idx'=>73, 'title'=>"Dallas Temperature 2", 'scale'=>0.1, 'unit' =>"&deg;C"], // Degrees ( &deg;C ), -55 - +115,
74 => [ 'idx'=>74, 'title'=>"Dallas Temperature 3", 'scale'=>0.1, 'unit' =>"&deg;C"], // Degrees ( &deg;C ), -55 - +115,
75 => [ 'idx'=>75, 'title'=>"Dallas Temperature 4", 'scale'=>0.1, 'unit' =>"&deg;C"], // Degrees ( &deg;C ), -55 - +115,
76 => [ 'idx'=>76, 'title'=>"Dallas Temperature ID 1"], // Dallas sensor ID
77 => [ 'idx'=>77, 'title'=>"Dallas Temperature ID 2"], // Dallas sensor ID
79 => [ 'idx'=>79, 'title'=>"Dallas Temperature ID 3"], // Dallas sensor ID
71 => [ 'idx'=>71, 'title'=>"Dallas Temperature ID 4"], // Dallas sensor ID
78 => [ 'idx'=>78, 'title'=>"iButton"], // iButton ID
207 => [ 'idx'=>207, 'title'=>"RFID"], // RFID ID
201 => [ 'idx'=>201, 'title'=>"LLS 1 Fuel Level", 'unit' =>"kvants or ltr"], // Fuel level measured by LLS sensor via RS232/RS485
202 => [ 'idx'=>202, 'title'=>"LLS 1 Temperature", 'unit' =>"&deg;C"], // Fuel temperature measured by LLS via RS232/RS485
203 => [ 'idx'=>203, 'title'=>"LLS 2 Fuel Level", 'unit' =>"kvants or ltr"], // Fuel level measured by LLS sensor via RS485
204 => [ 'idx'=>204, 'title'=>"LLS 2 Temperature", 'unit' =>"&deg;C"], // Fuel temperature measured by LLS via RS485
210 => [ 'idx'=>210, 'title'=>"LLS 3 Fuel Level", 'unit' =>"kvants or ltr"], // Fuel level measured by LLS sensor via RS485
211 => [ 'idx'=>211, 'title'=>"LLS 3 Temperature", 'unit' =>"&deg;C"], // Fuel temperature measured by LLS via RS485
212 => [ 'idx'=>212, 'title'=>"LLS 4 Fuel Level", 'unit' =>"kvants or ltr"], // Fuel level measured by LLS sensor via RS485
213 => [ 'idx'=>213, 'title'=>"LLS 4 Temperature", 'unit' =>"&deg;C"], // Fuel temperature measured by LLS via RS485
214 => [ 'idx'=>214, 'title'=>"LLS 5 Fuel Level", 'unit' =>"kvants or ltr"], // Fuel level measured by LLS sensor via RS485
215 => [ 'idx'=>215, 'title'=>"LLS 5 Temperature", 'unit' =>"&deg;C"], // Fuel temperature measured by LLS via RS485
15 => [ 'idx'=>15, 'title'=>"Eco Score", 'scale'=>0.01, 'unit' =>"-"], // Average amount of events on some distance
113 => [ 'idx'=>113, 'title'=>"Battery Level", 'unit' =>"%"], // Battery capacity level
238 => [ 'idx'=>238, 'title'=>"User ID"], // MAC address of NMEA receiver device connected via Bluetooth
237 => [ 'idx'=>237, 'title'=>"Network Type"], // 0 - 3G
8 => [ 'idx'=>8, 'title'=>"Authorized iButton"], // If ID is shown in this I/O that means that attached iButton is in iButton List
4 => [ 'idx'=>4, 'title'=>"Pulse Counter Din1"], // Counts pulses, count is reset when records are saved
5 => [ 'idx'=>5, 'title'=>"Pulse Counter Din2"], // Counts pulses, count is reset when records are saved
263 => [ 'idx'=>263, 'title'=>"BT Status"], // 0 - BT is disabled
264 => [ 'idx'=>264, 'title'=>"Barcode ID"], // Barcode ID
269 => [ 'idx'=>269, 'title'=>"Escort LLS Temperature #1", 'unit' =>"&deg;C"], // Fuel temperature
270 => [ 'idx'=>270, 'title'=>"Escort LLS Fuel level #1"], // Fuel Level
271 => [ 'idx'=>271, 'title'=>"Escort LLS Battery Voltage #1", 'scale'=>0.01, 'unit' =>"V"], // Battery Voltage
272 => [ 'idx'=>272, 'title'=>"Escort LLS Temperature #2", 'unit' =>"&deg;C"], // Fuel temperature
273 => [ 'idx'=>273, 'title'=>"Escort LLS Fuel level #2"], // Fuel Level
274 => [ 'idx'=>274, 'title'=>"Escort LLS Battery Voltage #2", 'scale'=>0.01, 'unit' =>"V"], // Battery Voltage
275 => [ 'idx'=>275, 'title'=>"Escort LLS Temperature #3", 'unit' =>"&deg;C"], // Fuel temperature
276 => [ 'idx'=>276, 'title'=>"Escort LLS Fuel level #3"], // Fuel Level
277 => [ 'idx'=>277, 'title'=>"Escort LLS Battery Voltage #3", 'scale'=>0.01, 'unit' =>"V"], // Battery Voltage
278 => [ 'idx'=>278, 'title'=>"Escort LLS Temperature #4", 'unit' =>"&deg;C"], // Fuel temperature
279 => [ 'idx'=>279, 'title'=>"Escort LLS Fuel level #4"], // Fuel Level
280 => [ 'idx'=>280, 'title'=>"Escort LLS Battery Voltage #4", 'scale'=>0.01, 'unit' =>"V"], // Battery Voltage
303 => [ 'idx'=>303, 'title'=>"Instant Movement"], // Logic: 0/1 returns movement value
327 => [ 'idx'=>327, 'title'=>"UL202-02 Sensor Fuel level", 'scale'=>0.1, 'unit' =>"mm"], // UL202-02 Sensor Fuel level
483 => [ 'idx'=>483, 'title'=>"UL202-02 Sensor Status"], // UL202-02 sensor status codes
380 => [ 'idx'=>380, 'title'=>"Digital output 3"], // Logic: 0/1
381 => [ 'idx'=>381, 'title'=>"Ground Sense"], // Logic: 0/1
387 => [ 'idx'=>387, 'title'=>"ISO6709  Coordinates"], // ISO6709  Coordinates
636 => [ 'idx'=>636, 'title'=>"UMTS/LTE Cell ID"], // 
403 => [ 'idx'=>403, 'title'=>"Driver Name"], // Driver name extracted from card, displayed without delimiters ($ signs)
404 => [ 'idx'=>404, 'title'=>"Driver card license type"], // <td style="vertical-align: middle; text-align: left;">
405 => [ 'idx'=>405, 'title'=>"Driver Gender"], // 
406 => [ 'idx'=>406, 'title'=>"Driver Card ID"], // 
407 => [ 'idx'=>407, 'title'=>"Driver Card Issue Year"], // Value from card as it is
408 => [ 'idx'=>408, 'title'=>"Driver Card Issue Year"], // -
409 => [ 'idx'=>409, 'title'=>"Driver Status Event"], // 
155 => [ 'idx'=>155, 'title'=>"Geofence zone 01"], // 
156 => [ 'idx'=>156, 'title'=>"Geofence zone 02"], // 
157 => [ 'idx'=>157, 'title'=>"Geofence zone 03"], // 
158 => [ 'idx'=>158, 'title'=>"Geofence zone 04"], // 
159 => [ 'idx'=>159, 'title'=>"Geofence zone 05"], // 
61 => [ 'idx'=>61, 'title'=>"Geofence zone 06"], // 
62 => [ 'idx'=>62, 'title'=>"Geofence zone 07"], // 
63 => [ 'idx'=>63, 'title'=>"Geofence zone 08"], // 
64 => [ 'idx'=>64, 'title'=>"Geofence zone 09"], // 
65 => [ 'idx'=>65, 'title'=>"Geofence zone 10"], // 
70 => [ 'idx'=>70, 'title'=>"Geofence zone 11"], // 
88 => [ 'idx'=>88, 'title'=>"Geofence zone 12"], // 
91 => [ 'idx'=>91, 'title'=>"Geofence zone 13"], // 
92 => [ 'idx'=>92, 'title'=>"Geofence zone 14"], // 
93 => [ 'idx'=>93, 'title'=>"Geofence zone 15"], // 
94 => [ 'idx'=>94, 'title'=>"Geofence zone 16"], // 
95 => [ 'idx'=>95, 'title'=>"Geofence zone 17"], // 
96 => [ 'idx'=>96, 'title'=>"Geofence zone 18"], // 
97 => [ 'idx'=>97, 'title'=>"Geofence zone 19"], // 
98 => [ 'idx'=>98, 'title'=>"Geofence zone 20"], // 
99 => [ 'idx'=>99, 'title'=>"Geofence zone 21"], // 
153 => [ 'idx'=>153, 'title'=>"Geofence zone 22"], // 
154 => [ 'idx'=>154, 'title'=>"Geofence zone 23"], // 
190 => [ 'idx'=>190, 'title'=>"Geofence zone 24"], // 
191 => [ 'idx'=>191, 'title'=>"Geofence zone 25"], // 
192 => [ 'idx'=>192, 'title'=>"Geofence zone 26"], // 
193 => [ 'idx'=>193, 'title'=>"Geofence zone 27"], // 
194 => [ 'idx'=>194, 'title'=>"Geofence zone 28"], // 
195 => [ 'idx'=>195, 'title'=>"Geofence zone 29"], // 
196 => [ 'idx'=>196, 'title'=>"Geofence zone 30"], // 
197 => [ 'idx'=>197, 'title'=>"Geofence zone 31"], // 
198 => [ 'idx'=>198, 'title'=>"Geofence zone 32"], // 
208 => [ 'idx'=>208, 'title'=>"Geofence zone 33"], // 
209 => [ 'idx'=>209, 'title'=>"Geofence zone 34"], // 
216 => [ 'idx'=>216, 'title'=>"Geofence zone 35"], // 
217 => [ 'idx'=>217, 'title'=>"Geofence zone 36"], // 
218 => [ 'idx'=>218, 'title'=>"Geofence zone 37"], // 
219 => [ 'idx'=>219, 'title'=>"Geofence zone 38"], // 
220 => [ 'idx'=>220, 'title'=>"Geofence zone 39"], // 
221 => [ 'idx'=>221, 'title'=>"Geofence zone 40"], // 
222 => [ 'idx'=>222, 'title'=>"Geofence zone 41"], // 
223 => [ 'idx'=>223, 'title'=>"Geofence zone 42"], // 
224 => [ 'idx'=>224, 'title'=>"Geofence zone 43"], // 
225 => [ 'idx'=>225, 'title'=>"Geofence zone 44"], // 
226 => [ 'idx'=>226, 'title'=>"Geofence zone 45"], // 
227 => [ 'idx'=>227, 'title'=>"Geofence zone 46"], // 
228 => [ 'idx'=>228, 'title'=>"Geofence zone 47"], // 
229 => [ 'idx'=>229, 'title'=>"Geofence zone 48"], // 
230 => [ 'idx'=>230, 'title'=>"Geofence zone 49"], // 
231 => [ 'idx'=>231, 'title'=>"Geofence zone 50"], // 
175 => [ 'idx'=>175, 'title'=>"Auto Geofence"], // 
250 => [ 'idx'=>250, 'title'=>"Trip"], // 
255 => [ 'idx'=>255, 'title'=>"Over Speeding", 'unit' =>"km/h"], // 
257 => [ 'idx'=>257, 'title'=>"Crash trace data"], // Crash trace data
285 => [ 'idx'=>285, 'title'=>"Blood alcohol content"], // Alcohol content in blood in perlims and mode. First 14 bits from MSB are perlims multiplied by 1000 and last to bits are 0 - Passive test, 1 Active test, 2 and 3 are reserved.
251 => [ 'idx'=>251, 'title'=>"Idling"], // 
253 => [ 'idx'=>253, 'title'=>"Green driving type"], // 
246 => [ 'idx'=>246, 'title'=>"Towing"], // 
252 => [ 'idx'=>252, 'title'=>"Unplug"], // 
247 => [ 'idx'=>247, 'title'=>"Crash detection"], // 
248 => [ 'idx'=>248, 'title'=>"Immobilizer"], // 
254 => [ 'idx'=>254, 'title'=>"Green Driving Value", 'scale'=>0.01, 'unit' =>"G or rad"], // Depending on green driving type: if harsh acceleration or braking – g*100 (value 123 -&gt; 1.23g). If Green driving source is „GPS“ – harsh cornering value is rad/s*100. If source is „Accelerometer“ – g*100.
249 => [ 'idx'=>249, 'title'=>"Jamming"], // 
14 => [ 'idx'=>14, 'title'=>"ICCID2"], // 
243 => [ 'idx'=>243, 'title'=>"Green driving event duration", 'unit' =>"ms"], // 
236 => [ 'idx'=>236, 'title'=>"Alarm"], // 
258 => [ 'idx'=>258, 'title'=>"EcoMaximum"], // 
259 => [ 'idx'=>259, 'title'=>"EcoAverage"], // 
260 => [ 'idx'=>260, 'title'=>"EcoDuration", 'unit' =>"ms"], // 
283 => [ 'idx'=>283, 'title'=>"Driving State"], // 
284 => [ 'idx'=>284, 'title'=>"Driving Records"], // 
391 => [ 'idx'=>391, 'title'=>"Private mode"], // Private mode state
317 => [ 'idx'=>317, 'title'=>"Crash event counter"], // Connects trace with specific eventual crash record
449 => [ 'idx'=>449, 'title'=>"Ignition On Counter", 'unit' =>"s"], // 
256 => [ 'idx'=>256, 'title'=>"VIN"], // VIN number
30 => [ 'idx'=>30, 'title'=>"Number of DTC"], // Number of DTC
31 => [ 'idx'=>31, 'title'=>"Engine Load", 'unit' =>"%"], // Calculated engine load value
32 => [ 'idx'=>32, 'title'=>"Coolant Temperature", 'unit' =>"&deg;C"], // 
33 => [ 'idx'=>33, 'title'=>"Short Fuel Trim", 'unit' =>"%"], // 
34 => [ 'idx'=>34, 'title'=>"Fuel pressure", 'unit' =>"kPa"], // 
35 => [ 'idx'=>35, 'title'=>"Intake MAP", 'unit' =>"kPa"], // 
36 => [ 'idx'=>36, 'title'=>"Engine RPM", 'unit' =>"rpm"], // 
37 => [ 'idx'=>37, 'title'=>"Vehicle Speed", 'unit' =>"km/h"], // 
38 => [ 'idx'=>38, 'title'=>"Timing Advance", 'unit' =>"&deg;"], // 
39 => [ 'idx'=>39, 'title'=>"Intake Air Temperature", 'unit' =>"&deg;C"], // 
40 => [ 'idx'=>40, 'title'=>"MAF", 'scale'=>0.01, 'unit' =>"g/sec"], // 
41 => [ 'idx'=>41, 'title'=>"Throttle Position", 'unit' =>"%"], // 
42 => [ 'idx'=>42, 'title'=>"Runtime since engine start", 'unit' =>"s"], // 
43 => [ 'idx'=>43, 'title'=>"Distance Traveled MIL On", 'unit' =>"km"], // 
44 => [ 'idx'=>44, 'title'=>"Relative Fuel Rail Pressure", 'scale'=>0.1, 'unit' =>"kPa"], // 
45 => [ 'idx'=>45, 'title'=>"Direct Fuel Rail Pressure", 'scale'=>10, 'unit' =>"kPa"], // 
46 => [ 'idx'=>46, 'title'=>"Commanded EGR", 'unit' =>"%"], // 
47 => [ 'idx'=>47, 'title'=>"EGR Error", 'unit' =>"%"], // 
48 => [ 'idx'=>48, 'title'=>"Fuel Level", 'unit' =>"%"], // 
49 => [ 'idx'=>49, 'title'=>"Distance Since Codes Clear", 'unit' =>"km"], // 
50 => [ 'idx'=>50, 'title'=>"Barometic Pressure", 'unit' =>"kPa"], // 
51 => [ 'idx'=>51, 'title'=>"Control Module Voltage", 'scale'=>0.001, 'unit' =>"V"], // 
52 => [ 'idx'=>52, 'title'=>"Absolute Load Value", 'unit' =>"%"], // 
53 => [ 'idx'=>53, 'title'=>"Ambient Air Temperature", 'unit' =>"&deg;C"], // 
54 => [ 'idx'=>54, 'title'=>"Time Run With MIL On", 'unit' =>"min"], // 
55 => [ 'idx'=>55, 'title'=>"Time Since Codes Cleared", 'unit' =>"min"], // 
56 => [ 'idx'=>56, 'title'=>"Absolute Fuel Rail Pressure", 'scale'=>0.1, 'unit' =>"kPa"], // 
57 => [ 'idx'=>57, 'title'=>"Hybrid battery pack life", 'unit' =>"%"], // 
58 => [ 'idx'=>58, 'title'=>"Engine Oil Temperature", 'unit' =>"&deg;C"], // 
59 => [ 'idx'=>59, 'title'=>"Fuel injection timing", 'scale'=>0.01, 'unit' =>"&deg;"], // 
281 => [ 'idx'=>281, 'title'=>"Fault Codes"], // 
60 => [ 'idx'=>60, 'title'=>"Fuel Rate", 'scale'=>0.01, 'unit' =>"l/100km"], // 
389 => [ 'idx'=>389, 'title'=>"OBD OEM Total Mileage", 'unit' =>"km"], // Total mileage received by requesting vehicle specific PID
390 => [ 'idx'=>390, 'title'=>"OBD OEM Fuel Level", 'scale'=>0.1, 'unit' =>"l"], // Fuel level in litres received by requesting vehicle specific PID
81 => [ 'idx'=>81, 'title'=>"Vehicle Speed", 'unit' =>"km/h"], // Vehicle Speed
82 => [ 'idx'=>82, 'title'=>"Accelerator Pedal Position", 'unit' =>"%"], // Value in percentages
83 => [ 'idx'=>83, 'title'=>"Fuel Consumed", 'scale'=>0.1, 'unit' =>"l"], // Value in liters
84 => [ 'idx'=>84, 'title'=>"Fuel level", 'scale'=>0.1, 'unit' =>"l"], // Value in liters
85 => [ 'idx'=>85, 'title'=>"Engine RPM", 'unit' =>"rpm"], // Value in rounds per minute
87 => [ 'idx'=>87, 'title'=>"Total Mileage", 'unit' =>"m"], // Value in meters
89 => [ 'idx'=>89, 'title'=>"Fuel level", 'unit' =>"%"], // Value in percentages
90 => [ 'idx'=>90, 'title'=>"Door Status"], // 
100 => [ 'idx'=>100, 'title'=>"Program Number"], // Value: Min – 0, Max – 99999
101 => [ 'idx'=>101, 'title'=>"Module ID 8B"], // Module ID 8 Bytes
388 => [ 'idx'=>388, 'title'=>"Module ID 17B"], // Module ID 17 Bytes
102 => [ 'idx'=>102, 'title'=>"Engine Worktime", 'unit' =>"min"], // Engine work time
103 => [ 'idx'=>103, 'title'=>"Engine Worktime (counted)", 'unit' =>"min"], // Total engine work time
105 => [ 'idx'=>105, 'title'=>"Total Mileage (counted)", 'unit' =>"m"], // Total Vehicle Mileage
107 => [ 'idx'=>107, 'title'=>"Fuel Consumed(counted)", 'scale'=>0.1, 'unit' =>"l"], // Total Fuel Consumed
110 => [ 'idx'=>110, 'title'=>"Fuel Rate", 'scale'=>0.1, 'unit' =>"l/h"], // Fuel rate
111 => [ 'idx'=>111, 'title'=>"AdBlue Level", 'unit' =>"%"], // AdBlue
112 => [ 'idx'=>112, 'title'=>"AdBlue Level", 'scale'=>0.1, 'unit' =>"l"], // AdBlue level
114 => [ 'idx'=>114, 'title'=>"Engine Load", 'unit' =>"%"], // Engine Load
115 => [ 'idx'=>115, 'title'=>"Engine Temperature", 'scale'=>0.1, 'unit' =>"&deg;C"], // Engine Temperature
118 => [ 'idx'=>118, 'title'=>"Axle 1 Load", 'unit' =>"kg"], // Axle 1 load
119 => [ 'idx'=>119, 'title'=>"Axle 2 Load", 'unit' =>"kg"], // Axle 2 load
120 => [ 'idx'=>120, 'title'=>"Axle 3 Load", 'unit' =>"kg"], // Axle 3 load
121 => [ 'idx'=>121, 'title'=>"Axle 4 Load", 'unit' =>"kg"], // Axle 4 load
122 => [ 'idx'=>122, 'title'=>"Axle 5 Load", 'unit' =>"kg"], // Axle 5 load
123 => [ 'idx'=>123, 'title'=>"Control State Flags"], // Control state flags
124 => [ 'idx'=>124, 'title'=>"Agricultural Machinery Flags"], // Agricultural machinery flags
125 => [ 'idx'=>125, 'title'=>"Harvesting Time", 'unit' =>"min"], // Harvesting time
126 => [ 'idx'=>126, 'title'=>"Area of Harvest", 'unit' =>"m<sup>2</sup>"], // Area of harvest in square meters
127 => [ 'idx'=>127, 'title'=>"LVC of Harvest", 'unit' =>"m<sup>2</sup>/h"], // Mowing efficiency
128 => [ 'idx'=>128, 'title'=>"Grain Mow Volume", 'unit' =>"kg"], // Mow volume
129 => [ 'idx'=>129, 'title'=>"Grain Moisture", 'unit' =>"%"], // Grain moisture
130 => [ 'idx'=>130, 'title'=>"Harvesting Drum RPM", 'unit' =>"rpm"], // Harvesting drum rpm
131 => [ 'idx'=>131, 'title'=>"Gap Under Harvesting Drum", 'unit' =>"mm"], // Gap under harvesting drum
132 => [ 'idx'=>132, 'title'=>"Security State Flags"], // Security state flags
133 => [ 'idx'=>133, 'title'=>"Tacho Total Distance", 'unit' =>"m"], // Tacho Total Vehicle Distance
134 => [ 'idx'=>134, 'title'=>"Trip Distance", 'unit' =>"m"], // Trip distance
135 => [ 'idx'=>135, 'title'=>"Tacho Vehicle Speed", 'unit' =>"km/h"], // Tacho vehicle speed
136 => [ 'idx'=>136, 'title'=>"Tacho Driver Card Presence"], // Tacho Driver Card Presence
137 => [ 'idx'=>137, 'title'=>"Driver 1 States"], // Driver 1 States
138 => [ 'idx'=>138, 'title'=>"Driver 2 States"], // Driver 2 States
139 => [ 'idx'=>139, 'title'=>"Driver 1 Driving Time", 'unit' =>"min."], // Driver 1 Continuous Driving Time, minutes
140 => [ 'idx'=>140, 'title'=>"Driver 2 Driving Time", 'unit' =>"min."], // Driver 2 Continuous Driving Time, minutes
141 => [ 'idx'=>141, 'title'=>"Driver 1 Break Time", 'unit' =>"min."], // Driver 1 Cumulative Break Time, minutes
142 => [ 'idx'=>142, 'title'=>"Driver 2 Break Time", 'unit' =>"min."], // Driver 2 Cumulative Break Time, minutes
143 => [ 'idx'=>143, 'title'=>"Driver 1 Acitivity Duratation", 'unit' =>"min."], // Driver 1 Duration Of Selected Activity, minutes
144 => [ 'idx'=>144, 'title'=>"Driver 2 Acitivity Duratation", 'unit' =>"min."], // Driver 2 Duration Of Selected Activity, minutes
145 => [ 'idx'=>145, 'title'=>"Driver 1 Driving Time", 'unit' =>"min."], // Driver 1 Cumulative Driving Time, minutes
146 => [ 'idx'=>146, 'title'=>"Driver 2 Driving Time", 'unit' =>"min."], // Driver 2 Cumulative Driving Time, minutes
147 => [ 'idx'=>147, 'title'=>"Driver 1 ID High"], // Driver 1 ID High
148 => [ 'idx'=>148, 'title'=>"Driver 1 ID Low"], // Driver 1 ID Low
149 => [ 'idx'=>149, 'title'=>"Driver 2 ID High"], // Driver 2 ID High
150 => [ 'idx'=>150, 'title'=>"Driver 2 ID Low"], // Driver 2 ID Low
151 => [ 'idx'=>151, 'title'=>"Battery Temperature", 'scale'=>0.1, 'unit' =>"&deg;C"], // 
152 => [ 'idx'=>152, 'title'=>"Battery Level", 'unit' =>"%"], // 
160 => [ 'idx'=>160, 'title'=>"DTC Faults Count"], // DTC faults
161 => [ 'idx'=>161, 'title'=>"Slope of Arm", 'unit' =>"Degrees &deg;"], // Slope Of Arm
162 => [ 'idx'=>162, 'title'=>"Rotation of Arm", 'unit' =>"Degrees &deg;"], // Rotation Of Arm
163 => [ 'idx'=>163, 'title'=>"Eject of Arm", 'unit' =>"m"], // Eject of arm
164 => [ 'idx'=>164, 'title'=>"Horizontal Distance Arm", 'unit' =>"m"], // Horizontal Distance Arm Vehicle
164 => [ 'idx'=>164, 'title'=>"Horizontal Distance Arm", 'unit' =>"m"], // Horizontal Distance Arm Vehicle
164 => [ 'idx'=>164, 'title'=>"Horizontal Distance Arm", 'unit' =>"m"], // Horizontal Distance Arm Vehicle
165 => [ 'idx'=>165, 'title'=>"Height Arm Above Ground", 'unit' =>"m"], // Height Arm Above Ground
166 => [ 'idx'=>166, 'title'=>"Drill RPM", 'unit' =>"rpm"], // Drill RPM
167 => [ 'idx'=>167, 'title'=>"Spread Salt", 'unit' =>"g/m<sup>2</sup>"], // Amount Of Spread Salt Square Meter
168 => [ 'idx'=>168, 'title'=>"Battery Voltage", 'unit' =>"V"], // Battery Voltage
169 => [ 'idx'=>169, 'title'=>"Spread Fine Grained Salt", 'unit' =>"T"], // Amount Of Spread Fine Grained Salt
170 => [ 'idx'=>170, 'title'=>"Coarse Grained Salt", 'unit' =>"T"], // Amount Of Coarse Grained Salt
171 => [ 'idx'=>171, 'title'=>"Spread DiMix", 'unit' =>"T"], // Amount Of Spread DiMix
172 => [ 'idx'=>172, 'title'=>"Spread Coarse Grained Calcium", 'unit' =>"m<sup>3</sup>"], // Amount Of Spread Coarse Grained Calcium
173 => [ 'idx'=>173, 'title'=>"Spread Calcium Chloride", 'unit' =>"m<sup>3</sup>"], // Amount Of Spread Calcium Chloride
174 => [ 'idx'=>174, 'title'=>"Spread Sodium Chloride", 'unit' =>"m<sup>3</sup>"], // Amount Of Spread Sodium Chloride
176 => [ 'idx'=>176, 'title'=>"Spread Magnesium Chloride", 'unit' =>"m<sup>3</sup>"], // Amount Of Spread Magnesium Chloride
177 => [ 'idx'=>177, 'title'=>"Amount Of Spread Gravel", 'unit' =>"T"], // Amount Of Spread Gravel
178 => [ 'idx'=>178, 'title'=>"Amount Of Spread Sand", 'unit' =>"T"], // Amount Of Spread Sand
183 => [ 'idx'=>183, 'title'=>"Width Pouring Left", 'unit' =>"m"], // Width Pouring Left
184 => [ 'idx'=>184, 'title'=>"Width Pouring Right", 'unit' =>"m"], // Width Pouring Right
185 => [ 'idx'=>185, 'title'=>"Salt Spreader Working Hours", 'unit' =>"h"], // Salt Spreader Working Hours
186 => [ 'idx'=>186, 'title'=>"Distance During Salting", 'unit' =>"km"], // Distance During Salting
187 => [ 'idx'=>187, 'title'=>"Load Weight", 'unit' =>"kg"], // Load Weight
188 => [ 'idx'=>188, 'title'=>"Retarder Load", 'unit' =>"%"], // Retarded Load
189 => [ 'idx'=>189, 'title'=>"Cruise Time", 'unit' =>"min"], // Cruise time
232 => [ 'idx'=>232, 'title'=>"CNG status"], // CNG status
233 => [ 'idx'=>233, 'title'=>"CNG used", 'unit' =>"kg"], // CNG used
234 => [ 'idx'=>234, 'title'=>"CNG level", 'unit' =>"%"], // CNG used
235 => [ 'idx'=>235, 'title'=>"Engine Oil Level"], // Oil Level
304 => [ 'idx'=>304, 'title'=>"Vehicle Range on Battery", 'unit' =>"m"], // Vehicle Range on Battery
305 => [ 'idx'=>305, 'title'=>"Vehicle Range On Additional Fuel", 'unit' =>"m"], // Vehicle Range On Additional Fuel
325 => [ 'idx'=>325, 'title'=>"VIN"], // VIN number
282 => [ 'idx'=>282, 'title'=>"DTC Faults code"], // DTC Fault code
517 => [ 'idx'=>517, 'title'=>"SecurityStateFlags_P4"], // Security state flags protocol 4, more information click here <a href="/view/FMB140_CAN_connection,_SIMPLE-CAN_/_MINI-CAN_connection#CAN_Adapter_State_Flags" 'title'="FMB140 CAN connection, SIMPLE-CAN / MINI-CAN connection">Flags</a>
518 => [ 'idx'=>518, 'title'=>"ControlStateFlags_P4"], // Control state flags protocol 4, more information click here <a href="/view/FMB140_CAN_connection,_SIMPLE-CAN_/_MINI-CAN_connection#CAN_Adapter_State_Flags" 'title'="FMB140 CAN connection, SIMPLE-CAN / MINI-CAN connection">Flags</a>
519 => [ 'idx'=>519, 'title'=>"IndicatorStateFlags_P4"], // Indicator state flags protocol 4, more information click here <a href="/view/FMB140_CAN_connection,_SIMPLE-CAN_/_MINI-CAN_connection#CAN_Adapter_State_Flags" 'title'="FMB140 CAN connection, SIMPLE-CAN / MINI-CAN connection">Flags</a>
520 => [ 'idx'=>520, 'title'=>"AgriculturalStateFlags_P4"], // Agricultural state flags protocol 4, more information click here <a href="/view/FMB140_CAN_connection,_SIMPLE-CAN_/_MINI-CAN_connection#CAN_Adapter_State_Flags" 'title'="FMB140 CAN connection, SIMPLE-CAN / MINI-CAN connection">Flags</a>
521 => [ 'idx'=>521, 'title'=>"UtilityStateFlags_P4"], // Utility state flags protocol 4, more information click here <a href="/view/FMB140_CAN_connection,_SIMPLE-CAN_/_MINI-CAN_connection#CAN_Adapter_State_Flags" 'title'="FMB140 CAN connection, SIMPLE-CAN / MINI-CAN connection">Flags</a>
522 => [ 'idx'=>522, 'title'=>"CisternStateFlags_P4"], // Cistern state flags protocol 4, more information click here <a href="/view/FMB140_CAN_connection,_SIMPLE-CAN_/_MINI-CAN_connection#CAN_Adapter_State_Flags" 'title'="FMB140 CAN connection, SIMPLE-CAN / MINI-CAN connection">Flags</a>
855 => [ 'idx'=>855, 'title'=>"Total LNG Used", 'unit' =>"kg"], // Total LNG used in kilograms
856 => [ 'idx'=>856, 'title'=>"Total LNG Used Counted", 'unit' =>"kg"], // Total LNG used counted in kg
857 => [ 'idx'=>857, 'title'=>"LNG Level Proc", 'unit' =>"%"], // LNG level in proc
858 => [ 'idx'=>858, 'title'=>"LNG Level kg", 'unit' =>"kg"], // LNG level in kg
385 => [ 'idx'=>385, 'title'=>"Beacon"], // List of Beacon IDs
25 => [ 'idx'=>25, 'title'=>"BLE Temperature #1", 'scale'=>0.01, 'unit' =>"&deg;C"], // 
26 => [ 'idx'=>26, 'title'=>"BLE Temperature #2", 'scale'=>0.01, 'unit' =>"&deg;C"], // 
27 => [ 'idx'=>27, 'title'=>"BLE Temperature #3", 'scale'=>0.01, 'unit' =>"&deg;C"], // 
28 => [ 'idx'=>28, 'title'=>"BLE Temperature #4", 'scale'=>0.01, 'unit' =>"&deg;C"], // 
29 => [ 'idx'=>29, 'title'=>"BLE Battery #1", 'unit' =>"%"], // Battery level of sensor #1;
20 => [ 'idx'=>20, 'title'=>"BLE Battery #2", 'unit' =>"%"], // Battery level of sensor #2;
22 => [ 'idx'=>22, 'title'=>"BLE Battery #3", 'unit' =>"%"], // Battery level of sensor #3;
23 => [ 'idx'=>23, 'title'=>"BLE Battery #4", 'unit' =>"%"], // Battery level of sensor #4;
86 => [ 'idx'=>86, 'title'=>"BLE Humidity #1", 'scale'=>0.1, 'unit' =>"%RH"], // Humidity;
104 => [ 'idx'=>104, 'title'=>"BLE Humidity #2", 'scale'=>0.1, 'unit' =>"%RH"], // Humidity;
106 => [ 'idx'=>106, 'title'=>"BLE Humidity #3", 'scale'=>0.1, 'unit' =>"%RH"], // Humidity;
108 => [ 'idx'=>108, 'title'=>"BLE Humidity #4", 'scale'=>0.1, 'unit' =>"%RH"], // Humidity;
270 => [ 'idx'=>270, 'title'=>"BLE Fuel Level #1"], // Fuel Level;
273 => [ 'idx'=>273, 'title'=>"BLE Fuel Level #2"], // Fuel Level;
276 => [ 'idx'=>276, 'title'=>"BLE Fuel Level #3"], // Fuel Level;
279 => [ 'idx'=>279, 'title'=>"BLE Fuel Level #4"], // Fuel Level;
306 => [ 'idx'=>306, 'title'=>"BLE Fuel Frequency #1"], // Frequency value of BLE fuel sensor #1;
307 => [ 'idx'=>307, 'title'=>"BLE Fuel Frequency #2"], // Frequency value of BLE fuel sensor #2;
308 => [ 'idx'=>308, 'title'=>"BLE Fuel Frequency #3"], // Frequency value of BLE fuel sensor #3;
309 => [ 'idx'=>309, 'title'=>"BLE Fuel Frequency #4"], // Frequency value of BLE fuel sensor #4;
335 => [ 'idx'=>335, 'title'=>"BLE Luminosity #1", 'unit' =>"lx"], // Luminosity value of BLE sensor;
336 => [ 'idx'=>336, 'title'=>"BLE Luminosity #2", 'unit' =>"lx"], // Luminosity value of BLE sensor;
337 => [ 'idx'=>337, 'title'=>"BLE Luminosity #3", 'unit' =>"lx"], // Luminosity value of BLE sensor;
338 => [ 'idx'=>338, 'title'=>"BLE Luminosity #4", 'unit' =>"lx"], // Luminosity value of BLE sensor;
331 => [ 'idx'=>331, 'title'=>"BLE 1 Custom #1"], // Custom IO element for BLE sensor;
463 => [ 'idx'=>463, 'title'=>"BLE 1 Custom #2"], // Custom IO element for BLE sensor;
464 => [ 'idx'=>464, 'title'=>"BLE 1 Custom #3"], // Custom IO element for BLE sensor;
465 => [ 'idx'=>465, 'title'=>"BLE 1 Custom #4"], // Custom IO element for BLE sensor;
466 => [ 'idx'=>466, 'title'=>"BLE 1 Custom #5"], // Custom IO element for BLE sensor;
332 => [ 'idx'=>332, 'title'=>"BLE 2 Custom #1"], // Custom IO element for BLE sensor;
467 => [ 'idx'=>467, 'title'=>"BLE 2 Custom #2"], // Custom IO element for BLE sensor;
468 => [ 'idx'=>468, 'title'=>"BLE 2 Custom #3"], // Custom IO element for BLE sensor;
469 => [ 'idx'=>469, 'title'=>"BLE 2 Custom #4"], // Custom IO element for BLE sensor;
470 => [ 'idx'=>470, 'title'=>"BLE 2 Custom #5"], // Custom IO element for BLE sensor;
333 => [ 'idx'=>333, 'title'=>"BLE 3 Custom #1"], // Custom IO element for BLE sensor;
471 => [ 'idx'=>471, 'title'=>"BLE 3 Custom #2"], // Custom IO element for BLE sensor;
472 => [ 'idx'=>472, 'title'=>"BLE 3 Custom #3"], // Custom IO element for BLE sensor;
473 => [ 'idx'=>473, 'title'=>"BLE 3 Custom #4"], // Custom IO element for BLE sensor;
474 => [ 'idx'=>474, 'title'=>"BLE 3 Custom #5"], // Custom IO element for BLE sensor;
334 => [ 'idx'=>334, 'title'=>"BLE 4 Custom #1"], // Custom IO element for BLE sensor;
475 => [ 'idx'=>475, 'title'=>"BLE 4 Custom #2"], // Custom IO element for BLE sensor;
476 => [ 'idx'=>476, 'title'=>"BLE 4 Custom #3"], // Custom IO element for BLE sensor;
477 => [ 'idx'=>477, 'title'=>"BLE 4 Custom #4"], // Custom IO element for BLE sensor;
478 => [ 'idx'=>478, 'title'=>"BLE 4 Custom #5"], // Custom IO element for BLE sensor;
);

?>