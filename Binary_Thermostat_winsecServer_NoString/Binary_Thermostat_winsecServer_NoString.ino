#include <ESP8266WiFi.h>
#include <DHT.h>
#include <DHT_U.h>
#include "DNSServer.h"
#include <ESP8266WebServer.h>
#include <_time.h>
#include <ESP8266httpUpdate.h>
#include <WiFiManager.h>
#include <TimeLib.h>
#include <ESP_EEPROM.h>
#include <cstring>
#define DHTTYPE DHT22




DHT dht(D4, DHTTYPE);

const char indexPage[] PROGMEM = R"page(<meta name="viewport" content="width=device-width, initial-scale=1"> <link rel="stylesheet" type="text/css" href="stylesheet.css"> <html> <div id=adjuster style=z-index:3;display:none;position:fixed;top:0;left:0;width:100%;height:100%;background=rgba(100,100,100,0) onclick=if(event.path[0]==this){this.style.display="none";submit(thermostatHolder)};> <table style=text-align:center;position:absolute;z-index=2;><tr><td id=adjmax></td></tr><tr><td><input style="width:1vw;-webkit-appearance: slider-vertical" id=set type=range orient=vertical oninput=adjust2(this) ></td><td id=adjValue></tr><tr><td id=adjmin></td></tr></table> </div> <div id=quickRun data-name=quickRun class=popout style=display:none;> <table> <tr><td colspan=2 style="text-align:center;font-size:2rem">Quick Run</td></tr> <tr><td>Cool</td><td><input onchange="if(Number(this.value)-10 < Number(qrHeat.value)){qrHeat.value=Number(this.value)-10}" type=number min=60 id=qrCool name=qrCool ></td></tr> <tr><td>Heat</td><td><input onchange="if(Number(this.value)+10 > Number(qrCool.value)){qrCool.value=Number(this.value)+10}" type=number max=80 id=qrHeat name=qrHeat ></td></tr> <tr><td>Minutes</td><td><input type=number id=qrTime value=30 name=qrTime></td></tr> <tr><td colspan=2 style=text-align:center;><button onclick=submit(quickRun)>Run</button><button onclick={qrTime.value=0;submit(quickRun);quickRun.style.display="none"}>Stop</button></td></tr> </table> </div> <div id=info style="position: absolute;top: 25%;width:100%;padding-bottom:25px;"> <table> <tr><td id=time style=padding-bottom:1vh;></td></tr> </table> <div id=thermostatHolder data-name=defaultTemps style="border: 5px solid black;margin: auto;padding: 1vw;background: linear-gradient(174deg, rgba(255,255,255,1) 0%, rgba(255,171,0,1) 0%, rgba(57,79,203,1) 100%);"> <Table style="text-align:center;"><tr> <td><input name=defaultCool id=defaultCool onclick=adjust(60,100,this,event); style=width:4rem;background:none;border:none;cursor:pointer;font-size:2rem></td> </tr><tr> <td id=temperature style=font-size:8rem></td> </tr><tr> <td><input name=defaultHeat id=defaultHeat onclick=adjust(40,80,this,event) style=width:4rem;background:none;border:none;cursor:pointer;font-size:2rem></td> </tr> </table> </div> <table style="text-align:center;"><tr> <td id=state></td> </tr><tr> <td id=source></td> </tr><tr> <td><Button onclick=quickRun.style.display="">QuickRun</button></td> </tr> </table> </div> </html> <script src="index.js"></script> )page";
const char indexJS[] PROGMEM = R"page( submitTimeZone(); defaultCool.value=stats.Cool; temperature.innerHTML=stats.Temperature; defaultHeat.value=stats.Heat; state.innerHTML=states[stats.State]; source.innerHTML=stats.Source; qrHeat.value=stats.Heat; qrCool.value=stats.Cool; thermostatHolder.style.width=thermostatHolder.clientHeight-20; thermostatHolder.style.borderRadius=thermostatHolder.clientHeight+"px"; function adjust2(ele){ adjValue.innerText=ele.value; adjTarget.value=ele.value; if (adjTarget==defaultHeat && Number(defaultCool.value) < Number(ele.value) + 10){ defaultCool.value=Number(ele.value) + 10; } if (adjTarget==defaultCool && Number(defaultHeat.value) > ele.value - 10){ defaultHeat.value=Number(ele.value) - 10; } }   function adjust(min,max,ele,event){ ele.blur(); if (stats.Source=="Default"){ set.style.height=thermostatHolder.clientHeight+"px"; adjTarget=ele; adjmax.innerText=max; adjmin.innerText=min; adjValue.innerText=ele.innerText; console.log(event); set.max=max; set.min=min; set.value=ele.innerText; pos=thermostatHolder.getBoundingClientRect(); adjuster.children[0].style.left=pos.x+pos.width; adjuster.children[0].style.top=pos.y; adjuster.style.display=""; set.focus(); } } function update() { clock = new Date(); var now = (clock.getHours() * 60) + (clock.getMinutes()); time.innerText = clock.toLocaleTimeString(); setTimeout(update, 1000); } clock = new Date(); update(); var Status = new EventSource("status.html"); Status.addEventListener('message', function(e) { stats = JSON.parse(e.data); if (adjuster.style.display=="none"){ defaultCool.value=stats.Cool; defaultHeat.value=stats.Heat; } temperature.innerHTML=Math.round(stats.Temperature); source.innerHTML=stats.Source; state.innerHTML=states[stats.State]; }); function qs(s) { return Array.from(...Array(document.querySelectorAll(s))); } function getTime(h, m) { if (m) { return hour * 60 + m; } else { var ampm = ""; var hour = parseInt(h / 60, 10); var minutes = h % 60; if (hour > 11) { ampm = "PM"; } else { ampm = "AM"; } if (hour % 12 == 0) { hour = 12; } else { hour = hour % 12; } return [hour, minutes, ampm]; } } function updateDays(ele, pos) { val = parseInt(qs("." + ele.className).map(function(ele) { if (ele.checked) return 1; else return 0 }).join(""), 2); schedule[pos][0] = val; } function IN(key,element){ if (element instanceof  Array){ var keys=element;  }else if (typeof(element) == "object"){ var keys=Object.keys(element); }else{keys=element;} if (keys.indexOf(key) > -1){ return true; }else{ return false; } } function submit(ele) { var inputs=qs("#"+ele.id+" input,select"); var packet="id="+ele.dataset.name; for (x in inputs){ packet+="&"+inputs[x].name+"="+inputs[x].value; } var now = Number((clock.getHours() * 60) + clock.getMinutes()); qs(".popout").forEach(ele => ele.style.display = "none"); var request = new XMLHttpRequest(); request.open("POST", "saveData.html", true); request.setRequestHeader('Content-type', 'application/x-www-form-urlencoded'); request.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { console.log(this.responseText); } }; request.send(packet); } function submitTimeZone(){ var d=new Date; var packet="id=offset&offset="+d.getTimezoneOffset()/-60; var request = new XMLHttpRequest(); request.open("POST", "saveData.html", true); request.setRequestHeader('Content-type', 'application/x-www-form-urlencoded'); request.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { console.log(this.responseText); } }; request.send(packet); })page";
const char scheduleJS[] PROGMEM = R"page(modified = 0; wdays = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]; setup(); function qs(s) { return Array.from(...Array(document.querySelectorAll(s))); } function save() { modified = 0; var names = ["days", "heat","cool", "start", "end"]; for (var pos in schedule) { var m = "id=schedule&pos=" + pos; for (var val = 0; val <names.length; val++) { m += "&" + names[val] + "=" + schedule[pos][val]; } var r = new XMLHttpRequest(); r.open("POST", "saveData.html", true); r.setRequestHeader('Content-type', 'application/x-www-form-urlencoded'); r.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { if (IN("Error",this.responseText)){ alert("ERROR:"+this.responseText); } } }; r.send(m); } var d = new XMLHttpRequest(); d.open("POST", "saveData.html", true); d.setRequestHeader('Content-type', 'application/x-www-form-urlencoded'); d.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { alert(this.responseText); saving.style.display = "none"; } }; d.send("id=scheduleDone"); saving.style.display = ""; } function getMTime(T) { hour = parseInt(T / 60, 10); minutes = T % 60; return [hour, minutes]; } function getTime(h, m) { console.log(arguments); if (m != undefined) { return hour * 60 + m; } else { var ampm = ""; hour = parseInt(h / 60, 10); minutes = h % 60; if (hour > 11) { ampm = "PM"; } else { ampm = "AM"; } if (hour % 12 == 0) { hour = 12; } else { hour = hour % 12; } return [hour, minutes, ampm]; } } function updateDays(ele, pos) { modified = 1; if (ele.dataset.value == "1") { ele.style.background = "red"; ele.style.color = "black"; ele.innerText = String.fromCharCode(10008); ele.dataset.value = 0; } else { ele.dataset.value = 1; ele.style.background = "green"; ele.style.color = "white"; ele.innerText = String.fromCharCode(10004); } val = parseInt(qs("." + ele.className).map(function(ele) { if (ele.dataset.value == "1") return 1; else return 0; }).join(""), 2); schedule[pos][0] = val; } function dayInput(days, pos) { days = String(days); console.log(days); var v = "<table style=width:20vw;font-size:1.5rem;><tr>"; for (var x = 0; x < 7; x++) { v += '<td onclick=updateDays(this,' + pos + ') class=p' + pos; if (days[x] == "1") { v += ' data-value=1 style="font-size:1.5rem;cursor:pointer;color:white;background:green;border:black solid 1px">&#10004'; } else { v += ' data-value=0 style="font-size:1.5rem;cursor:pointer;color:black;background:red;border:black solid 1px">&#10008'; } v += "</td><td>" + wdays[x] + "</td></tr>"; } v += "</table>"; return v; } function remove(x) { schedule[x] = [0, 65, 85, 0, 0]; setup(); } function addTime() { for (var x in schedule) { if (schedule[x][0] == 0) { schedule[x][0] = 127; schedule[x][1] = 65; schedule[x][2] = 85; schedule[x][3] = 0; schedule[x][4] = 0; schedule.sort((a, b) => { if (b[3] == 0) return -1; }); setup(); return; } } alert("All slots are occupied! Delete a schedule point to make space!"); } function update(ele) { modified = 1; pos = ele.dataset.value; if (ele.dataset.type == "heat") { schedule[pos][1] = Number(ele.value); } else if (ele.dataset.type=="cool"){ schedule[pos][2] == Number(ele.value); }else if (ele.dataset.type == "start") { var val = ele.value.split(":"); schedule[pos][3] = (Number(val[0]) * 60) + Number(val[1]); }else if (ele.dataset.type == "end") { var val = ele.value.split(":"); schedule[pos][4] = (Number(val[0]) * 60) + Number(val[1]); } } function mysort(ele) { var col = ele.cellIndex; var vals = []; var n = 0; for (x in schedule) { if (schedule[x][0] && schedule[x][3]) { vals[n] = [schedule[x][col], n]; n++; } } return vals; } function colSort(ele, rev = 0) { if (ele.className.indexOf("sorted") > -1) { rev = 1; ele.className = ele.className.replace("sorted", "reverse"); } else if (ele.className.indexOf("reverse") > -1) { rev = 0; ele.className = ele.className.replace("reverse", "sorted"); } else { for (x in qs(".sorted,.reverse")) { qs[x].className = qs[x].className.replace("sorted", ""); qs[x].className = qs[x].className.replace("reverse", ""); } ele.className += " sorted"; } col = ele.cellIndex; if (rev) { schedule.sort((a, b) => (b[col] - a[col])); } else { schedule.sort((a, b) => (a[col] - b[col])); } setup(); } function setup() { var weekdays = "SMTWTFS"; var t = '<table class=datatable ><tr><th class="sortable title" onclick=colSort(this,1)>Days</th><th class=title>Heat</th><th class=title>Cool</th><th class="sortable title" onclick=colSort(this)>Start</th><th class="sortable title" onclick=colSort(this)>End</th><th class=title>Action</th></tr>'; for (var x = 0; x < schedule.length; x++) { if (!schedule[x][0]) { continue; } var days = schedule[x][0].toString(2).padStart(7, '0'); var stringDays = ""; for (d in days) { if (days[d] == "1") { stringDays += "<u><b>" + weekdays[d] + "</u></b>"; } else { stringDays += weekdays[d]; } } t += '<tr><td selectable=false style=cursor:pointer onmousedown=dayInputMenu.innerHTML=dayInput("' + days + '",' + x + ') onclick=dayMenu.style.display="">' + stringDays + '</td><td><input type=number data-type="heat" data-value=' + x + ' onchange=update(this) max=80 min=40 value=' + schedule[x][1] + '></td><td><input type=number data-type="cool" data-value=' + x + ' onchange=update(this) max=120 min=65 value='+schedule[x][2]+'></td><td><input data-value=' + x + ' onchange=update(this) type=time data-type="start" value=' + String(getMTime(schedule[x][3])[0]).padStart(2, "0") + ':' + String(getMTime(schedule[x][3])[1]).padStart(2, "0") + '></td><td><input data-type="end" data-value=' + x + ' onchange=update(this) type=time value=' + String(getMTime(schedule[x][4])[0]).padStart(2, "0") + ':' + String(getMTime(schedule[x][4])[1]).padStart(2, "0") + '></td><td> <svg data-value='+ x + ' onclick=remove(' + x + ') xmlns="http://www.w3.org/2000/svg" height="1.5rem" viewBox="0 0 24 24" width="1.5rem" onclick="remove(this,this.parentElement.parentElement.id)"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"></path><path d="M0 0h24v24H0z" fill="none"></path></svg></td></tr>'; } t += "</table>"; ss.innerHTML = t; } function IN(key,element){ var keys; if (element instanceof  Array){ keys=element; }else if (typeof(element) == "object"){ keys=Object.keys(element); }else{keys=element;} if (keys.indexOf(key) > -1){ return true; }else{ return false; } })page";
const char schedulePage[] PROGMEM = R"page(<meta name="viewport" content="width=device-width, initial-scale=1"> <link rel="stylesheet" type="text/css" href="stylesheet.css"> <html> <div id=saving style="position:fixed;top:0;bottom:0;left:0;right:0;text-align:center;font-size:3rem;background:rgba(150,150,150,.5);display:none"> <div class=popout> Saving Data Please Wait</div> </div> <div id=save style=text-align:center> <button onclick=save()>Save Changes</button> <div style=font-size:.75rem;> <br>Click Column Heads to Sort Column <br>Click Day Column to Select Run Days </div> </div> <div id=ss></div> <div class=popout id=dayMenu style=display:none;> <div id=dayInputMenu></div> <button onmousedown=setup() onclick=dayMenu.style.display="none";>Save</button> <button onclick=dayMenu.style.display="none";>Cancel</button> </div> <div style=text-align:center><button onclick=addTime()>Add Time</button></div> </html> <script src="schedule.js">  </script>)page";
const char styleSheet[] PROGMEM = R"page(<style> @media screen and (min-width: 700px){ html{ font-size:35px; } } html{ font-size:3vw; } *{ font-size:1rem; font-family:sans-serif; } @media screen and (min-width:700px){ html{ font-size:20px; } } form{ text-align:center; } button,input[type=submit]{ margin:5px; border-radius:10px; border: solid black 3px; font-weight:bold; } .popout{ z-index:3; position:fixed; top:10vh; left:10vw; right:10vw; background:white; text-align:center; border:solid black 5px; border-radius:10px; box-shadow: 5px 5px 25px; padding:10px; } .sortable{ cursor:pointer; } table{ margin:auto; border-collapse:collapse; } .header{ background:rgb(240,240,240); border-bottom:solid 1px lightgray; text-align:left; cursor:pointer; transition:.5s; } .header:hover{ transition:.5s; padding-left:1rem; } .datatable{ border-collapse:collapse; width:90%; } input,svg,select{ cursor:pointer; text-align:center; text-align-last:center; margin-bottom:5px; } .datatable td{ text-align:center; width:33%; padding-left:1rem; padding-right:1rem; text-align:center; border-bottom:solid 1px rgb(140,140,140); } input:hover{ background-color:rgb(240,240,240); } #stable select{ border:none; text-align:center; } select:hover{ background-color:rgb(240,240,240); } .datatable input{ text-align:center; border:none; } .datatable select{ border:none; } input[type=number]{ width:3rem; } .title{ background:rgb(240,240,240); border-right:solid 1px black; border-left:solid 1px black; width:33%; border-top:2px solid black; border-bottom:2px solid black; } .divhead{ margin-bottom:.25rem; margin-top:1.5rem; font-weight:bold; text-decoration:underline; text-align:center; width:100%; } </style>)page";
const char settingsPage[] PROGMEM = R"page(<meta name="viewport" content="width=device-width, initial-scale=1"> <link rel="stylesheet" type="text/css" href="stylesheet.css"> <html style=text-align:center;> <div style=text-align:center;font-weight:bold;font-size:1.5rem;>Settings</div> <div class=divHead>Away Mode</div> <form onsubmit=s(this,event) id="Away" method=POST> Away mode: <select name=awayMode> <option value=0>None</option> <option value=1>Motion</option> </select> <br> Away Heat:<input type=number name=awayHeat value=40 max=80 min=40><br> Away Cool:<input type=number name=awayCool value=85 max=100 min=60><br> <input type=submit value=Save> </form> <div class=divHead>Wifi Settings</div> <div style=text-align:center; id=wifi> <form onsubmit=s(this,event) id="AP"  method=POST> Access Point Visibility: <select name=visibility> <Option value=0>Visible</option> <option value=1>Hidden</option> </select> <table> <tr> <td style=text-align:right>SSID</td> <td><input type=text name=ssid id=ssid></td> </tr> <tr> <td>Password</td> <td><input type=text name=password id=password></td> </tr> <tr> <td colspan=2 style=text-align:center;><input type=submit value=Save></td> </tr> </table> </form> </div> <div class=divHead>Remote Access</div> <form id=remoteServer onsubmit=s(this,event) method=POST><input type=text placeholder="User Name" id=serverUName name=serverUName><br><input type=password placeholder="password" id=serverPW name=serverPW ><a style="cursor:pointer"  onmousedown=serverPW.type="Text" onmouseup=serverPW.type="password">Show</a><br><input type=submit value=Save></form> <div class=divHead>Firmware Update</div> <form id=Update onsubmit=s(this,event)  method=POST> <input type=text placeholder="Update Server" value="jeffshomesite.com" name=source><br> <input value=Update type=submit onclick='alert("Device will update and reboot")'> </form> <div class=divHead>Reboot Thermostat</div> <form id=reboot onsubmit=s(this,event)> <input type=submit value=Reboot> </form> <div class=divHead>Erase All Saved Data</div> <form id=reset onsubmit=s(this,event) ><input type=submit value="Full Reset"> </form> <html> <script> function pairEncode(fd) { fd = Array.from(fd.entries()); f = fd[0][0] + "=" + fd[0][1]; for (var x = 1; x < fd.length; x++) { f += "&" + fd[x][0] + "=" + fd[x][1]; } return f; } ssid.value = SSID; function s(ele, eve) { eve.preventDefault(); if (ele.id == "reset") { if (!confirm("This will erase all saved data! Continue?")) { return false; } } fd = new FormData(ele); fd.set("id", ele.id); data = pairEncode(fd); var r = new XMLHttpRequest; r.onreadystatechange = function () { if (this.readyState == 4 && this.status == 200) { alert(this.responseText); } }; r.open("POST", "saveData.html", true); r.setRequestHeader('Content-type', 'application/x-www-form-urlencoded'); r.send(data); return false; } </script>)page";
const char header[] PROGMEM = R"page(<html> <table style=width:100%> <tr> <td style="text-align:left;width:20%;padding:1rem;"> <svg onclick=location="settings.html" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="3rem"> <path d="M0 0h24v24H0V0z" fill="none"></path> <path d="M19.43 12.98c.04-.32.07-.64.07-.98s-.03-.66-.07-.98l2.11-1.65c.19-.15.24-.42.12-.64l-2-3.46c-.12-.22-.39-.3-.61-.22l-2.49 1c-.52-.4-1.08-.73-1.69-.98l-.38-2.65C14.46 2.18 14.25 2 14 2h-4c-.25 0-.46.18-.49.42l-.38 2.65c-.61.25-1.17.59-1.69.98l-2.49-1c-.23-.09-.49 0-.61.22l-2 3.46c-.13.22-.07.49.12.64l2.11 1.65c-.04.32-.07.65-.07.98s.03.66.07.98l-2.11 1.65c-.19.15-.24.42-.12.64l2 3.46c.12.22.39.3.61.22l2.49-1c.52.4 1.08.73 1.69.98l.38 2.65c.03.24.24.42.49.42h4c.25 0 .46-.18.49-.42l.38-2.65c.61-.25 1.17-.59 1.69-.98l2.49 1c.23.09.49 0 .61-.22l2-3.46c.12-.22.07-.49-.12-.64l-2.11-1.65zM12 15.5c-1.93 0-3.5-1.57-3.5-3.5s1.57-3.5 3.5-3.5 3.5 1.57 3.5 3.5-1.57 3.5-3.5 3.5z"></path> </svg> <td> <td  style="text-align:center;width:60%;padding:1rem;font-size:2.5rem;font-weight:bold;cursor:pointer" onclick=location="index.html">Thermostat</td> <td  style="text-align:right;width:20%;padding:1rem;" > <svg onclick=location="scheduleSetup.html" width="3rem" viewBox="0 0 24 24"> <path d="M0 0h24v24H0V0z" fill="none"></path> <path d="M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm-.22-13h-.06c-.4 0-.72.32-.72.72v4.72c0 .35.18.68.49.86l4.15 2.49c.34.2.78.1.98-.24.21-.34.1-.79-.25-.99l-3.87-2.3V7.72c0-.4-.32-.72-.72-.72z"></path> </svg> </td> </tr> </table></html>)page";
char * dataDump(char * buff, unsigned int len) ;

char bigbuff[9500];
//const char settingsPage[] PROGMEM = R"page(<meta name="viewport" content="width=device-width, initial-scale=1"> <link rel="stylesheet" type="text/css" href="stylesheet.css"> <html style=text-align:center;> <div style=text-align:center;font-weight:bold;font-size:1.5rem;>Settings</div> <div class=divHead>Away Mode</div> <form onsubmit=s(this,event) id="Away" method=POST> Away mode:<select name=awayMode> <option value=0>None</option> <option value=1>Motion</option> </select> <br> Away Heat:<input type=number name=awayHeat value=40 max=80 min=40><br> Away Cool:<input type=number name=awayCool value=85 max=100 min=60><br> <input type=submit value=Save> </form> <div class=divHead>Wifi Settings</div> <div style=text-align:center; id=wifi> <form onsubmit=s(this,event) id="AP"  method=POST> Access Point Visibility: <select name=visibility> <Option value=0>Visible</option> <option value=1>Hidden</option> </select> <table><tr> <td style=text-align:right>SSID</td><td><input type=text name=ssid id=ssid></td> </tr><tr> <td>Password</td><td><input type=text name=password id=password></td> </tr><tr> <td colspan=2 style=text-align:center;><input type=submit value=Save></td> </tr></table>  </form> </div> <div class=divHead>Firmware Update</div> <form id=Update onsubmit=s(this,event)  method=POST> <input type=text placeholder="Update Server" value="jeffshomesite.com" name=source><br> <input value=Update type=submit onclick='alert("Device will update and reboot")'> </form> <div class=divHead>Reboot Thermostat</div> <form id=reboot onsubmit=s(this,event)> <input type=submit value=Reboot> </form> <div class=divHead>Erase All Saved Data</div> <form id=reset onsubmit=s(this,event) ><input type=submit value="Full Reset"> </form> <html> <script> function pairEncode(fd) { fd = Array.from(fd.entries()); f = fd[0][0] + "=" + fd[0][1]; for (var x = 1; x < fd.length; x++) { f += "&" + fd[x][0] + "=" + fd[x][1]; } return f; } ssid.value = SSID; function s(ele, eve) { eve.preventDefault(); if (ele.id == "reset") { if (!confirm("This will erase all saved data! Continue?")) { return false; } } fd = new FormData(ele); fd.set("id", ele.id); data = pairEncode(fd); var r = new XMLHttpRequest; r.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { alert(this.responseText); } }; r.open("POST", "saveData.html", true); r.setRequestHeader('Content-type', 'application/x-www-form-urlencoded'); r.send(data); return false; } </script>)page";

//header nonsense
//char * FPSTR(settingsPage);
//char * FPSTR(schedulePage);
//char * FPSTR(indexPage) ;
//char * FPSTR(styleSheet);
//char * FPSTR(indexJS);


bool connect();
bool serverRecv() ;
bool serverSend(String message) ;
char * dataDump(char * buff, unsigned int len);

bool cool[3] = {0, 0, 1};
bool heat[3] = {1, 0, 0};
bool off[3] = {0, 1, 0};
bool * cond[3] = {off, heat, cool};
//char * sStatus[3]={(char*)"Off",(char*)"Heat",(char*)"Cool"};
//String sStatus[3] = {"Off", "Heat", "Cool"};
time_t tempCheck = 0;
time_t serverCheckin = 0;
uint8_t Heat ;
uint8_t Cool ;
char Source[10] = "Default";
time_t minRunTime = now();



struct QR {
  time_t time = 0;
  uint8_t heat = 65;
  uint8_t cool = 85;
} quickRun;

WiFiClient piServer;
WiFiClient piServer1;
const uint8_t DNS_PORT = 53;          // Capture DNS requests on port 53
DNSServer dnsServer;              // Create the DNS object
ESP8266WebServer  server(80);
time_t time_update = 0;
char IPADDRESS[16];
time_t activeTime = 0;
uint8_t Status = 0;
//int pins[3] = {D3, D5, D6};
int pins[3] = {D3, 1, 3};
float Temperature = 75;
float Humidity;


uint8_t setStatus(uint8_t  state, bool force = 0) {
  if (Status == state && force == 0) {
    return state;
  }
  else if (now() < minRunTime && force == 0) {
    return state;
  }
  else {
    for (uint8_t x = 0; x < 3; x++) {
      digitalWrite(pins[x], cond[0][x]);
    }
    delay(1000);
    for (uint8_t x = 0; x < 3; x++) {
      digitalWrite(pins[x], cond[state][x]);
    }
    Status = state;
    if (state) {
      minRunTime = now() + 600;
    }
    return state;
  }
}

struct schedulePoint {
  uint8_t days = 0;
  uint8_t heat = 65;
  uint8_t cool = 85;
  uint16_t start = 0;
  uint16_t end = 0;
};

struct myData {
  char myName[25] = "Thermostat";
  schedulePoint schedule[64];
  char password[25];
  char ssid[25] = "Wifi Thermostat";
  bool hidden = false;
  uint8_t defaultHeat = 65;
  uint8_t defaultCool = 85;
  int timezone = 0;
  char serverUName[25] = "";
  char serverPW[25] = "";
} data;

void EEPROM_Startup() {
  EEPROM.begin(sizeof(myData));
  if (EEPROM.percentUsed() >= 0) {
    EEPROM.get(0, data);
    //Serial.println("EEPROM has data from a previous run.");
    //Serial.print(EEPROM.percentUsed());
    //Serial.println("% of ESP flash space currently used");
  } else {
    //Serial.println("EEPROM size changed - EEPROM data zeroed - commit() to make permanent");
  }
}

void fullReset() {
  WiFi.disconnect();
  for (uint8_t z = 0; z < 64; z++) {
    data.schedule[z].days = 0;
    data.schedule[z].heat = 65;
    data.schedule[z].cool = 85;
    data.schedule[z].end = 0;
    data.schedule[z].start = 0;
  }
  data.defaultCool = 85;
  data.defaultHeat = 65;
  data.password[0] = 0;
  strcpy(data.ssid, "Wifi Thermostat");
  data.hidden = false;
  EEPROM_Save();
  delay(1000);
  ESP.reset();
}


void readDHT() {
  float t = dht.readTemperature(true);
  uint8_t n = 0;
  while (isnan(t)) {
    n++;
    if (n == 10) break;
    t = dht.readTemperature(true);
  }
  if (!(isnan(t))) {
    Temperature = t;
  }
  t = dht.readHumidity();
  if (!(isnan(t))) {
    Humidity = t;
  }
}

char * get_JSON(char * buff, unsigned int buffSize) {
  int len;
  char convert[500];
  strcpy(buff, "<script>schedule=[");
  for (uint8_t z = 0; z < 64; z++) {
    if (z == 0) strcat(buff, "[");
    else strcat(buff, ",[");
    len = sprintf(convert, "%d,%d,%d,%d,%d]", data.schedule[z].days, data.schedule[z].heat, data.schedule[z].cool, data.schedule[z].start, data.schedule[z].end);
    if (len + strlen(buff) > buffSize) {
      strcpy(buff, "<script>ERROR=\"JSON too big for buffer\";</script>");
      return buff;
    }
    strcat(buff, convert);
  }
  strcat(buff, "];\n");
  sprintf(convert, "SSID=\"%s\";\n", data.ssid);
  strcat(buff, convert);;
  strcat(buff, "states=[\"Off\",\"Heating\",\"Cooling\"];\n");
  len = sprintf(convert, "stats={\"Temperature\":%0.2f,\"State\":%d,\"Heat\":%d,\"Cool\":%d,\"Source\":\"%s\"};\n</script>", Temperature, Status, Heat, Cool, Source);
  if (len + strlen(buff) > buffSize) {
    strcpy(buff, "<script>ERROR=\"JSON too big for buffer\";</script>");
    return buff;
  }
  strcat(buff, convert);
  return buff;
}




String EEPROM_Save() {
  //Serial.println("EEPROM Sector usage:" + String(EEPROM.percentUsed()) + "%");
  EEPROM.put(0, data);
  boolean ok = EEPROM.commit();
  //Serial.println((ok) ? "Commit OK" : "Commit Failed");
  return (ok) ? "Commit OK" : "Commit Failed";
}


String software_update(String ip_address) {
  //Serial.println("\nStarting update");
  //Serial.println(String(ip_address));
  t_httpUpdate_return ret = ESPhttpUpdate.update(ip_address.c_str(), 80, "/Binary_Thermostat.ino.d1_mini.bin");
  //Serial.println("UPDATE VALUE");
  //Serial.println(ret);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      //Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      return "HTTP_UPDATE_FAILD Error" + String(ESPhttpUpdate.getLastError()) + ":" + String(ESPhttpUpdate.getLastErrorString());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      //Serial.println("HTTP_UPDATE_NO_UPDATES");
      return "HTTP_UPDATE_NO_UPDATES";
      break;
    case HTTP_UPDATE_OK:
      //Serial.println("HTTP_UPDATE_OK");
      return "HTTP_UPDATE_OK";
      break;
  }
}


void update_time() {
  //Serial.println("offset=" + String(data.timezone));
  configTime(data.timezone * 3600, 0, "pool.ntp.org", "time.nist.gov");
  while (year() < 2020) {
    //Serial.print(".");
    delay(100);
    time_t t = time(NULL);
    setTime(t);
    time_update = now() + (24 * 3600);
  }
}

int rightNow() {
  return (hour() * 60) + minute();
}

bool timeCheck(int start, int end) {
  if (start < end) {
    if (rightNow() >= start  && rightNow() < end) {
      return 1;
    } else {
      return 0;
    }
  }
  else if (start > end) {
    if ((rightNow() >= 0 && rightNow() < end) || (rightNow() >= start) ) {
      return 1;
    } else {
      return 0;
    }
  }
}

bool dayCheck(uint8_t n, uint8_t d) {
  bool days[7];
  for (int i = 6; i > -1 ; --i)
  {
    days[i] = n & 1;
    n /= 2;
  }
  return days[d];
}

void onCheck(uint8_t heat, uint8_t cool) {
  if (Temperature < heat) {
    setStatus(1);
  }
  else if (Temperature > cool) {
    setStatus(2);
  }
  else if (Temperature > heat && Temperature < cool) {
    setStatus(0);
  }
}

void runCheck() {
  if (quickRun.time > now()) {
    Heat = quickRun.heat;
    Cool = quickRun.cool;
    strcpy(Source, "QuickRun");
    onCheck(quickRun.heat, quickRun.cool);
    return;
  }
  else {
    for (uint8_t x = 0; x < 64; x++) {
      if (data.schedule[x].end) {
        if (dayCheck(data.schedule[x].days, weekday() - 1)) {
          if (timeCheck(data.schedule[x].start, data.schedule[x].end)) {
            Heat = data.schedule[x].heat;
            Cool = data.schedule[x].cool;
            strcpy(Source, "Schedule");
            onCheck(data.schedule[x].heat, data.schedule[x].cool);
            return;
          }
        }
      }
    }
  }
  Heat = data.defaultHeat;
  Cool = data.defaultCool;
  strcpy(Source, "Default");
  onCheck(data.defaultHeat, data.defaultCool);
}

char fbuff[200];
char * footer(char * fbuff) {
  sprintf(fbuff, "<div style=\"left:0px;position:fixed;bottom:0px;font-size:50%%;text-align:center;background:white;right:0px;\">Local IP:%d.%d.%d.%d Compiled: %s %s</div>", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3], __DATE__ , __TIME__);
  return fbuff;
}

void setup() {
  dht.begin();
  Serial.begin(115200);
  for (uint8_t x = 0; x < 3; x++) {
    pinMode(pins[x], OUTPUT);
  }
  //Serial.println();
  WiFiManager wifiManager;
  wifiManager.autoConnect("Wifi-Thermostat-setup");
  //Serial.println("connected!");
  EEPROM_Startup();
  update_time();
  //Serial.println("Time set");
  // Wait for connection
  WiFi.hostname("MeshThermostat");
  //Serial.println("");
  //Serial.print("Connected to ");
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP());
  WiFi.mode(WIFI_AP_STA);
  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMsk(255, 255, 255, 0);
  WiFi.softAP(data.ssid);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  setStatus(0, 1);



  //////////////  WEBServer  /////////////////
  server.on("/stylesheet.css", [] () {
    server.send(200, "text/css", FPSTR(styleSheet));
  });
  server.on("/saveData.html", [] () {
    //Serial.println("requested saveData.html");
    //Serial.println("Request Arguments");
    for (uint8_t x = 0; x < server.args(); x++) {
      //Serial.println(server.argName(x) + " = " + server.arg(x));
    }
    if (server.arg("id") == "reboot") {
      server.send(200, "text/html", "Rebooting Device");
      ESP.reset();
    }
    if (server.arg("id") == "offset") {
      if (data.timezone != server.arg("offset").toInt()) {
        data.timezone = server.arg("offset").toInt();
        update_time();
        //Serial.println("TZ update:" + EEPROM_Save());
      }
      sprintf(bigbuff, "Offset=%d", data.timezone);
      server.send(200, "text/html", bigbuff);
    }
    if (server.arg("id") == "remoteServer") {
      server.arg("serverUName").toCharArray(data.serverUName, 25);
      server.arg("serverPW").toCharArray(data.serverPW, 25);
      delay(10);
      server.send(200, "text/html", EEPROM_Save());
    }
    if (server.arg("id") == "schedule") {
      uint8_t pos = server.arg("pos").toInt();
      uint8_t h = server.arg("heat").toInt();
      uint8_t c = server.arg("cool").toInt();
      uint8_t d = server.arg("days").toInt();
      int16_t s = server.arg("start").toInt();
      int16_t e = server.arg("end").toInt();
      sprintf(bigbuff, "%d TempError", pos);
      if (h > c ) server.send(200, "text/html", bigbuff);
      else {
        data.schedule[pos].heat = h;
        data.schedule[pos].cool = c;
        data.schedule[pos].days = d;
        data.schedule[pos].start = s;
        data.schedule[pos].end = e;
        server.send(200, "text/html", "recieved");
      }
    }
    if (server.arg("id") == "defaultTemps") {
      data.defaultHeat = server.arg("defaultHeat").toInt();
      data.defaultCool = server.arg("defaultCool").toInt();
      EEPROM_Save();
      server.send(200, "text/html", "Accepted");
    }
    if (server.arg("id") == "AP") {
      server.arg("ssid").toCharArray(data.ssid, 25);
      if (server.arg("hidden").toInt()) {
        data.hidden = true;
      } else {
        data.hidden = false;
      }
      server.arg("password").toCharArray(data.password, 25);
      server.send(200, "text/html", EEPROM_Save());
      WiFi.softAP(data.ssid, data.password, 1, data.hidden, 4);
    }
    if (server.arg("id") == "scheduleDone") {
      server.send(200, "text/html", EEPROM_Save());
      //Serial.println(get_JSON());
    }
    if (server.arg("id") == "Update") {
      //server.send(200, "text/html", "Device is updating...");
      server.send(200, "text/html", software_update(server.arg("source")));
    }
    if (server.arg("id") == "reset") {
      server.send(200, "text/html", "Wiping memory, device will reset now");
      fullReset();
    }
    if (server.arg("id") == "quickRun") {
      quickRun.heat = server.arg("qrHeat").toInt();
      quickRun.cool = server.arg("qrCool").toInt();
      quickRun.time = server.arg("qrTime").toInt();
      quickRun.time = now() + (quickRun.time * 60);
      if (server.arg("qrTime") == "0") {
        quickRun.time = 0;
      }
      server.send(200, "text/html", "QuickRun Started");
      delay(10);
    }
    yield();
  });
  server.on("/status.html", []() {
    sprintf(bigbuff, "retry:1000\ndata:{\"Temperature\":%0.2f,\"State\":%d,\"Heat\":%d,\"Cool\":%d,\"Source\":\"%s\"}\n\n", Temperature, Status, Heat, Cool, Source);
    server.send(200, "text/event-stream", bigbuff);
  });
  server.on("/", []() {
    get_JSON(bigbuff, sizeof(bigbuff));
    strcat_P(bigbuff, header);
    strcat_P(bigbuff, indexPage);
    strcat(bigbuff, footer(fbuff));
    server.send(200, "text/html", bigbuff);
    delay(500);
  });
  server.on("/index.js", []() {
    server.send(200, "application/javascript", FPSTR(indexJS));
  });
  server.on("/schedule.js", []() {
    server.send(200, "application/javascript", FPSTR(scheduleJS) );
  });
  server.on("/index.html", []() {
    //Serial.println("got /index.html request");
    get_JSON(bigbuff, sizeof(bigbuff));
    strcat_P(bigbuff, header);
    strcat_P(bigbuff, indexPage);
    strcat(bigbuff, footer(fbuff));
    server.send(200, "text/html", bigbuff);
    delay(500);
  });
  server.on("/scheduleSetup.html", [] () {
    //Serial.println("requested scheduleSetup.html");
    get_JSON(bigbuff, sizeof(bigbuff));
    strcat_P(bigbuff, header);
    strcat_P(bigbuff, schedulePage);
    server.send(200, "text/html", bigbuff);
    delay(500);
  });
  server.onNotFound([]() {
    //Serial.println("requested something unknown");
    get_JSON(bigbuff, sizeof(bigbuff));
    strcat_P(bigbuff, header);
    strcat_P(bigbuff, indexPage);
    strcat(bigbuff, footer(fbuff));
    server.send(200, "text/html", bigbuff);
    delay(500);
  });
  server.on("/settings.html", []() {
    get_JSON(bigbuff, sizeof(bigbuff));
    strcat_P(bigbuff, header);
    strcat_P(bigbuff, settingsPage);
    strcat(bigbuff, footer(fbuff));
    server.send(200, "text/html", bigbuff);
    delay(500);
  });
  server.on("/favicon.ico", []() {
    server.send(200, "text/html", "");
  });
  server.begin();
  dnsServer.start(DNS_PORT, "*", apIP);
}




void loop() {
  if (now() > tempCheck) {
    readDHT();
    while (!(piServer1.connected())) {
      if (piServer1.connect("192.168.0.103", 49998) != 1) {
        break;
      }
      piServer1.print("{\"name\":\"" + String(data.myName) + "\"}");
    }
    sprintf(bigbuff, "{\"Temp\":%0.2f,\"State\":%d,\"Heat\":%d,\"Cool\":%d,\"Memory\":%d,\"Source\":\"%s\",\"Humidity\":%0.2f}", Temperature, Status, Heat, Cool, ESP.getFreeHeap(), Source, Humidity);
    piServer1.print(bigbuff);
    tempCheck = now() + 10;
    Serial.println(ESP.getFreeHeap());
  }
  if (now() > time_update || year() < 2020) {
    update_time();
  }
  yield();
  dnsServer.processNextRequest();
  server.handleClient();
  runCheck();
  if (serverCheckin < now() && data.serverUName[0] != 0) {
    if (!piServer.connected()) {
      piServer.connect("jeffshomesite.com", 45002);
      WiFi.macAddress().toCharArray(bigbuff, 9500);
      piServer.printf("{\"User\":\"%s\",\"password\":\"%s\",\"Device\":\"Thermostat\",\"Mac\":\"%s\"}", data.serverUName, data.serverPW, bigbuff);
      if (piServer.connected() && piServer.readStringUntil(char(5)) == "Success") {
        piServer.print(dataDump(bigbuff, sizeof(bigbuff)));
      }
    }
    while (piServer.available()) {
      serverRecv();
    }
    runCheck();
    piServer.print(dataDump(bigbuff, sizeof(bigbuff)));
    serverCheckin = now() + 1;
    while (piServer.available()) {
      serverRecv();
    }
  }
}
