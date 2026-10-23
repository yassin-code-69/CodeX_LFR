# LFR PRO V3.2 - সংশোধিত সম্পূর্ণ Firmware

এই প্যাকেজটি **তোমার সর্বশেষ GitHub-এ push না-করা `LFR_PRO_V3.rar`**-এর ১৪টি source file থেকে তৈরি। আগের working file ও GitHub repository পরিবর্তন করা হয়নি। এটি **host-tested reliability patch**, hardware-এ সফল run-এর নিশ্চয়তা নয়।

## ১. কীভাবে Arduino IDE-তে খুলবে

1. ZIP Extract করো।
2. Extract-এর ভেতরের **`LFR_PRO_V3`** folder-এ যাও।
3. **`LFR_PRO_V3.ino`** ফাইল Arduino IDE-তে Open করো। একই folder-এর সব `.cpp` ও `.h` ফাইল একসঙ্গে থাকতে হবে।
4. তোমার আগের মতো STMicroelectronics STM32 Board package, **STM32F103C8T6 Blue Pill** বোর্ড এবং সঠিক Upload Method সিলেক্ট করো।
5. **Verify/Compile** করো। Flash/RAM usage-এর Screenshot রাখো। এই পরিবেশে STM32 target compiler ছিল না, তাই PC test pass করলেও Flash Size এবং board compatibility এখনো নিশ্চিত নয়।

## ২. প্রথম Flash-এর আগে জরুরি সেটিং

`LFR_Config.h` ফাইলে:

```cpp
#define LFR_ALLOW_RACE_START 0
```

প্রথমবার **0**-ই রাখবে। এর অর্থ Race Start বন্ধ, কিন্তু **Motor Check এবং Auto-Calibration-এ মোটর চলতে পারে**। তাই চাকা মাটি থেকে তুলে এবং motor-power disconnect হাতের কাছে রেখে পরীক্ষা করবে।

সফলভাবে নিচের সব পরীক্ষা করার পরে:

```cpp
#define LFR_ALLOW_RACE_START 1
```

করবে এবং **আবার Verify + Flash** করবে। এর আগে Race চালু হবে না।

## ৩. তোমার ৫১০–৫৩০ গ্রাম Robot-এর জন্য পরীক্ষার শুরু

তোমার setup: STM32F103C8T6, ১৪-চ্যানেলের Analog MUX Sensor, BTS7960, ১২V/১০০০RPM 25GA দুটি মোটর ও OLED। মোট ওজন battery-সহ **৫১০–৫৩০ গ্রাম**।

- প্রথমে **SAFE** speed profile বেছে নাও, FAST বা SPRINT নয়।
- OLED থেকে **FINISH = OFF**, **GAP = RECOVER**, **NAV = STRAIGHT** দিয়ে শুরু করো।
- WHITE এবং BLACK calibration করো, ১৪টি sensor পরীক্ষা করো, `SAVE SETTINGS` দিয়ে reboot করে যাচাই করো।
- Left/Right motor direction, PB3 BACK/E-STOP বোতাম এবং 90-degree corner low speed-এ যাচাই করো।
- **১০টি ধারাবাহিক slow lap** ভালোভাবে complete হলে ধাপে ধাপে Technical/Fast tuning করো।
- ওজন থেকে একাই braking distance, RPM, static friction বা PID gains হিসাব করা যায় না। Wheel grip, track surface এবং battery voltage বাস্তবে মাপতে হবে।

## ৪. কী কী পরিবর্তন করা হয়েছে

- নতুন Race-এ পুরোনো **Turbo counter** আর থেকে যায় না।
- 90-degree corner-এর broad **single-blob pattern**-কে false junction ধরা কমানো হয়েছে।
- Gap-এর পর Junction বা Full-Black Marker এলে recovery করার logic যোগ হয়েছে।
- Full-Black **Start Pad** ও Finish Marker-এর handling আলাদা করা হয়েছে; blind-area timeouts আছে।
- **Finish ON** হলে প্রাথমিক line-stability, marker duration এবং Script Route completion যাচাই হয়। Finish OFF থাকলে auto finish হয় না।
- Alpha-Beta PID estimator option রেখেছি, কিন্তু default-এ **stable derivative filter** রেখেছি।
- Motor ramp ২০০Hz/৩৩৩Hz control rate-এ আগের মতো ভিন্ন রকম না হয়ে measured time অনুযায়ী adjust হয়।
- Deadband Compensation default OFF; চালু করলে PWM output `maxPWM`-এর বেশি হবে না।
- FOLLOW অবস্থায় untested reverse counter-torque default OFF; Pivot-এর reverse চালু আছে।
- OLED screen update-এর পর race time গণনা শুরু হয়।
- OLED, EEPROM settings format, button pins ও মূল UI-এর structure অক্ষত রেখেছি।

## ৫. সবচেয়ে গুরুত্বপূর্ণ সীমাবদ্ধতা

**কোনো জায়গা Full-Black Finish Box নাকি Full-Black Crossing, শুধু ১৪টি IR sensor-এর একই pattern থেকে শতভাগ আলাদা করা যায় না।** তাই আয়োজকের Track Rule নিশ্চিত না হওয়া পর্যন্ত **FINISH OFF** রাখবে। কিছু connected T Junction-ও corner-এর মতো দেখাতে পারে। এসব ক্ষেত্রে বাস্তব Sensor Replay/Track Test প্রয়োজন।

এই প্যাকেজ এখনও নিচের পরীক্ষায় পাস করেছে বলে দাবি করছি না:

- STM32 Blue Pill-এ বাস্তব compilation/linking/Flash-RAM size
- আসল মেঝে ও ৫১০–৫৩০ গ্রাম chassis-এ braking, grip ও top speed
- Fast ADC channel crosstalk ও sensor scan timing
- PB3 E-STOP-এর electrical test
- EEPROM power-loss interruption test
- দুই Competition-এর আসল track সম্পূর্ণ করা

## ৬. Test Report কোথায় পাবে

ZIP-এর মধ্যে `TEST_REPORT.txt`, `CHANGELOG.md`, `TECHNICAL_README.md`, `SOURCE_CHANGES.patch` এবং `tests/` আছে। `tests/` folder-এর দুইটি script PC-তে `g++` দিয়ে চালানো যায়:

```sh
sh run_tests.sh
sh run_v32_regressions.sh
```

**তোমার জন্য পরবর্তী কাজ:** Arduino IDE-তে `Verify` করে Flash/RAM result জানাও। এরপর motor wheel-off bench testing এবং ধাপে ধাপে physical track tuning-এ যাওয়া যাবে। মূল RAR অবশ্যই backup হিসেবে রাখবে।
