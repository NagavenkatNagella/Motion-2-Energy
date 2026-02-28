#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP_Mail_Client.h>

// ============================================================
//   CONFIGURATION  
// ============================================================

const char* ssid     = "Nani";
const char* password = "1234567 ";

#define SMTP_HOST        "smtp.gmail.com"
#define SMTP_PORT        465
#define AUTHOR_EMAIL     "napariyojana@gmail.com"
#define AUTHOR_PASSWORD  "gwbd naku rgvk xvrr"
#define RECIPIENT_EMAIL  "n.nagavenkat26@gmail.com"


const char* SHEETS_URL ="https://script.google.com/macros/s/AKfycbwSMAYhpriGbn_jFsSjzy6WSJjZEkvq7l4gN2VPKHsAqcrCuYyZnPcDpWragGluBUy2/exec";


// Log interval: 15 s — give heap time to settle between TLS calls
const unsigned long SHEETS_LOG_INTERVAL = 15000UL;

// ============================================================
//   HARDWARE
// ============================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTPIN   D4    
#define DHTTYPE  DHT11
DHT dht(DHTPIN, DHTTYPE);

#define TOUCH1  D5
#define TOUCH2  D6
#define TOUCH3  D7

// ============================================================
//   WEB SERVER & EMAIL
// ============================================================

ESP8266WebServer server(80);
SMTPSession      smtp;

// ============================================================
//   GPS LOCATIONS
// ============================================================

const int   NUM_TURBINES              = 3;
const float TURBINE_LAT[NUM_TURBINES] = {9.575062f, 9.575180f, 9.575310f};
const float TURBINE_LON[NUM_TURBINES] = {77.675734f, 77.675890f, 77.676050f};
const float DEFAULT_LAT               = 9.575062f;
const float DEFAULT_LON               = 77.675734f;

// ============================================================
//   GLOBAL STATE
// ============================================================

float        temperature    = 0.0f;
float        humidity       = 0.0f;
String       statusMessage  = "Initializing";
bool         threatDetected = false;
String       threatTurbines = "";
bool         emailSent      = false;
bool         sheetsLogged   = false;
int          sheetsLogCount = 0;

const unsigned long EMAIL_COOLDOWN       = 300000UL;
unsigned long       lastEmailTime        = 0UL;

const unsigned long SENSOR_READ_INTERVAL = 1000UL;
unsigned long       lastSensorReadTime   = 0UL;

const unsigned long LCD_UPDATE_INTERVAL  = 2000UL;
unsigned long       lastLCDUpdateTime    = 0UL;

unsigned long       lastSheetsLogTime    = 0UL;
bool                sheetsUrlOk          = false;   // set true if URL isn't placeholder

unsigned long startTime    = 0UL;
int           threatCount  = 0;
float         maxTemp      = -999.0f;
float         minTemp      =  999.0f;
float         avgTemp      = 0.0f;
long          tempReadings = 0;

// ============================================================
//   PROTOTYPES
// ============================================================

void   smtpCallback(SMTP_Status status);
void   sendEmailNotification(const String& st, const String& turb);
void   sendIPAddressEmail();
void   readSensorsAndCheckThreats();
void   updateLCD();
void   logToGoogleSheets();
String getUptimeString();
void   handleRoot();
void   handleData();
void   handleStats();

// ── chunk A: head + nav + hero ──────────────────────────────
static const char HTML_A[] PROGMEM = R"RAW(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>GreenFlow – VAWT Monitor</title>
<link href="https://fonts.googleapis.com/css2?family=DM+Sans:wght@400;600;700&family=DM+Mono:wght@400;500&display=swap" rel="stylesheet">
<style>
:root{--bg:#f0f7fb;--sf:#fff;--bd:#e0eef5;--br:#00b4d8;--brd:#0096c7;--brl:#e0f7fc;--tp:#0d2137;--ts:#5f8199;--tm:#9ab5c5;--gr:#00c896;--gl:#e0faf3;--rd:#f05252;--rl:#fdecea;--sh0:0 1px 3px rgba(0,70,120,.07);--sh1:0 4px 16px rgba(0,70,120,.1);--r:16px;--rs:10px}
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'DM Sans',sans-serif;background:var(--bg);color:var(--tp);min-height:100vh;font-size:15px}
nav{background:var(--sf);border-bottom:1px solid var(--bd);display:flex;align-items:center;justify-content:space-between;padding:0 32px;height:58px;position:sticky;top:0;z-index:100;box-shadow:var(--sh0)}
.nav-logo{display:flex;align-items:center;gap:8px;font-weight:700;font-size:17px;color:var(--br)}
.nav-links{display:flex;gap:4px;list-style:none}
.nav-links a{display:flex;align-items:center;gap:6px;padding:6px 13px;border-radius:8px;text-decoration:none;color:var(--ts);font-weight:500;font-size:13px;transition:all .2s}
.nav-links a:hover,.nav-links a.active{background:var(--brl);color:var(--brd)}
.live-badge{display:flex;align-items:center;gap:6px;background:var(--brl);color:var(--brd);border:1.5px solid var(--br);border-radius:20px;padding:5px 14px;font-weight:600;font-size:12px}
.ldot{width:7px;height:7px;background:var(--br);border-radius:50%;animation:lp 1.4s ease-in-out infinite}
@keyframes lp{0%,100%{opacity:1;transform:scale(1)}50%{opacity:.4;transform:scale(1.4)}}
.hero{display:grid;grid-template-columns:1fr 380px;gap:32px;align-items:center;padding:48px 32px;max-width:1200px;margin:0 auto}
.hero h1{font-size:40px;font-weight:700;color:var(--br);line-height:1.15;margin-bottom:14px}
.hero p{color:var(--ts);font-size:15px;line-height:1.7;max-width:440px;margin-bottom:28px}
.hero-btns{display:flex;gap:10px}
.btn-p{background:var(--br);color:#fff;border:none;padding:12px 24px;border-radius:9px;font-family:'DM Sans',sans-serif;font-size:14px;font-weight:600;cursor:pointer;transition:background .2s,transform .15s}
.btn-p:hover{background:var(--brd);transform:translateY(-1px)}
.btn-s{background:var(--sf);color:var(--tp);border:1.5px solid var(--bd);padding:12px 24px;border-radius:9px;font-family:'DM Sans',sans-serif;font-size:14px;font-weight:600;cursor:pointer;transition:all .2s}
.btn-s:hover{border-color:var(--br);color:var(--br)}
.tc{background:var(--sf);border:1px solid var(--bd);border-radius:var(--r);height:260px;display:flex;align-items:center;justify-content:center;box-shadow:var(--sh1);position:relative;overflow:hidden}
.tc::before{content:'';position:absolute;inset:0;background:radial-gradient(circle at 60% 40%,rgba(0,180,216,.06),transparent 70%)}
.turb{width:140px;height:140px;position:relative}
.tctr{width:28px;height:28px;background:linear-gradient(135deg,#e0f7fc,#00b4d8);border-radius:50%;position:absolute;top:56px;left:56px;z-index:2;box-shadow:0 0 18px rgba(0,180,216,.5);animation:cp 2s ease-in-out infinite}
.tb{position:absolute;width:110px;height:18px;background:linear-gradient(90deg,rgba(0,180,216,.9),rgba(0,150,199,.7));top:61px;left:15px;transform-origin:55px 9px;border-radius:9px}
.tb:nth-child(1){animation:sf 3s linear infinite}
.tb:nth-child(2){transform:rotate(120deg);animation:sf 3s linear infinite}
.tb:nth-child(3){transform:rotate(240deg);animation:sf 3s linear infinite}
.tp{width:12px;height:90px;background:linear-gradient(to bottom,#b0cdd9,#8aa8b5);position:absolute;bottom:-56px;left:64px;border-radius:6px 6px 0 0}
@keyframes sf{0%{transform:rotate(0)}100%{transform:rotate(360deg)}}
@keyframes cp{0%,100%{box-shadow:0 0 18px rgba(0,180,216,.5)}50%{box-shadow:0 0 32px rgba(0,180,216,.8)}}
.main{max-width:1200px;margin:0 auto;padding:0 32px 56px}
.sg{display:grid;grid-template-columns:repeat(5,1fr);gap:18px;margin-bottom:24px}
.sc{background:var(--sf);border:1px solid var(--bd);border-radius:var(--r);padding:22px;box-shadow:var(--sh0);transition:box-shadow .2s,transform .2s}
.sc:hover{box-shadow:var(--sh1);transform:translateY(-2px)}
.sc.ta{border-color:var(--rd);background:var(--rl);animation:tap 1s ease-in-out infinite}
@keyframes tap{0%,100%{box-shadow:0 0 0 0 rgba(240,82,82,.3)}50%{box-shadow:0 0 0 7px rgba(240,82,82,0)}}
.sh{display:flex;align-items:center;justify-content:space-between;margin-bottom:14px}
.sl{font-size:11px;font-weight:600;letter-spacing:.8px;text-transform:uppercase;color:var(--tm)}
.si{width:34px;height:34px;border-radius:9px;display:flex;align-items:center;justify-content:center}
.si.b{background:var(--brl);color:var(--br)}.si.g{background:var(--gl);color:var(--gr)}.si.r{background:var(--rl);color:var(--rd)}
.sv{font-size:30px;font-weight:700;color:var(--tp);margin-bottom:5px;font-family:'DM Mono',monospace}
.ss{font-size:12px;color:var(--tm)}.ss.up{color:var(--gr)}.ss.dn{color:var(--rd)}
.bg{display:grid;grid-template-columns:1fr 300px;gap:18px;margin-bottom:24px}
.cc{background:var(--sf);border:1px solid var(--bd);border-radius:var(--r);padding:26px;box-shadow:var(--sh0)}
.ch{display:flex;align-items:center;justify-content:space-between;margin-bottom:20px}
.ct{font-size:12px;font-weight:600;letter-spacing:.6px;text-transform:uppercase;color:var(--ts)}
.ca{width:30px;height:30px;border:1px solid var(--bd);border-radius:7px;display:flex;align-items:center;justify-content:center;color:var(--tm);cursor:pointer;transition:all .2s}
.ca:hover{border-color:var(--br);color:var(--br)}
.chart-area{position:relative;height:210px}
.cl{display:flex;gap:18px;margin-top:12px;justify-content:center}
.li{display:flex;align-items:center;gap:6px;font-size:11px;color:var(--ts);font-weight:500}
.ld{width:26px;height:3px;border-radius:2px}
.ic{background:var(--sf);border:1px solid var(--bd);border-radius:var(--r);padding:26px;box-shadow:var(--sh0)}
.ir{display:flex;justify-content:space-between;align-items:center;padding:12px 0;border-bottom:1px solid var(--bd)}
.ir:last-of-type{border-bottom:none}
.il{font-size:13px;color:var(--ts);font-weight:500}
.iv{font-size:13px;color:var(--tp);font-weight:600;font-family:'DM Mono',monospace}
.lb{width:100%;margin-top:18px;background:var(--brl);color:var(--brd);border:1.5px solid var(--br);border-radius:var(--rs);padding:11px;font-family:'DM Sans',sans-serif;font-weight:600;font-size:13px;cursor:pointer;display:flex;align-items:center;justify-content:center;gap:7px;transition:all .2s}
.lb:hover{background:var(--br);color:#fff}
.sb{width:100%;margin-top:8px;background:#e8f5e9;color:#2e7d32;border:1.5px solid #66bb6a;border-radius:var(--rs);padding:11px;font-family:'DM Sans',sans-serif;font-weight:600;font-size:13px;cursor:pointer;display:flex;align-items:center;justify-content:center;gap:7px;transition:all .2s;text-decoration:none}
.sb:hover{background:#66bb6a;color:#fff}
.cb{display:flex;align-items:center;gap:7px;padding:7px 14px;border-radius:7px;margin-bottom:18px;font-size:13px;font-weight:600;transition:all .4s}
.cb.ok{background:var(--gl);color:var(--gr);border:1px solid rgba(0,200,150,.3)}
.cb.err{background:var(--rl);color:var(--rd);border:1px solid rgba(240,82,82,.3)}
.cd{width:7px;height:7px;border-radius:50%;background:currentColor}
.lc{background:var(--sf);border:1px solid var(--bd);border-radius:var(--r);padding:26px;box-shadow:var(--sh0);margin-bottom:24px}
table{width:100%;border-collapse:collapse;margin-top:4px}
th{text-align:left;font-size:11px;font-weight:600;text-transform:uppercase;letter-spacing:.6px;color:var(--tm);padding:11px 14px;border-bottom:2px solid var(--bd)}
td{padding:13px 14px;font-size:12px;color:var(--ts);border-bottom:1px solid var(--bd);font-family:'DM Mono',monospace}
tr:last-child td{border-bottom:none}
tr:hover td{background:var(--bg)}
.pill{display:inline-flex;align-items:center;gap:5px;padding:3px 11px;border-radius:20px;font-size:11px;font-weight:600;font-family:'DM Sans',sans-serif}
.pill.ok{background:var(--gl);color:var(--gr)}.pill.th{background:var(--rl);color:var(--rd)}
.nd td{text-align:center;color:var(--tm);padding:36px;font-family:'DM Sans',sans-serif;font-style:italic}
footer{background:var(--sf);border-top:1px solid var(--bd);padding:40px 32px}
.fi{max-width:1200px;margin:0 auto;display:grid;grid-template-columns:1.5fr 1fr 1fr;gap:36px}
.fb p{color:var(--tm);font-size:13px;line-height:1.8;max-width:240px;margin-top:10px}
.fs h4{font-size:12px;font-weight:700;margin-bottom:14px;color:var(--tp)}
.fs ul{list-style:none}.fs li{margin-bottom:9px}
.fs a{color:var(--tm);text-decoration:none;font-size:12px;transition:color .2s}.fs a:hover{color:var(--br)}
.ov{display:none;position:fixed;inset:0;background:rgba(0,0,0,.6);z-index:1000;backdrop-filter:blur(4px);animation:fi .3s ease}
.pp{position:absolute;top:50%;left:50%;transform:translate(-50%,-50%);width:90%;max-width:460px;background:var(--sf);border-radius:18px;padding:32px;box-shadow:0 20px 60px rgba(0,0,0,.3);border-top:4px solid var(--rd);animation:pb .4s ease}
.pi{text-align:center;font-size:52px;margin-bottom:10px}
.pt{text-align:center;font-size:20px;font-weight:700;color:var(--rd);margin-bottom:20px}
.pi2{background:var(--bg);border-radius:11px;padding:18px;margin-bottom:18px}
.pr{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid var(--bd);font-size:13px}
.pr:last-child{border-bottom:none}
.prl{color:var(--tm);font-weight:600;text-transform:uppercase;font-size:10px;letter-spacing:.6px}
.prv{color:var(--tp);font-weight:600}
.ab{width:100%;background:var(--rd);color:#fff;border:none;border-radius:9px;padding:14px;font-family:'DM Sans',sans-serif;font-size:14px;font-weight:700;cursor:pointer;transition:all .2s}
.ab:hover{background:#d63a3a;transform:translateY(-1px)}
.et{position:fixed;top:22px;right:22px;background:var(--gr);color:#fff;padding:14px 20px;border-radius:11px;z-index:2000;display:none;min-width:260px;box-shadow:0 8px 24px rgba(0,200,150,.3);animation:si .4s ease}
.et.show{display:block}
.et-t{font-weight:700;font-size:13px;margin-bottom:3px}
.et-m{font-size:11px;opacity:.9}
.et-x{position:absolute;top:9px;right:12px;background:none;border:none;color:#fff;font-size:17px;cursor:pointer;opacity:.7}
.st{position:fixed;top:80px;right:22px;background:#2e7d32;color:#fff;padding:12px 18px;border-radius:11px;z-index:2000;display:none;min-width:220px;box-shadow:0 8px 24px rgba(46,125,50,.3);animation:si .4s ease;font-size:12px;font-weight:600}
.st.show{display:block}
@keyframes fi{from{opacity:0}to{opacity:1}}
@keyframes si{from{transform:translateX(120%);opacity:0}to{transform:translateX(0);opacity:1}}
@keyframes pb{0%{transform:translate(-50%,-50%) scale(.8)}60%{transform:translate(-50%,-50%) scale(1.02)}100%{transform:translate(-50%,-50%) scale(1)}}
@media(max-width:1024px){.sg{grid-template-columns:repeat(3,1fr)}.bg{grid-template-columns:1fr}.hero{grid-template-columns:1fr}.tc{display:none}}
@media(max-width:600px){nav{padding:0 14px}.nav-links{display:none}.hero{padding:26px 14px}.hero h1{font-size:28px}.main{padding:0 14px 36px}.sg{grid-template-columns:1fr 1fr}footer{padding:26px 14px}.fi{grid-template-columns:1fr}}
svg{flex-shrink:0}
</style>
</head>
<body>
<nav>
  <div class="nav-logo">
    <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2L8 8H4l4 6H5l7 8 7-8h-3l4-6h-4L12 2z"/></svg>
    GreenFlow
  </div>
  <ul class="nav-links">
    <li><a href="#" class="active"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/><rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/></svg>Dashboard</a></li>
    <li><a href="#"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>Analytics</a></li>
    <li><a href="#"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>Logs</a></li>
  </ul>
  <div class="live-badge"><div class="ldot"></div>Live</div>
</nav>
<div class="hero">
  <div>
    <h1>Smart Turbine<br>Monitoring</h1>
    <p>Real-time analytics for Vertical Axis Wind Turbines with live Google Sheets logging.</p>
    <div class="hero-btns">
      <button class="btn-p" onclick="document.querySelector('.main').scrollIntoView({behavior:'smooth'})">Dashboard</button>
      <button class="btn-s">View Manual</button>
    </div>
  </div>
  <div class="tc">
    <div class="turb">
      <div class="tb"></div><div class="tb"></div><div class="tb"></div>
      <div class="tctr"></div><div class="tp"></div>
    </div>
  </div>
</div>
)RAW";

// ── chunk B: dashboard cards + chart + info panel ───────────
static const char HTML_B[] PROGMEM = R"RAW(
<div class="main">
<div class="cb ok" id="connBar"><div class="cd"></div><span id="connText">Connected – receiving live sensor data</span></div>
<div class="sg">
  <div class="sc" id="statusCard">
    <div class="sh"><span class="sl">System Status</span><div class="si b"><svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg></div></div>
    <div class="sv" id="statusValue" style="font-size:22px;font-family:'DM Sans',sans-serif">--</div>
    <div class="ss up" id="statusSub">Waiting...</div>
  </div>
  <div class="sc">
    <div class="sh"><span class="sl">Temperature</span><div class="si b"><svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z"/></svg></div></div>
    <div class="sv"><span id="tempValue">--</span>°C</div>
    <div class="ss" id="tempSub">DHT11</div>
  </div>
  <div class="sc">
    <div class="sh"><span class="sl">Humidity</span><div class="si b"><svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2.69l5.66 5.66a8 8 0 1 1-11.31 0z"/></svg></div></div>
    <div class="sv"><span id="humidValue">--</span>%</div>
    <div class="ss">DHT11</div>
  </div>
  <div class="sc">
    <div class="sh"><span class="sl">Threats</span><div class="si r"><svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg></div></div>
    <div class="sv" id="threatCount">0</div>
    <div class="ss">Session total</div>
  </div>
  <div class="sc">
    <div class="sh"><span class="sl">Sheets Rows</span><div class="si g"><svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="18" height="18" rx="2"/><line x1="3" y1="9" x2="21" y2="9"/><line x1="3" y1="15" x2="21" y2="15"/><line x1="9" y1="3" x2="9" y2="21"/></svg></div></div>
    <div class="sv" id="sheetsCount">0</div>
    <div class="ss" id="sheetsSub">Logged total</div>
  </div>
</div>
<div class="bg">
  <div class="cc">
    <div class="ch"><span class="ct">Live Metrics</span></div>
    <div class="chart-area"><canvas id="liveChart"></canvas></div>
    <div class="cl">
      <div class="li"><div class="ld" style="background:#00b4d8"></div>Temp °C</div>
      <div class="li"><div class="ld" style="background:#00c896"></div>Humidity %</div>
    </div>
  </div>
  <div class="ic">
    <div class="ch"><span class="ct">Uptime &amp; Info</span></div>
    <div class="ir"><span class="il">Uptime</span><span class="iv" id="uptime">--</span></div>
    <div class="ir"><span class="il">Avg Temp</span><span class="iv" id="avgTemp">--</span></div>
    <div class="ir"><span class="il">Max Temp</span><span class="iv" id="maxTemp">--</span></div>
    <div class="ir"><span class="il">Min Temp</span><span class="iv" id="minTemp">--</span></div>
    <div class="ir"><span class="il">Device IP</span><span class="iv" id="deviceIP">--</span></div>
    <div class="ir"><span class="il">Heap Free</span><span class="iv" id="heapFree">--</span></div>
    <button class="lb" onclick="openMaps()">
      <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z"/><circle cx="12" cy="10" r="3"/></svg>
      Locate Turbine
    </button>
    <a id="openSheetsBtn" class="sb" href="https://docs.google.com/spreadsheets/d/1vI1hhfSh3Jy1U-QVmlMLhaWtvdsBknOMou_Hcs5Zt5Q/edit?gid=0#gid=0" target="_blank">
      <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="18" height="18" rx="2"/><line x1="3" y1="9" x2="21" y2="9"/><line x1="3" y1="15" x2="21" y2="15"/><line x1="9" y1="3" x2="9" y2="21"/></svg>
      Open Sheets
    </a>
  </div>
</div>
<div class="lc">
  <div class="ch">
    <span class="ct">Event Log</span>
    <div class="ca" onclick="clearLog()" title="Clear"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="3 6 5 6 21 6"/><path d="M19 6l-1 14H6L5 6"/><path d="M10 11v6"/><path d="M14 11v6"/><path d="M9 6V4h6v2"/></svg></div>
  </div>
  <table><thead><tr><th>Time</th><th>Event</th><th>Turbine(s)</th><th>Temp</th><th>Humid</th><th>Status</th></tr></thead>
  <tbody id="logBody"><tr class="nd"><td colspan="6">No events yet – waiting for sensor data</td></tr></tbody></table>
</div>
</div>
)RAW";

// ── chunk C: footer + popups + scripts ──────────────────────
static const char HTML_C[] PROGMEM = R"RAW(
<footer>
  <div class="fi">
    <div class="fb">
      <div class="nav-logo" style="font-size:17px">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2L8 8H4l4 6H5l7 8 7-8h-3l4-6h-4L12 2z"/></svg>
        GreenFlow
      </div>
      <p>Sustainable energy monitoring for Vertical Axis Wind Turbines.</p>
    </div>
    <div class="fs"><h4>Quick Links</h4><ul><li><a href="#">Dashboard</a></li><li><a href="#">Analytics</a></li><li><a href="#">Logs</a></li></ul></div>
    <div class="fs"><h4>Support</h4><ul><li><a href="#">Docs</a></li><li><a href="#">Contact</a></li><li><a href="#">Privacy</a></li></ul></div>
  </div>
</footer>

<div class="ov" id="threatOverlay">
  <div class="pp">
    <div class="pi">⚠️</div>
    <div class="pt">Threat Detected!</div>
    <div class="pi2">
      <div class="pr"><span class="prl">Status</span><span class="prv" id="popupStatus">--</span></div>
      <div class="pr"><span class="prl">Turbine(s)</span><span class="prv" id="popupTurbines">--</span></div>
      <div class="pr"><span class="prl">Temperature</span><span class="prv" id="popupTemp">--</span></div>
      <div class="pr"><span class="prl">Humidity</span><span class="prv" id="popupHumid">--</span></div>
      <div class="pr"><span class="prl">Location</span><span class="prv" id="popupLoc" style="cursor:pointer;color:#00b4d8;text-decoration:underline" onclick="openMaps()">--</span></div>
      <div class="pr"><span class="prl">Time</span><span class="prv" id="popupTime">--</span></div>
    </div>
    <button class="ab" onclick="acknowledgeAlert()">✓ Acknowledge</button>
  </div>
</div>

<div class="et" id="emailToast">
  <button class="et-x" onclick="closeET()">×</button>
  <div class="et-t">✉ Email Alert Sent!</div>
  <div class="et-m" id="emailMsg">--</div>
</div>

<div class="st" id="sheetsToast">📊 Row logged to Google Sheets</div>

<script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/4.4.0/chart.umd.min.js"></script>
<script>
var MAX_PTS=20,labels=[],tData=[],hData=[];
var chart=new Chart(document.getElementById('liveChart').getContext('2d'),{
  type:'line',
  data:{labels:labels,datasets:[
    {label:'Temp',data:tData,borderColor:'#00b4d8',backgroundColor:'rgba(0,180,216,.07)',borderWidth:2,pointRadius:2,tension:.4,fill:true},
    {label:'Humid',data:hData,borderColor:'#00c896',backgroundColor:'rgba(0,200,150,.07)',borderWidth:2,pointRadius:2,tension:.4,fill:true}
  ]},
  options:{responsive:true,maintainAspectRatio:false,animation:{duration:300},
    plugins:{legend:{display:false},tooltip:{backgroundColor:'#fff',titleColor:'#0d2137',bodyColor:'#5f8199',borderColor:'#e0eef5',borderWidth:1,padding:9,cornerRadius:7}},
    scales:{
      x:{ticks:{color:'#9ab5c5',font:{size:9,family:'DM Mono'},maxRotation:0,maxTicksLimit:7},grid:{color:'#f0f7fb'},border:{display:false}},
      y:{ticks:{color:'#9ab5c5',font:{size:9,family:'DM Mono'}},grid:{color:'#f0f7fb'},border:{display:false},min:0,max:100}
    }
  }
});

var evHist=[],lastThr=false,curLat=9.575062,curLon=77.675734;
var etTimer=null,stTimer=null,fetchErr=0,MAX_ERR=5;
document.getElementById('deviceIP').textContent=window.location.hostname||'--';

// Poll /data every 2 s, /stats every 5 s  (reduced from 1 s — fixes lag)
var statTick=0;
function poll(){
  fetch('/data',{cache:'no-store'})
    .then(function(r){if(!r.ok)throw 0;return r.json()})
    .then(function(d){fetchErr=0;setConn(true);onData(d)})
    .catch(function(){fetchErr++;if(fetchErr>=MAX_ERR)setConn(false)});
  statTick++;
  if(statTick>=3){  // every ~6 s
    statTick=0;
    fetch('/stats',{cache:'no-store'})
      .then(function(r){return r.json()})
      .then(function(s){
        document.getElementById('uptime').textContent=s.uptime||'--';
        document.getElementById('threatCount').textContent=s.threats!==undefined?s.threats:'--';
        document.getElementById('maxTemp').textContent=s.maxTemp!==undefined?s.maxTemp.toFixed(1)+'°C':'--';
        document.getElementById('minTemp').textContent=s.minTemp!==undefined?s.minTemp.toFixed(1)+'°C':'--';
        document.getElementById('avgTemp').textContent=s.avgTemp!==undefined?s.avgTemp.toFixed(1)+'°C':'--';
        document.getElementById('heapFree').textContent=s.heap!==undefined?(s.heap/1024).toFixed(1)+' KB':'--';
        if(s.sheetsCount!==undefined)document.getElementById('sheetsCount').textContent=s.sheetsCount;
      }).catch(function(){});
  }
}

function onData(d){
  var sEl=document.getElementById('statusValue'),sCard=document.getElementById('statusCard'),sSub=document.getElementById('statusSub');
  sEl.textContent=d.status||'Unknown';
  if(d.threat){
    sEl.style.color='var(--rd)';sCard.classList.add('ta');
    sSub.className='ss dn';sSub.innerHTML='&#9888; Threat active';
  }else{
    sEl.style.color='var(--tp)';sCard.classList.remove('ta');
    sSub.className='ss up';sSub.innerHTML='&#10003; Normal';
  }
  var t=parseFloat(d.temp),h=parseFloat(d.humid);
  document.getElementById('tempValue').textContent=isNaN(t)?'--':t.toFixed(1);
  document.getElementById('humidValue').textContent=isNaN(h)?'--':h.toFixed(1);
  var ts=document.getElementById('tempSub');
  if(!isNaN(t)){if(t>40){ts.className='ss dn';ts.textContent='High!';}else if(t<10){ts.className='ss';ts.textContent='Low';}else{ts.className='ss up';ts.textContent='Optimal';}}
  if(d.lat)curLat=d.lat;if(d.lon)curLon=d.lon;
  if(d.emailSent)showET(d.turbines);
  if(d.sheetsLogged)showST();
  if(d.threat&&!lastThr){showPop(d.status,d.turbines,t,h);addLog(d.status,d.turbines||'--',t,h,true);}
  else if(!d.threat&&lastThr){addLog('Normal','--',t,h,false);}
  lastThr=d.threat;
  if(!isNaN(t)&&!isNaN(h)){
    var lbl=new Date().toLocaleTimeString('en-US',{hour12:false,hour:'2-digit',minute:'2-digit',second:'2-digit'});
    labels.push(lbl);tData.push(t);hData.push(h);
    if(labels.length>MAX_PTS){labels.shift();tData.shift();hData.shift();}
    chart.update('active');
  }
}
function setConn(ok){
  var b=document.getElementById('connBar'),tx=document.getElementById('connText');
  b.className='cb '+(ok?'ok':'err');
  tx.textContent=ok?'Connected – live data':'Connection lost – retrying…';
}
function showPop(st,turb,t,h){
  document.getElementById('popupStatus').textContent=st||'--';
  document.getElementById('popupTurbines').textContent=turb||'--';
  document.getElementById('popupTemp').textContent=isNaN(t)?'--':t.toFixed(1)+'°C';
  document.getElementById('popupHumid').textContent=isNaN(h)?'--':h.toFixed(1)+'%';
  document.getElementById('popupLoc').textContent=curLat.toFixed(6)+', '+curLon.toFixed(6);
  document.getElementById('popupTime').textContent=new Date().toLocaleString();
  document.getElementById('threatOverlay').style.display='block';
}
function acknowledgeAlert(){document.getElementById('threatOverlay').style.display='none';}
function openMaps(){window.open('https://maps.google.com/?q='+curLat+','+curLon,'_blank');}
function showET(turb){
  var t=document.getElementById('emailToast');
  document.getElementById('emailMsg').textContent='Alert sent for Turbine(s): '+(turb||'--');
  t.classList.add('show');if(etTimer)clearTimeout(etTimer);
  etTimer=setTimeout(function(){t.classList.remove('show');},6000);
}
function closeET(){document.getElementById('emailToast').classList.remove('show');}
function showST(){
  var t=document.getElementById('sheetsToast');
  document.getElementById('sheetsSub').textContent='Last: '+new Date().toLocaleTimeString();
  t.classList.add('show');if(stTimer)clearTimeout(stTimer);
  stTimer=setTimeout(function(){t.classList.remove('show');},3000);
}
function addLog(st,turb,t,h,isThr){
  evHist.unshift({time:new Date().toLocaleString(),status:st,turbines:turb,
    temp:isNaN(t)?'--':t.toFixed(1)+'°C',humid:isNaN(h)?'--':h.toFixed(1)+'%',isThr:isThr});
  if(evHist.length>50)evHist.pop();
  renderLog();
}
function renderLog(){
  var tb=document.getElementById('logBody');
  if(!evHist.length){tb.innerHTML='<tr class="nd"><td colspan="6">No events yet</td></tr>';return;}
  tb.innerHTML=evHist.map(function(x){
    return '<tr><td>'+x.time+'</td><td>'+x.status+'</td><td>'+x.turbines+'</td><td>'+x.temp+'</td><td>'+x.humid+'</td><td><span class="pill '+(x.isThr?'th':'ok')+'">'+(x.isThr?'⚠ Threat':'✓ Normal')+'</span></td></tr>';
  }).join('');
}
function clearLog(){evHist=[];renderLog();}
setInterval(poll,2000);
poll();
</script>
</body></html>
)RAW";

// ============================================================
//   SMTP CALLBACK
// ============================================================

void smtpCallback(SMTP_Status status) {
    if (status.success()) Serial.println(F("[EMAIL] Sent OK."));
    else Serial.println("[EMAIL] Error: " + String(status.info()));
}

// ============================================================
//   UPTIME STRING
// ============================================================

String getUptimeString() {
    unsigned long s = (millis() - startTime) / 1000UL;
    return String(s/3600) + "h " + String((s%3600)/60) + "m " + String(s%60) + "s";
}

// ============================================================

void logToGoogleSheets() {
    if (!sheetsUrlOk) {
        Serial.println(F("[SHEETS] Skipped — set SHEETS_URL and re-flash"));
        return;
    }
    if (WiFi.status() != WL_CONNECTED) return;

    uint32_t maxBlk = ESP.getMaxFreeBlockSize();
    Serial.printf("[SHEETS] max-block %u bytes\n", maxBlk);
    if (maxBlk < 18000) {
        Serial.printf("[SHEETS] Heap too low (%u) — retry later\n", maxBlk);
        return;
    }

    // ── Parse host + path from SHEETS_URL ──────────────────
    String url  = String(SHEETS_URL);
    int hs      = url.indexOf("://") + 3;
    int ps      = url.indexOf('/', hs);
    String host = url.substring(hs, ps);
    String path = url.substring(ps);

    // ── Build query string on the stack (no heap alloc) ────
    char qs[320];
    snprintf(qs, sizeof(qs),
        "?temp=%.1f&humid=%.1f&status=%s&threat=%s"
        "&turbines=%s&emailSent=%s"
        "&uptime=%s&lat=%.6f&lon=%.6f",
        temperature, humidity,
        statusMessage.c_str(),
        threatDetected ? "true" : "false",
        threatTurbines.length() > 0 ? threatTurbines.c_str() : "--",
        emailSent ? "true" : "false",
        getUptimeString().c_str(),
        DEFAULT_LAT, DEFAULT_LON
    );
    for (int i = 0; qs[i]; i++) if (qs[i] == ' ') qs[i] = '+';

    Serial.printf("[SHEETS] GET %s\n", host.c_str());

    // ── One raw TLS connection ──────────────────────────────
    {
        BearSSL::WiFiClientSecure client;
        client.setInsecure();
        client.setTimeout(15000);

        if (!client.connect(host.c_str(), 443)) {
            Serial.println(F("[SHEETS] Connection failed"));
            return;
        }

        // Send GET — path + query as char arrays (no String heap alloc)
        client.print(F("GET "));
        client.print(path);     
        client.print(qs);       
        client.println(F(" HTTP/1.0"));
        client.print(F("Host: "));
        client.println(host);
        client.println(F("User-Agent: ESP8266"));
        client.println(F("Connection: close"));
        client.println();       // blank line = end of request

        // ── Wait for response ──
        unsigned long t0 = millis();
        while (!client.available() && (millis() - t0) < 15000) {
            delay(5); yield();
        }

        if (!client.available()) {
            Serial.println(F("[SHEETS] Timeout — no response"));
            client.stop();
            return;
        }

        // Read only status line to save RAM
        String sl = client.readStringUntil('\n');
        sl.trim();
        client.stop();  // close immediately — reclaim TLS heap

        Serial.println("[SHEETS] Response: " + sl);

        // HtmlService returns 200 directly — no redirect needed
        if (sl.indexOf("200") >= 0) {
            sheetsLogged = true;
            sheetsLogCount++;
            Serial.printf("[SHEETS] Row %d logged OK\n", sheetsLogCount);
        } else if (sl.indexOf("302") >= 0) {
            Serial.println(F("[SHEETS] Got 302 — you are using old Apps Script!"));
            Serial.println(F("[SHEETS] Deploy GoogleAppsScript_VAWT_v3.gs and re-deploy."));
        } else {
            Serial.println(F("[SHEETS] Unexpected response"));
        }
    }

    yield();
    Serial.printf("[SHEETS] Heap after: %u free\n", ESP.getFreeHeap());
}

// ============================================================
//   SEND IP EMAIL  (boot notification)
// ============================================================

void sendIPAddressEmail() {
    smtp.closeSession();
    smtp.debug(0);

    Session_Config cfg;
    cfg.server.host_name  = SMTP_HOST;
    cfg.server.port       = SMTP_PORT;
    cfg.login.email       = AUTHOR_EMAIL;
    cfg.login.password    = AUTHOR_PASSWORD;
    cfg.login.user_domain = F("");
    cfg.secure.startTLS   = false;

    smtp.callback(smtpCallback);

    SMTP_Message msg;
    msg.sender.name  = F("VAWT System");
    msg.sender.email = AUTHOR_EMAIL;
    msg.subject      = F("VAWT System Started");
    msg.addRecipient(F("Admin"), RECIPIENT_EMAIL);

    String body = "VAWT System Online\n\n";
    body += "IP        : " + WiFi.localIP().toString() + "\n";
    body += "Dashboard : http://" + WiFi.localIP().toString() + "/\n";
    body += "Location  : " + String(DEFAULT_LAT,6) + ", " + String(DEFAULT_LON,6) + "\n";
    msg.text.content = body.c_str();

    if (!smtp.connect(&cfg)) { Serial.println(F("[EMAIL] Connect failed")); smtp.closeSession(); return; }
    if (!MailClient.sendMail(&smtp, &msg)) Serial.println(F("[EMAIL] Send failed"));
    smtp.closeSession();
}

// ============================================================
//   SEND THREAT EMAIL
// ============================================================

void sendEmailNotification(const String& st, const String& turb) {
    smtp.closeSession();
    smtp.debug(0);

    Session_Config cfg;
    cfg.server.host_name  = SMTP_HOST;
    cfg.server.port       = SMTP_PORT;
    cfg.login.email       = AUTHOR_EMAIL;
    cfg.login.password    = AUTHOR_PASSWORD;
    cfg.login.user_domain = F("");
    cfg.secure.startTLS   = false;

    smtp.callback(smtpCallback);

    SMTP_Message msg;
    msg.sender.name  = F("VAWT Alert");
    msg.sender.email = AUTHOR_EMAIL;
    msg.subject      = "ALERT: Threat at T" + turb;
    msg.addRecipient(F("Admin"), RECIPIENT_EMAIL);

    String body = "VAWT THREAT ALERT\n";
    body += "========================\n\n";
    body += "Status   : " + st + "\n";
    body += "Turbine  : " + turb + "\n";
    body += "Temp     : " + String(temperature,1) + " C\n";
    body += "Humidity : " + String(humidity,1) + " %\n";
    body += "Uptime   : " + getUptimeString() + "\n\n";

    bool s[3] = {digitalRead(TOUCH1)==HIGH, digitalRead(TOUCH2)==HIGH, digitalRead(TOUCH3)==HIGH};
    for (int i=0; i<NUM_TURBINES; i++) {
        if (s[i]) {
            body += "Turbine " + String(i+1) + ": https://maps.google.com/?q=";
            body += String(TURBINE_LAT[i],6) + "," + String(TURBINE_LON[i],6) + "\n";
        }
    }
    body += "\nDashboard: http://" + WiFi.localIP().toString() + "/\n";
    msg.text.content = body.c_str();

    if (!smtp.connect(&cfg)) { Serial.println(F("[EMAIL] Connect failed")); smtp.closeSession(); return; }
    if (!MailClient.sendMail(&smtp, &msg)) Serial.println(F("[EMAIL] Send failed"));
    else Serial.println(F("[EMAIL] Alert sent."));
    smtp.closeSession();
}

// ============================================================
//   WEB HANDLERS
// ============================================================

void handleRoot() {
    // Chunked transfer — send 3 PROGMEM chunks without building
    // a giant String in heap (saves ~6-8 KB peak allocation)
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, F("text/html"), "");
    server.sendContent_P(HTML_A);
    yield();
    server.sendContent_P(HTML_B);
    yield();
    server.sendContent_P(HTML_C);
    server.sendContent("");   // end chunked transfer
}

void handleData() {
    // Minimal JSON — no heap-heavy String concatenation
    char buf[220];
    snprintf(buf, sizeof(buf),
        "{\"status\":\"%s\",\"temp\":%.1f,\"humid\":%.1f,"
        "\"threat\":%s,\"turbines\":\"%s\","
        "\"emailSent\":%s,\"sheetsLogged\":%s,"
        "\"lat\":%.6f,\"lon\":%.6f}",
        statusMessage.c_str(),
        temperature, humidity,
        threatDetected ? "true" : "false",
        threatTurbines.c_str(),
        emailSent     ? "true" : "false",
        sheetsLogged  ? "true" : "false",
        DEFAULT_LAT, DEFAULT_LON
    );

    server.sendHeader(F("Cache-Control"), F("no-cache,no-store"));
    server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    server.send(200, F("application/json"), buf);

    if (emailSent)   emailSent   = false;
    if (sheetsLogged) sheetsLogged = false;
}

void handleStats() {
    float safeMax = (tempReadings>0) ? maxTemp : 0.0f;
    float safeMin = (tempReadings>0) ? minTemp : 0.0f;
    float safeAvg = (tempReadings>0) ? avgTemp : 0.0f;
    uint32_t heap = ESP.getFreeHeap();

    char buf[220];
    snprintf(buf, sizeof(buf),
        "{\"uptime\":\"%s\",\"threats\":%d,"
        "\"maxTemp\":%.1f,\"minTemp\":%.1f,\"avgTemp\":%.1f,"
        "\"sheetsCount\":%d,\"heap\":%u,"
        "\"sheetsUrl\":\"https://docs.google.com/spreadsheets\"}",
        getUptimeString().c_str(),
        threatCount,
        safeMax, safeMin, safeAvg,
        sheetsLogCount, heap
    );

    server.sendHeader(F("Cache-Control"), F("no-cache,no-store"));
    server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    server.send(200, F("application/json"), buf);
}

// ============================================================
//   SENSORS + THREAT DETECTION
// ============================================================

void readSensorsAndCheckThreats() {
    // DHT11 needs up to 2 seconds between reads; retry 3x with small delays
    float nh = NAN, nt = NAN;
    for (int attempt = 0; attempt < 3 && (isnan(nh) || isnan(nt)); attempt++) {
        if (attempt > 0) delay(250);
        nh = dht.readHumidity();
        nt = dht.readTemperature();
    }

    if (isnan(nh) || isnan(nt)) {
        Serial.println(F("[DHT] read failed (3 attempts) — check D4 wiring & 10k pull-up to 3.3V"));
        // Keep last known values so the dashboard doesn't go blank
        return;
    }

    humidity    = nh;
    temperature = nt;

    if (tempReadings == 0) { maxTemp=nt; minTemp=nt; avgTemp=nt; }
    else {
        if (nt>maxTemp) maxTemp=nt;
        if (nt<minTemp) minTemp=nt;
        avgTemp = ((avgTemp*(float)tempReadings)+nt) / (float)(tempReadings+1);
    }
    tempReadings++;

    bool t1=(digitalRead(TOUCH1)==HIGH);
    bool t2=(digitalRead(TOUCH2)==HIGH);
    bool t3=(digitalRead(TOUCH3)==HIGH);
    bool prev = threatDetected;

    if (!t1 && !t2 && !t3) {
        statusMessage  = F("System Normal");
        threatDetected = false;
        threatTurbines = "";
    } else {
        threatDetected = true;
        threatTurbines = "";
        if (t1) threatTurbines += "1 ";
        if (t2) threatTurbines += "2 ";
        if (t3) threatTurbines += "3 ";
        threatTurbines.trim();
        statusMessage = "Threat at T" + threatTurbines;

        if (!prev) {
            threatCount++;
            unsigned long now = millis();
            if ((now - lastEmailTime) >= EMAIL_COOLDOWN) {
                sendEmailNotification(statusMessage, threatTurbines);
                emailSent     = true;
                lastEmailTime = now;
            } else {
                Serial.println(F("[EMAIL] Cooldown active."));
            }
        }
    }
}

// ============================================================
//   LCD UPDATE
// ============================================================

void updateLCD() {
    lcd.clear();
    String r0 = statusMessage;
    if (r0.length() > 16) r0 = r0.substring(0,16);
    lcd.setCursor(0,0); lcd.print(r0);

    String r1 = "T:" + String(temperature,1) + "C H:" + String(humidity,0) + "%";
    if (r1.length() > 16) r1 = r1.substring(0,16);
    lcd.setCursor(0,1); lcd.print(r1);
}

// ============================================================
//   SETUP
// ============================================================
void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println(F("\n[BOOT] VAWT v5.4 — single-hop Sheets, HtmlService"));

    sheetsUrlOk = (String(SHEETS_URL).indexOf("YOUR_DEPLOYMENT_ID") == -1);
    
    Wire.begin(D2, D1);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0,0); lcd.print(F(" ---- VAWT ---- "));
    lcd.setCursor(0,1); lcd.print(F("connecting......"));

    dht.begin();
    delay(2000); 

    pinMode(TOUCH1, INPUT);
    pinMode(TOUCH2, INPUT);
    pinMode(TOUCH3, INPUT);

    WiFi.begin(ssid, password);
    Serial.print(F("[WIFI] Connecting"));
    int att=0;
    while (WiFi.status()!=WL_CONNECTED && att<40) { delay(500); Serial.print("."); att++; }

    if (WiFi.status()==WL_CONNECTED) {
        String ip = WiFi.localIP().toString();
        Serial.println("\n[WIFI] IP: " + ip);

        // ============================================================
        // ADDED NTP TIME SYNC HERE
        // ============================================================
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        Serial.print(F("[NTP] Waiting for time sync"));
        
        time_t now = time(nullptr);
        while (now < 8 * 3600 * 2) { 
            delay(500);
            Serial.print(".");
            now = time(nullptr);
        }
        Serial.println(F("\n[NTP] Time Synchronized!"));
        // ============================================================
        lcd.clear();
        lcd.setCursor(0, 0); 
        lcd.print(F("-WIFI- Connected")); 
        lcd.setCursor(0, 1); 
        lcd.print("IP: "); // Print the label
        lcd.print(ip);     // Print the variable containing the address
        lastEmailTime = millis() - EMAIL_COOLDOWN;
        lastSheetsLogTime = millis();

        sendIPAddressEmail();
        delay(500);

        server.on("/", handleRoot);
        server.on("/data", handleData);
        server.on("/stats", handleStats);
        server.begin();
        
        lcd.clear(); lcd.setCursor(0,0); lcd.print(F("Ready")); lcd.setCursor(0,1); lcd.print(ip);
    } else {
        lcd.clear(); lcd.setCursor(0,0); lcd.print(F("WiFi FAILED"));
    }
    startTime = millis();
}
// ============================================================
//   LOOP
// ============================================================

void loop() {
    server.handleClient();
    yield();

    unsigned long now = millis();

    if (now - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
        lastSensorReadTime = now;
        readSensorsAndCheckThreats();
    }

    if (now - lastLCDUpdateTime >= LCD_UPDATE_INTERVAL) {
        lastLCDUpdateTime = now;
        updateLCD();
    }

    if (now - lastSheetsLogTime >= SHEETS_LOG_INTERVAL) {
        lastSheetsLogTime = now;
        logToGoogleSheets();
    }
}
