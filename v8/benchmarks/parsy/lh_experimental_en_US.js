/* Copyright 2006-2008 Google Inc. All Rights Reserved. */ (function(){
;var ba=/\s+/g,ca=/^ /,da=/ $/;function ea(a){if(!a)return"";return a.replace(ba," ").replace(ca,"").replace(da,"")}
function fa(a){var b="",c="";for(var d=0;d<a.length;){var e=ga(a,d);if(e.charAt(0)=="<"){var f=e.indexOf(">");c=e.substring(1,f!=-1?f:e.length)}else if(c=="")b+=e;d+=e.length}if(c==""&&b.indexOf("@")!=-1){c=b;b=""}b=ea(b);b=ha(b,"'");b=ha(b,'"');c=ea(c);return[b,c]}
function ha(a,b){var c=a.length;if(a.charAt(0)==b&&a.charAt(c-1)==b)return a.substring(1,c-1);return a}
function ia(a){var b=[],c="",d;for(var e=0;e<a.length;){d=ga(a,e);if(d==","){ja(b,c);c="";e++;continue}c+=d;e+=d.length}if(c!=""||d==",")ja(b,c);return b}
var ka='"<([',la='">)]';function ga(a,b){var c=a.charAt(b),d=ka.indexOf(c);if(d==-1)return c;var e=a.indexOf(la.charAt(d),b+1),f=e>=0?a.substring(b,e+1):a.substr(b);return f}
function ja(a,b){b=ma(b);a[a.length]=b}
var na=/[()<>@,;:\\\".\[\]]/;function ma(a){var b=fa(a),c=b[0],d=b[1];if(c.indexOf('"')==-1){var e=na.test(c);if(e)c='"'+c+'"'}return c==""?d:(d==""?c:c+" <"+d+">")}
;;var oa=oa||{},j=this;var qa=function(a,b){var c=a.split("."),d=j,e;if(!(c[0]in d)&&d.execScript)d.execScript("var "+c[0]);while(e=c.shift())if(!c.length&&pa(b))d[e]=b;else d=d[e]?d[e]:(d[e]={})},
ra=function(a){var b=a.split("."),c=j;for(var d;d=b.shift();)if(c[d])c=c[d];else return null;return c},
sa=function(){},
ta=function(){throw Error("unimplemented abstract method");};
var va=function(a){var b=typeof a;if(b=="object")if(a){if(typeof a.length=="number"&&typeof a.splice!="undefined"&&!ua(a,"length"))return"array";if(typeof a.call!="undefined")return"function"}else return"null";else if(b=="function"&&typeof a.call=="undefined")return"object";return b};
if(Object.prototype.propertyIsEnumerable)var ua=function(a,b){return Object.prototype.propertyIsEnumerable.call(a,b)};
else ua=function(a,b){if(b in a)for(var c in a)if(c==b)return true;return false};
var pa=function(a){return typeof a!="undefined"};
var wa=function(a){return pa(a)&&!(a===null)},
xa=function(a){return va(a)=="array"},
ya=function(a){var b=va(a);return b=="array"||b=="object"&&typeof a.length=="number"},
za=function(a){return typeof a=="string"},
Aa=function(a){return typeof a=="boolean"},
Ba=function(a){return typeof a=="number"},
Ca=function(a){return va(a)=="function"},
Da=function(a){var b=va(a);return b=="object"||b=="array"||b=="function"},
Ga=function(a){if(a.hasOwnProperty&&a.hasOwnProperty(Ea))return a[Ea];if(!a[Ea])a[Ea]=++Fa;return a[Ea]},
Ea="closure_hashCode_",Fa=0,Ha=function(a){var b=va(a);if(b=="object"||b=="array"){if(a.ra)return a.ra();var c=b=="array"?[]:{};for(var d in a)c[d]=Ha(a[d]);return c}return a},
l=function(a,b){var c=a.jba;if(arguments.length>2){var d=Array.prototype.slice.call(arguments,2);if(c)d.unshift.apply(d,c);c=d}b=a.lba||b;a=a.kba||a;var e,f=b||j;e=c?function(){var g=Array.prototype.slice.call(arguments);g.unshift.apply(g,c);return a.apply(f,g)}:function(){return a.apply(f,
arguments)};
e.jba=c;e.lba=b;e.kba=a;return e},
Ia=function(a){var b=Array.prototype.slice.call(arguments,1);b.unshift(a,null);return l.apply(null,b)},
Ja=function(a,b){for(var c in b)a[c]=b[c]},
Ka=Date.now||function(){return(new Date).getTime()},
m=function(a,b){var c=b||{};for(var d in c)a=a.replace(new RegExp("\\{\\$"+d+"\\}","gi"),c[d]);return a},
La=function(a,b){qa(a,b)},
Ma=function(a,b,c){a[b]=c};
if(!Function.prototype.apply)Function.prototype.apply=function(a,b){var c=[],d,e;if(!a)a=j;if(!b)b=[];for(var f=0;f<b.length;f++)c[f]="args["+f+"]";e="oScope.__applyTemp__.peek().("+c.join(",")+");";if(!a.__applyTemp__)a.__applyTemp__=[];a.__applyTemp__.push(this);d=eval(e);a.__applyTemp__.pop();return d};
Function.prototype.bind=function(a){if(arguments.length>1){var b=Array.prototype.slice.call(arguments,1);b.unshift(this,a);return l.apply(null,b)}else return l(this,a)};
Function.prototype.partial=function(){var a=Array.prototype.slice.call(arguments);a.unshift(this,null);return l.apply(null,a)};
Function.prototype.inherits=function(a){o(this,a)};
var o=function(a,b){function c(){}
c.prototype=b.prototype;a.f=b.prototype;a.prototype=new c;a.prototype.constructor=a};
Function.prototype.mixin=function(a){Ja(this.prototype,a)};var Na=function(a,b){return a.indexOf(b)==0},
Oa=function(a,b){var c=a.length-b.length;return c>=0&&a.lastIndexOf(b,c)==c},
Pa=function(a){for(var b=1;b<arguments.length;b++){var c=String(arguments[b]).replace(/\$/g,"$$$$");a=a.replace(/\%s/,c)}return a};
var Qa=function(a){return/^[\s\xa0]*$/.test(a)},
Sa=function(a){return Qa(Ra(a))},
Ta=function(a){return a.replace(/(\r\n|\r|\n)/g,"\n")},
Ua=function(a){return a.replace(/\xa0|\s/g," ")},
Va=function(a){return a.replace(/^[\s\xa0]+|[\s\xa0]+$/g,"")};
var Wa=function(a){return a.replace(/[\s\xa0]+$/,"")},
Xa=function(a,b){var c=String(a).toLowerCase(),d=String(b).toLowerCase();return c<d?-1:(c==d?0:1)},
Ya=/(\.\d+)|(\d+)|(\D+)/g,Za=function(a,b){if(a==b)return 0;if(!a)return-1;if(!b)return 1;var c=a.toLowerCase().match(Ya),d=b.toLowerCase().match(Ya),e=Math.min(c.length,d.length);for(var f=0;f<e;f++){var g=c[f],h=d[f];if(g!=h){var i=parseInt(g,10);if(!isNaN(i)){var k=parseInt(h,10);if(!isNaN(k)&&i-k)return i-k}return g<h?-1:1}}if(c.length!=d.length)return c.length-d.length;return a<b?-1:1},
$a=/^[a-zA-Z0-9\-_.!~*'()]*$/,ab=function(a){a=String(a);if(!$a.test(a))return encodeURIComponent(a);return a},
bb=function(a){return decodeURIComponent(a.replace(/\+/g," "))},
cb=function(a,b){return a.replace(/(\r\n|\r|\n)/g,b?"<br />":"<br>")},
p=function(a,b){if(b)return a.replace(db,"&amp;").replace(eb,"&lt;").replace(fb,"&gt;").replace(gb,"&quot;");else{if(!hb.test(a))return a;if(a.indexOf("&")!=-1)a=a.replace(db,"&amp;");if(a.indexOf("<")!=-1)a=a.replace(eb,"&lt;");if(a.indexOf(">")!=-1)a=a.replace(fb,"&gt;");if(a.indexOf('"')!=-1)a=a.replace(gb,"&quot;");return a}},
db=/&/g,eb=/</g,fb=/>/g,gb=/\"/g,hb=/[&<>\"]/,lb=function(a){if(ib(a,"&"))return"document"in j&&!ib(a,"<")?jb(a):kb(a);return a},
jb=function(a){var b=j.document.createElement("a");b.innerHTML=a;if(b.normalize)b.normalize();a=b.firstChild.nodeValue;b.innerHTML="";return a},
kb=function(a){return a.replace(/&([^;]+);/g,function(b,c){switch(c){case "amp":return"&";case "lt":return"<";case "gt":return">";case "quot":return'"';default:if(c.charAt(0)=="#"){var d=Number("0"+c.substr(1));if(!isNaN(d))return String.fromCharCode(d)}return b}})},
mb=function(a,b){return cb(a.replace(/  /g," &#160;"),b)},
nb=function(a,b){var c=b.length;for(var d=0;d<c;d++){var e=c==1?b:b.charAt(d);if(a.charAt(0)==e&&a.charAt(a.length-1)==e)return a.substring(1,a.length-1)}return a},
ob=function(a,b,c){if(c)a=lb(a);if(a.length>b)a=a.substring(0,b-3)+"...";if(c)a=p(a);return a},
pb=function(a,b,c){if(c)a=lb(a);if(a.length>b){var d=Math.floor(b/2),e=a.length-d;d+=b%2;a=a.substring(0,d)+"..."+a.substring(e)}if(c)a=p(a);return a},
qb={"\u0008":"\\b","\u000c":"\\f","\n":"\\n","\r":"\\r","\t":"\\t","\u000b":"\\x0B",'"':'\\"',"'":"\\'","\\":"\\\\"},sb=function(a){a=String(a);if(a.quote)return a.quote();else{var b=['"'];for(var c=0;c<a.length;c++)b[c+1]=rb(a.charAt(c));b.push('"');return b.join("")}},
rb=function(a){if(a in qb)return qb[a];var b=a,c=a.charCodeAt(0);if(c>31&&c<127)b=a;else{if(c<256){b="\\x";if(c<16||c>256)b+="0"}else{b="\\u";if(c<4096)b+="0"}b+=c.toString(16).toUpperCase()}return qb[a]=b},
tb=function(a){var b={};for(var c=0;c<a.length;c++)b[a.charAt(c)]=true;return b},
ub=tb("()[]{}+-?*.$^|,:#<!\\"),ib=function(a,b){return a.indexOf(b)!=-1},
vb=function(a){a=String(a);var b="",c;for(var d=0;d<a.length;d++){c=a.charAt(d);if(c=="\u0008")c="\\x08";else if(c in ub)c="\\"+c;b+=c}return b};
var wb=function(a,b,c){var d=Math.floor(a),e=String(d);return(new Array(Math.max(0,b-e.length)+1)).join("0")+(pa(c)?a.toFixed(c):a)},
Ra=function(a){return a==null?"":String(a)},
xb=function(){return Math.floor(Math.random()*2147483648).toString(36)+(Math.floor(Math.random()*2147483648)^(new Date).getTime()).toString(36)},
zb=function(a,b){var c=0,d=String(a).split("."),e=String(b).split("."),f=Math.max(d.length,e.length);for(var g=0;c==0&&g<f;g++){var h=d[g]||"",i=e[g]||"",k=new RegExp("(\\d*)(\\D*)","g"),n=new RegExp("(\\d*)(\\D*)","g");do{var q=k.exec(h)||["","",""],t=n.exec(i)||["","",""];if(q[0].length==0&&t[0].length==0)break;var y=q[1].length==0?0:parseInt(q[1],10),z=t[1].length==0?0:parseInt(t[1],10);c=yb(y,z)||yb(q[2].length==0,t[2].length==0)||yb(q[2],t[2])}while(c==0)}return c},
yb=function(a,b){if(a<b)return-1;else if(a>b)return 1;return 0};m("am");m("pm");var Ab=m("Mon"),Bb=m("Tue"),Cb=m("Wed"),Db=m("Thu"),Eb=m("Fri"),Fb=m("Sat"),Gb=m("Sun"),Hb={0:Ab,1:Bb,2:Cb,3:Db,4:Eb,5:Fb,6:Gb};m("Monday");m("Tuesday");m("Wednesday");m("Thursday");m("Friday");m("Saturday");m("Sunday");m("Jan");m("Feb");m("Mar");m("Apr");m("May");m("Jun");m("Jul");m("Aug");m("Sep");m("Oct");m("Nov");m("Dec");var Ib=m("January"),Jb=m("February"),Kb=m("March"),Lb=m("April"),Mb=m("May"),Nb=m("June"),Ob=m("July"),Pb=m("August"),Qb=m("September"),Rb=m("October"),Sb=m("November"),
Tb=m("December"),Ub={0:Ib,1:Jb,2:Kb,3:Lb,4:Mb,5:Nb,6:Ob,7:Pb,8:Qb,9:Rb,10:Sb,11:Tb};var Vb=function(a,b){switch(b){case 1:return a%4==0&&(a%100!=0||a%400==0)?29:28;case 5:case 8:case 10:case 3:return 30}return 31},
Wb=function(a,b,c,d){var e=new Date(a,b,c),f=d||3,g=e.valueOf()+(f-(e.getDay()+6)%7)*86400000,h=(new Date((new Date(g)).getFullYear(),0,1)).valueOf();return Math.floor(Math.round((g-h)/86400000)/7)+1},
Xb=function(a,b,c,d,e,f){if(za(a)){var g=a,h=b;this.years=g=="y"?h:0;this.months=g=="m"?h:0;this.days=g=="d"?h:0;this.Ac=g=="h"?h:0;this.ld=g=="n"?h:0;this.md=g=="s"?h:0}else{this.years=a||0;this.months=b||0;this.days=c||0;this.Ac=d||0;this.ld=e||0;this.md=f||0}};
Xb.prototype.ap=function(a){var b=Math.min(this.years,this.months,this.days,this.Ac,this.ld,this.md),c=Math.max(this.years,this.months,this.days,this.Ac,this.ld,this.md);if(b<0&&c>0)return null;if(!a&&b==0&&c==0)return"PT0S";var d=[];if(b<0)d.push("-");d.push("P");if(this.years||a)d.push(Math.abs(this.years)+"Y");if(this.months||a)d.push(Math.abs(this.months)+"M");if(this.days||a)d.push(Math.abs(this.days)+"D");if(this.Ac||this.ld||this.md||a){d.push("T");if(this.Ac||a)d.push(Math.abs(this.Ac)+"H");
if(this.ld||a)d.push(Math.abs(this.ld)+"M");if(this.md||a)d.push(Math.abs(this.md)+"S")}return d.join("")};
Xb.prototype.equals=function(a){return a.years==this.years&&a.months==this.months&&a.days==this.days&&a.Ac==this.Ac&&a.ld==this.ld&&a.md==this.md};
Xb.prototype.ra=function(){return new Xb(this.years,this.months,this.days,this.Ac,this.ld,this.md)};
Xb.prototype.add=function(a){this.years+=a.years;this.months+=a.months;this.days+=a.days;this.Ac+=a.Ac;this.ld+=a.ld;this.md+=a.md};
var Yb=function(a,b,c){if(Ba(a))this.N=new Date(a,b||0,c||1);else if(Da(a))this.N=new Date(a.getFullYear(),a.getMonth(),a.getDate());else{this.N=new Date;this.N.setHours(0);this.N.setMinutes(0);this.N.setSeconds(0);this.N.setMilliseconds(0)}this.Bn=0;this.Xt=3};
Yb.prototype.ra=function(){var a=new Yb(this.getFullYear(),this.getMonth(),this.getDate());a.Bn=this.Bn;a.Xt=this.Xt;return a};
Yb.prototype.getFullYear=function(){return this.N.getFullYear()};
Yb.prototype.Cu=function(){return this.getFullYear()};
Yb.prototype.getMonth=function(){return this.N.getMonth()};
Yb.prototype.getDate=function(){return this.N.getDate()};
Yb.prototype.getTime=function(){return this.N.getTime()};
Yb.prototype.getDay=function(){return this.N.getDay()};
Yb.prototype.OY=function(){return(this.N.getDay()+6)%7};
Yb.prototype.jK=function(){return(this.OY()-this.Bn+7)%7};
Yb.prototype.getUTCFullYear=function(){return this.N.getUTCFullYear()};
Yb.prototype.getUTCMonth=function(){return this.N.getUTCMonth()};
Yb.prototype.getUTCDate=function(){return this.N.getUTCDate()};
Yb.prototype.getUTCDay=function(){return this.N.getDay()};
Yb.prototype.mJ=function(){return this.Bn};
Yb.prototype.IJ=function(){return Vb(this.getFullYear(),this.getMonth())};
Yb.prototype.RZ=function(){return Wb(this.getFullYear(),this.getMonth(),this.getDate(),this.Xt)};
Yb.prototype.getTimezoneOffset=function(){return this.N.getTimezoneOffset()};
Yb.prototype.zu=function(){var a,b=this.N.getTimezoneOffset();if(b==0)a="Z";else{var c=Math.abs(b)/60,d=Math.floor(c),e=(c-d)*60;a=(b>0?"-":"+")+wb(d,2)+":"+wb(e,2)}return a};
Yb.prototype.J=function(a){this.N=new Date(a.getFullYear(),a.getMonth(),a.getDate())};
Yb.prototype.setFullYear=function(a){this.N.setFullYear(a)};
Yb.prototype.ZD=function(a){this.setFullYear(a)};
Yb.prototype.setMonth=function(a){this.N.setMonth(a)};
Yb.prototype.setDate=function(a){this.N.setDate(a)};
Yb.prototype.setTime=function(a){return this.N.setTime(a)};
Yb.prototype.setUTCFullYear=function(a){this.N.setUTCFullYear(a)};
Yb.prototype.setUTCMonth=function(a){this.N.setUTCMonth(a)};
Yb.prototype.setUTCDate=function(a){this.N.setUTCDate(a)};
Yb.prototype.u8=function(a){this.Bn=a};
Yb.prototype.add=function(a){if(a.years||a.months){var b=a.years*12,c=a.months+b,d=this.Cu();while(c<-11){--d;c+=12}while(c>11){++d;c-=12}var e=this.getMonth()+c;if(e>11){e=12-e;++d}else if(e<0){e=12+e;--d}var f=this.getDate(),g=Vb(d,e);f=Math.min(g,f);this.setDate(1);this.ZD(d);this.setMonth(e);this.setDate(f)}if(a.days)if(Zb&&!$b)this.setDate(this.getDate()+a.days);else{if(!g){d=this.Cu();e=this.getMonth();g=Vb(d,e)}var h=a.days;while(h!=0){var f=this.getDate();if(h>0){--h;if(f+1<=g)this.setDate(++f);
else{f=1;++e;if(e>11){++d;e=0}this.ZD(d);this.setDate(f);this.setMonth(e);g=Vb(d,e)}}else{++h;if(f-1>0)this.setDate(--f);else{--e;if(e<0){--d;e=11}this.ZD(d);this.setMonth(e);g=Vb(d,e);f=g;this.setDate(f)}}}}};
Yb.prototype.ap=function(a,b){var c=[this.N.getFullYear(),wb(this.N.getMonth()+1,2),wb(this.N.getDate(),2)];return c.join(a?"-":"")+(b?this.zu():"")};
Yb.prototype.equals=function(a){return this.Cu()==a.Cu()&&this.getMonth()==a.getMonth()&&this.getDate()==a.getDate()};
Yb.prototype.toString=function(){return this.ap()};
var ac=function(a,b,c,d,e,f){if(pa(a)){Yb.call(this,a,b,c);if(pa(d))this.setHours(d);if(pa(e))this.setMinutes(e);if(pa(f))this.setSeconds(f)}else{Yb.call(this);this.N=new Date}};
o(ac,Yb);ac.prototype.getHours=function(){return this.N.getHours()};
ac.prototype.getMinutes=function(){return this.N.getMinutes()};
ac.prototype.getSeconds=function(){return this.N.getSeconds()};
ac.prototype.getMilliseconds=function(){return this.N.getMilliseconds()};
ac.prototype.getUTCHours=function(){return this.N.getUTCHours()};
ac.prototype.getUTCMinutes=function(){return this.N.getUTCMinutes()};
ac.prototype.getUTCSeconds=function(){return this.N.getUTCSeconds()};
ac.prototype.getUTCMilliseconds=function(){return this.N.getUTCMilliseconds()};
ac.prototype.setHours=function(a){this.N.setHours(a)};
ac.prototype.setMinutes=function(a){this.N.setMinutes(a)};
ac.prototype.setSeconds=function(a){this.N.setSeconds(a)};
ac.prototype.setMilliseconds=function(a){this.N.setMilliseconds(a)};
ac.prototype.setUTCHours=function(a){this.N.setUTCHours(a)};
ac.prototype.setUTCMinutes=function(a){this.N.setUTCMinutes(a)};
ac.prototype.setUTCSeconds=function(a){this.N.setUTCSeconds(a)};
ac.prototype.setUTCMilliseconds=function(a){this.N.setUTCMilliseconds(a)};
ac.prototype.add=function(a){Yb.prototype.add.call(this,a);if(a.Ac)this.setHours(this.N.getHours()+a.Ac);if(a.ld)this.setMinutes(this.N.getMinutes()+a.ld);if(a.md)this.setSeconds(this.N.getSeconds()+a.md)};
ac.prototype.ap=function(a,b){if(a)return this.N.getFullYear()+"-"+wb(this.N.getMonth()+1,2)+"-"+wb(this.N.getDate(),2)+" "+wb(this.N.getHours(),2)+":"+wb(this.N.getMinutes(),2)+":"+wb(this.N.getSeconds(),2)+(b?this.zu():"");return this.N.getFullYear()+wb(this.N.getMonth()+1,2)+wb(this.N.getDate(),2)+"T"+wb(this.N.getHours(),2)+wb(this.N.getMinutes(),2)+wb(this.N.getSeconds(),2)+(b?this.zu():"")};
ac.prototype.K$=function(a){return this.N.getFullYear()+"-"+wb(this.N.getMonth()+1,2)+"-"+wb(this.N.getDate(),2)+"T"+wb(this.N.getHours(),2)+":"+wb(this.N.getMinutes(),2)+":"+wb(this.N.getSeconds(),2)+(a?this.zu():"")};
ac.prototype.equals=function(a){return this.getTime()==a.getTime()};
ac.prototype.toString=function(){return this.ap()};
ac.prototype.ra=function(){var a=new ac(this.getFullYear(),this.getMonth(),this.getDate(),this.getHours(),this.getMinutes(),this.getSeconds());a.Bn=this.Bn;a.Xt=this.Xt;return a};var bc=function(a){return a[a.length-1]},
cc=function(a,b,c){if(a.indexOf)return a.indexOf(b,c);if(Array.indexOf)return Array.indexOf(a,b,c);var d=c==null?0:(c<0?Math.max(0,a.length+c):c);for(var e=d;e<a.length;e++)if(e in a&&a[e]===b)return e;return-1},
r=function(a,b,c){if(a.forEach)a.forEach(b,c);else if(Array.forEach)Array.forEach(a,b,c);else{var d=a.length,e=za(a)?a.split(""):a;for(var f=0;f<d;f++)if(f in e)b.call(c,e[f],f,a)}},
ec=function(a,b,c){var d=a.length,e=za(a)?a.split(""):a;for(var f=d-1;f>=0;--f)if(f in e)b.call(c,e[f],f,a)},
fc=function(a,b,c){if(a.filter)return a.filter(b,c);if(Array.filter)return Array.filter(a,b,c);var d=a.length,e=[],f=0,g=za(a)?a.split(""):a;for(var h=0;h<d;h++)if(h in g){var i=g[h];if(b.call(c,i,h,a))e[f++]=i}return e},
gc=function(a,b,c){if(a.map)return a.map(b,c);if(Array.map)return Array.map(a,b,c);var d=a.length,e=[],f=0,g=za(a)?a.split(""):a;for(var h=0;h<d;h++)if(h in g)e[f++]=b.call(c,g[h],h,a);return e},
hc=function(a,b,c,d){if(a.reduce)return d?a.reduce(l(b,d),c):a.reduce(b,c);var e=c;r(a,function(f,g){e=b.call(d,e,f,g,a)});
return e},
ic=function(a,b,c){if(a.some)return a.some(b,c);if(Array.some)return Array.some(a,b,c);var d=a.length,e=za(a)?a.split(""):a;for(var f=0;f<d;f++)if(f in e&&b.call(c,e[f],f,a))return true;return false},
jc=function(a,b,c){if(a.every)return a.every(b,c);if(Array.every)return Array.every(a,b,c);var d=a.length,e=za(a)?a.split(""):a;for(var f=0;f<d;f++)if(f in e&&!b.call(c,e[f],f,a))return false;return true},
lc=function(a,b,c){var d=kc(a,b,c);return d<0?null:(za(a)?a.charAt(d):a[d])},
kc=function(a,b,c){var d=a.length,e=za(a)?a.split(""):a;for(var f=0;f<d;f++)if(f in e&&b.call(c,e[f],f,a))return f;return-1},
mc=function(a,b){if(a.contains)return a.contains(b);return cc(a,b)>-1},
nc=function(a){return a.length==0},
oc=function(a){if(!xa(a))for(var b=a.length-1;b>=0;b--)delete a[b];a.length=0},
pc=function(a,b){if(!mc(a,b))a.push(b)},
rc=function(a,b,c){qc(a,c,0,b)},
tc=function(a,b){var c=cc(a,b),d;if(d=c!=-1)sc(a,c);return d},
sc=function(a,b){return Array.prototype.splice.call(a,b,1).length==1},
uc=function(a){if(xa(a))return a.concat();else{var b=[];for(var c=0,d=a.length;c<d;c++)b[c]=a[c];return b}},
vc=function(a){for(var b=1;b<arguments.length;b++){var c=arguments[b];if(!xa(c))a.push(c);else a.push.apply(a,c)}},
qc=function(a){return Array.prototype.splice.apply(a,wc(arguments,1))},
wc=function(a,b,c){return arguments.length<=2?Array.prototype.slice.call(a,b):Array.prototype.slice.call(a,b,c)},
yc=function(a,b,c){var d=0,e=a.length-1,f=c||xc;while(d<=e){var g=d+e>>1,h=f(b,a[g]);if(h>0)d=g+1;else if(h<0)e=g-1;else return g}return-(d+1)},
xc=function(a,b){return a>b?1:(a<b?-1:0)},
zc=function(a,b,c){var d=yc(a,b,c);if(d<0){rc(a,b,-(d+1));return true}return false},
Ac=function(a,b,c){var d=yc(a,b,c);return d>=0?sc(a,d):false};if("StopIteration"in j)var Bc=j.StopIteration;else Bc=Error("StopIteration");var Cc=function(){};
Cc.prototype.next=function(){throw Bc;};
Cc.prototype.__iterator__=function(){return this};
var Dc=function(a){if(a instanceof Cc)return a;if(typeof a.__iterator__=="function")return a.__iterator__(false);if(ya(a)){var b=0,c=new Cc;c.next=function(){while(true){if(b>=a.length)throw Bc;if(!(b in a)){b++;continue}return a[b++]}};
return c}throw Error("Not implemented");},
Ec=function(a,b,c){if(ya(a))try{r(a,b,c)}catch(d){if(d!==Bc)throw d;}else{a=Dc(a);try{while(true)b.call(c,a.next(),undefined,a)}catch(d){if(d!==Bc)throw d;}}},
Fc=function(a,b,c){a=Dc(a);try{while(true)if(b.call(c,a.next(),undefined,a))return true}catch(d){if(d!==Bc)throw d;}return false};var Gc=function(a,b,c){for(var d in a)b.call(c,a[d],d,a)},
Hc=function(a){var b=0;for(var c in a)b++;return b},
Ic=function(a){var b=[];for(var c in a)b.push(a[c]);return b},
Jc=function(a){var b=[];for(var c in a)b.push(c);return b},
Kc=function(a,b){return b in a},
Lc=function(a,b){for(var c in a)if(a[c]==b)return true;return false},
Mc=function(a){for(var b in a)return false;return true},
Oc=function(a){var b=Jc(a);for(var c=b.length-1;c>=0;c--)Nc(a,b[c])},
Nc=function(a,b){var c;if(c=b in a)delete a[b];return c},
Qc=function(a,b,c){if(b in a)throw Error('The object already contains the key "'+b+'"');Pc(a,b,c)},
Rc=function(a,b,c){if(b in a)return a[b];return c},
Pc=function(a,b,c){a[b]=c},
Sc=function(a){var b={};for(var c in a)b[c]=a[c];return b},
Tc=["constructor","hasOwnProperty","isPrototypeOf","propertyIsEnumerable","toLocaleString","toString","valueOf"],Uc=function(a){var b,c;for(var d=1;d<arguments.length;d++){c=arguments[d];for(b in c)a[b]=c[b];for(var e=0;e<Tc.length;e++){b=Tc[e];if(Object.prototype.hasOwnProperty.call(c,b))a[b]=c[b]}}};var Vc=function(a){if(typeof a.S=="function")return a.S();if(ya(a)||za(a))return a.length;return Hc(a)},
Wc=function(a){if(typeof a.xb=="function")return a.xb();if(za(a))return a.split("");if(ya(a)){var b=[],c=a.length;for(var d=0;d<c;d++)b.push(a[d]);return b}return Ic(a)},
Xc=function(a){if(typeof a.tc=="function")return a.tc();if(typeof a.xb=="function")return undefined;if(ya(a)||za(a)){var b=[],c=a.length;for(var d=0;d<c;d++)b.push(d);return b}return Jc(a)},
Yc=function(a,b){if(typeof a.contains=="function")return a.contains(b);if(typeof a.gh=="function")return a.gh(b);if(ya(a)||za(a))return mc(a,b);return Lc(a,b)},
Zc=function(a){if(typeof a.isEmpty=="function")return a.isEmpty();if(ya(a)||za(a))return nc(a);return Mc(a)},
$c=function(a){if(typeof a.clear=="function")a.clear();else if(ya(a))oc(a);else Oc(a)},
ad=function(a,b,c){if(typeof a.forEach=="function")a.forEach(b,c);else if(ya(a)||za(a))r(a,b,c);else{var d=Xc(a),e=Wc(a),f=e.length;for(var g=0;g<f;g++)b.call(c,e[g],d&&d[g],a)}},
bd=function(a,b,c){if(typeof a.some=="function")return a.some(b,c);if(ya(a)||za(a))return ic(a,b,c);var d=Xc(a),e=Wc(a),f=e.length;for(var g=0;g<f;g++)if(b.call(c,e[g],d&&d[g],a))return true;return false},
cd=function(a,b,c){if(typeof a.every=="function")return a.every(b,c);if(ya(a)||za(a))return jc(a,b,c);var d=Xc(a),e=Wc(a),f=e.length;for(var g=0;g<f;g++)if(!b.call(c,e[g],d&&d[g],a))return false;return true};var dd=function(a){this.o={};this.Pa=[];if(a)this.jj(a)};
dd.prototype.Ma=0;dd.prototype.ws=0;dd.prototype.S=function(){return this.Ma};
dd.prototype.xb=function(){this.gt();var a=[];for(var b=0;b<this.Pa.length;b++){var c=this.Pa[b];a.push(this.o[c])}return a};
dd.prototype.tc=function(){this.gt();return this.Pa.concat()};
dd.prototype.Ob=function(a){return ed(this.o,a)};
dd.prototype.gh=function(a){for(var b=0;b<this.Pa.length;b++){var c=this.Pa[b];if(ed(this.o,c)&&this.o[c]==a)return true}return false};
dd.prototype.isEmpty=function(){return this.Ma==0};
dd.prototype.clear=function(){this.o={};this.Pa.length=0;this.Ma=0;this.ws=0};
dd.prototype.remove=function(a){if(ed(this.o,a)){delete this.o[a];this.Ma--;this.ws++;if(this.Pa.length>2*this.Ma)this.gt();return true}return false};
dd.prototype.gt=function(){if(this.Ma!=this.Pa.length){var a=0,b=0;while(a<this.Pa.length){var c=this.Pa[a];if(ed(this.o,c))this.Pa[b++]=c;a++}this.Pa.length=b}if(this.Ma!=this.Pa.length){var d={},a=0,b=0;while(a<this.Pa.length){var c=this.Pa[a];if(!ed(d,c)){this.Pa[b++]=c;d[c]=1}a++}this.Pa.length=b}};
dd.prototype.get=function(a,b){if(ed(this.o,a))return this.o[a];return b};
dd.prototype.J=function(a,b){if(!ed(this.o,a)){this.Ma++;this.Pa.push(a);this.ws++}this.o[a]=b};
dd.prototype.jj=function(a){var b,c;if(a instanceof dd){b=a.tc();c=a.xb()}else{b=Jc(a);c=Ic(a)}for(var d=0;d<b.length;d++)this.J(b[d],c[d])};
dd.prototype.ra=function(){return new dd(this)};
dd.prototype.__iterator__=function(a){this.gt();var b=0,c=this.Pa,d=this.o,e=this.ws,f=this,g=new Cc;g.next=function(){while(true){if(e!=f.ws)throw Error("The map has changed since the iterator was created");if(b>=c.length)throw Bc;var h=c[b++];return a?h:d[h]}};
return g};
if(Object.prototype.hasOwnProperty)var ed=function(a,b){return Object.prototype.hasOwnProperty.call(a,b)};
else ed=function(a,b){return b in a&&a[b]!==Object.prototype[b]};
var fd=function(a,b){if(typeof a.Ob=="function")return a.Ob(b);if(ya(a))return b<a.length;return Kc(a,b)},
gd=function(a,b,c){if(typeof a.get=="function")return a.get(b,c);if(fd(a,b))return a[b];return c};var hd=function(a){this.o=new dd;if(a)this.jj(a)},
id=function(a){var b=typeof a;return b=="object"?"o"+Ga(a):b.substr(0,1)+a};
hd.prototype.S=function(){return this.o.S()};
hd.prototype.add=function(a){this.o.J(id(a),a)};
hd.prototype.jj=function(a){var b=jd(a),c=b.length;for(var d=0;d<c;d++)this.add(b[d])};
hd.prototype.ma=function(a){var b=jd(a),c=b.length;for(var d=0;d<c;d++)this.remove(b[d])};
hd.prototype.remove=function(a){return this.o.remove(id(a))};
hd.prototype.clear=function(){this.o.clear()};
hd.prototype.isEmpty=function(){return this.o.isEmpty()};
hd.prototype.contains=function(a){return this.o.Ob(id(a))};
hd.prototype.intersection=function(a){var b=new hd,c=jd(a);for(var d=0;d<c.length;d++){var e=c[d];if(this.contains(e))b.add(e)}return b};
hd.prototype.xb=function(){return this.o.xb()};
hd.prototype.ra=function(){return new hd(this)};
hd.prototype.equals=function(a){return this.S()!=Vc(a)?false:this.T2(a)};
hd.prototype.T2=function(a){var b=Vc(a);if(this.S()>b)return false;if(!(a instanceof hd)&&b>5)a=new hd(a);return cd(this,function(c){return Yc(a,c)})};
hd.prototype.__iterator__=function(){return this.o.__iterator__(false)};
var jd=function(a){return Wc(a)};var kd=function(a){var b=[];for(var c=0;c<a.length;c++)if(xa(a[c]))b.push(kd(a[c]));else b.push(a[c]);return"[ "+b.join(", ")+" ]"},
nd=function(a,b){try{var c=ld(a),d="Message: "+p(c.message)+'\nUrl: <a href="view-source:'+c.fileName+'" target="_new">'+c.fileName+"</a>\nLine: "+c.lineNumber+"\n\nBrowser stack:\n"+p(c.stack+"-> ")+"[end]\n\nJS stack traversal:\n"+p(md(b)+"-> ");return d}catch(e){return"Exception trying to expose exception! You win, we lose. "+e}},
ld=function(a){var b=ra("document.location.href");return typeof a=="string"?{message:a,name:"Unknown error",lineNumber:"Not available",fileName:b,stack:"Not available"}:(!a.lineNumber||!a.fileName||!a.stack?{message:a.message,name:a.name,lineNumber:a.lineNumber||a.line||"Not available",fileName:a.fileName||a.sourceURL||b,stack:a.stack||"Not available"}:a)},
md=function(a){return od(a||arguments.callee.caller,[])},
od=function(a,b){var c=[];if(mc(b,a))c.push("[...circular reference...]");else if(a&&b.length<50){c.push(pd(a)+"(");var d=a.arguments;for(var e=0;e<d.length;e++){if(e>0)c.push(", ");var f,g=d[e];switch(typeof g){case "object":f=g?"object":"null";break;case "string":f=g;break;case "number":f=String(g);break;case "boolean":f=g?"true":"false";break;case "function":f=pd(g);f=f?f:"[fn]";break;case "undefined":default:f=typeof g;break}if(f.length>40)f=f.substr(0,40)+"...";c.push(f)}b.push(a);c.push(")\n");
try{c.push(od(a.caller,b))}catch(h){c.push("[exception trying to get caller]\n")}}else if(a)c.push("[...long stack...]");else c.push("[end]");return c.join("")},
pd=function(a){var b=String(a);if(!qd[b]){var c=/function ([^\(]+)/.exec(b);if(c){var d=c[1],e=/^\$(.+)\$$/.exec(d);if(e)d=e[1].replace(/\${1,2}/g,".");qd[b]=d}else qd[b]="[Anonymous]"}return qd[b]},
rd=function(a,b,c,d){if(ra("document.all"))return"";var e=b||j,f=c||"",g=d||0;if(e==a)return f;for(var h in e){if(h=="Packages"||h=="sun"||h=="netscape"||h=="java")continue;if(e[h]==a)return f+h;if((typeof e[h]=="function"||typeof e[h]=="object")&&e[h]!=j&&e[h]!=ra("document")&&e.hasOwnProperty(h)&&g<6){var i=rd(a,e[h],f+h+".",g+1);if(i)return i}}return""},
qd={};var td=function(a,b,c){this.Nfa=sd++;this.Fea=Ka();this.Di=a;this.w4=b;this.Vca=c};
td.prototype.qI=null;td.prototype.pI=null;var sd=0;td.prototype.PA=function(){return this.Vca};
td.prototype.iJ=function(){return this.qI};
td.prototype.p8=function(a){this.qI=a};
td.prototype.jJ=function(){return this.pI};
td.prototype.q8=function(a){this.pI=a};
td.prototype.jq=function(){return this.Di};
td.prototype.ox=function(a){this.Di=a};
td.prototype.EJ=function(){return this.w4};
td.prototype.UD=function(a){this.w4=a};
td.prototype.FJ=function(){return this.Fea};var ud=function(a){this.Og=a;this.P=null;this.Va={};this.Pn=[]};
ud.prototype.Di=null;var vd=function(a,b){this.name=a;this.value=b};
vd.prototype.toString=function(){return this.name};
new vd("OFF",Infinity);var wd=new vd("SHOUT",1200),xd=new vd("SEVERE",1000),yd=new vd("WARNING",900),zd=new vd("INFO",800),Ad=new vd("CONFIG",700),Bd=new vd("FINE",500),Cd=new vd("FINER",400),Dd=new vd("FINEST",300);new vd("ALL",0);var Fd=function(a){return Ed(a)};
ud.prototype.getName=function(){return this.Og};
ud.prototype.NF=function(a){this.Pn.push(a)};
ud.prototype.TN=function(a){return tc(this.Pn,a)};
ud.prototype.Ia=function(){return this.P};
ud.prototype.ox=function(a){this.Di=a};
ud.prototype.jq=function(){return this.Di};
ud.prototype.vY=function(){if(this.Di)return this.Di;if(this.P)return this.P.vY();return null};
ud.prototype.qv=function(a){if(this.Di)return a.value>=this.Di.value;if(this.P)return this.P.qv(a);return false};
ud.prototype.log=function(a,b,c){if(!this.qv(a))return;var d=new td(a,String(b),this.Og);if(c){d.p8(c);d.q8(nd(c,arguments.callee.caller))}this.u3(d)};
ud.prototype.Zr=function(a,b){this.log(xd,a,b)};
ud.prototype.warning=function(a,b){this.log(yd,a,b)};
ud.prototype.info=function(a,b){this.log(zd,a,b)};
ud.prototype.Zb=function(a,b){this.log(Bd,a,b)};
ud.prototype.jf=function(a,b){this.log(Cd,a,b)};
ud.prototype.kf=function(a,b){this.log(Dd,a,b)};
ud.prototype.u3=function(a){if(!this.qv(a.jq()))return;var b=this;while(b){b.ST(a);b=b.Ia()}};
ud.prototype.ST=function(a){for(var b=0;b<this.Pn.length;b++)this.Pn[b](a)};
ud.prototype.X8=function(a){this.P=a};
ud.prototype.CS=function(a,b){this.Va[a]=b};
var Gd={},Hd=null,Id=function(){if(!Hd){Hd=new ud("");Gd[""]=Hd;Hd.ox(Ad)}},
Jd=function(){Id();return Hd},
Ed=function(a){Id();return a in Gd?Gd[a]:Kd(a)},
Kd=function(a){var b=new ud(a),c=a.split("."),d=c[c.length-1];c.length=c.length-1;var e=c.join("."),f=Ed(e);f.CS(d,b);b.X8(f);Gd[a]=b;return b};var Ld=function(){this.q6=Ka()},
Md=new Ld;Ld.prototype.J=function(a){this.q6=a};
Ld.prototype.reset=function(){this.J(Ka())};
Ld.prototype.get=function(){return this.q6};var Nd=function(a){this.TC=a||"";this.g$=Md};
Nd.prototype.$D=true;Nd.prototype.PP=true;Nd.prototype.KP=true;Nd.prototype.as=false;Nd.prototype.Dn=function(){throw Error("Must override formatRecord");};
var Pd=function(a){var b=new Date(a.FJ());return Od(b.getFullYear()-2000)+Od(b.getMonth()+1)+Od(b.getDate())+" "+Od(b.getHours())+":"+Od(b.getMinutes())+":"+Od(b.getSeconds())+"."+Od(Math.floor(b.getMilliseconds()/10))},
Od=function(a){if(a<10)return"0"+a;return String(a)},
Qd=function(a,b){var c=a.FJ()-b,d=c/1000,e=d.toFixed(3),f=0;if(d<1)f=2;else while(d<100){f++;d*=10}while(f-- >0)e=" "+e;return e},
Rd=function(a){Nd.call(this,a)};
o(Rd,Nd);Rd.prototype.as=true;Rd.prototype.Dn=function(a){var b;switch(a.jq().value){case wd.value:b="dbg-sh";break;case xd.value:b="dbg-sev";break;case yd.value:b="dbg-w";break;case zd.value:b="dbg-i";break;case Bd.value:default:b="dbg-f";break}var c=[];c.push(this.TC," ");if(this.$D)c.push("[",Pd(a),"] ");if(this.PP)c.push("[",mb(Qd(a,this.g$.get())),"s] ");if(this.KP)c.push("[",p(a.PA()),"] ");c.push('<span class="',b,'">',cb(mb(p(a.EJ()))));if(this.as&&a.iJ())c.push("<br>",cb(mb(a.jJ())));c.push("</span><br>");
return c.join("")};
var Sd=function(a){Nd.call(this,a)};
o(Sd,Nd);Sd.prototype.Dn=function(a){var b=[];b.push(this.TC," ");if(this.$D)b.push("[",Pd(a),"] ");if(this.PP)b.push("[",Qd(a,this.g$.get()),"s] ");if(this.KP)b.push("[",a.PA(),"] ");b.push(a.EJ(),"\n");if(this.as&&a.iJ())b.push(a.jJ(),"\n");return b.join("")};var Td=function(a){this.Yq=a||100;this.Pk=[]};
Td.prototype.or=0;Td.prototype.add=function(a){this.Pk[this.or]=a;this.or=(this.or+1)%this.Yq};
Td.prototype.get=function(a){a=this.KM(a);return this.Pk[a]};
Td.prototype.J=function(a,b){a=this.KM(a);this.Pk[a]=b};
Td.prototype.S=function(){return this.Pk.length};
Td.prototype.isEmpty=function(){return this.Pk.length==0};
Td.prototype.clear=function(){this.Pk.length=0;this.or=0};
Td.prototype.xb=function(){return this.$Y(this.S())};
Td.prototype.$Y=function(a){var b=this.S(),c=this.S()-a,d=[];for(var e=c;e<b;e++)d[e]=this.get(e);return d};
Td.prototype.tc=function(){var a=[],b=this.S();for(var c=0;c<b;c++)a[c]=c;return a};
Td.prototype.Ob=function(a){return a<this.S()};
Td.prototype.gh=function(a){var b=this.S();for(var c=0;c<b;c++)if(this.get(c)==a)return true;return false};
Td.prototype.KM=function(a){if(a>=this.Pk.length)throw Error("Out of bounds exception");if(this.Pk.length<this.Yq)return a;return(this.or+Number(a))%this.Yq};var Ud=function(a,b){this.vB=a||"";this.TC=b||"";this.dN=[];this.rO=new Td(500);this.Iw=l(this.nw,this);this.Zt=new Rd(this.TC);this.bca={};this.fx(true);this.Ea=this.SI("enabled")=="1";j.setInterval(l(this.o7,this),7500)};
Ud.prototype.Jaa="LOGGING";Ud.prototype.Pc=null;Ud.prototype.fF=false;Ud.prototype.iv=false;Ud.prototype.sG=null;Ud.prototype.LL=Ka();Ud.prototype.Vd=function(){if(this.Ea)this.GC()};
Ud.prototype.sb=function(){return this.Ea};
Ud.prototype.Ca=function(a){this.Ea=a;if(this.Ea){this.GC();if(this.Pc)this.jR()}this.RO("enabled",a?1:0)};
Ud.prototype.fx=function(a){if(a==this.iv)return;this.iv=a;var b=Jd();if(a)b.NF(this.Iw);else b.TN(this.Iw)};
Ud.prototype.nw=function(a){if(this.bca[a.PA()])return;var b=this.Zt.Dn(a);this.Oaa(b)};
Ud.prototype.Oaa=function(a){if(this.Ea){this.GC();this.rO.add(a);this.gF(a)}else this.rO.add(a)};
Ud.prototype.gF=function(a){this.dN.push(a);j.clearTimeout(this.sG);if(Ka()-this.LL>750)this.iR();else this.sG=j.setTimeout(l(this.iR,this),250)};
Ud.prototype.iR=function(){this.LL=Ka();if(this.Pc){var a=this.Pc.document.body,b=a&&a.scrollHeight-(a.scrollTop+a.clientHeight)<=100;this.Pc.document.write(this.dN.join(""));this.dN.length=0;if(b)this.Pc.scrollTo(0,1000000)}};
Ud.prototype.Naa=function(){var a=this.rO.xb();for(var b=0;b<a.length;b++)this.gF(a[b])};
Ud.prototype.GC=function(){if(this.Pc&&!this.Pc.closed||this.fF)return;var a=this.SI("dbg","0,0,800,500").split(","),b=Number(a[0]),c=Number(a[1]),d=Number(a[2]),e=Number(a[3]);this.fF=true;this.Pc=window.open("","dbg"+this.vB,"width="+d+",height="+e+",toolbar=no,resizable=yes,scrollbars=yes,left="+b+",top="+c+",status=no,screenx="+b+",screeny="+c);if(!this.Pc)if(!this.mea){alert("Logger popup was blocked");this.mea=true}this.fF=false;if(this.Pc)this.jR()};
Ud.prototype.jR=function(){if(!this.Pc)return;this.Pc.document.open();var a='<style>*{font:normal 14px monospace;}.dbg-sev{color:#F00}.dbg-w{color:#E92}.dbg-sh{font-weight:bold;color:#000}.dbg-i{color:#666}.dbg-f{color:#999}.dbg-ev{color:#0A0}.dbg-m{color:#990}</style><hr><div class="dbg-ev" style="text-align:center">'+this.Jaa+"<br><small>Logger: "+this.vB+"</small></div><hr>";this.gF(a);this.Naa()};
Ud.prototype.RO=function(a,b){a+=this.vB;document.cookie=a+"="+encodeURIComponent(b)+";expires="+(new Date(Ka()+2592000000)).toUTCString()};
Ud.prototype.SI=function(a,b){a+=this.vB;var c=String(document.cookie),d=c.indexOf(a+"=");if(d!=-1){var e=c.indexOf(";",d);return decodeURIComponent(c.substring(d+a.length+1,e==-1?c.length:e))}else return b||""};
Ud.prototype.o7=function(){if(!this.Pc||this.Pc.closed)return;var a=this.Pc.screenX||this.Pc.screenLeft||0,b=this.Pc.screenY||this.Pc.screenTop||0,c=this.Pc.outerWidth||800,d=this.Pc.outerHeight||500;this.RO("dbg",a+","+b+","+c+","+d)};var Vd=function(){};
Vd.prototype.Qz=false;Vd.prototype.W=function(){return this.Qz};
Vd.prototype.b=function(){if(!this.Qz)this.Qz=true};var Wd=function(a,b){Vd.call(this);this.Ii=b;this.Rd=[];this.nV(a)};
o(Wd,Vd);Wd.prototype.yz=null;Wd.prototype.Oz=null;Wd.prototype.Pr=function(a){this.yz=a};
Wd.prototype.WO=function(a){this.Oz=a};
Wd.prototype.Sd=function(){if(this.Rd.length)return this.Rd.pop();return this.ug()};
Wd.prototype.Ae=function(a){if(this.Rd.length<this.Ii)this.Rd.push(a);else this.xg(a)};
Wd.prototype.nV=function(a){if(a>this.Ii)throw Error("[goog.structs.SimplePool] Initial cannot be greater than max");for(var b=0;b<a;b++)this.Rd.push(this.ug())};
Wd.prototype.ug=function(){return this.yz?this.yz():{}};
Wd.prototype.xg=function(a){if(this.Oz)this.Oz(a);else if(Ca(a.b))a.b();else for(var b in a)delete a[b]};
Wd.prototype.b=function(){if(!this.W()){Wd.f.b.call(this);var a=this.Rd;while(a.length)this.xg(a.pop());this.Rd=null}};var Zd=function(){this.Rt=[];this.eN=new dd;this.mg=0;this.U$=0;this.T$=0;this.S$=0;this.aj=new dd;this.R$=0;this.zU=0;this.fk=1;this.oI=new Wd(0,4000);this.oI.ug=function(){return new Xd};
this.i$=new Wd(0,50);this.i$.ug=function(){return new Yd};
var a=this;this.hL=new Wd(0,2000);this.hL.ug=function(){return String(a.fk++)};
this.hL.xg=function(){};
this.Jba=3};
Zd.prototype.Ba=Fd("goog.debug.Trace");var Yd=function(){this.count=0;this.time=0;this.varAlloc=0};
Yd.prototype.toString=function(){var a=[];a.push(this.type," ",this.count," (",Math.round(this.time*10)/10," ms)");if(this.varAlloc)a.push(" [VarAlloc = ",this.varAlloc,"]");return a.join("")};
var Xd=function(){};
Xd.prototype.J$=function(a,b,c){var d=[];if(b==-1)d.push("    ");else d.push($d(this.eventTime-b));d.push(" ",ae(this.eventTime-a));if(this.eventType==0)d.push(" Start        ");else if(this.eventType==1){d.push(" Done ");var e=this.stopTime-this.startTime;d.push($d(e)," ms ")}else d.push(" Comment      ");d.push(c,this);if(this.totalVarAlloc>0)d.push("[VarAlloc ",this.totalVarAlloc,"] ");return d.join("")};
Xd.prototype.toString=function(){return this.type==null?this.comment:"["+this.type+"] "+this.comment};
Zd.prototype.reset=function(a){this.Jba=a;for(var b=0;b<this.Rt.length;b++){var c=this.oI.id;if(c)this.hL.Ae(c);this.oI.Ae(this.Rt[b])}this.Rt.length=0;this.eN.clear();this.mg=be();this.U$=0;this.T$=0;this.S$=0;this.R$=0;this.zU=0;var d=this.aj.tc();for(var b=0;b<d.length;b++){var e=d[b],f=this.aj.get(e);f.count=0;f.time=0;f.varAlloc=0;this.i$.Ae(f)}this.aj.clear()};
Zd.prototype.toString=function(){var a=[],b=-1,c=[];for(var d=0;d<this.Rt.length;d++){var e=this.Rt[d];if(e.eventType==1)c.pop();a.push(" ",e.J$(this.mg,b,c.join("")));b=e.eventTime;a.push("\n");if(e.eventType==0)c.push("|  ")}if(this.eN.S()!=0){var f=be();a.push(" Unstopped timers:\n");Ec(this.eN,function(i){a.push("  ",i," (",f-i.startTime," ms, started at ",ae(i.startTime),")\n")})}var g=this.aj.tc();
for(var d=0;d<g.length;d++){var h=this.aj.get(g[d]);if(h.count>1)a.push(" TOTAL ",h,"\n")}a.push("Total tracers created ",this.R$,"\n","Total comments created ",this.zU,"\n","Overhead start: ",this.U$," ms\n","Overhead end: ",this.T$," ms\n","Overhead comment: ",this.S$," ms\n");return a.join("")};
var $d=function(a){a=Math.round(a);var b="";if(a<1000)b=" ";if(a<100)b="  ";if(a<10)b="   ";return b+a},
ae=function(a){a=Math.round(a);var b=a/1000%60,c=a%1000;return String(100+b).substring(1,3)+"."+String(1000+c).substring(1,4)},
be=function(){return Ka()};
new Zd;var ce=function(a,b,c,d){this.top=pa(a)?Number(a):undefined;this.right=pa(b)?Number(b):undefined;this.bottom=pa(c)?Number(c):undefined;this.left=pa(d)?Number(d):undefined},
de=function(){var a=new ce(arguments[0].y,arguments[0].x,arguments[0].y,arguments[0].x);for(var b=1;b<arguments.length;b++){var c=arguments[b];a.top=Math.min(a.top,c.y);a.right=Math.max(a.right,c.x);a.bottom=Math.max(a.bottom,c.y);a.left=Math.min(a.left,c.x)}return a};
ce.prototype.ra=function(){return new ce(this.top,this.right,this.bottom,this.left)};
ce.prototype.toString=function(){return"("+this.top+"t, "+this.right+"r, "+this.bottom+"b, "+this.left+"l)"};
ce.prototype.contains=function(a){return ee(this,a)};
ce.prototype.expand=function(a,b,c,d){if(Da(a)){this.top-=a.top;this.right+=a.right;this.bottom+=a.bottom;this.left-=a.left}else{this.top-=a;this.right+=b;this.bottom+=c;this.left-=d}return this};
var ee=function(a,b){if(!a||!b)return false;return b.x>=a.left&&b.x<=a.right&&b.y>=a.top&&b.y<=a.bottom},
he=function(a,b){if(b.x>=a.left&&b.x<=a.right){if(b.y>=a.top&&b.y<=a.bottom)return 0;return b.y<a.top?a.top-b.y:b.y-a.bottom}if(b.y>=a.top&&b.y<=a.bottom)return b.x<a.left?a.left-b.x:b.x-a.right;return fe(b,new ge(b.x<a.left?a.left:a.right,b.y<a.top?a.top:a.bottom))};var ge=function(a,b){this.x=pa(a)?Number(a):undefined;this.y=pa(b)?Number(b):undefined};
ge.prototype.ra=function(){return new ge(this.x,this.y)};
ge.prototype.toString=function(){return"("+this.x+", "+this.y+")"};
var fe=function(a,b){var c=a.x-b.x,d=a.y-b.y;return Math.sqrt(c*c+d*d)},
ie=function(a,b){return new ge(a.x-b.x,a.y-b.y)};var je=function(a,b){a=Number(a);b=Number(b);this.start=a<b?a:b;this.end=a<b?b:a};
je.prototype.ra=function(){return new je(this.start,this.end)};
je.prototype.toString=function(){return"["+this.start+", "+this.end+"]"};var ke=function(a,b,c,d){this.left=pa(a)?Number(a):undefined;this.top=pa(b)?Number(b):undefined;this.width=pa(c)?Number(c):undefined;this.height=pa(d)?Number(d):undefined};
ke.prototype.ra=function(){return new ke(this.left,this.top,this.width,this.height)};
ke.prototype.qQ=function(){return new ce(this.top,this.left+this.width||undefined,this.top+this.height||undefined,this.left)};
ke.prototype.toString=function(){return"("+this.left+", "+this.top+" - "+this.width+"w x "+this.height+"h)"};
var le=function(a,b){if(a==b)return true;if(!a||!b)return false;return a.left==b.left&&a.width==b.width&&a.top==b.top&&a.height==b.height};
ke.prototype.intersection=function(a){var b=Math.max(this.left,a.left),c=Math.min(this.left+this.width,a.left+a.width);if(b<=c){var d=Math.max(this.top,a.top),e=Math.min(this.top+this.height,a.top+a.height);if(d<=e){this.left=b;this.top=d;this.width=c-b;this.height=e-d;return true}}return false};
var me=function(a,b){var c=Math.max(a.left,b.left),d=Math.min(a.left+a.width,b.left+b.width);if(c<=d){var e=Math.max(a.top,b.top),f=Math.min(a.top+a.height,b.top+b.height);if(e<=f)return true}return false};
ke.prototype.intersects=function(a){return me(this,a)};var ne=function(a,b){this.width=pa(a)?Number(a):undefined;this.height=pa(b)?Number(b):undefined},
oe=function(a,b){if(a==b)return true;if(!a||!b)return false;return a.width==b.width&&a.height==b.height};
ne.prototype.ra=function(){return new ne(this.width,this.height)};
ne.prototype.toString=function(){return"("+this.width+" x "+this.height+")"};
ne.prototype.pT=function(){return this.width*this.height};
ne.prototype.$F=function(){return this.width/this.height};
ne.prototype.isEmpty=function(){return!this.pT()};
ne.prototype.ceil=function(){this.width=Math.ceil(this.width);this.height=Math.ceil(this.height);return this};
ne.prototype.Yp=function(a){return this.width<=a.width&&this.height<=a.height};
ne.prototype.floor=function(){this.width=Math.floor(this.width);this.height=Math.floor(this.height);return this};
ne.prototype.round=function(){this.width=Math.round(this.width);this.height=Math.round(this.height);return this};
ne.prototype.scale=function(a){this.width*=a;this.height*=a;return this};
ne.prototype.uD=function(a){var b=this.$F()>a.$F()?a.width/this.width:a.height/this.height;return this.scale(b)};var pe=function(a,b,c){return Math.min(Math.max(a,b),c)},
qe=function(a,b){var c=a%b;return c*b<0?c+b:c};var Zb={},$b,re,s,se,te,ue,ve,we,xe,ye,ze,Ae,Be,Ce;(function(){var a=false,b=false,c=false,d=false,e=false,f=false,g=false,h=false,i=false,k=false,n=false,q="";if(j.navigator){var t=navigator.userAgent;a=typeof opera!="undefined";b=!a&&t.indexOf("MSIE")!=-1;c=!a&&t.indexOf("WebKit")!=-1;n=c&&t.indexOf("Mobile")!=-1;d=!a&&navigator.product=="Gecko"&&!c;e=d&&navigator.vendor=="Camino";f=!a&&t.indexOf("Konqueror")!=-1;g=f||c;var y,z;if(a)y=opera.version();else{if(d)z=/rv\:([^\);]+)(\)|;)/;else if(b)z=
/MSIE\s+([^\);]+)(\)|;)/;else if(c)z=/WebKit\/(\S+)/;else if(f)z=/Konqueror\/([^\);]+)(\)|;)/;if(z){z.test(t);y=RegExp.$1}}q=navigator.platform;h=q.indexOf("Mac")!=-1;i=q.indexOf("Win")!=-1;k=q.indexOf("Linux")!=-1}re=a;s=b;se=d;te=e;ue=f;$b=c;ve=$b;we=g;xe=y;ye=q;ze=h;Ae=i;Be=k;Ce=n})();
var De=function(a,b){return zb(a,b)},
Ee=function(a){return zb(xe,a)>=0};var Fe,He=function(){if(!Fe)Fe=new Ge;return Fe},
Je=function(a){return a?new Ge(Ie(a)):He()},
Ke=function(){return He().Ic()},
u=function(a){return He().A(a)},
Le=u,Me=function(a,b,c){return He().wY(a,b,c)},
Ne=Me,Pe=function(a,b){Gc(b,function(c,d){if(d=="style")a.style.cssText=c;else if(d=="class")a.className=c;else if(d=="for")a.htmlFor=c;else if(d in Oe)a.setAttribute(Oe[d],c);else a[d]=c})},
Oe={cellpadding:"cellPadding",cellspacing:"cellSpacing",colspan:"colSpan",rowspan:"rowSpan",valign:"vAlign",height:"height",width:"width",usemap:"useMap",frameborder:"frameBorder"},Qe=function(a){var b=a||j||window,c=b.document;if($b&&!Ee("500")&&!Ce){if(typeof b.innerHeight=="undefined")b=window;var d=b.innerHeight,e=b.document.documentElement.scrollHeight;if(b==b.top)if(e<d)d-=15;return new ne(b.innerWidth,d)}var f=Je(c),g=f.iY()=="CSS1Compat"&&(!re||re&&Ee("9.50"))?c.documentElement:c.body;return new ne(g.clientWidth,
g.clientHeight)},
Se=function(a){var b=a||j||window,c=b.document,d,e;if(!$b&&c.compatMode=="CSS1Compat"){d=c.documentElement.scrollLeft;e=c.documentElement.scrollTop}else{d=c.body.scrollLeft;e=c.body.scrollTop}return new ge(d,e)},
Te=function(a){return Je(a).SZ()},
v=function(){var a=He();return a.m.apply(a,arguments)},
w=function(a){return He().createElement(a)},
Ue=function(a){return He().createTextNode(a)},
Ve=function(a){return He().S1(a)},
x=function(a,b){a.appendChild(b)},
We=function(a){var b;while(b=a.firstChild)a.removeChild(b)},
Xe=function(a,b){if(b.parentNode)b.parentNode.insertBefore(a,b)},
Ye=function(a,b){if(b.parentNode)b.parentNode.insertBefore(a,b.nextSibling)},
A=function(a){return a&&a.parentNode?a.parentNode.removeChild(a):null},
$e=function(a){return Ze(a.firstChild,true)},
af=function(a){return Ze(a.nextSibling,true)},
bf=function(a){return Ze(a.previousSibling,false)},
Ze=function(a,b){while(a&&a.nodeType!=1)a=b?a.nextSibling:a.previousSibling;return a};
var cf=$b&&De(xe,"521")<=0,df=function(a,b){if(typeof a.contains!="undefined"&&!cf&&b.nodeType==1)return a==b||a.contains(b);if(typeof a.compareDocumentPosition!="undefined")return a==b||Boolean(a.compareDocumentPosition(b)&16);while(b&&a!=b)b=b.parentNode;return b==a},
Ie=function(a){return a.nodeType==9?a:a.ownerDocument||a.document},
ef=function(a){return $b?a.document||a.contentWindow.document:a.contentDocument||a.contentWindow.document},
B=function(a,b){if("textContent"in a)a.textContent=b;else if(a.firstChild&&a.firstChild.nodeType==3){while(a.lastChild!=a.firstChild)a.removeChild(a.lastChild);a.firstChild.data=b}else{while(a.hasChildNodes())a.removeChild(a.lastChild);var c=Ie(a);a.appendChild(c.createTextNode(b))}},
gf=function(a,b){var c=[];ff(a,b,c,true);return c.length?c[0]:undefined},
ff=function(a,b,c,d){if(a!=null)for(var e=0,f;f=a.childNodes[e];e++){if(b(f)){c.push(f);if(d)return}ff(f,b,c,d)}},
hf={SCRIPT:1,STYLE:1,HEAD:1,IFRAME:1,OBJECT:1},jf={IMG:" ",BR:"\n"},lf=function(a){var b;if(s&&"innerText"in a)b=Ta(a.innerText);else{var c=[];kf(a,c,true);b=c.join("")}b=b.replace(/\xAD/g,"");b=b.replace(/ +/g," ");if(b!=" ")b=b.replace(/^\s*/,"");return b},
mf=function(a){var b=[];kf(a,b,false);return b.join("")},
kf=function(a,b,c){if(a.nodeName in hf){}else if(a.nodeType==3)if(c)b.push(String(a.nodeValue).replace(/(\r\n|\r|\n)/g,""));else b.push(a.nodeValue);else if(a.nodeName in jf)b.push(jf[a.nodeName]);else{var d=a.firstChild;while(d){kf(d,b,c);d=d.nextSibling}}},
nf=function(a){if(a&&typeof a.length=="number")if(Da(a))return typeof a.item=="function"||typeof a.item=="string";else if(Ca(a))return typeof a.item=="function";return false},
Ge=function(a){this.ke=a||j.document||document};
Ge.prototype.Ic=function(){return this.ke};
Ge.prototype.A=function(a){return za(a)?this.ke.getElementById(a):a};
Ge.prototype.nR=Ge.prototype.A;Ge.prototype.wY=function(a,b,c){var d=a||"*",e=c||this.ke,f=e.getElementsByTagName(d);if(b){var g=[];for(var h=0,i;i=f[h];h++){var k=i.className;if(typeof k.split=="function"&&mc(k.split(" "),b))g.push(i)}return g}else return f};
Ge.prototype.m=function(a,b){if(s&&b&&b.name)a="<"+a+' name="'+p(b.name)+'">';var c=this.createElement(a);if(b)Pe(c,b);if(arguments.length>2){function d(g){if(g)this.appendChild(c,za(g)?this.createTextNode(g):g)}
for(var e=2;e<arguments.length;e++){var f=arguments[e];if(ya(f)&&!(Da(f)&&f.nodeType>0))r(nf(f)?uc(f):f,d,this);else d.call(this,f)}}return c};
Ge.prototype.createElement=function(a){return this.ke.createElement(a)};
Ge.prototype.createTextNode=function(a){return this.ke.createTextNode(a)};
Ge.prototype.S1=function(a){var b=this.ke.createElement("div");b.innerHTML=a;if(b.childNodes.length==1)return b.firstChild;else{var c=this.ke.createDocumentFragment();while(b.firstChild)c.appendChild(b.firstChild);return c}};
Ge.prototype.iY=function(){if(this.ke.compatMode)return this.ke.compatMode;if($b){var a=this.m("div",{style:"position:absolute;width:0;height:0;width:1"}),b=a.style.width=="1px"?"BackCompat":"CSS1Compat";return this.ke.compatMode=b}return"BackCompat"};
Ge.prototype.SZ=function(){var a=this.ke;if(a.parentWindow)return a.parentWindow;if($b&&!Ee("500")&&!Ce){var b=a.createElement("script");b.innerHTML="document.parentWindow=window";var c=a.documentElement;c.appendChild(b);c.removeChild(b);return a.parentWindow}return a.defaultView};
Ge.prototype.appendChild=x;Ge.prototype.Jh=We;Ge.prototype.removeNode=A;Ge.prototype.nJ=$e;Ge.prototype.contains=df;var of=function(a,b){a.className=b},
pf=function(a){var b=a.className;return b&&typeof b.split=="function"?b.split(" "):[]},
C=function(a){var b=pf(a),c=wc(arguments,1),d=1;for(var e=0;e<c.length;e++)if(!mc(b,c[e])){b.push(c[e]);d&=1}else d&=0;a.className=b.join(" ");return Boolean(d)},
qf=function(a){var b=pf(a),c=wc(arguments,1),d=0;for(var e=0;e<b.length;e++)if(mc(c,b[e])){qc(b,e--,1);d++}a.className=b.join(" ");return d==c.length},
rf=function(a,b,c){var d=pf(a),e=false;for(var f=0;f<d.length;f++)if(d[f]==b){qc(d,f--,1);e=true}if(e){d.push(c);a.className=d.join(" ")}return e},
sf=function(a,b){return mc(pf(a),b)},
tf=function(a,b,c){if(c)C(a,b);else qf(a,b)},
uf=function(a,b){var c=!sf(a,b);tf(a,b,c);return c};var xf=function(a){var b=new dd;vf(a,b,wf);return b},
zf=function(a){var b=[];vf(a,b,yf);return b.join("&")},
vf=function(a,b,c){var d=a.elements;for(var e,f=0;e=d[f];f++){if(e.disabled||e.tagName.toLowerCase()=="fieldset")continue;var g=e.name,h=e.type.toLowerCase();switch(h){case "file":case "submit":case "reset":case "button":break;case "select-multiple":var i=Af(e);for(var k,n=0;k=i[n];n++)c(b,g,k);break;default:var k=Af(e);if(k!=null)c(b,g,k)}}var q=a.getElementsByTagName("input");for(var t,f=0;t=q[f];f++)if(t.form==a&&t.type.toLowerCase()=="image"){g=t.name;c(b,g,t.value);c(b,g+".x","0");c(b,g+".y",
"0")}},
wf=function(a,b,c){var d=a.get(b);if(!d){d=[];a.J(b,d)}d.push(c)},
yf=function(a,b,c){a.push(encodeURIComponent(b)+"="+encodeURIComponent(c))},
Bf=function(a,b){if(a.tagName=="FORM"){var c=a.elements;for(var a,d=0;a=c[d];d++)Bf(a,b)}else{if(b==true)a.blur();a.disabled=b}},
Af=function(a){var b=a.type;if(!pa(b))return null;switch(b.toLowerCase()){case "checkbox":case "radio":return a.checked?a.value:null;case "select-one":return Cf(a);case "select-multiple":return Df(a);default:return pa(a.value)?a.value:null}};
var Cf=function(a){var b=a.selectedIndex;return b>=0?a.options[b].value:null},
Df=function(a){var b=[];for(var c,d=0;c=a.options[d];d++)if(c.selected)b.push(c.value);return b.length?b:null};var D=function(a,b){this.type=a;this.target=b;this.currentTarget=this.target};
o(D,Vd);D.prototype.nk=false;D.prototype.Go=true;D.prototype.stopPropagation=function(){this.nk=true};
D.prototype.preventDefault=function(){this.Go=false};var Ef=function(a,b){if(a)this.Vd(a,b)};
o(Ef,D);var Ff=[1,4,2];Ef.prototype.type=null;Ef.prototype.target=null;Ef.prototype.currentTarget=null;Ef.prototype.relatedTarget=null;Ef.prototype.offsetX=0;Ef.prototype.offsetY=0;Ef.prototype.clientX=0;Ef.prototype.clientY=0;Ef.prototype.screenX=0;Ef.prototype.screenY=0;Ef.prototype.button=0;Ef.prototype.keyCode=0;Ef.prototype.charCode=0;Ef.prototype.ctrlKey=false;Ef.prototype.altKey=false;Ef.prototype.shiftKey=false;Ef.prototype.metaKey=false;Ef.prototype.gf=null;Ef.prototype.Vd=function(a,b){this.type=
a.type;this.target=a.target||a.srcElement;this.currentTarget=b;this.relatedTarget=a.relatedTarget?a.relatedTarget:(this.type==Gf?a.fromElement:(this.type==Hf?a.toElement:null));this.offsetX=typeof a.layerX=="number"?a.layerX:a.offsetX;this.offsetY=typeof a.layerY=="number"?a.layerY:a.offsetY;this.clientX=typeof a.clientX=="number"?a.clientX:a.pageX;this.clientY=typeof a.clientY=="number"?a.clientY:a.pageY;this.screenX=a.screenX||0;this.screenY=a.screenY||0;this.button=a.button;this.keyCode=a.keyCode||
0;this.charCode=a.charCode||(this.type==If?a.keyCode:0);this.ctrlKey=a.ctrlKey;this.altKey=a.altKey;this.shiftKey=a.shiftKey;this.metaKey=a.metaKey;this.gf=a;this.Go=null;this.nk=null};
Ef.prototype.Vl=function(a){return s?(this.type==E?a==0:!(!(this.gf.button&Ff[a]))):($b&&!Ee("420")?this.gf.button==1&&a==0:this.gf.button==a)};
Ef.prototype.stopPropagation=function(){this.nk=true;if(this.gf.stopPropagation)this.gf.stopPropagation();else this.gf.cancelBubble=true};
Ef.prototype.preventDefault=function(){this.Go=false;if(!this.gf.preventDefault){this.gf.returnValue=false;try{this.gf.keyCode=-1}catch(a){}}else this.gf.preventDefault()};
Ef.prototype.du=function(){return this.gf};
Ef.prototype.b=function(){if(!this.W()){D.prototype.b.call(this);this.gf=null}};var Jf=function(){},
Kf=0;Jf.prototype.IB=null;Jf.prototype.listener=null;Jf.prototype.KN=null;Jf.prototype.src=null;Jf.prototype.type=null;Jf.prototype.capture=null;Jf.prototype.Bq=null;Jf.prototype.key=0;Jf.prototype.removed=false;Jf.prototype.gz=false;Jf.prototype.Vd=function(a,b,c,d,e,f){if(Ca(a))this.IB=true;else if(a&&a.Zc&&Ca(a.Zc))this.IB=false;else throw Error("Invalid listener argument");this.listener=a;this.KN=b;this.src=c;this.type=d;this.capture=!(!e);this.Bq=f;this.gz=false;this.key=++Kf;this.removed=false};
Jf.prototype.Zc=function(a){if(this.IB)return this.listener.call(this.Bq||this.src,a);return this.listener.Zc.call(this.listener,a)};var Lf={},Mf={},Nf={},Of=new Wd(0,600);Of.Pr(function(){return{Ma:0}});
Of.WO(function(a){a.Ma=0});
var Pf=new Wd(0,600);Pf.Pr(function(){return[]});
Pf.WO(function(a){a.length=0;delete a.Jv;delete a.HM});
var Qf=new Wd(0,600);Qf.Pr(function(){var a=function(b){return Rf.call(a.src,a.key,b)};
return a});
var Sf=function(){return new Jf},
Tf=new Wd(0,600);Tf.Pr(Sf);var Uf=function(){return new Ef},
Vf=function(){var a=null;if(s){a=new Wd(0,600);a.Pr(Uf)}return a},
Wf=Vf(),Xf="on",Yf={},F=function(a,b,c,d,e){if(!b)throw Error("Invalid event type");else if(xa(b)){for(var f=0;f<b.length;f++)F(a,b[f],c,d,e);return null}var g=!(!d),h=Mf;if(!(b in h))h[b]=Of.Sd();h=h[b];if(!(g in h)){h[g]=Of.Sd();h.Ma++}h=h[g];var i=Ga(a),k,n;if(!h[i]){k=(h[i]=Pf.Sd());h.Ma++}else{k=h[i];for(var f=0;f<k.length;f++){n=k[f];if(n.listener==c&&n.Bq==e){if(n.removed)break;return k[f].key}}}var q=Qf.Sd();q.src=a;n=Tf.Sd();n.Vd(c,q,a,b,g,e);var t=n.key;q.key=t;k.push(n);Lf[t]=n;if(!Nf[i])Nf[i]=
Pf.Sd();Nf[i].push(n);if(a.addEventListener){if(a==j||!a.AH)a.addEventListener(b,q,g)}else a.attachEvent(Zf(b),q);return t},
$f=function(a,b,c,d,e){if(xa(b)){for(var f=0;f<b.length;f++)$f(a,b[f],c,d,e);return null}var g=F(a,b,c,d,e),h=Lf[g];h.gz=true;return g},
G=function(a,b,c,d,e){if(xa(b)){for(var f=0;f<b.length;f++)G(a,b[f],c,d,e);return null}var g=!(!d),h=ag(a,b,g);if(!h)return false;for(var f=0;f<h.length;f++)if(h[f].listener==c&&h[f].capture==g&&h[f].Bq==e)return H(h[f].key);return false},
H=function(a){if(!Lf[a])return false;var b=Lf[a];if(b.removed)return false;var c=b.src,d=b.type,e=b.KN,f=b.capture;if(c.removeEventListener){if(c==j||!c.AH)c.removeEventListener(d,e,f)}else if(c.detachEvent)c.detachEvent(Zf(d),e);var g=Ga(c),h=Mf[d][f][g];if(Nf[g]){var i=Nf[g];tc(i,b);if(i.length==0)delete Nf[g]}b.removed=true;h.HM=true;bg(d,f,g,h);delete Lf[a];return true},
bg=function(a,b,c,d){if(!d.Jv)if(d.HM){for(var e=0,f=0;e<d.length;e++){if(d[e].removed){Tf.Ae(d[e]);continue}if(e!=f)d[f]=d[e];f++}d.length=f;d.HM=false;if(f==0){Pf.Ae(d);delete Mf[a][b][c];Mf[a][b].Ma--;if(Mf[a][b].Ma==0){Of.Ae(Mf[a][b]);delete Mf[a][b];Mf[a].Ma--}if(Mf[a].Ma==0){Of.Ae(Mf[a]);delete Mf[a]}}}},
cg=function(a,b,c){var d=0,e=a==null,f=b==null,g=c==null;c=!(!c);if(!e){var h=Ga(a);if(Nf[h]){var i=Nf[h];for(var k=i.length-1;k>=0;k--){var n=i[k];if((f||b==n.type)&&(g||c==n.capture)){H(n.key);d++}}}}else Gc(Nf,function(q){for(var t=q.length-1;t>=0;t--){var y=q[t];if((f||b==y.type)&&(g||c==y.capture)){H(y.key);d++}}});
return d},
ag=function(a,b,c){var d=Mf;if(b in d){d=d[b];if(c in d){d=d[c];var e=Ga(a);if(d[e])return d[e]}}return null},
dg=function(a,b,c,d,e){var f=!(!d),g=ag(a,b,f);if(g)for(var h=0;h<g.length;h++)if(g[h].listener==c&&g[h].capture==f&&g[h].Bq==e)return g[h];return null},
E="click",I="mousedown",eg="mouseup",Gf="mouseover",Hf="mouseout",fg="mousemove",If="keypress",gg="keydown",hg="keyup",ig="blur",jg="focus",kg="change",lg="select",mg="resize",ng="contextmenu",Zf=function(a){if(a in Yf)return Yf[a];return Yf[a]=Xf+a},
pg=function(a,b,c,d){var e=1,f=Mf;if(b in f){f=f[b];if(c in f){f=f[c];var g=Ga(a);if(f[g]){var h=f[g];if(!h.Jv)h.Jv=1;else h.Jv++;try{var i=h.length;for(var k=0;k<i;k++){var n=h[k];if(n&&!n.removed)e&=og(n,d)!==false}}finally{h.Jv--;bg(b,c,g,h)}}}}return Boolean(e)},
og=function(a,b){var c=a.Zc(b);if(a.gz)H(a.key);return c},
qg=function(a,b){if(za(b))b=new D(b,a);else if(!(b instanceof D)){var c=b;b=new D(b.type,a);Uc(b,c)}else b.target=b.target||a;var d=1,e,f=b.type,g=Mf;if(!(f in g))return true;g=g[f];var h=true in g,i=false in g;if(h){e=[];for(var k=a;k;k=k.ru())e.push(k);for(var n=e.length-1;!b.nk&&n>=0;n--){b.currentTarget=e[n];d&=pg(e[n],b.type,true,b)&&b.Go!=false}}if(i)if(h)for(var n=0;!b.nk&&n<e.length;n++){b.currentTarget=e[n];d&=pg(e[n],b.type,false,b)&&b.Go!=false}else for(var q=a;!b.nk&&q;q=q.ru()){b.currentTarget=
q;d&=pg(q,b.type,false,b)&&b.Go!=false}return Boolean(d)},
Rf=function(a,b){if(!Lf[a])return true;var c=Lf[a],d=c.type,e=Mf;if(!(d in e))return true;e=e[d];var f;if(s){var g=b||ra("window.event"),h=true in e;if(h){if(g.keyCode<0||g.returnValue!=undefined)return true;rg(g)}Ga(c.src);var i=Wf.Sd();i.Vd(g,this);f=true;try{if(h){var k=Pf.Sd();for(var n=i.currentTarget;n;n=n.parentNode)k.push(n);for(var q=k.length-1;!i.nk&&q>=0;q--){i.currentTarget=k[q];f&=pg(k[q],d,true,i)}for(var q=0;!i.nk&&q<k.length;q++){i.currentTarget=k[q];f&=pg(k[q],d,false,i)}}else f=
og(c,i)}finally{if(k){k.length=0;Pf.Ae(k)}i.b();Wf.Ae(i)}return f}var t=new Ef(b,this);try{f=og(c,t)}finally{t.b()}return f},
rg=function(a){var b=false;if(a.keyCode==0)try{a.keyCode=-1;return}catch(c){b=true}if(b||a.returnValue==undefined)a.returnValue=true};var J=function(){};
o(J,Vd);J.prototype.AH=true;J.prototype.LC=null;J.prototype.ru=function(){return this.LC};
J.prototype.tx=function(a){this.LC=a};
J.prototype.addEventListener=function(a,b,c,d){F(this,a,b,c,d)};
J.prototype.removeEventListener=function(a,b,c,d){G(this,a,b,c,d)};
J.prototype.dispatchEvent=function(a){return qg(this,a)};
J.prototype.b=function(){if(!this.W()){Vd.prototype.b.call(this);cg(this);this.LC=null}};var sg=function(a){this.Fm=v("div",{style:"position:absolute;left:0;top:-1000px;"},"X");x(Ke().body,this.Fm);this.Ch=this.Fm.offsetWidth;if(a)this.yE=F(a,"tick",this.DG,false,this);else this.Lg=j.setInterval(l(this.DG,this),50)};
o(sg,J);sg.prototype.yE=null;sg.prototype.Lg=null;sg.prototype.b=function(){if(!this.W()){sg.f.b.call(this);A(this.Fm);if(this.yE)H(this.yE);if(this.Lg)window.clearInterval(this.Lg)}};
sg.prototype.DG=function(){var a=this.Fm.offsetWidth;if(this.Ch!=a){this.Ch=a;this.dispatchEvent("fontsizechange")}};var vg=function(a,b){if(tg(a))a.selectionStart=b;else if(s){var c=ug(a),d=c[0],e=c[1];if(d.inRange(e)){var f=a.value,g=0,h=b;while(g!=-1&&g<h){g=f.indexOf("\r\n",g);if(g!=-1&&g<h){b--;g++}}d.collapse(true);d.move("character",b);d.select()}}},
wg=function(a){if(tg(a))return a.selectionStart;if(s){var b=ug(a),c=b[0],d=b[1];if(c.inRange(d)){c.setEndPoint("EndToStart",d);return c.text.length}}return 0},
xg=function(a,b){if(tg(a))a.selectionEnd=b;else if(s){var c=ug(a),d=c[0],e=c[1];if(d.inRange(e)){e.collapse();e.moveEnd("character",b-wg(a));e.select()}}},
yg=function(a,b){var c=a.ownerDocument||a.document;if(tg(a)){a.selectionStart=b;a.selectionEnd=b}else if(c.selection&&a.createTextRange){var d=a.createTextRange();d.collapse(true);d.move("character",b);d.select()}},
ug=function(a){var b=a.ownerDocument||a.document,c=b.selection.createRange(),d;if(a.type=="textarea"){d=c.duplicate();d.moveToElementText(a)}else d=a.createTextRange();return[d,c]},
tg=function(a){try{return typeof a.selectionStart=="number"}catch(b){return false}};;var K=function(a){this.j=a};
o(K,Vd);var zg=new Wd(0,100);K.prototype.g=function(a,b,c,d,e){if(xa(b)){for(var f=0;f<b.length;f++)this.g(a,b[f],c,d,e);return}var g=F(a,b,c||this,d||false,e||this.j||this);if(this.Pa)this.Pa[g]=true;else if(this.te){this.Pa=zg.Sd();this.Pa[this.te]=true;this.te=null;this.Pa[g]=true}else this.te=g};
K.prototype.Sa=function(a,b,c,d,e){if(!this.te&&!this.Pa)return;if(xa(b)){for(var f=0;f<b.length;f++)this.Sa(a,b[f],c,d,e);return}var g=dg(a,b,c||this,d||false,e||this.j||this);if(g){var h=g.key;H(h);if(this.Pa)Nc(this.Pa,h);else if(this.te==h)this.te=null}};
K.prototype.ma=function(){if(this.Pa){for(var a in this.Pa){H(a);delete this.Pa[a]}zg.Ae(this.Pa);this.Pa=null}else if(this.te)H(this.te)};
K.prototype.b=function(){if(!this.W()){Vd.prototype.b.call(this);this.ma()}};
K.prototype.Zc=function(){throw Error("EventHandler.handleEvent not implemented");};var Ag=function(a){J.call(this);this.e=a;var b=s?"propertychange":"input";this.Sq=F(this.e,b,this)};
o(Ag,J);Ag.prototype.Zc=function(a){var b=a.du();if(b.type=="propertychange"&&b.propertyName=="value"||b.type=="input"){if(s){var c=b.srcElement;if(c!=Ie(c).activeElement)return}var d=new Ef(b);d.type="input";try{this.dispatchEvent(d)}finally{d.b()}}};
Ag.prototype.b=function(){if(!this.W()){Ag.f.b.call(this);H(this.Sq);this.e=null}};var Bg=function(a){if(!s&&!($b&&Ee("525")))return true;if(a>=48&&a<=57)return true;if(a>=96&&a<=106)return true;if(a>=65&&a<=90)return true;if(a==27&&$b)return false;switch(a){case 13:case 27:case 32:case 63:case 107:case 109:case 110:case 111:case 186:case 189:case 187:case 188:case 190:case 191:case 192:case 222:case 219:case 220:case 221:return true;default:return false}},
Cg=function(a){if(a>=48&&a<=57)return true;if(a>=96&&a<=106)return true;if(a>=65&&a<=90)return true;switch(a){case 32:case 63:case 107:case 109:case 110:case 111:case 186:case 189:case 187:case 188:case 190:case 191:case 192:case 222:case 219:case 220:case 221:return true;default:return false}};var Dg=function(a){J.call(this);if(a)this.rc(a)};
o(Dg,J);Dg.prototype.e=null;Dg.prototype.tv=null;Dg.prototype.QB=null;Dg.prototype.uv=null;Dg.prototype.xv=-1;Dg.prototype.Mq=-1;Dg.prototype.SL=0;var Eg={"3":13,"12":144,"63232":38,"63233":40,"63234":37,"63235":39,"63236":112,"63237":113,"63238":114,"63239":115,"63240":116,"63241":117,"63242":118,"63243":119,"63244":120,"63245":121,"63246":122,"63247":123,"63248":44,"63272":46,"63273":36,"63275":35,"63276":33,"63277":34,"63289":144,"63302":45},Fg={Up:38,Down:40,Left:37,Right:39,Enter:13,F1:112,F2:113,
F3:114,F4:115,F5:116,F6:117,F7:118,F8:119,F9:120,F10:121,F11:122,F12:123,"U+007F":46,Home:36,End:35,PageUp:33,PageDown:34,Insert:45},Gg=s||$b&&Ee("525");Dg.prototype.yh=function(a){if(Gg&&!Bg(a.keyCode))this.Zc(a);else this.Mq=a.keyCode};
Dg.prototype.n0=function(){this.xv=-1};
Dg.prototype.Zc=function(a){var b=a.du(),c,d;if(s&&a.type==If){c=this.Mq;d=c!=13&&c!=27?b.keyCode:0}else if($b&&a.type==If){c=this.Mq;d=b.charCode>=0&&b.charCode<63232&&Cg(c)?b.charCode:0}else if(re){c=this.Mq;d=Cg(c)?b.keyCode:0}else{c=b.keyCode||this.Mq;d=b.charCode||0}var e=c,f=b.keyIdentifier;if(c){if(c>=63232&&c in Eg)e=Eg[c];else if(c==25&&a.shiftKey)e=9}else if(f&&f in Fg)e=Fg[f];var g=e==this.xv;this.xv=e;if($b){if(g&&b.timeStamp-this.SL<50)return;this.SL=b.timeStamp}var h=new Hg(e,d,g,b);
try{this.dispatchEvent(h)}finally{h.b()}};
Dg.prototype.rc=function(a){if(this.uv)this.detach();this.e=a;this.tv=F(this.e,If,this);this.QB=F(this.e,gg,this.yh,false,this);this.uv=F(this.e,hg,this.n0,false,this)};
Dg.prototype.detach=function(){if(this.tv){H(this.tv);H(this.QB);H(this.uv);this.tv=null;this.QB=null;this.uv=null}this.e=null;this.xv=-1};
Dg.prototype.b=function(){if(!this.W()){Dg.f.b.call(this);this.detach()}};
var Hg=function(a,b,c,d){Ef.call(this,d);this.type="key";this.keyCode=a;this.charCode=b;this.repeat=c};
o(Hg,Ef);var Ig=function(a){J.call(this);this.e=a;var b=se?"DOMMouseScroll":"mousewheel";this.Sq=F(this.e,b,this)};
o(Ig,J);var Jg="mousewheel";Ig.prototype.Sq=null;Ig.prototype.Zc=function(a){var b=0,c=a.du();if(c.type=="mousewheel"){b=-c.wheelDelta/40;if($b)b/=3;else if(re)b=-b}else b=c.detail;if(b>100)b=3;else if(b<-100)b=-3;var d=new Kg(b,c);try{this.dispatchEvent(d)}finally{d.b()}};
Ig.prototype.b=function(){if(!this.W()){Ig.f.b.call(this);H(this.Sq);this.Sq=null}};
var Kg=function(a,b){Ef.call(this,b);this.type=Jg;this.detail=a};
o(Kg,Ef);var Lg=[768,111,276,6,264,44,2,0,2,1,2,1,2,0,73,5,54,19,18,0,102,6,2,6,3,1,2,3,36,0,31,26,92,10,337,2,57,0,2,15,4,3,14,1,30,2,57,0,2,15,10,0,11,1,30,2,57,17,35,1,16,2,57,0,2,15,21,1,30,2,57,0,2,25,43,0,60,25,42,2,59,24,44,1,57,0,2,24,44,1,59,25,43,1,71,41,62,0,3,6,13,7,99,0,3,8,12,5,75,1,28,0,2,0,2,0,5,1,50,19,2,1,9,44,10,0,102,13,29,3,774,0,947,2,30,2,30,1,31,1,67,29,10,0,46,2,156,0,119,27,117,16,8,1,78,4,933,3,585,1,195,27,3903,5,106,1,30568,0,4,0,5,0,24,4,21239,0,738,15,17,3],Ng=function(a,b){var c=
b||10;if(c>a.length)return a;var d=[],e=0,f=0,g=0;for(var h=0;h<a.length;h++){var i=a.charCodeAt(h),k=false;if(i>=768){if(Lg[0]>Lg[1])for(var n=1;n<Lg.length;n++)Lg[n]+=Lg[n-1];var q=yc(Lg,i);k=q>=0||-q%2==0}if(e>=c&&i>32&&!k){d.push(a.substring(g,h),Mg);g=h;e=0}if(!f)if(i==60||i==38)f=i;else if(i<=32)e=0;else if(k){}else e++;else if(i==62&&f==60)f=0;else if(i==59&&f==38){f=0;e++}}d.push(a.substr(g));return d.join("")},
Mg=$b?"<wbr></wbr>":(re?"&shy;":"<wbr>");var Pg=function(a,b){J.call(this);this.Lg=a||1;this.Ux=b||Og;this.qG=l(this.G$,this);this.Yl=Ka()};
o(Pg,J);Pg.prototype.enabled=false;var Og=j.window,Qg=0.8;Pg.prototype.ab=null;Pg.prototype.setInterval=function(a){this.Lg=a;if(this.ab&&this.enabled){this.stop();this.start()}else if(this.ab)this.stop()};
Pg.prototype.G$=function(){if(this.enabled){var a=Ka()-this.Yl;if(a>0&&a<this.Lg*Qg){this.ab=this.Ux.setTimeout(this.qG,this.Lg-a);return}this.lW();if(this.enabled){this.ab=this.Ux.setTimeout(this.qG,this.Lg);this.Yl=Ka()}}};
Pg.prototype.lW=function(){this.dispatchEvent("tick")};
Pg.prototype.start=function(){this.enabled=true;if(!this.ab){this.ab=this.Ux.setTimeout(this.qG,this.Lg);this.Yl=Ka()}};
Pg.prototype.stop=function(){this.enabled=false;this.Ux.clearTimeout(this.ab);this.ab=null};
Pg.prototype.b=function(){if(!this.W()){J.prototype.b.call(this);this.stop();this.Ux=null}};
var Rg=function(a,b,c){if(Ca(a)){if(c)a=l(a,c)}else if(a&&typeof a.Zc=="function")a=l(a.Zc,a);else throw Error("Invalid listener argument");return Og.setTimeout(a,b||0)},
Sg=function(a){Og.clearTimeout(a)};var Tg=function(a){return 1-Math.pow(1-a,3)},
Ug=function(a,b,c,d){J.call(this);if(!xa(a)||!xa(b)){throw Error("Start and end parameters must be arrays");return}if(a.length!=b.length){throw Error("Start and end points must be the same length");return}this.Gm=a;this.jI=b;this.Pp=c;this.vS=d;this.Ym=[]};
o(Ug,J);var Vg="begin",Wg="animate",Xg=[],Yg=null,Zg=function(){Og.clearTimeout(Yg);var a=Ka();r(Xg,function(b){if(b)b.BH(a)});
Yg=Xg.length==0?null:Og.setTimeout(Zg,20)},
$g=function(a){if(!mc(Xg,a))Xg.push(a);if(Yg==null)Yg=Og.setInterval(Zg,20)},
ah=function(a){tc(Xg,a);if(nc(Xg.length)&&Yg!=null){Og.clearInterval(Yg);Yg=null}};
Ug.prototype.Bb=0;Ug.prototype.EI=0;Ug.prototype.jg=0;Ug.prototype.mg=null;Ug.prototype.kI=null;Ug.prototype.RB=null;Ug.prototype.play=function(a){if(a||this.Bb==0){this.jg=0;this.Ym=this.Gm}else if(this.Bb==1)return false;ah(this);this.mg=Ka();if(this.Bb==-1)this.mg-=this.Pp*this.jg;this.kI=this.mg+this.Pp;this.RB=this.mg;if(this.jg==0)this.lh(Vg);this.lh("play");if(this.Bb==-1)this.lh("resume");this.Bb=1;$g(this);this.BH(this.mg);return true};
Ug.prototype.stop=function(a){ah(this);this.Bb=0;if(a)this.jg=1;this.IE(this.jg);this.lh("stop");this.lh("end")};
Ug.prototype.pause=function(){if(this.Bb==1){ah(this);this.Bb=-1;this.lh("pause")}};
Ug.prototype.b=function(){if(!this.W()){if(this.Bb!=0)this.stop();this.lh("destroy");Ug.f.b.call(this)}};
Ug.prototype.dW=function(){this.b()};
Ug.prototype.BH=function(a){this.jg=(a-this.mg)/(this.kI-this.mg);if(this.jg>=1)this.jg=1;this.EI=1000/(a-this.RB);this.RB=a;if(Ca(this.vS))this.IE(this.vS(this.jg));else this.IE(this.jg);if(this.jg==1){this.Bb=0;ah(this);this.lh("finish");this.lh("end")}else if(this.Bb==1)this.lh(Wg)};
Ug.prototype.IE=function(a){this.Ym=new Array(this.Gm.length);for(var b=0;b<this.Gm.length;b++)this.Ym[b]=(this.jI[b]-this.Gm[b])*a+this.Gm[b]};
Ug.prototype.lh=function(a){this.dispatchEvent(new bh(a,this))};
var bh=function(a,b){D.call(this,a);this.coords=b.Ym;this.x=b.Ym[0];this.y=b.Ym[1];this.z=b.Ym[2];this.duration=b.Pp;this.progress=b.jg;this.fps=b.EI;this.state=b.Bb;this.anim=b};
o(bh,D);bh.prototype.jH=function(){return gc(this.coords,Math.round)};var hh=function(a){var b={};a=String(a);var c=a.charAt(0)=="#"?a:"#"+a;if(ch(c)){b.hex=dh(c);b.type="hex";return b}else{var d=eh(a);if(d.length){b.hex=fh(d[0],d[1],d[2]);b.type="rgb";return b}else if(gh.hasOwnProperty(a.toLowerCase())){b.hex=gh[a.toLowerCase()];b.type="named";return b}}throw Error(a+" is not a valid color string");},
ih=/#(.)(.)(.)/,dh=function(a){if(!ch(a))throw Error("'"+a+"' is not a valid hex color");if(a.length==4)a=a.replace(ih,"#$1$1$2$2$3$3");return a.toLowerCase()},
fh=function(a,b,c){a=Number(a);b=Number(b);c=Number(c);if(isNaN(a)||a<0||a>255||isNaN(b)||b<0||b>255||isNaN(c)||c<0||c>255)throw Error('"('+a+","+b+","+c+'") is not a valid RGB color');var d=jh(a.toString(16)),e=jh(b.toString(16)),f=jh(c.toString(16));return"#"+d+e+f};
var kh=/^#(?:[0-9a-f]{3}){1,2}$/i,ch=function(a){return kh.test(a)},
lh=/^(?:rgb)?\((0|[1-9]\d{0,2}),\s?(0|[1-9]\d{0,2}),\s?(0|[1-9]\d{0,2})\)$/i,eh=function(a){var b=a.match(lh);if(b){var c=Number(b[1]),d=Number(b[2]),e=Number(b[3]);if(c>=0&&c<=255&&d>=0&&d<=255&&e>=0&&e<=255)return[c,d,e]}return[]},
jh=function(a){return a.length==1?"0"+a:a};
var gh={aliceblue:"#f0f8ff",antiquewhite:"#faebd7",aqua:"#00ffff",aquamarine:"#7fffd4",azure:"#f0ffff",beige:"#f5f5dc",bisque:"#ffe4c4",black:"#000000",blanchedalmond:"#ffebcd",blue:"#0000ff",blueviolet:"#8a2be2",brown:"#a52a2a",burlywood:"#deb887",cadetblue:"#5f9ea0",chartreuse:"#7fff00",chocolate:"#d2691e",coral:"#ff7f50",cornflowerblue:"#6495ed",cornsilk:"#fff8dc",crimson:"#dc143c",cyan:"#00ffff",darkblue:"#00008b",darkcyan:"#008b8b",darkgoldenrod:"#b8860b",darkgray:"#a9a9a9",darkgrey:"#a9a9a9",
darkgreen:"#006400",darkkhaki:"#bdb76b",darkmagenta:"#8b008b",darkolivegreen:"#556b2f",darkorange:"#ff8c00",darkorchid:"#9932cc",darkred:"#8b0000",darksalmon:"#e9967a",darkseagreen:"#8fbc8f",darkslateblue:"#483d8b",darkslategray:"#2f4f4f",darkslategrey:"#2f4f4f",darkturquoise:"#00ced1",darkviolet:"#9400d3",deeppink:"#ff1493",deepskyblue:"#00bfff",dimgray:"#696969",dimgrey:"#696969",dodgerblue:"#1e90ff",firebrick:"#b22222",floralwhite:"#fffaf0",forestgreen:"#228b22",fuchsia:"#ff00ff",gainsboro:"#dcdcdc",
ghostwhite:"#f8f8ff",gold:"#ffd700",goldenrod:"#daa520",gray:"#808080",grey:"#808080",green:"#008000",greenyellow:"#adff2f",honeydew:"#f0fff0",hotpink:"#ff69b4",indianred:"#cd5c5c",indigo:"#4b0082",ivory:"#fffff0",khaki:"#f0e68c",lavender:"#e6e6fa",lavenderblush:"#fff0f5",lawngreen:"#7cfc00",lemonchiffon:"#fffacd",lightblue:"#add8e6",lightcoral:"#f08080",lightcyan:"#e0ffff",lightgoldenrodyellow:"#fafad2",lightgray:"#d3d3d3",lightgrey:"#d3d3d3",lightgreen:"#90ee90",lightpink:"#ffb6c1",lightsalmon:"#ffa07a",
lightseagreen:"#20b2aa",lightskyblue:"#87cefa",lightslategray:"#778899",lightslategrey:"#778899",lightsteelblue:"#b0c4de",lightyellow:"#ffffe0",lime:"#00ff00",limegreen:"#32cd32",linen:"#faf0e6",magenta:"#ff00ff",maroon:"#800000",mediumaquamarine:"#66cdaa",mediumblue:"#0000cd",mediumorchid:"#ba55d3",mediumpurple:"#9370d8",mediumseagreen:"#3cb371",mediumslateblue:"#7b68ee",mediumspringgreen:"#00fa9a",mediumturquoise:"#48d1cc",mediumvioletred:"#c71585",midnightblue:"#191970",mintcream:"#f5fffa",mistyrose:"#ffe4e1",
moccasin:"#ffe4b5",navajowhite:"#ffdead",navy:"#000080",oldlace:"#fdf5e6",olive:"#808000",olivedrab:"#6b8e23",orange:"#ffa500",orangered:"#ff4500",orchid:"#da70d6",palegoldenrod:"#eee8aa",palegreen:"#98fb98",paleturquoise:"#afeeee",palevioletred:"#d87093",papayawhip:"#ffefd5",peachpuff:"#ffdab9",peru:"#cd853f",pink:"#ffc0cb",plum:"#dda0dd",powderblue:"#b0e0e6",purple:"#800080",red:"#ff0000",rosybrown:"#bc8f8f",royalblue:"#4169e1",saddlebrown:"#8b4513",salmon:"#fa8072",sandybrown:"#f4a460",seagreen:"#2e8b57",
seashell:"#fff5ee",sienna:"#a0522d",silver:"#c0c0c0",skyblue:"#87ceeb",slateblue:"#6a5acd",slategray:"#708090",slategrey:"#708090",snow:"#fffafa",springgreen:"#00ff7f",steelblue:"#4682b4",tan:"#d2b48c",teal:"#008080",thistle:"#d8bfd8",tomato:"#ff6347",turquoise:"#40e0d0",violet:"#ee82ee",wheat:"#f5deb3",white:"#ffffff",whitesmoke:"#f5f5f5",yellow:"#ffff00",yellowgreen:"#9acd32"};var mh,L=function(a,b,c){if(za(b))nh(a,c,b);else Gc(b,Ia(nh,a))},
nh=function(a,b,c){a.style[oh(c)]=b},
ph=function(a,b){return a.style[oh(b)]},
qh=function(a,b){var c=Ie(a);if(c.defaultView&&c.defaultView.getComputedStyle){var d=c.defaultView.getComputedStyle(a,"");if(d)return d[b]}return null};
var rh=function(a,b){return qh(a,b)||(a.currentStyle?a.currentStyle[b]:null)||a.style[b]};
var sh=function(a,b,c){var d,e;if(b instanceof ge){d=b.x;e=b.y}else{d=b;e=c}a.style.left=typeof d=="number"?Math.round(d)+"px":d;a.style.top=typeof e=="number"?Math.round(e)+"px":e},
th=function(a){var b;b=a?(a.nodeType==9?a:Ie(a)):Ke();if(s&&b.compatMode!="CSS1Compat")return b.body;return b.documentElement},
uh=function(a){var b=Ie(a),c=se&&b.getBoxObjectFor&&rh(a,"position")=="absolute"&&(a.style.top==""||a.style.left=="");if(typeof mh=="undefined")mh=te&&!Ee("1.8.0.11");var d=new ge(0,0),e=th(b);if(a==e)return d;var f=null,g;if(a.getBoundingClientRect){g=a.getBoundingClientRect();var h=Se(Te(b));d.x=g.left+h.x;d.y=g.top+h.y}else if(b.getBoxObjectFor&&!c&&!mh){g=b.getBoxObjectFor(a);var i=b.getBoxObjectFor(e);d.x=g.screenX-i.screenX;d.y=g.screenY-i.screenY}else{f=a;do{d.x+=f.offsetLeft;d.y+=f.offsetTop;
if($b&&rh(f,"position")=="fixed"){d.x+=b.body.scrollLeft;d.y+=b.body.scrollTop;break}f=f.offsetParent}while(f&&f!=a);if(re||$b&&rh(a,"position")=="absolute")d.y-=b.body.offsetTop;f=a.offsetParent;while(f&&f!=b.body){d.x-=f.scrollLeft;if(!re||f.tagName!="TR")d.y-=f.scrollTop;f=f.offsetParent}}return d},
vh=function(a){return uh(a).y},
xh=function(a,b){var c=wh(a),d=wh(b);return new ge(c.x-d.x,c.y-d.y)},
wh=function(a){var b=new ge;if(a.nodeType==1)if(a.getBoundingClientRect){var c=a.getBoundingClientRect();b.x=c.left;b.y=c.top}else{var d=Se(Te(Ie(a))),e=uh(a);b.x=e.x-d.x;b.y=e.y-d.y}else{b.x=a.clientX;b.y=a.clientY}return b},
yh=function(a,b,c){var d=uh(a);if(b instanceof ge){c=b.y;b=b.x}var e=b-d.x,f=c-d.y;sh(a,a.offsetLeft+e,a.offsetTop+f)},
zh=function(a,b,c){var d;if(b instanceof ne){d=b.height;b=b.width}else d=c;a.style.width=typeof b=="number"?Math.round(b)+"px":b;a.style.height=typeof d=="number"?Math.round(d)+"px":d},
Ah=function(a){if(rh(a,"display")!="none")return new ne(a.offsetWidth,a.offsetHeight);var b=a.style,c=b.visibility,d=b.position;b.visibility="hidden";b.position="absolute";b.display="";var e=a.offsetWidth,f=a.offsetHeight;b.display="none";b.position=d;b.visibility=c;return new ne(e,f)},
Bh=function(a){var b=uh(a),c=Ah(a);return new ke(b.x,b.y,c.width,c.height)},
oh=function(a){return String(a).replace(/\-([a-z])/g,function(b,c){return c.toUpperCase()})};
var Ch=function(a,b){var c=a.style;if("opacity"in c)c.opacity=b;else if("MozOpacity"in c)c.MozOpacity=b;else if("KhtmlOpacity"in c)c.KhtmlOpacity=b;else if("filter"in c)c.filter="alpha(opacity="+b*100+")"},
Dh=function(a,b){var c=a.style;if(s)c.filter='progid:DXImageTransform.Microsoft.AlphaImageLoader(src="'+b+'", sizingMethod="crop")';else{c.backgroundImage="url("+b+")";c.backgroundPosition="top left";c.backgroundRepeat="no-repeat"}},
M=function(a,b){a.style.display=b?"":"none"},
Eh=function(a){var b=a.style;b.position="relative";if(s){b.zoom="1";b.display="inline"}else b.display=se?(Ee("1.9a")?"inline-block":"-moz-inline-box"):"inline-block"},
Fh=function(a){return"rtl"==rh(a,"direction")},
Gh=se?"MozUserSelect":($b?"WebkitUserSelect":null),Hh=function(a,b,c){var d=!c?a.getElementsByTagName("*"):null,e=Gh;if(e){var f=b?"none":"";a.style[e]=f;if(d)for(var g=0,h;h=d[g];g++)h.style[e]=f}else if(s||re){var f=b?"on":"";a.setAttribute("unselectable",f);if(d)for(var g=0,h;h=d[g];g++)h.setAttribute("unselectable",f)}};var Ih=function(a,b,c,d,e){Ug.call(this,b,c,d,e);this.element=a};
o(Ih,Ug);var Jh=function(a,b,c){Ih.apply(this,arguments);if(b.length!=2||c.length!=2){throw Error("Start and end points must be 2D");return}var d=[Vg,Wg,"end"];F(this,d,this.p4,false,this)};
o(Jh,Ih);Jh.prototype.p4=function(a){this.element.style.left=Math.round(a.x)+"px";this.element.style.top=Math.round(a.y)+"px"};
var Kh=function(a,b,c,d){var e=[a.offsetLeft,a.offsetTop];F(this,Vg,this.i9,false,this);Jh.call(this,a,e,b,c,d)};
o(Kh,Jh);Kh.prototype.i9=function(){this.Gm=[this.element.offsetLeft,this.element.offsetTop]};
var Lh=function(a,b,c){Ih.apply(this,arguments);if(b.length!=2||c.length!=2){throw Error("Start and end points must be 2D");return}var d=[Vg,Wg,"end"];F(this,d,this.Ef,false,this);this.yf=Math.max(this.jI[0],this.Gm[0]);this.Bd=Math.max(this.jI[1],this.Gm[1])};
o(Lh,Ih);Lh.prototype.Ef=function(a){this.sU(Math.round(a.x),Math.round(a.y),this.yf,this.Bd);this.element.style.width=Math.round(a.x)+"px";this.element.style.marginLeft=Math.round(a.x)-this.yf+"px";this.element.style.marginTop=Math.round(a.y)-this.Bd+"px"};
Lh.prototype.sU=function(a,b,c,d){this.element.style.clip="rect("+(d-b)+"px "+c+"px "+d+"px "+(c-a)+"px)"};
var Mh=function(a,b,c){Ih.apply(this,arguments);if(b.length!=2||c.length!=2){throw Error("Start and end points must be 2D");return}var d=[Vg,Wg,"end"];F(this,d,this.t7,false,this)};
o(Mh,Ih);Mh.prototype.t7=function(a){this.element.scrollLeft=Math.round(a.x);this.element.scrollTop=Math.round(a.y)};
var Nh=function(a,b,c){Ih.apply(this,arguments);if(b.length!=2||c.length!=2){throw Error("Start and end points must be 2D");return}var d=[Vg,Wg,"end"];F(this,d,this.Ef,false,this)};
o(Nh,Ih);Nh.prototype.Ef=function(a){this.element.style.width=Math.round(a.x)+"px";this.element.style.height=Math.round(a.y)+"px"};
var Oh=function(a,b,c,d,e){Ih.call(this,a,[b],[c],d,e);var f=[Vg,Wg,"end"];F(this,f,this.W6,false,this)};
o(Oh,Ih);Oh.prototype.W6=function(a){this.element.style.width=Math.round(a.x)+"px"};
var Ph=function(a,b,c,d,e){Ih.call(this,a,[b],[c],d,e);var f=[Vg,Wg,"end"];F(this,f,this.V6,false,this)};
o(Ph,Ih);Ph.prototype.V6=function(a){this.element.style.height=Math.round(a.x)+"px"};
var Qh=function(a,b,c,d,e){if(Ba(b))b=[b];if(Ba(c))c=[c];Ih.call(this,a,b,c,d,e);if(b.length!=1||c.length!=1){throw Error("Start and end points must be 1D");return}var f=[Vg,Wg,"end"];F(this,f,this.aX,false,this)};
o(Qh,Ih);Qh.prototype.aX=function(a){Ch(this.element,a.x)};
Qh.prototype.show=function(){this.element.style.display=""};
Qh.prototype.hide=function(){this.element.style.display="none"};
var Rh=function(a,b,c){Qh.call(this,a,1,0,b,c)};
o(Rh,Qh);var Sh=function(a,b,c){Qh.call(this,a,0,1,b,c)};
o(Sh,Qh);var Th=function(a,b,c){Qh.call(this,a,1,0,b,c);F(this,Vg,this.show,false,this);F(this,"end",this.hide,false,this)};
o(Th,Qh);var Uh=function(a,b,c){Qh.call(this,a,0,1,b,c);F(this,Vg,this.show,false,this)};
o(Uh,Qh);var Vh=function(a,b,c){Ih.apply(this,arguments);if(b.length!=3||c.length!=3){throw Error("Start and end points must be 3D");return}var d=[Vg,Wg,"end"];F(this,d,this.gx,false,this)};
o(Vh,Ih);Vh.prototype.gx=function(a){var b="rgb("+a.jH().join(",")+")";this.element.style.backgroundColor=b};
var Wh=function(a,b,c){Ih.apply(this,arguments);if(b.length!=3||c.length!=3){throw Error("Start and end points must be 3D");return}var d=[Vg,Wg,"end"];F(this,d,this.gx,false,this)};
o(Wh,Ih);Wh.prototype.gx=function(a){var b="rgb("+a.jH().join(",")+")";this.element.style.color="rgb("+b+")"};var Xh=function(a,b,c){this.target=a;this.handle=b||a;this.Ea=true;this.Ht=false;this.limits=c||new ke;this.ke=this.target.ownerDocument||this.target.document;this.screenX=0;this.screenY=0;this.deltaX=0;this.deltaY=0;F(this.handle,I,this.$i,false,this);F(this.ke,fg,this.AM,false,this);F(this.ke,eg,this.il,false,this)};
o(Xh,J);Xh.prototype.HA=function(){return this.Ea};
Xh.prototype.Ca=function(a){this.Ea=a};
Xh.prototype.b=function(){if(this.W())return;J.prototype.b.call(this);G(this.handle,I,this.$i,false,this);G(this.ke,fg,this.AM,false,this);G(this.ke,eg,this.il,false,this);delete this.target;delete this.handle};
Xh.prototype.$i=function(a){if(this.Ea&&!this.Ht){var b=this.dispatchEvent(new Yh("start",this,a.clientX,a.clientY,a));if(b!==false){this.screenX=a.screenX;this.screenY=a.screenY;this.deltaX=this.target.offsetLeft;this.deltaY=this.target.offsetTop;this.Ht=true;a.preventDefault()}}};
Xh.prototype.il=function(a){if(this.Ht){this.Ht=false;var b=this.VL(this.deltaX),c=this.WL(this.deltaY);this.dispatchEvent(new Yh("end",this,a.clientX,a.clientY,a,b,c))}};
Xh.prototype.AM=function(a){if(this.Ht&&this.Ea){var b=a.screenX-this.screenX,c=a.screenY-this.screenY;this.deltaX+=b;this.deltaY+=c;var d=this.VL(this.deltaX),e=this.WL(this.deltaY);this.screenX=a.screenX;this.screenY=a.screenY;var f=this.dispatchEvent(new Yh("beforedrag",this,a.clientX,a.clientY,a,d,e));if(f!==false){this.QV(d,e);this.dispatchEvent(new Yh("drag",this,a.clientX,a.clientY,a,d,e));a.preventDefault()}}};
Xh.prototype.VL=function(a){var b=this.limits,c=typeof b.left!="undefined"?b.left:null,d=typeof b.width!="undefined"?b.width:0,e=c!=null?c+d:Infinity,f=c!=null?c:-Infinity;return Math.min(e,Math.max(f,a))};
Xh.prototype.WL=function(a){var b=this.limits,c=typeof b.top!="undefined"?b.top:null,d=typeof b.height!="undefined"?b.height:0,e=c!=null?c+d:Infinity,f=c!=null?c:-Infinity;return Math.min(e,Math.max(f,a))};
Xh.prototype.QV=function(a,b){this.target.style.left=a+"px";this.target.style.top=b+"px"};
var Yh=function(a,b,c,d,e,f,g){D.call(this,a);this.type=a;this.clientX=c;this.clientY=d;this.browserEvent=e;this.left=pa(f)?f:b.deltaX;this.top=pa(g)?g:b.deltaY;this.dragger=b};
o(Yh,D);var Zh=function(){this.ta=[];this.Ee=[];this.wD=[]};
o(Zh,J);Zh.prototype.MB=false;Zh.prototype.NB=false;Zh.prototype.Zo=null;Zh.prototype.Gy=null;Zh.prototype.Gt=null;Zh.prototype.xk=null;Zh.prototype.Qx=null;Zh.prototype.Wd=false;var $h=5;Zh.prototype.L2=function(){return this.Wd};
Zh.prototype.ub=function(){throw Error("Call to pure virtual method");};
Zh.prototype.TF=function(a){this.Ee.push(a);a.NB=true;this.MB=true};
Zh.prototype.Vd=function(){if(this.Wd)return;for(var a,b=0;a=this.ta[b];b++)this.qL(a);this.Wd=true};
Zh.prototype.qL=function(a){if(this.MB){F(a.element,I,a.bm,false,a);if(this.xk)C(a.element,this.xk)}if(this.NB&&this.Qx)C(a.element,this.Qx)};
Zh.prototype.w6=function(){for(var a,b=0;a=this.ta[b];b++){if(this.MB){G(a.element,I,a.bm,false,a);if(this.xk)qf(a.element,this.xk)}if(this.NB&&this.Qx)qf(a.element,this.Qx)}this.ta.length=0};
Zh.prototype.$i=function(a,b){if(this.ff)return;this.ff=b;var c=new ai("dragstart",this,this.ff);if(this.dispatchEvent(c)==false){c.b();this.ff=null;return}c.b();var d=b.fu();this.Of=this.ZU(d);Ie(d).body.appendChild(this.Of);this.fd=this.bV(d,this.Of,a);F(this.fd,"drag",this.tC,false,this);F(this.fd,"end",this.il,false,this);this.h6();this.ij=null;this.n2();this.fd.$i(a);a.preventDefault()};
Zh.prototype.h6=function(){this.ks=[];for(var a,b=0;a=this.Ee[b];b++)for(var c,d=0;c=a.ta[d];d++)this.KF(a,c);if(!this.cj)this.cj=new ce(0,0,0,0)};
Zh.prototype.bV=function(a,b,c){var d=this.tY(a,c);b.style.position="absolute";b.style.left=d.x+"px";b.style.top=d.y+"px";return new Xh(b)};
Zh.prototype.il=function(a){var b=this.ij;if(b&&b.Ec){var c=a.clientX,d=a.clientY,e=this.KJ(),f=c+e.x,g=d+e.y,h;if(this.Zo)h=this.Zo(b.Xd,b.Nb,f,g);var i=new ai("drag",this,this.ff,b.Ec,b.Xd,b.e,c,d,f,g);this.dispatchEvent(i);i.b();var k=new ai("drop",this,this.ff,b.Ec,b.Xd,b.e,c,d,f,g,h);b.Ec.dispatchEvent(k);k.b()}var n=new ai("dragend",this,this.ff);this.dispatchEvent(n);n.b();G(this.fd,"drag",this.tC,false,this);G(this.fd,"end",this.il,false,this);this.bT(this.ij?this.ij.Xd:null)};
Zh.prototype.bT=function(){this.rW()};
Zh.prototype.rW=function(){this.tW();this.fd.b();A(this.Of);delete this.ff;delete this.Of;delete this.fd;delete this.ks;delete this.ij};
Zh.prototype.tC=function(a){var b=a.clientX,c=a.clientY,d=this.KJ();b+=d.x;c+=d.y;var e=this.ij,f;if(e){if(this.Zo)f=this.Zo(e.Xd,e.Nb,b,c);if(this.ov(b,c,e.Nb)&&f==this.Gy)return;if(e.Ec){var g=new ai("dragout",this,this.ff,e.Ec,e.Xd,e.e);this.dispatchEvent(g);g.b();var h=new ai(this,this.ff,e.Ec,e.Xd,e.e,this.Gy);e.Ec.dispatchEvent(h);h.b()}this.Gy=f;this.ij=null}if(!this.ov(b,c,this.cj))return;e=(this.ij=this.MZ(b,c));if(e&&e.Ec){if(this.Zo)f=this.Zo(e.Xd,e.Nb,b,c);var i=new ai("dragover",this,
this.ff,e.Ec,e.Xd,e.e);i.subtarget=f;this.dispatchEvent(i);i.b();var k=new ai("dragover",this,this.ff,e.Ec,e.Xd,e.e,a.clientX,a.clientY,undefined,undefined,f);e.Ec.dispatchEvent(k);k.b()}if(!e)this.cV(b,c)};
Zh.prototype.n2=function(){var a,b,c,d;for(b=0;a=this.wD[b];b++){F(a.e,"scroll",this.fH,false,this);a.eH=[];a.p7=a.e.scrollLeft;a.q7=a.e.scrollTop;var e=uh(a.e),f=Ah(a.e);a.Nb=new ce(e.y,e.x+f.width,e.y+f.height,e.x)}for(b=0;d=this.ks[b];b++)for(c=0;a=this.wD[c];c++)if(df(a.e,d.e)){a.eH.push(d);d.vD=a}};
Zh.prototype.tW=function(){for(var a=0,b;b=this.wD[a];a++){G(b.e,"scroll",this.fH,false,this);b.eH=[]}};
Zh.prototype.fH=function(a){for(var b=0,c;c=this.wD[b];b++)if(a.target==c.e){var d=c.q7-c.e.scrollTop,e=c.p7-c.e.scrollLeft;c.q7=c.e.scrollTop;c.p7=c.e.scrollLeft;for(var f=0,g;g=c.eH[f];f++){g.Nb.top+=d;g.Nb.left+=e;g.Nb.bottom+=d;g.Nb.right+=e}}};
Zh.prototype.ZU=function(a){var b=this.TG(a);if(this.Gt)C(b,this.Gt);return b};
Zh.prototype.tY=function(a){var b=uh(a);b.x+=(parseInt(rh(a,"marginLeft"),10)||0)*2;b.y+=(parseInt(rh(a,"marginTop"),10)||0)*2;return b};
Zh.prototype.TG=function(a){switch(a.tagName.toLowerCase()){case "tr":return v("table",null,v("tbody",null,a.cloneNode(true)));case "td":case "th":return v("table",null,v("tbody",null,v("tr",null,a.cloneNode(true))));default:return a.cloneNode(true)}};
Zh.prototype.KF=function(a,b){var c=b.uY(),d=this.ks;for(var e=0;e<c.length;e++){var f=c[e],g=uh(f),h=Ah(f),i=new ce(g.y,g.x+h.width,g.y+h.height,g.x);d.push(new bi(i,a,b,f));if(d.length==1)this.cj=new ce(i.top,i.right,i.bottom,i.left);else{var k=this.cj;k.left=Math.min(i.left,k.left);k.right=Math.max(i.right,k.right);k.top=Math.min(i.top,k.top);k.bottom=Math.max(i.bottom,k.bottom)}}};
Zh.prototype.cV=function(a,b){var c=new ce(this.cj.top,this.cj.right,this.cj.bottom,this.cj.left);for(var d,e=0;d=this.ks[e];e++){if(d.Nb.right<=a&&d.Nb.right>c.left)c.left=d.Nb.right;if(d.Nb.left>=a&&d.Nb.left<c.right)c.right=d.Nb.left;if(d.Nb.bottom<=b&&d.Nb.bottom>c.top)c.top=d.Nb.bottom;if(d.Nb.top>=b&&d.Nb.top<c.bottom)c.bottom=d.Nb.top}this.ij=new bi(c)};
Zh.prototype.MZ=function(a,b){for(var c,d=0;c=this.ks[d];d++)if(this.ov(a,b,c.Nb))if(c.vD){var e=c.vD.Nb;if(this.ov(a,b,e))return c}else return c;return null};
Zh.prototype.ov=function(a,b,c){return a>c.left&&a<c.right&&b>c.top&&b<c.bottom};
Zh.prototype.KJ=function(){var a=Ie(this.Of),b=Te(a);return Se(b)};
Zh.prototype.b=function(){if(!this.W()){Zh.f.b.call(this);this.w6()}};
var ai=function(a,b,c,d,e,f,g,h,i,k,n){D.call(this,a);this.dragSource=b;this.dragSourceItem=c;this.dropTarget=d;this.dropTargetItem=e;this.dropTargetElement=f;this.clientX=g;this.clientY=h;this.viewportX=i;this.viewportY=k;this.subtarget=n};
o(ai,D);ai.prototype.b=function(){if(!this.W()){ai.f.b.call(this);this.dragSource=undefined;this.dragSourceItem=undefined;this.dropTarget=undefined;this.dropTargetItem=undefined;this.dropTargetElement=undefined}};
var ci=function(a,b){this.element=u(a);this.data=b;this.P=null;if(!this.element)throw Error("Invalid argument");};
o(ci,J);ci.prototype.bn=null;ci.prototype.FA=function(a){return a};
ci.prototype.fu=function(){return this.bn};
ci.prototype.uY=function(){return[this.element]};
ci.prototype.bm=function(a){var b=this.FA(a.target);if(b)this.V3(a,b)};
ci.prototype.V3=function(a,b){F(b,fg,this.ko,false,this);F(b,Hf,this.ko,false,this);F(b,eg,this.rC,false,this);this.bn=b;this.$P=new ge(a.clientX,a.clientY)};
ci.prototype.ko=function(a){var b=Math.abs(a.clientX-this.$P.x)+Math.abs(a.clientY-this.$P.y);if(b>$h){var c=this.bn;G(c,fg,this.ko,false,this);G(c,Hf,this.ko,false,this);G(c,eg,this.rC,false,this);this.P.$i(a,this)}};
ci.prototype.rC=function(){var a=this.bn;G(a,fg,this.ko,false,this);G(a,Hf,this.ko,false,this);G(a,eg,this.rC,false,this);delete this.$P;this.bn=null};
var bi=function(a,b,c,d){this.Nb=a;this.Ec=b;this.Xd=c;this.e=d};
bi.prototype.vD=null;var di=function(a,b){Zh.call(this);var c=new ci(a,b);if(c){c.P=this;this.ta.push(c)}};
o(di,Zh);var ei=function(){Zh.call(this)};
o(ei,Zh);ei.prototype.ub=function(a,b){var c=new ci(a,b);this.GS(c)};
ei.prototype.GS=function(a){a.P=this;this.ta.push(a);if(this.L2())this.qL(a)};var fi;;var gi=function(a){fi=a},
hi=function(){if(fi==null)fi="en";return fi},
ii="DateTimeConstants",ji="NumberFormatConstants",li=function(a,b,c){if(!ki[b])ki[b]={};ki[b][c]=a;if(fi==null)fi=c},
mi=function(a,b){return a in ki&&b in ki[a]},
ki={},ni=function(a,b){li(a,ii,b)},
oi=function(a,b){li(a,ji,b)},
pi=function(a,b){var c=b?b:hi();if(!(a in ki))ki[a]={};return ki[a][c]},
qi={DECIMAL_SEP:".",GROUP_SEP:",",PERCENT:"%",ZERO_DIGIT:"0",PLUS_SIGN:"+",MINUS_SIGN:"-",EXP_SYMBOL:"E",PERMILL:"\u2030",INFINITY:"\u221e",NAN:"NaN",MONETARY_SEP:".",MONETARY_GROUP_SEP:",",DECIMAL_PATTERN:"#,##0.###",SCIENTIFIC_PATTERN:"0.###E0",PERCENT_PATTERN:"#,##0%",CURRENCY_PATTERN:"\u00a4#,##0.00",DEF_CURRENCY_CODE:"USD"},ri={ERAS:["BC","AD"],ERANAMES:["Before Christ","Anno Domini"],NARROWMONTHS:["J","F","M","A","M","J","J","A","S","O","N","D"],MONTHS:["January","February","March","April",
"May","June","July","August","September","October","November","December"],SHORTMONTHS:["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"],WEEKDAYS:["Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"],SHORTWEEKDAYS:["Sun","Mon","Tue","Wed","Thu","Fri","Sat"],NARROWWEEKDAYS:["S","M","T","W","T","F","S"],SHORTQUARTERS:["Q1","Q2","Q3","Q4"],QUARTERS:["1st quarter","2nd quarter","3rd quarter","4th quarter"],AMPMS:["AM","PM"],DATEFORMATS:["EEEE, MMMM d, yyyy","MMMM d, yyyy",
"MMM d, yyyy","M/d/yy"],TIMEFORMATS:["h:mm:ss a v","h:mm:ss a z","h:mm:ss a","h:mm a"],ZONESTRINGS:null},si=ni,ti=oi;var ui=function(){},
wi=function(a){if(typeof a=="number")return vi(a);var b=new ui;b.timeZoneId=a[0];b.standardOffset=-a[1];b.tzNames=a[2];b.transitions=a[3];return b},
vi=function(a){var b=new ui;b.standardOffset=a;b.timeZoneId=xi(a);var c=yi(a);b.tzNames=[c,c];b.transitions=[];return b},
zi=function(a){var b=["GMT"];b.push(a<=0?"+":"-");a=Math.abs(a);b.push(wb(Math.floor(a/60)%100,2),":",wb(a%60,2));return b.join("")},
xi=function(a){if(a==0)return"Etc/GMT";var b=["Etc/GMT",a<0?"-":"+"];a=Math.abs(a);b.push(Math.floor(a/60)%100);a=a%60;if(a!=0)b.push(":",wb(a,2));return b.join("")},
yi=function(a){if(a==0)return"UTC";var b=["UTC",a<0?"+":"-"];a=Math.abs(a);b.push(Math.floor(a/60)%100);a=a%60;if(a!=0)b.push(":",a);return b.join("")};
ui.prototype.ZI=function(a){var b=a.getTime()/1000/3600,c=0;while(c<this.transitions.length&&b>=this.transitions[c])c+=2;return c==0?0:this.transitions[c-1]};
ui.prototype.EY=function(a){return zi(this.Bl(a))};
ui.prototype.WY=function(a){return this.tzNames[this.yL(a)?3:1]};
ui.prototype.Bl=function(a){return this.standardOffset-this.ZI(a)};
ui.prototype.rZ=function(a){var b=-this.Bl(a),c=[b<0?"-":"+"];b=Math.abs(b);c.push(wb(Math.floor(b/60)%100,2),wb(b%60,2));return c.join("")};
ui.prototype.wu=function(a){return this.tzNames[this.yL(a)?2:0]};
ui.prototype.yL=function(a){return this.ZI(a)>0};var Ai=function(){this.ga=pi("DateTimeConstants",hi());this.oc=[]},
Bi=[/^\'(?:[^\']|\'\')*\'/,/^(?:G+|y+|M+|k+|S+|E+|a+|h+|K+|H+|c+|L+|Q+|d+|m+|s+|v+|z+|Z+)/,/^[^\'GyMkSEahKHcLQdmsvzZ]+/];Ai.prototype.Kf=function(a){while(a)for(var b=0;b<Bi.length;++b){var c=a.match(Bi[b]);if(c){var d=c[0];a=a.substring(d.length);if(b==0)if(d=="''")d="'";else{d=d.substring(1,d.length-1);d=d.replace(/\'\'/,"'")}this.oc.push({text:d,type:b});break}}};
Ai.prototype.Cn=function(a,b){if(!b)b=wi(a.getTimezoneOffset());var c=(a.getTimezoneOffset()-b.Bl(a))*60000,d=c?new Date(a.getTime()+c):a,e=d;if(d.getTimezoneOffset()!=a.getTimezoneOffset()){c+=c>0?-86400000:86400000;e=new Date(a.getTime()+c)}var f=[];for(var g=0;g<this.oc.length;++g){var h=this.oc[g].text;if(1==this.oc[g].type)f.push(this.CX(h,a,d,e,b));else f.push(h)}return f.join("")};
Ai.prototype.lj=function(a){var b;if(a<4)b=this.ga.DATEFORMATS[a];else if(a<8)b=this.ga.TIMEFORMATS[a-4];else if(a<12)b=this.ga.DATEFORMATS[a-8]+" "+this.ga.TIMEFORMATS[a-8];else this.lj(10);return this.Kf(b)};
Ai.prototype.BX=function(a,b){var c=b.getFullYear()>0?1:0;return a>=4?this.ga.ERANAMES[c]:this.ga.ERAS[c]};
Ai.prototype.MX=function(a,b){var c=b.getFullYear();if(c<0)c=-c;return a==2?wb(c%100,2):String(c)};
Ai.prototype.FX=function(a,b){var c=b.getMonth();switch(a){case 5:return this.ga.NARROWMONTHS[c];case 4:return this.ga.MONTHS[c];case 3:return this.ga.SHORTMONTHS[c];default:return wb(c+1,a)}};
Ai.prototype.vX=function(a,b){return wb(b.getHours()||24,a)};
Ai.prototype.DX=function(a,b){var c=b.getTime()%1000/1000;return c.toFixed(Math.min(3,a)).substr(2)+(a>3?wb(0,a-3):"")};
Ai.prototype.zX=function(a,b){var c=b.getDay();return a>=4?this.ga.WEEKDAYS[c]:this.ga.SHORTWEEKDAYS[c]};
Ai.prototype.wX=function(a,b){var c=b.getHours();return this.ga.AMPMS[c>=12&&c<24?1:0]};
Ai.prototype.uX=function(a,b){return wb(b.getHours()%12||12,a)};
Ai.prototype.sX=function(a,b){return wb(b.getHours()%12,a)};
Ai.prototype.tX=function(a,b){return wb(b.getHours(),a)};
Ai.prototype.IX=function(a,b){var c=b.getDay();switch(a){case 5:return this.ga.STANDALONENARROWWEEKDAYS[c];case 4:return this.ga.STANDALONEWEEKDAYS[c];case 3:return this.ga.STANDALONESHORTWEEKDAYS[c];default:return wb(c,1)}};
Ai.prototype.JX=function(a,b){var c=b.getMonth();switch(a){case 5:return this.ga.STANDALONENARROWMONTHS[c];case 4:return this.ga.STANDALONEMONTHS[c];case 3:return this.ga.STANDALONESHORTMONTHS[c];default:return wb(c+1,a)}};
Ai.prototype.GX=function(a,b){var c=Math.floor(b.getMonth()/3);return a<4?this.ga.SHORTQUARTERS[c]:this.ga.QUARTERS[c]};
Ai.prototype.yX=function(a,b){return wb(b.getDate(),a)};
Ai.prototype.EX=function(a,b){return wb(b.getMinutes(),a)};
Ai.prototype.HX=function(a,b){return wb(b.getSeconds(),a)};
Ai.prototype.KX=function(a,b,c){return a<4?c.rZ(b):c.EY(b)};
Ai.prototype.LX=function(a,b,c){return a<4?c.wu(b):c.WY(b)};
Ai.prototype.CX=function(a,b,c,d,e){var f=a.length;switch(a.charAt(0)){case "G":return this.BX(f,c);case "y":return this.MX(f,c);case "M":return this.FX(f,c);case "k":return this.vX(f,d);case "S":return this.DX(f,d);case "E":return this.zX(f,c);case "a":return this.wX(f,d);case "h":return this.uX(f,d);case "K":return this.sX(f,d);case "H":return this.tX(f,d);case "c":return this.IX(f,c);case "L":return this.JX(f,c);case "Q":return this.GX(f,c);case "d":return this.yX(f,c);case "m":return this.EX(f,
d);case "s":return this.HX(f,d);case "v":return e.timeZoneId;case "z":return this.LX(f,b,e);case "Z":return this.KX(f,b,e);default:return""}};var Ci=function(){this.ga=pi("DateTimeConstants",hi());this.oc=[]};
Ci.prototype.year=0;Ci.prototype.jo=0;Ci.prototype.FH=0;Ci.prototype.Ac=0;Ci.prototype.ld=0;Ci.prototype.md=0;Ci.prototype.Tv=0;Ci.prototype.Kf=function(a){var b=false,c="";for(var d=0;d<a.length;d++){var e=a.charAt(d);if(e==" "){if(c.length>0){this.oc.push({text:c,count:0,abutStart:false});c=""}this.oc.push({text:" ",count:0,abutStart:false});while(d+1<a.length&&a.charAt(d+1)==" ")d++}else if(b)if(e=="'")if(d+1<a.length&&a.charAt(d+1)=="'"){c+=e;++d}else b=false;else c+=e;else if("GyMdkHmsSEDahKzZv".indexOf(e)>=
0){if(c.length>0){this.oc.push({text:c,count:0,abutStart:false});c=""}var f=this.aZ(a,d);this.oc.push({text:e,count:f,abutStart:false});d+=f-1}else if(e=="'")if(d+1<a.length&&a.charAt(d+1)=="'"){c+="'";d++}else b=true;else c+=e}if(c.length>0)this.oc.push({text:c,count:0,abutStart:false});this.H3()};
Ci.prototype.lj=function(a){var b;if(a<4)b=this.ga.DATEFORMATS[a];else if(a<8)b=this.ga.TIMEFORMATS[a-4];else if(a<12){b=this.ga.DATEFORMATS[a-8];b+=" ";b+=this.ga.TIMEFORMATS[a-8]}else return this.lj(10);return this.Kf(b)};
Ci.prototype.parse=function(a,b,c){return this.z2(a,b,c,false)};
Ci.prototype.z2=function(a,b,c,d){var e=new Di,f=[b],g=-1,h=0,i=0;for(var k=0;k<this.oc.length;++k)if(this.oc[k].count>0){if(g<0&&this.oc[k].abutStart){g=k;h=b;i=0}if(g>=0){var n=this.oc[k].count;if(k==g){n-=i;i++;if(n==0)return 0}if(!this.eQ(a,f,this.oc[k],n,e)){k=g-1;f[0]=h;continue}}else{g=-1;if(!this.eQ(a,f,this.oc[k],0,e))return 0}}else{g=-1;if(this.oc[k].text.charAt(0)==" "){var q=f[0];this.WP(a,f);if(f[0]>q)continue}else if(a.indexOf(this.oc[k].text,f[0])==f[0]){f[0]+=this.oc[k].text.length;
continue}return 0}return e.MT(c,d)?f[0]-b:0};
Ci.prototype.aZ=function(a,b){var c=a.charAt(b),d=b+1;while(d<a.length&&a.charAt(d)==c)++d;return d-b};
Ci.prototype.KB=function(a){if(a.count<=0)return false;var b="MydhHmsSDkK".indexOf(a.text.charAt(0));return b>0||b==0&&a.count<3};
Ci.prototype.H3=function(){var a=false;for(var b=0;b<this.oc.length;b++)if(this.KB(this.oc[b])){if(!a&&b+1<this.oc.length&&this.KB(this.oc[b+1])){a=true;this.oc[b].abutStart=true}}else a=false};
Ci.prototype.WP=function(a,b){var c=a.substring(b[0]).match(/^\s+/);if(c)b[0]+=c[0].length};
Ci.prototype.eQ=function(a,b,c,d,e){this.WP(a,b);var f=b[0],g=c.text.charAt(0),h=-1;if(this.KB(c))if(d>0){if(f+d>a.length)return false;h=this.Ar(a.substring(0,f+d),b)}else h=this.Ar(a,b);switch(g){case "G":e.era=this.ho(a,b,this.ga.ERAS);return true;case "M":return this.m$(a,b,e,h);case "E":return this.k$(a,b,e);case "a":e.ampm=this.ho(a,b,this.ga.AMPMS);return true;case "y":return this.n$(a,b,f,h,c,e);case "d":e.day=h;return true;case "S":return this.l$(h,b,f,e);case "h":if(h==12)h=0;case "K":case "H":case "k":e.Ac=
h;return true;case "m":e.ld=h;return true;case "s":e.md=h;return true;case "z":case "Z":case "v":return this.t$(a,b,e);default:return false}};
Ci.prototype.n$=function(a,b,c,d,e,f){var g;if(d<0){g=a.charAt(b[0]);if(g!="+"&&g!="-")return false;b[0]++;d=this.Ar(a,b);if(d<0)return false;if(g=="-")d=-d}if(g==null&&b[0]-c==2&&e.count==2)f.n9(d);else f.year=d;return true};
Ci.prototype.m$=function(a,b,c,d){if(d<0){d=this.ho(a,b,this.ga.MONTHS);if(d<0)d=this.ho(a,b,this.ga.SHORTMONTHS);if(d<0)return false;c.jo=d;return true}else{c.jo=d-1;return true}};
Ci.prototype.k$=function(a,b,c){var d=this.ho(a,b,this.ga.WEEKDAYS);if(d<0)d=this.ho(a,b,this.ga.SHORTWEEKDAYS);if(d<0)return false;c.dayOfWeek=d;return true};
Ci.prototype.l$=function(a,b,c,d){var e=b[0]-c;d.Tv=e<3?a*Math.pow(10,3-e):Math.round(a/Math.pow(10,e-3));return true};
Ci.prototype.t$=function(a,b,c){if(a.indexOf("GMT",b[0])==b[0]){b[0]+=3;return this.iN(a,b,c)}return this.iN(a,b,c)};
Ci.prototype.iN=function(a,b,c){if(b[0]>=a.length){c.tzOffset=0;return true}var d=1;switch(a.charAt(b[0])){case "-":d=-1;case "+":b[0]++}var e=b[0],f=this.Ar(a,b);if(f==0&&b[0]==e)return false;var g;if(b[0]<a.length&&a.charAt(b[0])==":"){g=f*60;b[0]++;e=b[0];f=this.Ar(a,b);if(f==0&&b[0]==e)return false;g+=f}else{g=f;if(g<24&&b[0]-e<=2)g*=60;else g=g%100+g/100*60}g*=d;c.tzOffset=-g;return true};
Ci.prototype.Ar=function(a,b){var c=a.substring(b[0]).match(/^\d+/);if(!c)return-1;b[0]+=c[0].length;return parseInt(c[0],10)};
Ci.prototype.ho=function(a,b,c){var d=0,e=-1,f=a.substring(b[0]).toLowerCase();for(var g=0;g<c.length;++g){var h=c[g].length;if(h>d&&f.indexOf(c[g].toLowerCase())==0){e=g;d=h}}if(e>=0)b[0]+=d;return e};
var Di=function(){};
Di.prototype.n9=function(a){var b=new Date,c=b.getFullYear()-80,d=c%100;this.ambiguousYear=a==d;a+=Math.floor(c/100)*100+(a<d?100:0);return this.year=a};
Di.prototype.MT=function(a,b){if(this.era!=undefined&&this.year!=undefined&&this.era==0&&this.year>0)this.year=-(this.year-1);if(this.year!=undefined)a.setFullYear(this.year);var c=a.getDate();a.setDate(1);if(this.jo!=undefined)a.setMonth(this.jo);if(this.day!=undefined)a.setDate(this.day);else a.setDate(c);if(this.Ac==undefined)this.Ac=a.getHours();if(this.ampm!=undefined&&this.ampm>0)if(this.Ac<12)this.Ac+=12;a.setHours(this.Ac);if(this.ld!=undefined)a.setMinutes(this.ld);if(this.md!=undefined)a.setSeconds(this.md);
if(this.Tv!=undefined)a.setMilliseconds(this.Tv);if(b&&(this.year!=undefined&&this.year!=a.getFullYear()||this.jo!=undefined&&this.jo!=a.getMonth()||this.FH!=undefined&&this.FH!=a.getDate()||this.Ac>=24||this.ld>=60||this.md>=60||this.Tv>=1000))return false;if(this.tzOffset!=undefined){var d=a.getTimezoneOffset();a.setTime(a.getTime()+(this.tzOffset-d)*60*1000)}if(this.ambiguousYear){var e=new Date;e.setFullYear(e.getFullYear()-80);if(a.getTime()<e.getTime())a.setFullYear(e.getFullYear()+100)}if(this.dayOfWeek!=
undefined)if(this.day==undefined){var f=(7+this.dayOfWeek-a.getDay())%7;if(f>3)f-=7;var g=a.getMonth();a.setDate(a.getDate()+f);if(a.getMonth()!=g)a.setDate(a.getDate()+(f>0?-7:7))}else if(this.dayOfWeek!=a.getDay())return false;return true};var Ei={USD:"$",ARS:"$",AWG:"\u0192",AUD:"$",BSD:"$",BBD:"$",BEF:"\u20a3",BZD:"$",BMD:"$",BOB:"$",BRL:"R$",BRC:"\u20a2",GBP:"\u00a3",BND:"$",KHR:"\u17db",CAD:"$",KYD:"$",CLP:"$",CNY:"\u5143",COP:"\u20b1",CRC:"\u20a1",CUP:"\u20b1",CYP:"\u00a3",DKK:"kr",DOP:"\u20b1",XCD:"$",EGP:"\u00a3",SVC:"\u20a1",EUR:"\u20ac",XEU:"\u20a0",FKP:"\u00a3",FJD:"$",FRF:"\u20a3",GIP:"\u00a3",GRD:"\u20af",GGP:"\u00a3",GYD:"$",NLG:"\u0192",HKD:"$",INR:"\u20a8",IRR:"\ufdfc",IEP:"\u00a3",IMP:"\u00a3",ILS:"\u20aa",ITL:"\u20a4",
JMD:"$",JPY:"\u00a5",JEP:"\u00a3",KPW:"\u20a9",KRW:"\u20a9",LAK:"\u20ad",LBP:"\u00a3",LRD:"$",LUF:"\u20a3",MTL:"\u20a4",MUR:"\u20a8",MXN:"$",MNT:"\u20ae",NAD:"$",NPR:"\u20a8",ANG:"\u0192",NZD:"$",OMR:"\ufdfc",PKR:"\u20a8",PEN:"S/.",PHP:"\u20b1",QAR:"\ufdfc",RUB:"\u0440\u0443\u0431",SHP:"\u00a3",SAR:"\ufdfc",SCR:"\u20a8",SGD:"$",SBD:"$",ZAR:"R",ESP:"\u20a7",LKR:"\u20a8",SEK:"kr",SRD:"$",SYP:"\u00a3",TWD:"\u5143",THB:"\u0e3f",TTD:"$",TRY:"\u20a4",TRL:"\u20a4",TVD:"$",UYU:"\u20b1",VAL:"\u20a4",VND:"\u20ab",
YER:"\ufdfc",ZWD:"$"};var Fi=function(){this.ga=pi("NumberFormatConstants",hi())};
Fi.prototype.lj=function(a,b){switch(a){case 1:return this.Kf(this.ga.DECIMAL_PATTERN,b);case 2:return this.Kf(this.ga.SCIENTIFIC_PATTERN,b);case 3:return this.Kf(this.ga.PERCENT_PATTERN,b);case 4:return this.Kf(this.ga.CURRENCY_PATTERN,b)}return""};
Fi.prototype.Kf=function(a,b){this.xda=a;this.A2=b||this.ga.DEF_CURRENCY_CODE;this.Bba=Ei[this.A2];this.iC=40;this.Fh=1;this.sM=3;this.Vv=0;this.mC=0;this.Gr="";this.QC="";this.nr="-";this.vC="";this.ew=1;this.mK=3;this.OV=false;this.jv=false;this.$E=false;this.k5(this.xda)};
Fi.prototype.parse=function(a,b){var c=b||[0],d=c[0],e=0,f=a.indexOf(this.Gr,c[0])==c[0],g=a.indexOf(this.nr,c[0])==c[0];if(f&&g)if(this.Gr.length>this.nr.length)g=false;else if(this.Gr.length<this.nr.length)f=false;if(f)c[0]+=this.Gr.length;else if(g)c[0]+=this.nr.length;if(a.indexOf(this.ga.INFINITY,c[0])==c[0]){c[0]+=this.ga.INFINITY.length;e=Infinity}else e=this.j5(a,c);if(f){if(!(a.indexOf(this.QC,c[0])==c[0])){c[0]=d;return 0}c[0]+=this.QC.length}else if(g){if(!(a.indexOf(this.vC,c[0])==c[0])){c[0]=
d;return 0}c[0]+=this.vC.length}return g?-e:e};
Fi.prototype.j5=function(a,b){var c=false,d=false,e=false,f=1,g=this.jv?this.ga.MONETARY_SEP:this.ga.DECIMAL_SEP,h=this.jv?this.ga.MONETARY_GROUP_SEP:this.ga.GROUP_SEP,i=this.ga.EXP_SYMBOL,k="";for(;b[0]<a.length;b[0]++){var n=a.charAt(b[0]),q=this.rY(n);if(q>=0&&q<=9){k+=q;e=true}else if(n==g.charAt(0)){if(c||d)break;k+=".";c=true}else if(n==h.charAt(0)){if(c||d)break;continue}else if(n==i.charAt(0)){if(d)break;k+="E";d=true}else if(n=="+"||n=="-")k+=n;else if(n==this.ga.PERCENT.charAt(0)){if(f!=
1)break;f=100;if(e){b[0]++;break}}else if(n==this.ga.PERMILL.charAt(0)){if(f!=1)break;f=1000;if(e){b[0]++;break}}else break}return parseFloat(k)/f};
Fi.prototype.Cn=function(a){if(isNaN(a))return this.ga.NAN;var b=[],c=a<0||a==0&&1/a<0;b.push(c?this.nr:this.Gr);if(!isFinite(a))b.push(this.ga.INFINITY);else{a*=c?-1:1;a*=this.ew;this.$E?this.o$(a,b):this.pE(a,this.Fh,b)}b.push(c?this.vC:this.QC);return b.join("")};
Fi.prototype.pE=function(a,b,c){var d=Math.pow(10,this.sM);a=Math.round(a*d);var e=Math.floor(a/d),f=Math.floor(a-e*d),g=this.Vv>0||f>0,h="",i=e;while(i>1.0E20){h="0"+h;i=Math.round(i/10)}h=i+h;var k=this.jv?this.ga.MONETARY_SEP:this.ga.DECIMAL_SEP,n=this.jv?this.ga.MONETARY_GROUP_SEP:this.ga.GROUP_SEP,q=this.ga.ZERO_DIGIT.charCodeAt(0),t=h.length;if(e>0||b>0){for(var y=t;y<b;y++)c.push(this.ga.ZERO_DIGIT);for(var y=0;y<t;y++){c.push(String.fromCharCode(q+h.charAt(y)*1));if(t-y>1&&this.mK>0&&(t-y)%
this.mK==1)c.push(n)}}else if(!g)c.push(this.ga.ZERO_DIGIT);if(this.OV||g)c.push(k);var z=""+(f+d),O=z.length;while(z.charAt(O-1)=="0"&&O>this.Vv+1)O--;for(var y=1;y<O;y++)c.push(String.fromCharCode(q+z.charAt(y)*1))};
Fi.prototype.MF=function(a,b){b.push(this.ga.EXP_SYMBOL);if(a<0){a=-a;b.push(this.ga.MINUS_SIGN)}var c=""+a;for(var d=c.length;d<this.mC;d++)b.push(this.ga.ZERO_DIGIT);b.push(c)};
Fi.prototype.o$=function(a,b){if(a==0){this.pE(a,this.Fh,b);this.MF(0,b);return}var c=Math.floor(Math.log(a)/Math.log(10));a/=Math.pow(10,c);var d=this.Fh;if(this.iC>1&&this.iC>this.Fh){while(c%this.iC!=0){a*=10;c--}d=1}else if(this.Fh<1){c++;a/=10}else{c-=this.Fh-1;a*=Math.pow(10,this.Fh-1)}this.pE(a,d,b);this.MF(c,b)};
Fi.prototype.rY=function(a){var b=a.charCodeAt(0);if(48<=b&&b<58)return b-48;else{var c=this.ga.ZERO_DIGIT.charCodeAt(0);return c<=b&&b<c+10?b-c:-1}};
Fi.prototype.Aw=function(a,b){var c="",d=false,e=a.length;for(;b[0]<e;b[0]++){var f=a.charAt(b[0]);if(f=="'"){if(b[0]+1<e&&a.charAt(b[0]+1)=="'"){b[0]++;c+="'"}else d=!d;continue}if(d)c+=f;else switch(f){case "#":case "0":case ",":case ".":case ";":return c;case "\u00a4":this.jv=true;if(b[0]+1<e&&a.charAt(b[0]+1)=="\u00a4"){b[0]++;c+=this.A2}else c+=this.Bba;break;case "%":if(this.ew!=1)throw Error("Too many percent/permill");this.ew=100;c+=this.ga.PERCENT;break;case "\u2030":if(this.ew!=1)throw Error("Too many percent/permill");
this.ew=1000;c+=this.ga.PERMILL;break;default:c+=f}}return c};
Fi.prototype.m5=function(a,b){var c=-1,d=0,e=0,f=0,g=-1,h=a.length;for(var i=true;b[0]<h&&i;b[0]++){var k=a.charAt(b[0]);switch(k){case "#":if(e>0)f++;else d++;if(g>=0&&c<0)g++;break;case "0":if(f>0)throw Error('Unexpected "0" in pattern "'+a+'"');e++;if(g>=0&&c<0)g++;break;case ",":g=0;break;case ".":if(c>=0)throw Error('Multiple decimal separators in pattern "'+a+'"');c=d+e+f;break;case "E":if(this.$E)throw Error('Multiple exponential symbols in pattern "'+a+'"');this.$E=true;this.mC=0;while(b[0]+
1<h&&a.charAt(b[0]+1)==this.ga.ZERO_DIGIT.charAt(0)){b[0]++;this.mC++}if(d+e<1||this.mC<1)throw Error('Malformed exponential pattern "'+a+'"');i=false;break;default:b[0]--;i=false;break}}if(e==0&&d>0&&c>=0){var n=c;if(n==0)n++;f=d-n;d=n-1;e=1}if(c<0&&f>0||c>=0&&(c<d||c>d+e)||g==0)throw Error('Malformed pattern "'+a+'"');var q=d+e+f;this.sM=c>=0?q-c:0;if(c>=0){this.Vv=d+e-c;if(this.Vv<0)this.Vv=0}var t=c>=0?c:q;this.Fh=t-d;if(this.$E){this.iC=d+this.Fh;if(this.sM==0&&this.Fh==0)this.Fh=1}this.mK=Math.max(0,
g);this.OV=c==0||c==q};
Fi.prototype.k5=function(a){var b=[0];this.Gr=this.Aw(a,b);var c=b[0];this.m5(a,b);var d=b[0]-c;this.QC=this.Aw(a,b);if(b[0]<a.length&&a.charAt(b[0])==";"){b[0]++;this.nr=this.Aw(a,b);b[0]+=d;this.vC=this.Aw(a,b)}};var Gi=function(a,b,c){var d=new Ai;d.Kf(a);return d.Cn(b,c)},
Hi=function(a){var b=new Ai;b.lj(a);return b},
Ii=function(a,b,c,d){var e=new Ci;e.Kf(a);return e.parse(b,c,d)},
Ji=function(a,b){var c=new Fi;c.lj(a);return c.Cn(b)};;var Ki="complete",Li="success",Mi="error",Ni="timeout";var Oi=function(a,b,c,d,e){if(/[;=]/.test(a))throw Error('Invalid cookie name "'+a+'"');if(/;/.test(b))throw Error('Invalid cookie value "'+b+'"');if(!pa(c))c=-1;var f=e?";domain="+e:"",g=d?";path="+d:"",h;if(c<0)h="";else if(c==0){var i=new Date(1970,1,1);h=";expires="+i.toUTCString()}else{var k=new Date((new Date).getTime()+c*1000);h=";expires="+k.toUTCString()}document.cookie=a+"="+b+f+g+h},
Pi=function(a,b){var c=a+"=",d=String(document.cookie).split(/\s*;\s*/);for(var e=0,f;f=d[e];e++)if(f.indexOf(c)==0)return f.substr(c.length);return b},
Ri=function(a,b,c){var d=Qi(a);Oi(a,"",0,b,c);return d},
Qi=function(a){var b={};return Pi(a,b)!==b};var Si=function(a){J.call(this);this.Bi={};this.j=new K(this);this.P=a};
o(Si,J);Si.prototype.start=function(){Gc(this.Bi,this.l3,this)};
Si.prototype.l3=function(a,b){var c;if(this.P){var d=Je(this.P);c=d.m("img")}else c=new Image;this.Bi[b]=c;var e=s?"readystatechange":"load";this.j.g(c,[e,"abort",Mi],this.Q4);c.id=b;c.src=a};
Si.prototype.Q4=function(a){var b=a.currentTarget;if(b){if(s)if(b.readyState==Ki)a.type="load";else return;if(a.type=="load"){if(typeof b.naturalWidth=="undefined")b.naturalWidth=b.width;if(typeof b.naturalHeight=="undefined")b.naturalHeight=b.height}this.dispatchEvent({type:a.type,target:b});Nc(this.Bi,b.id);if(Mc(this.Bi)){this.dispatchEvent(Ki);if(this.j)this.j.ma()}}};
Si.prototype.b=function(){if(!this.W()){if(this.Bi)this.Bi=null;if(this.j){this.j.b();this.j=null}Si.f.b.call(this)}};var N=function(a,b){var c;if(a instanceof N){this.xm(b==null?a.JY():b);this.xx(a.YJ());this.Cx(a.fK());this.ix(a.ul());this.vx(a.Dl());this.Wr(a.Nj());this.Am(a.El().ra());this.Oo(a.JA())}else if(a&&(c=String(a).match(Ti()))){this.xm(!(!b));this.xx(c[1],true);this.Cx(c[2],true);this.ix(c[3],true);this.vx(c[4]);this.Wr(c[5],true);this.Am(c[6]);this.Oo(c[7],true)}else{this.xm(!(!b));this.bd=new Ui(null,this,this.zh)}};
N.prototype.Si="";N.prototype.dp="";N.prototype.pn="";N.prototype.wo=null;N.prototype.ik="";N.prototype.bd=null;N.prototype.En="";N.prototype.S2=false;N.prototype.zh=false;N.prototype.toString=function(){if(this.ie)return this.ie;var a=[];if(this.Si)a.push(Vi(this.Si,Wi),":");if(this.pn){a.push("//");if(this.dp)a.push(Vi(this.dp,Wi),"@");a.push(Xi(this.pn));if(this.wo!=null)a.push(":",String(this.Dl()))}if(this.ik)a.push(Vi(this.ik,Yi));var b=String(this.bd);if(b)a.push("?",b);if(this.En)a.push("#",
Xi(this.En));return this.ie=a.join("")};
N.prototype.X6=function(a){var b=this.ra(),c=a.B1();if(c)b.xx(a.YJ());else c=a.D1();if(c)b.Cx(a.fK());else c=a.u1();if(c)b.ix(a.ul());else c=a.Cq();var d=a.Nj();if(c)b.vx(a.Dl());else{c=a.w1();if(c)if(!/^\//.test(d))d=b.Nj().replace(/\/?[^\/]*$/,"/"+d)}if(c)b.Wr(d);else c=a.z1();if(c)b.Am(a.ti());else c=a.v1();if(c)b.Oo(a.JA());return b};
N.prototype.ra=function(){return new Zi(this.Si,this.dp,this.pn,this.wo,this.ik,this.bd.ra(),this.En,this.zh)};
N.prototype.YJ=function(){return this.Si};
N.prototype.xx=function(a,b){this.oh();delete this.ie;this.Si=b?$i(a):a;if(this.Si)this.Si=this.Si.replace(/:$/,"");return this};
N.prototype.B1=function(){return!(!this.Si)};
N.prototype.fK=function(){return this.dp};
N.prototype.Cx=function(a,b){this.oh();delete this.ie;this.dp=b?$i(a):a;return this};
N.prototype.D1=function(){return!(!this.dp)};
N.prototype.ul=function(){return this.pn};
N.prototype.ix=function(a,b){this.oh();delete this.ie;this.pn=b?$i(a):a;return this};
N.prototype.u1=function(){return!(!this.pn)};
N.prototype.Dl=function(){return this.wo};
N.prototype.vx=function(a){this.oh();delete this.ie;if(a){a=Number(a);if(isNaN(a)||a<0)throw Error("Bad port number "+a);this.wo=a}else this.wo=null;return this};
N.prototype.Cq=function(){return this.wo!=null};
N.prototype.Nj=function(){return this.ik};
N.prototype.Wr=function(a,b){this.oh();delete this.ie;this.ik=b?$i(a):a;return this};
N.prototype.w1=function(){return!(!this.ik)};
N.prototype.z1=function(){return this.bd!==null&&this.bd.toString()!==""};
N.prototype.Am=function(a){this.oh();delete this.ie;if(a instanceof Ui){this.bd=a;this.bd.ea=this;this.bd.xm(this.zh)}else this.bd=new Ui(a,this,this.zh);return this};
N.prototype.ti=function(){return this.bd.toString()};
N.prototype.El=function(){return this.bd};
N.prototype.fa=function(a,b){this.oh();delete this.ie;this.bd.J(a,b);return this};
N.prototype.VD=function(a,b){this.oh();delete this.ie;if(!xa(b))b=[String(b)];this.bd.x9(a,b);return this};
N.prototype.Ga=function(a){return this.bd.get(a)};
N.prototype.JA=function(){return this.En};
N.prototype.Oo=function(a,b){this.oh();delete this.ie;this.En=b?$i(a):a;return this};
N.prototype.v1=function(){return!(!this.En)};
N.prototype.D3=function(){this.oh();this.fa("zx",xb());return this};
N.prototype.oh=function(){if(this.S2)throw Error("Tried to modify a read-only Uri");};
N.prototype.xm=function(a){this.zh=a;if(this.bd)this.bd.xm(a)};
N.prototype.JY=function(){return this.zh};
var aj=function(a,b){return a instanceof N?a.ra():new N(a,b)},
Zi=function(a,b,c,d,e,f,g,h){var i=new N(null,h);i.xx(a);i.Cx(b);i.ix(c);i.vx(d);i.Wr(e);i.Am(f);i.Oo(g);return i},
$i=function(a){return a?decodeURIComponent(a):""},
Xi=function(a){if(za(a))return encodeURIComponent(a);return null},
bj=/^[a-zA-Z0-9\-_.!~*'():\/;?]*$/,Vi=function(a,b){var c=null;if(za(a)){c=a;if(!bj.test(c))c=encodeURI(a);if(c.search(b)>=0)c=c.replace(b,cj)}return c},
cj=function(a){var b=a.charCodeAt(0);return"%"+(b>>4&15).toString(16)+(b&15).toString(16)},
dj=null,Ti=function(){if(!dj)dj=/^(?:([^:\/?#]+):)?(?:\/\/(?:([^\/?#]*)@)?([^\/?#:@]*)(?::([0-9]+))?)?([^?#]+)?(?:\?([^#]*))?(?:#(.*))?$/;return dj},
Wi=/[#\/\?@]/g,Yi=/[\#\?]/g,Ui=function(a,b,c){this.Cc=new dd;this.ea=b;this.zh=!(!c);if(a){var d=a.split("&");for(var e=0;e<d.length;e++){var f=d[e].indexOf("="),g,h;if(f>=0){g=d[e].substring(0,f);h=d[e].substring(f+1)}else g=d[e];g=bb(g);g=this.Jj(g);this.add(g,h?bb(h):"")}}};
Ui.prototype.Ma=0;Ui.prototype.S=function(){return this.Ma};
Ui.prototype.add=function(a,b){this.Wn();a=this.Jj(a);if(!this.Ob(a))this.Cc.J(a,b);else{var c=this.Cc.get(a);if(xa(c))c.push(b);else this.Cc.J(a,[c,b])}this.Ma++;return this};
Ui.prototype.remove=function(a){a=this.Jj(a);if(this.Cc.Ob(a)){this.Wn();var b=this.Cc.get(a);if(xa(b))this.Ma-=b.length;else this.Ma--;return this.Cc.remove(a)}return false};
Ui.prototype.clear=function(){this.Wn();this.Cc.clear();this.Ma=0};
Ui.prototype.isEmpty=function(){return this.Ma==0};
Ui.prototype.Ob=function(a){a=this.Jj(a);return this.Cc.Ob(a)};
Ui.prototype.gh=function(a){var b=this.xb();return mc(b,a)};
Ui.prototype.tc=function(){var a=this.Cc.xb(),b=this.Cc.tc(),c=[];for(var d=0;d<b.length;d++){var e=a[d];if(xa(e))for(var f=0;f<e.length;f++)c.push(b[d]);else c.push(b[d])}return c};
Ui.prototype.xb=function(a){var b;if(a){var c=this.Jj(a);if(this.Ob(c)){var d=this.Cc.get(c);if(xa(d))return d;else{b=[];b.push(d)}}else b=[]}else{var e=this.Cc.xb();b=[];for(var f=0;f<e.length;f++){var g=e[f];if(xa(g))vc(b,g);else b.push(g)}}return b};
Ui.prototype.J=function(a,b){this.Wn();a=this.Jj(a);if(this.Ob(a)){var c=this.Cc.get(a);if(xa(c))this.Ma-=c.length;else this.Ma--}this.Cc.J(a,b);this.Ma++;return this};
Ui.prototype.get=function(a,b){a=this.Jj(a);if(this.Ob(a)){var c=this.Cc.get(a);return xa(c)?c[0]:c}else return b};
Ui.prototype.x9=function(a,b){this.Wn();a=this.Jj(a);if(this.Ob(a)){var c=this.Cc.get(a);if(xa(c))this.Ma-=c.length;else this.Ma--}if(b.length>0){this.Cc.J(a,b);this.Ma+=b.length}};
Ui.prototype.toString=function(){if(this.ie)return this.ie;var a=[],b=0,c=this.Cc.tc();for(var d=0;d<c.length;d++){var e=c[d],f=ab(e),g=this.Cc.get(e);if(xa(g))for(var h=0;h<g.length;h++){if(b>0)a.push("&");a.push(f,"=",ab(g[h]));b++}else{if(b>0)a.push("&");a.push(f,"=",ab(g));b++}}return this.ie=a.join("")};
Ui.prototype.Wn=function(){delete this.ie;if(this.ea)delete this.ea.ie};
Ui.prototype.ra=function(){var a=new Ui;a.Cc=this.Cc.ra();return a};
Ui.prototype.Jj=function(a){var b=String(a);if(this.zh)b=b.toLowerCase();return b};
Ui.prototype.xm=function(a){var b=a&&!this.zh;if(b){this.Wn();ad(this.Cc,function(c,d){var e=d.toLowerCase();if(d!=e){this.remove(d);this.add(e,c)}},
this)}this.zh=a};
Ui.prototype.extend=function(){for(var a=0;a<arguments.length;a++){var b=arguments[a];ad(b,function(c,d){this.add(d,c)},
this)}};var ej=function(a,b){this.ea=new N(a);this.oba=b?b:"callback";this.Dk=5000},
fj="_callbacks_",gj=0;ej.prototype.c9=function(a){this.Dk=a};
ej.prototype.send=function(a,b,c){if(!document.documentElement.firstChild){if(c)c(a);return null}var d="_"+(gj++).toString(36)+Ka().toString(36);if(!j[fj])j[fj]={};var e=w("script"),f=null;if(this.Dk>0){var g=hj(d,e,a,c);f=j.setTimeout(g,this.Dk)}var h=this.ea.ra();ij(a,h);if(b){var i=jj(d,e,b,f);j[fj][d]=i;h.VD(this.oba,fj+"."+d)}Pe(e,{type:"text/javascript",id:d,charset:"UTF-8",src:h.toString()});x(document.documentElement.firstChild,e);return{Za:d,Dk:f}};
ej.prototype.cf=function(a){if(a&&a.Za){var b=u(a.Za);if(b&&b.tagName=="SCRIPT"&&typeof j[fj][a.Za]=="function"){a.Dk&&j.clearTimeout(a.Dk);A(b);delete j[fj][a.Za]}}};
var hj=function(a,b,c,d){return function(){kj(a,b);if(d)d(c)}},
jj=function(a,b,c,d){return function(e){j.clearTimeout(d);kj(a,b);c(e)}},
kj=function(a,b){j.setTimeout(function(){A(b);if(j[fj][a])delete j[fj][a]},
0)},
ij=function(a,b){for(var c in a)if(!a.hasOwnProperty||a.hasOwnProperty(c))b.VD(c,a[c]);return b};var mj=function(){return lj()};
var lj=null,nj=null,oj=null,pj=function(a,b){lj=a;nj=b;oj=null},
rj=function(){var a=qj();return a?new ActiveXObject(a):new XMLHttpRequest},
sj=function(){var a=qj(),b={};if(a){b[0]=true;b[1]=true}return b};
pj(rj,sj);var tj=null,qj=function(){if(!tj&&typeof XMLHttpRequest=="undefined"&&typeof ActiveXObject!="undefined"){var a=["MSXML2.XMLHTTP.6.0","MSXML2.XMLHTTP.3.0","MSXML2.XMLHTTP","Microsoft.XMLHTTP"];for(var b=0;b<a.length;b++){var c=a[b];try{new ActiveXObject(c);tj=c;return c}catch(d){}}throw Error("Could not create ActiveXObject. ActiveX might be disabled, or MSXML might not be installed");}return tj};var uj=function(){if(!se)return;this.Xm={};this.iF={};this.jE=[]};
uj.prototype.Ba=Fd("goog.net.xhrMonitor");uj.prototype.LN=function(a){if(!se)return;var b=za(a)?a:(Da(a)?Ga(a):"");this.Ba.kf("Pushing context: "+a+" ("+b+")");this.jE.push(b)};
uj.prototype.xN=function(){if(!se)return;var a=this.jE.pop();this.Ba.kf("Popping context: "+a);this.jaa(a)};
uj.prototype.K3=function(a){if(!se)return;var b=Ga(a);this.Ba.Zb("Opening XHR : "+b);for(var c=0;c<this.jE.length;c++){var d=this.jE[c];this.Rs(this.Xm,d,b);this.Rs(this.iF,b,d)}};
uj.prototype.J3=function(a){if(!se)return;var b=Ga(a);this.Ba.Zb("Closing XHR : "+b);delete this.iF[b];for(var c in this.Xm){tc(this.Xm[c],b);if(this.Xm[c].length==0)delete this.Xm[c]}};
uj.prototype.jaa=function(a){var b=this.iF[a],c=this.Xm[a];if(b&&c){this.Ba.kf("Updating dependent contexts");r(b,function(d){r(c,function(e){this.Rs(this.Xm,d,e);this.Rs(this.iF,e,d)},
this)},
this)}};
uj.prototype.Rs=function(a,b,c){if(!a[b])a[b]=[];if(!mc(a[b],c))a[b].push(c)};
var vj=new uj;var wj=function(a){if(/^\s*$/.test(a))return false;var b=/\\["\\\/bfnrtu]/g,c=/"[^"\\\n\r\u2028\u2029\x00-\x1f\x7f-\x9f]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?/g,d=/(?:^|:|,)(?:[\s\u2028\u2029]*\[)+/g,e=/^[\],:{}\s\u2028\u2029]*$/;return e.test(a.replace(b,"@").replace(c,"]").replace(d,""))},
xj=function(a){a=String(a);if(typeof a.parseJSON=="function")return a.parseJSON();if(wj(a))try{return eval("("+a+")")}catch(b){}throw Error("Invalid JSON string: "+a);},
yj=function(a){return eval("("+a+")")},
zj=null,Bj=function(a){if(!zj)zj=new Aj;return zj.N7(a)},
Aj=function(){};
Aj.prototype.N7=function(a){if(a!=null&&typeof a.toJSONString=="function")return a.toJSONString();var b=[];this.DD(a,b);return b.join("")};
Aj.prototype.DD=function(a,b){switch(typeof a){case "string":this.FO(a,b);break;case "number":this.P7(a,b);break;case "boolean":b.push(a);break;case "undefined":b.push("null");break;case "object":if(a==null){b.push("null");break}if(xa(a)){this.O7(a,b);break}this.Q7(a,b);break;default:throw Error("Unknown type: "+typeof a);}};
var Cj={'"':'\\"',"\\":"\\\\","/":"\\/","\u0008":"\\b","\u000c":"\\f","\n":"\\n","\r":"\\r","\t":"\\t","\u000b":"\\u000b"};Aj.prototype.FO=function(a,b){b.push('"',a.replace(/[\\\"\x00-\x1f\x80-\uffff]/g,function(c){if(c in Cj)return Cj[c];var d=c.charCodeAt(0),e="\\u";if(d<16)e+="000";else if(d<256)e+="00";else if(d<4096)e+="0";return Cj[c]=e+d.toString(16)}),
'"')};
Aj.prototype.P7=function(a,b){b.push(isFinite(a)&&!isNaN(a)?a:"null")};
Aj.prototype.O7=function(a,b){var c=a.length;b.push("[");var d="";for(var e=0;e<c;e++){b.push(d);this.DD(a[e],b);d=","}b.push("]")};
Aj.prototype.Q7=function(a,b){b.push("{");var c="";for(var d in a){b.push(c);this.FO(d,b);b.push(":");this.DD(a[d],b);c=","}b.push("}")};var Dj=function(){J.call(this);this.headers=new dd};
o(Dj,J);Dj.prototype.Ba=Fd("goog.net.XhrLite");var Ej=[],Gj=function(a,b,c,d,e,f){var g=new Dj;Ej.push(g);if(b)F(g,Ki,b);F(g,"ready",Ia(Fj,g));if(f)g.Yr(f);g.send(a,c,d,e)},
Fj=function(a){a.b();tc(Ej,a)};
Dj.prototype.qg=false;Dj.prototype.Ta=null;Dj.prototype.oy=null;Dj.prototype.UB="";Dj.prototype.QL="";Dj.prototype.Zn=0;Dj.prototype.Oq="";Dj.prototype.gA=false;Dj.prototype.yB=false;Dj.prototype.Ck=0;Dj.prototype.Fe=null;Dj.prototype.Yr=function(a){this.Ck=Math.max(0,a)};
Dj.prototype.send=function(a,b,c,d){if(this.qg)throw Error("[goog.net.XhrLite] Object is active with another request");var e=b||"GET";this.UB=a;this.Oq="";this.Zn=0;this.QL=e;this.qg=true;this.Ta=new mj;this.oy=oj||(oj=nj());vj.K3(this.Ta);this.Ta.onreadystatechange=l(this.xr,this);try{this.V("Opening Xhr");this.Ta.open(e,a,true)}catch(f){this.V("Error opening Xhr: "+f.message);this.xn(5,f);return}var g=c?String(c):"",h=this.headers.ra();if(d)ad(d,function(i,k){h.J(k,i)});
if(e=="POST"&&!h.Ob("Content-Type"))h.J("Content-Type","application/x-www-form-urlencoded;charset=utf-8");ad(h,function(i,k){this.Ta.setRequestHeader(k,i)},
this);try{if(this.Fe){Og.clearTimeout(this.Fe);this.Fe=null}if(this.Ck>0){this.V("Will abort after "+this.Ck+"ms if incomplete");this.Fe=Og.setTimeout(l(this.Dk,this),this.Ck)}this.V("Sending request");this.gA=false;this.yB=true;this.Ta.send(g);this.yB=false}catch(f){this.V("Send error: "+f.message);this.xn(5,f)}};
Dj.prototype.dispatchEvent=function(a){if(this.Ta){vj.LN(this.Ta);try{Dj.f.dispatchEvent.call(this,a)}finally{vj.xN()}}else Dj.f.dispatchEvent.call(this,a)};
Dj.prototype.Dk=function(){if(typeof oa=="undefined"){}else if(this.Ta){this.Oq="Timed out after "+this.Ck+"ms, aborting";this.Zn=8;this.V(this.Oq);this.dispatchEvent(Ni);this.abort(8)}};
Dj.prototype.xn=function(a,b){this.qg=false;if(this.Ta)this.Ta.abort();this.Oq=b;this.Zn=a;this.SH();this.ft()};
Dj.prototype.SH=function(){if(!this.gA){this.gA=true;this.dispatchEvent(Ki);this.dispatchEvent(Mi)}};
Dj.prototype.abort=function(a){if(this.Ta){this.V("Aborting");this.qg=false;this.Ta.abort();this.Zn=a||7;this.dispatchEvent(Ki);this.dispatchEvent("abort");this.ft()}};
Dj.prototype.b=function(){if(!this.W()){if(this.Ta){this.qg=false;this.Ta.abort();this.ft(true)}Dj.f.b.call(this)}};
Dj.prototype.xr=function(){if(!this.qg)return;if(typeof oa=="undefined"){}else if(this.oy[1]&&this.oq()==4&&this.nf()==2)this.V("Local request error detected and ignored");else{if(this.yB&&this.oq()==4){Rg(this.xr,0,this);return}this.dispatchEvent("readystatechange");if(this.Kq()){this.V("Request complete");this.qg=false;if(this.Re()){this.dispatchEvent(Ki);this.dispatchEvent(Li)}else{this.Zn=6;this.Oq=this.cK()+" ["+this.nf()+"]";this.SH()}this.ft()}}};
Dj.prototype.ft=function(a){if(this.Ta){this.Ta.onreadystatechange=this.oy[0]?sa:null;var b=this.Ta;this.Ta=null;this.oy=null;if(this.Fe){Og.clearTimeout(this.Fe);this.Fe=null}if(!a){vj.LN(b);this.dispatchEvent("ready");vj.xN()}vj.J3(b)}};
Dj.prototype.zd=function(){return this.qg};
Dj.prototype.Kq=function(){return this.oq()==4};
Dj.prototype.Re=function(){switch(this.nf()){case 0:case 200:case 204:case 304:return true;default:return false}};
Dj.prototype.oq=function(){return this.Ta?this.Ta.readyState:0};
Dj.prototype.nf=function(){try{return this.oq()>2?this.Ta.status:-1}catch(a){this.Ba.warning("Can not get status: "+a.message);return-1}};
Dj.prototype.cK=function(){try{return this.oq()>2?this.Ta.statusText:""}catch(a){this.Ba.Zb("Can not get status: "+a.message);return""}};
Dj.prototype.QY=function(){return this.UB};
Dj.prototype.Sb=function(){return this.Ta?this.Ta.responseText:""};
Dj.prototype.VJ=function(){return this.Ta?this.Ta.responseXML:null};
Dj.prototype.Uf=function(){return this.Ta?xj(this.Ta.responseText):undefined};
Dj.prototype.getResponseHeader=function(a){return this.Ta&&this.Kq()?this.Ta.getResponseHeader(a):undefined};
Dj.prototype.PY=function(){return this.Zn};
Dj.prototype.V=function(a){this.Ba.Zb(a+" ["+this.QL+" "+this.UB+" "+this.nf()+"]")};var Hj={},Ij={"1":"NativeMessagingTransport","2":"FrameElementMethodTransport","3":"IframeRelayTransport","4":"IframePollingTransport","5":"FlashTransport"},Jj="SETUP_ACK",Kj={},Mj=function(a,b){var c=b||Lj,d=c.length,e="";while(a-- >0)e+=c.charAt(Math.floor(Math.random()*d));return e},
Lj="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",Nj=Fd("goog.net.xpc");var Oj=function(){};
o(Oj,Vd);Oj.prototype.Vh=0;Oj.prototype.Xa=function(){return this.Vh};
Oj.prototype.getName=function(){return Ij[this.Vh]||""};
Oj.prototype.Im=function(){throw Error("Not implemented");};
Oj.prototype.sg=function(){throw Error("Not implemented");};
Oj.prototype.send=function(){throw Error("Not implemented");};var Pj=function(a){this.La=a};
o(Pj,Oj);Pj.prototype.Vh=1;var Qj=function(){if(!this.Wd){F(document,"message",this.dda,false,this);this.Wd=true}};
Pj.prototype.Im=function(a){switch(a){case "SETUP":this.send("tp",Jj);case Jj:this.La.oo();break}};
Pj.prototype.sg=function(){Qj();this.send("tp","SETUP")};
Pj.prototype.send=function(a,b){this.La.Ih.document.postMessage(this.La.name+"|"+a+":"+b)};
Pj.prototype.b=function(){if(this.W())return;Vd.prototype.b.call(this);G(document,"message",this.dda,false,this)};var Rj=function(a){this.La=a;this.Jw=[];this.Oba=l(this.bW,this)};
o(Rj,Oj);Rj.prototype.Vh=2;Rj.prototype.wy=0;Rj.prototype.NR=1;Rj.prototype.cD=false;Rj.prototype.ab=0;Rj.prototype.sg=function(){this.b7=window.parent==this.La.Ih?this.NR:this.wy;if(this.b7==this.wy){this.$j=this.La.Gq;this.$j.XPC_toOuter=l(this.nL,this)}else this.fG()};
Rj.prototype.fG=function(){var a=true;try{if(!this.$j)this.$j=window.frameElement;if(this.$j&&this.$j.XPC_toOuter){this.cN=this.$j.XPC_toOuter;this.$j.XPC_toOuter.XPC_toInner=l(this.nL,this);a=false;this.send("tp",Jj);this.La.oo()}}catch(b){Nj.Zr("exception caught while attempting setup: "+b)}if(a){if(!this.rT)this.rT=l(this.fG,this);window.setTimeout(this.rT,100)}};
Rj.prototype.Im=function(a){if(this.b7==this.wy&&!this.La.bk()&&a==Jj){this.cN=this.$j.XPC_toOuter.XPC_toInner;this.La.oo()}else throw Error("Got unexpected transport message.");};
Rj.prototype.nL=function(a,b){if(!this.cD&&this.Jw.length==0)this.La.Kp(a,b);else{this.Jw.push({serviceName:a,payload:b});if(this.Jw.length==1)this.ab=window.setTimeout(this.Oba,1)}};
Rj.prototype.bW=function(){while(this.Jw.length){var a=this.Jw.shift();this.La.Kp(a.serviceName,a.payload)}};
Rj.prototype.send=function(a,b){this.cD=true;this.cN(a,b);this.cD=false};
Rj.prototype.b=function(){if(this.W())return;Vd.prototype.b.call(this);this.cN=null;this.$j=null};var Tj=function(a){this.La=a;this.zda=this.La.Md.pru;this.o5=this.La.Md.ifrid;if($b)Sj()};
o(Tj,Oj);if($b){var Uj=[],Sj=function(){if(!this.mU)this.mU=window.setTimeout(function(){Vj()},
this.Raa)},
Vj=function(a){var b=Ka(),c=a||this.Tea;while(this.Z1.length&&b-this.Z1[0].timestamp>=c){var d=this.Z1.shift().iframeElement;A(d);Nj.kf("iframe removed")}this.mU=window.setTimeout(this.dfa,this.Raa)}}Tj.prototype.Vh=3;
Tj.prototype.sg=function(){this.send("tp","SETUP")};
Tj.prototype.Im=function(a){if(a=="SETUP"){this.send("tp",Jj);this.La.oo()}else if(a==Jj)this.La.oo()};
Tj.prototype.send=function(a,b){if(s){var c=document.createElement("div");c.innerHTML='<iframe onload="this.xpcOnload()"></iframe>';var d=c.childNodes[0];c=null;d.xpcOnload=Wj}else{var d=document.createElement("iframe");if($b)Uj.push({timestamp:Ka(),iframeElement:d});else F(d,"load",Wj)}var e=d.style;e.visibility="hidden";e.width=(d.style.height="0px");e.position="absolute";var f=this.zda;f+="#"+this.La.name;if(this.o5)f+=","+this.o5;f+="|"+a+":"+encodeURIComponent(b);d.src=f;document.body.appendChild(d);
Nj.kf("msg sent: "+f)};
var Wj=function(){Nj.kf("iframe-load");A(this);this.xpcOnload=null};
window.xpcRelay=function(a,b){var c=b.indexOf(":"),d=b.substring(0,c),e=b.substring(c+1);Kj[a].Kp(d,decodeURIComponent(e))};
Tj.prototype.b=function(){if(this.W())return;Vd.prototype.b.call(this);if($b)Vj(0)};var Xj=function(a){this.La=a;this.dx=this.La.Md.ppu;this.Lda=this.La.Md.lpu;this.CD=[]},
Yj,Zj;o(Xj,Oj);Xj.prototype.Vh=4;Xj.prototype.ex=0;Xj.prototype.ep=false;Xj.prototype.sg=function(){this.EU()};
Xj.prototype.EU=function(){Nj.Zb("constructing sender frames.");var a=this.La.name+"_msg";this.s4=this.bH(a);this.v4=window.frames[a];a=this.La.name+"_ack";this.wS=this.bH(a);this.yS=window.frames[a];this.EG()};
Xj.prototype.bH=function(a){var b=w("iframe"),c=b.style;c.position="absolute";c.top="-10px";c.left="10px";c.width="1px";c.height="1px";b.id=(b.name=a);b.src=this.dx+"#INITIAL";document.body.appendChild(b);return b};
Xj.prototype.EG=function(){if(this.iU)window.clearTimeout(this.iU);if(!(this.CL("msg")&&this.CL("ack"))){Nj.kf("foreign frames not (yet) present");if(!this.hU)this.hU=l(this.EG,this);this.iU=window.setTimeout(this.hU,100)}else{Nj.Zb("foreign frames present");this.t4=new $j(this,this.La.Ih.frames[this.La.name+"_msg"],l(this.W5,this));this.xS=new $j(this,this.La.Ih.frames[this.La.name+"_ack"],l(this.V5,this));this.HG()}};
Xj.prototype.CL=function(a){try{var b=this.La.Ih.frames[this.La.name+"_"+a];if(!b||b.location.href.indexOf(this.Lda)!=0)return false}catch(c){return false}return true};
Xj.prototype.HG=function(){var a=this.La.Ih.frames;if(!(a[this.La.name+"_ack"]&&a[this.La.name+"_msg"])){if(!this.kU)this.kU=l(this.HG,this);window.setTimeout(this.kU,100);Nj.Zb("local frames not (yet) present")}else{this.u4=new ak(this.dx,this.v4);this.Dy=new ak(this.dx,this.yS);Nj.Zb("local frames ready");window.setTimeout(l(function(){this.u4.send("SETUP");this.Mfa=true;this.ep=true;Nj.Zb("SETUP sent")},
this),100)}};
Xj.prototype.FG=function(){if(this.EO&&this.d6){this.La.oo();if(this.Lp){Nj.Zb("delivering queued messages ("+this.Lp.length+")");for(var a=0,b;a<this.Lp.length;a++){b=this.Lp[a];this.La.Kp(b.service,b.payload)}delete this.Lp}}else Nj.kf("checking if connected: ack sent:"+this.EO+", ack rcvd: "+this.d6)};
Xj.prototype.W5=function(a){Nj.kf("msg received: "+a);if(a=="SETUP"){if(!this.Dy)return;this.Dy.send(Jj);Nj.kf("SETUP_ACK sent");this.EO=true;this.FG()}else if(this.La.bk()||this.EO){var b=a.indexOf("|"),c=a.substring(0,b),d=a.substring(b+1);b=c.indexOf(",");if(b==-1){var e=c;this.Dy.send("ACK:"+e);this.LH(d)}else{var e=c.substring(0,b);this.Dy.send("ACK:"+e);var f=c.substring(b+1).split("/");f[0]=parseInt(f[0],10);f[1]=parseInt(f[1],10);if(f[0]==1)this.Tb=[];this.Tb.push(d);if(f[0]==f[1]){this.LH(this.Tb.join(""));
delete this.Tb}}}else Nj.warning("received msg, but channel is not connected")};
Xj.prototype.V5=function(a){Nj.kf("ack received: "+a);if(a==Jj){this.ep=false;this.d6=true;this.FG()}else if(this.La.bk()){if(!this.ep){Hj.warning("got unexpected ack");return}var b=parseInt(a.split(":")[1],10);if(b==this.ex){this.ep=false;this.DO()}else Nj.warning("got ack with wrong sequence")}else Nj.warning("received ack, but channel not connected")};
Xj.prototype.DO=function(){if(this.ep||!this.CD.length)return;var a=this.CD.shift();++this.ex;this.u4.send(this.ex+a);Nj.kf("msg sent: "+this.ex+a);this.ep=true};
Xj.prototype.LH=function(a){var b=a.indexOf(":"),c=a.substr(0,b),d=a.substring(b+1);if(!this.La.bk()){(this.Lp||(this.Lp=[])).push({service:c,payload:d});Nj.kf("queued delivery")}else this.La.Kp(c,d)};
Xj.prototype.Fs=3800;Xj.prototype.send=function(a,b){var c=a+":"+b;if(!s||b.length<=this.Fs)this.CD.push("|"+c);else{var d=b.length,e=Math.ceil(d/this.Fs),f=0,g=1;while(f<d){this.CD.push(","+g+"/"+e+"|"+c.substr(f,this.Fs));g++;f+=this.Fs}}this.DO()};
Xj.prototype.b=function(){if(this.W())return;Vd.prototype.b.call(this);var a=bk;tc(a,this.t4);tc(a,this.xS);this.t4=(this.xS=null);A(this.s4);A(this.wS);this.s4=(this.wS=null);this.v4=(this.yS=null)};
var bk=[],ck=function(){var a=false;try{for(var b=0,c=this.PN.length;b<c;b++)a=a||this.PN[b].i6()}catch(d){Nj.info("receive_() failed: "+d);this.PN[b].qd.La.D4();if(!this.PN.length)return}var e=Ka();if(a)Yj=e;var f=e-Yj<this.Vea?this.Taa:this.Uea;this.MN=window.setTimeout(this.Mda,f)};
l(ck,Xj);var dk=function(){Nj.Zb("starting receive-timer");Yj=Ka();if(this.MN)window.clearTimeout(this.MN);this.MN=window.setTimeout(this.Mda,this.Taa)},
ak=function(a,b){this.dx=a;this.I7=b;this.CH=0};
ak.prototype.send=function(a){this.CH=++this.CH%2;var b=this.dx+"#"+this.CH+encodeURIComponent(a);try{if($b)this.I7.location.href=b;else this.I7.location.replace(b)}catch(c){Nj.Zr("sending failed",c)}Zj=Ka()};
var $j=function(a,b,c){this.qd=a;this.c6=b;this.sba=c;this.IV=this.c6.location.href.split("#")[0]+"#INITIAL";bk.push(this);dk()};
$j.prototype.i6=function(){var a=this.c6.location.href;if(a!=this.IV){this.IV=a;var b=a.split("#")[1];if(b){b=b.substr(1);this.sba(decodeURIComponent(b))}return true}else return false};var fk=function(a){this.Md=a;this.name=this.Md.cn||Mj(10);this.S7={};Kj[this.name]=this;F(window,"unload",ek);Nj.info("CrossPageChannel created: "+this.name)};
o(fk,Vd);fk.prototype.qd=null;fk.prototype.Bb=1;fk.prototype.bk=function(){return this.Bb==2};
fk.prototype.Ih=null;fk.prototype.Gq=null;fk.prototype.pP=function(a){this.Ih=a};
fk.prototype.DV=function(){if(this.qd)return;if(!this.Md.tp)if(Ca(document.postMessage))this.Md.tp=1;else if(se)this.Md.tp=2;else if(s&&this.Md.pru)this.Md.tp=3;else if(this.Md.lpu&&this.Md.ppu)this.Md.tp=4;switch(this.Md.tp){case 1:this.qd=new Pj(this);break;case 2:this.qd=new Rj(this);break;case 3:this.qd=new Tj(this);break;case 4:this.qd=new Xj(this);break}if(this.qd)Nj.info("Transport created: "+this.qd.getName());else throw Error("CrossPageChannel: No suitable transport found!");};
fk.prototype.SV=false;fk.prototype.DU=false;fk.prototype.sg=function(a){this.uba=a;if(this.SV){this.DU=true;return}if(this.Md.ifrid)this.Gq=u(this.Md.ifrid);if(this.Gq){var b=this.Gq.contentWindow;if(!b)b=window.frames[this.Md.ifrid];this.pP(b)}if(!this.Ih)if(window==top)throw Error("CrossPageChannel: Can't connect, peer window-object not set.");else this.pP(window.parent);this.DV();this.qd.sg()};
fk.prototype.close=function(){if(!this.bk())return;this.Bb=3;this.qd.b();this.qd=null;Nj.info('Channel "'+this.name+'" closed')};
fk.prototype.oo=function(){if(this.bk())return;this.Bb=2;Nj.info('Channel "'+this.name+'" connected');this.uba()};
fk.prototype.D4=function(){Nj.info("Transport Error");this.close()};
fk.prototype.send=function(a,b){if(!this.bk()){Nj.Zr("Can't send. Channel not connected.");return}if(this.Ih.closed){Nj.Zr("Peer has disappeared.");this.close();return}if(Da(b))b=Bj(b);this.qd.send(a,b)};
fk.prototype.Kp=function(a,b){if(!a||a=="tp")this.qd.Im(b);else if(this.bk()){var c=this.S7[a];if(c){if(c.jsonEncoded)try{b=xj(b)}catch(d){Nj.info("Error parsing JSON-encoded payload.");return}c.callback(b)}else Nj.info('CrossPageChannel::deliver_(): No such service: "'+a+'" (payload: '+b+")")}else Nj.info("CrossPageChannel::deliver_(): Not connected.")};
fk.prototype.b=function(){if(this.W())return;Vd.prototype.b.call(this);this.close();this.Ih=null;this.Gq=null;this.S7=null;Kj[this.name]=null};
var ek=function(){for(var a in Kj){var b=Kj[a];if(b)b.b()}};var gk=function(a,b){this.te=a;this.Ua=b};
gk.prototype.Ij=function(){return this.te};
gk.prototype.L=function(){return this.Ua};
gk.prototype.ra=function(){return new gk(this.te,this.Ua)};var hk=function(){this.Ie=[]};
hk.prototype.Ya=0;hk.prototype.Ak=0;hk.prototype.Tp=function(a){this.Ie[this.Ak++]=a};
hk.prototype.cl=function(){if(this.Ya==this.Ak)return undefined;var a=this.Ie[this.Ya];delete this.Ie[this.Ya];this.Ya++;return a};
hk.prototype.peek=function(){if(this.Ya==this.Ak)return undefined;return this.Ie[this.Ya]};
hk.prototype.S=function(){return this.Ak-this.Ya};
hk.prototype.isEmpty=function(){return this.Ak-this.Ya==0};
hk.prototype.clear=function(){this.Ie=[];this.Ya=0;this.Ak=0};
hk.prototype.contains=function(){return Yc(this.Ie)};
hk.prototype.remove=function(a){var b=cc(this.Ie,a);if(b<0)return false;if(b==this.Ya)this.cl();else{sc(this.Ie,b);this.Ak--}return true};
hk.prototype.xb=function(){var a=[];for(var b=this.Ya;b<this.Ak;b++)a.push(this.Ie[b]);return a};var ik=function(a,b){Vd.call(this);this.k4=a||0;this.Ii=b||10;if(this.k4>this.Ii)throw Error("[goog.structs.Pool] Min can not be greater than max");this.Rd=new hk;this.Ci=new hd;this.qp()};
o(ik,Vd);ik.prototype.Sd=function(){var a=this.t6();if(a)this.Ci.add(a);return a};
ik.prototype.Ae=function(a){if(this.Ci.remove(a)){this.Ls(a);return true}return false};
ik.prototype.t6=function(){var a;while(this.pJ()>0){a=this.Rd.cl();if(!this.jw(a))this.qp();else break}if(!a&&this.S()<this.Ii)a=this.ug();return a};
ik.prototype.Ls=function(a){this.Ci.remove(a);if(this.jw(a)&&this.S()<this.Ii)this.Rd.Tp(a);else this.xg(a)};
ik.prototype.qp=function(){var a=this.Rd;while(this.S()<this.k4)a.Tp(this.ug());while(this.S()>this.Ii&&this.pJ()>0)this.xg(a.cl())};
ik.prototype.ug=function(){return{}};
ik.prototype.xg=function(a){if(typeof a.b=="function")a.b();else for(var b in a)a[b]=null};
ik.prototype.jw=function(a){if(typeof a.canBeReused=="function")return a.canBeReused();return true};
ik.prototype.contains=function(a){return this.Rd.contains(a)||this.Ci.contains(a)};
ik.prototype.S=function(){return this.Rd.S()+this.Ci.S()};
ik.prototype.LY=function(){return this.Ci.S()};
ik.prototype.pJ=function(){return this.Rd.S()};
ik.prototype.isEmpty=function(){return this.Rd.isEmpty()&&this.Ci.isEmpty()};
ik.prototype.b=function(){if(!this.W()){ik.f.b.call(this);if(this.LY()>0)throw Error("[goog.structs.Pool] Objects not released");Ec(this.Ci,this.xg,this);this.Ci.clear();this.Ci=null;var a=this.Rd;while(!a.isEmpty())this.xg(a.cl());this.Rd=null}};var jk=function(a){this.eg=[];if(a)this.v2(a)};
jk.prototype.Bh=function(a,b){var c=new gk(a,b),d=this.eg;d.push(c);this.bw(d.length-1)};
jk.prototype.v2=function(a){var b,c;if(a instanceof jk){b=a.tc();c=a.xb();if(a.S()<=0){var d=this.eg;for(var e=0;e<b.length;e++)d.push(new gk(b[e],c[e]));return}}else{b=Jc(a);c=Ic(a)}for(var e=0;e<b.length;e++)this.Bh(b[e],c[e])};
jk.prototype.remove=function(){var a=this.eg,b=a.length,c=a[0];if(b<=0)return undefined;else if(b==1)$c(a);else{a[0]=a.pop();this.$v(0)}return c.L()};
jk.prototype.peek=function(){var a=this.eg;if(a.length==0)return undefined;return a[0].L()};
jk.prototype.$v=function(a){var b=this.eg,c=b.length,d=b[a];while(a<Math.floor(c/2)){var e=this.RY(a),f=this.tZ(a),g=f<c&&b[f].Ij()<b[e].Ij()?f:e;if(b[g].Ij()>d.Ij())break;b[a]=b[g];a=g}b[a]=d};
jk.prototype.bw=function(a){var b=this.eg,c=b[a];while(a>0){var d=this.iZ(a);if(b[d].Ij()>c.Ij()){b[a]=b[d];a=d}else break}b[a]=c};
jk.prototype.RY=function(a){return a*2+1};
jk.prototype.tZ=function(a){return a*2+2};
jk.prototype.iZ=function(a){return Math.floor((a-1)/2)};
jk.prototype.xb=function(){var a=this.eg,b=[],c=a.length;for(var d=0;d<c;d++)b.push(a[d].L());return b};
jk.prototype.tc=function(){var a=this.eg,b=[],c=a.length;for(var d=0;d<c;d++)b.push(a[d].Ij());return b};
jk.prototype.gh=function(a){return bd(this.eg,function(b){return b.L()==a})};
jk.prototype.Ob=function(a){return bd(this.eg,function(b){return b.Ij()==a})};
jk.prototype.ra=function(){return new jk(this)};
jk.prototype.S=function(){return Vc(this.eg)};
jk.prototype.isEmpty=function(){return Zc(this.eg)};
jk.prototype.clear=function(){$c(this.eg)};var lk=function(a,b){this.Ii=a||0;this.fz=!(!b);this.o=new dd;this.Ya=new kk(null);this.Ya.next=(this.Ya.prev=this.Ya)};
lk.prototype.get=function(a,b){var c=this.o.get(a);if(c){if(this.fz){c.remove();this.FB(c)}return c.value}return b};
lk.prototype.J=function(a,b){var c=this.o.get(a);if(c){c.value=b;if(this.fz){c.remove();this.FB(c)}}else{c=new kk(a,b);this.o.J(a,c);this.FB(c)}};
lk.prototype.peek=function(){return this.Ya.next.value};
lk.prototype.remove=function(a){var b=this.o.get(a);if(b){b.remove();this.o.remove(a);return true}return false};
lk.prototype.S=function(){return this.o.S()};
lk.prototype.isEmpty=function(){return this.o.isEmpty()};
lk.prototype.tc=function(){return this.map(function(a,b){return b})};
lk.prototype.xb=function(){return this.map(function(a){return a})};
lk.prototype.contains=function(a){return ic(this.Se,function(b){return b.L()==a})};
lk.prototype.Ob=function(a){return this.o.Ob(a)};
lk.prototype.clear=function(){this.o.clear();this.Ya.next=(this.Ya.prev=this.Ya)};
lk.prototype.forEach=function(a,b){for(var c=this.Ya.next;c!=this.Ya;c=c.next)a.call(b,c.value,c.key,this)};
lk.prototype.map=function(a,b){var c=[];for(var d=this.Ya.next;d!=this.Ya;d=d.next)c.push(a.call(b,d.value,d.key,this));return c};
lk.prototype.some=function(a,b){for(var c=this.Ya.next;c!=this.Ya;c=c.next)if(a.call(b,c.value,c.key,this))return true;return false};
lk.prototype.every=function(a,b){for(var c=this.Ya.next;c!=this.Ya;c=c.next)if(!a.call(b,c.value,c.key,this))return false;return true};
lk.prototype.FB=function(a){if(this.fz){a.next=this.Ya.next;a.prev=this.Ya;this.Ya.next=a;a.next.prev=a}else{a.prev=this.Ya.prev;a.next=this.Ya;this.Ya.prev=a;a.prev.next=a}this.W$()};
lk.prototype.W$=function(){if(this.Ii)for(var a=this.o.S();a>this.Ii;a--){var b=this.fz?this.Ya.prev:this.Ya.next;b.remove();this.o.remove(b.key)}};
var kk=function(a,b){this.key=a;this.value=b};
kk.prototype.remove=function(){this.prev.next=this.next;this.next.prev=this.prev;this.prev=(this.next=null)};var mk=function(){jk.call(this)};
o(mk,jk);mk.prototype.Tp=function(a,b){this.Bh(a,b)};
mk.prototype.cl=function(){return this.remove()};var nk=function(a,b){this.mD=new mk;ik.call(this,a,b)};
o(nk,ik);nk.prototype.Sd=function(a,b){if(!a)return nk.f.Sd.call(this);var c=b||100;this.mD.Tp(c,a);this.iB();return undefined};
nk.prototype.iB=function(){var a=this.mD;while(a.S()>0){var b=this.Sd();if(!b)return;else{var c=a.cl();c.apply(this,[b])}}};
nk.prototype.Ls=function(a){nk.f.Ls.call(this,a);this.iB()};
nk.prototype.qp=function(){nk.f.qp.call(this);this.iB()};
nk.prototype.b=function(){if(!this.W()){nk.f.b.call(this);this.mD.clear();this.mD=null}};var ok=function(a,b,c){nk.call(this,b,c);this.VK=a};
o(ok,nk);ok.prototype.ug=function(){var a=new Dj,b=this.VK;if(b)ad(b,function(c,d){a.headers.J(d,c)});
return a};
ok.prototype.xg=function(a){a.b()};
ok.prototype.jw=function(a){return!a.W()&&!a.zd()};var pk=function(a,b,c,d,e){this.rM=pa(a)?a:1;this.Ck=pa(e)?Math.max(0,e):0;this.As=new ok(b,c,d);this.Be=new dd;this.p=new K(this)};
o(pk,J);var qk=["ready",Ki,Li,Mi,"abort",Ni];pk.prototype.Yr=function(a){this.Ck=Math.max(0,a)};
pk.prototype.send=function(a,b,c,d,e,f,g){var h=this.Be;if(h.get(a))throw Error("[goog.net.XhrManager] ID in use");var i=new rk(b,l(this.yq,this,a),c,d,e,g);this.Be.J(a,i);var k=l(this.d_,this,a);this.As.Sd(k,f)};
pk.prototype.abort=function(a,b){var c=this.Be.get(a);if(c){var d=c.xhrLite;c.T7(true);if(b){this.YN(d,c.eB());$f(d,"ready",function(){this.As.Ae(d)},
false,this);this.Be.remove(a)}if(d)d.abort()}};
pk.prototype.d_=function(a,b){var c=this.Be.get(a);if(c&&!c.xhrLite){this.YS(b,c.eB());b.Yr(this.Ck);c.xhrLite=b;this.dispatchEvent(new sk("ready",this,a,b));this.mO(a,b);if(c.QX())b.abort()}else this.As.Ae(b)};
pk.prototype.yq=function(a,b){switch(b.type){case "ready":this.mO(a,b.target);break;case Ki:return this.Fu(a,b.target,b);case Li:this.g1(a,b.target);break;case Ni:case Mi:this.wi(a,b.target);break;case "abort":this.WZ(a,b.target);break}return null};
pk.prototype.mO=function(a,b){var c=this.Be.get(a);if(c&&!c.jY()&&c.xA()<=this.rM){c.e2();b.send(c.uq(),c.YY(),c.Vc(),c.GY())}else{if(c){this.YN(b,c.eB());this.Be.remove(a)}this.As.Ae(b)}};
pk.prototype.Fu=function(a,b,c){var d=this.Be.get(a);if(b.PY()==7||b.Re()||d.xA()>this.rM){this.dispatchEvent(new sk(Ki,this,a,b));if(d){d.e8(true);if(d.NI())return d.NI().call(b,c)}}return null};
pk.prototype.WZ=function(a,b){this.dispatchEvent(new sk("abort",this,a,b))};
pk.prototype.g1=function(a,b){this.dispatchEvent(new sk(Li,this,a,b))};
pk.prototype.wi=function(a,b){var c=this.Be.get(a);if(c.xA()>this.rM)this.dispatchEvent(new sk(Mi,this,a,b))};
pk.prototype.YN=function(a,b,c){var d=c||qk;this.p.Sa(a,d,b)};
pk.prototype.YS=function(a,b,c){var d=c||qk;this.p.g(a,d,b)};
pk.prototype.b=function(){if(!this.W()){pk.f.b.call(this);this.As.b();this.As=null;this.p.b();this.p=null;var a=this.Be;ad(a,function(b){b.b()});
a.clear();this.Be=null}};
var sk=function(a,b,c,d){D.call(this,a,b);this.id=c;this.xhrLite=d};
o(sk,D);sk.prototype.b=function(){if(!this.W){sk.f.b.call(this);this.id=null;this.xhrLite=null}};
var rk=function(a,b,c,d,e,f){this.cp=a;this.eda=c||"GET";this.Pb=d;this.VK=e;this.qT=0;this.kt=false;this.uS=false;this.Paa=b;this.BU=f;this.xhrLite=null};
o(rk,Vd);rk.prototype.uq=function(){return this.cp};
rk.prototype.YY=function(){return this.eda};
rk.prototype.Vc=function(){return this.Pb};
rk.prototype.GY=function(){return this.VK};
rk.prototype.xA=function(){return this.qT};
rk.prototype.e2=function(){this.qT++};
rk.prototype.e8=function(a){this.kt=a};
rk.prototype.jY=function(){return this.kt};
rk.prototype.T7=function(a){this.uS=a};
rk.prototype.QX=function(){return this.uS};
rk.prototype.eB=function(){return this.Paa};
rk.prototype.NI=function(){return this.BU};
rk.prototype.b=function(){if(!this.W()){rk.f.b.call(this);this.Paa=null;this.BU=null}};var P=function(a){J.call(this);this.K=a||Je();this.Vw=tk};
o(P,J);var uk=0,tk=null,vk="disable",wk="enable",xk="action",yk="change",zk="Component already rendered",Ak="Unable to set parent component",Bk=function(a,b){switch(a){case 1:return b?vk:wk;case 2:return b?"highlight":"unhighlight";case 4:return b?"activate":"deactivate";case 8:return b?"select":"unselect";case 16:return b?"check":"uncheck";case 32:return b?"focus":"blur";case 64:return b?"open":"close";default:}throw Error("Invalid component state");};
P.prototype.Za=null;P.prototype.K=null;P.prototype.Eb=false;P.prototype.e=null;P.prototype.Vw=null;P.prototype.k=null;P.prototype.P=null;P.prototype.Va=null;P.prototype.Lf=null;P.prototype.dF=null;P.prototype.H=function(){return this.Za||(this.Za=this.bZ())};
P.prototype.kx=function(a){if(this.P&&this.P.Lf){Nc(this.P.Lf,this.Za);Qc(this.P.Lf,a,this)}this.Za=a};
P.prototype.D=function(){return"goog.ui.Component"};
P.prototype.A=function(){return this.e};
P.prototype.od=function(a){this.e=a};
P.prototype.WD=function(a){if(this==a)throw Error(Ak);if(a&&this.P&&this.P.LI(this.Za))throw Error(Ak);this.P=a};
P.prototype.Ia=function(){return this.P};
P.prototype.ru=function(){return this.P};
P.prototype.tx=function(){throw Error("Method not supported");};
P.prototype.Ha=function(){return this.Eb};
P.prototype.m=function(){this.e=this.K.createElement("div")};
P.prototype.C=function(a){this.Qg(a)};
P.prototype.Qg=function(a,b){if(this.Eb)throw Error(zk);if(!this.e)this.m();if(a)a.insertBefore(this.e,b||null);else this.K.Ic().body.appendChild(this.e);if(!this.P||this.P.Ha())this.n()};
P.prototype.ia=function(a){if(this.Eb)throw Error(zk);else if(a&&this.kb(a)){this.dF=true;if(!this.K||this.K.Ic()!=Ie(a))this.K=Je(a);this.$(a);this.n()}else throw Error("Invalid element to decorate");};
P.prototype.kb=function(){return true};
P.prototype.$=function(a){this.e=a};
P.prototype.n=function(){this.Eb=true;this.sh(function(a){if(a.A())a.n()})};
P.prototype.u=function(){this.sh(function(a){if(a.Ha())a.u()});
this.Eb=false};
P.prototype.b=function(){if(!this.W()){P.f.b.call(this);if(this.Eb)this.u();this.sh(function(a){a.b()});
if(!this.dF&&this.e)A(this.e);this.Va=null;this.Lf=null;this.e=null;this.k=null;this.P=null}};
P.prototype.na=function(){return this.k};
P.prototype.U=function(a){this.k=a};
P.prototype.bZ=function(){return":"+uk++};
P.prototype.Z=function(a,b){this.mp(a,this.Ag(),b)};
P.prototype.mp=function(a,b,c){if(a.Eb&&(c||!this.Eb))throw Error(zk);if(b<0||b>this.Ag())throw Error("Child component index out of bounds");a.WD(this);if(!this.Lf||!this.Va){this.Lf={};this.Va=[]}Qc(this.Lf,a.H(),a);rc(this.Va,a,b);if(c){if(!this.e)this.m();var d=this.wd(b+1);a.Qg(this.Oa(),d?d.e:null)}else if(this.Eb&&!a.Eb&&a.e)a.n()};
P.prototype.Oa=function(){return this.e};
P.prototype.Xn=function(){if(this.Vw==null)this.Vw=Fh(this.Eb?this.e:this.K.Ic().body);return this.Vw};
P.prototype.kB=function(){return!(!this.Va)&&this.Va.length!=0};
P.prototype.Ag=function(){return this.Va?this.Va.length:0};
P.prototype.LI=function(a){return this.Lf&&a?Rc(this.Lf,a)||null:null};
P.prototype.wd=function(a){return this.Va?this.Va[a]||null:null};
P.prototype.sh=function(a,b){if(this.Va)r(this.Va,a,b)};
P.prototype.Un=function(a){return this.Va&&a?cc(this.Va,a):-1};
P.prototype.removeChild=function(a,b){if(a){var c=za(a)?a:a.H();a=this.LI(c);if(c&&a){Nc(this.Lf,c);tc(this.Va,a);if(b){a.u();if(a.e)A(a.e)}a.WD(null)}}if(!a)throw Error("Child is not in parent component");return a};
P.prototype.SN=function(a,b){return this.removeChild(this.wd(a),b)};
P.prototype.Jh=function(a){while(this.kB())this.SN(0,a)};var Ck=se&&!Ee("1.9a"),Dk=function(a,b){if(se){a.setAttribute("role",b);a.roleName=b}},
Ek=function(a,b,c){if(se)if(Ck)a.setAttributeNS("http://www.w3.org/2005/07/aaa",b,c);else a.setAttribute("aria-"+b,c)};var Fk=function(){},
Gk=null;Fk.prototype.Pu=function(a){return a&&a.tabIndex>=1};
Fk.prototype.Cj=function(a,b){if(a)a.tabIndex=b?Math.max(a.tabIndex,1):-1};
Fk.prototype.wA=function(){return undefined};
Fk.prototype.m=function(a){var b=this.si(a);return a.K.m("div",b?{"class":b.join(" ")}:null,a.Vc())};
Fk.prototype.Oa=function(a){return a};
Fk.prototype.aA=function(a,b,c){var d=a.A();if(d)tf(d,b,c)};
Fk.prototype.kb=function(){return true};
Fk.prototype.ia=function(a,b){if(b.id)a.kx(b.id);var c=this.Oa(b);if(c&&c.firstChild)a.sm(c.firstChild.nextSibling?uc(c.childNodes):c.firstChild);else a.sm(null);a.YD(this.FZ(pf(b)));C(b,this.O());var d=a.kJ();if(d)r(d,function(e){this.aA(a,e,true)},
this);return b};
Fk.prototype.sf=function(a){var b=a.A();Hh(b,true,se);tf(b,this.O()+"-rtl",a.Xn());var c=this.wA();if(c)Dk(b,c)};
Fk.prototype.Vb=function(a,b,c){var d=a.A();if(d){var e=this.Bg(b);if(e)tf(d,e,c);this.faa(a,b,c)}};
Fk.prototype.faa=function(a,b,c){var d;switch(b){case 1:d="disabled";break;case 4:d="pressed";break;case 8:d="selected";break;case 16:d="checked";break;case 64:d="expanded";break;case 2:case 32:default:break}if(d){var e=a.A();if(e)Ek(e,d,c)}};
Fk.prototype.hc=function(a,b){var c=this.Oa(a);if(c)if(za(b))B(c,b);else{We(c);if(b){function d(e){if(e){var f=Ie(c);c.appendChild(za(e)?f.createTextNode(e):e)}}
if(xa(b))r(b,d);else d(b)}}};
Fk.prototype.$b=function(a){return a.A()};
Fk.prototype.O=function(){return"goog-control"};
Fk.prototype.si=function(a){var b=this.gY(a.bK()),c=a.kJ();return c?b.concat(c):b};
Fk.prototype.gY=function(a){var b=[this.O()];if(a)for(var c=1;a;c<<=1)if(a&c){b.push(this.Bg(c));a&=~c}return b};
Fk.prototype.Bg=function(a){if(a){var b=this.O();switch(a){case 1:return b+"-disabled";case 2:return b+"-hover";case 4:return b+"-active";case 8:return b+"-selected";case 16:return b+"-checked";case 32:return b+"-focused";case 64:return b+"-open"}}return null};
Fk.prototype.FZ=function(a){var b=0;if(a)r(a,function(c){b|=this.xu(c)},
this);return b};
Fk.prototype.xu=function(a){if(a){var b=bc(a.split("-"));switch(b){case "disabled":return 1;case "hover":return 2;case "active":return 4;case "selected":return 8;case "checked":return 16;case "focused":return 32;case "open":return 64}}return 0};var Hk=function(){Fk.call(this)};
o(Hk,Fk);var Ik=null;Hk.prototype.wA=function(){return"button"};
Hk.prototype.m=function(a){var b={},c=this.si(a);if(c)b["class"]=c.join(" ");if(!a.sb())b.disabled=true;if(a.Qj())b.title=a.Qj();if(a.L())b.value=a.L();return a.K.m("button",b,a.Hj()||"")};
Hk.prototype.kb=function(a){return a.tagName=="BUTTON"||a.tagName=="INPUT"&&(a.type=="button"||a.type=="submit"||a.type=="reset")};
Hk.prototype.ia=function(a,b){b=Hk.f.ia.call(this,a,b);a.m9(this.Qj(b));a.w9(this.L(b));if(b.disabled){a.YD(a.bK()|1);C(b,this.Bg(1))}return b};
Hk.prototype.Vb=function(a,b,c){Hk.f.Vb.call(this,a,b,c);if(b==1){var d=a.A();if(d&&(d.tagName=="BUTTON"||d.tagName=="INPUT"))d.disabled=c}};
Hk.prototype.L=function(a){return a&&a.value};
Hk.prototype.qa=function(a,b){if(a&&(a.tagName=="BUTTON"||a.tagName=="INPUT"))a.value=b};
Hk.prototype.Qj=function(a){return a&&a.title};
Hk.prototype.Dm=function(a,b){if(a)a.title=b};
Hk.prototype.O=function(){return"goog-button"};var Q=function(a,b,c){P.call(this,c);this.pa=b||Gk||(Gk=new Fk);this.sm(a)};
o(Q,P);var Jk={},Kk=function(a,b){Jk[a]=b},
Lk=function(a){if(a){var b=pf(a);for(var c=0,d=b.length;c<d;c++){var e=b[c];if(e&&e in Jk)return Jk[e]()}}return null};
Q.prototype.pa=null;Q.prototype.Pb=null;Q.prototype.Bb=0;Q.prototype.hs=39;Q.prototype.uT=255;Q.prototype.Mx=0;Q.prototype.la=true;Q.prototype.j=null;Q.prototype.ob=null;Q.prototype.ni=null;Q.prototype.Dg=function(){return this.j||(this.j=new K(this))};
Q.prototype.$b=function(){return this.pa.$b(this)};
Q.prototype.hq=function(){return this.ob||(this.ob=new Dg)};
Q.prototype.Tf=function(){return this.pa};
Q.prototype.kJ=function(){return this.ni};
Q.prototype.Jd=function(a){if(a){if(this.ni)this.ni.push(a);else this.ni=[a];this.pa.aA(this,a,true)}};
Q.prototype.r6=function(a){if(a&&this.ni){tc(this.ni,a);if(this.ni.length==0)this.ni=null;this.pa.aA(this,a,false)}};
Q.prototype.D=function(){return"goog.ui.Control"};
Q.prototype.m=function(){this.od(this.pa.m(this))};
Q.prototype.Oa=function(){return this.pa.Oa(this.A())};
Q.prototype.kb=function(a){return this.pa.kb(a)};
Q.prototype.$=function(a){this.od(this.pa.ia(this,a));if(a.style.display=="none")this.la=false};
Q.prototype.n=function(){Q.f.n.call(this);var a=this.A();this.pa.sf(this);this.M(this.la,true);if(this.hs&-2){var b=this.Dg();b.g(a,Gf,this.Kl);b.g(a,I,this.Ud);b.g(a,eg,this.Ll);b.g(a,Hf,this.Jl);if(this.wf(32)){var c=this.$b();b.g(c,jg,this.zq);b.g(c,ig,this.Hg);var d=this.hq();d.rc(c);b.g(d,"key",this.Jc)}}};
Q.prototype.u=function(){Q.f.u.call(this);if(this.j)this.j.ma();if(this.ob)this.ob.detach()};
Q.prototype.b=function(){if(!this.W()){Q.f.b.call(this);if(this.j){this.j.b();this.j=null}if(this.ob){this.ob.b();this.ob=null}this.pa=null;this.Pb=null;this.ni=null}};
Q.prototype.Vc=function(){return this.Pb};
Q.prototype.hc=function(a){this.pa.hc(this.A(),a);this.sm(a)};
Q.prototype.sm=function(a){this.Pb=a};
Q.prototype.Hj=function(){if(!this.Pb||za(this.Pb))return this.Pb;var a=xa(this.Pb)?gc(this.Pb,lf).join(""):lf(this.Pb);return a&&Va(a)};
Q.prototype.Sg=function(a){this.hc(a)};
Q.prototype.Ja=function(a){return this.la&&(a||!this.Q2())};
Q.prototype.Q2=function(){var a=this.Ia();return a&&typeof a.Ja=="function"&&!a.Ja()};
Q.prototype.M=function(a,b){if(b||this.la!=a&&this.dispatchEvent(a?"show":"hide")){this.la=a;var c=this.A();if(c){M(c,a);if(this.wf(32)){this.pa.Cj(this.$b(),this.sb());if(!this.sb())this.Tr(false)}else this.pa.Cj(this.$b(),false)}return true}return false};
Q.prototype.sb=function(){return this.Ja()&&!this.Xf(1)};
Q.prototype.P2=function(){var a=this.Ia();return a&&typeof a.sb=="function"&&!a.sb()};
Q.prototype.Ca=function(a){if(!this.P2()&&this.Xl(1,!a)){if(this.wf(32)){if(!a)this.Tr(false);var b=a&&this.la;this.pa.Cj(this.$b(),b);if(!b)this.Tr(false)}if(!a){this.Fd(false);this.Ab(false)}this.Vb(1,!a)}};
Q.prototype.Ab=function(a){if(this.Xl(2,a))this.Vb(2,a)};
Q.prototype.zd=function(){return this.Xf(4)};
Q.prototype.Fd=function(a){if(this.sb()&&this.Xl(4,a))this.Vb(4,a)};
Q.prototype.Yn=function(){return this.Xf(8)};
Q.prototype.Oc=function(a){if(this.Xl(8,a))this.Vb(8,a)};
Q.prototype.ak=function(){return this.Xf(16)};
Q.prototype.rm=function(a){if(this.Xl(16,a))this.Vb(16,a)};
Q.prototype.Tr=function(a){if(this.Xl(32,a))this.Vb(32,a)};
Q.prototype.isOpen=function(){return this.Xf(64)};
Q.prototype.Jb=function(a){if(this.Xl(64,a))this.Vb(64,a)};
Q.prototype.bK=function(){return this.Bb};
Q.prototype.Xf=function(a){return!(!(this.Bb&a))};
Q.prototype.Vb=function(a,b){if(this.wf(a)&&b!=this.Xf(a)){this.pa.Vb(this,a,b);this.Bb=b?this.Bb|a:this.Bb&~a}};
Q.prototype.YD=function(a){this.Bb=a};
Q.prototype.wf=function(a){return!(!(this.hs&a))};
Q.prototype.Sh=function(a,b){if(b!=this.wf(a)){if(this.Ha())throw Error(zk);if(!b&&this.Xf(a))this.Vb(a,false);this.hs=b?this.hs|a:this.hs&~a}};
Q.prototype.tf=function(a){return!(!(this.uT&a))&&this.wf(a)};
Q.prototype.HD=function(a,b){this.Mx=b?this.Mx|a:this.Mx&~a};
Q.prototype.Xl=function(a,b){return this.wf(a)&&this.Xf(a)!=b&&(!(this.Mx&a)||this.dispatchEvent(Bk(a,b)))};
Q.prototype.Kl=function(a){if(a.relatedTarget&&!df(this.A(),a.relatedTarget)&&this.dispatchEvent("enter")&&this.sb()&&this.tf(2))this.Ab(true)};
Q.prototype.Jl=function(a){if(a.relatedTarget&&!df(this.A(),a.relatedTarget)&&this.dispatchEvent("leave")){if(this.tf(4))this.Fd(false);if(this.tf(2))this.Ab(false)}};
Q.prototype.Ud=function(a){if(this.sb()){if(this.tf(2))this.Ab(true);if(this.tf(4)&&a.Vl(0))this.Fd(true)}var b=this.$b();if(this.pa.Pu(b)){if(!se)b.focus()}else a.preventDefault()};
Q.prototype.Ll=function(a){if(this.sb()&&this.tf(2))this.Ab(true);if(this.zd()&&this.Oi(a)&&this.tf(4))this.Fd(false)};
Q.prototype.Oi=function(){if(this.tf(16))this.rm(!this.ak());if(this.tf(8))this.Oc(true);if(this.tf(64))this.Jb(!this.isOpen());return this.dispatchEvent(xk)};
Q.prototype.zq=function(){if(this.tf(32))this.Tr(true)};
Q.prototype.Hg=function(){if(this.tf(4))this.Fd(false);if(this.tf(32))this.Tr(false)};
Q.prototype.Jc=function(a){return this.sb()&&a.keyCode==13&&this.Oi(a)};
Kk("goog-control",function(){return new Q(null)});var Mk=function(a,b,c){Q.call(this,a,b||Ik||(Ik=new Hk),c)};
o(Mk,Q);Mk.prototype.Ua=null;Mk.prototype.Zx=null;Mk.prototype.L=function(){return this.Ua};
Mk.prototype.qa=function(a){this.Ua=a;this.pa.qa(this.A(),a)};
Mk.prototype.w9=function(a){this.Ua=a};
Mk.prototype.Qj=function(){return this.Zx};
Mk.prototype.Dm=function(a){this.Zx=a;this.pa.Dm(this.A(),a)};
Mk.prototype.m9=function(a){this.Zx=a};
Mk.prototype.D=function(){return"goog.ui.Button"};
Mk.prototype.b=function(){if(!this.W()){Mk.f.b.call(this);this.Ua=null;this.Zx=null}};
Kk("goog-button",function(){return new Mk(null)});var Nk=function(){Hk.call(this)};
o(Nk,Hk);var Ok=null,Pk=function(){return Ok||(Ok=new Nk)},
Qk="goog-custom-button";Nk.prototype.m=function(a){var b=this.si(a),c={"class":"goog-inline-block "+b.join(" "),title:a.Qj()||""};return a.K.m("div",c,this.oj(a.Vc(),a.K))};
Nk.prototype.Oa=function(a){return a&&a.firstChild&&a.firstChild.firstChild};
Nk.prototype.oj=function(a,b){var c="goog-inline-block "+this.O();return b.m("div",{"class":c+"-outer-box"},b.m("div",{"class":c+"-inner-box"},a))};
Nk.prototype.kb=function(a){return a.tagName=="DIV"};
Nk.prototype.s1=function(a,b){var c=a.K.nJ(b);if(c&&c.className.indexOf(this.O()+"-outer-box")!=-1){var d=a.K.nJ(c);if(d&&d.className.indexOf(this.O()+"-inner-box")!=-1)return true}return false};
Nk.prototype.ia=function(a,b){Rk(b,true);Rk(b,false);if(!this.s1(a,b))b.appendChild(this.oj(b.childNodes,a.K));C(b,"goog-inline-block",this.O());return Nk.f.ia.call(this,a,b)};
Nk.prototype.O=function(){return Qk};
var Rk=function(a,b){if(a){var c=b?a.firstChild:a.lastChild,d;while(c&&c.parentNode==a){d=b?c.nextSibling:c.previousSibling;if(c.nodeType==3){var e=c.nodeValue;if(Va(e)=="")a.removeChild(c);else{c.nodeValue=b?e.replace(/^[\s\xa0]+/,""):Wa(e);break}}else break;c=d}}};var Sk=function(a,b,c){Mk.call(this,a,b||Pk(),c)};
o(Sk,Mk);Kk(Qk,function(){return new Sk(null)});var Tk=function(a){J.call(this);var b;b=Da(a)?new Yb(a):new Yb;if(!mi(ii,hi()))ni(ri,hi());this.N=b;this.dd=this.N.ra();this.dd.setDate(1);this.fR=["","","","","","",""];this.Dv=[];this.Hba=Hi(0)};
o(Tk,J);Tk.prototype.bE=true;Tk.prototype.S9=true;Tk.prototype.tI=true;Tk.prototype.TP=true;Tk.prototype.UP=true;Tk.prototype.ZF=true;Tk.prototype.T9=true;Tk.prototype.qt=false;Tk.prototype.HH=null;var Uk=0;Tk.prototype.CY=function(){return this.dd.CY()};
Tk.prototype.v8=function(a){this.dd.u8(a);this.$g();this.QN()};
Tk.prototype.EP=function(a,b){this.fR[a]=b;this.dD()};
Tk.prototype.f9=function(a){this.bE=a;this.$g()};
Tk.prototype.r8=function(a){this.tI=a;this.$g()};
Tk.prototype.g9=function(a){this.TP=a;this.dD()};
Tk.prototype.Q5=function(){this.dd.add(new Xb("m",-1));this.$g()};
Tk.prototype.z4=function(){this.dd.add(new Xb("m",1));this.$g()};
Tk.prototype.S5=function(){this.dd.add(new Xb("y",-1));this.$g()};
Tk.prototype.C4=function(){this.dd.add(new Xb("y",1));this.$g()};
Tk.prototype.AO=function(){this.setDate(new Yb)};
Tk.prototype.xO=function(){if(this.ZF)this.setDate(null)};
Tk.prototype.getDate=function(){return this.N};
Tk.prototype.setDate=function(a){var b=false;if(a==null&&this.N!=null||a!=null&&this.N==null)b=true;else if(a==null)b=false;else if(a.getDate()!=this.N.getDate()||a.getMonth()!=this.N.getMonth()||a.getFullYear()!=this.N.getFullYear())b=true;this.N=Da(a)?new Yb(a):null;if(a){this.dd.J(this.N);this.dd.setDate(1)}this.$g();var c=new Vk("select",this,this.N?this.N:null);this.dispatchEvent(c);if(b){var d=new Vk("change",this,this.N?this.N:null);this.dispatchEvent(d)}};
Tk.prototype.create=function(a){this.Y=a;var b=Ie(a);a.className="goog-date-picker";var c=b.createElement("table"),d=b.createElement("thead"),e=b.createElement("tbody"),f=b.createElement("tfoot");Dk(e,"grid");e.tabIndex="0";this.x$=e;var g=v("tr",{className:"goog-date-picker-head"}),h=b.createElement("td");h.colSpan=5;this.pj(h,"<",this.Q5);this.Qp=this.pj(h,"",this.R9,"goog-date-picker-month");this.pj(h,">",this.z4);g.appendChild(h);h=b.createElement("td");h.colSpan=3;this.pj(h,"<",this.S5);this.Yz=
this.pj(h,"",this.U9,"goog-date-picker-year");this.pj(h,">",this.C4);g.appendChild(h);d.appendChild(g);this.nh=[];for(var i=0;i<7;i++){g=b.createElement("tr");this.nh[i]=[];for(var k=0;k<8;k++){h=b.createElement(k==0||i==0?"th":"td");if((k==0||i==0)&&k!=i){h.className=k==0?"goog-date-picker-week":"goog-date-picker-wday";Dk(h,k==0?"rowheader":"columnheader")}g.appendChild(h);this.nh[i][k]=h}e.appendChild(g)}g=v("tr",{className:"goog-date-picker-foot"});h=v("td",{colSpan:4,className:"goog-date-picker-today-cont"});
var n=m("Today");this.LW=this.pj(h,n,this.AO);M(this.LW,this.T9);g.appendChild(h);h=b.createElement("td");h.colSpan=4;h.className="goog-date-picker-none-cont";var q=m("None");this.aI=this.pj(h,q,this.xO);M(this.aI,this.ZF);g.appendChild(h);f.appendChild(g);c.cellSpacing="0";c.cellPadding="0";c.appendChild(d);c.appendChild(e);c.appendChild(f);a.appendChild(c);this.qt=true;this.QN();this.$g();this.Dv.push(F(e,E,this.Z_,false,this));this.Dv.push(F(a,If,this.$_,false,this));a.tabIndex=0};
Tk.prototype.b=function(){if(!this.W()){J.prototype.b.call(this);this.kn();var a=this.Dv.length;for(var b=0;b<a;b++)H(this.Dv[b]);this.Y.innerHTML="";this.Y=null;this.nh=null;this.Qp=null;this.Yz=null;this.LW=null;this.aI=null}};
Tk.prototype.Z_=function(a){if(a.target.tagName=="TD"){var b,c=-2,d=-2;for(b=a.target;b;b=b.previousSibling,c++){}for(b=a.target.parentNode;b;b=b.previousSibling,d++){}var e=this.mc[d][c];this.setDate(e.ra())}};
Tk.prototype.$_=function(a){var b,c;switch(a.keyCode){case 33:a.preventDefault();b=-1;break;case 34:a.preventDefault();b=1;break;case 37:a.preventDefault();c=-1;break;case 39:a.preventDefault();c=1;break;case 38:a.preventDefault();c=-7;break;case 40:a.preventDefault();c=7;break;case 36:a.preventDefault();return this.AO();case 46:a.preventDefault();return this.xO();default:return}var d;if(this.N){d=this.N.ra();d.add(new Xb(0,b,c))}else{d=this.dd.ra();d.setDate(1)}this.setDate(d)};
Tk.prototype.R9=function(a){a.stopPropagation();var b=[];for(var c=0;c<12;c++)b.push(Ub[c]);this.qH(this.Qp,b,this.z0,Ub[this.dd.getMonth()])};
Tk.prototype.U9=function(a){a.stopPropagation();var b=[],c=this.dd.getFullYear()-5;for(var d=0;d<11;d++)b.push(String(c+d));this.qH(this.Yz,b,this.p1,this.dd.getFullYear())};
Tk.prototype.z0=function(a){var b=a;for(var c=-1;b;b=bf(b),c++){}this.dd.setMonth(c);this.$g();if(this.Qp.focus)this.Qp.focus()};
Tk.prototype.p1=function(a){if(a.firstChild.nodeType==3){this.dd.setFullYear(a.firstChild.nodeValue);this.$g()}this.Yz.focus()};
Tk.prototype.qH=function(a,b,c,d){this.kn();var e=Ie(a),f=v("div",{className:"goog-date-picker-menu"});this.Zq=null;var g=w("ul");for(var h=0;h<b.length;h++){var i=v("li",null,b[h]);if(b[h]==d)this.Zq=i;g.appendChild(i)}f.appendChild(g);f.style.left=a.offsetLeft+a.parentNode.offsetLeft+"px";f.style.top=a.offsetTop+"px";f.style.width=a.clientWidth+"px";this.Qp.parentNode.appendChild(f);this.R=f;if(!this.Zq)this.Zq=g.firstChild;this.Zq.className="goog-date-picker-menu-selected";this.tM=c;F(this.R,E,
this.DK,false,this);F(this.R,If,this.EK,false,this);F(e,E,this.kn,false,this);f.tabIndex=0;f.focus()};
Tk.prototype.DK=function(a){a.stopPropagation();this.kn();if(this.tM)this.tM(a.target)};
Tk.prototype.EK=function(a){a.stopPropagation();var b,c=this.Zq;switch(a.keyCode){case 35:a.preventDefault();b=c.parentNode.lastChild;break;case 36:a.preventDefault();b=c.parentNode.firstChild;break;case 38:a.preventDefault();b=c.previousSibling;break;case 40:a.preventDefault();b=c.nextSibling;break;case 13:case 9:case 0:a.preventDefault();this.kn();this.tM(c);break}if(b&&b!=c){c.className="";b.className="goog-date-picker-menu-selected";this.Zq=b}};
Tk.prototype.kn=function(){if(this.R){var a=Ie(this.R);G(this.R,E,this.DK,false,this);G(this.R,If,this.EK,false,this);G(a,E,this.kn,false,this);A(this.R);delete this.R}};
Tk.prototype.pj=function(a,b,c,d){var e=Ie(a),f=e.createElement("button");f.className=d||"goog-date-picker-btn";f.appendChild(e.createTextNode(b));a.appendChild(f);this.Dv.push(F(f,E,c,false,this));return f};
Tk.prototype.$g=function(){if(!this.qt)return;var a=this.dd.ra();a.setDate(1);B(this.Qp,Ub[a.getMonth()]);B(this.Yz,a.getFullYear());var b=a.jK(),c=a.IJ();a.add(new Xb("m",-1));a.setDate(a.IJ()-(b-1));if(this.bE&&!this.tI&&c+b<33)a.add(new Xb("d",-7));b=a.jK();var d=new Xb("d",1);this.mc=[];for(var e=0;e<6;e++){this.mc[e]=[];for(var f=0;f<7;f++){this.mc[e][f]=a.ra();a.add(d)}b=0}this.dD()};
Tk.prototype.dD=function(){if(!this.qt)return;var a=this.dd.getMonth(),b=new Yb,c=b.getFullYear(),d=b.getMonth(),e=b.getDate();for(var f=0;f<6;f++){if(this.TP){B(this.nh[f+1][0],this.mc[f][0].RZ());of(this.nh[f+1][0],"goog-date-picker-week")}else{B(this.nh[f+1][0],"");of(this.nh[f+1][0],"")}this.x$.title=this.N?this.Hba.Cn(this.N):"";for(var g=0;g<7;g++){var h=this.mc[f][g],i=this.nh[f+1][g+1];if(!i.id)i.id="goog-dp-"+Uk++;Dk(i,"gridcell");if(this.S9||h.getMonth()==a){var k=[];if(h.getMonth()!=a)k.push("goog-date-picker-other-month");
var n=(g+this.dd.mJ()+7)%7;if(this.fR[n])k.push(this.fR[n]);if(h.getDate()==e&&h.getMonth()==d&&h.getFullYear()==c){k.push("goog-date-picker-today");var q=m("Today");i.title=q}if(this.N&&h.getDate()==this.N.getDate()&&h.getMonth()==this.N.getMonth()&&h.getFullYear()==this.N.getFullYear()){k.push("goog-date-picker-selected");Ek(this.x$,"activedescendant",i.id)}if(this.HH){var t=this.HH(h);if(t)k.push(t)}of(i,k.join(" "));B(i,h.getDate())}else{of(i,"");B(i,"")}}if(f>=4)M(this.nh[f+1][0].parentNode,
this.mc[f][0].getMonth()==a||this.bE)}};
Tk.prototype.QN=function(){if(!this.qt)return;if(this.UP)for(var a=0;a<7;a++){var b=this.nh[0][a+1],c=(a+this.dd.mJ()+7)%7;B(b,Hb[c])}M(this.nh[0][0].parentNode,this.UP)};
var Vk=function(a,b,c){D.call(this,a);this.target=b;this.date=c};
o(Vk,D);var R=function(a,b,c){P.call(this,c);this.Gc=a||"modal-dialog";this.aF=!(!b);this.kc=Wk;this.p=new K(this)};
o(R,P);R.prototype.Gc=null;R.prototype.aF=false;R.prototype.xM=true;R.prototype.XH=true;R.prototype.xT=0.3;R.prototype.Fk="";R.prototype.Pb="";R.prototype.kc=null;R.prototype.fd=null;R.prototype.la=false;R.prototype.sW=false;R.prototype.Qc=null;R.prototype.Rc=null;R.prototype.pg=null;R.prototype.Zg=null;R.prototype.zE=null;R.prototype.Ek=null;R.prototype.df=null;R.prototype.he=null;R.prototype.p=null;R.prototype.D=function(){return"goog.ui.Dialog"};
R.prototype.Xi=function(a){this.Fk=a;if(this.Zg)B(this.Zg,a)};
R.prototype.Td=function(){return this.Fk};
R.prototype.hc=function(a){this.Pb=a;if(this.df)this.df.innerHTML=a};
R.prototype.Vc=function(){return this.Pb};
R.prototype.Oa=function(){if(!this.df)this.C();return this.df};
R.prototype.Au=function(){if(!this.Ha())this.C();return this.Zg};
R.prototype.aV=function(){var a=new Xh(this.e,this.pg);C(this.pg,this.Gc+"-title-draggable");return a};
R.prototype.m=function(){this.jM();this.e=this.K.m("div",{className:this.Gc,tabIndex:0},this.pg=this.K.m("div",{className:this.Gc+"-title",id:this.H()},this.Zg=this.K.m("span",{className:this.Gc+"-title-text"},this.Fk),this.Ek=this.K.m("span",{className:this.Gc+"-title-close"})),this.df=this.K.m("div",{className:this.Gc+"-content"}),this.he=this.K.m("div",{className:this.Gc+"-buttons"}));this.zE=this.pg.id;Dk(this.e,"dialog");Ek(this.e,"labelledby",this.zE);if(this.Pb)this.df.innerHTML=this.Pb;M(this.e,
false);if(this.kc)this.kc.Xy(this.he)};
R.prototype.jM=function(){if(this.aF&&!this.Rc){this.Rc=this.K.m("iframe",{frameborder:0,style:"border: 0",className:this.Gc+"-bg"});M(this.Rc,false);Ch(this.Rc,0)}else if(!this.aF&&this.Rc){A(this.Rc);this.Rc=null}if(this.xM&&!this.Qc){this.Qc=this.K.m("div",{className:this.Gc+"-bg"});Ch(this.Qc,this.xT);M(this.Qc,false)}else if(!this.xM&&this.Qc){A(this.Qc);this.Qc=null}};
R.prototype.C=function(a){if(this.Ha())throw Error(zk);if(!this.e)this.m();var b=a||this.K.Ic().body;this.ZN(b);R.f.C.call(this,b)};
R.prototype.ZN=function(a){if(this.Rc)a.appendChild(this.Rc);if(this.Qc)a.appendChild(this.Qc)};
R.prototype.kb=function(a){return a&&a.tagName&&a.tagName=="DIV"&&R.f.kb.call(this,a)};
R.prototype.$=function(a){this.e=a;C(this.e,this.Gc);var b=this.Gc+"-content";this.df=gf(this.e,function(g){return g.nodeType==1&&sf(g,b)});
if(this.df)this.Pb=this.df.innerHTML;else{this.df=this.K.m("div",{className:b});if(this.Pb)this.df.innerHTML=this.Pb;this.e.appendChild(this.df)}var c=this.Gc+"-title",d=this.Gc+"-title-text",e=this.Gc+"-title-close";this.pg=gf(this.e,function(g){return g.nodeType==1&&sf(g,c)});
if(this.pg){this.Zg=gf(this.pg,function(g){return g.nodeType==1&&sf(g,d)});
this.Ek=gf(this.pg,function(g){return g.nodeType==1&&sf(g,e)})}else{this.pg=this.K.m("div",
{className:c});this.e.insertBefore(this.pg,this.df)}if(this.Zg)this.Fk=lf(this.Zg);else{this.Zg=this.K.m("span",{className:d},this.Fk);this.pg.appendChild(this.Zg)}Ek(this.e,"labelledby",this.zE);if(!this.Ek){this.Ek=this.K.m("span",{className:e},this.Fk);this.pg.appendChild(this.Ek)}var f=this.Gc+"-buttons";this.he=gf(this.e,function(g){return g.nodeType==1&&sf(g,f)});
if(this.he){this.kc=new Xk(this.K);this.kc.ia(this.he)}else{this.he=this.K.m("div",{className:f});this.e.appendChild(this.he);if(this.kc)this.kc.Xy(this.he)}this.jM();this.ZN(Ie(this.e).body)};
R.prototype.n=function(){R.f.n.call(this);if(this.XH&&!this.fd)this.fd=this.aV();this.p.g(this.Ek,E,this.ZM);this.p.g(this.he,E,this.RM);Dk(this.e,"dialog");if(this.Zg.id!=="")Ek(this.e,"labelledby",this.Zg.id)};
R.prototype.u=function(){this.p.Sa(this.Ek,E,this.ZM);this.p.Sa(this.he,E,this.RM);if(this.Ja())this.M(false);if(this.fd){this.fd.b();this.fd=null}R.f.u.call(this)};
R.prototype.M=function(a){if(a==this.la)return;var b=this.K.Ic(),c=Te(b)||window;if(!this.Ha())this.C(b.body);if(a){this.iO();this.Qa();this.p.g(b,gg,this.rr,true);this.p.g(c,mg,this.yr,true)}else{this.p.Sa(b,gg,this.rr,true);this.p.Sa(c,mg,this.yr,true)}if(this.Rc)M(this.Rc,a);if(this.Qc)M(this.Qc,a);M(this.e,a);if(a&&this.rl()){var d=this.rl().$I();if(d){var e=this.he.getElementsByTagName("button");for(var f=0,g;g=e[f];f++)if(g.name==d){g.focus();break}}}this.la=a;if(!a){this.dispatchEvent("afterhide");
if(this.sW)this.b()}};
R.prototype.Ja=function(){return this.la};
R.prototype.iO=function(){if(this.Rc)M(this.Rc,false);if(this.Qc)M(this.Qc,false);var a=this.K.Ic(),b=Te(a)||window,c=Qe(b),d=a.body.scrollWidth,e=Math.max(a.body.scrollHeight,c.height);if(this.Rc){M(this.Rc,true);zh(this.Rc,d,e)}if(this.Qc){M(this.Qc,true);zh(this.Qc,d,e)}if(this.XH){var f=Ah(this.e);this.fd.limits=new ke(0,0,d-f.width,e-f.height)}};
R.prototype.Qa=function(){var a=this.K.Ic(),b=Te(a)||window,c=Se(b),d=c.x,e=c.y,f=Ah(this.e),g=Qe(b),h=Math.max(d+g.width/2-f.width/2,0),i=Math.max(e+g.height/2-f.height/2,0);sh(this.e,h,i)};
R.prototype.ZM=function(){var a=this.rl(),b=a&&a.eu();if(b){var c=a.get(b);if(this.dispatchEvent(new Yk(b,c)))this.M(false)}else this.M(false)};
R.prototype.b=function(){if(!this.W()){R.f.b.call(this);if(this.p){this.p.b();this.p=null}if(this.Qc){A(this.Qc);this.Qc=null}if(this.Rc){A(this.Rc);this.Rc=null}this.Ek=null;this.he=null}};
R.prototype.Rg=function(a){this.kc=a;if(this.he)this.kc.Xy(this.he)};
R.prototype.rl=function(){return this.kc};
R.prototype.RM=function(a){var b=this.mX(a.target);if(b){var c=b.name,d=this.rl().get(c);if(this.dispatchEvent(new Yk(c,d)))this.M(false)}};
R.prototype.mX=function(a){var b=a;while(b!=null&&b!=this.he){if(b.tagName=="BUTTON")return b;b=b.parentNode}return null};
R.prototype.rr=function(a){if(a.keyCode==27){var b=this.rl();if(b.eu()){var c=b.get(b.eu());if(this.dispatchEvent(new Yk(b.eu(),c)))this.M(false)}else this.M(false)}else if(a.keyCode==13){var b=this.rl(),d=a.target&&a.target.tagName=="BUTTON"?a.target.name:b.$I();if(d&&this.dispatchEvent(new Yk(d,b.get(d))))this.M(false)}};
R.prototype.yr=function(){this.iO()};
var Yk=function(a,b){this.type="dialogselect";this.key=a;this.caption=b};
o(Yk,D);var Zk="dialogselect",Xk=function(a){this.K=a||Je();dd.call(this)},
Wk,$k,al,bl,cl;o(Xk,dd);Xk.prototype.IH=null;Xk.prototype.e=null;Xk.prototype.at=null;Xk.prototype.J=function(a,b,c,d){dd.prototype.J.call(this,a,b);if(c)this.IH=a;if(d)this.at=a};
Xk.prototype.Xy=function(a){this.e=a;this.C()};
Xk.prototype.C=function(){if(this.e){this.e.innerHTML="";var a=Je(this.e);ad(this,function(b,c){this.e.appendChild(a.m("button",{name:c},b))},
this)}};
Xk.prototype.ia=function(a){if(!a||a.nodeType!=1)return;this.e=a;var b=this.e.getElementsByTagName("button");for(var c=0,d,e,f;d=b[c];c++){e=d.name||d.id;f=lf(d)||d.value;if(e)this.J(e,f,c==0,d.name==dl)}};
Xk.prototype.$I=function(){return this.IH};
Xk.prototype.eu=function(){return this.at};
Xk.prototype.eY=function(a){var b=this.e.getElementsByTagName("button");for(var c=0,d;d=b[c];c++)if(d.name==a||d.id==a)return d;return null};
var dl="cancel";(function(){var a=m("OK"),b=m("Cancel"),c=m("Yes"),d=m("No"),e=m("Save"),f=m("Continue"),g=new Xk;g.J("ok",a,true);$k=g;var h=new Xk;h.J("ok",a,true);h.J(dl,b,false,true);Wk=h;var i=new Xk;i.J("yes",c,true);i.J("no",d,false,true);al=i;var k=new Xk;k.J("yes",c);k.J("no",d,true);k.J(dl,b,false,true);bl=k;var n=new Xk;n.J("continue",f);n.J("save",e);n.J(dl,b,true,true);cl=n})();var el=function(a,b){P.call(this,b);this.cg=a||""};
o(el,P);el.prototype.qh=null;el.prototype.D=function(){return"goog.ui.LabelInput"};
el.prototype.m=function(){this.e=this.K.m("input",{type:"text"})};
el.prototype.$=function(a){this.e=a;if(!this.cg)this.cg=a.getAttribute("label")||""};
el.prototype.n=function(){el.f.n.call(this);this.Vs();this.dt();this.e.ag=this};
el.prototype.u=function(){el.f.u.call(this);this.ln();this.e.ag=null};
el.prototype.Vs=function(){var a=new K(this);a.g(this.e,jg,this.Aq);a.g(this.e,ig,this.f_);if(se)a.g(this.e,[If,gg,hg],this.L_);var b=window;a.g(b,"load",this.n1);this.p=a;this.cG()};
el.prototype.cG=function(){if(!this.fca&&this.p&&this.e.form){this.p.g(this.e.form,"submit",this.P_);this.fca=true}};
el.prototype.ln=function(){if(this.p){this.p.b();this.p=null}};
el.prototype.b=function(){if(!this.W()){el.f.b.call(this);this.ln();this.e=null}};
el.prototype.uy="label-input-label";el.prototype.Aq=function(){qf(this.e,this.uy);if(!this.Pl()&&!this.mL)this.e.value=""};
el.prototype.f_=function(){this.qh=null;this.dt()};
el.prototype.L_=function(a){if(a.keyCode==27){if(a.type==gg)this.qh=this.e.value;else if(a.type==If)this.e.value=this.qh;else if(a.type==hg)this.qh=null;a.preventDefault()}};
el.prototype.P_=function(){if(!this.Pl()){this.e.value="";Rg(this.a_,10,this)}};
el.prototype.a_=function(){if(!this.Pl())this.e.value=this.cg};
el.prototype.n1=function(){this.dt()};
el.prototype.Pl=function(){return this.e.value!=""&&this.e.value!=this.cg};
el.prototype.clear=function(){this.e.value="";if(this.qh!=null)this.qh=""};
el.prototype.qa=function(a){if(this.qh!=null)this.qh=a;this.e.value=a;this.dt()};
el.prototype.L=function(){if(this.qh!=null)return this.qh;return this.Pl()?this.e.value:""};
el.prototype.dt=function(){this.cG();if(!this.Pl()){if(!this.mL)C(this.e,this.uy);Rg(this.Z6,10,this)}else qf(this.e,this.uy)};
el.prototype.pX=function(){var a=this.Pl();this.mL=true;this.e.focus();if(!a)this.e.value=this.cg;this.e.select();Rg(this.qX,10,this)};
el.prototype.qX=function(){this.mL=false};
el.prototype.Z6=function(){if(this.e&&!this.Pl())this.e.value=this.cg};var fl=function(a,b,c){D.call(this,a,b);this.item=c};
o(fl,D);var gl=function(a,b){this.j=new K(this);this.Sr(a);if(b)this.o9(b)};
o(gl,J);var hl="toggle_display";gl.prototype.e=null;gl.prototype.hG=true;gl.prototype.gG=null;gl.prototype.vc=false;gl.prototype.I9=false;gl.prototype.RL=-1;gl.prototype.vv=-1;gl.prototype.da=hl;gl.prototype.Xa=function(){return this.da};
gl.prototype.o9=function(a){this.da=a};
gl.prototype.A=function(){return this.e};
gl.prototype.Sr=function(a){this.lI();this.e=a};
gl.prototype.LO=function(a){this.lI();this.hG=a};
gl.prototype.lI=function(){if(this.vc)throw Error("Can not change this state of the popup while showing.");};
gl.prototype.Ja=function(){return this.vc};
gl.prototype.AL=function(){return this.vc||Ka()-this.vv<150};
gl.prototype.M=function(a){if(a){if(!this.e)throw Error("Caller must call setElement before trying to show thepopup");this.Wg()}else this.qf()};
gl.prototype.Qa=function(){};
gl.prototype.Wg=function(){if(this.vc)return;if(!this.pr())return;this.Qa();if(this.hG){var a=Ie(this.e);this.j.g(a,I,this.lw,true);if(s){var b=a.activeElement;while(b&&b.nodeName=="IFRAME"){try{var c=ef(b)}catch(d){break}a=c;b=a.activeElement}this.j.g(a,I,this.lw,true);this.j.g(a,"deactivate",this.SM)}else this.j.g(a,ig,this.SM)}if(this.da==hl){this.e.style.visibility="visible";M(this.e,true)}else if(this.da=="move_offscreen")this.Qa();this.vc=true;this.Bf()};
gl.prototype.qf=function(a){if(this.vc){if(!this.I4(a))return;if(this.j)this.j.ma();if(this.da==hl)if(this.I9)Rg(this.WK,0,this);else this.WK();else if(this.da=="move_offscreen")this.q4();this.vc=false;this.gk(a)}};
gl.prototype.WK=function(){this.e.style.visibility="hidden";M(this.e,false)};
gl.prototype.q4=function(){this.e.style.left="-200px";this.e.style.top="-200px"};
gl.prototype.pr=function(){return this.dispatchEvent("beforeshow")};
gl.prototype.Bf=function(){this.RL=Ka();this.vv=-1;this.dispatchEvent("show")};
gl.prototype.I4=function(a){return this.dispatchEvent({type:"beforehide",target:a})};
gl.prototype.gk=function(a){this.vv=Ka();this.dispatchEvent({type:"hide",target:a})};
gl.prototype.lw=function(a){if(!df(this.e,a.target)&&(!this.gG||df(this.gG,a.target)))this.qf(a.target)};
gl.prototype.SM=function(a){var b=Ie(this.e);if(s){var c=b.activeElement;if(c&&df(this.e,c))return}else if(a.target!=b)return;if(Ka()-this.RL<150)return;this.qf()};
gl.prototype.b=function(){if(!this.W()){gl.f.b.call(this);this.j.b();this.e=null;this.j=null}};var il=function(a,b){this.AN=5;this.Pi=b;gl.call(this,a)};
o(il,gl);il.prototype.gC=null;il.prototype.nZ=function(){return this.AN};
il.prototype.a9=function(a){this.AN=a;if(this.vc)this.Qa()};
il.prototype.nq=function(){return this.Pi};
il.prototype.setPosition=function(a){this.Pi=a;if(this.vc)this.Qa()};
il.prototype.Qa=function(){if(!this.Pi)return;if(!this.vc){this.e.style.visibility="hidden";M(this.e,true)}this.Pi.Qa(this.e,this.AN,this.gC);if(!this.vc)M(this.e,false)};
var ll=function(a,b,c,d,e,f,g){var h=uh(c),i=uh(a),k=new ge(i.x-h.x+c.offsetLeft,i.y-h.y+c.offsetTop),n=1,q=1,t=Ah(a),y=jl(a,b);switch(y){case 1:break;case 2:k.x+=t.width;n=-1;break;case 3:k.y+=t.height;q=-1;break;case 4:k.x+=t.width;k.y+=t.height;n=-1;q=-1;break}if(e){k.x+=n*e.x;k.y+=q*e.y}return kl(k,c,d,f,g)},
kl=function(a,b,c,d,e){var f=jl(b,c);if(d||f!=1){var g=Ah(b);switch(f){case 1:a.x+=d.left;a.y+=d.top;break;case 2:a.x-=g.width;if(d){a.x-=d.right;a.y+=d.top}break;case 3:a.y-=g.height;if(d){a.x+=d.left;a.y-=d.bottom}break;case 4:a.x-=g.width;a.y-=g.height;if(d){a.x-=d.right;a.y-=d.bottom}break}}var h=e||0;if(h!=0){var i=Ie(b),k=Te(i)||window,n=Ah(b),q=Qe(k),t=Se(k),y=a.x-t.x,z=q.width-y,O=a.y-t.y,aa=q.height-O;if(n.width>z)if(h&1)a.x-=n.width-z;else if(h&2)return false;if(n.height>aa)if(h&4)a.y-=
n.height-aa;else if(h&8)return false;if(a.x-t.x<0)if(h&1)a.x-=a.x-t.x;else if(h&2)return false;if(a.y-t.y<0)if(h&4)a.y-=a.y-t.y;else if(h&8)return false}sh(b,a);return true},
jl=function(a,b){var c=Fh(a);switch(b){case 1:case 2:case 3:case 4:return b;case 5:return c?2:1;case 6:return c?1:2;case 7:return c?4:3;case 8:return c?3:4}},
ml=function(a){switch(a){case 1:return 2;case 2:return 1;case 3:return 4;case 4:return 3;case 5:return 6;case 6:return 5;case 7:return 8;case 8:return 7}},
nl=function(a){switch(a){case 1:return 3;case 2:return 4;case 3:return 1;case 4:return 2;case 5:return 7;case 6:return 8;case 7:return 5;case 8:return 6}};
var ol=function(){};
ol.prototype.Qa=function(){};
var pl=function(a,b){this.element=a;this.corner=b};
o(pl,ol);pl.prototype.Qa=function(a,b,c){ll(this.element,this.corner,a,b,null,c)};
var ql=function(a,b,c){pl.call(this,a,b);this.Xaa=c||false};
o(ql,pl);ql.prototype.Qa=function(a,b,c){var d=ll(this.element,this.corner,a,b,null,c,10);if(!d){d=ll(this.element,b,a,this.corner,null,c,10);if(!d)if(this.Xaa)d=ll(this.element,this.corner,a,b,null,c,5);else ll(this.element,this.corner,a,b,null,c,0)}};
var rl=function(a,b){this.coordinate=a instanceof ge?a:new ge(a,b)};
o(rl,ol);rl.prototype.Qa=function(a,b,c){kl(this.coordinate,a,b,c)};
var sl=function(a,b){this.coordinate=a instanceof ge?a:new ge(a,b)};
o(sl,ol);sl.prototype.Qa=function(a,b,c){var d=th(a);ll(d,1,a,b,this.coordinate,c)};
var tl=function(a,b){this.coordinate=a instanceof ge?a:new ge(a,b)};
o(tl,ol);tl.prototype.Qa=function(a,b,c){var d=th(a),e=new ge(this.coordinate.x+d.scrollLeft,this.coordinate.y+d.scrollTop);ll(d,1,a,b,e,c)};
var ul=function(a,b){tl.call(this,a,b)};
o(ul,tl);ul.prototype.Qa=function(a,b,c){var d=th(a),e=new ge(this.coordinate.x+d.scrollLeft,this.coordinate.y+d.scrollTop),f=10;if(ll(d,1,a,b,e,c,f))return;if(ll(d,1,a,nl(b),e,c,f))return;if(ll(d,1,a,ml(b),e,c,f))return;if(ll(d,1,a,nl(ml(b)),e,c,f))return;ll(d,1,a,b,e,c)};var vl=function(a){il.call(this,a);this.p=new K(this)};
o(vl,il);var wl="itemaction";vl.prototype.b=function(){vl.f.b.call(this);this.p.b()};
vl.prototype.Bf=function(){vl.f.Bf.call(this);this.p.g(this.e,Gf,this.vr);this.p.g(this.e,Hf,this.sr);this.p.g(this.e,I,this.EC);this.p.g(this.e,eg,this.wr);this.p.g(this.e,s?gg:If,this.qr)};
vl.prototype.gk=function(a){vl.f.gk.call(this,a);this.p.ma()};
vl.prototype.pe=function(){};
vl.prototype.ib=function(){};
vl.prototype.vr=function(){};
vl.prototype.sr=function(){};
vl.prototype.EC=function(){};
vl.prototype.wr=function(){};
vl.prototype.qr=function(){};var xl=function(a){vl.call(this,a)};
o(xl,vl);xl.prototype.Ub=null;xl.prototype.FL="menu-item";xl.prototype.BO="menu-item-selected";xl.prototype.wv=Ka();xl.prototype.b=function(){xl.f.b.call(this);this.Ub=null};
xl.prototype.wJ=function(){return this.FL};
xl.prototype.pe=function(){return this.Ub};
xl.prototype.ib=function(a){if(this.Ub)qf(this.Ub,this.BO);this.Ub=a;if(this.Ub){C(this.Ub,this.BO);var b=this.Ub.offsetTop,c=this.Ub.offsetHeight,d=this.e.scrollTop,e=this.e.offsetHeight;if(b<d)this.e.scrollTop=b;else if(b+c>d+e)this.e.scrollTop=b+c-e}};
xl.prototype.Bf=function(){xl.f.Bf.call(this);s?this.e.firstChild.focus():this.e.focus()};
xl.prototype.pu=function(a){var b=this.e.getElementsByTagName("*"),c=b.length,d;if(this.Ub)for(var e=0;e<c;e++)if(b[e]==this.Ub){d=a?e-1:e+1;break}if(!pa(d))d=a?c-1:0;for(var e=0;e<c;e++){var f=a?-1:1,g=d+f*e%c;if(g<0)g+=c;else if(g>=c)g-=c;if(this.JB(b[g]))return b[g]}return null};
xl.prototype.vr=function(a){var b=this.In(a.target);if(b==null)return;if(Ka()-this.wv>150)this.ib(b)};
xl.prototype.sr=function(a){var b=this.In(a.target);if(b==null)return;if(Ka()-this.wv>150)this.ib(null)};
xl.prototype.EC=function(a){a.preventDefault()};
xl.prototype.wr=function(a){var b=this.In(a.target);if(b==null)return;this.M(false);this.UM(b)};
xl.prototype.qr=function(a){switch(a.keyCode){case 40:this.ib(this.pu(false));this.wv=Ka();break;case 38:this.ib(this.pu(true));this.wv=Ka();break;case 13:if(this.Ub){this.UM();this.M(false)}break;case 27:this.M(false);break;default:if(a.charCode){var b=String.fromCharCode(a.charCode);this.z7(b,1,true)}break}a.preventDefault();this.dispatchEvent(a)};
xl.prototype.z7=function(a,b,c){var d=this.e.getElementsByTagName("*"),e=d.length,f;if(e==0)return;if(!this.Ub||(f=cc(d,this.Ub))==-1)f=0;var g=f,h=new RegExp("^"+vb(a),"i"),i=c&&this.Ub,k=b||1;do{if(d[f]!=i&&this.JB(d[f])){var n=lf(d[f]);if(n.match(h))break}f+=k;if(f==e)f=0;else if(f<0)f=e-1}while(f!=g);if(this.Ub!=d[f])this.ib(d[f])};
xl.prototype.UM=function(a){this.dispatchEvent(new fl(wl,this,a||this.Ub))};
xl.prototype.JB=function(a){return sf(a,this.FL)};
xl.prototype.In=function(a){var b=Ie(a).body;while(a!=null&&a!=b){if(this.JB(a))return a;a=a.parentNode}return null};var yl=function(a,b){var c=a||"menu",d=b||Ke().body;this.e=v("div",{tabIndex:0,"class":c});Dk(this.e,"menu");Ek(this.e,"haspopup",true);d.appendChild(this.e);xl.call(this,this.e);this.hg=null;this.ta=[]};
o(yl,xl);yl.prototype.zn=null;yl.prototype.jO=null;yl.prototype.Nm=10;yl.prototype.getZIndex=function(){return this.Nm};
yl.prototype.add=function(a){var b=this.A();if(!b)throw Error("setElement() called before create()");if(a.Lj())throw Error("Menu item already added to a menu");a.kP(this);this.ta.push(a);b.appendChild(a.create())};
yl.prototype.remove=function(a){a.remove();a.kP(null);tc(this.ta,a)};
yl.prototype.focus=function(){this.e.focus()};
yl.prototype.W8=function(a){this.hg=a;this.tx(a)};
yl.prototype.JO=function(a,b,c){if(this.zn)H(this.zn);if(this.Ts!=a){this.SG=false;this.vv=-1}var d=c||E;this.zn=F(a,d,this.ro,false,this);this.jO=F(window,mg,this.yr,false,this);this.setPosition(new pl(a,b||7));this.Ts=a};
yl.prototype.b=function(){if(!this.W()){for(var a=0;a<this.ta.length;a++)this.ta[a].b();H(this.zn);H(this.jO);A(this.e);this.e=null;this.Ts=null;yl.f.b.call(this)}};
yl.prototype.M=function(a,b){if(this.AL()&&a)return;if(a==false){if(this.Kb){this.Kb.jt();xl.prototype.ib.call(this,null)}if(this.hg&&!b)this.hg.focus()}else{if(this.hg)this.Nm=this.hg.getZIndex()+1;this.e.style.zIndex=this.Nm}if(b&&this.hg)this.hg.M(a,b);if(this.hj){window.clearTimeout(this.hj);this.hj=null}this.Kb=null;gl.prototype.M.call(this,a)};
yl.prototype.Qh=function(a){this.ib(a==-1?null:this.e.childNodes[a])};
yl.prototype.sv=function(a,b){if(a||!this.Kb||!this.Kb.TK())xl.prototype.ib.call(this,a);var c=a?this.Kn(a):null;if(c&&c!=this.Kb){if(b&&this.Kb){this.Kb.jt();this.Kb=null}if(this.hj){window.clearTimeout(this.hj);this.hj=null}if(!b)this.hj=window.setTimeout(l(this.Mh,this,c),300);if(this.hg){this.hg.ib(this.Ts);this.e.focus()}}};
yl.prototype.ib=function(a){var b,c;if(!a){b=null;c=null}else if(typeof a.nodeType!="undefined"){b=a;c=this.Kn(b)}else{c=a;b=c.e}if(b||!this.Kb||!this.Kb.TK())xl.prototype.ib.call(this,b);if(c==this.Kb)return;if(this.Kb&&b)this.Kb.jt();if(b){var c=this.Kn(b);if(c.Qn()){c.FC();c.sq().focus()}this.Kb=c}};
yl.prototype.pe=function(){return this.Ub?this.ta[cc(this.e.childNodes,this.Ub)]:null};
yl.prototype.Mh=function(a){if(this.hj){window.clearTimeout(this.hj);this.hj=null}var b=this.Kn(this.Ub);if(b!=a)return;if(this.Kb&&a)this.Kb.jt();if(a.Qn()){a.FC();a.sq().focus()}else this.e.focus();this.Kb=a};
yl.prototype.Ey=function(a){var b=this.Kn(a);if(b.Qn()){b.FC();var c=b.sq();c.focus();this.Kb=b}else{this.M(false,true);this.dispatchEvent(new fl(wl,this,b))}};
yl.prototype.ro=function(){if(!this.SG)this.M(true);this.SG=false};
yl.prototype.gH=function(a){if(df(this.e,a))return true;if(this.Kb&&this.Kb.Qn())return this.Kb.sq().gH(a);return false};
yl.prototype.lw=function(a){if(this.Ts==a.target||df(this.Ts,a.target))this.SG=true;var b=this;while(b.hg)b=b.hg;if(!b.gH(a.target))this.qf()};
yl.prototype.vr=function(a){var b=this.In(a.target);if(b==null)return;this.sv(b)};
yl.prototype.sr=function(a){var b=this.In(a.target);if(b==null)return;this.sv(null)};
yl.prototype.wr=function(a){var b=this.In(a.target);if(b==null)return;this.Ey(b)};
yl.prototype.yr=function(){if(!this.W()&&this.Ja())this.Qa()};
yl.prototype.qr=function(a){var b=false;switch(a.keyCode){case 37:if(this.hg)this.M(false);b=true;break;case 39:var c=this.Kn(this.Ub);if(this.Ub&&c.Qn()){this.Ey(this.Ub);c.sq().Qh(0)}b=true;break;case 40:this.sv(this.pu(false),true);b=true;break;case 38:this.sv(this.pu(true),true);b=true;break;case 13:if(this.Ub)this.Ey(this.Ub);b=true;break;case 27:this.M(false);b=true;break}if(b)a.preventDefault()};
yl.prototype.Bf=function(){yl.f.Bf.call(this);this.ib(null);var a=Fh(this.e);if(a)C(this.e,"goog-rtl");else qf(this.e,"goog-rtl");if(!this.hg)this.e.focus()};
yl.prototype.Kn=function(a){var b=-1;for(var c=a;c;c=bf(c))b++;return b==-1?null:this.ta[b]};
var zl=function(a,b,c){this.pb=String(a);this.Ua=b||a;this.ng=c||null;this.R=null;this.e=null};
zl.prototype.Hj=function(){return this.pb};
zl.prototype.L=function(){return this.Ua};
zl.prototype.Sg=function(a){this.pb=a;if(this.e)this.e.firstChild.nodeValue=a};
zl.prototype.qa=function(a){this.Ua=a};
zl.prototype.b=function(){this.remove();if(this.ng)this.ng.b()};
zl.prototype.kP=function(a){this.R=a;if(this.ng)this.ng.W8(a)};
zl.prototype.Lj=function(){return this.R};
zl.prototype.create=function(){if(!this.R)throw Error("MenuItem is not attached to a menu");var a,b;if(this.ng){b=v("span",{"class":"goog-menu-arrow-right"},"\u25b6");a=v("span",{"class":"goog-menu-arrow-left"},"\u25c0")}this.e=v("div",{"class":this.R.wJ()},this.pb,a,b);return this.e};
zl.prototype.remove=function(){A(this.e);this.e=null};
zl.prototype.Qn=function(){return this.ng!=null};
zl.prototype.TK=function(){return this.Qn()?this.ng.AL():false};
zl.prototype.sq=function(){return this.ng};
zl.prototype.FC=function(){if(this.ng){var a=this.ng,b=a.nZ();switch(b){case 1:b=2;break;case 2:b=1;break;case 5:b=6;break;case 6:b=5;break}a.JO(this.e,b);a.M(true)}};
zl.prototype.jt=function(){if(this.ng)this.ng.M(false)};
var Al=function(){zl.call(this,null)};
o(Al,zl);Al.prototype.create=function(){if(!this.R)throw Error("MenuSeparator is not attached to a menu");this.e=w("hr");Dk(this.e,"separator");return this.e};var Bl=function(){return Fk.call(this)};
o(Bl,Fk);var Cl=null,Dl=function(){return Cl||(Cl=new Bl)},
El="goog-menuseparator";Bl.prototype.m=function(a){return a.K.m("div",{"class":this.O()})};
Bl.prototype.ia=function(a,b){if(b.tagName=="HR"){var c=b;b=this.m(a);Xe(b,c);A(c)}else C(b,this.O());return b};
Bl.prototype.hc=function(){};
Bl.prototype.O=function(){return El};var Fl=function(a,b){Q.call(this,null,a||Dl(),b);this.Sh(1,false);this.Sh(2,false);this.Sh(4,false);this.Sh(32,false);this.YD(1)};
o(Fl,Q);Fl.prototype.D=function(){return"goog.ui.Separator"};
Fl.prototype.n=function(){Fl.f.n.call(this);Dk(this.A(),"separator")};
Kk(El,function(){return new Fl});var Gl=function(){},
Hl=null;Gl.prototype.Pu=function(a){return a&&a.tabIndex>=1};
Gl.prototype.Cj=function(a,b){if(a)a.tabIndex=b?Math.max(a.tabIndex,1):-1};
Gl.prototype.m=function(a){return a.K.m("div",{"class":this.si(a).join(" ")})};
Gl.prototype.Oa=function(a){return a};
Gl.prototype.kb=function(a){return a.tagName=="DIV"};
Gl.prototype.ia=function(a,b){if(b.id)a.kx(b.id);this.PV(a,b);var c=this.O(),d=false,e=pf(b);if(e)r(e,function(f){if(f==c)d=true;else if(f==c+"-disabled")a.Ca(false);else if(f==c+"-horizontal")a.sx(Il);else if(f==c+"-vertical")a.sx(Jl)});
if(!d)C(b,c);return b};
Gl.prototype.PV=function(a,b){if(b){var c=b.firstChild,d;while(c&&c.parentNode==b){d=c.nextSibling;if(c.nodeType==1){var e=this.tl(c);if(e){a.Z(e);e.ia(c)}}else if(!c.nodeValue||Va(c.nodeValue)=="")b.removeChild(c);c=d}}};
Gl.prototype.tl=function(a){return Lk(a)};
Gl.prototype.sf=function(a){var b=a.A();Hh(b,true,se);Dk(b,"container");if(a.Nn()==Il&&se&&!Ee("1.9a")&&a.Xn()){var c=this.Oa(b);a.sh(function(d){var e=d.A();if(e&&e.parentNode==c)Kl(d)})}};
Gl.prototype.$b=function(a){return a.A()};
Gl.prototype.O=function(){return"goog-container"};
Gl.prototype.si=function(a){var b=this.O(),c=a.Nn()==Il,d=[b,b+(c?"-horizontal":"-vertical")];if(!a.sb())d.push(this.O()+"-disabled");return d};
Gl.prototype.DA=function(){return Jl};
var Kl=function(a){var b=a.A();if(b){var c=b.parentNode,d=b.nextSibling,e=a instanceof Fl?"display:-moz-box;position:relative;top:4px":"display:-moz-box;position:relative";c.insertBefore(a.K.m("div",{style:e},b),d)}},
Ll=function(a){var b=a.A();if(b){var c=b.parentNode;c.parentNode.insertBefore(b,c);A(c)}};var S=function(a,b,c){P.call(this,c);this.pa=b||Hl||(Hl=new Gl);this.Hb=a||this.pa.DA()};
o(S,P);var Il="horizontal",Jl="vertical";S.prototype.j=null;S.prototype.U2=null;S.prototype.ob=null;S.prototype.pa=null;S.prototype.Hb=null;S.prototype.la=true;S.prototype.Ea=true;S.prototype.pA=true;S.prototype.zc=-1;S.prototype.$c=null;S.prototype.qC=false;S.prototype.Dg=function(){return this.j||(this.j=new K(this))};
S.prototype.$b=function(){return this.U2||this.pa.$b(this)};
S.prototype.hq=function(){return this.ob||(this.ob=new Dg(this.$b()))};
S.prototype.Tf=function(){return this.pa};
S.prototype.D=function(){return"goog.ui.Container"};
S.prototype.m=function(){this.od(this.pa.m(this))};
S.prototype.Oa=function(){return this.pa.Oa(this.A())};
S.prototype.kb=function(a){return this.pa.kb(a)};
S.prototype.$=function(a){this.od(this.pa.ia(this,a));if(a.style.display=="none")this.la=false};
S.prototype.n=function(){S.f.n.call(this);var a=this.A();this.pa.sf(this);this.M(this.la,true);var b=this.Dg();b.g(this,"enter",this.Gu);b.g(this,"highlight",this.b0);b.g(this,"unhighlight",this.i1);b.g(this,"open",this.I0);b.g(this,"close",this.k_);b.g(a,I,this.Ud);b.g(Ie(a),eg,this.Ll);if(this.lv())this.eI(true)};
S.prototype.eI=function(a){var b=this.Dg(),c=this.$b();if(a){b.g(c,jg,this.zq);b.g(c,ig,this.Hg);b.g(this.hq(),"key",this.Jc)}else{b.Sa(c,jg,this.zq);b.Sa(c,ig,this.Hg);b.Sa(this.hq(),"key",this.Jc)}};
S.prototype.u=function(){S.f.u.call(this);this.be(-1);if(this.j)this.j.ma();if(this.$c)this.$c.Jb(false);this.qC=false};
S.prototype.b=function(){if(!this.W()){S.f.b.call(this);if(this.j){this.j.b();this.j=null}if(this.ob){this.ob.b();this.ob=null}this.$c=null;this.pa=null}};
S.prototype.Gu=function(a){var b=a.target;if(b&&this.$s(b)){b.Ab(true);if(this.O2())b.Fd(true)}return false};
S.prototype.b0=function(a){var b=this.Un(a.target);if(b>-1&&b!=this.zc){var c=this.wl();if(c)c.Ab(false);this.zc=b;c=this.wl();if(this.$c&&c!=this.$c)if(c.wf(64))c.Jb(true);else this.$c.Jb(false);Ek(this.A(),"activedescendent",c.A().id)}};
S.prototype.i1=function(a){if(a.target==this.wl()){this.zc=-1;Ek(this.A(),"activedescendent",null)}};
S.prototype.I0=function(a){var b=a.target;if(b&&b!=this.$c&&b.Ia()==this){if(this.$c)this.$c.Jb(false);this.$c=b}};
S.prototype.k_=function(a){if(a.target==this.$c)this.$c=null};
S.prototype.Ud=function(a){if(this.Ea)this.zm(true);var b=this.$b();if(this.pa.Pu(b))b.focus();else a.preventDefault()};
S.prototype.Ll=function(){this.zm(false)};
S.prototype.zq=function(){};
S.prototype.Hg=function(){this.be(-1);this.zm(false);if(this.$c)this.$c.Jb(false)};
S.prototype.Jc=function(a){if(!this.Ea||this.Ag()==0)return false;var b=this.wl();if(b&&typeof b.Jc=="function"&&b.Jc(a))return true;if(this.$c&&this.$c!=b&&typeof this.$c.Jc=="function"&&this.$c.Jc(a))return true;switch(a.keyCode){case 27:if(this.lv())this.$b().blur();else return false;break;case 36:this.N1();break;case 35:this.O1();break;case 38:if(this.Hb==Jl)this.sB();else return false;break;case 37:if(this.Hb==Il)if(this.Xn())this.rB();else this.sB();else return false;break;case 40:if(this.Hb==
Jl)this.rB();else return false;break;case 39:if(this.Hb==Il)if(this.Xn())this.sB();else this.rB();else return false;break;default:return false}a.preventDefault();return true};
S.prototype.mp=function(a,b,c){a.HD(2,true);a.HD(64,true);a.Sh(32,false);S.f.mp.call(this,a,b,c);if(c&&this.Ha()&&this.Xn()&&this.Nn()==Il&&se&&!Ee("1.9a"))Kl(a);if(b<=this.zc)this.zc++};
S.prototype.removeChild=function(a,b){var c=this.Un(a);if(c!=-1)if(c==this.zc)a.Ab(false);else if(c<this.zc)this.zc--;if(b&&this.Ha()&&this.Xn()&&this.Nn()==Il&&se&&!Ee("1.9a"))Ll(a);return S.f.removeChild.call(this,a,b)};
S.prototype.Nn=function(){return this.Hb};
S.prototype.sx=function(a){if(this.A())throw Error(zk);this.Hb=a};
S.prototype.Ja=function(){return this.la};
S.prototype.M=function(a,b){if(b||this.la!=a&&this.dispatchEvent(a?"show":"hide")){this.la=a;var c=this.A();if(c){M(c,a);if(this.lv())this.pa.Cj(this.$b(),this.Ea&&this.la)}return true}return false};
S.prototype.sb=function(){return this.Ea};
S.prototype.Ca=function(a){if(this.Ea!=a&&this.dispatchEvent(a?wk:vk)){if(a){this.Ea=true;this.sh(function(b){if(b.wasDisabled)delete b.wasDisabled;else b.Ca(true)})}else{this.sh(function(b){if(b.sb())b.Ca(false);
else b.wasDisabled=true});
this.Ea=false;this.zm(false)}if(this.lv())this.pa.Cj(this.$b(),a&&this.la)}};
S.prototype.lv=function(){return this.pA};
S.prototype.MD=function(a){if(a!=this.pA&&this.Ha())this.eI(a);this.pA=a;if(this.Ea&&this.la)this.pa.Cj(this.$b(),a)};
S.prototype.be=function(a){var b=this.wd(a);if(b)b.Ab(true);else if(this.zc>-1)this.wl().Ab(false)};
S.prototype.Ab=function(a){this.be(this.Un(a))};
S.prototype.wl=function(){return this.wd(this.zc)};
S.prototype.N1=function(){this.Vu(function(a,b){return(a+1)%b},
this.Ag()-1)};
S.prototype.O1=function(){this.Vu(function(a,b){a--;return a<0?b-1:a},
0)};
S.prototype.rB=function(){this.Vu(function(a,b){return(a+1)%b},
this.zc)};
S.prototype.sB=function(){this.Vu(function(a,b){a--;return a<0?b-1:a},
this.zc)};
S.prototype.Vu=function(a,b){var c=b<0?this.Un(this.$c):b,d=this.Ag();c=a(c,d);var e=0;while(e<=d){var f=this.wd(c);if(f&&this.$s(f)){this.be(c);return true}e++;c=a(c,d)}return false};
S.prototype.$s=function(a){return a.sb()&&a.wf(2)};
S.prototype.O2=function(){return this.qC};
S.prototype.zm=function(a){this.qC=a};var Ml=function(){Fk.call(this)};
o(Ml,Fk);var Nl=null;Ml.prototype.m=function(a){var b=Ml.f.m.call(this,a);this.Bm(a,b,a.wf(8)||a.wf(16));return b};
Ml.prototype.Oa=function(a){return this.RK(a)?a.lastChild:a};
Ml.prototype.ia=function(a,b){var c=sf(b,"goog-option");a.QO(c);b=Ml.f.ia.call(this,a,b);this.Bm(a,b,c);return b};
Ml.prototype.Bm=function(a,b,c){var d=this.O()+"-checkbox",e=this.RK(b);if(c&&!e){var f=a.K.m("div"),g,h=this.Oa(b).firstChild;while(g=h){h=g.nextSibling;f.appendChild(g)}b.innerHTML="";b.appendChild(a.K.m("div",{"class":d}));b.appendChild(f)}else if(!c&&e){var g,h=this.Oa(b).firstChild,i=b.lastChild;while(g=h){h=g.nextSibling;b.appendChild(g)}var k;for(g=i;g;g=k){k=g.previousSibling;b.removeChild(g)}}tf(b,"goog-option",c)};
Ml.prototype.Bg=function(a){switch(a){case 2:return this.O()+"-highlight";case 16:case 8:return"goog-option-selected";default:return Ml.f.Bg.call(this,a)}};
Ml.prototype.xu=function(a){return a=="goog-option-selected"?16:Ml.f.xu.call(this,a)};
Ml.prototype.O=function(){return"goog-menuitem"};
Ml.prototype.RK=function(a){var b=this.O()+"-checkbox";return a&&a.firstChild&&a.firstChild.nodeType==1&&a.firstChild.className==b};var Ol=function(a,b,c,d){Q.call(this,a,d||Nl||(Nl=new Ml),c);this.qa(b)};
o(Ol,Q);Ol.prototype.D=function(){return"goog.ui.MenuItem"};
Ol.prototype.L=function(){var a=this.na();return a!=null?a:this.Hj()};
Ol.prototype.qa=function(a){this.U(a)};
Ol.prototype.Bm=function(a){this.Sh(8,a);if(this.ak()&&!a)this.rm(false);var b=this.A();if(b)this.pa.Bm(this,b,a)};
Ol.prototype.QO=function(a){this.Sh(16,a);var b=this.A();if(b)this.pa.Bm(this,b,a)};
Kk("goog-menuitem",function(){return new Ol(null)});var Pl=function(a,b,c){Ol.call(this,a,b,c);this.QO(true)};
o(Pl,Ol);Pl.prototype.D=function(){return"goog.ui.CheckBoxMenuItem"};
var Ql=function(){return new Pl(null)};
Kk("goog-checkbox-menuitem",Ql);var Rl=function(a,b,c){Ol.call(this,a,b,c);this.Bm(true)};
o(Rl,Ol);Rl.prototype.D=function(){return"goog.ui.Option"};
Rl.prototype.Oi=function(){return this.dispatchEvent(xk)};
var Sl=function(){return new Rl(null)};
Kk("goog-option",Sl);var Tl=function(a){Fl.call(this,Dl(),a)};
o(Tl,Fl);Tl.prototype.D=function(){return"goog.ui.MenuSeparator"};
Kk(El,function(){return new Fl});var Ul=function(){Gl.call(this)};
o(Ul,Gl);var Vl=null;Ul.prototype.kb=function(a){return a.tagName=="UL"||Ul.f.kb.call(this,a)};
Ul.prototype.tl=function(a){return a.tagName=="HR"?new Fl:Ul.f.tl.call(this,a)};
Ul.prototype.fh=function(a,b){return df(a.A(),b)};
Ul.prototype.O=function(){return"goog-menu"};
Ul.prototype.sf=function(a){Ul.f.sf.call(this,a);var b=a.A();Dk(b,"menu");Ek(b,"haspopup","true")};var Wl=function(a,b){S.call(this,Jl,b||Vl||(Vl=new Ul),a);this.MD(false)};
o(Wl,S);Wl.prototype.Ss=true;Wl.prototype.hT=false;Wl.prototype.D=function(){return"goog.ui.Menu"};
Wl.prototype.O=function(){return this.Tf().O()};
Wl.prototype.fh=function(a){return this.Tf().fh(this,a)||this.kB()&&ic(this.Va,function(b){return typeof b.fh=="function"&&b.fh(a)})};
Wl.prototype.ub=function(a){this.Z(a,true)};
Wl.prototype.bh=function(a,b){this.mp(a,b,true)};
Wl.prototype.kg=function(a){var b=this.removeChild(a,true);if(b)b.b()};
Wl.prototype.Kh=function(a){var b=this.SN(a,true);if(b)b.b()};
Wl.prototype.vh=function(a){return this.wd(a)};
Wl.prototype.yl=function(){return this.Ag()};
Wl.prototype.nu=function(){return this.Va||[]};
Wl.prototype.setPosition=function(a,b){var c=this.Ja();if(!c)M(this.e,true);yh(this.e,a,b);if(!c)M(this.e,false)};
Wl.prototype.nq=function(){return this.Ja()?uh(this.e):null};
Wl.prototype.IO=function(a){this.Ss=a;if(a)this.MD(true)};
Wl.prototype.$X=function(){return this.Ss};
Wl.prototype.M=function(a,b){var c=Wl.f.M.call(this,a,b);if(c&&a&&this.Ha()&&this.Ss)this.$b().focus();return c};
Wl.prototype.Gu=function(a){if(this.Ss)this.$b().focus();return Wl.f.Gu.call(this,a)};
Wl.prototype.$s=function(a){return(this.hT||a.sb())&&a.Ja()&&a.wf(2)};var Xl=function(a){Wl.call(this,a);this.IO(true);this.Ee=new dd};
o(Xl,Wl);Xl.prototype.la=false;Xl.prototype.N$=false;Xl.prototype.NL=0;Xl.prototype.xH=null;Xl.prototype.$=function(a){Xl.f.$.call(this,a);var b=a.getAttribute("for")||a.htmlFor;if(b)this.rc(this.K.A(b),3)};
Xl.prototype.n=function(){Xl.f.n.call(this);ad(this.Ee,this.Us,this);this.j.g(this,xk,this.QM);this.j.g(this.K.Ic(),I,this.kw,true);if($b)this.j.g(this.K.Ic(),ng,this.kw,true)};
Xl.prototype.u=function(){Xl.f.u.call(this);ad(this.Ee,this.zt,this);this.j.Sa(this,xk,this.QM);this.j.Sa(this.K.Ic(),I,this.kw,true);if($b)this.j.Sa(this.K.Ic(),ng,this.kw,true)};
Xl.prototype.rc=function(a,b,c,d,e){var f=Ga(a);if(this.Ee.Ob(f))throw Error("Can not attach menu to the same element more than once.");var g={e:a,B$:b,bda:c,hA:d?ng:I,gC:e};this.Ee.J(f,g);if(this.Ha())this.Us(g)};
Xl.prototype.Us=function(a){this.j.g(a.e,a.hA,this.ow)};
Xl.prototype.detach=function(a){var b=Ga(a);if(!this.Ee.Ob(b))throw Error("Menu not attached to provided element, unable to detach.");if(this.Ha())this.zt(this.Ee.get(b));this.Ee.remove(b)};
Xl.prototype.zt=function(a){this.j.Sa(a.e,a.hA,this.ow)};
Xl.prototype.Xo=function(a,b,c){var d=this.Ja();if((d||this.eR())&&this.N$){this.hide();return}var e=a.B$?new ql(a.e,a.B$):new ul(b,c),f=a.bda||5;if(!d)this.e.style.visibility="hidden";M(this.e,true);e.Qa(this.e,f,a.gC);if(!d)this.e.style.visibility="visible";this.xH=a.e;this.be(-1);this.M(true)};
Xl.prototype.hide=function(){this.M(false);if(!this.Ja()){this.NL=Ka();this.xH=null}};
Xl.prototype.eR=function(){return Ka()-this.NL<50};
Xl.prototype.QM=function(){this.hide()};
Xl.prototype.ow=function(a){var b=this.Ee.tc();for(var c=0;c<b.length;c++){var d=this.Ee.get(b[c]);if(d.e==a.currentTarget){this.Xo(d,a.clientX,a.clientY);a.preventDefault();a.stopPropagation();return}}};
Xl.prototype.kw=function(a){if(this.Ja()&&!this.fh(a.target))this.hide()};
Xl.prototype.Hg=function(a){Xl.f.Hg.call(this,a);this.hide()};
Xl.prototype.b=function(){if(!this.W()){Xl.f.b.call(this);if(this.Ee){this.Ee.clear();this.Ee=null}}};var Yl=function(){Hk.call(this)};
o(Yl,Hk);var Zl=null,$l=function(){return Zl||(Zl=new Yl)};
Yl.prototype.m=function(a){var b=this.si(a),c={"class":"goog-inline-block "+b.join(" "),title:a.Qj()||""};return a.K.m("div",c,a.Vc())};
Yl.prototype.kb=function(a){return a.tagName=="DIV"};
Yl.prototype.ia=function(a,b){C(b,"goog-inline-block");return Yl.f.ia.call(this,a,b)};
Yl.prototype.L=function(){return null};
Yl.prototype.O=function(){return"goog-flat-button"};
var am=function(){return new Mk(null,$l())};
Kk("goog-flat-button",am);var bm=function(){Yl.call(this)};
o(bm,Yl);var cm=null;var dm="goog-flat-menu-button";bm.prototype.m=function(a){var b=this.si(a),c={"class":"goog-inline-block "+b.join(" "),title:a.Qj()||""};return a.K.m("div",c,[this.createCaption(a.Vc(),a.K),this.Gp(a.K)])};
bm.prototype.Oa=function(a){return a&&a.firstChild};
bm.prototype.ia=function(a,b){var c=Ne("*","goog-menu",b)[0];if(c){M(c,false);a.K.Ic().body.appendChild(c);var d=new Wl;d.ia(c);a.uk(d)}b.appendChild(this.createCaption(b.childNodes,a.K));b.appendChild(this.Gp(a.K));return bm.f.ia.call(this,a,b)};
bm.prototype.createCaption=function(a,b){return b.m("div",{"class":"goog-inline-block "+this.O()+"-caption"},a)};
bm.prototype.Gp=function(a){return a.m("div",{"class":"goog-inline-block "+this.O()+"-dropdown"},"\u00a0")};
bm.prototype.O=function(){return dm};
var fm=function(){return new em(null,null,cm||(cm=new bm))};
Kk(dm,fm);var gm=function(){Nk.call(this)};
o(gm,Nk);var hm=null;gm.prototype.Oa=function(a){return gm.f.Oa.call(this,a&&a.firstChild)};
gm.prototype.ia=function(a,b){var c=Ne("*","goog-menu",b)[0];if(c){M(c,false);x(Ie(c).body,c);var d=new Wl;d.ia(c);a.uk(d)}return gm.f.ia.call(this,a,b)};
gm.prototype.oj=function(a,b){return gm.f.oj.call(this,[this.createCaption(a,b),this.Gp(b)],b)};
gm.prototype.createCaption=function(a,b){return b.m("div",{"class":"goog-inline-block "+this.O()+"-caption"},a)};
gm.prototype.Gp=function(a){return a.m("div",{"class":"goog-inline-block "+this.O()+"-dropdown"},"\u00a0")};
gm.prototype.O=function(){return"goog-menu-button"};var em=function(a,b,c,d){Mk.call(this,a,c||hm||(hm=new gm),d);this.Sh(64,true);this.uk(b)};
o(em,Mk);em.prototype.R=null;em.prototype.XF=true;em.prototype.pB=true;em.prototype.D=function(){return"goog.ui.MenuButton"};
em.prototype.n=function(){em.f.n.call(this);if(this.R)this.Dg().g(this.R,xk,this.xi)};
em.prototype.u=function(){em.f.u.call(this);if(this.R){this.Jb(false);this.R.u();var a=this.R.A();if(a)A(a)}};
em.prototype.b=function(){if(!this.W()){em.f.b.call(this);if(this.R){this.R.b();this.R=null}}};
em.prototype.Ud=function(a){em.f.Ud.call(this,a);if(this.zd()){this.Jb(!this.isOpen());if(this.R)this.R.zm(this.isOpen())}};
em.prototype.Ll=function(a){em.f.Ll.call(this,a);if(this.R){if(!this.zd())this.R.zm(false);if(this.R.$X()&&this.R.$b())this.R.$b().focus()}};
em.prototype.Oi=function(){this.Fd(false);return true};
em.prototype.vK=function(a){if(this.R&&this.R.Ja()&&!this.fh(a.target))this.Jb(false)};
em.prototype.fh=function(a){return a&&df(this.A(),a)||this.R&&this.R.fh(a)};
em.prototype.Jc=function(a){if(this.R){if(this.R.Ja()){var b=this.R.Jc(a);if(a.keyCode==27){this.Jb(false);return true}return b}if(a.keyCode==40||a.keyCode==38){this.Jb(true);return true}}return false};
em.prototype.xi=function(){if(this.R)this.Jb(false)};
em.prototype.CK=function(){if(this.R)this.Jb(false)};
em.prototype.Hg=function(a){if(this.pB&&this.R)this.Jb(false);em.f.Hg.call(this,a)};
em.prototype.Lj=function(){if(!this.R)this.uk(new Wl(this.K));return this.R};
em.prototype.uk=function(a){var b=this.R;if(a!=b){var c=this.Dg();if(b){this.Jb(false);if(this.Ha())c.Sa(b,xk,this.xi)}this.R=a;if(a){a.WD(this);a.M(false);if(this.pB)a.IO(false);if(this.Ha())c.g(a,xk,this.xi)}}return b};
em.prototype.ub=function(a){this.Lj().ub(a)};
em.prototype.bh=function(a,b){this.Lj().bh(a,b)};
em.prototype.kg=function(a){if(this.R)this.R.kg(a)};
em.prototype.Kh=function(a){if(this.R)this.R.Kh(a)};
em.prototype.vh=function(a){return this.R&&this.R.vh(a)};
em.prototype.yl=function(){return this.R?this.R.yl():0};
em.prototype.M=function(a,b){var c=em.f.M.call(this,a,b);if(this.R&&c&&!this.Ja())this.Jb(false);return c};
em.prototype.Ca=function(a){em.f.Ca.call(this,a);if(this.R&&!this.sb())this.Jb(false)};
em.prototype.Xo=function(){this.Jb(true)};
em.prototype.Jb=function(a){em.f.Jb.call(this,a);if(!this.R||!this.R.kB())return;var b=this.Dg();if(a){if(!this.R.Ha())this.R.C();this.R.be(-1);this.B5();this.R.M(true);b.g(this.K.Ic(),I,this.vK,true);if(!this.pB)b.g(this.R,"blur",this.CK)}else{this.Fd(false);this.R.M(false);this.R.zm(false);b.Sa(this.K.Ic(),I,this.vK,true);b.Sa(this.R,"blur",this.CK)}};
em.prototype.B5=function(){var a=this.XF?7:8,b=new ql(this.A(),a),c=this.R.A();if(!this.R.Ja()){c.style.visibility="hidden";M(c,true)}var d=this.XF?5:6;b.Qa(c,d,new ce(0,0,0,0));if(!this.R.Ja()){M(c,false);c.style.visibility="visible"}};
Kk("goog-menu-button",function(){return new em(null)});var im=function(a){J.call(this);this.ta=[];this.Ly(a)};
o(im,J);im.prototype.ta=null;im.prototype.Ui=null;im.prototype.Jo=null;im.prototype.d9=function(a){this.Jo=a};
im.prototype.yl=function(){return this.ta.length};
im.prototype.h2=function(a){return a?cc(this.ta,a):-1};
im.prototype.vh=function(a){return this.ta[a]};
im.prototype.Ly=function(a){if(a){r(a,function(b){this.Mh(b,false)},
this);vc(this.ta,a)}};
im.prototype.ub=function(a){this.bh(a,this.yl())};
im.prototype.bh=function(a,b){if(a){this.Mh(a,false);rc(this.ta,a,b)}};
im.prototype.kg=function(a){if(a&&tc(this.ta,a))if(a==this.Ui){this.Ui=null;this.dispatchEvent(lg)}};
im.prototype.Kh=function(a){this.kg(this.vh(a))};
im.prototype.pe=function(){return this.Ui};
im.prototype.ib=function(a){if(a!=this.Ui){this.Mh(this.Ui,false);this.Ui=a;this.Mh(a,true)}this.dispatchEvent(lg)};
im.prototype.Oj=function(){return this.h2(this.Ui)};
im.prototype.Qh=function(a){this.ib(this.vh(a))};
im.prototype.clear=function(){oc(this.ta);this.Ui=null};
im.prototype.b=function(){if(!this.W()){im.f.b.call(this);this.ta=null;this.Ui=null}};
im.prototype.Mh=function(a,b){if(a)if(typeof this.Jo=="function")this.Jo(a,b);else if(typeof a.Oc=="function")a.Oc(b)};var jm=function(a,b,c,d){em.call(this,a,b,c,d);this.VO(a)};
o(jm,em);jm.prototype.va=null;jm.prototype.uj=null;jm.prototype.D=function(){return"goog.ui.Select"};
jm.prototype.n=function(){jm.f.n.call(this);this.HE()};
jm.prototype.$=function(a){jm.f.$.call(this,a);var b=this.Hj();if(b)this.VO(b);else this.Qh(0)};
jm.prototype.b=function(){if(!this.W()){jm.f.b.call(this);if(this.va){this.va.b();this.va=null}this.uj=null}};
jm.prototype.xi=function(a){this.ib(a.target);jm.f.xi.call(this,a);a.stopPropagation();this.dispatchEvent(xk)};
jm.prototype.Lu=function(){var a=this.pe();jm.f.qa.call(this,a&&a.L());this.HE()};
jm.prototype.uk=function(a){var b=jm.f.uk.call(this,a);if(a!=b){if(this.va)this.va.clear();if(a)if(this.va)this.va.Ly(a.nu());else this.Az(a.nu())}return b};
jm.prototype.VO=function(a){this.uj=a;this.HE()};
jm.prototype.ub=function(a){jm.f.ub.call(this,a);if(this.va)this.va.ub(a);else this.Az(this.Lj().nu())};
jm.prototype.bh=function(a,b){jm.f.bh.call(this,a,b);if(this.va)this.va.bh(a,b);else this.Az(this.Lj().nu())};
jm.prototype.kg=function(a){jm.f.kg.call(this,a);if(this.va)this.va.kg(a)};
jm.prototype.Kh=function(a){jm.f.Kh.call(this,a);if(this.va)this.va.Kh(a)};
jm.prototype.ib=function(a){if(this.va)this.va.ib(a)};
jm.prototype.Qh=function(a){if(this.va)this.ib(this.va.vh(a))};
jm.prototype.qa=function(a){if(a&&this.va)for(var b=0,c;c=this.va.vh(b);b++)if(c&&c.L()==a){this.ib(c);return}this.ib(null)};
jm.prototype.pe=function(){return this.va&&this.va.pe()};
jm.prototype.Oj=function(){return this.va&&this.va.Oj()};
jm.prototype.Az=function(a){this.va=new im(a);this.Dg().g(this.va,lg,this.Lu)};
jm.prototype.HE=function(){var a=this.pe();this.hc(a?a.Hj():this.uj)};
jm.prototype.Jb=function(a){jm.f.Jb.call(this,a);if(this.isOpen())this.Lj().be(this.Oj())};
var km=function(){return new jm(null)};
Kk("goog-select",km);var lm=function(){J.call(this)};
o(lm,J);lm.prototype.Ua=0;lm.prototype.Ng=0;lm.prototype.zf=100;lm.prototype.Ke=0;lm.prototype.bj=1;lm.prototype.Qe=false;lm.prototype.em=false;lm.prototype.px=function(a){this.em=a};
lm.prototype.px=function(a){this.em=a};
lm.prototype.qa=function(a){a=this.Ho(a);if(this.Ua!=a){this.Ua=a+this.Ke>this.zf?this.zf-this.Ke:(a<this.Ng?this.Ng:a);if(!this.Qe&&!this.em)this.dispatchEvent(yk)}};
lm.prototype.L=function(){return this.Ho(this.Ua)};
lm.prototype.um=function(a){a=this.Ho(a);if(this.Ke!=a){this.Ke=a<0?0:(this.Ua+a>this.zf?this.zf-this.Ua:a);if(!this.Qe&&!this.em)this.dispatchEvent(yk)}};
lm.prototype.uh=function(){return this.f7(this.Ke)};
lm.prototype.Vr=function(a){if(this.Ng!=a){var b=this.Qe;this.Qe=true;this.Ng=a;if(a+this.Ke>this.zf)this.Ke=this.zf-this.Ng;if(a>this.Ua)this.qa(a);if(a>this.zf){this.Ke=0;this.Qo(a);this.qa(a)}this.Qe=b;if(!this.Qe&&!this.em)this.dispatchEvent(yk)}};
lm.prototype.Xc=function(){return this.Ho(this.Ng)};
lm.prototype.Qo=function(a){a=this.Ho(a);if(this.zf!=a){var b=this.Qe;this.Qe=true;this.zf=a;if(a<this.Ua)this.qa(a-this.Ke);if(a<this.Ng){this.Ke=0;this.Vr(a);this.qa(this.zf)}if(a<this.Ng+this.Ke)this.Ke=this.zf-this.Ng;if(a<this.Ua+this.Ke)this.Ke=this.zf-this.Ua;this.Qe=b;if(!this.Qe&&!this.em)this.dispatchEvent(yk)}};
lm.prototype.Wc=function(){return this.Ho(this.zf)};
lm.prototype.YA=function(){return this.bj};
lm.prototype.Rh=function(a){if(this.bj!=a){this.bj=a;var b=this.Qe;this.Qe=true;this.Qo(this.Wc());this.um(this.uh());this.qa(this.L());this.Qe=b;if(!this.Qe&&!this.em)this.dispatchEvent(yk)}};
lm.prototype.Ho=function(a){if(this.bj==null)return a;return this.Ng+Math.round((a-this.Ng)/this.bj)*this.bj};
lm.prototype.f7=function(a){if(this.bj==null)return a;return Math.round(a/this.bj)*this.bj};var T=function(a){P.call(this,a);this.rb=new lm;F(this.rb,yk,this.Ju,false,this)};
o(T,P);var mm="vertical";T.prototype.Hb="horizontal";T.prototype.HB=false;T.prototype.CM=false;T.prototype.oG=10;T.prototype.er=0;T.prototype.D=function(){return"goog.ui.SliderBase"};
T.prototype.O=ta;T.prototype.m=ta;T.prototype.$=ta;T.prototype.n=function(){T.f.n.call(this);this.Vs();this.my()};
T.prototype.u=function(){T.f.u.call(this);this.ln()};
T.prototype.Vs=function(){this.aR=new Xh(this.pc);this.Xba=new Xh(this.hf);this.ob=new Dg(this.e);this.Ve=new Ig(this.e);var a=new K(this);a.g(this.aR,"beforedrag",this.pK);a.g(this.Xba,"beforedrag",this.pK);a.g(this.ob,"key",this.yh);a.g(this.e,"mousedown",this.Oe);a.g(this.Ve,Jg,this.Wj);this.e.tabIndex=0;this.p=a};
T.prototype.ln=function(){if(this.p){this.p.b();this.p=null}};
T.prototype.pK=function(a){var b=a.dragger==this.aR?this.pc:this.hf,c;if(this.Hb==mm){var d=this.e.clientHeight-b.offsetHeight;c=(d-a.top)/d*(this.Wc()-this.Xc())+this.Xc()}else{var e=this.e.clientWidth-b.offsetWidth;c=a.left/e*(this.Wc()-this.Xc())+this.Xc()}c=a.dragger==this.aR?Math.min(Math.max(c,this.Xc()),this.L()+this.uh()):Math.min(Math.max(c,this.L()),this.Wc());this.Xr(b,c);a.preventDefault()};
T.prototype.yh=function(a){var b=true;switch(a.keyCode){case 36:this.Sy(this.Xc());break;case 35:this.Sy(this.Wc());break;case 33:this.ir(this.Gj());break;case 34:this.ir(-this.Gj());break;case 37:case 40:this.ir(a.shiftKey?-this.Gj():-this.bB());break;case 39:case 38:this.ir(a.shiftKey?this.Gj():this.bB());break;default:b=false}if(b)a.preventDefault()};
T.prototype.Oe=function(a){if(this.e.focus)this.e.focus();if(df(this.pc,a.target)||df(this.hf,a.target))return;if(this.CM)this.Sy(this.gK(a));else this.d$(a)};
T.prototype.Wj=function(a){var b=a.detail>0?-1:1;this.ir(b*this.bB());a.preventDefault()};
T.prototype.d$=function(a){this.oE(a);this.Xg=this.MI(this.gK(a));this.f2=this.Hb==mm?this.SB<this.Xg.offsetTop:this.SB>this.Xg.offsetLeft+this.Xg.offsetWidth;var b=Ie(this.e);F(b,"mouseup",this.Vj,true,this);F(this.e,"mousemove",this.oE,false,this);if(!this.Tl){this.Tl=new Pg(200);F(this.Tl,"tick",this.LK,false,this)}this.LK();this.Tl.start()};
T.prototype.LK=function(){var a;if(this.Hb==mm){var b=this.SB,c=this.Xg.offsetTop;if(this.f2){if(b<c)a=this.Pj(this.Xg)+this.Gj()}else{var d=this.Xg.offsetHeight;if(b>c+d)a=this.Pj(this.Xg)-this.Gj()}}else{var e=this.SB,f=this.Xg.offsetLeft;if(this.f2){var g=this.Xg.offsetWidth;if(e>f+g)a=this.Pj(this.Xg)+this.Gj()}else if(e<f)a=this.Pj(this.Xg)-this.Gj()}if(a!=undefined)this.Xr(this.Xg,a)};
T.prototype.Vj=function(){if(this.Tl)this.Tl.stop();var a=Ie(this.e);G(a,"mouseup",this.Vj,true,this);G(this.e,"mousemove",this.oE,false,this)};
T.prototype.UA=function(a){var b=xh(a,this.e);return this.Hb==mm?b.y:b.x};
T.prototype.oE=function(a){this.SB=this.UA(a)};
T.prototype.gK=function(a){var b=this.Xc(),c=this.Wc();if(this.Hb==mm){var d=this.pc.offsetHeight,e=this.e.clientHeight-d,f=this.UA(a)-d/2;return(c-b)*(e-f)/e+b}else{var g=this.pc.offsetWidth,h=this.e.clientWidth-g,i=this.UA(a)-g/2;return(c-b)*i/h+b}};
T.prototype.Pj=function(a){if(a==this.pc)return this.rb.L();else if(a==this.hf)return this.rb.L()+this.rb.uh();else throw Error("Illegal thumb element. Neither minThumb nor maxThumb");};
T.prototype.ir=function(a){var b=this.Pj(this.pc)+a,c=this.Pj(this.hf)+a;b=pe(b,this.Xc(),this.Wc()-this.er);c=pe(c,this.Xc()+this.er,this.Wc());this.DP(b,c-b)};
T.prototype.Xr=function(a,b){if(a==this.hf&&b<=this.rb.Wc()&&b>=this.rb.L()+this.er)this.rb.um(b-this.rb.L());if(a==this.pc&&b>=this.Xc()&&b<=this.rb.L()+this.rb.uh()-this.er){var c=this.rb.uh()-(b-this.rb.L());if(Math.round(b)+Math.round(c)!=Math.round(b+c))return;this.DP(b,c)}};
T.prototype.DP=function(a,b){if(this.rb.Xc()<=a&&a<=this.Wc()-b&&this.er<=b&&b<=this.Wc()-a){if(a==this.L()&&b==this.uh())return;this.rb.px(true);this.rb.um(0);this.rb.qa(a);this.rb.um(b);this.rb.px(false);this.my();this.dispatchEvent(yk)}};
T.prototype.Xc=function(){return this.rb.Xc()};
T.prototype.Vr=function(a){this.rb.Vr(a)};
T.prototype.Wc=function(){return this.rb.Wc()};
T.prototype.Qo=function(a){this.rb.Qo(a)};
T.prototype.MI=function(a){return a<=this.rb.L()+this.rb.uh()/2?this.pc:this.hf};
T.prototype.Ju=function(){this.my();this.dispatchEvent(yk)};
T.prototype.my=function(){if(this.pc&&!this.HB){var a=this.aB(this.Pj(this.pc)),b=this.aB(this.Pj(this.hf));if(this.Hb==mm){this.pc.style.top=a.y+"px";this.hf.style.top=b.y+"px"}else{this.pc.style.left=a.x+"px";this.hf.style.left=b.x+"px"}}};
T.prototype.aB=function(a){var b=new ge;if(this.pc){var c=this.Xc(),d=this.Wc(),e=(a-c)/(d-c);if(this.Hb==mm){var f=this.pc.offsetHeight,g=this.e.clientHeight-f,h=Math.round(e*g);b.y=g-h}else{var i=this.e.clientWidth-this.pc.offsetWidth,k=Math.round(e*i);b.x=k}}return b};
T.prototype.Sy=function(a){a=Math.min(this.Wc(),Math.max(a,this.Xc()));if(this.st)this.st.stop(true);var b,c=this.MI(a),d=this.aB(a);b=this.Hb==mm?[c.offsetLeft,d.y]:[d.x,c.offsetTop];var e=new Kh(c,b,100);this.st=e;F(e,"end",this.OW,false,this);this.HB=true;this.Xr(c,a);e.play(false)};
T.prototype.OW=function(){this.HB=false};
T.prototype.sx=function(a){if(this.Hb!=a){var b=this.O(this.Hb),c=this.O(a);this.Hb=a;if(this.e){rf(this.e,b,c);this.pc.style.left=(this.pc.style.top="");this.hf.style.left=(this.hf.style.top="");this.my()}}};
T.prototype.Nn=function(){return this.Hb};
T.prototype.b=function(){if(!this.W()){T.f.b.call(this);if(this.Tl)this.Tl.b();this.Tl=null;if(this.st)this.st.b();this.ln();this.st=null;this.e=null;this.pc=null;this.rb.b();if(this.ob){this.ob.b();this.ob=null}if(this.Ve){this.Ve.b();this.Ve=null}}};
T.prototype.Gj=function(){return this.oG};
T.prototype.Z7=function(a){this.oG=a};
T.prototype.zQ=1;T.prototype.bB=function(){return this.zQ};
T.prototype.p9=function(a){this.zQ=a};
T.prototype.YA=function(){return this.rb.YA()};
T.prototype.Rh=function(a){this.rb.Rh(a)};
T.prototype.Q8=function(a){this.CM=a};
T.prototype.L=function(){return this.rb.L()};
T.prototype.qa=function(a){this.Xr(this.pc,a)};
T.prototype.uh=function(){return this.rb.uh()};
T.prototype.um=function(a){this.Xr(this.hf,this.rb.L()+a)};var nm=function(a){T.call(this,a);this.rb.um(0)};
o(nm,T);nm.prototype.D=function(){return"goog.ui.Slider"};
var om="goog-slider-thumb";nm.prototype.O=function(a){return"goog-slider-"+a};
nm.prototype.m=function(){this.pc=this.tH();this.hf=this.pc;var a=this.O(this.Hb);this.e=this.K.m("div",{className:a},this.pc);Dk(this.e,"slider")};
nm.prototype.$=function(a){this.e=a;C(this.e,this.O(this.Hb));Ek(a,"valuemin",this.Xc());Ek(a,"valuemax",this.Wc());var b=gf(this.e,function(c){return c.nodeType==1&&sf(c,om)});
if(!b){b=this.tH();this.e.appendChild(b)}this.pc=b;this.hf=this.pc};
nm.prototype.tH=function(){var a=this.K.m("div",{"class":om});Dk(a,"button");return a};
nm.prototype.n=function(){nm.f.n.call(this);Dk(this.e,"slider");Ek(this.e,"valuemin",this.Xc());Ek(this.e,"valuemax",this.Wc())};
nm.prototype.Ju=function(a){nm.f.Ju.call(this,a);Ek(this.e,"valuemin",this.Xc());Ek(this.e,"valuemax",this.Wc());Ek(this.e,"valuenow",this.L())};var pm=function(a,b,c){Mk.call(this,a,b||Pk(),c);this.Sh(16,true)};
o(pm,Mk);pm.prototype.D=function(){return"goog.ui.ToggleButton"};
var qm=function(){return new pm(null)};
Kk("goog-toggle-button",qm);var rm=function(){gm.call(this)};
o(rm,gm);var sm=null;var tm="goog-color-menu-button";rm.prototype.createCaption=function(a,b){var c=b.m("div",{"class":tm+"-indicator"},a);return rm.f.createCaption(c,b)};
rm.prototype.qa=function(a,b){if(a){var c=this.Oa(a);if(c&&c.firstChild)c.firstChild.style.borderBottomColor=b||(s?"":"transparent")}};
rm.prototype.sf=function(a){this.qa(a.A(),a.L());C(a.A(),tm);rm.f.sf.call(this,a)};var um=function(){Fk.call(this)};
o(um,Fk);var vm=null,wm=0,xm=function(){return vm||(vm=new um)};
um.prototype.m=function(a){var b=this.si(a);return a.K.m("div",b?{"class":b.join(" ")}:null,this.hV(a.Vc(),a.getSize(),a.K))};
um.prototype.hV=function(a,b,c){var d=[];for(var e=0,f=0;e<b.height;e++){var g=[];for(var h=0;h<b.width;h++){var i=a&&a[f++];g.push(this.wz(i,c))}d.push(this.zz(g,c))}return this.zV(d,c)};
um.prototype.zV=function(a,b){var c=b.m("table",{"class":this.O()+"-table"},b.m("tbody",{"class":this.O()+"-body"},a));c.cellSpacing=0;c.cellPadding=0;Dk(c,"grid");return c};
um.prototype.zz=function(a,b){return b.m("tr",{"class":this.O()+"-row"},a)};
um.prototype.wz=function(a,b){var c=b.m("td",{"class":this.O()+"-cell",id:this.O()+"-cell-"+wm++},a);Dk(c,"gridcell");return c};
um.prototype.kb=function(){return false};
um.prototype.ia=function(){return null};
um.prototype.hc=function(a,b){if(a){var c=Ne("tbody",this.O()+"-body",a)[0];if(c){var d=0;r(c.rows,function(k){r(k.cells,function(n){We(n);if(b){var q=b[d++];if(q)x(n,q)}})});
if(d<b.length){var e=[],f=Je(a),g=c.rows[0].cells.length;while(d<b.length){var h=b[d++];e.push(this.wz(h,f));if(e.length==g){var i=this.zz(e,f);x(c,i);e.length=0}}if(e.length>0){while(e.length<g)e.push(this.wz("",f));var i=this.zz(e,f);x(c,i)}}}Hh(a,true,se)}};
um.prototype.AA=function(a,b){var c=a.A();while(b&&b.nodeType==1&&b!=c){if(b.tagName=="TD"&&sf(b,this.O()+"-cell"))return b.firstChild;b=b.parentNode}return null};
um.prototype.YK=function(a,b,c){if(b){var d=b.parentNode;tf(d,this.O()+"-cell-hover",c);var e=a.A().firstChild;Ek(e,"activedescendent",d.id)}};
um.prototype.A7=function(a,b,c){if(b){var d=b.parentNode;tf(d,this.O()+"-cell-selected",c)}};
um.prototype.O=function(){return"goog-palette"};var ym=function(a,b,c){Q.call(this,a,b||xm(),c)};
o(ym,Q);ym.prototype.tb=null;ym.prototype.zc=-1;ym.prototype.va=null;ym.prototype.D=function(){return"goog.ui.Palette"};
ym.prototype.b=function(){if(!this.W()){ym.f.b.call(this);if(this.va){this.va.b();this.va=null}this.tb=null}};
ym.prototype.sm=function(a){ym.f.sm.call(this,a);this.UF();if(this.va){this.va.clear();this.va.Ly(a)}else{this.va=new im(a);this.va.d9(l(this.Mh,this));this.Dg().g(this.va,lg,this.Lu)}this.zc=-1};
ym.prototype.Hj=function(){return null};
ym.prototype.Sg=function(){};
ym.prototype.Kl=function(a){ym.f.Kl.call(this,a);var b=this.Tf().AA(this,a.target);if(b&&a.relatedTarget&&df(b,a.relatedTarget))return;if(b!=this.ku())this.ZO(b)};
ym.prototype.Jl=function(a){ym.f.Jl.call(this,a);var b=this.Tf().AA(this,a.target);if(b&&a.relatedTarget&&df(b,a.relatedTarget))return;if(b==this.ku())this.Tf().YK(this,b,false)};
ym.prototype.Ud=function(a){ym.f.Ud.call(this,a);if(this.zd()){var b=this.Tf().AA(this,a.target);if(b!=this.ku())this.ZO(b)}};
ym.prototype.Oi=function(){var a=this.ku();if(a){this.ib(a);return this.dispatchEvent(xk)}return false};
ym.prototype.Jc=function(a){var b=this.Vc(),c=b?b.length:0,d=this.tb.width;if(c==0||!this.sb())return false;if(a.keyCode==13||a.keyCode==32)return this.Oi(a);if(a.keyCode==36){this.be(0);return true}else if(a.keyCode==35){this.be(c-1);return true}var e=this.zc<0?this.Oj():this.zc;switch(a.keyCode){case 37:if(e==-1)e=c;if(e>0){this.be(e-1);a.preventDefault();return true}break;case 39:if(e<c-1){this.be(e+1);a.preventDefault();return true}break;case 38:if(e==-1)e=c+d-1;if(e>=d){this.be(e-d);a.preventDefault();
return true}break;case 40:if(e==-1)e=-d;if(e<c-d){this.be(e+d);a.preventDefault();return true}break}return false};
ym.prototype.Lu=function(){};
ym.prototype.getSize=function(){return this.tb};
ym.prototype.h9=function(a,b){if(this.A())throw Error(zk);this.tb=Ba(a)?new ne(a,b):a;this.UF()};
ym.prototype.ku=function(){var a=this.Vc();return a&&a[this.zc]};
ym.prototype.be=function(a){if(a!=this.zc){this.ZK(this.zc,false);this.zc=a;this.ZK(a,true)}};
ym.prototype.ZO=function(a){var b=this.Vc();this.be(b?cc(b,a):-1)};
ym.prototype.Oj=function(){return this.va?this.va.Oj():-1};
ym.prototype.pe=function(){return this.va&&this.va.pe()};
ym.prototype.Qh=function(a){if(this.va)this.va.Qh(a)};
ym.prototype.ib=function(a){if(this.va)this.va.ib(a)};
ym.prototype.ZK=function(a,b){if(this.A()){var c=this.Vc();if(c&&a>=0&&a<c.length)this.Tf().YK(this,c[a],b)}};
ym.prototype.Mh=function(a,b){if(this.A())this.Tf().A7(this,a,b)};
ym.prototype.UF=function(){var a=this.Vc();if(a)if(this.tb&&this.tb.width){var b=Math.ceil(a.length/this.tb.width);if(!Ba(this.tb.height)||this.tb.height<b)this.tb.height=b}else{var c=Math.ceil(Math.sqrt(a.length));this.tb=new ne(c,c)}else this.tb=new ne(0,0)};var zm=function(a,b,c){this.ZG=a;ym.call(this,null,b||xm(),c);this.d8(a)};
o(zm,ym);zm.prototype.hw=null;zm.prototype.D=function(){return"goog.ui.ColorPalette"};
zm.prototype.d8=function(a){this.ZG=a;this.hw=null;this.hc(this.YU())};
zm.prototype.tu=function(){var a=this.pe();if(a){var b=ph(a,"background-color");return hh(b).hex}else return null};
zm.prototype.zx=function(a){var b=a?hh(a).hex:null;if(!this.hw)this.hw=gc(this.ZG,function(c){return hh(c).hex});
this.Qh(b?cc(this.hw,b):-1)};
zm.prototype.YU=function(){var a=this.K;return gc(this.ZG,function(b){return a.m("div",{"class":"goog-palette-colorswatch",style:"background-color:"+b})})};var Am=function(a,b,c,d){em.call(this,a,b,c||sm||(sm=new rm),d)};
o(Am,em);var Bm={GRAYSCALE:["#000","#444","#666","#999","#ccc","#eee","#f3f3f3","#fff"],SOLID:["#f00","#f90","#ff0","#0f0","#0ff","#00f","#90f","#f0f"],PASTEL:["#f4cccc","#fce5cd","#fff2cc","#d9ead3","#d0e0e3","#cfe2f3","#d9d2e9","#ead1dc","#ea9999","#f9cb9c","#ffe599","#b6d7a8","#a2c4c9","#9fc5e8","#b4a7d6","#d5a6bd","#e06666","#f6b26b","#ffd966","#93c47d","#76a5af","#6fa8dc","#8e7cc3","#c27ba0","#cc0000","#e69138","#f1c232","#6aa84f","#45818e","#3d85c6","#674ea7","#a64d79","#990000","#b45f06","#bf9000",
"#38761d","#134f5c","#0b5394","#351c75","#741b47","#660000","#783f04","#7f6000","#274e13","#0c343d","#073763","#20124d","#4c1130"]},Cm=function(a,b){var c=new Wl(b);if(a)r(a,c.ub,c);Gc(Bm,function(d){var e=new zm(d,null,b);e.h9(8);c.ub(e)});
return c};
Am.prototype.tu=function(){return this.L()};
Am.prototype.zx=function(a){this.qa(a)};
Am.prototype.qa=function(a){for(var b=0,c;c=this.vh(b);b++)if(typeof c.zx=="function")c.zx(a);Am.f.qa.call(this,a)};
Am.prototype.D=function(){return"goog.ui.ColorMenuButton"};
Am.prototype.xi=function(a){if(typeof a.target.tu=="function")this.qa(a.target.tu());else if(a.target.L()=="none")this.qa(null);Am.f.xi.call(this,a);a.stopPropagation();this.dispatchEvent(xk)};
Am.prototype.Jb=function(a){if(a&&this.yl()==0){this.uk(Cm(null,this.K));this.qa(this.L())}Am.f.Jb.call(this,a)};
Kk(tm,function(){return new Am(null)});var Dm=function(){gm.call(this)};
o(Dm,gm);var Em=null,Fm=function(){return Em||(Em=new Dm)},
Gm="goog-toolbar-menu-button";Dm.prototype.O=function(){return Gm};var Hm=function(){Dm.call(this)};
o(Hm,Dm);var Im=null;Hm.prototype.createCaption=function(a,b){return rm.prototype.createCaption.call(this,a,b)};
Hm.prototype.qa=function(a,b){rm.prototype.qa.call(this,a,b)};
Hm.prototype.sf=function(a){this.qa(a.A(),a.L());C(a.A(),"goog-toolbar-color-menu-button");Hm.f.sf.call(this,a)};var Jm=function(){Nk.call(this)};
o(Jm,Nk);var Km=null,Lm=function(){return Km||(Km=new Jm)},
Mm="goog-toolbar-button";Jm.prototype.O=function(){return Mm};var Nm=function(){return Bl.call(this)};
o(Nm,Bl);var Om=null,Pm=function(){return Om||(Om=new Nm)},
Qm="goog-toolbar-separator";Nm.prototype.m=function(a){return a.K.m("div",{"class":this.O()+" goog-inline-block"},"\u00a0")};
Nm.prototype.ia=function(a,b){b=Nm.f.ia.call(this,a,b);C(b,"goog-inline-block");return b};
Nm.prototype.O=function(){return Qm};var Rm=function(){Gl.call(this)};
o(Rm,Gl);var Sm=null;Rm.prototype.tl=function(a){return a.tagName=="HR"?new Fl(Pm()):Rm.f.tl.call(this,a)};
Rm.prototype.O=function(){return"goog-toolbar"};
Rm.prototype.DA=function(){return Il};var Tm=function(a,b,c){S.call(this,b,a||Sm||(Sm=new Rm),c)};
o(Tm,S);Tm.prototype.D=function(){return"goog.ui.Toolbar"};
Kk(Mm,function(){return new Mk(null,Lm())});
Kk("goog-toolbar-toggle-button",function(){var a=new Mk(null,Lm());a.Sh(16,true);return a});
Kk("goog-toolbar-color-menu-button",function(){return new Am(null,null,Im||(Im=new Hm))});
Kk(Gm,function(){return new em(null,null,Fm())});
Kk("goog-toolbar-select",function(){return new jm(null,null,Fm())});
Kk(Qm,function(){return new Fl(Pm())});var Um=function(a,b,c){this.K=c||Je(a?u(a):null);il.call(this,this.K.m("div",{style:"position:absolute;display:none;"}));this.Fz=new ge(0,0);this.ge=null;this.Ie=new hd;if(a)this.rc(a);if(b!=null)this.l9(b)};
o(Um,il);var Vm=[];Um.prototype.className="goog-tooltip";Um.prototype.L9=500;Um.prototype.H1=0;Um.prototype.up=null;Um.prototype.rc=function(a){a=u(a);this.Ie.add(a);F(a,Gf,this.se,false,this);F(a,Hf,this.Hu,false,this);F(a,fg,this.Uj,false,this);F(a,jg,this.Aq,false,this);F(a,ig,this.Hu,false,this)};
Um.prototype.detach=function(a){if(a){var b=u(a);this.MH(b);this.Ie.remove(b)}else{var c=this.Ie.xb();for(var b,d=0;b=c[d];d++)this.MH(b);this.Ie.clear()}};
Um.prototype.MH=function(a){G(a,Gf,this.se,false,this);G(a,Hf,this.Hu,false,this);G(a,fg,this.Uj,false,this);G(a,jg,this.Aq,false,this);G(a,ig,this.Hu,false,this)};
Um.prototype.l9=function(a){B(this.e,a)};
Um.prototype.y8=function(a){this.e.innerHTML=a};
Um.prototype.Sr=function(a){if(this.e){A(this.e);this.e=null}gl.prototype.Sr.call(this,a);if(a)x(this.K.Ic().body,a)};
Um.prototype.$A=function(){return lf(this.e)};
Um.prototype.pr=function(){gl.prototype.pr.call(this);for(var a,b=0;a=Vm[b];b++)if(!df(a.e,this.ge))a.M(false);pc(Vm,this);this.e.className=this.className;this.Dp();F(this.e,Gf,this.OK,false,this);F(this.e,Hf,this.NK,false,this);return true};
Um.prototype.gk=function(){tc(Vm,this);for(var a,b=0;a=Vm[b];b++)if(df(this.e,a.up))a.M(false);else{a.ge=null;a.nE()}G(this.e,Gf,this.OK,false,this);G(this.e,Hf,this.NK,false,this);this.up=null;gl.prototype.gk.call(this)};
Um.prototype.U3=function(a,b){if(this.ge==a)this.A5(a,b)};
Um.prototype.A5=function(a,b){var c;if(b)c=b;else{var d=new ge(this.Fz.x,this.Fz.y);c=new Wm(d)}this.up=a;this.setPosition(c);this.M(true)};
Um.prototype.T3=function(){if(this.ge==null||this.ge!=this.e&&!this.Ie.contains(this.ge))this.M(false);this.Dp()};
Um.prototype.se=function(a){this.ge=a.target;this.Dp();if(!this.Ja())this.aQ(a.target)};
Um.prototype.Uj=function(a){var b=Te(this.K.Ic())||window,c=Se(b);this.Fz.x=a.clientX+c.x;this.Fz.y=a.clientY+c.y};
Um.prototype.Aq=function(a){this.ge=a.target;if(!this.Ja()){var b=new Xm(this.ge);this.aQ(a.target,b);this.Dp()}};
Um.prototype.Hu=function(a){if(a.target==this.ge)this.ge=null;this.nE();this.rU()};
Um.prototype.OK=function(){if(this.ge!=this.e){this.Dp();this.ge=this.e}};
Um.prototype.NK=function(){if(this.ge==this.e){this.ge=null;this.nE()}};
Um.prototype.aQ=function(a,b){this.RP=Rg(l(this.U3,this,a,b),this.L9)};
Um.prototype.rU=function(){if(this.RP){Sg(this.RP);this.RP=null}};
Um.prototype.nE=function(){if(!this.zi)this.zi=Rg(this.T3,this.H1,this)};
Um.prototype.Dp=function(){if(this.zi){Sg(this.zi);this.zi=null}};
Um.prototype.b=function(){if(!this.W()){J.prototype.b.call(this);this.detach();if(this.e)A(this.e);this.ge=null;this.K=null;this.e=null}};
var Wm=function(a,b){sl.call(this,a,b)};
o(Wm,sl);Wm.prototype.Qa=function(a,b,c){var d=th(a),e=c?new ce(c.top+10,c.right,c.bottom,c.left+10):new ce(10,0,0,10),f=ll(d,5,a,5,this.coordinate,e,9);if(!f)ll(d,5,a,7,this.coordinate,e,5)};
var Xm=function(a){pl.call(this,a,4)};
o(Xm,pl);Xm.prototype.Qa=function(a,b,c){var d=new ge(0,0),e=c?new ce(c.top,c.right,c.bottom,c.left-10):new ce(0,0,0,-10),f=ll(this.element,this.corner,a,b,d,e,9);if(!f)ll(this.element,2,a,3,d,e,5)};var Ym=function(a,b,c){this.ZL=a;this.Lg=b||0;this.j=c;this.ud=l(this.UH,this)};
o(Ym,Vd);Ym.prototype.Za=0;Ym.prototype.b=function(){if(!this.W()){Ym.f.b.call(this);this.stop();this.ZL=null;this.j=null}};
Ym.prototype.start=function(a){this.stop();this.Za=Rg(this.ud,pa(a)?a:this.Lg)};
Ym.prototype.stop=function(){if(this.zd())Sg(this.Za);this.Za=0};
Ym.prototype.yI=function(){this.UH();this.stop()};
Ym.prototype.zd=function(){return this.Za!=0};
Ym.prototype.UH=function(){this.Za=0;if(this.ZL)this.ZL.call(this.j)};var $m=function(a,b,c,d){J.call(this);if(a&&!b)throw Error("Can't use invisible history without providing a blank page.");var e;if(c)e=c;else{var f="history_state"+Zm;document.write(Pa('<input type="text" name="%s" id="%s" style="display:none" />',f,f));e=u(f)}this.Ru=e;this.Gd=c?Te(Ie(c)):window;this.Um=this.Gd.location.href.split("#")[0]+"#";this.jL=b;this.ab=new Pg(150);this.vs=!a;this.p=new K(this);if(a||s){var g;if(d)g=d;else{var h="history_iframe"+Zm,i=this.jL?'src="'+p(this.jL)+'"':"";document.write(Pa('<iframe id="%s" style="display:none" %s></iframe>',
h,i));g=u(h)}this.ec=g}if(s){this.p.g(this.Gd,"load",this.J4);this.documentLoaded=false;this.H9=false}if(this.vs)this.vm(this.Bu());else this.Po(this.Ru.value);Zm++};
o($m,J);$m.prototype.Ea=false;$m.prototype.Kv=false;$m.prototype.zv=null;$m.prototype.eo=null;$m.prototype.b=function(){if(!this.W()){$m.f.b.call(this);this.p.b();this.Ca(false)}};
$m.prototype.Ca=function(a){if(a==this.Ea)return;if(s&&!this.documentLoaded){this.H9=a;return}if(a){if(re)this.p.g(this.Gd.document,an,this.U4);else if(se)this.p.g(this.Gd,"pageshow",this.Bf);if(!s||this.documentLoaded){this.p.g(this.ab,"tick",this.wN);this.Ea=true;this.ab.start();this.dispatchEvent(new bn(this.Bu()))}}else{this.Ea=false;this.p.ma();this.ab.stop()}};
$m.prototype.J4=function(){this.documentLoaded=true;if(this.Ru.value)this.Po(this.Ru.value,true);this.Ca(this.H9)};
$m.prototype.Bf=function(a){if(a.du().persisted){this.Ca(false);this.Ca(true)}};
$m.prototype.Bu=function(){return this.eo!==null?this.eo:(this.vs?this.OA(this.Gd):this.KA()||"")};
$m.prototype.Th=function(a,b){this.x8(a,false,b)};
$m.prototype.OA=function(a){var b=a.location.href,c=b.indexOf("#");return c<0?"":b.substring(c+1)};
$m.prototype.x8=function(a,b,c){if(this.Bu()!=a)if(this.vs){this.vm(a,b);if(s)this.Po(a,b,c);if(this.enabled)this.wN()}else{this.Po(a,b);this.eo=(this.zv=(this.Ru.value=a));this.dispatchEvent(new bn(a))}};
$m.prototype.vm=function(a,b){var c=this.Um+(a||""),d=this.Gd.location;if(c!=d.href)if(b)d.replace(c);else d.href=c};
$m.prototype.Po=function(a,b,c){if(!cn&&a!=this.KA()){a=ab(a);if(s){var d=ef(this.ec);d.open("text/html",b?"replace":undefined);d.write(Pa("<title>%s</title><body>%s</body>",c||this.Gd.document.title,a));d.close()}else{var e=this.jL+"#"+a,f=this.ec.contentWindow;if(f)if(b)f.location.replace(e);else f.location.href=e}}};
$m.prototype.KA=function(){if(s){var a=ef(this.ec);return a.body?bb(a.body.innerHTML):null}else if(cn)return null;else{var b=this.ec.contentWindow;if(b){var c;try{c=bb(this.OA(b))}catch(d){if(!this.Kv)this.hP(true);return null}if(this.Kv)this.hP(false);return c||null}else return null}};
$m.prototype.wN=function(){if(this.vs){var a=this.OA(this.Gd);if(a!=this.zv)this.Ze(a)}if(!this.vs||s){var b=this.KA()||"";if(this.eo==null||b==this.eo){this.eo=null;if(b!=this.zv)this.Ze(b)}}};
$m.prototype.Ze=function(a){this.zv=(this.Ru.value=a);if(this.vs){if(s)this.Po(a);this.vm(a)}else this.Po(a);this.dispatchEvent(new bn(this.Bu()))};
$m.prototype.hP=function(a){if(this.Kv!=a)this.ab.setInterval(a?10000:150);this.Kv=a};
$m.prototype.U4=function(){this.ab.stop();this.ab.start()};
var cn=$b&&De(xe,"419")<=0,an=[I,gg,fg],Zm=0,bn=function(a){D.call(this,"navigate");this.token=a};
o(bn,D);var dn,en;(function(){var a="ScriptEngine"in j,b=false,c="0";if(a){b=j.ScriptEngine()=="JScript";if(b)c=j.ScriptEngineMajorVersion()+"."+j.ScriptEngineMinorVersion()+"."+j.ScriptEngineBuildVersion()}dn=b;en=c})();var fn=function(){this.nj=dn?[]:"";this.F.apply(this,arguments)};
fn.prototype.J=function(a){this.clear();this.F(a)};
if(dn){fn.prototype.bz=0;fn.prototype.F=function(){if(arguments.length==1)this.nj[this.bz++]=arguments[0];else{this.nj.push.apply(this.nj,arguments);this.bz=this.nj.length}return this}}else fn.prototype.F=function(){for(var a=0;a<arguments.length;a++)this.nj+=arguments[a];
return this};
fn.prototype.clear=function(){if(dn){this.nj.length=0;this.bz=0}else this.nj=""};
fn.prototype.toString=function(){if(dn){var a=this.nj.join("");this.clear();if(a)this.F(a);return a}else return this.nj};var gn,hn;(function(){function a(h){var i=h.match(/[\d]+/g);i.length=3;return i.join(".")}
var b=false,c="";if(navigator.plugins&&navigator.plugins.length){var d=navigator.plugins["Shockwave Flash"];if(d){b=true;if(d.description)c=a(d.description)}if(navigator.plugins["Shockwave Flash 2.0"]){b=true;c="2.0.0.11"}}else if(navigator.mimeTypes&&navigator.mimeTypes.length){var e=navigator.mimeTypes["application/x-shockwave-flash"];b=e&&e.enabledPlugin;if(b)c=a(e.enabledPlugin.description)}else try{var f=new ActiveXObject("ShockwaveFlash.ShockwaveFlash.7");b=true;c=a(f.GetVariable("$version"))}catch(g){try{var f=
new ActiveXObject("ShockwaveFlash.ShockwaveFlash.6");b=true;c="6.0.21"}catch(g){try{var f=new ActiveXObject("ShockwaveFlash.ShockwaveFlash");b=true;c=a(f.GetVariable("$version"))}catch(g){}}}gn=b;hn=c})();var jn,kn,ln="hasPicasa";(function(){var a=false;if(s){j[ln]=a;document.write('<!--[if gte Picasa 2]><script type="text/javascript">this.'+ln+"=true;<\/script><![endif]--\>");a=j[ln];j[ln]=undefined}else if(navigator.mimeTypes&&navigator.mimeTypes["application/x-picasa-detect"])a=true;jn=a;kn=a?"2":""})();var mn={},nn=[],on=true,pn=[];function qn(a,b,c,d){d.className="transnote";if(mn[a])mn[a].push(d);else mn[a]=[d];nn.push(d);if(!on)d.style.display="none";d.onclick=function(e){if(!e)e=window.event;e.cancelBubble=true;if(e.stopPropagation)e.stopPropagation();rn(a,b,c);return false}}
function rn(a,b,c){var d=mn[a],e;for(e=0;e<d.length;e++)d[e].className="transnoteviewed";window.open("http://www.google.com/transconsole/context?project="+c+"&msg_id="+a+"&lang="+b,"","width=800, height=600, resizable=1, scrollbars=1")}
function sn(a,b,c){var d=document.createElement("span");d.innerHTML="[TC]";qn(a,b,c,d);return d}
function tn(a,b,c){this.docId=a;this.lang=b;this.proj=c}
function un(a){for(var b=0;b<pn.length;b++){var c=pn[b],d=sn(c.docId,c.lang,c.proj);if(a.nextSibling)a.parentNode.insertBefore(d,a.nextSibling);else a.parentNode.appendChild(d)}pn=[]}
function vn(a,b){var c,d=[];for(c=a.firstChild;c;c=c.nextSibling)d.push(c);for(var e=0;e<d.length;e++){c=d[e];if(c.nodeType==3){var f=/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/.exec(c.data);while(f){c.data=c.data.replace(/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/,"");pn.push(new tn(f[1],f[2],f[3]));f=/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/.exec(c.data)}}else if(c.nodeType==1){var g=c.tagName.toLowerCase();switch(g){case "input":var h=
c.type.toLowerCase();if(h=="button"||h=="submit"||h=="reset"){f=/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/.exec(c.value);while(f){c.value=c.value.replace(/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/,"");pn.push(new tn(f[1],f[2],f[3]));f=/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/.exec(c.value)}}break;case "script":var f=/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/.exec(c.text);while(f){c.text=c.text.replace(/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/,
"");pn.push(new tn(f[1],f[2],f[3]));f=/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/.exec(c.text)}break;case "img":var f=/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/.exec(c.alt);while(f){c.alt=c.alt.replace(/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/,"");c.title=c.title.replace(/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/,"");pn.push(new tn(f[1],f[2],f[3]));f=/\[transmessage:id=([0-9]+):lang=([^\]:]+):project=([^\]]+)\]/.exec(c.alt)}break;
case "a":vn(c,b+1);break;default:vn(c,b);break}}if(!b)un(c)}}
function wn(){vn(document.body,0);window.setTimeout(wn,500)}
;var yn=function(a,b,c){J.call(this);this.io=a;this.Jo=c;this.pa=b;F(b,["hilite","select",xn,"dismiss"],this);this.ej=null;this.ca=[];this.Ig=-1;this.pi=0;this.Ec=null;this.nn=null};
o(yn,J);yn.prototype.P3=10;yn.prototype.iG=true;yn.prototype.YF=false;var xn="canceldismiss";yn.prototype.Zc=function(a){if(a.target==this.pa)switch(a.type){case "hilite":this.Ai(a.row);break;case "select":this.Io();break;case xn:this.jz();break;case "dismiss":this.QH();break}};
yn.prototype.X7=function(a){this.iG=a};
yn.prototype.V7=function(a){this.YF=a};
yn.prototype.Th=function(a,b){if(this.ej==a)return;this.ej=a;this.io.pk(this.ej,this.P3,l(this.M3,this),b);this.jz()};
yn.prototype.KZ=function(){return this.Ec};
yn.prototype.Uo=function(a){this.Ec=a};
yn.prototype.isOpen=function(){return this.pa.Ja()};
yn.prototype.aL=function(){if(this.Ig>=this.pi&&this.Ig<this.pi+this.ca.length-1){this.Ai(this.Ig+1);return true}else if(this.Ig==-1){this.Ai(this.pi);return true}return false};
yn.prototype.bL=function(){if(this.Ig>this.pi){this.Ai(this.Ig-1);return true}else if(this.YF&&this.Ig==this.pi)this.Ai(-1);return false};
yn.prototype.Ai=function(a){this.Ig=a;this.pa.Ai(a);return this.LA(a)!=-1};
yn.prototype.Io=function(){var a=this.LA(this.Ig);if(a!=-1){var b=this.ca[a];this.Jo.ax(b);this.ji();this.dispatchEvent({type:"update",row:b});return true}else{this.ji();this.dispatchEvent({type:"update",row:null});return false}};
yn.prototype.ji=function(){this.Ig=-1;this.ej=null;this.pi+=this.ca.length;this.ca.length=0;window.clearTimeout(this.nn);this.nn=null;this.pa.ji()};
yn.prototype.QH=function(){if(!this.nn)this.nn=window.setTimeout(l(this.ji,this),100)};
yn.prototype.jz=function(){window.setTimeout(l(function(){if(this.nn){window.clearTimeout(this.nn);this.nn=null}},
this),10)};
yn.prototype.b=function(){if(!this.W()){yn.f.b.call(this);this.pa.b();this.Jo.b();this.io=null}};
yn.prototype.M3=function(a,b,c){if(this.ej!=a)return;var d=c?this.LA(this.Ig):null;this.pi+=this.ca.length;this.ca=b;var e=[];for(var f=0;f<b.length;++f)e.push({id:this.tJ(f),data:b[f]});this.pa.I6(e,this.ej,this.Ec);if(this.iG&&e.length!=0){var g=d!=null?this.tJ(d):this.pi;this.Ai(g)}else this.Ig=-1};
yn.prototype.LA=function(a){var b=a-this.pi;if(b<0||b>=this.ca.length)return-1;return b};
yn.prototype.tJ=function(a){return this.pi+a};var zn=function(a,b,c,d){Vd.call(this);this.Ko=wa(a)?a:",;";this.g3=b||"";this.lr=c!=null?c:true;this.ab=new Pg(d||150);this.Iba=this.Ko.substring(0,1);this.Iea=new RegExp("^[\\s"+this.Ko+"]+|[\\s"+this.Ko+"]+$","g");this.gea=new RegExp("\\s*["+this.Ko+"]$");this.He=new K(this);this.Js=new K(this);this.ob=new Dg};
o(zn,Vd);zn.prototype.Kaa=true;zn.prototype.OX=true;zn.prototype.YQ=false;zn.prototype.M7=true;zn.prototype.xc=null;zn.prototype.Qq="";zn.prototype.Lm=false;zn.prototype.qD=false;zn.prototype.Uy=function(a){this.jb=a};
zn.prototype.dG=function(a){Dk(a,"select");Ek(a,"haspopup",true);this.He.g(a,jg,this.qo);this.He.g(a,ig,this.CC);if(!this.xc)this.Js.g(a,gg,this.VM)};
zn.prototype.NH=function(a){if(a==this.xc)this.CC();this.He.Sa(a,jg,this.qo);this.He.Sa(a,ig,this.CC);if(!this.xc)this.Js.Sa(a,gg,this.VM)};
zn.prototype.Vy=function(){for(var a=0;a<arguments.length;a++)this.dG(arguments[a])};
zn.prototype.eW=function(){for(var a=0;a<arguments.length;a++)this.NH(arguments[a])};
zn.prototype.ax=function(a,b){var c=this.jb.KZ();if(pa(b)?b:this.lr){var d=wg(c),e=this.eK(c.value,d),f=this.iE(c.value),g=a.toString();if(!this.gea.test(g))g=Wa(g)+this.Iba;if(this.Kaa){if(e!=0&&!Qa(f[e-1]))g=" "+g;if(e<f.length&&!Qa(f[e+1]))g=g+" "}if(g!=f[e]){f[e]=g;c.value=f.join("");var h=0;for(var i=0;i<=e;i++)h+=f[i].length;c.focus();vg(c,h);xg(c,h)}}else c.value=a.toString();this.qD=true};
zn.prototype.b=function(){if(!this.W()){zn.f.b.call(this);this.He.b();this.He=null;this.Js.b();this.Js=null}};
zn.prototype.Jc=function(a){switch(a.keyCode){case 40:if(this.jb.isOpen()){this.$v();a.preventDefault();return true}else if(!this.lr){this.Ze(true);a.preventDefault();return true}break;case 38:if(this.jb.isOpen()){this.bw();a.preventDefault();return true}break;case 9:this.Ze();if(this.jb.Io()&&this.lr){a.preventDefault();return true}break;case 13:this.Ze();if(this.jb.Io()){a.preventDefault();return true}break;case 27:if(this.jb.isOpen()){this.jb.ji();return true}break;case 229:if(!this.Lm){this.He.g(this.xc,
hg,this.DC);this.Lm=true;return true}break}if(this.M7&&this.lr&&a.charCode&&this.Ko.indexOf(String.fromCharCode(a.charCode))!=-1){this.Ze();if(this.jb.Io()){a.preventDefault();return true}}return false};
zn.prototype.NS=function(){this.ob.rc(this.xc);this.He.g(this.ob,"key",this.WM);if(s)this.He.g(this.xc,If,this.TM)};
zn.prototype.x6=function(){this.He.Sa(this.ob,"key",this.WM);this.ob.detach();if(s)this.He.Sa(this.xc,If,this.TM);if(this.Lm)this.He.Sa(this.xc,hg,this.DC)};
zn.prototype.qo=function(a){this.Js.ma();if(this.jb)this.jb.jz();if(a.target!=this.xc){this.xc=a.target;this.ab.start();this.He.g(this.ab,"tick",this.YM);this.Qq=this.xc.value;this.NS()}};
zn.prototype.CC=function(){if(this.xc){this.x6();this.xc=null;this.Lm=false;this.ab.stop();this.He.Sa(this.ab,"tick",this.YM);if(this.jb)this.jb.QH()}};
zn.prototype.YM=function(){if(!this.Lm)this.Ze()};
zn.prototype.VM=function(a){this.qo(a)};
zn.prototype.WM=function(a){if(this.jb&&!this.Lm)this.Jc(a)};
zn.prototype.DC=function(a){if(a.keyCode==13){this.Lm=false;this.He.Sa(this.xc,hg,this.DC)}};
zn.prototype.TM=function(a){if(this.lr&&this.Ko.indexOf(String.fromCharCode(a.charCode))!=-1){this.Ze();if(this.jb.Io())a.preventDefault()}};
zn.prototype.Ze=function(a){if(a||this.xc&&this.xc.value!=this.Qq){if(!this.qD){var b=this.l5();if(this.jb){this.jb.Uo(this.xc);this.jb.Th(b,this.xc.value)}}this.Qq=this.xc.value}this.qD=false};
zn.prototype.bw=function(){return this.YQ?this.jb.aL():this.jb.bL()};
zn.prototype.$v=function(){return this.YQ?this.jb.bL():this.jb.aL()};
zn.prototype.l5=function(){var a=wg(this.xc),b=this.xc.value;return this.V$(this.iE(b)[this.eK(b,a)])};
zn.prototype.V$=function(a){return String(a).replace(this.Iea,"")};
zn.prototype.eK=function(a,b){var c=this.iE(a);if(b==a.length)return c.length-1;var d=0;for(var e=0,f=0;e<c.length&&f<b;e++){f+=c[e].length;d=e}return d};
zn.prototype.iE=function(a){if(!this.lr)return[a];var b=String(a).split(""),c=[],d=[];for(var e=0,f=false;e<b.length;e++)if(this.g3&&this.g3.indexOf(b[e])!=-1){if(this.OX&&!f){c.push(d.join(""));d.length=0}d.push(b[e]);f=!f}else if(!f&&this.Ko.indexOf(b[e])!=-1){d.push(b[e]);c.push(d.join(""));d.length=0}else d.push(b[e]);c.push(d.join(""));return c};var An=function(a,b,c,d){this.P=a||Ke().body;this.K=Je(this.P);this.M6=!a;this.e=null;this.ej="";this.ca=[];this.tB=-1;this.e$=-1;this.la=false;this.className="ac-renderer";this.rowClassName="ac-row";this.activeClassName="active";this.tj=b;this.Oea=d!=null?d:true;this.Wda=c!=null?c:false;this.IL=null;this.BE=false},
Bn;o(An,J);var Cn=0;An.prototype.vp=null;An.prototype.I6=function(a,b,c){this.ej=b;this.ca=a;this.tB=0;this.e$=Ka();this.Ec=c;this.Ww=[];this.redraw()};
An.prototype.ji=function(){if(this.la){this.la=false;M(this.e,false)}};
An.prototype.show=function(){if(!this.la){this.la=true;M(this.e,true)}};
An.prototype.Ja=function(){return this.la};
An.prototype.cL=function(a){this.Q1();this.tB=a;if(a>=0&&a<this.e.childNodes.length){var b=this.Ww[a];C(b,this.activeClassName);Ek(this.e,"activedescendant",b.id);B(this.vp,lf(b))}};
An.prototype.Q1=function(){if(this.tB>=0)qf(this.Ww[this.tB],this.activeClassName)};
An.prototype.Ai=function(a){if(a==-1)this.cL(-1);else for(var b=0;b<this.ca.length;b++)if(this.ca[b].id==a){this.cL(b);return}};
An.prototype.O8=function(a){C(a,this.className)};
An.prototype.S3=function(){if(!this.e){this.vp=this.K.m("div",{style:"display:none"});Dk(this.vp,"region");Ek(this.vp,"live","rude");this.K.appendChild(this.P,this.vp);this.e=this.K.m("div",{style:"display:none"});this.O8(this.e);Dk(this.e,"menu");var a=this.e.id="goog-acr-"+Cn++;Ek(this.Ec,"controls",a);this.K.appendChild(this.P,this.e);F(this.e,E,this.xh,false,this);F(this.e,I,this.Oe,false,this);F(this.K.Ic(),E,this.uK,false,this);F(this.e,Gf,this.se,false,this)}};
An.prototype.redraw=function(){this.S3();if(this.BE)this.e.style.visibility="hidden";this.Ww.length=0;this.K.Jh(this.e);if(this.tj&&this.tj.C)this.tj.C(this,this.e,this.ca,this.ej);else{var a=null;Ec(this.ca,function(f){f=this.Nw(f,this.ej);if(this.BE)this.e.insertBefore(f,a);else this.K.appendChild(this.e,f);a=f},
this)}if(this.ca.length==0){this.ji();return}else this.show();this.P5(this.e);if(this.Ec&&this.M6){var b=uh(this.Ec),c=Ah(this.Ec),d=Ah(th(this.Ec)),e=Ah(this.e);b.y=this.BE?b.y-e.height:b.y+c.height;if((this.Wda||b.x+e.width>d.width)&&this.IL!="LEFT"){b.x=b.x+c.width-e.width;this.IL="RIGHT"}else this.IL="LEFT";yh(this.e,b);if(this.BE)this.e.style.visibility="visible"}Hh(this.e,true)};
An.prototype.Y7=function(a){this.M6=a};
An.prototype.b=function(){if(!this.W()){An.f.b.call(this);if(this.e){G(this.e,E,this.xh,false,this);G(this.e,I,this.Oe,false,this);G(this.K.Ic(),E,this.uK,false,this);G(this.e,Gf,this.se,false,this);this.K.removeNode(this.e);this.e=null;this.la=false}this.P=null}};
An.prototype.P5=function(a){if(se&&ze){a.style.width="";a.style.overflow="visible";a.style.width=a.offsetWidth;a.style.overflow="auto"}};
An.prototype.H6=function(a,b,c){c.innerHTML=p(a.data.toString())};
An.prototype.Rn=function(a,b){if(a.nodeType==3){var c,d=null;if(xa(b)){c=b.length>0?b[0]:"";if(b.length>1)d=wc(b,1)}else c=b;if(c.length==0)return;var e=a.nodeValue;c=vb(c);var f=new RegExp("(.*?)(^|\\W+)("+c+")","gi"),g=[],h=0,i=f.exec(e);while(i){g.push(i[1]);g.push(i[2]);g.push(i[3]);h=f.lastIndex;i=f.exec(e)}g.push(e.substring(h));if(g.length>1){a.nodeValue=g[0]+g[1];var k=this.K.createElement("b");this.K.appendChild(k,this.K.createTextNode(g[2]));k=a.parentNode.insertBefore(k,a.nextSibling);
for(var n=g.length-1;n>=3;n--)a.parentNode.insertBefore(this.K.createTextNode(g[n]),k.nextSibling)}else if(d)this.Rn(a,d)}else{var q=a.firstChild;while(q){var t=q.nextSibling;this.Rn(q,b);q=t}}};
An.prototype.Nw=function(a,b){var c=this.K.m("div",{className:this.rowClassName});if(this.tj&&this.tj.Mw)this.tj.Mw(a,b,c);else this.H6(a,b,c);if(b&&this.Oea)this.Rn(c,b);c.id="goog-acri-"+Cn++;C(c,this.rowClassName);Dk(c,"menuitem");this.Ww.push(c);return c};
An.prototype.WJ=function(a){while(a&&a!=this.e&&!sf(a,this.rowClassName))a=a.parentNode;return a?cc(this.Ww,a):-1};
An.prototype.xh=function(a){var b=this.WJ(a.target);if(b>=0)this.dispatchEvent({type:"select",row:this.ca[b].id});a.stopPropagation()};
An.prototype.Oe=function(a){this.dispatchEvent(xn);a.stopPropagation()};
An.prototype.uK=function(){this.dispatchEvent("dismiss")};
An.prototype.se=function(a){var b=this.WJ(a.target);if(b>=0){if(Ka()-this.e$<300)return;this.dispatchEvent({type:"hilite",row:this.ca[b].id})}};
var Dn=function(){};
Dn.prototype.C=function(){};var En=function(a,b){this.ca=a;this.Nea=!b};
En.prototype.pk=function(a,b,c){var d=this.oZ(a,b);if(d.length==0&&this.Nea)d=this.BZ(a,b);c(a,d)};
En.prototype.oZ=function(a,b){var c=[];if(a!=""){var d=vb(a),e=new RegExp("(^|\\W+)"+d,"i");Fc(this.ca,function(f){if(String(f).match(e))c.push(f);return c.length>=b})}return c};
En.prototype.BZ=function(a,b){var c=[];Ec(this.ca,function(f){var g=a.toLowerCase(),h=String(f).toLowerCase(),i=0;if(h.indexOf(g)!=-1)i=parseInt(h.indexOf(g)/4,10);else{var k=g.split(""),n=-1,i=0,q=10;for(var t=0,y;y=k[t];t++){var z=h.indexOf(y);if(z>n){var O=z-n-1;if(O>q-5)O=q-5;i+=O;n=z}else{i+=q;q+=5}}}if(i<g.length*6)c.push({str:f,score:i})});
c.sort(function(f,g){return f.score-g.score});
var d=[];for(var e=0;e<b&&e<c.length;e++)d.push(c[e].str);return d};var Fn=function(a,b,c,d){var e=new En(a,!d),f=new An,g=new zn(null,null,!(!c));yn.call(this,e,f,g);g.Uy(this);g.Vy(b)};
o(Fn,yn);var Gn=function(a){if(a)this.Vg(a)};
Gn.prototype.Vg=function(a,b,c,d){this.De=a;if(!c&&!d){if(Oa(a,"?")){this.TT=true;a=a.substring(0,a.length-1)}if(Oa(a,"()"))if(Oa(a,"name()")||Oa(a,"count()")||Oa(a,"position()")){var e=a.lastIndexOf("/");if(e!=-1){this.jA=a.substring(e+1);a=a.substring(0,e)}else{this.jA=a;a="."}if(this.jA=="count()")this.Aca=true}}this.Tb=b||a.split("/");this.tb=this.Tb.length;this.Yl=this.Tb[this.tb-1];this.za=this.Tb[0];if(this.tb==1){this.oO=this;this.gv=Na(a,"$")}else{this.oO=Hn(this.za,null,this,null);this.gv=
this.oO.gv;this.za=this.oO.za}if(this.tb==1&&!this.gv){this.G2=a=="."||a=="";this.N2=Na(a,"@");this.D2=a=="*|text()";this.C2=a=="@*";this.E2=a=="*"}};
Gn.prototype.bc=function(){return this.De};
Gn.prototype.Ia=function(){if(!this.wda){if(this.tb>1)this.gN=Hn(null,this.Tb.slice(0,this.Tb.length-1),this,null);this.wda=true}return this.gN};
Gn.prototype.GJ=function(){if(!this.ida){if(this.tb>1)this.IM=Hn(null,this.Tb.slice(1),null,this);this.ida=true}return this.IM};
Gn.prototype.L=function(a){if(a==null)a=In();else if(this.gv)a=a.gu?a.gu():In();if(this.Aca){var b=this.HJ(a);return b.S()}if(this.tb==1)return a.lc(this.za);else if(this.tb==0)return a.get();var c=a.Na(this.za);return c==null?null:this.GJ().L(c)};
Gn.prototype.HJ=function(a,b){return this.RA(a,false,b)};
Gn.prototype.Gg=function(a,b){return this.RA(a,true,b)};
Gn.prototype.RA=function(a,b,c){if(a==null)a=In();else if(this.gv)a=a.gu?a.gu():In();if(this.tb==0&&b)return a;else if(this.tb==0&&!b)return new Jn([a]);else if(this.tb==1)if(b)return a.Na(this.za,c);else{var d=a.Na(this.za);return d&&d.uf()?d.vb():a.vb(this.za)}else{var e=a.Na(this.za,c);if(e==null&&b)return null;else if(e==null&&!b)return new Kn;return this.GJ().RA(e,b,c)}};
Gn.prototype.TT=false;Gn.prototype.Tb=[];Gn.prototype.tb=null;Gn.prototype.za=null;Gn.prototype.Yl=null;Gn.prototype.G2=false;Gn.prototype.N2=false;Gn.prototype.D2=false;Gn.prototype.C2=false;Gn.prototype.E2=false;Gn.prototype.jA=null;Gn.prototype.gN=null;Gn.prototype.IM=null;var Mn=function(a){var b=Ln[a];if(b==null){b=new Gn(a);Ln[a]=b}return b},
Hn=function(a,b,c,d){var e=a||b.join("/"),f=Ln[e];if(f==null){f=new Gn;f.Vg(e,b,c,d);Ln[e]=f}return f},
Ln={};Mn(".");Mn("*|text()");Mn("*");Mn("@*");Mn("name()");Mn("count()");Mn("position()");var Nn=function(){};
Nn.prototype.get=function(){};
Nn.prototype.J=function(){};
Nn.prototype.vb=function(){};
Nn.prototype.Na=function(){};
Nn.prototype.lc=function(){};
Nn.prototype.Ff=function(){};
Nn.prototype.Fa=function(){};
Nn.prototype.tm=function(){};
Nn.prototype.mb=function(){};
Nn.prototype.ua=function(){};
Nn.prototype.uf=sa;var On="NOT_LOADED",Pn=function(){};
Pn.prototype.J=function(){};
Pn.prototype.vb=function(){return new Kn};
Pn.prototype.Na=function(){return null};
Pn.prototype.lc=function(){return null};
Pn.prototype.mb=function(){var a="",b=this.Fa();if(this.mq&&this.mq())a=this.mq().mb()+(b.indexOf("[")!=-1?"":"/");return a+b};
Pn.prototype.ua=function(){};
Pn.prototype.mq=null;var Qn=function(){};
Qn.prototype.add=function(){};
Qn.prototype.get=function(){};
Qn.prototype.Qf=function(){};
Qn.prototype.S=function(){};
Qn.prototype.Oh=function(){};
Qn.prototype.removeNode=function(){};
var Jn=function(a){this.o={};this.Se=[];this.Zf={};if(a)for(var b=0,c;c=a[b];b++)this.add(c)};
Jn.prototype.add=function(a){this.Se.push(a);var b=a.Fa();if(b!=null){this.o[b]=a;this.Zf[b]=this.Se.length-1}};
Jn.prototype.get=function(a){return this.o[a]||null};
Jn.prototype.Qf=function(a){return this.Se[a]||null};
Jn.prototype.S=function(){return this.Se.length};
Jn.prototype.Oh=function(a,b){if(b==null)this.removeNode(a);else{var c=this.Zf[a];if(c!=null){this.o[a]=b;this.Se[c]=b}else this.add(b)}};
Jn.prototype.removeNode=function(a){var b=this.Zf[a];if(b!=null){this.Se.splice(b,1);delete this.o[a];delete this.Zf[a];for(var c in this.Zf)if(this.Zf[c]>b)this.Zf[c]--}return b!=null};
Jn.prototype.indexOf=function(a){return this.Zf[a]};
var Kn=function(){Jn.call(this)};
o(Kn,Jn);Kn.prototype.add=function(){throw Error("Can't add to EmptyNodeList");};
var Rn=function(a,b){this.qz=a;Jn.call(this,b)};
o(Rn,Jn);Rn.prototype.add=function(a){if(!this.qz){this.F(a);return}var b=yc(this.Se,a,this.qz);if(b<0)b=-(b+1);for(var c in this.Zf)if(this.Zf[c]>=b)this.Zf[c]++;rc(this.Se,a,b);var d=a.Fa();if(d!=null){this.o[d]=a;this.Zf[d]=b}};
Rn.prototype.F=function(a){Rn.f.add.call(this,a)};
Rn.prototype.Oh=function(a,b){if(b==null)this.removeNode(a);else{var c=this.Zf[a];if(c!=null){if(this.qz){var d=this.qz(this.Se[c],b);if(d==0){this.o[a]=b;this.Se[c]=b}else{this.removeNode(a);this.add(b)}}}else this.add(b)}};
var Sn=Fd("goog.ds");var Tn=function(){this.Ip=new Jn;this.wT=new dd;this.YL={};this.Fv={};this.gT={};this.Tba=0;this.pL={}},
Un=null;o(Tn,Nn);var In=function(){if(!Un)Un=new Tn;return Un};
Tn.prototype.op=function(a,b,c){var d=!(!b),e=c||a.Fa();if(!Na(e,"$"))e="$"+e;a.tm(e);this.Ip.add(a);this.wT.J(e,d)};
Tn.prototype.hu=function(a){return this.gT[a]?this.gT[a].Gg():this.Ip.get(a)};
Tn.prototype.get=function(){return this.Ip};
Tn.prototype.J=function(){throw Error("Can't set on DataManager");};
Tn.prototype.vb=function(a){return a?new Jn([this.Na(a)]):this.Ip};
Tn.prototype.Na=function(a){return this.hu(a)};
Tn.prototype.lc=function(a){var b=this.hu(a);return b?b.get():null};
Tn.prototype.Fa=function(){return""};
Tn.prototype.mb=function(){return""};
Tn.prototype.ua=function(){var a=this.Ip.S();for(var b=0;b<a;b++){var c=this.Ip.Qf(b),d=this.wT.get(c.Fa());if(d)c.ua()}};
Tn.prototype.uf=function(){return false};
Tn.prototype.addListener=function(a,b,c){var d=0;if(Oa(b,"/...")){d=1000;b=b.substring(0,b.length-4)}else if(Oa(b,"/*")){d=1;b=b.substring(0,b.length-2)}c=c||"";var e=b+":"+c+":"+Ga(a),f={dataPath:b,id:c,fn:a},g=Mn(b),h=Ga(a);if(!this.Fv[h])this.Fv[h]={};this.Fv[h][e]={listener:f,items:[]};while(g){var i={listener:f,maxAncestors:d},k=this.YL[g.bc()];if(k==null){k={};this.YL[g.bc()]=k}k[e]=i;d=0;g=g.Ia();this.Fv[h][e].items.push({key:e,obj:k})}};
Tn.prototype.OF=function(a,b,c){var d=b.indexOf("*");if(d==-1){this.addListener(a,b,c);return}var e=b.substring(0,d)+"...",f="$";if(Oa(b,"/...")){b=b.substring(0,b.length-4);f=""}var g=vb(b),h=g.replace(/\\\*/g,"([^\\/]+)")+f,i=new RegExp(h),k=function(t){var y=i.exec(t);if(y){y.shift();a(t,c,y)}};
this.addListener(k,e,c);var n=Ga(a);if(!this.pL[n])this.pL[n]={};var q=b+":"+c;this.pL[n][q]={listener:{dataPath:e,fn:k,id:c}}};
Tn.prototype.y6=function(a,b,c){if(b&&Oa(b,"/..."))b=b.substring(0,b.length-4);else if(b&&Oa(b,"/*"))b=b.substring(0,b.length-2);this.z6(this.Fv,false,a,b,c)};
Tn.prototype.z6=function(a,b,c,d,e){var f=Ga(c),g=a[f];if(g!=null)for(var h in g){var i=g[h],k=i.listener;if((!d||d==k.dataPath)&&(!e||e==k.id)){if(b)this.y6(k.fn,k.dataPath,k.id);if(i.items)for(var n=0;n<i.items.length;n++){var q=i.items[n];delete q.obj[q.key]}delete g[h]}}};
Tn.prototype.yc=function(a){if(this.kfa)return;var b=Mn(a),c=0;while(b){var d=this.YL[b.bc()];if(d)for(var e in d){var f=d[e],g=f.listener;if(c<=f.maxAncestors)g.fn(a,g.id)}c++;b=b.Ia()}this.Tba++};var Vn=function(a,b,c){this.P=c;this.Od=b;this.vP(a)};
Vn.prototype.vP=function(a){this.za=a;this.vd=null};
Vn.prototype.get=function(){return!Da(this.za)?this.za:this.vb()};
Vn.prototype.J=function(a){if(a&&Da(this.za))throw Error("Can't set group nodes to new values yet");if(this.P)this.P.za[this.Od]=a;this.za=a;this.vd=null;In().yc(this.mb())};
Vn.prototype.vb=function(a){if(!this.za)return new Kn;if(!a||a=="*"){this.Mf(false);return this.vd}else if(a.indexOf("*")==-1)return this.za[a]!=null?new Jn([this.Na(a)]):new Kn;else throw Error("Selector not supported yet ("+a+")");};
Vn.prototype.Mf=function(a){if(this.vd&&!a)return;if(!Da(this.za)){this.vd=new Kn;return}var b=new Jn;if(this.za!=null){var c;if(xa(this.za)){var d=this.za.length;for(var e=0;e<d;e++){var f=this.za[e],g=f.id,h=g!=null?String(g):"["+e+"]";c=new Vn(f,h,this);b.add(c)}}else for(var h in this.za){var i=this.za[h];if(i.Fa)b.add(i);else if(!Ca(i)){c=new Vn(i,h,this);b.add(c)}}}this.vd=b};
Vn.prototype.Na=function(a,b){if(!this.za)return null;var c=this.vb().get(a);if(!c&&b){var d={};if(xa(this.za)){d.id=a;this.za.push(d)}else this.za[a]=d;c=new Vn(d,a,this);if(this.vd)this.vd.add(c)}return c};
Vn.prototype.lc=function(a){if(this.vd){var b=this.vb().get(a);return b?b.get():null}else return this.za?this.za[a]:null};
Vn.prototype.Ff=function(a,b){var c=null,d=null,e=false;if(b!=null)if(b.Fa){d=b;d.P=this}else d=xa(b)||Da(b)?new Vn(b,a,this):new Wn(this.za,a,this);if(xa(this.za)){this.Mf();var f=this.vd.indexOf(a);if(b==null){var g=this.vd.get(a);if(g)c=g.mb();this.za.splice(f,1)}else if(f)this.za[f]=b;else this.za.push(b);if(f==null)e=true;this.vd.Oh(a,d)}else if(Da(this.za)){if(b==null){this.Mf();var g=this.vd.get(a);if(g)c=g.mb();delete this.za[a]}else{if(!this.za[a])e=true;this.za[a]=b}if(this.vd)this.vd.Oh(a,
d)}var h=In();if(d){h.yc(d.mb());if(e&&this.uf()){h.yc(this.mb());h.yc(this.mb()+"/count()")}}else if(c){h.yc(c);if(this.uf()){h.yc(this.mb());h.yc(this.mb()+"/count()")}}return d};
Vn.prototype.Fa=function(){return this.Od};
Vn.prototype.tm=function(a){this.Od=a};
Vn.prototype.mb=function(){var a="";if(this.P)a=this.P.mb()+"/";return a+this.Od};
Vn.prototype.ua=function(){};
Vn.prototype.uf=function(){return this.Cca!=null?this.Cca:xa(this.za)};
var Wn=function(a,b,c){Pn.call(this);this.Od=b;this.P=a;this.vda=c||this.P};
o(Wn,Pn);Wn.prototype.get=function(){return this.P[this.Od]};
Wn.prototype.J=function(a){var b=this.P[this.Od];this.P[this.Od]=a;if(b!=a)In().yc(this.mb())};
Wn.prototype.Fa=function(){return this.Od};
Wn.prototype.mq=function(){return this.vda};var Xn=function(a,b){Vn.call(this,null,b,null);this.ea=a?new N(a):null};
o(Xn,Vn);Xn.prototype.Te=On;Xn.dataSources={};Xn.prototype.ua=function(){if(this.ea){Xn.dataSources[this.Od]=this;Sn.info("Sending JS request for DataSource "+this.Fa()+" to "+this.ea);this.Te="LOADING";var a=new N(this.ea);a.VD("callback","JsonReceive."+this.Od);j.JsonReceive[this.Od]=l(this.k6,this);var b=w("script");b.src=a;Ne("head")[0].appendChild(b)}else{this.za={};this.Te=On}};
Xn.prototype.k6=function(a){this.vP(a);this.Te="LOADED";In().yc(this.Fa())};
j.JsonReceive={};var Yn=function(a,b){if(!a)throw Error("Cannot create a fast data node without a data name");this.__dataName=a;this.__parent=b};
Yn.prototype.Fa=function(){return this.__dataName};
Yn.prototype.tm=function(a){this.__dataName=a};
Yn.prototype.mb=function(){var a;a=this.__parent?this.__parent.mb()+"/":"";return a+this.Fa()};
var Zn=function(a,b,c){Yn.call(this,b,c);this.sI(a)};
o(Zn,Yn);Zn.prototype.sI=function(a){for(var b in a)this[b]=a[b]};
var bo=function(a,b,c){return xa(a)?new $n(a,b,c):(Da(a)?new Zn(a,b,c):new ao(a,b,c))},
co=new Kn;Zn.prototype.J=function(){throw"Not implemented yet";};
Zn.prototype.vb=function(a){if(!a||a=="*")return this;else if(a.indexOf("*")==-1){var b=this.Na(a);return b?new $n([b],null):new Kn}else throw Error("Unsupported selector: "+a);};
Zn.prototype.hR=function(a){var b=this[a];if(b!=null&&!b.Fa)this[a]=bo(this[a],a,this)};
Zn.prototype.Na=function(a,b){this.hR(a);var c=this[a]||null;if(c==null&&b){c=new Zn({},a,this);this[a]=c}return c};
Zn.prototype.Ff=function(a,b){if(b!=null)this[a]=b;else delete this[a];In().yc(this.mb()+"/"+a)};
Zn.prototype.lc=function(a){var b=this[a];return b!=null?(b.Fa?b.get():b):null};
Zn.prototype.uf=function(){return false};
Zn.prototype.zl=function(){var a={};for(var b in this)if(!Na(b,"__")&&!Ca(this[b]))a[b]=this[b].__dataName?this[b].zl():this[b];return a};
Zn.prototype.ra=function(){return bo(this.zl(),this.Fa())};
Zn.prototype.add=function(a){this.Ff(a.Fa(),a)};
Zn.prototype.get=function(a){return arguments.length==0?this:this.Na(a)};
Zn.prototype.Qf=function(a){var b=0;for(var c in this)if(!Na(c,"__")&&!Ca(this[c])){if(b==a){this.hR(c);return this[c]}++b}return null};
Zn.prototype.S=function(){var a=0;for(var b in this)if(!Na(b,"__")&&!Ca(this[b]))++a;return a};
Zn.prototype.Oh=function(a,b){this.Ff(a,b)};
Zn.prototype.removeNode=function(a){delete this[a]};
var ao=function(a,b,c){this.Ua=a;Yn.call(this,b,c)};
o(ao,Yn);ao.prototype.get=function(){return this.Ua};
ao.prototype.J=function(a){if(xa(a)||Da(a))throw"can only set PrimitiveFastDataNode to primitive values";this.Ua=a;In().yc(this.mb())};
ao.prototype.vb=function(){return co};
ao.prototype.Na=function(){return null};
ao.prototype.lc=function(){return null};
ao.prototype.Ff=function(){throw Error("Cannot set a child node for a PrimitiveFastDataNode");};
ao.prototype.uf=function(){return false};
ao.prototype.zl=function(){return this.Ua};
var $n=function(a,b,c){this.ic=[];for(var d=0;d<a.length;++d){var e=a[d].id||"["+d+"]";this.ic.push(bo(a[d],e,this));if(a[d].id){if(!this.o)this.o={};this.o[a[d].id]=d}}Yn.call(this,b,c)};
o($n,Yn);$n.prototype.J=function(){throw Error("Cannot set a FastListNode to a new value");};
$n.prototype.vb=function(){return this};
$n.prototype.Na=function(a,b){var c=this.ou(a);if(c==null&&this.o)c=this.o[a];if(c!=null&&this.ic[c])return this.ic[c];else if(b){this.Ff(a,{});return this.Na(a)}else return null};
$n.prototype.lc=function(a){var b=this.Na(a);return b?b.get():null};
$n.prototype.ou=function(a){return a.charAt(0)=="["&&a.charAt(a.length-1)=="]"?Number(a.substring(1,a.length-1)):null};
$n.prototype.Ff=function(a,b){var c=this.ic.length;if(b!=null){if(!b.Fa)b=bo(b,a,this);var d=this.ou(a);if(d!=null){if(d<0||d>=this.ic.length)throw Error("List index out of bounds: "+d);this.ic[a]=b}else{if(!this.o)this.o={};this.ic.push(b);this.o[a]=this.ic.length-1}}else this.removeNode(a);var e=In();e.yc(this.mb()+"/"+a);if(this.ic.length!=c)this.aC()};
$n.prototype.aC=function(){var a=In();a.yc(this.mb());a.yc(this.mb()+"/count()")};
$n.prototype.uf=function(){return true};
$n.prototype.zl=function(){var a=[];for(var b=0;b<this.ic.length;++b)a.push(this.ic[b].zl());return a};
$n.prototype.add=function(a){if(!a.Fa)a=bo(a,String(this.ic.length),this);this.ic.push(a);var b=In();b.yc(this.mb()+"/["+(this.ic.length-1)+"]");this.aC()};
$n.prototype.get=function(a){return arguments.length==0?this.ic:this.Na(a)};
$n.prototype.Qf=function(a){var b=this.ic[a];return b!=null?b:null};
$n.prototype.S=function(){return this.ic.length};
$n.prototype.Oh=function(){throw Error("Setting child nodes of a FastListNode is not implemented, yet");};
$n.prototype.removeNode=function(a){var b=this.ou(a);if(b==null&&this.o)b=this.o[a];if(b!=null){this.ic.splice(b,1);if(this.o){var c=null;for(var d in this.o)if(this.o[d]==b)c=d;else if(this.o[d]>b)--this.o[d];if(c)delete this.o[c]}var e=In();e.yc(this.mb()+"/["+b+"]");this.aC()}};
$n.prototype.indexOf=function(a){var b=this.ou(a);if(b==null&&this.o)b=this.o[a];if(b==null)throw Error("Cannot determine index for: "+a);return b};var eo=function(a,b,c,d,e){Zn.call(this,{},b,null);if(a){this.ea=new N(a);this.Ta=new Dj;this.Mea=!(!e);F(this.Ta,Ki,this.kt,false,this)}else this.ea=null;this.Lx=c;this.eA=d};
o(eo,Zn);eo.prototype.Lx=null;eo.prototype.eA=null;eo.prototype.Am=function(a){this.bd=a};
eo.prototype.ua=function(){Sn.info("Sending JS request for DataSource "+this.Fa()+" to "+this.ea);if(this.ea)if(this.Mea){var a;a=!this.bd?this.ea.El():this.bd;var b=this.ea.ra();b.Am(null);this.Ta.send(b,"POST",a)}else this.Ta.send(this.ea);else this.Te=On};
eo.prototype.Nx=function(){In().yc(this.Fa())};
eo.prototype.kt=function(){if(this.Ta.Re()){Sn.info("Got data for DataSource "+this.Fa());var a=this.Ta.Sb();if(this.Lx){var b=a.indexOf(this.Lx);a=a.substring(b+this.Lx.length)}if(this.eA){var c=a.lastIndexOf(this.eA);a=a.substring(0,c)}try{var d=eval("["+a+"][0]");this.sI(d);this.Te="LOADED"}catch(e){this.Te="FAILED";Sn.Zr("Failed to parse data: "+e.message)}j.setTimeout(l(this.Nx,this),0)}else{Sn.info("Data retrieve failed for DataSource "+this.Fa());this.Te="FAILED"}};var fo=function(a,b){if(b&&!a)throw Error("Can't create document with namespace and no root tag");if(document.implementation&&document.implementation.createDocument)return document.implementation.createDocument(b||"",a||"",null);else if(typeof ActiveXObject!="undefined"){var c=new ActiveXObject("MSXML2.DOMDocument");if(c){if(a)c.appendChild(c.createNode(1,a,b||""));return c}}throw Error("Your browser does not support creating new documents");},
go=function(a){if(typeof DOMParser!="undefined")return(new DOMParser).parseFromString(a,"application/xml");else{var b=new ActiveXObject("MSXML2.DOMDocument");b.loadXML(a);return b}};var ho=function(a,b,c){this.P=b;this.Od=c||(a?a.nodeName:null);this.T8(a)};
ho.prototype.T8=function(a){this.Gh=a;if(a!=null)switch(a.nodeType){case 2:case 3:this.Ua=a.nodeValue;break;case 1:if(a.childNodes.length==1&&a.firstChild.nodeType==3)this.Ua=a.firstChild.nodeValue}};
ho.prototype.Mf=function(){if(this.vd)return;var a=new Jn;if(this.Gh!=null){var b=this.Gh.childNodes;for(var c=0,d;d=b[c];c++)if(d.nodeType!=3||!/^[\r\n\t ]*$/.test(d.nodeValue)){var e=new ho(d,this,d.nodeName);a.add(e)}}this.vd=a};
ho.prototype.SU=function(){if(this.sT)return;var a=new Jn;if(this.Gh!=null&&this.Gh.attributes!=null){var b=this.Gh.attributes;for(var c=0,d;d=b[c];c++){var e=new ho(d,this,d.nodeName);a.add(e)}}this.sT=a};
ho.prototype.get=function(){this.Mf();return this.Ua!=null?this.Ua:this.vd};
ho.prototype.J=function(){throw Error("Can't set on XmlDataSource yet");};
ho.prototype.vb=function(a){if(a&&a=="@*"){this.SU();return this.sT}else if(a==null||a=="*"){this.Mf();return this.vd}else throw Error("Unsupported selector");};
ho.prototype.Na=function(a){if(Na(a,"@")){var b=this.Gh.getAttributeNode(a.substring(1));return b?new ho(b,this):null}else return this.vb().get(a)};
ho.prototype.lc=function(a){if(Na(a,"@")){var b=this.Gh.getAttributeNode(a.substring(1));return b?b.nodeValue:null}else{var b=this.Na(a);return b?b.get():null}};
ho.prototype.Fa=function(){return this.Od};
ho.prototype.tm=function(a){this.Od=a};
ho.prototype.mb=function(){var a="";if(this.P)a=this.P.mb()+(this.Od.indexOf("[")!=-1?"":"/");return a+this.Od};
ho.prototype.ua=function(){};
var io=function(){return fo("nothing")},
jo=function(a,b){ho.call(this,null,null,b);this.ea=a?new N(a):null};
o(jo,ho);jo.prototype.Te=On;jo.prototype.ua=function(){if(this.ea){Sn.info("Sending XML request for DataSource "+this.Fa()+" to "+this.ea);this.Te="LOADING";this.Pca=new ko(this.ea,l(this.Nx,this),l(this.cX,this))}else{this.Gh=io();this.Te=On}};
jo.prototype.Nx=function(){Sn.info("Got data for DataSource "+this.Fa());var a=this.Pca.sZ(),b=a.responseXML;if(b&&!b.hasChildNodes()&&Da(a.responseText))b=go(a.responseText);if(!b||!b.hasChildNodes()){this.Te="FAILED";this.Gh=io()}else{this.Te="LOADED";this.Gh=b.documentElement}if(this.Fa())In().yc(this.Fa())};
jo.prototype.cX=function(){Sn.info("Data retrieve failed for DataSource "+this.Fa());this.Te="FAILED";this.Gh=io();if(this.Fa())In().yc(this.Fa())};
var ko=function(a,b,c){this.Co=null;this.oda=b;this.K4=c||this.t_;this.ea=new N(a);this.r3()};
ko.prototype.r3=function(){this.Co=new mj;if(this.Co)try{this.Co.onreadystatechange=l(this.xr,this);this.Co.open("GET",String(this.ea),true);this.Co.send(null)}catch(a){this.K4.call(this)}};
ko.prototype.xr=function(){var a=this.Co,b=a.readyState;if(b==4){var c=a.status,d;d=c==200||c==0?l(this.O4,this):l(this.L4,this);window.setTimeout(d,10)}};
ko.prototype.O4=function(){this.oda(this)};
ko.prototype.L4=function(){this.K4(this)};
ko.prototype.t_=function(){throw Error("Error fetching data from URL: "+this.ea);};
ko.prototype.sZ=function(){return this.Co};var lo=function(){this.Iw=l(this.nw,this);this.Zt=new Sd;this.Zt.$D=false;this.Zt.as=false;this.iv=false;this.Uca=""};
lo.prototype.fx=function(a){if(a==this.iv)return;var b=Jd();if(a)b.NF(this.Iw);else{b.TN(this.Iw);this.logBuffer=""}};
lo.prototype.nw=function(a){var b=this.Zt.Dn(a);if(window.console&&window.console.firebug)switch(a.jq()){case wd:window.console.info(b);break;case xd:window.console.error(b);break;case yd:window.console.warn(b);break;default:window.console.debug(b);break}else if(window.console)window.console.log(b);else this.Uca+=b};
var mo=null,no=function(){if(!mo)mo=new lo;if(document.location.href.indexOf("Debug=true")!=-1)mo.fx(true)};var oo={},po=function(a,b){var c=window._Messages||[],d=a;for(var e=0;e<c.length;e++)if(c[e][a]){d=c[e][a];break}var f=b||{},g=function(h,i){return i in f?f[i]:h};
d=d.replace(new RegExp("\\{\\$(\\w+)\\}","g"),g);return d},
qo=function(a,b){document.write(po(a,b))};
La("_cp_getMsg",po);La("_cp_writeMsg",qo);var ro=function(a){this.no={};this.ES(a)};
ro.prototype.ES=function(a){for(var b=0;b<a.length;b++)this.DS(a[b])};
ro.prototype.DS=function(a){var b=this.rJ(a);this.oL(b[0]);if(b[1])this.oL(this.qJ(b[0],b[1]))};
ro.prototype.oL=function(a){this.no[a]=this.no[a]?this.no[a]+1:1};
ro.prototype.rJ=function(a){return Va(a).split(/[\s,]+/)};
ro.prototype.wu=function(a){var b=this.rJ(a),c=b[0];if(!this.no[c]||this.no[c]<=1)return c;if(b[1]){c=this.qJ(b[0],b[1]);if(!this.no[c]||this.no[c]<=1)return c}return a};
ro.prototype.qJ=function(a,b){return a+" "+b.charAt(0).toLocaleUpperCase()+"."};
ro.prototype.xX=function(a,b,c,d){var e=vb(c),f=new RegExp('(^|<| |"|\\()'+e,"i");if(a&&f.test(a))return a;if(f.test(b))return this.bI(b,d);if(a)return this.wu(a);return this.bI(b,d)};
ro.prototype.bI=function(a,b){if(b)return a.split("@")[0];return a};var so=function(a,b,c){zn.call(this,a,b,c)};
o(so,zn);so.prototype.ax=function(a){if(a instanceof to)return;if(a.sl)a=a.sl().join(", ");so.f.ax.call(this,a)};var vo=function(a,b){this.cca=a;this.rda=b;this.Sk=[];this.vq=new uo([])};
vo.prototype.i8=function(a,b){this.Sk=a;this.vq=new uo(b)};
var xo=function(a,b,c,d){var e=a.members,f=[],g=[],h=[];for(var i=0;i<e.length;++i){var k=e[i].email;if(c.hasContext&&c.addressHash[k])g.push(e[i]);else f.push(e[i]);if(d[k])h.push(e[i])}return new wo(a,b,e,f,g,h)};
vo.prototype.pk=function(a,b,c){var d=[];if(a=="")return[];if(c){var e="";for(var f=0;f<c.length;++f){var g=c.charAt(f);if(g==";"||g==","){e=Va(e);if(e.length>0)d.push(e);e=""}else e+=g}e=Va(e);if(e.length>0)d.push(e)}return this.lX(a,d,b)};
vo.prototype.fW=function(a){var b={};for(var c=0;c<a.length;++c){var d=a[c],e=yo(d);if(e.email)b[e.email]=true}var f={},g=[],h={},i=false,k=0,n={},q=true;for(var c=0;c<this.Sk.length;++c){var t=this.Sk[c],y=t.email;if(b[y]){h[y]=true;i=true;k++;var z=t.groups;if(q){for(var O=0;O<z.length;++O){var aa=z[O];f[aa.id]=1;g.push(aa.id)}q=false}else for(var O=0;O<z.length;++O){var aa=z[O];if(f[aa.id])f[aa.id]++}}}for(var c=0;c<g.length;++c){var dc=g[c];if(f[dc]==k)n[dc]=true}return{hasContext:i,groupIdHash:n,
addressHash:h}};
vo.prototype.U5=function(a,b){var c={},d=[];for(var e=0;e<a.length;++e){var f=a[e].groups;if(f)for(var g=0;g<f.length;++g){var h=f[g].id;if(!f[g].implicit)continue;if(b.hasContext&&!b.groupIdHash[h])continue;if(h.charAt(0)!="^")if(!c[h]){var i=this.vq.get(h);if(i){c[h]=true;d.push(i)}}}}d.sort(function(k,n){return k.affinity>n.affinity?-1:(n.affinity>k.affinity?1:0)});
return d};
var zo=function(){};
zo.prototype.aH=function(){var a=arguments,b=function(c,d){var e,f;for(e=0;e<a.length;e++){f=a[e](c,d);if(f!=0)return f}return 0};
return b};
zo.prototype.vG=function(a,b){return a.bu()>b.bu()?-1:(b.bu()>a.bu()?1:0)};
zo.prototype.wG=function(a,b){var c=a.wb().name||"",d=b.wb().name||"";if(c>d)return 1;else if(d>c)return-1;return 0};
zo.prototype.LT=function(a,b){var c=function(e){if(e.Xa()!=0)return false;return!e.MA()},
d=function(e){return e.Xa()==1};
if(c(a)&&!c(b))return-1;if(!c(a)&&c(b))return 1;if(d(a)&&!d(b))return-1;if(!d(a)&&d(b))return 1;return 0};
vo.prototype.eV=function(a){var b={};for(var c=0;c<a.length;++c)if(a[c].email)b[a[c].email]=true;return b};
vo.prototype.lX=function(a,b,c){var d=this.jX(a),e=[],f=[];if(this.cca){f=this.kX(a,c);var g={hasContext:false};if(b.length>0)g=this.fW(b);var h=this.U5(d,g,c),i=this.eV(d);for(var k=0;k<c&&k<h.length;++k){var n=h[k];e.push(xo(n,n.affinity,g,i))}}for(var k=0;k<c&&k<f.length;++k){var n=f[k];e.push(xo(n,20000000,g,i))}for(var k=0;k<c&&k<d.length;++k){var q=d[k],t=q.affinity;if(k==0)t=10000000;e.push(new Ao(q,t))}var y=new zo,z=y.aH(y.vG,y.wG);e.sort(z);if(e.length>c)e.splice(c,e.length-c);if(this.rda){var O=
y.aH(y.LT,y.vG,y.wG);e.sort(O)}return e};
vo.prototype.kX=function(a,b){var c=vb(a),d=new RegExp('(^|<| |"|\\()'+c,"i"),e=[],f=this.vq.eZ();for(var g=0;g<f.length&&e.length<b;++g){var h=f[g];if(h.name&&h.name.match(d))e.push(h)}return e};
vo.prototype.jX=function(a){var b=vb(a),c=new RegExp('(^|<| |"|\\()'+b,"i"),d=[],e=this.Sk;for(var f=0;f<e.length;++f){var g=e[f];if(c.test(g.toString()))d.push(g)}return d};
var Bo=function(){};
Bo.prototype.sl=function(){return[]};
Bo.prototype.Hn=function(){return[]};
Bo.prototype.Xa=function(){return this.da};
Bo.prototype.bu=function(){return this.aT};
Bo.prototype.wb=function(){return this.Xd};
var Ao=function(a,b){this.da=0;this.Xd=a;this.aT=b};
o(Ao,Bo);Ao.prototype.sl=function(){return[this.Xd.toString()]};
Ao.prototype.Hn=function(){return this.Xd.name?[this.Xd.name]:[]};
Ao.prototype.MA=function(){return!(!this.Xd.isDomainContact)};
var wo=function(a,b,c,d,e,f){this.da=1;this.Xd=a;this.aT=b;this.fB=c;this.hda=d;this.gfa=e;this.Zca=f;this.T={};for(var g=0;g<e.length;++g)this.T[e[g].email]=true};
o(wo,Bo);wo.prototype.sl=function(){return this.hda};
wo.prototype.Hn=function(){var a=[];for(var b=0;b<this.fB.length;++b)if(this.fB[b].name)a.push(this.fB[b].name);return a};
wo.prototype.getContext=function(){return this.T};
wo.prototype.XY=function(){return this.fB};
wo.prototype.QA=function(){return this.Zca};
var to=function(a){this.da=a};
to.prototype.toString=function(){return""};
to.prototype.Xa=function(){return this.da};
var uo=function(a){this.bN=[];this.E1=new dd;for(var b=0;b<a.length;++b){var c=a[b];this.bN.push(c);this.E1.J(c.id,c)}this.bN.sort(function(d,e){if(d.affinity>e.affinity)return-1;else if(e.affinity>d.affinity)return 1;else{var f=d.affinity||"",g=e.affinity||"";if(f>g)return 1;else if(g>f)return-1;return 0}})};
uo.prototype.get=function(a){return this.E1.get(a,null)};
uo.prototype.eZ=function(){return this.bN};var Co=function(){An.apply(this,arguments)};
o(Co,An);Co.prototype.Ny=function(a,b,c,d){var e;if(b=="MSG_TITLE_CONTACTS")e="People";else if(b=="MSG_TITLE_GROUPS")e="Groups";else if(b=="MSG_TITLE_DOMAIN")e=d+" contacts";var f=new Do(a,e);c.push({data:f,id:-1})};
Co.prototype.N5=function(){if(this.ca.length==0)return;if(this.tj&&this.tj.FD){var a=new hd;for(var b=0;b<this.ca.length;b++)if(this.ca[b].data.Hn)a.jj(this.ca[b].data.Hn());this.tj.FD(new ro(a.xb()))}var b,c=[],d=false,e=false,f=false;for(b=0;b<this.ca.length;b++){var g=this.ca[b],h=g.data;if(h.Xa()==0){if(!d&&h.MA()==false){this.Ny("ContactIcon","MSG_TITLE_CONTACTS",c,null);d=true}if(!f&&h.MA()==true){var i=h.wb(),k=i.email,n=Eo(k);this.Ny("ContactIcon","MSG_TITLE_DOMAIN",c,n);f=true}}if(!e&&h.Xa()==
1){this.Ny("GroupIcon","MSG_TITLE_GROUPS",c,null);e=true}c.push(this.ca[b])}this.ca=c};
Co.prototype.Nw=function(a,b){var c=a.data.Xa()=="TITLE",d=c?null:b;return Co.f.Nw.call(this,a,d)};
Co.prototype.redraw=function(){var a=false;this.N5();var b=this.ca.length;if(b>0&&this.ca[b-1].data.Xa&&this.ca[b-1].data.Xa()=="MORE")a=true;Co.f.redraw.call(this);if(a){var c=po("SEARCHING_FOR_MATCHES");if(b>1){var d=this.K.createElement("div"),e=this.K.m("div",{style:"background-color: #A0B0FF; margin: 8px 0px"});if(s){var f=this.K.m("img",{width:"1",height:"1"});e.appendChild(f)}else e.style.height="1px";d.appendChild(e);this.e.appendChild(d);c=po("SEARCHING_FOR_MORE_MATCHES")}var g=this.K.m("div");
g.innerHTML=c;this.e.appendChild(g)}};
var Fo=function(a,b){this.Em=a;this.xQ=b;this.Fm=v("div",{style:"position:absolute; top:-100px; left:-1000px;"},"");x(Ke().body,this.Fm)};
Fo.prototype.jC=function(a){B(this.Fm,a);return this.Fm.offsetWidth};
var Eo=function(a){if(!a)return null;var b=a.indexOf("@");if(b<1)return null;var c=a.substr(b+1,a.length-b-1);return c};
Fo.prototype.FD=function(a){this.FU=a};
Fo.prototype.aO=function(a,b,c,d,e){var f="",g=0;if(!this.FU)this.FD(new ro([]));for(var h=0;h<a.length;++h){var i=a[h],k=i.name,n=i.email,q=this.FU.xX(k,n,e,b),t=this.jC(q+", ");if(g+t<c||h<d){if(h>0)f+=", ";f+=p(q);g+=t}else break}return{result:f,count:h,width:g}};
Fo.prototype.D6=function(a,b){var c=b.wb().name,d=b.XY(),e=true,f="";if(d.length>0){f=Eo(d[0].email);for(var g=1;g<d.length&&e;++g)e=e&&Eo(d[g].email)==f}var h=400,i=0;if(!this.xQ&&c)i=this.jC("&lt;"+c+"&gt; ");if(e)i+=this.jC("@"+f);var k=2,n=this.aO(b.QA(),e,h-i,k,a),q=n.count;i+=n.width;var t=function(Re){return!mc(b.QA(),Re)},
y=fc(b.sl(),t),z=this.aO(y,e,h-i,k-q,a);q+=z.count;i+=z.width;var O=n.result;if(n.count>0&&z.count>0)O+=", ";O+=z.result;var aa=b.QA().length+y.length;if(q<aa){var dc=po("AND_OTHERS",{count:aa-q});O+=" "+dc}if(e)O+="@"+f;return O};
Fo.prototype.J6=function(a,b){if(!this.Em)return;var c=[],d=a.data,e=d.HY();if(window.manifest&&window.manifest.image&&window.manifest.image[e])e=window.manifest.image[e];else e+=".png";c.push("<table><tr>");c.push('<td style="padding: 1px;" width="15">');c.push('<img src = "'+e+'"></td>');c.push('<td style="padding: 1px; color: #666666">');c.push(p(d.$A()));c.push("</td></tr></table>");b.innerHTML=c.join("")};
Fo.prototype.Mw=function(a,b,c){var d=a.data;if(d instanceof to)return;if(d.Xa()=="TITLE"){this.J6(a,c);return}var e=[],f=d.Xa()==1;if(this.Em){e.push("<table><tr>");e.push('<td style="padding: 1px;" width="15">');e.push("</td>");e.push('<td style="padding: 1px;">')}var g=d.wb().name;if(g)e.push('"'+p(g)+'"');if(this.xQ&&g)e.push("<br>");if(!f){if(!this.xQ&&g)e.push(" ");if(g)e.push("&lt;");e.push(p(d.wb().email));if(g)e.push("&gt;")}else if(!g||this.Em){if(g)e.push(" (");e.push(this.D6(b,d));if(g)e.push(")")}if(this.Em)e.push("</td></tr></table>");
c.innerHTML=e.join("")};
var Do=function(a,b){this.pca=a;this.Gea=b};
Do.prototype.Xa=function(){return"TITLE"};
Do.prototype.HY=function(){return this.pca};
Do.prototype.$A=function(){return this.Gea};var Go=function(){this.CB=false;this.Sk=[];this.ffa=[];this.Bc=null;this.$f=[];this.vq=[];this.YW={};this.jb=null;this.pa=null;this.io=null;this.JP=false;this.d2=false;this.Em=false;this.Hz=null;this.oea=true;this.P$={};this.O3=250;this.M9=false},
Ho="[A-Za-z0-9!#\\$%\\*\\/\\?\\|\\^\\{\\}`~&'\\+\\-=_]",Io="(?:"+Ho+"(?:[A-Za-z0-9!#\\$%\\*\\/\\?\\|\\^\\{\\}`~&'\\+\\-=_\\.]*"+Ho+")?@[\\.A-Za-z0-9\\-]+)",Jo=new RegExp("^"+Io+"$"),Ko=new RegExp("^(.+?)??(?:<("+Io+")>)?,?$"),Lo=function(a){return Mn("Emails/[0]/Address").L(a)||Mn("Email").L(a)},
Mo=function(a,b){var c=new Go;if(!b.uri&&b.serverBase)b.uri=b.serverBase+"data/contacts";c.create(a,b);return c};
j._EmailAc_create=Mo;Go.prototype.create=function(a,b){var c=u(a);if(b.groups){this.JP=true;this.d2=!(!b.implicitGroups)}if(b.extendedInterface)this.Em=true;if(b.debugData)this.Hz=b.debugData;if(b.inputHandler)this.Bc=b.inputHandler;if(b.max)this.N8(b.max);var d=new N(b.uri);this.Vd(c,d.toString(),true,b.rightAlign,b.twoLine)};
Go.prototype.b=function(){this.Bc.b();this.pa.b()};
Go.prototype.TU=function(){return new yn(this,this.pa,this.Bc)};
Go.prototype.Tf=function(){return this.pa};
Go.prototype.N8=function(a){this.O3=a<0?10100:a};
Go.prototype.Vd=function(a,b,c,d,e,f){if(this.CB){Sn.warning("Init already called");return}this.CB=true;this.ea=b;this.io=new vo(this.JP,this.Em);if(!this.Hz)this.$L("$Contacts",l(this.kK,this),null,false);var g;g=xa(a)?a[0].ownerDocument.body:a.ownerDocument.body;var h=new Fo(this.Em,e),i=new Co(g,h,d);i.Y7(true);this.pa=i;if(!this.Bc)this.Bc=new so(",;",'"',f);this.jb=this.TU();this.Bc.Uy(this.jb);if(this.Hz){var k=In();k.op(this.Hz.contacts);this.kK("$Contacts")}if(a)this.LS(a)};
Go.prototype.$L=function(a,b,c,d){var e=new N(this.ea);e.fa("out","js");if(c){e.fa("tok",c);e.fa("cl",false);e.fa("psort","Name")}if(d)e.fa("cd","true");if(this.JP)e.fa("groups","true");if(this.d2)e.fa("igroups","true");var f=c?10:this.O3;e.fa("max",f);var g=new eo(e,a,"&&&START&&&","&&&END&&&"),h=In();h.op(g,true);h.addListener(b,a);g.ua()};
Go.prototype.LS=function(a){if(!this.CB){Sn.warning("Init should be called first");return}if(xa(a)){this.$f=this.$f.concat(a);for(var b=0;b<a.length;b++)this.Bc.Vy.call(this.Bc,a[b])}else{this.$f.push(a);this.Bc.Vy.call(this.Bc,a)}};
Go.prototype.u6=function(a){if(!this.CB){Sn.warning("Init should be called first");return}if(xa(a))r(a,function(b){this.u6(b)},
this);else{tc(this.$f,a);this.Bc.eW(a)}};
Go.prototype.pk=function(a,b,c,d){this.HV=a;var e=[];if(a!=""){if(!this.io)return;e=this.io.pk(a,b,d)}if(this.M9&&a)if(!this.QK(a,b,true))if(e.length<b){var f=e.concat([]);j.setTimeout(l(this.SX,this,f,b,a,c),500);e.push(new to("MORE"))}c(a,e)};
Go.prototype.QK=function(a,b,c){var d=this.P$[a];return d&&(c||d.count<b)?true:(a.length>1?this.QK(a.substring(0,a.length-1),b,false):false)};
var No=0;Go.prototype.SX=function(a,b,c,d){if(c==this.HV&&c!=""){var e="$AdditionalContacts"+No++,f=l(this.UZ,this,a,b,c,d);this.$L(e,f,c,true)}};
Go.prototype.UZ=function(a,b,c,d,e){var f=[].concat(a);if(c==this.HV){var g=this.QI(e),h={};for(var i=0;i<a.length;i++)if(a[i].Xa()==0){var k=a[i].wb().email;h[k]=true}i=0;while(f.length<b&&i<g.S()){var n=g.Qf(i),k=Lo(n);if(!h[k]){var q=this.mH(n);f.push(new Ao(q,q.affinity,true));this.YW[k]=true;this.Sk.push(q)}i++}d(c,f,true);this.P$[c]={count:f.length}}};
Go.prototype.QI=function(a){var b=In().hu(a),c=b.Na("Body",true).Na("Contacts",true).vb();return c};
Go.prototype.tG=function(a,b,c,d,e){if(d!=null){var f=d.length;for(var g=0;g<f;g++){var h=d[g].id,i=a.get(h);if(i){if(e)i.implicit=true;i.members.push(c);if(h.charAt(0)!="^")b.push(i)}}}};
Go.prototype.kK=function(a){var b=[],c=[],d=new dd,e=In().hu(a),f=e.Na("Body").Na("Groups");if(f!=null){var g=f.vb();t=g.S();Sn.info("Got "+t+" groups");for(var h=0;h<t;h++){var i=g.Qf(h),k=i.lc("id");if(k.charAt(0)=="^"||i.IsSystem=="true")continue;var n=this.iV(i);c.push(n);d.J(k,n)}}Sn.info("Created groups array");var q=this.QI(a),t=q.S();Sn.info("Got "+t+" contacts");if(t>0)for(var h=0;h<this.$f.length;h++)this.$f[h].setAttribute("autocomplete","off");for(var h=0;h<t;h++){var y=q.Qf(h),z=Lo(y);
if(!z&&this.oea)continue;var g=y.lc("Groups"),O=y.lc("ImplicitGroups"),aa=[],dc=this.mH(y);this.tG(d,aa,dc,g,false);this.tG(d,aa,dc,O,true);if(g!=null||O!=null)dc.groups=aa;b.push(dc);this.YW[z]=true}Sn.info("Created contacts array");var Re=e.Na("Body").Na("UserData");if(Re)this.M9=Re.lc("ShowDomainContacts");this.Sk=b;this.vq=c;this.io.i8(this.Sk,this.vq);Sn.info("Set data source for matcher")};
Go.prototype.mH=function(a){var b=a.Id,c=a.Name,d=a.Affinity,e=!(!a.DomainContact);if(!d)d=0;var f=Lo(a),g={};g.data=a;g.id=b;g.name=c;g.email=f;g.isDomainContact=e;g.affinity=d;var h=Oo(c,f);g.formattedValue=h;g.groups=[];g.toString=function(){return this.formattedValue};
return g};
Go.prototype.iV=function(a){return{members:[],data:a,id:a.id,affinity:a.Affinity||0,name:a.Name,toString:function(){return this.name}}};
var _emailAutocomplete=j._emailAutocomplete=new Go;j._initEmailAutocomplete=l(j._emailAutocomplete.Vd,j._emailAutocomplete);var _initEmailAutocomplete=j._initEmailAutocomplete,Oo=function(a,b){var c=a?'"'+a+'"':"",d=b?" <"+b+">":"";return c+d},
yo=function(a){var b=null,c=null;a=Va(a);var d=a.match(Ko);if(d){b=d[1]||null;c=d[2]||null}if(b&&!c){var e=b.match(Jo);if(e){b="";c=e[0]}}if(b){b=Va(b);if(Na(b,'"')&&Oa(b,'"'))b=b.substr(1,b.length-2)}if(c)c=Va(c);return{name:b,email:c}};var Po=function(){this.Pn={};this.yea={};this.Yba={}},
Qo=null,Ro=function(){if(!Qo)Qo=new Po;return Qo};
Po.prototype.fireEvent=function(a){var b=null;if(za(a)){b=new D(a);var c=arguments[1];if(za(c)){var d=2,e=c;while(e){var f=arguments[d++];b[e]=f;e=arguments[d++]}}else for(var g in c)b[g]=c[g]}else if(a instanceof D)b=a;this.T5(b)};
Po.prototype.T5=function(a){var b=a.type,c=this.Pn[b];if(c){var d=c(a),e;e=d?this.yea[b]||[]:this.Yba[b]||[];for(var f=0;f<e.length;f++)e[f](a)}};
Po.prototype.Zc=function(a,b){this.Pn[a]=b};var To=function(a,b){var c=b||window;if(a=="Parent"){this.cQ="ParentStub"+Number(new Date);this.mm="ChildStub"}else{this.cQ="ChildStub";this.mm=this.jZ();this.so=c.opener||c.parent;if(this.mm&&!this.so[this.mm]){var d=0;while(this.so&&!this.so[this.mm]&&d<10){this.so=this.so.parent;d++}}c[So]=this}c[this.cQ]=this.j$.bind(this);this.da=a};
To.inherits(J);var Uo=function(a){var b=a||window;return b[So]};
To.prototype.c8=function(a,b){this.so=a;this.ec=b};
var So="__topLevelProxy__";To.prototype.type=null;To.prototype.jZ=function(){var a=new N(document.location.href);return a.Ga("eventCallback")};
To.prototype.dK=function(){return this.cQ};
To.prototype.on=function(a){return this.Z5(a)};
To.prototype.Z5=function(a){a.type=a.type;var b=this.ec?this.ec.contentWindow:null,c=this.so||b;return this.mm&&c&&c[this.mm]?c[this.mm](a):true};
To.prototype.j$=function(a){var b=new D(a.type,this);for(var c in a)if(!(c in b))b[c]=a[c];var d=J.prototype.dispatchEvent.call(this,b);return d};var Vo=function(){};
Vo.inherits(J);Vo.prototype.tP=function(a){this.Kda=a};
Vo.prototype.dispatchEvent=function(a){if(this.Kda.on(a))J.prototype.dispatchEvent.call(this,a)};var Wo=function(a){To.call(this,"Parent",a||window)};
Wo.inherits(To);var Xo=null,Yo=function(a){switch(a){case "NONE":return"0";case "THIN":return"1px solid black";case "OUTSET":return"2px outset black"}},
$o=function(a,b,c){var d=b||{};d.border=d.border||"OUTSET";var e=new Zo(c);e.$m(a,d,null);return e},
bp=function(a,b,c,d){var e=b||{};e.border=e.border||"NONE";var f=u(c),g=new ap(d);g.$m(a,e,f);return g};
Wo.prototype.$m=function(){};
Wo.prototype.show=function(){};
Wo.prototype.hide=function(){};
Wo.prototype.b=function(){};
var cp=function(a,b){var c=b.serverbased?"widgets":"ui",d=new N((Xo?Xo:(j.manifest?j.manifest.serverBase:""))+"/"+c+"/"+a),e=new N(document.location.href);if(e.Ga("js"))d.fa("js",e.Ga("js"));else d.fa("js","RAW");if(b)ad(b,function(f,g){if(f!=null)d.fa(g,f)});
return d};
var dp=0;La("createIframeComponent",bp);La("createIframeDialogComponent",$o);var ap=function(a){Wo.call(this,a)};
ap.inherits(Wo);ap.prototype.ec=null;var ep=function(a,b,c,d){var e="hosted-"+dp++ +"-"+Number(new Date),f=Le(e);if(!f){var g=new Ge(d?Ie(d):document);f=g.m("iframe",{id:e});L(f,"display","block");L(f,"backgroundColor","#FFF");L(f,"border",Yo(c));L(f,"margin","0");L(f,"padding","0");L(f,"overflow","hidden");zh(f,a,b)}return f};
ap.prototype.$m=function(a,b,c){this.ea=cp(a,b);this.iba=b.border;this.la=!(b.hide=="true");this.l4=b.mock;this.hF=b.x||20;this.kR=b.y||20;this.fp=b.width||600;this.Qu=b.height||480;this.ada=b.maximize=="true";this.Pi=b.position||"relative";this.eba=b.background;this.Nm=b.zIndex;this.Wu=b.hl;this.KC=c;var d=ep(this.fp,this.Qu,this.iba,this.KC);d.style.position=this.Pi;if(this.Nm)d.style.zIndex=this.Nm;if(this.eba){d.allowtransparency=true;d.style.backgroundColor="transparent"}if(this.Wu)this.ea.fa("hl",
this.Wu);if(!this.l4){this.ea.fa("eventCallback",this.dK());this.ea.fa("js","RAW");this.ea.D3();d.src=this.ea.toString();d.id=xb()}this.fA(d);if(this.l4){new To("Child",d.contentWindow);d.contentWindow[So].mm=this.dK()}this.c8(d.contentWindow,d)};
ap.prototype.fA=function(a){this.ec=a;if(this.KC){this.KC.appendChild(a);this.XE()}else{a.style.position="absolute";document.body.appendChild(a)}if(a.style.position=="absolute"){var b=this.la?this.hF:this.hF-4000;yh(a,b,this.kR)}else{this.XE();if(!this.la)a.style.visibility="hidden"}if(window.frames[a.id])if(window.frames[a.id].location!=a.src)window.frames[a.id].location=a.src};
ap.prototype.XE=function(){var a=this.Ofa||this.KC;if(a&&this.ada){var b=Ah(a);zh(this.ec,b);if(this.Pi=="absolute"){var c=uh(a);if(!this.la)c.x-=4000;this.ec.style.left=c.x+"px";this.ec.style.top=c.y+"px"}}};
ap.prototype.show=function(){this.la=true;this.XE();this.on("PRESHOW");var a=this;j.setTimeout(function(){a.ec.style.display="block";a.ec.style.visibility="visible";a.on("SHOW")},
5)};
ap.prototype.hide=function(){this.la=false;if(this.Pi=="absolute"){var a=uh(this.ec);this.ec.style.left=a.x-4000+"px";this.ec.style.visibility="hidden"}else this.ec.style.display="none"};
ap.prototype.b=function(){if(this.ec){this.ec.parentNode.removeChild(this.ec);this.ec=null}Wo.prototype.b.call(this)};var Zo=function(a){ap.call(this,a)};
Zo.inherits(ap);Zo.prototype.fA=function(a){if(se)C(document.body,"background");var b=new R;b.Oa().appendChild(a);b.Rg(new Xk);b.M(this.la);this.ef=b;this.ec=a};
Zo.prototype.show=function(){this.ef.M(true)};
Zo.prototype.hide=function(){this.ef.M(false)};
Zo.prototype.b=function(){this.ef.b();qf(document.body,"background");this.ef=null;this.ec=null;ap.prototype.b.call(this)};var fp=function(){this.uH={}};
fp.prototype.uH=null;var gp=null,hp=function(){if(!gp)gp=new fp;return gp};
fp.prototype.VI=function(a){return this.uH[a]};
fp.prototype.create=function(a,b,c){var d=this.VI(a);if(d){if(b)b=u(b);return d(b,c,a)}};
fp.prototype.lH=function(a){var b=this.VI(a);if(b){var c={},d=(new N(document.location.href)).El();ad(d,function(g,h){c[h]=g});
var e=new To("Child"),f=b(document.body,c,a);if(f.tP)f.tP(e)}};
fp.prototype.cz=function(a){var b=a||document.body,c=b.getAttribute("g:type");if(c){var d={},e=b.attributes;for(var f=0;f<e.length;f++){var g=e[f].name;if(Na(g,"g:"))d[g.substring(2)]=e[f].value}this.create(c,b,d)}var h=b.childNodes.length;for(var f=0;f<h;f++){var i=b.childNodes[f];if(i.nodeType==1)this.cz(i)}};
fp.prototype.zI=function(a){var b=Uo();if(b){if(za(a))a=new D(a);return b.on(a)}else return true};var ip=function(){La("Xuit",hp());Ma(fp.prototype,"buildUi",fp.prototype.cz);Ma(fp.prototype,"create",fp.prototype.create);Ma(fp.prototype,"createAsFullPage",fp.prototype.lH);Ma(fp.prototype,"fireHostedEvent",fp.prototype.zI);Ma(Wo.prototype,"hide",Wo.prototype.hide);Ma(Wo.prototype,"show",Wo.prototype.show);Ma(Wo.prototype,"dispose",Wo.prototype.b)};
ip();var jp=function(a){if(a)this.Vg(a)},
kp={},lp=function(a){var b=kp[a];if(b==null){b=new jp(a);kp[a]=b}return b},
mp=function(a){var b=new jp;b.Ua=a;return b};
jp.prototype.Vg=function(a){this.De=a;if(a=="TRUE"||a=="true")this.Ua=true;else if(a=="FALSE"||a=="false")this.Ua=false;else if(Na(a,'"')&&Oa(a,'"'))this.Ua=a.substring(1,a.length-1);else if(Na(a,"'")&&Oa(a,"'"))this.Ua=a.substring(1,a.length-1);else if(!isNaN(a))this.Ua=Number(a);else throw Error("Invalid static expression: "+a);};
jp.prototype.L=function(){return this.Ua};
jp.prototype.jd=function(){return[]};var np=function(a){this.Vg(a)},
op={},pp=function(a){var b=op[a];if(b==null){b=new np(a);op[a]=b}return b};
np.prototype.Vg=function(a){this.De=a;this.kh=[];if(this.wQ("||"))this.yn=this.XW;else if(this.wQ("and"))this.yn=this.SW;else if(this.ay("!="))this.yn=this.WW;else if(this.ay("="))this.yn=this.TW;else if(this.ay("<"))this.yn=this.VW;else if(this.ay(">"))this.yn=this.UW;else{this.Wba=this.XB(a);this.yn=this.PZ}};
np.prototype.ay=function(a){var b=this.De,c=b.indexOf(a);if(c!=-1){this.pda=a;this.nl=this.XB(Va(b.substring(0,c)));this.pm=this.XB(Va(b.substring(c+a.length)));return true}else return false};
np.prototype.wQ=function(a){var b=this.De,c=b.indexOf(a);if(c!=-1){this.pda=a;this.nl=pp(Va(b.substring(0,c)));this.pm=pp(Va(b.substring(c+a.length)));this.kh=this.kh.concat(this.nl.jd());this.kh=this.kh.concat(this.pm.jd());return true}else return false};
np.prototype.XB=function(a){var b=a.substring(0,1),c=a.toLowerCase();if("0123456789'\"".indexOf(b)==-1&&c!="true"&&c!="false"){var d=Mn(a);this.kh.push(d);return d}else return lp(a)};
np.prototype.PZ=function(a){return this.Wba.L(a)?true:false};
np.prototype.TW=function(a){return this.nl.L(a)==this.pm.L(a)};
np.prototype.WW=function(a){return this.nl.L(a)!=this.pm.L(a)};
np.prototype.VW=function(a){return this.nl.L(a)<this.pm.L(a)};
np.prototype.UW=function(a){return this.nl.L(a)>this.pm.L(a)};
np.prototype.XW=function(a){return this.nl.L(a)||this.pm.L(a)};
np.prototype.SW=function(a){return this.nl.L(a)&&this.pm.L(a)};
np.prototype.L=function(a){return this.yn(a)};
np.prototype.jd=function(){return this.kh};var qp=function(a,b,c){var d=Mn(a);if(d.Ia())return d.Ia().Gg(c,true).Ff(d.Yl,b);else d.Gg(c).J(b)},
rp=function(a,b){return Mn(a).Gg(b)},
sp=function(a,b){var c=za(a)?rp(a):a;b=b||0;if(b>20)return"";var d="                 ",e="";e=e+d.substring(0,b)+c.Fa()+":";var f=c.vb(),g=c.get();if(f.S()==0&&(!g||!g.S))e=e+g;else{e=e+"{\n";for(var h=0;h<f.S();h++)e=e+sp(f.Qf(h),b+1)+"\n";e=e+d.substring(0,b)+"}"}return e};var tp=function(){};
tp.prototype.ia=function(a,b){if(ve||re)return;var c=b||{},d=new up,e=true,f=c.enabled;if(f){var g=new np(f),h=g.jd();for(var i=0;i<h.length;i++)In().addListener(l(vp,null,d,g),h[i].bc());e=g.L()}var k=a.ownerDocument.createElement("div");while(a.childNodes.length>0)k.appendChild(a.firstChild);d.V8(b);d.C3(a);d.hc(k);d.Ca(e);d.C(a);return d};
var vp=function(a,b){a.Ca(b.L())},
up=function(){this.bi="#FCFCFC";this.GH="#F4F4F4";this.bf="#B5B5B5";this.yi="#9CBED8";this.ii="#B0B0B0";this.Ea=true};
o(up,J);up.prototype.C=function(a){this.Y=a;F(a,E,this.xh,false,this);F(a,Gf,this.se,false,this);F(a,Hf,this.pf,false,this);F(a,I,this.Oe,false,this);F(a,eg,this.Vj,false,this)};
up.prototype.V8=function(a){this.zw=a||{};this.bi=this.zw["background-color"]||this.bi;this.GH=this.zw["down-background-color"]||this.GH;this.bf=this.zw["border-color"]||this.bf;this.yi=this.zw["hover-border-color"]||this.yi;this.ii=this.zw["down-border-color"]||this.ii};
up.prototype.C3=function(a){this.Y=a;this.Et=new Ge(a.ownerDocument);C(a,"goog-button");a.style.border="0";a.style.background="transparent";a.style.margin="0";a.style.padding="0";a=(this.vL=a.appendChild(this.Et.m("div")));this.$C=this.Wq(a,"l1","background-color:"+this.bf);this.yo=this.Wq(a,"l2","border-color: "+this.bf+";background-color: "+this.bi);this.Wq(a,"l2","border-color: "+this.bf+";background-color: "+this.bi);this.Ed=a.appendChild(this.Et.m("div",{"class":"mid",style:"border-color: "+
this.bf+";background-color: "+this.bi+";padding: 1px 1px;"}));this.zo=this.Wq(a,"l2","border-color: "+this.bf+";background-color: "+this.bi);this.aD=this.Wq(a,"l1","background-color: "+this.bf)};
up.prototype.Wq=function(a,b,c){var d=a.appendChild(this.Et.m("div",{style:c,"class":b}));if(s)d.style.width="100%";d.appendChild(this.Et.m("img",{width:"1",height:"1",src:wp().vu()+"/static/images/Transparent.gif"}));return d};
up.prototype.xh=function(){this.dispatchEvent(E)};
up.prototype.HP=function(){this.$C.style.backgroundColor=this.bf;this.yo.style.borderColor=this.bf;this.yo.style.backgroundColor=this.bi;this.Ed.style.borderLeft="1px solid "+this.bf;this.Ed.style.borderRight="1px solid "+this.bf;this.Ed.style.padding="1px 1px";this.Ed.style.backgroundColor=this.bi;this.zo.style.borderColor=this.bf;this.zo.style.backgroundColor=this.bi;this.aD.style.backgroundColor=this.bf};
up.prototype.Q9=function(){this.$C.style.backgroundColor=this.yi;this.yo.style.borderColor=this.yi;this.yo.style.backgroundColor=this.yi;this.Ed.style.borderLeft="2px solid "+this.yi;this.Ed.style.borderRight="2px solid "+this.yi;this.Ed.style.padding="1px 0px";this.Ed.style.backgroundColor=this.bi;this.zo.style.borderColor=this.yi;this.zo.style.backgroundColor=this.yi;this.aD.style.backgroundColor=this.yi};
up.prototype.K9=function(){this.$C.style.backgroundColor=this.ii;this.yo.style.borderColor=this.ii;this.yo.style.backgroundColor=this.ii;this.Ed.style.borderLeft="2px solid "+this.ii;this.Ed.style.borderRight="2px solid "+this.ii;this.Ed.style.padding="2px 0px";this.Ed.style.backgroundColor=this.GH;this.zo.style.borderColor=this.ii;this.zo.style.backgroundColor=this.ii;this.aD.style.backgroundColor=this.ii};
up.prototype.se=function(){this.Q9()};
up.prototype.pf=function(){this.HP()};
up.prototype.Oe=function(){this.K9()};
up.prototype.Vj=function(){this.HP()};
up.prototype.hc=function(a){a.style.cssFloat="left";while(a.childNodes.length>0)this.Ed.appendChild(a.firstChild);if(s){this.Y.style.visibility="hidden";j.setTimeout(l(this.gE,this),0)}else this.gE();F(window,mg,l(this.gE,this))};
up.prototype.gE=function(){var a,b=10,c=this.Et.Ic().compatMode=="BackCompat",a=Ah(this.Ed).width;if(a<10)return;this.Y.style.visibility="";if(s){this.vL.style.width="1px";this.Y.style.overflow="visible";a=Ah(this.Ed).width;this.Y.style.width=a+b+"px";if(!c){this.Lv(this.$C,4);this.Lv(this.yo,6);this.Lv(this.zo,6);this.Lv(this.aD,4)}this.vL.style.width=a+b+"px"}else{this.Ed.style.cssFloat="left";a=Ah(this.Ed).width;this.Ed.style.cssFloat="";if(re&&c)this.Y.style.width=a+2+"px";this.vL.style.width=
a+b+"px"}};
up.prototype.Lv=function(a,b){a.style.width=Ah(a).width-b+"px"};
up.prototype.Ca=function(a){if(this.Ea!=a){if(a){qf(this.Y,"disabled");this.Y.disabled=false}else{C(this.Y,"disabled");this.Y.disabled=true}this.Ea=a}};var zp=function(a,b,c){if(a){this.Ul=a;this.b2=b||a.getAttribute("icon");if(this.b2){this.ka=v("div",{className:this.Ul.className});this.Ul.className="inner";this.I=w("img");this.I.src=this.b2;this.I.style.verticalAlign="bottom";this.ka.appendChild(this.I);if(this.I.readyState!="complete"){this.I.style.display="none";F(this.I,"load",this.rf,false,this)}if(s)this.I.style.verticalAlign="top";a.parentNode.replaceChild(this.ka,a);this.ka.appendChild(a);this.Ul.style.border="0px";this.Ul.style.padding=
"0px";this.Ul.style.paddingLeft="4px";this.ka.style.verticalAlign="top";this.Ul.style.whiteSpace="nowrap";this.Ul.style.width="80%";var d=Ah(this.ka).height;if(d>5){var e=s?0:xp(this.ka)-2;this.ka.style.height=d-e+"px"}this.ka.style.overflow="hidden"}this.Jp=c||a.getAttribute("defaultText");if(this.Jp)yp(this.Ul,this.Jp)}};
zp.prototype.rf=function(){this.I.style.display="inline"};var Ap=function(a,b){this.Y=a;if(a==null)return;this.dA=b?pp(b):null;if(this.dA){var c=l(this.iQ,this);ad(this.dA.jd(),function(h){In().addListener(c,h.De)});
this.iQ()}if(s){var d=Ah(a);a.style.width=String(d.width*0.77)+"px";var e=Number(a.currentStyle.paddingTop.split("px")[0]),f=Number(a.currentStyle.paddingBottom.split("px")[0]);a.style.paddingTop=String(e+2)+"px";var g=f>=2?f-2:0;a.style.paddingBottom=String(g)+"px"}F(a,I,l(this.J5,this));F(a,eg,l(this.CN,this));F(a,Hf,l(this.L3,this));F(a,Gf,l(this.baa,this));F(a,E,l(this.CN,this))};
Ap.prototype.J5=function(a){C(a.target,"presubmit");a.target.K5=true};
Ap.prototype.CN=function(a){qf(a.target,"presubmit");a.target.K5=false};
Ap.prototype.L3=function(a){qf(a.target,"presubmit")};
Ap.prototype.baa=function(a){if(a.target.K5)C(a.target,"presubmit")};
Ap.prototype.iQ=function(){if(this.dA)this.Y.disabled=!this.dA.L()};var Bp=function(a,b,c,d){var e=a.split("."),f=d||j,g=f,h=e.length;for(var i=0;i<h-1;i++){if(!g[e[i]])g[e[i]]={};g=g[e[i]]}g[e[h-1]]=b;if(c)f[c]=b},
yp=function(a,b){if(a.value==""||a.value==b||a.value==a.defaultText){a.defaulted=true;a.value=b;C(a,"input-default")}a.defaultText=b;F(a,jg,function(){if(a.defaulted){a.value="";qf(a,"input-default");a.defaulted=false}});
F(a,ig,l(Cp,null,a));F(document,If,function(c){if(c.target==a&&c.keyCode==27)c.stopPropagation()},
true)},
Cp=function(a){if(a.value==""&&!a.defaulted){a.defaulted=true;a.value=a.defaultText;C(a,"input-default")}},
Dp=function(a,b){b=b<0?0:b;a.style.height=b+"px";a.__lastHeight=b},
Ep=function(a,b){b=b<0?0:b;a.style.width=b+"px";a.__lastWidth=b},
Gp=function(a){return Fp(a,"paddingLeft")+Fp(a,"paddingRight")+Fp(a,"marginLeft")+Fp(a,"marginRight")+Fp(a,"borderLeftWidth")+Fp(a,"borderRightWidth")},
xp=function(a){return Fp(a,"paddingTop")+Fp(a,"paddingBottom")+Fp(a,"marginTop")+Fp(a,"marginBottom")+Fp(a,"borderTopWidth")+Fp(a,"borderBottomWidth")},
Fp=function(a,b){var c=Ie(a),d;if(c.defaultView&&c.defaultView.getComputedStyle){var e=c.defaultView.getComputedStyle(a,"");if(e)d=e[b]}if(!d)d=a.currentStyle?a.currentStyle[b]:a.style[b];if(Oa(d,"px")){var d=Number(d.substring(0,d.length-2));if(d<-10)d=0;return d}else return 0};var Hp=function(a,b){this.Y=u(a);this.vg=Mn(b);In().addListener(l(this.bl,this),b);F(this.Y,E,l(this.MW,this),false,this);var c=this.vg.L();if(c)this.Y.checked=c};
Hp.prototype.bl=function(){if(!this.Sl){var a=this.vg.L();this.Rl=true;this.Y.checked=a!=0;this.Rl=false}};
Hp.prototype.MW=function(){if(!this.Rl){var a=this.Y.checked;this.Sl=true;qp(this.vg.De,a);this.Sl=false}};var Ip=function(a,b,c,d,e){this.Y=u(a);this.vg=Mn(b);this.hfa=c;this.Jp=d||this.Y.getAttribute("defaultText");this.N3=e;if(this.N3)this.Y.maxLength=this.N3;In().addListener(l(this.bl,this),b);F(this.Y,kg,this.tn,false,this);F(this.Y,hg,this.tn,false,this);var f=this.vg.L();if(f)this.Y.value=f;if(this.Jp){F(this.Y,ig,this.nz,false,this);F(this.Y,jg,this.qo,false,this);this.nz()}};
Ip.prototype.bl=function(){if(!this.Sl){var a=this.vg.L();this.Rl=true;this.Y.value=a?a:"";this.Rl=false;this.wt=false;if(this.Jp)this.nz()}};
Ip.prototype.tn=function(){if(!this.Rl){var a=this.Y.value||null;this.Sl=true;var b=this.vg.Ia().Gg(null,true);b.Ff(this.vg.Yl,a);this.Sl=false}};
Ip.prototype.nz=function(){if(this.Y.value==""&&!this.wt){this.wt=true;C(this.Y,"input-default");this.Y.value=this.Jp}else{qf(this.Y,"input-default");this.wt=false}};
Ip.prototype.qo=function(){if(this.wt){this.Y.value="";qf(this.Y,"input-default");this.wt=false}};var Jp=function(a){this.Y=a;this.tea=200;this.yda=1000;this.m1=l(this.PK,this);F(a,["input","propertychange"],this.PK,false,this);F(a,If,this.l0,false,this);F(a,gg,this.yh,false,this);var b=[jg,E,ig];F(a,b,this.L0,false,this)};
Uc(Jp.prototype,J.prototype);Jp.prototype.l0=function(a){j.setTimeout(this.m1,0);if(!this.dispatchEvent(a))a.preventDefault()};
Jp.prototype.yh=function(a){var b=a.keyCode,c=null;if(b==40)c="DOWN_ARROW";else if(b==38)c="UP_ARROW";else if(b==13)c="ENTER";else if(b==27)c="ESCAPE";else if(b==9)c="TAB";if(s&&(b==8||b==46))j.setTimeout(this.m1,0);if(c){var d=this.dispatchEvent(c);if(!d)a.preventDefault()}};
Jp.prototype.L0=function(a){this.dispatchEvent(a)};
Jp.prototype.PK=function(){var a=this.Y.defaulted?"":this.Y.value;if(a!=this.Qq){this.TH("VALUE",a);if(this.b$)j.clearTimeout(this.b$);this.b$=j.setTimeout(l(this.IG,this,"STABLE_VALUE",a),this.tea);if(this.n5)j.clearTimeout(this.n5);this.n5=j.setTimeout(l(this.IG,this,"PAUSED_VALUE",a),this.yda);this.Qq=a}};
Jp.prototype.IG=function(a,b){var c=this.Y.defaulted?"":this.Y.value;if(c==b)this.TH(a,b)};
Jp.prototype.TH=function(a,b){var c=new D(a);c.value=b;this.dispatchEvent(c)};var Kp=function(a,b,c){this.Y=u(a);this.vg=Mn(b);var d=this.vg.L();if(c){this.MV=c;ad(c.xb(),function(g,h){var i=w("option");i.value=h;i.text=g;this.Y.options[this.Y.options.length++]=i})}for(var e=0;e<this.Y.options.length;e++){var f=this.Y.options[e];
if(d==f.value)f.selected=true}In().addListener(l(this.bl,this),b);F(this.Y,kg,this.tn,false,this);F(this.Y,hg,this.tn,false,this)};
Kp.prototype.bl=function(){if(!this.Sl){var a=this.vg.L();this.Rl=true;this.Y.value=a?a:"";this.Rl=false}};
Kp.prototype.tn=function(){if(!this.Rl){var a=this.Y.value||null;this.Sl=true;qp(this.vg.De,a);this.Sl=false}};var Lp=function(a){return a.nodeType==1&&(a.getAttribute("maximize")=="true"||a.getAttribute("maximize")=="compatible"&&(se||re))},
Mp=function(a,b,c){for(var d=a.firstChild;d;d=d.nextSibling)Mp(d,b,c);if(Lp(a)){if(a.__lastWidth)Ep(a,a.__lastWidth+b);if(a.__lastHeight)Dp(a,a.__lastHeight+c)}},
Np=function(a){var b;if(Lp(a)&&a.nodeName!="TR"){var c,d;if(a.parentNode.nodeName=="BODY"){var e=Qe();c=e.width;d=e.height;a.parentNode.style.overflow="hidden"}else{var f=a.parentNode;c=a.parentNode.offsetWidth-Gp(f);if(se)c-=Gp(a);var g=a.parentNode.offsetHeight;if(ve&&a.parentNode.nodeName=="TD"&&a.parentNode.parentNode.__lastHeight)g=a.parentNode.parentNode.__lastHeight;d=g-xp(f)}Gp(a);if(a.nodeName!="TABLE")Ep(a,c);Dp(a,d);if(a.offsetHeight>d)Dp(a,2*d-a.offsetHeight);if(a.nodeName=="TABLE"){var h=
null,i=0,k;for(var k=0;k<a.rows.length;k++){var n=a.rows[k];if(n.getAttribute("maximize")=="true"){h=n;h.style.height="100%"}}if(h){for(var k=0;k<a.rows.length;k++){var n=a.rows[k];if(n!=h){var q=ve&&n.firstChild?n.firstChild.offsetHeight:n.offsetHeight;i+=q}}Dp(h,d-i-2);h.style.width=""}}}for(b=a.firstChild;b;b=b.nextSibling)Np(b)};var Op=function(){this.dr=[];this.Pq={};In().op(new Vn(this.Pq,"LastMessage"))};
Op.prototype.add=function(a,b){var c=a.length<150,d=new D("NOTIFY_USER");d.message=a;d.level=b;var e=true;if(c)e=Uo().on(d);if(e)this.nW(a,b)};
Op.prototype.nW=function(a,b){var c=b||"INFO",d={Message:a,Level:c};this.dr.push(d);this.Pq.Message=a;this.Pq.Level=c;In().yc("$LastMessage")};
Op.prototype.clear=function(){var a=new D("CLEAR_NOTIFICATIONS");if(Uo().on(a))this.pU()};
Op.prototype.pU=function(){var a={Message:null};this.dr.push(a);this.Pq.Message=null;this.Pq.Level=null;In().op(new Vn(this.Pq,"LastMessage"));In().yc("$LastMessage")};var Pp=function(a,b){this.j4=a;this.DH=b||"$LastMessage";this.cda=Mn(this.DH+"/Message");this.Jea=Mn(this.DH+"/Level");In().addListener(l(this.B2,this),this.DH+"/...");this.GB=false};
Pp.prototype.B2=function(){if(!this.GB)j.setTimeout(l(this.Qg,this),0);this.GB=true};
Pp.prototype.Qg=function(){if(this.GB){var a=Le(this.j4);if(a){var b=this.cda.L();if(b==null){qf(a,"error");qf(a,"info");a.innerHTML=""}else{var c=this.Jea.L();if(c=="INFO"){C(a,"info");qf(a,"error")}else{C(a,"error");qf(a,"info")}C(a,"hilite");if(b.length<150)a.innerHTML=p(b);else{var d=v("div",{style:"display:none; position:absolute; background-color:white;padding:8px; border:1px dashed #AAA;text-decoration: none"});d.innerHTML=p(b).replace(/\n/g,"<br/>");var e=v("a",{href:"#"});F(e,E,function(){d.style.display=
d.style.display=="block"?"none":"block"});
e.innerHTML="Details";a.innerHTML="";a.appendChild(document.createTextNode("Operation failed. Please try again in a moment. "));a.appendChild(e);a.appendChild(d);var f=uh(a),g=Ah(a),h=Ah(d);sh(d,f.x+g.width-h.width,f.y+g.height)}j.setTimeout(l(this.I5,this),1000)}}this.GB=false}};
Pp.prototype.I5=function(){var a=Le(this.j4);if(a)qf(a,"hilite")};var Qp=function(){this.zT="..";this.kO={};this.gR={};this.ys={};this.fv={};this.mca={};this.eF={}},
wp=function(){if(!Rp)Rp=new Qp;return Rp},
Rp=null;Qp.prototype.e9=function(a,b,c){a=new N(a);if(b&&c){if(!this.ys[b])this.ys[b]={};this.ys[b][c]=a}else if(b)this.gR[b]=a;else if(c)this.kO[c]=a;else this.zT=a};
Qp.prototype.vu=function(a,b){return this.ys[a]&&this.ys[a][b]?this.ys[a][b]:(this.gR[a]?this.gR[a]:(this.kO[b]?this.kO[b]:this.zT))};
Qp.prototype.o6=function(a){this.fv[a.H()]=a};
Qp.prototype.eG=function(a,b,c){var d=this.fv[a];if(d&&d.rc){d.rc(b,c);return true}var e=this.mca[a];if(e&&e.rc){e.rc(b,c);return true}return false};
Qp.prototype.Ct=function(a,b,c){var d=this.fv[a];if(d&&d.Ct){d.Ct(b,c);return true}return false};
Qp.prototype.fl=function(a,b,c,d){var e=this.fv[a];if(e&&e.fl){e.fl(b,c,d);return true}return false};
var Sp=0;Qp.prototype.lV=function(a,b,c,d,e){if(ze&&se)C(document.body,"background");var f=e||window,g=Je(f.document.documentElement),h=this.mV(a,b,g),i=this.GV(a,c,d,f);if(!s)h.src=i;var k=new R(null,false,g);k.Oa().appendChild(h);k.Rg(new Xk);k.M(true);if(s)h.src=i;this.eF[a]=k;return h.contentWindow};
Qp.prototype.mV=function(a,b,c){var d=a+"-iframe",e=c.nR(d);if(!e){e=c.m("iframe",{id:d});L(e,"display","block");L(e,"backgroundColor","#FFF");L(e,"border","1px solid #555");zh(e,b.width,b.height)}return e};
Qp.prototype.GV=function(a,b,c,d){var e=new N(this.vu()+"/ui/"+a);if(c){var f="UiWidgetCallback"+Sp++,g=d||window;g[f]=c;e.fa("done",f)}e.fa("js","RAW");e.fa("pop","TRUE");if(b)ad(b,function(h,i){if(h!=null)e.fa(i,h)});
return e};
Qp.prototype.QW=function(a){var b=this.eF[a];if(b){b.b();qf(document.body,"background")}delete this.eF[a]};
Qp.prototype.K1=function(a){var b=this.eF[a];if(b)b.M(false)};
Qp.prototype.J7=function(a,b){var c=this.fv[a],d=Le(c.H()+"-iframe");if(d&&d.contentWindow.UI_receive)d.contentWindow.UI_receive(b)};
Qp.prototype.aG=function(a){var b=a||document;for(var c=b.firstChild;c;c=c.nextSibling)this.aG(c);if(b.getAttribute){var d=b.getAttribute("gtype");if(d){var e={};for(var f=0;f<b.attributes.length;f++){var g=b.attributes[f];e[g.name]=g.value}var h=this.eG(d,b,e);if(!h)this.Ct(d,b,e)}}};
window.UI_setServerBase=function(a,b,c){wp().e9(a,b,c)};
window.UI_getServerBase=function(a,b){wp().vu(a,b)};
window.UI_attachAndDisplayAllWidgets=function(a){wp().aG(a)};
window.UI_attachWidget=function(a,b,c){wp().eG(a,b,c)};
window.UI_displayInline=function(a,b,c){wp().Ct(a,b,c)};
window.UI_displayIframe=function(a,b){wp().fl(a,b)};;var Tp=function(a,b,c){J.call(this);this.sn=u(a);this.Jt=u(b);this.Dj=c==true;this.sn.tabIndex=0;F(this.sn,E,this.M4,false,this);F(this.sn,gg,this.N4,false,this);this.No(this.Dj)};
o(Tp,J);Tp.prototype.expand=function(){this.No(true)};
Tp.prototype.collapse=function(){this.No(false)};
Tp.prototype.rQ=function(){this.No(!this.Dj)};
Tp.prototype.No=function(a){this.Jt.style.display=a?"":"none";this.QE(a);this.Dj=a;this.dispatchEvent(new Up("toggle",this,this.Dj))};
Tp.prototype.J2=function(){return this.Dj};
Tp.prototype.QE=function(a){if(a){qf(this.sn,"goog-zippy-collapsed");C(this.sn,"goog-zippy-expanded")}else{qf(this.sn,"goog-zippy-expanded");C(this.sn,"goog-zippy-collapsed")}};
Tp.prototype.N4=function(a){if(a.keyCode==13||a.keyCode==32){this.rQ();a.preventDefault();a.stopPropagation()}};
Tp.prototype.M4=function(){this.rQ()};
var Up=function(a,b,c){D.call(this,a,b);this.expanded=c};
o(Up,D);var Vp=function(a,b,c){var d=v("div",{style:"overflow:hidden"}),e=u(b);e.parentNode.replaceChild(d,e);d.appendChild(e);this.Xz=d;this.jc=null;Tp.call(this,a,e,c);var f=this.Dj;this.Xz.style.display=f?"":"none";this.QE(f)};
o(Vp,Tp);Vp.prototype.lT=500;Vp.prototype.kT=Tg;Tp.prototype.hv=function(){return this.jc!=null};
Vp.prototype.No=function(a){if(this.Dj==a&&!this.jc)return;if(this.Xz.style.display=="none")this.Xz.style.display="";var b=this.Jt.offsetHeight,c=0;if(this.jc){a=this.Dj;cg(this.jc);this.jc.stop();c=b-Math.abs(parseInt(this.Jt.style.marginTop,10))}else c=a?0:b;this.QE(a);this.jc=new Ug([0,c],[0,a?b:0],this.lT,this.kT);var d=[Vg,Wg,"end"];F(this.jc,d,this.G4,false,this);F(this.jc,"end",l(this.H4,this,a));this.jc.play(false)};
Vp.prototype.G4=function(a){var b=this.Jt.offsetHeight;this.Jt.style.marginTop=0-(b-a.y)+"px"};
Vp.prototype.H4=function(a){cg(this.jc);this.Dj=a;this.jc=null;if(!a)this.Xz.style.display="none";this.dispatchEvent(new Up("toggle",this,a))};var Wp="/ui/",Xp=function(a){Wp=a};
j.HtmlUtil_setServerBase=Xp;var Yp=function(a){var b=new N(Wp+"ContactManager"),c=a||"THREE";b.fa("style",c);b.fa("js","RAW");b.fa("pop","TRUE");return b},
Zp=function(a,b){var c=b=="TWO"?502:750,d=new ke(0,0,c,500),e="toolbar=no,location=no,menubar=no,scrollbars=no,resizable=yes,status=no,width="+d.width+",height="+d.height+",top="+d.top+",left="+d.left,f=Yp(b).toString();window.open(f,"_manager",e)};
Bp("goog.focus.ContactManagerLauncher.popManager",Zp);var $p=function(a,b){this.Y=a;this.Laa=b;this.vN=b.exists=="true";this.Qt=[];this.Wy(a);if(!this.vN)this.sH(po("ADD_PICTURE"),false,true)};
$p.prototype.b=function(){for(var a=0;a<this.Qt.length;a++)H(this.Qt[a]);if(this.Bk&&this.Bk.parentNode)this.Bk.parentNode.removeChild(this.Bk);if(this.ci&&this.Bk.ci)this.ci.parentNode.removeChild(this.Bk)};
$p.prototype.Wy=function(a){this.Qt.push(F(a,Gf,this.P4,false,this));this.Qt.push(F(a,Hf,this.im,false,this));this.Qt.push(F(a,E,this.Y4,false,this))};
$p.prototype.ka=null;$p.prototype.zi=null;$p.prototype.P4=function(){if(this.ci){this.zG();return}this.ci=this.VU();if(this.vN)this.sH(po("CHANGE_PICTURE"),true)};
$p.prototype.VU=function(){var a=Ah(this.Y);a.width-=Gp(this.Y);a.height-=xp(this.Y);if(s){a.width+=4;a.height+=4}a.width+=2;a.height+=2;var b=v("div");this.Wy(b);b.style.position="absolute";b.style.zIndex=20;zh(b,a);b.style.border="2px solid blue";var c=uh(this.Y);c.x-=3;c.y-=3;b=this.Y.parentNode.insertBefore(b,this.Y);yh(b,c);return b};
$p.prototype.sH=function(a,b,c){if(!this.Y.parentNode)return;var d=v("div");this.Wy(d);var e=b?"background-color: white;":"";d.innerHTML='<div style="text-align: center; '+e+'font-family: Arial; font-size: 12px; padding: 2px"><a href="javascript:void(0)">'+a+"</a></div>";d.style.position="relative";d.style.zIndex=20;Ye(d,this.Y);var f=(this.Y.offsetHeight+d.offsetHeight)/2;d.style.top="-"+f+"px";this.Bk=d;this.cfa=c};
$p.prototype.im=function(a){if(this.ci){var b=a.relatedTarget;if(!b||b!=this.Y&&!df(this.ci,b))this.qB(true)}};
$p.prototype.hZ=function(a){var b={dn:a.Lb()},c=a.getPictureUrl(true);if(c)b.eUrl=c;aq(b);return b};
var aq=function(a){var b=Mn("$UserData/IsPicasaUser").L(),c=Mn("$UserData/Email").L(),d=b&&c&&(Oa(c,"gmail.com")||Oa(c,"googlemail.com"));a.userId=d?c.substring(0,c.indexOf("@")):null;a.urlEnabled=1;a.extraUploadParams="out=hjs";a.suppressHide="1"};
$p.prototype.Y4=function(){this.qB(false);var a=Mn("$Contacts/"+this.Laa.contactid).Gg(),b=this.hZ(a);wp().fl("photopicker.PhotoPicker",b,l(this.Ou,this))};
$p.prototype.qB=function(a){this.zG();if(a){var b=l(this.qB,this,false);this.zi=j.setTimeout(b,20);return}if(this.Y&&this.ci){this.ci.parentNode.removeChild(this.ci);this.ci=null;if(this.vN){this.Bk.parentNode.removeChild(this.Bk);this.Bk=null}}};
$p.prototype.zG=function(){if(this.zi){j.clearTimeout(this.zi);this.zi=null}};
$p.prototype.Ou=function(a){var b=this.Laa.contactid,c=a.type;if(c=="PHOTO_PICKED")if(a.evergreen)oo.ContactData.get().chooseEvergreen(b);else if(a.noPhoto)oo.ContactData.get().deletePhoto(b);else oo.ContactData.get().savePhoto(b,a.imageUrl,a.cropString);else if(c=="SUGGEST_DONE"&&a.suggestMessage)oo.ContactData.get().sendPhotoSuggestion(b,a.suggestMessage);else bq()};var cq=function(){},
dq,eq={};cq.prototype.H=function(){return"photopicker.PhotoPicker"};
var fq=function(){if(!dq){var a=new cq;wp().o6(a);dq=true}},
bq=function(){wp().K1("photopicker.PhotoPicker");wp().J7("photopicker.PhotoPicker",{type:"CLOSE_NO_REMOVE"});wp().QW("photopicker.PhotoPicker");Ro().fireEvent("ENABLE_WINDOW")};
cq.prototype.rc=function(a,b){for(var c in eq){var d=eq[c];if(!d.Y||!df(document,d.Y)){d.b();delete eq[c]}}if(a&&!eq[a.id]){var e=new $p(a,b);eq[a.id]=e}};
cq.prototype.fl=function(a,b,c){var d=new ne(545,332);if(s)d.height-=16;wp().lV(this.H(),d,a,b,c);Ro().fireEvent("DISABLE_WINDOW")};var gq,hq,iq,jq=function(a,b){var c=307,d;if(a){d=uh(a);var e=Ah(a);d.y+=e.height}else d=new ge(0,0);var f=e.width>c?e.width:c,g=554,h=313,i=600;if(b){d.x+=b.screenX-b.clientX;d.y+=b.screenY-b.clientY}if(d.x+h>screen.width)d.x=screen.width-h;if(d.y+i>screen.height)d.y=screen.height-i;var k=new ke(d.x,d.y,f,g);return k},
lq=function(a,b){var c=new N(Wp.replace(/ui\/?$/,""));kq(c,a,b)},
kq=function(a,b,c,d){var e;if(b){b=Le(b);var e=b.getAttribute("id")}else e=null;var f;f=!ve&&b?Te(Ie(b))||window:window;var g=jq(b,c),h="toolbar=no,location=no,menubar=no,scrollbars=no,resizable=yes,status=no,width="+g.width+",height="+g.height+",top="+g.top+",left="+g.left,i=mq(a,e,d);if(!gq||gq.closed)gq=f.open(i.toString(),"_picker",h);gq.focus()},
mq=function(a,b,c){var d=a.X6(new N("ui/ContactPicker"));if(b)d.fa("inputId",b);if(c){if(c.serviceName)d.fa("service",c.serviceName);if(c.hl)d.fa("hl",c.hl)}return d},
oq=function(a,b,c){if(hq)nq();var d=jq(a,null),e=mq(a,c);window.iframeDone=nq;e.fa("done","iframeDone");var f=Le("picker-iframe-div");if(f==null){var f=v("div",{id:"picker-iframe-div",style:"position: absolute; background-color; #FFF",zIndex:10});document.body.appendChild(f);f.innerHTML='<iframe id="picker-iframe" style="display:none; background-color; #FFF; border: 0"></iframe>'}var g=Le("picker-iframe");g.style.display="block";yh(f,d.left,d.top);zh(g,d.width,d.height);g.src=e;iq=f;hq=g},
nq=function(){A(iq);iq=null;hq=null};
La("goog.focus.PickerLauncher.popPicker",lq);La("goog.focus.PickerLauncher.popIframe",oq);var pq=function(a,b,c,d,e,f,g){this.Bj=b;this.Fda=c;this.R7=a;this.ud=d;this.iea=!(!e);this.E$=f||"";this.Gd=g};
pq.prototype.a3=function(){if(!Mn("$UserData").Gg()){var a=new Dj;F(a,Ki,this.c_,false,this);a.send(this.R7+"/gastatus?out=js","GET")}else this.TL()};
pq.prototype.c_=function(a){var b=a.target;if(b.Re()){var c=this.hN(b.Sb());if(c.Body){var d=c.Body.UserData,e=new Zn(d,"UserData");In().op(e)}}this.TL()};
pq.prototype.TL=function(){var a={};a.dn=this.Bj;a.profileMode="1";a.eUrl=this.Fda;if(this.E$)a.theme=this.E$;aq(a);wp().fl("photopicker.PhotoPicker",a,l(this.Ou,this),this.Gd)};
pq.prototype.Ou=function(a){if(a.type=="PHOTO_PICKED")this.K7(a);else bq()};
pq.prototype.K7=function(a){var b=new Ui;b.add("out","js");b.add("cid","p");b.add("tok",a.tok);if(a.evergreen)return;else if(a.noPhoto)b.add("action","DELETE");else{b.add("action","SET");b.add("crop",a.cropString);b.add("photo",a.imageUrl)}if(this.iea)b.add("p","true");var c=new Dj;F(c,Ki,this.l1,false,this);c.send(this.R7+"/update/photos","POST",b.toString())};
pq.prototype.l1=function(a){bq();var b=a.target,c=null;if(b.Re())c=this.hN(b.Sb());var d={};if(c&&c.Success){d.success=true;if(c.Body){d.newUrl=c.Body.NewUrl;d.newUrlF=c.Body.NewUrlF}}else d.success=false;this.ud(d)};
pq.prototype.hN=function(a){var b=a.indexOf("&&&START&&&")+"&&&START&&&".length,c=a.lastIndexOf("&&&END&&&");return yj(a.substring(b,c))};
var qq=function(a,b,c,d,e,f){fq();var g=new pq(wp().vu(),a,b,c,d,e,f);g.a3()};
window.launchProfilePhotoPicker=qq;var rq=function(a){this.clear();if(a)this.jj(a)};
rq.prototype.S=function(){return this.Ma};
rq.prototype.xb=function(){return Ic(this.ic)};
rq.prototype.tc=function(){return Ic(this.Pa)};
rq.prototype.Ob=function(a){var b=Ga(a);return Kc(this.Pa,b)};
rq.prototype.gh=function(a){return Lc(this.ic,a)};
rq.prototype.isEmpty=function(){return this.Ma==0};
rq.prototype.clear=function(){this.Pa={};this.ic={};this.Ma=0};
rq.prototype.remove=function(a){var b=Ga(a);if(Nc(this.Pa,b)){delete this.ic[b];this.Ma--;return true}return false};
rq.prototype.get=function(a,b){var c=Ga(a);if(c in this.ic)return this.ic[c];return b};
rq.prototype.J=function(a,b){var c=Ga(a);if(!(c in this.Pa)){this.Pa[c]=a;this.Ma++}this.ic[c]=b};
rq.prototype.jj=function(a){var b,c;if(a instanceof rq){b=a.tc();c=a.xb()}else{b=Jc(a);c=Ic(a)}for(var d=0;d<b.length;d++)this.J(b[d],c[d])};
rq.prototype.ra=function(){return new rq(this)};var sq=function(){this.el=new rq;this.ab=null;this.Qda=l(this.Lc,this)};
sq.prototype.I3=function(a,b,c){if(!Ca(c))throw Error("not a function: "+c);if(!a.EA)throw Error("Nodes for markDirty must support getDepth");b=b||[];var d=b.join(","),e=this.el.get(a);if(!e){e={};this.el.J(a,e)}if(!e[d])e[d]=[];pc(e[d],c);this.r7()};
sq.prototype.nM=function(a,b){b=b||[];var c=this.el.get(a);if(c)if(b.length==0)this.el.remove(a);else{var d=String(b);delete c[d];if(c.length==0)this.el.remove(a)}};
sq.prototype.r7=function(){if(!this.ab)this.ab=j.setTimeout(this.Qda,50)};
sq.prototype.Lc=function(){delete this.ab;var a=this.el.tc();a.sort(function(d,e){return d.EA()-e.EA()});
for(var b=0;b<a.length;b++){var c=a[b];this.n6(c);this.el.remove(c)}};
sq.prototype.n6=function(a){while(true){var b=this.el.get(a),c=this.DY(b);if(c==null)return;var d=c==""?[]:c.split(","),e=b[c];for(var f=0;f<e.length;f++){var g=e[f];g.call(a,null,"",d)}delete b[c]}};
sq.prototype.DY=function(a){if(a!=null)for(var b in a)return b;return null};var tq=function(){};
tq.prototype.C=function(){};
tq.prototype.jd=function(){};
tq.prototype.Ol=function(){};var uq=function(){};
o(uq,tq);uq.prototype.C=function(){};
uq.prototype.jd=function(){return[]};
uq.prototype.Ol=function(){return true};
uq.prototype.Ow=function(){};var vq=function(){};
vq.prototype.fs=function(){};
vq.prototype.Sp=function(){};
vq.prototype.ct=function(){};
vq.prototype.Jx=function(){};
vq.prototype.Ot=function(){};var xq=function(a,b){this.Y=a;this.Dt=a.ownerDocument;this.Mi=[];this.bv=false;this.hE=null;this.a$=null;this.Kt=[];this.zC=0;this.Ue=b||"SET_CONTENT";this.rV=this.Ue!="SET_CONTENT";this.L6=this.Ue=="REPLACE";if(this.rV){var c=wq[a.nodeName]||"DIV";this.R1=w(c);this.an=this.R1}else this.an=a;this.Kt.push(this.an)};
o(xq,vq);var wq={TR:"TABLE",TD:"TR",OPTION:"SELECT"},yq={},zq={},Aq={},Bq={style:true,script:true},Cq={option:true,tr:true,td:true,tbody:true,thead:true},Dq={br:true,img:true};xq.prototype.fs=function(a,b){var c,d;if(this.zC==0&&Cq[a]){var e=this.Dt.createElement(a),f=b.length;for(var g=0;g<f;g++){var h=b[g];if(h.name=="id")c=h.value;else if(h.name=="g.oncreate"){d=h.value;continue}e.setAttribute(h.name,h.value)}if(c&&d)this.$M.push({id:c,script:d});this.Kt.push(e);this.an.appendChild(e);this.an=
e;this.Daa=true;return}this.zC++;if(Bq[a])return this.X5(a,b);var i=yq[a];if(!i){i="<"+a;yq[a]=i}var k=i;if(b!=null){var f=b.length;for(var g=0;g<f;g++){var h=b[g],n=h.name,q=h.value;if(n=="id")c=q;else if(n=="g.oncreate"){d=q;continue}var t=Aq[n];if(t==null){t=" "+n+'="';Aq[n]=t}k=k+t+p(q)+'"'}if(c&&d)this.$M.push({id:c,script:d})}k=k+">";this.Mi.push(k)};
xq.prototype.Sp=function(a){if(this.zC>0)this.zC--;else{if(this.an.childNodes.length==0)this.an.innerHTML=this.Mi.join("");this.Mi=[];this.Kt.pop();this.an=this.Kt[this.Kt.length-1];return}if(this.bv==true){this.bv=false;return}if(!Dq[a]){var b=zq[a];if(b==null){b="</"+a+">";zq[a]=b}this.Mi.push(b)}if(this.Mi.length>1000){var c=this.Mi.join("");this.Mi=[c]}};
xq.prototype.ct=function(a){if(this.bv==true)return this.Z9(a);this.Mi.push(p(a))};
xq.prototype.Jx=function(){this.Mi=[];this.s7=[];this.$M=[]};
xq.prototype.Ot=function(){var a=this.Mi.join("");if(this.rV){var b=this.R1;if(!this.Daa)b.innerHTML=a;if(b.childNodes.length==1&&this.L6)this.Y.parentNode.replaceChild(b.childNodes[0],this.Y);else{if(this.Y.parentNode.nodeName=="SELECT")var c=this.Y.parentNode.value;var d;while(d=b.childNodes[0])this.Y.parentNode.insertBefore(d,this.Y);if(this.L6)this.Y.parentNode.removeChild(this.Y);if(c!=null)this.Y.parentNode.value=c}}else if(!this.Daa)this.Y.innerHTML=a;ad(this.s7,function(e){eval(e)});
ad(this.$M,function(e){var f=u(e.id);(new Function(e.script)).call(f)})};
xq.prototype.X5=function(a,b){if(a=="style"){var c;for(var d=0;d<b.length;d++)if(b[d].name=="id")c=b[d].value;this.hE="style";this.a$=c+"_ss";this.bv=true}else if(a=="script"){this.hE="script";this.bv=true}else throw Error("No handler for "+a);};
xq.prototype.Z9=function(a){if(this.hE=="style")this.WS(this.a$,a);else if(this.hE=="script")this.s7.push(a)};
xq.prototype.xV=function(a){var b=null,c=Le(a);if(c==null){c=this.Dt.createElement("style");c.setAttribute("type","text/css");c.setAttribute("id",a);this.Dt.body.appendChild(c)}for(var d=0;d<this.Dt.styleSheets.length;d++){var e=this.Dt.styleSheets[d],f=e.owningElement;if(f==null)f=e.ownerNode;if(f.id==a)b=e}return b};
xq.prototype.WS=function(a,b){if(Le(a))return;var c=this.xV(a),d=b.split("}");if(c.insertRule){for(var e=0;e<d.length;e++)if(d[e]!="")c.insertRule(d[e]+"}",0)}else if(c.cssText!=null)for(var e=0;e<d.length;e++)if(d[e]!=""){var f=d[e].split("{"),g=f[0],h=f[1],i=g.split(",");for(var k=0;k<i.length;k++)if(i[k]!="")c.addRule(i[k],h)}};var Eq=function(a,b){this.Vg(a,b)},
Fq=function(a,b){if(a.indexOf("{{")==-1||a.indexOf("function(")!=-1)return mp(a);else if(Na(a,"{{")&&Oa(a,"}}")){var c=Mn(a.substring(2,a.length-2));if(b==1){c=new Gn(c.bc());c.M2=true}c.jd=function(){return[c]};
return c}else return new Eq(a,b)};
Eq.prototype.Vg=function(a,b){this.De=a;this.Tb=[];this.kh=[];if(this.De.indexOf("{{")==-1)this.Tb.push(mp(this.De));else{var c=this.De.split("{{");for(var d=0;d<c.length;d++){var e=c[d];if(e.length>0){var f=e.indexOf("}}");if(f==-1)this.Tb.push(mp(e));else{var g=e.substring(0,f),h;if(b==1){h=new Gn(g);h.M2=true}else h=Mn(g);this.Tb.push(h);this.kh.push(h);if(f<e.length-1){var i=e.substring(f+2);this.Tb.push(mp(i))}}}}}};
Eq.prototype.L=function(a){var b=this.Tb.length,c=[];for(var d=0;d<b;d++){var e=this.Tb[d].L(a);if(this.Tb[d].M2)if(e==null)e="null";else if(za(e))e=sb(e);c.push(e==null?"":e)}return c.join("")};
Eq.prototype.jd=function(){return this.kh};var Gq=function(a,b,c,d){this.JM=a.nodeName;this.fc=c;this.cW=d;this.e6(a);this.h8(b);this.Mf(a)},
Hq=Fd("goog.focus.TemplateNode"),Iq=null,Jq=0,Kq=function(){if(!Iq)Iq=new sq;return Iq};
Gq.prototype.vw=true;Gq.prototype.EA=function(){return this.cW};
Gq.prototype.b=function(){};
Gq.prototype.e6=function(a){this.Tm={};var b=a.attributes;for(var c=0,d;d=b[c];++c){var e=d.nodeName,f=d.nodeValue;if(f!=null)if(e=="if")this.wB=pp(f);else if(e=="context")this.KU=Mn(f);else if(e=="repeat")this.kD=Mn(f);else if(e=="id"){this.Za=f;this.Y1=Fq(this.Za)}else{var g=Na(e,"on")||e=="g.oncreate";this.Tm[e]=Fq(f,g?1:0)}}};
Gq.prototype.h8=function(a){this.c7=a;this.to=this.c7;if(this.KU)this.to=this.SJ(this.KU,true);if(this.kD)this.to=this.SJ(this.kD,true)};
Gq.prototype.TJ=function(a,b){var c=a.De,d=b?this.c7:this.to;if(c.indexOf("$")!=0&&d)c=d.De+"/"+c;return c};
Gq.prototype.SJ=function(a,b){return Mn(this.TJ(a,b))};
Gq.prototype.Vd=function(){if(!this.Wd){this.FS();if(!this.Za){this.Za="Node"+Jq++;this.Y1=Fq(this.Za)}this.Wd=true}};
Gq.prototype.FS=function(){if(this.wB){var a=this.xz(this.lU);this.Ev(this.wB,a,true)}if(this.kD){var b=this.xz(this.waa);this.Ev(this.to.Ia(),b,true)}var c=this.xz(this.kaa);for(var d=0;d<this.Cp.length;d++){var e=this.Cp[d];if(!e.Ol())this.Ev(e,c,false)}for(var f in this.Tm)this.Ev(this.Tm[f],c,false)};
Gq.prototype.xz=function(a){var b=this;return function(c,d,e){Kq().I3(b,e,a,c)}};
Gq.prototype.Ev=function(a,b,c){var d=In();if(a.jd){var e=a.jd();for(var f=0;f<e.length;f++){var g=this.TJ(e[f],c);d.OF(b,g)}}else{var g=a.De;d.OF(b,g)}};
Gq.prototype.Mf=function(a){if(!this.Cp){var b=this.JM=="script"?1:0;this.Cp=[];var c=a.childNodes;for(var d=0,e;e=c[d];++d)if(!(e.nodeType==3&&/^[\n\r\t ]*$/.test(e.nodeValue))){var f=Lq().FV(e,this.to,this.fc,this.cW+1,b);this.Cp.push(f)}}};
Gq.prototype.vc=function(){return this.wB?this.wB.L(this):true};
Gq.prototype.C=function(a,b){if(b&&b.Na)b=Mn(b.mb());if(b){this.setDataContext(b);this.Mf()}this.Jr(a)};
Gq.prototype.nx=function(a){this.uc=a||[];this.rn=this.YI(this.uc)};
Gq.prototype.eq=function(a,b){var c=[];for(var d in this.Tm)if(!a||d!="style"){var e=this.Tm[d].L(this)||"";c.push({name:d,value:e})}var f=b||this.iu();c.push({name:"id",value:f});if(a){var g=this.Tm.style,h=(g?g.L(this)+"; ":"")+"display: none";c.push({name:"style",value:h})}return c};
Gq.prototype.Ix=function(a,b,c){if(this.vw)a.fs(b,c)};
Gq.prototype.Nt=function(a,b){if(this.vw)a.Sp(b)};
Gq.prototype.Jr=function(a,b,c){Kq().nM(this,this.uc);var d=this.JM,e=this.vc();if(this.kD&&!b){this.G6(a,c);this.Sda=true}else if(e){var f=this.eq();this.Ix(a,d,f);this.$N(a);this.Nt(a,d);this.Sda=true}else if(d!="option"){var f=this.eq(true);if(this.vw){a.fs(d,f);a.Sp(d)}}};
Gq.prototype.G6=function(a,b){var c=Mn(this.IA(this.uc)).HJ(),d=this.JM;for(var e=0;e<c.S();e++){var f=c.Qf(e);this.uc.push(f.Fa());this.rn=this.YI(this.uc);if(this.vc()){var g=this.eq();this.Ix(a,d,g);this.$N(a);this.Nt(a,d);this.KS()}this.uc.pop()}if(!b&&d!="option"){this.eO=this.iu()+"-R";var h=this.eq(true,this.eO);this.Ix(a,d,h);this.Nt(a,d)}};
Gq.prototype.KS=function(){if(!this.Zz)this.Zz={};var a=this.Zz;for(var b=0;b<this.uc.length-1;b++){if(!a[this.uc[b]])a[this.uc[b]]={};a=a[this.uc[b]]}a[String(this.uc[this.uc.length-1])]=this.iu()};
Gq.prototype.C5=function(a){var b=this.vc(),c=a.style.display!="none";if(b!=c){if(b)this.bO(a);a.style.display=b?"":"none";if(!b){Kq().nM(this,this.uc);while(a.lastChild)a.removeChild(a.lastChild)}}};
Gq.prototype.bO=function(a){var b=new xq(a,"REPLACE");this.V("render in renderElement");b.Jx();this.Jr(b,true);b.Ot()};
Gq.prototype.$N=function(a){this.Mf();for(var b=0;b<this.Cp.length;b++){var c=this.Cp[b];if(c.Ow)c.Ow(a,this);else{c.Vd();c.nx(this.uc);c.C(a)}}};
Gq.prototype.iu=function(){var a=this.Y1.L(this);if(this.uc)for(var b=0;b<this.uc.length;b++)a+="-"+this.uc[b];return a};
Gq.prototype.qb=function(){return Le(this.iu())};
Gq.prototype.YI=function(a){var b=this.IA(a),c=Mn(b).Gg();return c};
Gq.prototype.IA=function(a){var b=this.to?this.to.De:"";if(a)for(var c=0;c<a.length;c++)b=b.replace(/\*/,a[c]);return b};
Gq.prototype.V=function(a){var b=Bd;if(Hq.qv(b)){var c=[a,"View=",this.fc.Za," Node=",this.Za];for(var d=1;d<arguments.length;d++)c.push(arguments[d]);Hq.log(b,c.join(", "))}};
Gq.prototype.kaa=function(a,b,c){this.nx(c);var d=this.qb();if(d)this.bO(d)};
Gq.prototype.waa=function(a,b,c){var d=Le(this.eO);if(d){c=c||[];var e=this.Zz;for(var f=0;f<c.length;f++)e=this.Zz[String(c[f])];for(var g in e){var h=e[g],i=Le(h);if(i)i.parentNode.removeChild(i)}for(var g in e)delete e[g];this.nx(c);var k=new xq(Le(this.eO),"INSERT_BEFORE");this.V("render in updateRepeated");k.Jx();this.Jr(k,null,true);k.Ot()}};
Gq.prototype.lU=function(a,b,c){this.nx(c);var d=this.qb();if(d)this.C5(d)};
Gq.prototype.Ol=function(){return true};
Gq.prototype.Na=function(a){if(a=="$Context"){var b=this.IA(this.uc),c=Mn(b);if(c.Gg())var d=c.Gg().Fa();else if(this.uc)d=this.uc[this.uc.length-1];if(c&&c.Ia()){var e=c.Ia().Gg(),f=e.vb().S()-1,g=e.vb().indexOf(d)}if(b!="")b=b+"/";var h=this.uc.length>0?"-"+this.uc.join("-"):"",i=this.fc?this.fc.Qea:"",k=new Vn({"@dataName":d,"@cid":h,"@vid":i,"@dataPath":b,"@last":f,"@index":g},"Context");return k}else return a=="$View"?this.fc.Aea:(a.indexOf("$")==0?In().Na(a):(this.rn?this.rn.Na(a):null))};
Gq.prototype.lc=function(a){return a=="$Context"?new Vn({},"Context"):(a.indexOf("$")==0?In().lc(a):(this.rn?this.rn.lc(a):""))};
Gq.prototype.vb=function(a){return this.rn?this.rn.vb(a):new Kn};
Gq.prototype.gu=function(){return this};
var Mq=function(a,b){this.wE=a;this.D$=Fq(a,b);this.kh=this.D$.jd()};
Mq.prototype.Ow=function(a,b){a.ct(Ra(this.D$.L(b)))};
Mq.prototype.jd=function(){return this.kh};
Mq.prototype.Ol=function(){return false};
var Oq=function(a,b,c,d){Gq.call(this,a,b,this,d);this.Aea=c;this.Qea=Nq++;this.vw=false};
o(Oq,Gq);var Nq=0,Pq=function(a,b,c,d){Gq.call(this,a,b,c,d)};
o(Pq,Gq);Pq.prototype.Jr=function(a){var b="span";this.Ix(a,b,this.eq());var c=this.Tm.text.L(this);if(c){c=c.replace("\n\r","\n");var d=c.split(/[\n\r]/);for(var e=0;e<d.length;e++){var f=d[e];if(e>0){a.fs("br",[]);a.Sp("br")}a.ct(f)}}this.Nt(a,b)};var Rq=function(){var a={};ad(Qq,function(b){a[b]=true});
this.oca=a;this.Tfa={};this.Haa={};this.JV={};this.Bfa=0;this.Ufa=[]},
Sq,Lq=function(){if(Sq==null)Sq=new Rq;return Sq};
Rq.prototype.FV=function(a,b,c,d,e){d=d||0;var f=a.nodeName;if(f=="#text")return new Mq(a.nodeValue,e);else if(f=="#comment")return new uq;else if(f=="MultilineText")return new Pq(a,b,c,d);else if(this.oca[f])return new Gq(a,b,c,d);else if(f=="View")return new Oq(a,b,null,d);else if(this.Haa[f])return new Oq(this.Haa[f],b,this.sV(a),d);else if(this.JV[f])return this.JV[f](a,b,c);else throw Error("No view exists matching name: "+f);};
Rq.prototype.sV=function(a){var b={},c=a.attributes.length;for(var d=0;d<c;++d){var e=a.attributes[d];b["@"+e.nodeName]=e.nodeValue}return bo(b,"$View")};
var Qq=["p","div","b","i","br","ul","li","a","hr","span","img","table","tr","select","option","td","th","tbody","#text","h1","h2","h3","dl","dt","input","iframe","form","textarea","col","#comment","strike","button","script","style","thead","link","em"];var Tq="addshaperequested",Uq="albumlistloaded",Vq="applypersonrequested",Wq="editpersonrequested",Xq="communicationerror",Yq="mouseoutpersontag",Zq="mouseoverpersontag",$q="mouseclickpersontag",ar="photoschanged",br="photoshapesloaded",cr="photourlreceived",dr="subjectlistloaded",er="subjectshapesloaded",fr="subjectschanged";{var hr=function(){var a=u("lhid_frOpt");if(a){var b=Me(null,"lhcl_frOpt-optIn",a)[0];if(b){var c=m("Start processing photos"),d=new Sk(c);d.Jd("gphoto-froptbutton");F(d,xk,gr);d.C(b)}}},
gr=function(){var a=document.getElementById("lhid_optInForm");a.froptin.value="true";a.submit()},
ir=function(){var a=document.getElementById("lhid_optInForm");a.froptin.value="false";a.submit()}};{var jr=function(){this.p=new K(this);var a=u("lhid_name_tags_opt_status");this.JC=a.value=="true"?true:false;this.gg=this.JC;if(!this.gg)M(u("lhid_default_album_name_tags"),false);this.HC=new Sk("");this.HC.Jd("lhcl_frOpt-Button");this.p.g(this.HC,xk,this.V4);var b=u("lhid_name_tags_button");this.HC.C(b);this.QQ()},
kr="lhcl_gphoto_fr_opt_settings_handler",lr=m("Turn name tags"),mr=m("Name tags are"),nr=m("ON"),or=m("OFF"),pr=function(){new jr};
jr.prototype.V4=function(){this.gg=!this.gg;var a=u("lhid_name_tags_opt_status");a.value=this.gg?"true":"false";this.QQ();this.taa()};
jr.prototype.QQ=function(){this.HC.Sg(lr+" "+(!this.gg?nr:or));var a=u("lhid_name_tags_status");B(a,mr+" "+(this.gg?nr:or)+".")};
jr.prototype.taa=function(){var a=m("Note!"),b=m('Click the "Save Settings" button at the top or buttom of this page to turn name tags'),c=m("To start we need to sort and group faces in your photos which can take a few minutes"),d=m("Turning name tags off will delete your name tags data."),e=m("If you want to use name tags, but don't want anyone else to see them, try hiding name tags for both public and unlisted albums."),f=u("lhid_name_tags_notes");if(this.JC==this.gg){We(f);if(this.JC)M(u("lhid_name_tags_fine"),
true);else M(u("lhid_default_album_name_tags"),false)}else if(this.JC){M(u("lhid_name_tags_fine"),false);var g=v("div",{"class":kr}),h=v("span",{"class":kr+"-warning"});B(h,d);var i=Ue(" "+e),k=v("div",{"class":kr}),n=v("span",{"class":kr+"-note"});B(n,a);var q=Ue(" "+b+" "+or+".");x(g,h);x(g,i);x(k,n);x(k,q);x(f,g);x(f,k)}else{M(u("lhid_default_album_name_tags"),true);var k=v("div",{"class":kr}),n=v("span",{"class":kr+"-note"});B(n,a);var q=Ue(" "+b+" "+nr+". "+c+".");x(k,n);x(k,q);x(f,k)}}};var qr=function(a,b,c,d,e){this.canonicalTitle=d;this.id=b;this.numPhotos=e;this.ownerId=a;this.stats={fullyProcessedPhotos:0,namedShapes:0,nonFaces:0,strangers:0,skipped:0,numProcessedPhotos:0,partiallyProcessedPhotos:0,hiddenShapes:0,unnamedShapes:0};this.title=c};{var vr=function(a){this.efa=Rc(a,"clusterindex");this.xa={};this.$o=[];if(xa(a))r(a,function(f){this.xa[f.H()]=f},
this);else{var b=rr(),c=a.faces,d=a.shapes;this.xa=sr(c,d);var e=[];Gc(this.xa,function(f,g){if(!f.dc())f.Bx(b.client.qe()[tr]);var h=b.client.ac()[g];if(h)this.xa[g]=h;else e.push(f)},
this);b.client.My(e);if(a.subjects)r(a.subjects,function(f){this.$o.push(new ur(f))},
this)}};
vr.prototype.ac=function(){return this.xa}};{var zr=function(){this.client=new wr(this);this.combined=new xr(this);this.server=new yr(this)};
o(zr,Vd);var Ar="delete",Br="modify",Cr={ADD:"add",DELETE:Ar,MODIFY:Br},Dr=null;zr.prototype.b=function(){if(this.W())return;this.client.b();this.combined.b();this.server.b()};
var rr=function(){if(!Dr)Dr=new zr;return Dr}};{var Er=function(a,b){b=b||"";this.rp=Rc(a,b+"aid",null);this.Za=a.id;this.xa={};this.cp=Rc(a,"url",null);var c=a[b+"shapes"];if(c){r(c,function(e){this.xa[e.H()]=e},
this);this.Ik()}if(!b){this.xo={};var d=Jc(a).join(",");Gc(Cr,function(e){var f=e+"_";if((new RegExp("(^|,)"+f)).test(d))this.xo[e]=new Er(a,f)},
this)}};
Er.prototype.xo=null;Er.prototype.dh=function(a){this.xa[a.H()]=a;this.Ik()};
Er.prototype.dq=function(a){if(a){var b=this.xo[a];return b&&b.dq()||null}return this.rp};
Er.prototype.H=function(){return this.Za};
Er.prototype.ac=function(a){if(a){var b=this.xo[a];return b&&b.ac()||null}return this.xa};
Er.prototype.uq=function(){return this.cp};
Er.prototype.d4=function(a){var b=a.SA("add");if(b)this.W3(b);var c=a.SA(Ar);if(c)this.Y3(c);var d=a.SA(Br);if(d)this.a4(d)};
Er.prototype.Lw=function(a){Nc(this.xa,a.H())};
Er.prototype.r9=function(a){this.cp=a};
Er.prototype.SA=function(a){return this.xo&&this.xo[a]||null};
Er.prototype.W3=function(a){var b=a.xa;if(b){Uc(this.xa,b);this.Ik()}};
Er.prototype.Y3=function(a){var b=a.xa;if(b)Gc(b,function(c){Nc(this.xa,c.H())},
this)};
Er.prototype.a4=function(a){this.cp=a.cp||this.cp};
Er.prototype.Ik=function(){if(this.xa)Gc(this.xa,function(a){a.Z8(this)},
this)}};{var Fr=function(a,b){b=b||"";this.Bz=a[b+"createdby"]||"Auto";this.mB=a.hasfacetemplate||false;this.Za=a.id;this.Hb=Rc(a,b+"orientation",0);this.gs=Rc(a,b+"subject",null);this.fQ=Rc(a,b+"subtype",0);this.dQ=Rc(a,b+"style",null);this.wE=Rc(a,b+"text",null);this.Eea=Rc(a,"url",null);this.da=a.type;this.lR=Rc(a,b+"zorder",0);var c=a[b+"photo"],d=a[b+"photoid"],e=rr().client.lY(),f=Rc(a,b+"aid",e&&e.id||null),g=a[b+"photourl"];this.ba=c||new Er({id:d,aid:f,url:g});this.ba.dh(this);var h=a[b+"x1"],i=
a[b+"y1"],k=a[b+"x2"],n=a[b+"y2"],q=a[b+"w"],t=a[b+"h"];if(!isNaN(h*i*k*n*q*t)){this.Vm=new ke(h,i,k-h,n-i);this.vo=new ne(q,t)}var y=a[b+"suggestions"];if(y)this.$o=gc(y,function(O){if(!(O instanceof ur))O=new ur(O);return O});
if(!b){this.Ew={};var z=Jc(a).join(",");Gc(Cr,function(O){var aa=O+"_";if((new RegExp("(^|,)"+aa)).test(z))this.Ew[O]=new Fr(a,aa)},
this)}};
Fr.prototype.Vm=null;Fr.prototype.ba=null;Fr.prototype.vo=null;Fr.prototype.Ew=null;Fr.prototype.$o=null;Fr.prototype.getBounds=function(){return this.Vm};
Fr.prototype.UI=function(){return this.Bz};
Fr.prototype.H=function(){return this.Za};
var Gr=function(a){return gc(a,function(b){return b.H()})};
Fr.prototype.Sf=function(){return this.ba};
Fr.prototype.PJ=function(){return this.vo};
Fr.prototype.dc=function(){return this.gs};
var Hr=function(a){var b=[];Gc(a,function(c){b.push(c.dc())},
this);return b};
Fr.prototype.Gl=function(){return this.Eea};
Fr.prototype.Xa=function(){return this.da};
Fr.prototype.C1=function(){return this.mB};
Fr.prototype.Yj=function(){return!Na(this.Za,"tempid-")};
Fr.prototype.vf=function(){return this.gs.vf()};
Fr.prototype.f4=function(a){var b=a.pZ(Br);if(b)this.b4(b)};
Fr.prototype.Rw=function(a){this.Za=a};
Fr.prototype.OO=function(a,b){this.Vm=a;this.vo=b};
Fr.prototype.Z8=function(a){this.ba=a};
Fr.prototype.Bx=function(a){this.gs=a};
Fr.prototype.pZ=function(a){return this.Ew&&this.Ew[a]||null};
Fr.prototype.b4=function(a){this.Bz=a.Bz||this.Bz;this.mB=a.mB||this.mB;this.Hb=a.Hb||this.Hb;this.gs=a.gs||this.gs;this.fQ=a.fQ||this.fQ;this.dQ=a.dQ||this.dQ;this.wE=a.wE||this.wE;this.lR=a.lR||this.lR;this.Vm=a.Vm||this.Vm;this.vo=a.vo||this.vo;this.$o=a.$o||this.$o}};{var U=function(a,b){b=b||"";var c=a.type;this.wg=Rc(a,b+"dispname",null);this.Bj=Rc(a,b+"email",null);this.Tn=Rc(a,b+"iconicshapeurl",null);this.Za=a.id;this.Og=Rc(a,b+"name",null);this.OC=Rc(a,b+"gphotousername",null);this.Eda=!(!Rc(a,b+"gphotouser",false));this.xa=null;this.s$=Rc(a,"subjectsearchurl");this.da=Ba(c)?c:1;this.YC=Rc(a,b+"publiclyvisible",null);var d=Rc(a,b+"numfaceshapes"),e=Rc(a,b+"numphotos"),f=Rc(a,b+"faces"),g=Rc(a,b+"shapes");if(f||g){this.xa=sr(f,g);this.Ik()}this.Af=d||0;this.iw=
e||0;if(!b){this.Fw={};var h=Jc(a).join(",");Gc(Cr,function(i){var k=i+"_";if((new RegExp("(^|,)"+k)).test(h))this.Fw[i]=new U(a,k)},
this)}},
Ir=null,Jr="img/noportrait.jpg?sky=lhm";U.prototype.Af=null;U.prototype.iw=null;U.prototype.Fw=null;U.prototype._subject_=true;var Kr=function(a,b){return Xa(a.Lb(),b.Lb())};
U.prototype.au=function(a,b){var c=b||15;return this.$x(this.wg,c,a)};
U.prototype.Lb=function(a,b){var c=this.wg||this.Og||this.Bj||Lr,d=b||15;return this.$x(c,d,a)};
U.prototype.ne=function(a,b){var c=b||40;return this.$x(this.Bj,c,a)};
var sr=function(a,b){var c={};r([a,b],function(d,e){if(d)r(d,function(f){if(!(f instanceof Fr)){f.createdby=e==0?"Auto":"User";f.type="Face";f=new Fr(f)}c[f.H()]=f})});
return c};
U.prototype.D8=function(a){this.Tn=a};
U.prototype.l8=function(a){this.wg=a};
U.prototype.xl=function(){if(this.Tn&&(this.Tn.indexOf(Jr)<0||Hc(this.xa)==0))return this.Tn;if(this.xa)for(var a in this.xa)return this.xa[a].Gl();return Jr};
U.prototype.H=function(){return this.Za};
var Mr=function(a){return gc(a,function(b){return b.H()})};
U.prototype.vJ=function(){if(Ir==this)return Infinity;return this.Mj()};
U.prototype.ac=function(a){if(a){var b=this.Fw[a];return b&&b.ac()||null}return this.xa};
U.prototype.getName=function(a,b){var c=b||40;return this.$x(this.Og,c,a)};
U.prototype.Mj=function(){return this.Af||0};
U.prototype.mZ=function(){return this.OC||null};
U.prototype.R2=function(){return this.Eda};
U.prototype.IZ=function(){return this.s$};
U.prototype.Xa=function(){return this.da};
U.prototype.Yj=function(){return!Na(this.Za,"tempid-")};
U.prototype.vf=function(){return this.da==1};
U.prototype.BL=function(){return this.YC};
U.prototype.h4=function(a){var b=a.TA("add");if(b)this.X3(b);var c=a.TA(Ar);if(c)this.Z3(c);var d=a.TA(Br);if(d)this.c4(d)};
var Nr=function(a){return a.vf()},
Or=function(a,b){return b.Mj()-a.Mj()};
U.prototype.Sw=function(a){this.Za=a};
U.prototype.k9=function(a){this.s$=a};
U.prototype.s9=function(){Ir=this};
U.prototype.Jk=function(){this.Af=0;this.da=1};
U.prototype.TA=function(a){return this.Fw[a]};
U.prototype.X3=function(a){if(this.Af!=null)this.Af+=a.Af||0;var b=a.xa;if(b){if(!this.xa)this.xa={};Uc(this.xa,b);this.Ik()}};
U.prototype.Z3=function(a){if(this.Af!=null)this.Af-=a.Af||0;var b=a.xa;if(b&&this.xa)Gc(b,function(c){Nc(this.xa,c.H())},
this)};
U.prototype.c4=function(a){this.wg=a.wg||this.wg;var b=wa(a.Bj);this.Bj=b?a.Bj:this.Bj;this.Tn=a.Tn||this.Tn;this.Og=a.Og||this.Og;this.Af=Ba(a.Af)?a.Af:this.Af;this.OC=a.OC||this.OC;this.xa=a.xa||this.xa;if(a.YC!=null)this.YC=a.YC;this.Ik()};
U.prototype.$x=function(a,b,c){if(Sa(a))return"";if(b>0)a=ob(a,b);if(c)a=p(a);return a};
U.prototype.Ik=function(){if(this.xa)Gc(this.xa,function(a){a.Bx(this)},
this)};
U.prototype.toString=function(){return this.Lb()}};var ur=function(a){this.confidenceScore=a.confidencescore;this.subject=rr().client.qe()[a.id]};{var wr=function(a){J.call(this);this.Ld={};this.B=a;this.p=new K(this);this.ad={};this.xa={};this.aj={numTotalFacesInAlbum:0,numAssignedFacesInAlbum:0,numUnassignedFacesInAlbum:0,numHiddenFacesInAlbum:0,numUnassignedFaceClusters:0};this.Mb={};this.ZQ={};this.clearData()};
o(wr,J);var tr="100001";wr.prototype.Ez=null;wr.prototype.JF=function(a){var b=this.cB("aname");r(this.Mm(a),function(c){this.Ld[c.id]=c;if(c.canonicalTitle==b)this.UO(c)},
this)};
wr.prototype.TS=function(a){r(this.Mm(a),function(b){this.ad[b.H()]=b},
this)};
wr.prototype.My=function(a,b){var c=[];a=this.Mm(a);if(a.length==0)return;var d=a[0].dc().H(),e=!ic(a,function(f){return f.dc().H()!=d});
if(e)c.push(new U({id:d,add_shapes:a,add_numshapes:b?0:a.length}));else r(a,function(f){c.push(new U({id:f.dc().H(),add_shapes:[f],add_numshapes:b?0:1}))},
this);this.Ji(Br,c)};
wr.prototype.kj=function(a){a=this.Mm(a);var b=[];r(a,function(c){b.push(c)},
this);if(b.length>0)this.Ji("add",b)};
wr.prototype.clearData=function(){Oc(this.Ld);Oc(this.ad);Oc(this.xa);this.aj.numAssignedFacesInAlbum=0;this.aj.numUnassignedFacesInAlbum=0;this.aj.numUnassignedFaceClusters=0;this.RG();if(this.B.combined)this.B.combined.R6()};
wr.prototype.RG=function(){Oc(this.Mb);this.kj(new U({id:tr,name:"",dispname:Lr,type:0}));this.kj(new U({id:"100002",name:null,dispname:null,type:2}));this.kj(new U({id:"100003",name:null,dispname:null,type:3}));this.kj(new U({id:"100004",name:null,dispname:null,type:4}))};
wr.prototype.b=function(){if(this.W())return;wr.f.b.call(this);delete this.Ld;delete this.B;delete this.ad;delete this.xa;delete this.aj;delete this.Mb;delete this.ZQ;this.p.b()};
wr.prototype.Gn=function(){return this.Ld};
wr.prototype.lY=function(){return this.Ez};
wr.prototype.Cl=function(){return this.ad};
wr.prototype.ac=function(){return this.xa};
wr.prototype.qq=function(a,b){var c=Ic(this.qe()),d=b?fc(c,b):c;return d.sort(a||Kr)};
wr.prototype.OZ=function(a,b){var c=a||function(e,f){return xc(f.vJ(),e.vJ())},
d=this.qq(c,function(e){var f=e.H(),g=0!=e.Xa()&&f!=tr&&f!="100002"&&f!="100003"&&f!="100004";return b?b(e)&&g:g});
return d};
wr.prototype.qe=function(){return this.Mb};
wr.prototype.cB=function(a){return this.ZQ[a]};
wr.prototype.vM=function(a){this.Ji(Br,a)};
wr.prototype.ve=function(a,b,c){c=c||0;a.s9();b=this.Mm(b);this.hD(b);this.Ji(Br,new U({id:a.H(),add_shapes:b,add_numshapes:b.length||c}))};
wr.prototype.WN=function(a){this.cr(Ar,a)};
wr.prototype.hD=function(a,b){this.Ji(Br,gc(this.Mm(a),function(c){return new U({id:c.dc().H(),delete_shapes:[c],delete_numshapes:b?0:1})}))};
wr.prototype.Ri=function(a){this.Ji(Ar,a)};
wr.prototype.ok=function(a,b,c,d){this.Ji(Br,new U({id:a.H(),modify_name:b,modify_dispname:c,modify_email:d}))};
wr.prototype.Y6=function(a,b){var c=this.Mb[a];c.k9(b)};
wr.prototype.Rw=function(a,b){var c=this.xa[a];delete this.xa[a];this.xa[b]=c;c.Rw(b);var d=c.dc();if(d){var e=d.ac();delete e[a];e[b]=c}var f=c.Sf();if(f){var g=f.ac();delete g[a];g[b]=c}};
wr.prototype.Sw=function(a,b){var c=this.Mb[a];delete this.Mb[a];this.Mb[b]=c;c.Sw(b)};
wr.prototype.UO=function(a){var b=a||null;if(this.Ez!=b){this.Ez=b;this.dispatchEvent("currentalbumchanged")}};
wr.prototype.wm=function(a,b){this.Ji(Br,new U({id:a.H(),modify_iconicshapeurl:b}))};
wr.prototype.Cm=function(a,b){this.Ji(Br,new U({id:a.H(),modify_publiclyvisible:b}))};
wr.prototype.CP=function(a,b){this.ZQ[a]=b};
wr.prototype.lC=function(a,b,c,d){d=this.Mm(d);var e={};r(d,function(f){var g=f.H(),h=c[g];e[g]=true;switch(a){case "add":if(!h)b.call(this,f);c[g]=f;break;case Ar:case Br:if(h)b.call(this,f);break}},
this);return Jc(e)};
wr.prototype.cr=function(a,b){this.dispatchEvent({type:ar,subType:a,photoIds:this.lC(a,this.e4[a],this.ad,b)})};
wr.prototype.e4={add:function(){},
"delete":function(a){this.hD(Ic(a.ac()),true);delete this.ad[a.H()]},
modify:function(a){this.ad[a.H()].d4(a)}};
wr.prototype.uM=function(a,b){this.dispatchEvent({type:"shapeschanged",subType:a,shapeIds:this.lC(a,this.g4[a],this.xa,b)})};
wr.prototype.g4={add:function(a){var b=a.Sf();if(!this.ad[b.H()])this.cr("add",b);else this.cr(Br,new Er({id:b.H(),add_shapes:[a]}))},
"delete":function(a){delete this.xa[a.H()];this.cr(Br,new Er({id:a.Sf().H(),delete_shapes:[a]}))},
modify:function(a){Gc(Cr,function(c){var d=a.Sf(c);if(d)this.cr(c,d)},
this);var b=this.xa[a.H()];if(b)this.xa[a.H()].f4(a)}};
wr.prototype.Ji=function(a,b){this.dispatchEvent({type:fr,subType:a,subjectIds:this.lC(a,this.i4[a],this.Mb,b)})};
wr.prototype.i4={add:function(a){this.uM("add",Ic(a.ac()))},
"delete":function(a){if(a.Xa()==0)throw Error("Cannot delete unnamed subject");this.ve(this.qe()[tr],Ic(a.ac(),a.Mj()));delete this.Mb[a.H()]},
modify:function(a){Gc(Cr,function(b){var c=a.ac(b);if(c)this.uM(b,Ic(c))},
this);this.Mb[a.H()].h4(a)}};
wr.prototype.Mm=function(a){return xa(a)?a:[a]}};{var xr=function(a){J.call(this);this.V=Ed(this.D());this.B=a;this.xfa={};this.dM={};this.eM={}};
o(xr,J);var Pr=0;xr.prototype.dh=function(a,b){this.B.server.dh(a,l(function(c){var d,e=Qr(c.Sb());try{d=yj(e)}catch(f){return}this.B.client.Rw(a.H(),d.shapeid);if(b)b();this.wj()},
this));this.B.client.My(a)};
xr.prototype.Ps=function(a,b,c,d,e,f,g,h,i,k){var n=new U({id:e||Rr(),name:a,dispname:b,email:c,type:1,numshapes:0,shapes:[]});this.B.server.Ps(n,l(function(q){var t,y=Qr(q.Sb());try{t=yj(y)}catch(z){return}var O=n.H();this.B.client.Y6(O,t.subjectsearchurl);this.B.client.Sw(O,t.subjectid);this.ve(n,d,f,h,i,k);var aa=m("New person added: {$name}",{name:n.Lb()});this.wj(aa,function(){this.Ri([n])})},
this),g);this.B.client.kj(n)};
xr.prototype.su=function(a){var b=a.uq();if(b)this.dispatchEvent({type:cr,photo:a});else this.B.server.su(a,l(this.R_,this,a))};
var Rr=function(){return"tempid-"+Pr++};
xr.prototype.h3=function(a){if(!this.eT)if(this.dT)this.dispatchEvent(Uq);else{this.eT=true;this.B.server.xW(a,l(this.D_,this))}};
xr.prototype.i3=function(a){if(!this.aba)this.B.server.yW(a,l(this.E_,this));this.aba=false};
xr.prototype.n3=function(a,b,c){var d=[];for(var e=0;e<a.length;e++){var f=a[e];if(!this.dM[f])if(!this.B.client.Cl()[f]){this.dM[f]=true;d.push(f)}}if(d.length>0)this.B.server.zZ(d,b,c,l(this.S_,this,d));else this.dispatchEvent({type:br,photoIds:a})};
xr.prototype.o3=function(a){this.V.jf("loadShapesForSubjects");var b=[],c=a.length;for(var d=0;d<c;++d){var e=a[d],f=e.H();if(!this.eM[f]||!e.vf()||e.Mj()!=Hc(e.ac())){b.push(e);this.eM[f]=true}}if(b.length==0)this.dispatchEvent({type:er});else this.B.server.AZ(b,l(this.T_,this))};
xr.prototype.co=function(a,b,c,d,e){if(!this.q$)if(this.gQ&&!e)this.dispatchEvent(dr);else{if(e)this.B.client.RG();this.q$=true;this.B.server.zW(a,b,c,d,l(this.F_,this))}};
xr.prototype.ar=function(a,b,c){b=fc(b,function(d){return d.vf()},
this);if(b.length==0)return;if(a.Xa()==-1){this.Jk(a,l(this.ar,this,a,b,c));return}this.B.server.ar(a,b,l(function(){if(c)c();var d=m("Subjects merged: {$numSubjects}",{numSubjects:b.length+1});this.wj(d)},
this));r(b,function(d){this.B.client.ve(a,Ic(d.ac()),d.Mj())},
this);this.B.client.Ri(b)};
xr.prototype.ve=function(a,b,c,d,e,f,g){b=fc(b,function(k){return k.dc().H()!=a.H()},
this);if(b.length==0)return;if(a.Xa()==-1){this.Jk(a,l(this.ve,this,a,b,c,null,null,null));return}var h=true,i=b[0].dc();r(b,function(k){if(i!=k.dc())h=false});
this.B.server.ve(a,b,l(function(){if(c)c(a,b);var k=h?l(this.ve,this,i,b,null,null,null,null,true):null,n;switch(a.H()){case tr:k=null;break;case "100004":var q=m("Photos skipped: {$numShapes}",{numShapes:b.length});n=q;break;case "100003":var t=m("Photos marked as non-faces: {$numShapes}",{numShapes:b.length});n=t;break;case "100002":var y=m("Photos marked as ignored faces: {$numShapes}",{numShapes:b.length});n=y;break;default:var z=m("{$numShapes} photos tagged.",{numShapes:b.length});n=z}this.wj(n,
k)},
this),d,e,f,g);this.B.client.ve(a,b)};
xr.prototype.Lw=function(a,b){this.B.server.YV(a,l(function(){if(b)b();var c=m("Shape removed");this.wj(c)},
this));this.B.client.hD([a])};
xr.prototype.Ri=function(a,b){this.B.server.Ri(a,l(function(){if(b)b();var c=m("People removed: {$numSubjects}",{numSubjects:a.length});this.wj(c)},
this));this.B.client.Ri(a)};
xr.prototype.ok=function(a,b,c,d,e){var f=a.getName(),g=a.Lb(),h=a.ne();this.B.server.ok(a,b,c,d,l(function(){if(e)e();var i=m("{$fromName} renamed to {$toName}",{fromName:g,toName:c});this.wj(i,function(){this.ok(a,f,g,h)})},
this));this.B.client.ok(a,b,c,d)};
xr.prototype.R6=function(){this.dT=false;this.gQ=false};
xr.prototype.wm=function(a,b,c){this.B.server.wm(a,b,l(function(){if(c)c();this.wj()},
this));this.B.client.wm(a,b.Gl())};
xr.prototype.Cm=function(a,b){this.B.server.Cm(a,b);this.B.client.Cm(a,b)};
xr.prototype.XD=function(a,b,c,d){this.B.server.XD(a,b,c,d)};
xr.prototype.j9=function(a){this.B.client.kj(gc(a,function(b){return new U(b)}));
this.gQ=true};
xr.prototype.Jk=function(a,b){a.Jk();this.B.server.Jk(a,b)};
xr.prototype.wj=function(a,b){this.dispatchEvent({type:xk,actionDescription:a||null,undo:b?l(b,this):null})};
xr.prototype.D_=function(a){var b,c=Qr(a.Sb());try{b=yj(c).feed.entry}catch(d){return}if(!b)b=[];this.B.client.JF(gc(b,function(e){return new qr(e.gphoto$user.$t,e.gphoto$id.$t,e.title.$t,e.gphoto$name.$t,e.gphoto$numphotos.$t)}));
this.dispatchEvent(Uq);this.dT=true;this.eT=false};
xr.prototype.E_=function(a){var b,c=Qr(a.Sb());try{b=yj(c)}catch(d){return}var e=this.AS(b);this.dispatchEvent({type:"albumstatsloaded",aggregate:e})};
xr.prototype.AS=function(a){var b=this.B.client.Gn(),c={fullyProcessedPhotos:0,namedShapes:0,nonFaces:0,strangers:0,skipped:0,nonProcessedPhotos:0,partiallyProcessedPhotos:0,hiddenShapes:0,unnamedShapes:0,totalFaces:0};r(a,function(d){var e={};e.albumId=d.albumid;e.fullyProcessedPhotos=d.numfullyprocessedphotos;c.fullyProcessedPhotos+=e.fullyProcessedPhotos;e.namedShapes=d.numnamedfaces+d.numnamedpersonshape;c.namedShapes+=e.namedShapes;e.nonFaces=d.numnotaface+d.numnotafacepersonshape;c.nonFaces+=
e.nonFaces;e.strangers=d.numdontcareface+d.numdontcarepersonshape;c.strangers+=e.strangers;e.skipped=d.numskippedface+d.numskippedpersonshape;c.skipped+=e.skipped;e.nonProcessedPhotos=d.numnotprocessedphotos;c.nonProcessedPhotos+=e.nonProcessedPhotos;e.partiallyProcessedPhotos=d.numdetectionprocessedphotos;c.partiallyProcessedPhotos+=e.partiallyProcessedPhotos;e.hiddenShapes=d.numdontcareface+d.numdontcarepersonshape+d.numskippedface+d.numskippedpersonshape+d.numnotaface+d.numnotafacepersonshape;
c.hiddenShapes+=e.hiddenShapes;e.unnamedShapes=d.numunnamedfaces+d.numunnamedpersonshape;c.totalFaces+=e.namedShapes+e.unnamedShapes;var f=b[d.albumid];if(f)f.stats=e},
this);return c};
xr.prototype.F_=function(a){var b,c=Qr(a.Sb());try{b=yj(c)}catch(d){return}this.B.client.kj(gc(b,function(e){return new U(e)}));
this.dispatchEvent(dr);this.gQ=true;this.q$=false};
xr.prototype.R_=function(a,b){var c,d=Qr(b.Sb());try{c=yj(d).imgsrc}catch(e){return}a.r9(c);this.dispatchEvent({type:cr,photo:a})};
xr.prototype.S_=function(a,b){var c,d=Qr(b.Sb());try{c=yj(d)}catch(e){return}for(var f=0;f<a.length;f++){var g=this.B.client.Cl()[a[f]];if(g)this.B.client.WN(g)}var h=this.B.client.qe();r(c,function(i){var k=new U(i),n=h[k.H()];if(n)this.B.client.My(Ic(k.ac()),true);else this.B.client.kj(k)},
this);this.dispatchEvent({type:br,photoIds:a});for(var f=0;f<a.length;f++)this.dM[a[f]]=false};
xr.prototype.T_=function(a){this.V.jf("handleGetShapesForSubject_");var b=Qr(a.Sb()),c;try{c=yj(b)}catch(d){return}var e=[],f=c.length;for(var g=0;g<f;++g){var h=new U(c[g]);e.push(h);var i=h.H();this.V.jf("handleGetShapesForSubject_: sid="+i);var k=this.B.client.qe()[i].ac(),n=new U({id:i,delete_shapes:k});this.B.client.vM(n);var q=Ic(h.ac()),t=new U({id:i,add_shapes:q,modify_numshapes:q.length});this.B.client.vM(t);this.eM[i]=false}this.dispatchEvent({type:er})};
xr.prototype.D=function(){return"gphoto.sapphire.data.datahandler.Combined"}};{var yr=function(a){J.call(this);this.V=Ed(this.D());this.HT=0;this.B=a;this.gL=new pk(0,null,1,1,0);this.V2=new Ym(this.j7,500,this)};
o(yr,J);yr.prototype.yv=null;yr.prototype.fk=0;yr.prototype.mN=0;yr.prototype.Ni=0;var Sr=_createShapePath,Tr=_createSubjectPath,Ur="/lh/subjectList",Vr=_updateShapePath,Wr=_updateSubjectPath;yr.prototype.dh=function(a,b){var c=a.getBounds(),d=a.PJ();this.Ad(Sr,b,[["iid",a.Sf().H()],["sid",a.dc().H()],["stype",a.Xa()],["zorder",0],["x1",Math.round(c.left)],["y1",Math.round(c.top)],["x2",Math.round(c.left+c.width)],["y2",Math.round(c.top+c.height)],["w",d.width],["h",d.height]])};
yr.prototype.Ps=function(a,b,c){var d=[["sname",a.getName()],["dname",a.Lb()]],e=a.ne();if(e)d.push(["email",e]);var f=a.H();if(a.Yj()&&f)d.push(["coid",f]);if(Aa(c))d.push(["ndnick",c]);this.Ad(Tr,b,d)};
yr.prototype.YV=function(a,b){this.Ad(Vr,b,[["opt","DELETE_SHAPE"],["iid",a.Sf().H()],["shids",a.H()]])};
yr.prototype.xW=function(a,b){this.Ad("/data/feed/api/user/"+(a||this.B.client.cB("uname")),b,[["kind","album"],["alt","json"]],"GET")};
yr.prototype.yW=function(a,b,c){var d=[];if(b)d.push(["throt",true]);if(a)d.push(["aids",a.join(",")]);this.Ad("/lh/albumStats",c,d)};
yr.prototype.zW=function(a,b,c,d,e){var f=[];if(a)f.push(["uname",a]);if(b)f.push(["max",b]);if(c)f.push(["cd",c]);if(d)f.push(["aids",d.join(",")]);this.Ad(Ur,e,f)};
yr.prototype.AZ=function(a,b){this.V.jf("getShapesForSubjects");var c=gc(a,function(d){return d.H()},
this);this.V.jf("sids="+c.join(","));this.Ad(Ur,b,[["sids",c.join(",")],["fd",true],["sd",true]])};
yr.prototype.su=function(a,b){this.Ad("/lh/rssPhoto",b,[["uname",this.B.client.cB("uname")],["iid",a.H()],["js",true]],"GET")};
yr.prototype.zZ=function(a,b,c,d){var e=[["fd",true],["sd",true],["sg",b]];e.push(["iids",a.join(",")]);if(c)e.push(["uname",c]);var f=(new N(document.location.href)).Ga("authkey");if(f)e.push(["authkey",f]);this.Ad(Ur,d,e)};
yr.prototype.hv=function(){return this.HT>0};
yr.prototype.ar=function(a,b,c){if(b.length==0)return;this.Ad(Wr,c,[["opt","MERGE_SUBJECTS"],["sid",a.H()],["sids",Mr(b).join(",")]])};
yr.prototype.ve=function(a,b,c,d,e,f,g){if(b.length==0)return;var h=[],i=[];r(b,function(n){if(n.Xa()=="Face"&&n.UI()=="Auto")h.push(n.H());else i.push(n)});
var k=[["opt","MOVE_FACES"],["sid",typeof a=="string"?a:a.H()]];if(h.length>0)k.push(["fids",h.join(",")]);if(i.length>0)k.push(["shids",Gr(i).join(",")]);if(d)k.push(["ctype",d]);if(e)k.push(["ptype",e]);if(f)k.push(["cscore",f]);if(g)k.push(["undo",g]);this.Ad(Wr,c,k)};
var Qr=function(a){if(Na(a,"while(1);"))a=a.substr(9);return a};
yr.prototype.Ri=function(a,b){this.Ad(Wr,b,[["opt","DELETE_SUBJECTS"],["sids",Mr(a).join(",")]])};
yr.prototype.ok=function(a,b,c,d,e){this.Ad(Wr,e,[["opt","RENAME_SUBJECT"],["sid",a.H()],["sname",b],["dname",c],["email",d]])};
yr.prototype.ED=function(a){var b=this.hv();this.HT+=a?1:-1;if(b!=this.hv())this.dispatchEvent("busymodechanged")};
yr.prototype.wm=function(a,b,c){this.Ad(Wr,c,[["opt","SET_ICONIC_FACE"],["sid",a.H()],["fid",b.H()]])};
yr.prototype.Cm=function(a,b,c){this.Ad(Wr,c,[["opt","SET_PUBLICLY_VISIBLE"],["sid",a.H()],["pvisible",b]])};
yr.prototype.XD=function(a,b,c,d){var e=[["pref",b]];if(c)e.push(["value",c]);this.Ad(a,d,e)};
yr.prototype.Jk=function(a,b,c){var d=a.getName(),e=d.indexOf(" "),f=e>=0?d.substr(0,Math.min(15,e)):d,g=[["coid",a.H()],["sname",d],["dname",f||Lr]],h=a.ne();if(h)g.push(["email",h]);if(Aa(c))g.push(["ndnick",c]);this.Ad(Tr,b,g)};
var Xr=function(a){var b=new Ui,c=a.length;for(var d=0;d<c;d++){var e=a[d];b.add(e[0],e[1])}return b.toString()};
yr.prototype.T1=function(a,b,c,d){if(d.id==b){this.gO(a,c);this.B.server.ED(false);this.dispatchEvent(Xq)}};
yr.prototype.U1=function(a,b,c,d,e){if(e.id==c){this.gO(a,d);var f=true;if(a)if(this.Ni>0){f=false;this.yv=function(){b(e.xhrLite);this.yv=null}}this.V2.start();
if(f&&b)b(e.xhrLite);this.B.server.ED(false)}};
yr.prototype.Ad=function(a,b,c,d){var e=a=="/lh/nameTagPaging";if(e)this.mN++;this.Ni++;if(!c)c={};this.B.server.ED(true);d=d||"POST";var f=Xr(c);if(d=="GET"){a+="?"+f;f=""}var g=this.fk++,h=new K(this);h.g(this.gL,Li,l(this.U1,this,e,b,g,h));h.g(this.gL,Mi,l(this.T1,this,e,g,h));this.gL.send(g,a,d,f);return g};
yr.prototype.gO=function(a,b){b.b();this.Ni--;if(a)this.mN--};
yr.prototype.j7=function(){if(this.Ni==0){if(this.yv!=null)this.yv()}else this.V2.start()};
yr.prototype.D=function(){return"gphoto.sapphire.data.datahandler.Server"}};var Yr="pwa.";var Zr=/\?.*$/,$r=/\/s\d+(?:-c)?\//,as=function(a){return a.replace(Zr,"").replace($r,"/")},
bs=function(a){var b=as(a),c=b.lastIndexOf("/"),d=b.substr(0,c),e=b.substr(c);return d+"/d"+e},
cs=function(a,b,c){var d=b instanceof ne?Math.max(b.width,b.height):b;if(!d)return a;var e=a.lastIndexOf("/"),f=a.substr(0,e),g=a.substr(e),h="/s"+d+(c?"-c":"");return f+h+g},
gs=function(a,b){var c=a.ra();if(c.Yp(b)||c.Yp(ds))return c;c.uD(b);return es(c,fs)},
is=function(a,b){var c=a.ra();if(c.Yp(hs))return hs;var d=Math.min(c.width,c.height);c.width=d;c.height=d;if(!c.Yp(b))c.uD(b);return es(c,[32,48,64,72,104,136,144,150,160])},
es=function(a,b){var c=Math.floor(Math.max(a.width,a.height));if(c){var d=js(c,b);return a.scale(d/c).floor()}else return a.floor()},
js=function(a,b){var c=b[0];for(var d=1;d<b.length;d++)if(a>=b[d])c=b[d];else break;return c},
fs=[32,48,64,72,94,104,110,128,144,150,160,200,220,288,320,400,512,576,640,720,800,912,1024,1152,1280,1440,1600],ds=new ne(fs[0],fs[0]),hs=ds;var ks=function(){Vd.call(this);this.Rz={};this.Ni={};this.s3=new lk(100,true);this.j=new K(this)},
ls;o(ks,Vd);ks.prototype.N6=0;ks.prototype.b=function(){if(!this.W()){ks.f.b.call(this);Gc(this.Rz,function(a){a.b()});
this.j.b()}};
ks.prototype.ua=function(a,b,c,d,e,f,g){var h=++this.N6,i=this.XA(a,d,true);if(b){var k=Math.max(b.width,b.height);if(k<=c)c=0}var n=new ms(h,a,c,d,e,f,g);if(!mc(i,c)){this.Ni[h]=n;var q=this.dJ(a);q.ua(n)}else this.$G(n,Ki);return h};
ks.prototype.cf=function(a){var b=Rc(this.Ni,a);if(b){Nc(this.Ni,a);var c=this.dJ(b.baseUrl);c.remove(b);return true}return false};
ks.prototype.ZY=function(a,b,c){var d=-1,e=this.XA(a,!(!c),false);if(e&&e.length)if(b==0)d=e[0]==0?0:bc(e);else for(var f=0;f<e.length;f++){var g=e[f];if(g<b)d=g;else{if(g<=b*2)d=g;break}}return d};
ks.prototype.XA=function(a,b,c){var d=(b?"c-":"")+a,e=this.s3.get(d);if(c&&!e){e=[];this.s3.J(d,e)}return e};
ks.prototype.dJ=function(a){var b=new N(a),c=b.ul()||"/";if(!Kc(this.Rz,c)){var d=new ns(this);this.Rz[c]=d;this.j.g(d,[Ki,Mi],this.Il)}return this.Rz[c]};
ks.prototype.Il=function(a){var b=a.type,c=a.target.UJ();r(c,function(d){Nc(this.Ni,d);this.$G(d,b)},
this)};
ks.prototype.$G=function(a,b){var c=b==Mi?0:os,d=a.callback;if(c==os)this.L8(a.baseUrl,a.imgMax,a.crop);else if(a.errorCallback)d=a.errorCallback;var e,f=new ps(c,a.baseUrl,a.imgMax,a.crop);if(a.img){var g=cs(a.baseUrl,a.imgMax,a.crop);if(a.img.src!=g){if(d)e=$f(a.img,"load",Ia(d,f));a.img.src=g}}if(d&&!e)d(f)};
ks.prototype.L8=function(a,b,c){var d=this.XA(a,!(!c),true);zc(d,b)};
var qs=function(){if(!ls)ls=new ks;return ls},
rs=function(a,b,c,d,e,f,g){var h=qs(),i=c instanceof ne?Math.max(c.width,c.height):c;return h.ua(a,b,i,d,e,f||null,g||null)},
ss=function(a){var b=qs();return b.cf(a)},
ts=function(a,b,c){var d=ls;return d?d.ZY(a,b,c):-1},
ps=function(a,b,c,d){this.status=a;this.baseUrl=b;this.imgMax=c;this.crop=d},
os;ps.prototype.EZ=function(){return cs(this.baseUrl,this.imgMax,this.crop)};
var ms=function(a,b,c,d,e,f,g){this.key=a;this.baseUrl=b;this.imgMax=c;this.crop=d;this.callback=e;this.errorCallback=f;this.img=g},
us=function(a){this.src=a;this.Be=[];this.hO={}};
us.prototype.US=function(a,b){this.hO[a]=b;this.Be.push(b)};
us.prototype.A6=function(a){var b=Rc(this.hO,a);if(b){Nc(this.hO,b);tc(this.Be,b)}};
us.prototype.A1=function(){return this.Be.length>0};
us.prototype.UJ=function(){return this.Be};
var ns=function(){this.eC=[];this.Dd=[];this.Cw={};this.j=new K(this)};
o(ns,J);ns.prototype.b=function(){if(!this.W()){ns.f.b.call(this);r(this.eC,function(a){a.b()});
this.j.b()}};
ns.prototype.ua=function(a){var b=a.key,c=cs(a.baseUrl,a.imgMax,a.crop),d=Rc(this.Cw,c);if(!d){d=new us(c);this.Cw[c]=d;this.Dd.push(d)}d.US(b,a);this.cM()};
ns.prototype.remove=function(a){var b=a.key,c=cs(a.baseUrl,a.imgMax,a.crop),d=Rc(this.Cw,c);if(d){d.A6(b);if(!d.A1()){tc(this.Dd,d);Nc(this.Cw,c)}}};
ns.prototype.cM=function(){if(this.Dd.length){var a=this.dY();if(a){var b=this.Dd.pop();Nc(this.Cw,b.src);a.ua(b)}}};
ns.prototype.dY=function(){var a=lc(this.eC,function(b){return!b.TY()});
if(!a&&this.eC.length<4){a=new vs(this);this.eC.push(a);this.j.g(a,[Ki,Mi],this.Fu)}return a};
ns.prototype.Fu=function(){this.cM()};
var vs=function(a){J.call(this);this.tx(a);this.j=new K(this);this.Uda=new Ym(this.S6,100,this)};
o(vs,J);vs.prototype.Yd=false;vs.prototype.b=function(){if(!this.W()){vs.f.b.call(this);this.j.b();if(this.I){A(this.I);this.I=null}}};
vs.prototype.ua=function(a){this.Fo();this.Yd=true;this.p5=a;var b=a.UJ(),c=lc(b,function(f){return f.img}),
d=c?c.img:this.KY(),e=a.src;if(d.src==e&&d.complete){this.Fo();this.dispatchEvent(new D(Ki,a))}else{this.j.g(d,["load","abort",Mi],this.Il);d.src=e}};
vs.prototype.TY=function(){return this.Yd};
vs.prototype.KY=function(){if(!this.I){var a=w("img");this.I=a;L(a,"position","absolute");L(a,"visibility","hidden");zh(a,1,1);sh(a,10,-10);x(document.body,a)}return this.I};
vs.prototype.Fo=function(){this.Yd=false;this.p5=null;this.j.ma();if(this.I)this.Uda.start()};
vs.prototype.S6=function(){if(this.I&&!this.Yd)this.I.src="img/t.gif"};
vs.prototype.Il=function(a){var b=a.type=="load"?Ki:Mi,c=this.p5;this.Fo();this.dispatchEvent(new D(b,c))};var ws={name:""},xs=function(a){return a},
ys=typeof j._features!="undefined"&&j._features.googlephotos&&!j._features.default144thumbnails?[72,94,110,128,160,220,288]:[72,94,110,144,160,220,288],zs=ys[3],As=ys.length-1,Bs="deletefailed";var Cs="itemsadded",Ds="noitems",Es="totalchanged",Fs="too_many_results_requested",Gs="query_string_too_long";var Hs=function(a){if(za(a))this.ea=new N(a);else if(a instanceof N)this.ea=a;else throw Error("Undefined object: "+a);this.ea.fa("kind","photo");this.ea.fa("alt","jsonm");this.li=null};
Hs.prototype.o8=function(a){this.Je=a};
Hs.prototype.url=function(){return this.ea.toString()};
Hs.prototype.z8=function(a){this.ea.fa("hl",a)};
Hs.prototype.Bx=function(a){this.ea.fa("subjectid",a)};
Hs.prototype.dc=function(){return this.ea.Ga("subjectid")};
Hs.prototype.Lq=function(){return false};
Hs.prototype.NO=function(a,b){this.ea.fa("start-index",a);this.ea.fa("max-results",b)};
Hs.prototype.t9=function(a){this.ea.Wr(this.ea.Nj().replace(/(\/user\/).*(\/|$)/,"$1"+a+"$2"))};
Hs.prototype.th=function(a,b){return new this.Je(a,b)};
Hs.prototype.lq=function(){var a={},b=this.dc();if(b)a.subjectid=b;return a};
Hs.prototype.Cg=function(){return this.li};var Is=function(a,b){this.wa=a;this.r=b;this.HL=0;this.bD=b.entry};
Is.prototype.next=function(){if(this.HL>=this.bD.length)return null;var a=this.bD[this.HL];++this.HL;return this.Uk(a)};
Is.prototype.Uk=function(){return null};
Is.prototype.SY=function(){return this.bD?this.bD.length:0};var Js=function(){},
Ks=function(a,b,c){this.vea=a;this.Sv=b;this.Fca=c};
Ks.prototype.nf=function(){return this.vea};
Ks.prototype.Sb=function(){return this.Sv};
Ks.prototype.Uf=function(){return this.Fca};var Ls=function(){Js.call(this)};
o(Ls,Js);Ls.prototype.send=function(a,b,c,d,e){if(e&&e!="GET")throw Error("Only GET supported in JSONP transport");var f=new ej(a);f.c9(30000);var g=l(this.Xj,this,b);f.send(null,g,d||c)};
Ls.prototype.Xj=function(a,b){a(new Ks(0,"",b))};var Ms=function(){Js.call(this)};
o(Ms,Js);var Ns={"Content-Type":"application/atom+xml","X-HTTP-Method-Override":"PUT"},Os={"Content-Type":"application/atom+xml","X-HTTP-Method-Override":"DELETE"};Ms.prototype.send=function(a,b,c,d,e,f,g){var h=new Dj;F(h,Ki,l(this.Xj,this,b,c));h.Yr(30000);if(d)F(h,Ni,d);if(c)F(h,Mi,c);if(!g&&e=="POST")g=Ns;F(h,"ready",h.b,false,h);h.send(a,e,f,g)};
Ms.prototype.Xj=function(a,b,c){var d=c.target;G(d,l(this.Xj,this));var e,f=d.nf();if(f<0)return;if(f>=400)if(b){b(c);return}else e=new Ks(f,d.Sb(),"");else try{var g=d.Sb(),h=g?yj(g):null;e=new Ks(0,"",h)}catch(i){if(b){b(i);return}}a(e)};var Qs=function(a,b){this.Jda=za(a)?Ia(Ps,a):a;this.JN=new dd;this.ae(b)},
Ps=function(a,b){if(Rs==b)return;return Rc(b,a)};
Qs.prototype.ae=function(a){if(this.Q&&this.Q!=a)this.Q.KD(null);this.Q=a;if(!this.Q)return;this.Q.KD(l(this.mI,this));if(this.Q.isLoaded())r(this.Q.vA(),this.mI,this)};
Qs.prototype.mI=function(a,b){var c=this.Jda(a);if(c){var d=this.JN.get(c);if(!d){d=[];this.JN.J(c,d)}d.push(b)}};
Qs.prototype.nA=function(a,b,c,d){var e=this.JN.get(a);if(c||e&&!d)b(e||[]);else if(this.Q.isLoaded()||d)this.Q.j3(l(this.nA,this,a,b,true,false));else this.Q.ua(0,this.Q.Yc(),l(this.nA,this,a,b,c,d))};var V=function(a){J.call(this);this.wa=a};
o(V,J);V.prototype.Hr=0;V.prototype.id="-1";V.prototype.vI="";V.prototype.Q=null;V.prototype.fL="";V.prototype.mi="";V.prototype.BB=true;V.prototype.isOwner=function(a){if(!a)return false;var b;if(this.$a&&this.$a.user){if(typeof a=="string")b=a;else if(a.name)b=a.name;return this.$a.user==b}return false};
V.prototype.lf=function(){if(!this.fc){var a,b="";if(this.$a&&this.$a.user){var c=this.$a.user;if(typeof c=="string")a=c;else if(c.name){a=c.name;b=c.nickname||""}this.fc=this.wa.Ne(a,b)}}return this.fc};
V.prototype.Jm=function(){};
V.prototype.Hf=function(a){this.$q(a)};
V.prototype.Gk=function(a){this.$q(a)};
V.prototype.$q=function(a){if(!this.$a)this.$a=a;else Uc(this.$a,a);this.BB=true};
V.prototype.Vn=function(){if(this.BB&&this.$a){this.BB=false;var a=this.$a.link;if(a)for(var b=0;b<a.length;++b){var c=a[b],d=c.href||"";switch(c.rel){case "http://schemas.google.com/g/2005#feed":this.vI=d;break;case "alternate":this.fL=d;break;case "self":this.mi=d;break;case "edit":this.Rba=d;break}}}};
V.prototype.bc=function(){return this.Ce};
V.prototype.Td=function(){return this.$a?this.$a.title:""+this};
V.prototype.lJ=function(){this.Vn();return this.vI};
V.prototype.vl=function(){this.Vn();return this.mi};
V.prototype.lu=function(){this.Vn();return this.fL};
V.prototype.gJ=function(){this.Vn();return this.Rba};
V.prototype.Wa=function(a){if(!this.Q)this.q2(a);return this.Q};
V.prototype.q2=function(a){this.Q=this.wa.zY(this,a)};
V.prototype.Rj=function(){return this.Q?this.Q.Rj():this.wa.aJ()};
V.prototype.H=function(){return this.id||"-1"};
V.prototype.thumbnail=function(a){var b="img/picasalogo.gif",c=new ne(27,25),d=this.fq();if(d){var e=Math.min(a||0,ys.length),f=ys[e],g=new ne(f,f);b=d.url||b;if("width"in d&&"height"in d){c.width=d.width;c.height=d.height;var h=gs(c,g);if(!oe(c,h)){b=cs(b,h);c=h}}else{c=is(g,g);b=cs(b,c,true)}}return{url:b,height:String(c.height),width:String(c.width)}};
V.prototype.naa=function(a){if(!this.Q)this.Q=a};
V.prototype.ua=function(a){if(this.isLoaded())a(this);else this.Tq(a)};
V.prototype.reload=function(a){this.Tq(a)};
V.prototype.Tq=function(a){var b=this.Rj();b.send(this.vl(),l(function(c){if(c.status==404){if(typeof this.Mo!="undefined")this.Mo();a(this);return}var d=c.Uf();this.Gk(d.entry);a(this)},
this))};
V.prototype.S=function(){if(this.$a)return Number(this.$a.numPhotos);return 0};
V.prototype.isLoaded=function(){return!(!this.$a)};
V.prototype.Gl=function(a){var b=this.thumbnail(a);return b.url};
V.prototype.fq=function(){if(this.$a){var a=this.$a.media,b=a?a.content:null;if(b&&b.length)return b[0]}return null};
V.prototype.Rf=function(){var a=this.fq();if(a)return this.gw(a.url);return null};
V.prototype.Jn=function(){var a,b=this.thumbnail();if(b)a=b.width/b.height;return a||0};
V.prototype.yt=function(){this.wa.yt(this)};
V.prototype.XV=function(a,b,c){this.G5(a,b,c)};
V.prototype.D5=function(a,b,c,d){this.RC(this.gJ(),Ns,a,"POST",b,c,d)};
V.prototype.G5=function(a,b,c){this.RC(this.gJ(),Os,"","POST",a,b,c)};
V.prototype.RC=function(a,b,c,d,e,f,g){if(this.Hr){++this.Hr;if(this.Hr>10){if(g)g("Timed out attempting to post");return}Rg(l(function(){this.RC(a,b,c,d,e,f,g)},
this),50);return}this.Hr=1;var h=l(function(i){this.E5(i,e,f)},
this);this.Rj().send(a,h,f?l(function(){f(this)},
this):null,g,d,c,b)};
V.prototype.E5=function(a,b,c){this.Hr=0;if(a.nf()>=400){if(c)c(this)}else{var d=a.Uf();if(d&&"entry"in d)this.Gk(d.entry);if(b)b(this)}};
var Ss=function(a){var b,c;if(a){if("lat"in a&&"lon"in a){b=a.lat;c=a.lon}var d=a.georss$where;if(d){var e=d.gml$Point;if(e){var f=e.gml$pos;if(f){var g=f.split(" ");if(g.length==2){b=g[0];c=g[1]}}}}}var h=Number(b),i=Number(c),k=null;if(!isNaN(h+i))k={lat:h,lon:i};return k};
V.prototype.gw=function(a){return a.replace(/%2D/gi,"-").replace(/%2E/gi,".").replace(/%5F/gi,"_").replace(/%7E/gi,"~")};var Ts=function(a,b,c,d){V.call(this,a);this.Hf(b);this.user=c;this.album=d};
o(Ts,V);Ts.prototype.type="Tag";Ts.prototype.toString=function(){var a="Tag "+this.t;if(this.user)a+=", user: "+this.user;if(this.album)a+=", album: "+this.album;return a};
var Us=function(a){var b=hc(a,function(d,e){d.push(p(e.t));return d},
[]).join(","),c='<entry xmlns="http://www.w3.org/2005/Atom" xmlns:media="http://search.yahoo.com/mrss/"><category scheme="http://schemas.google.com/g/2005#kind" term="http://schemas.google.com/photos/2007#photo" /><media:group><media:keywords>'+b+"</media:keywords></media:group></entry>";return c};
Ts.prototype.Hf=function(a){this.t=a.title;this.w=a.weight?a.weight:1};
var Vs=function(a){var b=a.split(","),c=[];if(b)for(var d=0;d<b.length;d++){var e=Va(b[d]);if(e.length>0)c.push(e)}return c},
Ws=function(a){a=Ua(a);var b=a.match(/[^,'" ][^, ]*|\"[^"]*(?:\"|$)|\'[^']*(?:\'|$)/g),c={},d=[];if(b)for(var e=0;e<b.length;e++){var f=nb(nb(Ua(b[e]),"'"),'"');if(f.length&&f!="'"&&f!='"'&&!c[f]){d.push(f);c[f]=1}}return d};var Xs=function(a,b,c,d){V.call(this,a);this.deleted=false;this.src="../img/picasaweblogo-en_US.gif";this.user=c;this.album=d;if(b){var e=b,f=e.media,g=f.content[0];this.s=this.gw(g.url);this.id=e.gphoto$id;this.w=Number(g.width);this.h=Number(g.height);this.Sg(p(f.description));this.t=p(f.title);this.snippet=e.snippet||"";this.snippetType=e.snippettype||"";this.truncated=e.truncated=="1";var h=e.author;this.Hw(e);if(!this.user&&h){var i=h[0],k=i.email,n=i.name||k;this.user=this.wa.Ne(k,n)}if(!this.album&&
this.user)this.album=this.wa.gd(this.user.name,e.albumId,e.albumTitle,e.albumCTitle);var q=f.content[1];if(q){this.vsurl=q.url;this.videostatus=e.videostatus}this.Mg=true}};
o(Xs,V);Xs.prototype.rD=false;Xs.prototype.sA=false;Xs.prototype.type="Photo";var Ys=Fd("gphoto.api.Photo");Xs.prototype.V=function(a){Ys.Zb(a)};
Xs.prototype.Hp=false;Xs.prototype.aW=false;var Zs="photoDataReady";Xs.prototype.r=null;Xs.prototype.ph=null;Xs.prototype.isOwner=function(a){if(!a)return false;var b;if(typeof a=="string")b=a;else if(a.name)b=a.name;return this.user&&this.user.name==b};
Xs.prototype.lf=function(){if(!this.fc)this.fc=this.user instanceof $s?this.user:Xs.f.lf.call(this);return this.fc};
Xs.prototype.gd=function(){return this.album};
Xs.prototype.toString=function(){return"Photo "+this.t+", in album: "+this.album+", user: "+this.user};
Xs.prototype.Wa=function(){return null};
Xs.prototype.Ei=function(a){if(this.r&&this.ph)a(this);else{$f(this,Zs,function(){a(this)},
false,this);this.Gv()}};
Xs.prototype.vl=function(){this.Vn();if(!this.mi)this.mi=Pa("/data/entry/api/user/%s/albumid/%s/photoid/%s?alt=jsonm",this.user.name,this.album.id,this.id);return this.mi};
Xs.prototype.Gv=function(){if(this.rD||this.sA)return;if(this.r&&this.ph)this.dispatchEvent(Zs);else{if(!this.user.name){this.Mg=true;this.r=null;this.Mo();return}var a="/lh/rssPhoto?uname="+this.user.name+"&iid="+this.id+"&v="+Ka()+"&js=1";this.rD=true;this.sA=true;var b=this.Rj();b.send(a,l(this.i7,this));b.send(this.vl(),l(this.PW,this))}};
Xs.prototype.i7=function(a){if(this.r)return;try{this.rD=false;var b=a.nf();if(b>=400){this.Mo();return}this.r=a.Uf();this.Hw(this.r);if(this.r.allowprints)this.gd().prints=this.r.allowprints=="1";if(this.r.video){this.vsurl=this.r.video.streamer_url;this.vdurl=this.r.video.download_url}this.dl=ws.name==this.r.uname||this.r.allowdownload=="1"}catch(a){alert(a);return}if(this.ph&&this.r)this.dispatchEvent(Zs)};
Xs.prototype.Gb=function(){return!(!this.vsurl)};
var at=function(a,b){var c="";if(a=="ready")c="img/experimental/video_ready"+b+".png";else if(a=="pending")c="img/experimental/video_pending"+b+".png";else if(a=="failed")c="img/experimental/video_failed"+b+".png";if(c){var d=document.getElementsByTagName("base")[0].href;c=d+c}return c};
Xs.prototype.thumbnail=function(a){var b=at(this.videostatus,"");if(!b)return Xs.f.thumbnail.call(this,a);var c=new ne(288,216);return{url:b,height:String(c.height),width:String(c.width)}};
Xs.prototype.PW=function(a){try{this.sA=false;var b=a.nf();if(b>=400){this.Mo();return}this.ph=a.Uf().entry;this.Gk(this.ph)}catch(a){alert(a);return}if(this.ph&&this.r)this.dispatchEvent(Zs)};
Xs.prototype.Gk=function(a){V.prototype.Gk.call(this,a);var b=a.media,c=b.content[0];this.s=this.gw(c.url);this.id=a.gphoto$id;this.w=Number(c.width);this.h=Number(c.height);this.Sg(p(b.description));this.t=p(b.title);if(this.$a)this.$a.media.keywords=b.keywords;this.snippet=a.snippet||"";this.snippetType=a.snippettype||"";this.truncated=a.truncated=="1";var d=b.content[1];if(d){this.vsurl=d.url;this.videostatus=a.videostatus}this.Hw(a);this.Mg=true};
Xs.prototype.Hf=function(a){if(!a)return;if(!this.$a)this.$a=a;if(!this.user){var b=a.author[0],c=b.email,d=b.name||c;this.user=this.wa.Ne(c,d)}var e=a.media,f=e.content[0];this.s=this.gw(f.url);this.id=a.gphoto$id;this.w=Number(f.width);this.h=Number(f.height);this.Sg(p(e.description));this.t=p(e.title);this.snippet=a.snippet||"";this.snippetType=a.snippettype||"";this.truncated=a.truncated=="1";this.Hw(a);if(!this.album)this.album=this.wa.gd(this.user.name,a.albumId,a.albumTitle,a.albumCTitle);
var g=e.content[1];if(g){this.vsurl=ab(g.url);this.videostatus=a.videostatus}this.Mg=true;Xs.f.Hf.call(this,a);this.snippet=a.snippet||this.snippet;this.snippetType=a.snippettype||this.snippetType;if(!this.truncated&&a.truncated=="1")this.truncated=true;if("author"in a){var h=a.author;if(!this.user.name)this.user.name=h[0].email;if(!this.user.nickname)this.user.nickname=h.name}if(!this.user.name)this.Mo()};
Xs.prototype.DL=function(a){return this.truncated&&a.bc().Lq()};
Xs.prototype.Mo=function(){this.aW=true;this.dispatchEvent("deleted")};
Xs.prototype.Jn=function(){if(Ba(this.w)&&Ba(this.h))return this.w/this.h;return 0};
Xs.prototype.Rf=function(a){if(this.s&&a)this.s.replace(/\/s\d{2,3}(-c)?\//,"/"+a+"/");return this.s};
Xs.prototype.On=function(){if(!this.Dc){var a,b;if(this.$a)b=Rc(this.$a,"media");else if(this.ph)b=Rc(this.ph,"media");if(pa(b))a=Rc(b,"keywords");this.Dc=[];if(pa(a)){var c=Vs(a);for(var d=0;d<c.length;++d){var e=c[d];if(!e)continue;var f=this.wa.ZA({title:e},this.user,this.album);this.Dc.push(f)}}}return this.Dc};
Xs.prototype.B6=function(a,b){if(this.Dc)tc(this.Dc,a);if(b)this.SC()};
Xs.prototype.PS=function(a,b,c){if(!this.Dc)this.On();var d=this.wa.ZA({title:a},b);this.Dc.push(d);if(c)this.SC();return d};
Xs.prototype.SC=function(a,b,c){var d=Us(this.On());this.D5(d,a,b,c)};
Xs.prototype.Sg=function(a){this.c=ob(a,1024)};
Xs.prototype.Hw=function(a){var b=Ss(a);if(b){if(isNaN(this.lat))this.lat=b.lat;if(isNaN(this.lon))this.lon=b.lon}this.bb=this.bb||a.bb};
Xs.prototype.NA=function(){return this.r.cc};
Xs.prototype.eP=function(a){this.r.cc=a};
Xs.prototype.zL=function(){return this.r.ccOverride};
Xs.prototype.fP=function(a){this.r.ccOverride=a};var bt=function(a,b,c,d){Is.call(this,a,b);this.Kk=c;if(!this.Kk)if(b.user)this.Kk=a.Ne(b.user,b.nickname);this.Da=d};
o(bt,Is);bt.prototype.Uk=function(a){var b=this.wa.Sf(a,this.Kk,this.Da);b.Hf(a);return b};var ct=function(a,b,c){Is.call(this,a,b);this.Kk=c};
o(ct,Is);ct.prototype.Uk=function(a){var b=this.wa.gd(this.Kk.name,a.gphoto$id,a.title,a.gphoto$name);b.Hf(a);return b};var dt=function(a,b,c,d){Is.call(this,a,b);this.Kk=c;this.Da=d};
o(dt,Is);dt.prototype.Uk=function(a){return this.wa.ZA(a,this.Kk,this.Da)};var et=function(a,b){Hs.call(this,a);this.us=b;this.ea.fa("kind","album");var c=this.ea.Nj().match(/\/user\/(.*)$/);if(c&&c.length>1){var d=c[1];if(d!=this.us){this.t9(this.us);var e=this.ea.El();if(e.Ob("authkey")){e.remove("authkey");e.remove("access")}}}this.Je=ct};
o(et,Hs);et.prototype.th=function(a,b){if(this.Je===ct){if(!this.li){var c=a.Ne(b.user,b.nickname);c.Jm(b);this.li=c}return new this.Je(a,b,this.li)}else return et.f.th.call(this,a,b)};var ft=function(a){Hs.call(this,a);this.Je=bt};
o(ft,Hs);ft.prototype.Kk=null;ft.prototype.Da=null;ft.prototype.Je=bt;ft.prototype.WX=function(a,b){var c=a.gd(b.user,b.gphoto$id,b.title,"");c.Jm(b);return c};
ft.prototype.th=function(a,b){if(!this.li)this.li=this.WX(a,b);else this.li.Gk(b);if(this.Je==bt){var c=a.Ne(b.user,b.nickname);return new this.Je(a,b,c,this.li)}else return ft.f.th.call(this,a,b)};var gt=function(a,b,c,d,e){V.call(this,a);this.username=b;this.id=c;this.title=d;this.name=d;this.ctitle=e;this.link="/lh/sredir?uname="+this.username+"&target=ALBUM&id="+this.id;this.prints=false};
o(gt,V);gt.prototype.type="Album";gt.prototype.J8=function(a){if(a)this.link=a};
gt.prototype.G8=function(a){this.kmlLink=a};
gt.prototype.M8=function(a){this.mapLink=a};
gt.prototype.q9=function(a){this.uploadlink=a};
gt.prototype.H8=function(a){this.largepreview=a};
gt.prototype.fq=function(){if(this.$a&&this.$a.media)return gt.f.fq.call(this);var a="img/picasalogo.gif";if(this.largepreview)a=as(this.largepreview);return{url:a}};
gt.prototype.isOwner=function(a){var b;if(typeof a=="string")b=a;else if(a.name)b=a.name;return this.username==a};
gt.prototype.lf=function(){if(!this.fc)this.fc=this.username&&!this.$a?this.wa.Ne(this.username,""):gt.f.lf.call(this);return this.fc};
gt.prototype.toString=function(){return"[Album: "+this.title+", user: "+this.username+"]"};
gt.prototype.vl=function(){this.Vn();if(!this.mi)this.mi=Pa("/data/entry/api/user/%s/albumid/%s?alt=jsonm",this.username,this.id);return this.mi};
gt.prototype.Jm=function(a){if(!this.$a)this.Hf(a)};
gt.prototype.$q=function(a){gt.f.$q.call(this,a);this.link=this.lu()||this.link;this.title=this.title||this.$a.title;this.ctitle=this.ctitle||this.$a.gphoto$name;this.name=this.name||this.ctitle};
gt.prototype.bc=function(){if(!this.Ce)this.Ce=new ft(this.lJ());return this.Ce};
gt.prototype.S=function(){if(this.$a)return Number(this.$a.totalResults||this.$a.numPhotos);return 0};var $s=function(a,b,c){V.call(this,a);this.name=b;this.nickname=c;this.icon="img/noportrait.jpg";this.galleryName=xs(this.nickname);this.loaded=false;this.OH=false;this.email=""};
o($s,V);$s.prototype.type="User";var ht="userDataReady";$s.prototype.bc=function(){if(!this.Ce)this.Ce=new et(this.lJ(),this.name);return this.Ce};
$s.prototype.Jm=function(a){$s.f.Jm.call(this,a);this.icon=a.icon;this.galleryName=a.title};
$s.prototype.isOwner=function(a){var b;if(typeof a=="string")b=a;else if(a.name)b=a.name;return this.name==b};
$s.prototype.lf=function(){return this};
$s.prototype.toString=function(){return"User "+this.nickname+", username: "+this.name};
$s.prototype.ua=function(a){this.Ei(a)};
$s.prototype.Ei=function(a){if(this.OH)a(this);else{F(this,ht,function(){a(this)},
true,this);this.Gv()}};
$s.prototype.Gv=function(){if(this.OH){this.loaded=true;this.dispatchEvent({type:ht});return}var a=this.vl()||"/data/entry/api/user/"+this.name+"?alt=jsonm",b=this.Rj();b.send(a,l($s.prototype.yq,this))};
$s.prototype.Hf=function(a){$s.f.Hf.call(this,a);this.icon=a.thumbnail||this.icon;this.galleryName=a.title;this.name=a.user;this.nickname=a.nickname};
$s.prototype.yq=function(a){var b=a.Uf();if(b){this.OH=true;this.loaded=true;this.Hf(b.entry);this.dispatchEvent({type:ht})}};var it=function(a){if(za(a))this.ea=new N(a);else if(a instanceof N)this.ea=a;this.ea.fa("alt","jsonm");this.ea.fa("pp","1");this.Je=dt};
o(it,Hs);it.prototype.toString=function(){return"TagSource: "+this.ea.Nj()};
it.prototype.Je=dt;it.prototype.th=function(a,b){if(this.Je==dt){var c=a.Ne(b.user,b.nickname),d=null;return new this.Je(a,b,c,d)}else return it.f.th.call(this,a,b)};var jt=function(a){Hs.call(this,a);this.Je=bt};
o(jt,Hs);jt.prototype.uP=function(a,b){if(a)this.ea.fa("q",a);else this.ea.El().remove("q");if(pa(b))this.Qr(b)};
jt.prototype.ti=function(){return this.ea.Ga("q")};
jt.prototype.SO=function(a){var b=this.TI();if(b!=a){b=a||"G";this.ea.fa("psc",b);if(b=="G")this.ea.fa("access","public")}};
jt.prototype.TO=function(a){if(!a)this.ea.El().remove("uname");else this.ea.fa("uname",a)};
jt.prototype.TI=function(){return this.ea.Ga("psc")||"G"};
jt.prototype.t1=function(){return!(!this.ea.Ga("psc"))};
jt.prototype.Qr=function(a){var b=this.Lq();if(b!=a){b=a;if(b)this.ea.fa("filter","1");else this.ea.El().remove("filter")}};
jt.prototype.Lq=function(){return this.ea.Ga("filter")=="1"};
jt.prototype.lq=function(){var a=jt.f.lq.call(this);if(this.t1())a.psc=this.TI();var b=this.ti(),c=this.On();if(b)a.q=b;else if(c)a.tags=c;return a};
jt.prototype.On=function(){return this.ea.Ga("tag")};var kt=function(a,b,c,d,e){this.Q=a;this.yp=b;this.oa=c;this.Ma=d;this.Ej=e;this.Sfa=this.yp.length;this.Q.addEventListener(Cs,l(this.ua,this));for(var f=0;f<this.yp.length;++f)this.Q.dC(this.yp[f])};
kt.prototype.rz=false;kt.prototype.Kq=function(){return this.rz};
kt.prototype.ua=function(){if(this.rz)return true;var a=[];for(var b=0;b<this.yp.length;++b){var c=this.yp[b];if(!this.Q.Zl[c])a.push(c)}if(a.length==0){this.rz=true;this.Q.removeEventListener(Cs,this.ua.bind,false,this);this.Ej(this.Q,this.oa,this.Ma);return true}else this.yp=a;return false};var lt=function(a,b,c,d){this.wa=a;this.Ce=b;this.qd=c||a.aJ();this.p=new K(this);this.p.g(this.wa,"deleteinitiated",this.x_);this.p.g(this.wa,"deletesucceeded",this.y_);this.p.g(this.wa,Bs,this.w_);this.ta=[];this.Zl=[];this.jk=new dd;lt.prototype.zb=Ba(d)?d:24;this.P6()};
o(lt,J);var Rs="PENDING";lt.prototype.Xe=null;lt.prototype.Vk=0;lt.prototype.nN=false;lt.prototype.toString=function(){return"Feed: "+this.Ce};
lt.prototype.KD=function(a){lt.prototype.nI=a};
lt.prototype.P6=function(){this.reset()};
lt.prototype.bc=function(){return this.Ce};
lt.prototype.reset=function(){this.Xe=null;this.Vk=0;this.ta=[];this.Zl=[]};
lt.prototype.b=function(){lt.f.b.call(this);this.reset()};
lt.prototype.Y8=function(a){this.nN=a};
lt.prototype.Wi=function(a){this.Efa=true;this.zb=a};
lt.prototype.Yc=function(){return this.zb};
lt.prototype.Rj=function(){return this.qd};
lt.prototype.DN=function(a,b,c){if(!b)return;this.IN(a,b);var d=this.Ws(a);this.Zl[d]=c};
lt.prototype.iX=function(a){return cc(this.ta,a)};
lt.prototype.SK=function(a){return!(!this.Me(a))};
lt.prototype.Me=function(a){if(a<this.ta.length)return this.ta[a];return null};
lt.prototype.vA=function(){return this.ta};
lt.prototype.Ws=function(a){return Math.floor(a/this.zb)};
lt.prototype.Sj=function(a){var b=this.Ws(a),c=true;if(!this.Zl[b]){this.dC(b);c=false}var d=a/this.zb,e=d-b;if(e<0.1)b=Math.max(0,b-1);if(this.isLoaded()){if(e>0.9)b=Math.min(this.Zl.length-1,b+1);if(!this.Zl[b])Rg(l(this.dC,this,b))}return c};
lt.prototype.dC=function(a){this.Zl[a]=true;var b=a*this.zb,c=b+1;if(c>1000)return;var d=Math.min(this.zb,1000-c+1);if(d<=0)return;if(this.nN)for(var e=b;e<b+d;++e)this.ta[e]=Rs;this.Ce.NO(c,d);this.qd.send(this.Ce.url(),l(lt.prototype.XC,this,a*this.zb,this.zb),l(this.wi,this),l(this.jB,this))};
lt.prototype.jB=function(a){if(a){var b=a.target;if(b)G(b,Ni,l(this.jB,this))}var c={type:"error",status:"timeout",value:a?a.currentTarget.cK():""};this.dispatchEvent(c)};
lt.prototype.wi=function(a){var b;if(a){b=a.target;G(b,Mi,l(this.wi,this))}var c,d;if(!b)c="unknown";else{c=b.Sb();var e=new N(b.QY());if(Fs==c)d=e.Ga("start-index");else if(Gs==c){var f=e.Ga("q");d=f?f.length:-1}}var g={type:"error",status:c,value:""+d};this.dispatchEvent(g)};
lt.prototype.XC=function(a,b,c){if(c.nf()==500){var d=c.Sb(),e="";if(Fs==d)e=""+a;else if(Gs==d){var f=this.Ce.ti();e=""+f.length}this.dispatchEvent({type:"error",status:d,value:e});return}this.IN(a,c.Uf())};
lt.prototype.IN=function(a,b){if(b){var c=this.Xe,d=b.feed;this.Xe=Number(d.totalResults);this.Vk=Math.max(this.Vk,Number(d.crowdedLength||this.Xe));if(0==this.Xe)this.dispatchEvent(Ds);this.MS(a,d);if(c!=this.Xe)this.dispatchEvent(Es)}};
lt.prototype.MS=function(a,b){if(!b)return;var c=this.Ce.th(this.wa,b),d=Math.min(a+c.SY(),this.Xe),e=d-a;this.ta.length=Math.max(this.ta.length,d);for(var f=0;f<e;++f){var g=a+f,h=c.next();this.Ms(g,h)}Math.max(this.Vk,this.ta.length);var i=this.Ce.Cg();if(i)i.naa(this);this.dispatchEvent({type:Cs,offset:a,count:e,crowdedCount:this.Yb()})};
lt.prototype.Ms=function(a,b){if(this.ta[a]&&this.ta[a]!=Rs)return;if(this.nI)this.nI(b,a);this.ta[a]=b};
lt.prototype.ua=function(a,b,c){var d=this.Ws(a),e=this.Ws(a+b-1),f=[];for(var g=d;g<=e;++g)if(!this.Zl[g])f.push(g);if(f.length==0){c(this,a,b);return true}else{var h=new kt(this,f,a,b,c);return h.Kq()}return false};
lt.prototype.j3=function(a){var b=1000;if(this.isLoaded()){b=Math.min(this.Xe,b);if(this.ta.length==b){a(this,0,b);return}}this.Ce.NO(1,b);var c=l(function(d){this.XC(0,b,d);a(this,0,this.Xe)},
this);this.qd.send(this.Ce.url(),c,l(this.wi,this),l(this.jB,this))};
lt.prototype.isLoaded=function(){return null!=this.Xe};
lt.prototype.Yb=function(){return Math.min(this.Vk||this.Xe,1000)};
lt.prototype.ms=function(){return this.Xe||0};
lt.prototype.x_=function(a){var b=a.target,c=this.iX(b);if(c>=0)if(this.remove(c))this.jk.J(b.id,[b,c])};
lt.prototype.remove=function(a){var b=false;if(sc(this.ta,a)){b=true;--this.Vk}--this.Xe;this.dispatchEvent(Es);return b};
lt.prototype.y_=function(a){var b=a.target;this.jk.remove(b.id)};
lt.prototype.w_=function(a){var b=a.target,c=this.jk.get(b.id);if(c){rc(this.ta,c[0],c[1]);this.jk.remove(b.id);++this.Vk}++this.Xe;this.dispatchEvent(Es)};var mt=function(a){this.ad=new dd;this.Ld=new dd;this.$Q=new dd;this.Dc=new dd;this.YG={};this.YG.Album=this.Ld;this.YG.Photo=this.ad;this.Kba=a||new Ms;this.jk=new dd};
o(mt,J);mt.prototype.reset=function(){this.ad.clear();this.Ld.clear();this.$Q.clear()};
mt.prototype.aJ=function(){return this.Kba};
mt.prototype.YX=function(a,b,c,d,e){var f=this.gd(b,c,d||"",e||"");a=nt(a);var g=new ft(a);if(!g.li)g.li=f;return g};
mt.prototype.VA=function(a,b,c,d,e){var f=nt(e||"/data/feed/api/all"),g=new jt(f);g.uP(b||"",d);return g};
mt.prototype.ZJ=function(a,b){var c=this.VA(null,a),d=new lt(this,c,b);return d};
mt.prototype.VX=function(a,b,c,d){var e=this.YX(a,b,c.id,c.title||"",c.name||"");return new lt(this,e,d,1000)};
mt.prototype.OJ=function(a,b){a=nt(a);var c=new Hs(a);c.o8(bt);return new lt(this,c,b)};
mt.prototype.zY=function(a,b){var c=nt(a.bc());return new lt(this,c,b||a.Rj())};
mt.prototype.gd=function(a,b,c,d){var e=a+b,f=this.Ld.get(e);if(!f){f=new gt(this,a,b,c,d);this.Ld.J(e,f)}return f};
mt.prototype.UX=function(a,b){var c=a+b;return this.Ld.get(c)};
mt.prototype.Ne=function(a,b){var c=this.$Q.get(a);if(!c){c=new $s(this,a,b);this.$Q.J(a,c)}return c};
mt.prototype.Sf=function(a,b,c){if(!a)return null;var d=a.gphoto$id,e=this.ad.get(d);if(!e){e=new Xs(this,a,b,c);this.ad.J(d,e)}return e};
mt.prototype.MJ=function(a,b,c){var d=this.ad.get(a);if(!d){d=new Xs(this);if(b){d.user=this.Ne(b,"");if(c)d.album=this.gd(b,c,"","")}d.id=a;this.ad.J(a,d)}return d};
mt.prototype.ZA=function(a,b,c){if(!a)return null;var d=a.title,e=this.Dc.get(d);if(!e){e=new Ts(this,a,b,c);this.Dc.J(d,e)}return e};
mt.prototype.yt=function(a){if(!a||!a.id)throw Error("Invalid Entity: "+a);var b=this.YG[a.type];if(!b)throw Error("Can't delete entities of type: "+a?a.type:"null");var c=a.id;if(this.jk.get(c))return;var d=new D("deleterequested",a);if(this.dispatchEvent(d)){this.dispatchEvent(new D("deleteinitiated",a));this.jk.J(c,a);b.remove(c);a.XV(l(this.A_,this),l(this.z_,this));this.dispatchEvent(new D("deleteposted",a))}else{var e=new D("deletecancelled",a);this.dispatchEvent(e)}};
mt.prototype.A_=function(a){this.jk.remove(a.id);var b=new D("deletesucceeded",a);this.dispatchEvent(b)};
mt.prototype.z_=function(a){this.jk.remove(a.id);var b=new D(Bs,a);this.dispatchEvent(b)};
mt.prototype.D=function(){return"gphoto.api.DataManager"};
mt.prototype.V=function(a){if(!this.Ba)this.Ba=Fd(this.D());this.Ba.Zb(a)};
var nt=function(a){if(typeof j._features!="undefined"&&j._features.PERFORMANCE_EXPERIMENTS)a=a.replace("/data/feed/api/","/data/feed/t/");return a};var ot=function(a,b,c){P.call(this);this.tk(a);this.Vi(b);this.U(c||null)};
o(ot,P);ot.prototype.oa=-1;ot.prototype.Aa=false;ot.prototype.Eq=false;ot.prototype.Ea=true;ot.prototype.Eg=function(){return this.P?this.oa:-1};
ot.prototype.Vi=function(a){this.oa=a};
ot.prototype.db="";ot.prototype.m=function(){this.e=w("div");of(this.e,this.db)};
ot.prototype.u=function(){if(this.Eb)A(this.e);ot.f.u.call(this)};
ot.prototype.D=function(){return"goog.ui.icon.AbstractIcon"};
ot.prototype.tk=function(a){this.db=a;if(this.Eb)of(this.e,this.db)};
ot.prototype.wl=function(){return this.Eq};
ot.prototype.Ab=function(a){this.Eq=!(!a)};
ot.prototype.oe=function(){return this.Aa};
ot.prototype.Oc=function(a){this.Aa=!(!a);this.Ab(a)};
ot.prototype.sb=function(){return this.Ea};
ot.prototype.Ca=function(a){if(this.Ea!=a){this.Ea=a;var b=a?wk:vk;this.dispatchEvent(new D(b,this))}};
ot.prototype.U=function(a){ot.f.U.call(this,a);if(this.Eb)this.aa()};
ot.prototype.aa=function(){};
ot.prototype.xj=function(a){if(!a.Vl(0))return;this.dispatchEvent(new pt(this,a))};
ot.prototype.RH=function(a){if(!a.Vl(0))return;this.dispatchEvent(new pt(this,a,true));a.preventDefault()};
ot.prototype.zk=false;ot.prototype.columns=0;ot.prototype.width=0;ot.prototype.height=0;var qt=function(a,b){return a.oa-b.oa},
pt=function(a,b,c){D.call(this,rt,a);this.activated=c||false;this.shiftKey=b.shiftKey;this.ctrlKey=b.ctrlKey;this.mouseEvent=b};
o(pt,D);var st=function(a,b,c,d){ot.call(this,b,c,d)};
o(st,ot);st.prototype.D=function(){return"goog.ui.icon.BasicIcon"};
st.prototype.n=function(){st.f.n.call(this);x(this.e,v("div",{style:"background:url("+this.k.src+");width:"+this.k.width+"px;height:"+this.k.height+"px;"}));x(this.e,v("div",null,"caption"));this.cv()};
st.prototype.cv=function(a,b){F(a||this.e,I,this.xj,false,this);F(b||this.e,I,this.RH,false,this)};
st.prototype.u=function(){if(this.Eb){A(this.e);G(this.e,I,this.xj,false,this)}ot.f.u.call(this)};
st.prototype.Ab=function(a){st.f.Ab.call(this,a);var b=this.db+tt;if(a)C(this.e,b);else qf(this.e,b)};
var tt="-selected";var ut=function(){J.call(this)};
o(ut,J);ut.prototype.S=function(){return 0};
ut.prototype.wb=function(){return null};
ut.prototype.Eo=function(){};
ut.prototype.Bh=function(a,b){this.dispatchEvent(new vt("insert",this,b))};
ut.prototype.remove=function(a){this.dispatchEvent(new vt("remove",this,a))};
ut.prototype.jD=function(a,b){for(var c=0;c<b.length;c++){var d=b[c],e,f;if(Ba(d))e=(f=d);else{e=d.start;f=d.end}for(var g=e;g<=f;g++){var h=this.remove(g);if(g<a)this.Bh(--a,h);else this.Bh(a++,h)}}};
ut.prototype.Hk=function(a,b){this.dispatchEvent(new vt("update",this,a,b))};
var rt="iconselected",vt=function(a,b,c,d){D.call(this,a,b);this.count=b.S();this.index=c;this.endIndex=d||c};
o(vt,D);var wt=function(a){this.xf=a||[]};
o(wt,ut);wt.prototype.S=function(){return this.xf.length};
wt.prototype.wb=function(a){return this.xf[a]};
wt.prototype.Eo=function(a,b,c){if(a>=0&&a<this.xf.length){b=Math.min(b+1,this.xf.length);Rg(Ia(c,a,this.xf.slice(a,b)))}};
wt.prototype.Bh=function(a,b){this.xf.splice(b,0,a);wt.f.Bh.call(this,a,b)};
wt.prototype.F=function(a){this.xf.push(a);wt.f.Bh.call(this,a,this.xf.length-1)};
wt.prototype.remove=function(a){if(a>=0&&a<this.xf.length){var b=this.xf.splice(a,1)[0];wt.f.remove.call(this,a);return b}else return undefined};var xt=function(){Zh.call(this);this.j=new K};
o(xt,Zh);xt.prototype.b=function(){if(!this.W()){xt.f.b.call(this);this.j.b()}};
xt.prototype.Ea=false;xt.prototype.HA=function(){return this.Ea};
xt.prototype.Ca=function(a){this.Ea=!(!a)};
xt.prototype.ub=function(a,b){var c=new yt(a,this,b);this.ta.push(c);this.j.g(c.element,I,c.bm,true,c);if(this.xk)C(c.element,this.xk)};
xt.prototype.kg=function(a){for(var b=0;b<this.ta.length;b++){var c=this.ta[b];if(c.element==a){this.j.Sa(c.element,I,c.bm,true,c);if(this.xk)qf(c.element,this.xk);this.ta.splice(b,1)}}};
xt.prototype.$i=function(a,b){if(this.ff)return;this.ff=b;var c=new ai("dragstart",this,this.ff);if(this.dispatchEvent(c)==false){c.b();return}c.b();var d=b.fu(),e;if(d.parentNode){e=uh(d);e.x-=parseInt(rh(d,"marginLeft"),10)*2;e.y-=parseInt(rh(d,"marginTop"),10)*2;this.Of=this.TG(d)}else{e=new ge(a.clientX,a.clientY);var f=Se();e.x+=f.x;e.y+=f.y;if(b.Bl)e=ie(e,b.Bl());this.Of=d}this.Of.style.position="absolute";this.Of.style.left=e.x+"px";this.Of.style.top=e.y+"px";if(this.Gt)C(this.Of,this.Gt);
Ie(d).body.appendChild(this.Of);this.fd=new Xh(this.Of);F(this.fd,"drag",this.tC,false,this);F(this.fd,"end",this.il,false,this);this.ks=[];for(var g,h=0;g=this.Ee[h];h++)for(var i,k=0;i=g.ta[k];k++)this.KF(g,i);if(!this.cj)this.cj=new ce(0,0,0,0);this.ij=null;this.fd.$i(a);a.preventDefault()};
var yt=function(a,b,c){ci.call(this,a,c);this.P=b};
o(yt,ci);yt.prototype.FA=function(){return this.element};
yt.prototype.fu=function(){return this.data&&this.data.eJ?this.data.eJ():this.bn};
yt.prototype.Bl=function(){return this.data&&this.data.fJ?this.data.fJ():new ge(0,0)};
yt.prototype.bm=function(a){if(this.P.HA()&&a.Vl(0))yt.f.bm.call(this,a)};var W=function(a,b,c){P.call(this);this.Xk=b||"goog-icon-list";this.Aa=[];this.j=new K(this);this.Wv=new K(this);this.VH=new K(this);this.bP(c||this.Xk+"-icon");this.aP(a)};
o(W,P);var zt=Fd("goog.ui.IconList");W.prototype.V=function(a){zt.Zb(a)};
W.prototype.Pe=null;W.prototype.Xu="";W.prototype.g5=false;W.prototype.$k=0;W.prototype.Q3=0;W.prototype.cx=null;W.prototype.le=null;W.prototype.Nc=0;W.prototype.Mz={};W.prototype.b=function(){if(!this.W()){W.f.b.call(this);this.j.b();this.Wv.b();this.VH.b();if(this.Nq)H(this.Nq)}};
W.prototype.D=function(){return"goog.ui.IconList"};
W.prototype.n=function(){W.f.n.call(this);of(this.e,this.Xk);this.Lc()};
W.prototype.u=function(){this.j.ma();W.f.u.call(this)};
W.prototype.Lc=function(){if(!this.Eb)return;if(!this.k||this.k&&this.k.isLoaded&&!this.k.isLoaded())return;if(this.nc)this.pz();this.oH();this.mW()};
W.prototype.oH=function(){if(this.Pe.prototype.zk){var a=w("table");x(this.e,a);L(a,"width","100%");this.nc=w("tbody");x(a,this.nc)}else{this.nc=w("div");x(this.e,this.nc);var b=w("div");L(b,"clear","both");L(b,"font-size","0");x(this.e,b)}of(this.nc,this.Xk+"-area")};
W.prototype.mW=function(){if(!this.k)return;var a=this.k.S(),b=0,c=a;if(this.g5){var d=this.XJ()*this.Q3;b=this.$k*d;c=Math.min(b+d,a)}var e=this.Pe.prototype.columns;this.kV(b,c,e);this.j.g(this,rt,this.gB);this.j.g(this,wk,this.xK);this.j.g(this,vk,this.xK)};
W.prototype.kV=function(a,b,c){if(c){var d=this.Pe.prototype.zk,e;for(var f=a;f<b;f++){if((f-a)%c==0){e=w(d?"tr":"div");x(this.nc,e)}this.Op(f,e)}}else for(var f=a;f<b;f++)this.Op(f,this.nc)};
W.prototype.Op=function(a,b){var c;if(this.k)c=this.k.wb(a);var d=new this.Pe(this,this.Xu,a,c);this.Z(d);d.C(b);if(this.le)this.le.ub(d.A(),d)};
W.prototype.pz=function(){this.Pd();this.j.ma();this.sh(function(a){a.b()});
this.Va=null;this.Lf=null;if(this.Eb)We(this.e);this.nc=null};
W.prototype.XJ=function(){var a=this.Pe.prototype.columns,b=this.Pe.prototype.width;if(a)return a;else if(b)return Math.floor(this.nc.offsetWidth/b)||1;else if(this.Va.length){var c=uh(this.Va[0].A()).x;for(var d=1;d<this.Va.length;d++){var e=this.Va[d],f=uh(e.A()).x;if(f<=c)break;c=f}return d}return 0};
W.prototype.U=function(a){if(this.k){this.pz();this.Wv.ma()}W.f.U.call(this,a);if(this.k){this.Wv.g(this.k,"insert",this.i0);this.Wv.g(this.k,"remove",this.Ku);this.Wv.g(this.k,"update",this.Nu)}if(this.Eb)this.Lc()};
W.prototype.i0=function(){this.Lc()};
W.prototype.Ku=function(){this.Lc()};
W.prototype.Nu=function(a){var b=a.index,c=a.endIndex;for(b;b<=c;b++){var d=this.wd(b);if(d)d.aa(a)}};
W.prototype.tk=function(a){this.Xk=a;if(this.Eb)of(this.e,a)};
W.prototype.aP=function(a,b){if(this.Pe!=a){this.Pe=a;this.pz();if(b)this.bP(b);this.Lc()}};
W.prototype.bP=function(a){if(this.Xu!=a){this.Xu=a;this.sh(function(b){b.tk(a)})}};
W.prototype.enableDragging=function(a){if(a&&!this.le){this.le=new xt;this.le.TF(this.le);if(this.Eb)this.sh(function(b){this.le.ub(b.A(),b)},
this);this.VH.g(this.le,"dragstart",this.Hl);this.VH.g(this.le,"dragend",this.vi)}if(this.le)this.le.Ca(a)};
W.prototype.HS=function(a){if(!this.le){this.enableDragging(true);this.enableDragging(false)}this.le.TF(a)};
W.prototype.Hl=function(){for(var a=0;a<this.Aa.length;a++)Ch(this.Aa[a].A(),0.5)};
W.prototype.vi=function(){for(var a=0;a<this.Aa.length;a++)Ch(this.Aa[a].A(),1)};
W.prototype.Ug=function(a){this.Nc=a;if(a==0)this.Pd();else if(a==1){var b=this.cx;this.Pd();if(b)this.Mr(b,true)}};
W.prototype.oe=function(){return this.Aa};
W.prototype.dZ=function(){return this.Aa.length};
W.prototype.gB=function(a){var b=a.target;if(this.Nc==1){a.ctrlKey=false;a.shiftKey=false}if(ze){a.ctrlKey=a.shiftKey;a.shiftKey=false}if(a.shiftKey){if(this.Aa.length==0)this.ux(b);var c=this.QJ(),d=b.Eg();if(d<c){var e=c;c=d;d=e}if(!a.ctrlKey)this.Pd();for(var f=c;f<=d;f++)this.Mr(this.Va[f],true)}else if(a.ctrlKey||this.Nc==3){this.Mr(b,!b.oe());this.ux(b)}else if(!b.oe()){this.Pd();this.Mr(b,true);this.ux(b)}if(a.activated)this.hW();else this.Bt();a.stopPropagation();a.preventDefault()};
W.prototype.xK=function(a){var b=a.target,c=String(b.Eg()),d=false;if(a.type==wk&&Rc(this.Mz,c)){Nc(this.Mz,c);d=true}else if(a.type==vk&&b.oe()){Pc(this.Mz,c,true);d=true}if(d){this.CG(b,a.type==wk);this.Bt()}a.stopPropagation();a.preventDefault()};
W.prototype.wO=function(a){var b=this.wd(a);if(b){this.Pd();this.Mr(b,true);this.ux(b);this.Bt()}};
W.prototype.Pd=function(){if(this.Aa.length){while(this.Aa.length)this.Aa.pop().Oc(false);this.Bt()}};
W.prototype.Mr=function(a,b){if(a.sb()){this.CG(a,b);this.TB=a}};
W.prototype.CG=function(a,b){var c=b?zc:Ac;c(this.Aa,a,qt);a.Oc(b)};
W.prototype.ux=function(a){this.cx=a};
W.prototype.QJ=function(){return this.cx?this.cx.Eg():0};
W.prototype.Un=function(a){return this.Va?yc(this.Va,a,qt):-1};
W.prototype.Bt=function(){this.dispatchEvent(lg)};
W.prototype.hW=function(){this.dispatchEvent(kg)};
W.prototype.gI=function(a){if(a&&!this.Nq)this.Nq=F(document,gg,this.m0,false,this);else if(!a&&this.Nq){H(this.Nq);this.Nq=null}};
W.prototype.m0=function(a){if(!(this.Va&&this.Va.length&&At(a)))return;var b=a.keyCode,c=this.TB?this.TB.Eg():this.QJ(),d=this.XJ();if(b==38||b==40){var e=b==38?c-d:c+d;if(e>=0&&e<this.Va.length)c=e}else if(b==37){var e=c-1;if(e%d<c%d)c=e}else if(b==39){var e=c+1;if(e%d>c%d)c=e}c=pe(c,0,this.Va.length-1);var f=this.Va[c];if(f!=this.TB)a.preventDefault();if(!a.shiftKey)this.Pd();this.gB(new pt(f,a));this.TB=f};
var At=function(a){if(a.altKey&&!a.ctrlKey)return false;switch(a.keyCode){case 38:case 40:case 37:case 39:return true}return false};var Et=function(a,b,c,d){J.call(this);this.fi=this.y3();this.hea=a;this.us=b;this.DB=c;this.tw=d;this.ut={};this.Kd=[];this.rea=[];this.Nc=0;this.qk=null;this.Yea=false;this.b6=false;this.TV=false;this.nd=null;this.Nr=null;this.y9();this.hi=null;this.Fba=null;this.IU=parseInt(this.OI("contentWidth"),10);this.wba=parseInt(this.OI("contentHeight"),10);this.q3();if(this.hi!=null)this.Nc=this.hi.selectMode;this.tQ=new Bt(this);this.mc=new Ct(this);this.Sm=new Dt(this);this.Rfa={};this.c3();if(this.hi!=
null)this.xJ();document.body.appendChild(this.fi)};
o(Et,J);Et.prototype.RI=function(){var a={};a.width=this.IU;a.height=this.wba;return a};
Et.prototype.c3=function(){var a=document.createElement("table"),b=document.createElement("tbody"),c=document.createElement("tr"),d=document.createElement("td");a.appendChild(b);a.setAttribute("cellpadding","0");a.setAttribute("cellspacing","0");a.style.borderCollapse="collapse";b.appendChild(c);c.appendChild(d);d.appendChild(this.tQ.getContainer());c=document.createElement("tr");this.hH=document.createElement("td");this.hH.height=s?266:270;b.appendChild(c);c.appendChild(this.hH);this.b3();c=document.createElement("tr");
d=document.createElement("td");b.appendChild(c);c.appendChild(d);var e=document.createElement("center");e.style.padding="20px";this.qk=document.createElement("input");this.qk.type="button";this.qk.disabled="1";this.qk.value=Ft;F(this.qk,E,l(this.AD,this));e.appendChild(this.qk);var f=document.createElement("input");f.type="button";f.value=Gt;Ht(f,"position:relative; left:6px");F(f,E,l(this.bU,this));e.appendChild(f);d.appendChild(e);this.fi.appendChild(a)};
Et.prototype.b3=function(){var a=document.createElement("table");a.setAttribute("cellspacing","8");a.setAttribute("cellpadding","0");Ht(a,"position:relative; left:4px");var b=document.createElement("tbody"),c=document.createElement("tr"),d=document.createElement("td");a.appendChild(b);b.appendChild(c);c.appendChild(d);d.width=this.IU;d.appendChild(this.Sm.getContainer());d.appendChild(this.mc.getContainer());this.hH.appendChild(a)};
Et.prototype.bJ=function(){return this.fi};
Et.prototype.show=function(a){this.fi.style.display="block";this.jm=a;this.mc.reset()};
Et.prototype.hide=function(){this.fi.style.display="none";if(this.jm)this.jm();this.jm=null};
Et.prototype.yx=function(a,b){if(this.qk){this.qk.disabled=a?"":"1";if(b!=null)this.qk.value=b}};
Et.prototype.JJ=function(){return this.tw};
Et.prototype.rx=function(a,b){if(this.tw!=a){this.tw=a;var c=this.mc.gd();if(a=="BANNER"&&this.Nc!=1||c&&c.id!=this.DB){this.Nc=0;this.xJ()}this.tQ.SE()}this.TV=b};
Et.prototype.JI=function(){return this.Kd};
Et.prototype.pY=function(){return this.hi};
Et.prototype.r1=function(){return this.hi!=null&&this.hi.selectMode==0};
Et.prototype.TX=function(a){for(var b=0;b<this.Kd.length;b++){var c=this.Kd[b];if(a==c.id)return c}return null};
Et.prototype.Ug=function(a){if(this.Nc!=a){this.Sm.getContainer().style.display=a==0?"block":"none";this.mc.getContainer().style.display=a==1?"block":"none";this.Nc=a}var b=this.uu();if(b)this.yx(b.lB(),b.WA());this.tQ.Ug(a)};
Et.prototype.uu=function(){if(this.Nc==0)return this.Sm;else if(this.Nc==1)return this.mc;return null};
Et.prototype.AD=function(){if(this.Nc==0){this.mc.GO(this.Sm.$J());this.Ug(1)}else if(this.Nc==1){this.hide();this.nd=this.mc.gd();this.Nr=this.mc.xZ();if(this.Nr){this.OQ(this.Nr);if(!this.TV)this.MM(null)}}else this.hide()};
Et.prototype.n7=function(a){if(this.nd&&this.Nr){this.MM(a);return false}return true};
Et.prototype.bU=function(){this.hide()};
Et.prototype.y3=function(){var a=document.createElement("div"),b="position: absolute; top: 20px; left:20px; z-index:1001; display:none; padding:0px; width:408px; height:380px; ";if(s&&!Ee("8")){var c=document.getElementsByTagName("base")[0],d=c.href+"img/dialogframe.png",e=document.createElement("div"),f='position:absolute; width:408px; height:380px; display:block; z-index:-1; filter: progid:DXImageTransform.Microsoft.AlphaImageLoader(enabled="true", src="'+d+'", sizingMethod="scale");';Ht(e,f);a.appendChild(e)}else b+=
'background-image: url("img/dialogframe.png")';Ht(a,b);return a};
Et.prototype.OI=function(a){return this.ut[a]};
Et.prototype.y9=function(){this.ut.commandLabel=It;this.ut.logoURL="img/googlephotos_sm.gif";this.ut.contentWidth=364;this.ut.contentHeight=248};
Et.prototype.Zi=function(){if(this.Nc==0&&this.Sm)this.Sm.Zi();else if(this.Nc==1&&this.mc)this.mc.Zi()};
Et.prototype.OQ=function(a){var b="lhcl_portrait_id",c="48",d=false;if(this.tw=="BANNER"){b="lhid_cover_id";if(!_features.googlephotos)c="160";d=_album.blogger}var e=a.thumbnailURL,f=e.split("?"),g=f[0];g=g.replace(/\/s160-c\//,"/s"+c+"-c/");var h=document.getElementById(b);if(h)if(d)h.style.background="url('"+g+"') no-repeat left";else h.src=g;this.dispatchEvent(new Jt(this.nd.id,g))};
Et.prototype.kN=function(a){var b;if(s){b=new ActiveXObject("Microsoft.XMLDOM");b.async=false;b.loadXML(a)}else{var c=new DOMParser;b=c.parseFromString(a,"text/xml")}return b};
Et.prototype.MM=function(a){var b=this.nd.id,c=this.hea+"&uname="+this.us+"&aid="+b,d=(new Date).getTime();c+="&nocache="+d;var e="selectedphotos="+this.Nr.id+"&noredir=true&optgt="+this.tw;Gj(c,l(this.j6,this,this.Nr,a),"POST",e)};
Et.prototype.j6=function(a,b,c){var d=c.target;if(d.Re()){this.OQ(a);if(b)b.submit()}};
Et.prototype.xJ=function(){var a=this.hi.url;this.wx(true);Gj(a,l(this.Kw,this))};
Et.prototype.Kw=function(a){var b=this.uu(),c=a.target;this.wx(false);if(!c.Re()){if(b.yj)b.yj(Kt);return}var d=c.Sb();if(d.substring(0,5)!="<?xml"){if(b.yj)b.yj(Lt);return}var e=null,f=this.Nc==0,g;g=s?this.kN(d):c.VJ();e=new Mt(this,g,f);if(e!=null)this.Kd=e.yJ();this.D7();var h=this.uu();if(h)h.Zi()};
Et.prototype.D7=function(){if(this.DB&&this.DB.length>0){var a=this.TX(this.DB);if(a!=null){this.mc.GO(a);this.Ug(1)}}};
Et.prototype.LB=function(){return this.b6};
Et.prototype.wx=function(a){this.b6=a};
Et.prototype.c2=function(){return this.hi&&this.hi.extraParams=="q=*"};
Et.prototype.q3=function(){var a="http://"+document.domain+":"+document.location.port+"/data/feed/api/user/"+this.us+"?kind=album&thumbsize=160",b=new Nt(Ot,"lh",this.us,"Lighthouse",a,0,null,null);this.rea.push(b);this.hi=b;this.Fba="lh"};
function Pt(a,b,c){this.title=a;this.id=b;this.thumbnail=c;this.r5=[];this.description=null;this.Cr=0;this.Q=null;this.gX=false;this.Gfa=null}
Pt.prototype.QS=function(a){this.r5.push(a)};
Pt.prototype.S=function(){return this.Cr};
Pt.prototype.kZ=function(){return this.r5};
function Qt(a,b,c,d,e){this.title=a;this.id=c;this.thumbnailURL=d;this.fullsizeURL=e;this.mediaType=b;this.fp=-1;this.Qu=-1}
Qt.prototype.k8=function(a,b){this.fp=a;this.Qu=b};
Qt.prototype.Jn=function(){return 1};
Qt.prototype.sY=function(){if(this.fp==-1||this.Qu==-1)return null;return this.fp.toString()+"x"+this.Qu.toString()};function Dt(a){this.hb=a;this.contentSize=this.hb.RI(true);this.fi=this.x3();this.sk=-1;this.Lr=[]}
Dt.prototype.$J=function(){if(this.sk<0)return null;var a=this.hb.JI();return a[this.sk]};
Dt.prototype.getContainer=function(){return this.fi};
Dt.prototype.lB=function(){return this.sk>=0};
Dt.prototype.WA=function(){return Rt};
Dt.prototype.Zi=function(){this.v5()};
Dt.prototype.x3=function(){var a=document.createElement("div"),b=this.contentSize.height+8,c="position:relative; top:16px; left:6px;  font-family:sans-serif; height:"+b+"px; width:"+this.contentSize.width;c+="px; background: white; overflow:auto; overflow-x:hidden";Ht(a,c);var d=document.createElement("table");d.width="100%";d.setAttribute("cellspacing","0");d.setAttribute("cellpadding","0");d.style.borderCollapse="collapse";var e=document.createElement("tbody");d.appendChild(e);this.ai=e;this.Py=
d;this.Qm(e,true);a.appendChild(d);F(a,I,l(this.Xv,this));a.ondblclick=l(this.Ft,this);return a};
Dt.prototype.v5=function(){var a=this.hb.JI();this.Lr=[];this.sk=-1;this.Py.removeChild(this.ai);this.ai=document.createElement("tbody");this.Py.appendChild(this.ai);if(a.length<1){this.Qm(this.ai,false);return}for(var b=0;b<a.length;b++){var c=a[b],d=document.createElement("tr");this.ai.appendChild(d);d.style.background=b%2?"#ffffff":"#eeeeee";var e=document.createElement("td");e.height=32;e.width=42;var f=document.createElement("img");f.src=c.thumbnail!=null?c.thumbnail:"img/transparent.gif";f.width=
32;f.height=28;f.hspace=4;f.style.border="1px solid #808080";e.appendChild(f);d.appendChild(e);var e=document.createElement("td");e.height=32;var g=c.S(),h=c.title.replace(/_/g," "),i=document.createElement("font");i.color="#4D4D4D";i.size="4";i.style.cursor="default";i.innerHTML=p(h);e.appendChild(i);var k=document.createElement("font");k.innerHTML="&nbsp;("+g+")";k.style.position="relative";k.style.top="-2px";e.appendChild(k);d.appendChild(e);d.JL=i;d.OU=k;d.Ra=f;this.Lr.push(d)}};
Dt.prototype.Qm=function(a,b){var c=document.createElement("tr"),d=document.createElement("td"),e="<br><br><br><center><font size=4 font color=#4D4D4D>";e+=this.hb.LB()||b?St+'<img src="img/spin.gif" hspace=8></font></center>':Tt;d.innerHTML=e;c.appendChild(d);a.appendChild(c)};
Dt.prototype.yj=function(a){this.Py.removeChild(this.ai);this.ai=document.createElement("tbody");this.Py.appendChild(this.ai);var b=document.createElement("tr"),c=document.createElement("td");c.innerHTML="<br><br><br><center><font size=4 font color=#cc0000>"+a+"</font></center>";b.appendChild(c);this.ai.appendChild(b)};
Dt.prototype.E7=function(a){if(this.sk!=a){var b;if(this.sk>=0){b=this.Lr[this.sk];b.style.background=this.sk%2?"#ffffff":"#eeeeee";b.JL.color="#4D4D4D";b.OU.color="#4D4D4D";b.Ra.style.border="1px solid #808080"}b=this.Lr[a];if(b){b.style.background="#112ABB";b.JL.color="#ffffff";b.OU.color="#ffffff";b.Ra.style.border="1px solid white";this.sk=a;this.hb.yx(true,Ft)}}};
Dt.prototype.Xv=function(a){var b=Ut(a),c=uh(this.ai),d=Ah(this.fi),e=b.y-c.y;if(ve&&b.x-c.x>=d.width-16)return;var f=0;for(var g=0;g<this.Lr.length;g++){var h=this.Lr[g],i=Ah(h).height;if(ve)i=Math.max(32,Ah(h.JL).height);f+=i;if(e<=f){this.E7(g);return}}};
Dt.prototype.Ft=function(){if(this.$J())this.hb.AD()};function Ct(a){this.hb=a;this.lt=this.hb.RI(true);this.fi=this.z3();this.Ti=-1;this.Ql=[];this.tN=[];this.nd=null;this.lF=5;this.mT=this.hb.mT}
Ct.prototype.GO=function(a){if(a!=this.nd){this.nd=a;if(a!=null)this.m3();this.Ti=-1;this.yN()}};
Ct.prototype.gd=function(){return this.nd};
Ct.prototype.getContainer=function(){return this.fi};
Ct.prototype.lB=function(){return this.Ti>=0};
Ct.prototype.mR=function(){return true};
Ct.prototype.WA=function(){return Vt};
Ct.prototype.z3=function(){var a=document.createElement("div"),b=4,c=this.lt.height;if(this.mR())c-=20;else b=16;var d="display:none; position:relative; top:"+b+"px; left:6px; height:"+c+"px; width:"+this.lt.width+"px";Ht(a,d);this.Tj=document.createElement("div");d="height:"+c+"px; width:"+this.lt.width+"px; padding-left: 6px; padding-top: 4px; font-family:sans-serif; background: white; overflow:auto";Ht(this.Tj,d);this.dj=document.createElement("div");this.Tj.appendChild(this.dj);this.Qm();F(this.Tj,
I,l(this.Xv,this));this.Tj.ondblclick=l(this.Ft,this);a.appendChild(this.Tj);if(this.mR()){this.We=document.createElement("div");Ht(this.We,"position:absolute; top:238px; left:69px; display: none");this.XP=new Wt("zoom","img/slidertrack.png","img/sliderknob.png",215,0,100,50,17,24,l(this.Caa,this),null);var e=this.XP.DZ();this.XP.Is();this.We.appendChild(e);a.appendChild(this.We)}else this.We=null;return a};
Ct.prototype.Caa=function(a){this.setZoom(Math.round((100-a)/10))};
Ct.prototype.yN=function(){this.Tj.removeChild(this.dj);this.dj=document.createElement("div");this.Tj.appendChild(this.dj);this.Ql=[];this.tN=[];var a;a=this.nd==null?this.hb.Kd:this.nd.kZ();if(a.length<1){if(this.hb.LB())this.Qm();else this.yj(Xt);if(this.We)this.We.style.display="none";return}for(var b=0;b<a.length;b++){var c=a[b],d=document.createElement("img"),e=Math.round((this.lt.width-60)/this.lF),f=c.Jn();if(f==0)var g=Math.round(e*3/4);else if(f>1)var g=Math.round(e/f);else{e=Math.round(e*
f);var g=Math.round(e/f)}d.src=c.thumbnailURL;d.width=e;d.height=g;if(c.title!=null)d.title=this.NZ(c);if(b==this.Ti){d.style.border="2px solid #112ABB";d.hspace=0;d.vspace=0}else{d.style.border="1px solid #808080";d.hspace=1;d.vspace=1}this.dj.appendChild(d);this.Ql.push(d);this.tN.push(c)}if(this.We)this.We.style.display=a.length>0?"inline":"none"};
Ct.prototype.Qm=function(){if(this.hb.c2()&&!this.hb.LB())return;var a=document.createElement("div");a.innerHTML="<br><br><br><center><font size=4 font color=#4D4D4D>"+Yt+'<img src="img/spin.gif" hspace=8></font></center>';this.dj.appendChild(a)};
Ct.prototype.yj=function(a){this.Tj.removeChild(this.dj);this.dj=document.createElement("div");this.Tj.appendChild(this.dj);var b=document.createElement("div");b.innerHTML="<br><br><br><center><font size=4 font color=#cc0000>"+a+"</font></center>";this.dj.appendChild(b)};
Ct.prototype.NZ=function(a){var b=a.title,c=a.sY();if(c!=null)b+=", "+c;return b};
Ct.prototype.xZ=function(){if(this.Ti<0)return null;return this.tN[this.Ti]};
Ct.prototype.Oc=function(a){if(a!=this.Ti){if(this.Ti>=0){var b=this.Ql[this.Ti];b.style.border="1px solid #808080";b.hspace=1;b.vspace=1}var c=this.Ql[a];c.style.border="2px solid #112ABB";c.hspace=0;c.vspace=0;this.Ti=a;this.hb.yx(true,It)}};
Ct.prototype.Baa=function(){var a=Math.round((this.lt.width-40)/this.lF);for(var b=0;b<this.Ql.length;b++){var c=this.Ql[b];c.width=a;c.height=a}};
Ct.prototype.NT=function(a){for(var b=0;b<this.Ql.length;b++){var c=this.Ql[b],d=uh(c),e=Ah(c);if(a.x>=d.x&&a.y>=d.y&&a.x<d.x+e.width&&a.y<d.y+e.height)return b}return null};
Ct.prototype.m3=function(){var a;if(this.nd!=null){if(this.nd.Q==null||this.nd.gX)return;a=this.nd.Q}else{var b=this.hb.pY();a=b.url}if(!this.mT)a+="&nocache="+Ka();this.hb.wx(true);Gj(a,l(this.Kw,this))};
Ct.prototype.Kw=function(a){var b=null,c=a.target,d=document.location.hash;if(d.length>0&&d.charAt(0)=="#")d=d.substring(1);this.hb.wx(false);if(!c.Re()){var e=this.hb.uu();e.yj(Zt);return}var f;f=s?this.hb.kN(c.Sb()):c.VJ();b=new Mt(this.hb,f,false);var g=null;if(b!=null){if(this.nd==null)this.hb.Kd=[];g=b.yJ();for(var h=0;h<g.length;h++){var i=g[h];if(this.nd!=null)this.nd.QS(i);else this.hb.Kd.push(i);if(i.id==d){this.Ti=h;this.hb.yx(true,It)}}}if(this.nd)this.nd.gX=true;this.Zi()};
Ct.prototype.Xv=function(a){var b=Ut(a),c=this.NT(b);if(c!=null)this.Oc(c)};
Ct.prototype.Ft=function(){this.hb.AD()};
Ct.prototype.setZoom=function(a){if(a<1)a=1;if(this.lF!=a){this.lF=a;this.Baa()}};
Ct.prototype.Zi=function(){this.yN()};
Ct.prototype.reset=function(){this.XP.qa(50)};function Mt(a,b,c){this.hb=a;this.Saa=b;this.xf=[];this.ZW(c)}
Mt.prototype.yJ=function(){return this.xf};
Mt.prototype.ZW=function(a){var b=this.Saa.getElementsByTagName("entry");for(var c=0;c<b.length;c++){var d=b[c];if(a)this.zS(d);else this.RS(d)}};
Mt.prototype.qi=function(a,b){var c=this.rq(a,b);for(var d=0;d<c.length;d++){var e=lf(c[d]);if(e)return e}return null};
Mt.prototype.rq=function(a,b){var c=b,d=b.split(":");if(d.length>1){if(!s)c=d[1]}else if(ve&&!Ee("420"))c="*";var e=a.getElementsByTagName(c),f=[];for(var g=0,h=e.length;g<h;g++)if(e[g].nodeName==b)f.push(e[g]);return f};
Mt.prototype.BJ=function(a,b){var c=this.rq(a,"link");for(var d=0;d<c.length;d++){var e=c[d].getAttribute("rel");if(e!=b)continue;return c[d].getAttribute("href")}return null};
Mt.prototype.zS=function(a){var b=this.qi(a,"title"),c=this.rq(a,"media:thumbnail"),d=c[0].getAttribute("url"),e=this.qi(a,"gphoto:numphotos"),f=this.BJ(a,"http://schemas.google.com/g/2005#feed"),g=aj(f);g.fa("thumbsize",160);f=g.toString();var h=this.qi(a,"gphoto:id"),i=this.qi(a,"summary");d=this.Ky(d,f);var k=new Pt(b,h,d);k.Cr=parseInt(e,10);k.Q=f;k.description=i;this.xf.push(k)};
Mt.prototype.RS=function(a){var b=this.qi(a,"title"),c=this.BJ(a,"alternate"),d=this.rq(a,"media:thumbnail"),e=d[0].getAttribute("url"),f=this.rq(a,"media:content"),g=f[0].getAttribute("url"),h=this.qi(a,"gphoto:id"),i=this.qi(a,"gphoto:videostatus");if(i&&i!="final"){e=at(i,"_cropped");g=at(i,"")}else{e=this.Ky(e,c);g=this.Ky(g,c)}var k=parseInt(this.qi(a,"gphoto:width"),10),n=parseInt(this.qi(a,"gphoto:height"),10),q=new Qt(b,"image/jpeg",h,e,g);q.k8(k,n);this.xf.push(q)};
Mt.prototype.Ky=function(a,b){if(a==null)return null;var c=a;if(a.length<7||a.substring(0,7)!="http://"){var d=b.split("/");c="http://"+d[2]+a}return c};function Nt(a,b,c,d,e,f,g,h){this.title=a;this.id=b;this.user=c;this.Cn=d;this.url=e;this.selectMode=f;this.description=g;this.extraParams=h;this.hasTags=false;this.canDelete=false;this.xba=[];this.Dfa=false}
Nt.prototype.push=function(a){this.xba.push(a)};function Bt(a){this.hb=a;this.Q$=this.d3();this.Nc=0;this.SE();this.yfa=false;this.wfa=0}
Bt.prototype.d3=function(){var a=document.createElement("div"),b=s?"4":"8";Ht(a,"position:relative; top:4px; left:12px; background: transparent; font-family:sans-serif; width:380px; padding:0px; height:42px");var c=document.createElement("div");Ht(c,"position:absolute; top:"+b+"px; left:4px; color: white; font-weight: bold; cursor:default");c.unselectable="on";c.innerHTML=$t;a.appendChild(c);this.Rq=document.createElement("div");Ht(this.Rq,"position:absolute; top:32px; left:4px; color: #777777; cursor: pointer");
a.appendChild(this.Rq);F(this.Rq,E,l(this.I$,this),true);F(a,I,l(this.Hl,this),true);return a};
Bt.prototype.getContainer=function(){return this.Q$};
Bt.prototype.K8=function(a){this.Rq.innerHTML="<font size=2><b>"+a+"</b></font>"};
Bt.prototype.I$=function(){if(this.Nc==1)if(this.hb.r1()&&this.hb.JJ()=="PORTRAIT"){this.hb.Ug(0);this.hb.Sm.Zi()}};
Bt.prototype.SE=function(){var a=this.hb.JJ()=="BANNER",b=au,c="#777777",d="none";if(a)b=bu;else if(this.Nc==1){b=cu;c="#112ABB";d="underline"}this.K8(b);this.Rq.style.color=c;this.Rq.style.textDecoration=d};
Bt.prototype.Ug=function(a){this.Nc=a;this.SE()};
Bt.prototype.Hl=function(a){this.sL=Ut(a);var b=this.hb.bJ(),c=uh(this.Q$),d=this.sL.y-c.y;this.xca=parseInt(b.style.left,10);this.yca=parseInt(b.style.top,10);if(d>28)return false;this.aw=l(this.xq,this);this.dy=l(this.vi,this);this.uw=l(this.pf,this);F(document.body,fg,this.aw);F(document.body,eg,this.dy);F(document.body,Hf,this.uw);return false};
Bt.prototype.xq=function(a){var b=Ut(a),c=this.hb.bJ(),d=b.y-this.sL.y+this.yca,e=b.x-this.sL.x+this.xca,f=Qe(),g=f.width-40,h=f.height-40;d=Math.max(Math.min(d,h),0);e=Math.max(Math.min(e,g),0);c.style.top=d+"px";c.style.left=e+"px";du(a);return false};
Bt.prototype.vi=function(a){G(document.body,fg,this.aw);G(document.body,eg,this.dy);G(document.body,Hf,this.uw);du(a);return false};
Bt.prototype.pf=function(a){if(!(a.relatedTarget||a.toElement))this.vi(a)};var Ht=function(a,b){if(s)a.style.setAttribute("cssText",b);else a.setAttribute("style",b)},
du=function(a){if(a.stopPropagation){a.preventDefault();a.stopPropagation()}else{a.cancelBubble=true;a.returnValue=false}},
eu=function(){var a=0,b=0;if(ve){a=-1*window.pageXOffset;b=-1*window.pageYOffset}else{var c=th();a=c.scrollLeft;b=c.scrollTop}return{x:a,y:b}},
Ut=function(a){var b={x:a.clientX,y:a.clientY},c=eu();b.x+=c.x;b.y+=c.y;return b};function Wt(a,b,c,d,e,f,g,h,i,k,n){this.pea=a;this.hba=b;this.wca=c;this.GT=d;this.e3=h;this.nO=i;this.ud=k;this.wW=n;this.We=null;this.Uv=e;this.dk=f;this.Hy=this.GT-this.e3-this.nO;this.sj=g;this.h$=g;this.zca=0;this.Qg()}
Wt.prototype.DZ=function(){return this.We};
Wt.prototype.Qg=function(){this.We=document.createElement("div");this.$y=document.createElement("img");this.$y.setAttribute("src",this.hba);this.$y.setAttribute("width",this.GT);this.vca=document.createElement("div");this.vca.setAttribute("id",this.pea+"-knob");this.Ah=document.createElement("img");this.Ah.setAttribute("src",this.wca);this.Ah.setAttribute("border","0");Ht(this.Ah,"position:relative; top:0px; left:0px; cursor:move");this.Ah.style.top="2px";this.We.appendChild(this.$y);this.We.appendChild(this.Ah);
this.PD()};
Wt.prototype.Is=function(){this.Hca=l(this.Hl,this);this.gba=l(this.g_,this);F(this.Ah,I,this.Hca);F(this.$y,E,this.gba)};
Wt.prototype.Hl=function(a){var b=Ut(a);this.zca=b.x-this.nO;this.sfa=parseInt(this.Ah.style.left,10);this.h$=this.sj;this.yT=uh(this.We);this.aw=l(this.xq,this);this.dy=l(this.vi,this);this.uw=l(this.pf,this);F(document.body,fg,this.aw);F(document.body,eg,this.dy);F(document.body,Hf,this.uw);du(a);return false};
Wt.prototype.xq=function(a){var b=Ut(a),c=Math.round(this.Ah.width/2),d=b.x-this.yT.x;if(d<this.Hy/2)c=0;this.sj=this.PT(d-c);this.PD();if(this.ud)this.ud(this.sj);du(a);return false};
Wt.prototype.vi=function(a){G(document.body,fg,this.aw);G(document.body,eg,this.dy);G(document.body,Hf,this.uw);if(this.wW)this.wW(this.h$,this.sj);du(a);return false};
Wt.prototype.pf=function(a){if(!(a.relatedTarget||a.toElement))this.vi(a)};
Wt.prototype.g_=function(a){this.yT=uh(this.We);return this.xq(a)};
Wt.prototype.MY=function(a){var b=this.OT(a),c=Math.round(this.Ah.width/2);return b-c-this.nO-this.Hy};
Wt.prototype.PD=function(){this.Ah.style.left=this.MY(this.sj)+"px"};
Wt.prototype.L=function(){return this.sj};
Wt.prototype.qa=function(a){if(a!=this.sj){this.sj=a;this.PD();if(this.ud)this.ud(this.sj)}};
Wt.prototype.OT=function(a){return(a-this.Uv)*this.Hy/(this.dk-this.Uv)};
Wt.prototype.PT=function(a){var b=Math.round(this.Ah.width/2),c=this.dk-this.Uv,d=this.Uv+Math.round(c*(a-this.e3)/(this.Hy-b));return Math.max(Math.min(d,this.dk),this.Uv)};;var Jt=function(a,b){D.call(this,fu);this.aid=a;this.thumbUrl=b};
o(Jt,D);var fu="coverchange";var hu=function(){this.un=[];this.te=gu(this)},
iu;o(hu,Vd);hu.prototype.Qy=false;var ju=new lk;hu.prototype.Sv="";var ku=0,gu=function(a){if(ju.isEmpty()){window.onbeforeunload=lu;F(window,"unload",mu)}var b=ku++;ju.J(b,a);return b},
lu=function(){iu=null;ju.forEach(function(a){if(!a.Qy&&a.I2())iu=a.Sv;else a.Qy=false});
if(iu)return iu};
hu.prototype.b=function(){if(!this.W()){hu.f.b.call(this);oc(this.un);ju.remove(this.te);if(ju.isEmpty()){window.onbeforeunload=null;G(window,"unload",mu)}}};
var mu=function(){var a=ju.tc();for(var b in a)ju.get(b).b()};
hu.prototype.CI=function(){this.Qy=true};
hu.prototype.I2=function(){return ic(this.un,function(a){return a.reader(a.elem)!=a.value})};
var nu=function(a){return a.value},
ou=function(a){return a.checked},
pu=function(a){var b=new Array(a.length);for(var c=0;c<a.length;c++)b[c]=a[c].selected?"T":"f";return b.join("")},
qu=function(a){return a.src};
hu.prototype.eD=function(a,b){if(!b){var c=ya(a);if(a.type=="checkbox"||a.type=="radio")b=ou;else if(a.type=="select-multiple")b=pu;else if("value"in a)b=nu;else if("src"in a)b=qu;else if(a.tagName=="FORM"||c){var d=c?a:[].concat(a.getElementsByTagName("input"),a.getElementsByTagName("select"),a.getElementsByTagName("textarea"));for(var e=0;e<d.length;e++)this.eD(d[e]);return}}if(b){var f=b(a);this.un.push({elem:a,value:f,reader:b})}};
hu.prototype.UD=function(a){this.Sv=a};
hu.prototype.A$=function(){for(var a=0;a<this.un.length;a++)this.un[a].value=this.un[a].reader(this.un[a].elem)};var su=function(a,b){D.call(this,ru,a);this.entity=a;this.albumId=b};
o(su,D);var ru="objectionrequest";m("View order");var tu=m("Community"),uu=m("Favorites"),vu=m("Link");m("Embed");m("Share Photos");var Lr=m("Unnamed"),wu=m("Type a name here");m("Clear");var xu=m("Cancel"),yu=m("Apply");m("Add New Person");var zu=m("Uploading..."),Au=m("Some of the selected files appear to be invalid. These files are marked in red. Please remove these files and try the upload again."),Bu=m("Not a valid file type.");m("Not a valid JPEG file.");var Cu=m("Select a thumbnail and click the map to place it or drag and drop the  thumbnails onto the map."),
Du=m("Back to album view"),Eu=m("The operation timed out.");m("Unknown");var Fu=m("My Favorites"),Gu=m("My Photos"),Hu=m("Community Photos");m("\u00ab Back to Search Results");var Iu=m("More from this album");m("Tag");m("Tag");m("Tag");var Ju=m("Tag");m("Tag");m("hide my tags");m("choose from my tags");m("Loading&#8230;");var Ku=m("Tags"),Lu=m("Cancel"),Mu=m("Add");m("Add Tag");var Nu=m('For example: beach "pacific ocean"'),Ou=m("You can enter a space separated list of tags.");m("No tags");m("This list displays the photos you have previously uploaded to orkut scraps, which can be found in the Google Photos Drop Box.");
m("This list displays the photos you have previously uploaded to orkut scraps, which can be found in the Picasa Web Albums Drop Box.");m("In the Google Photos tab, you can find uploaded photos in the Drop Box.");m("In the Picasa Web Albums tab, you can find uploaded photos in the Drop Box.");m("In the Google Photos tab, you can find uploaded photos in the Drop Box.");var cu=m("&laquo;&nbsp;Back to Albums List"),bu=m("Select a Cover Photo:"),au=m("Select an Album to browse:"),$t=m("Photo Chooser"),
Zt=m("Sorry, we couldn't load the feed.  Try again in a little while."),Yt=m("Loading Photos..."),Xt=m("No Images Found."),Vt=m("Choose Photo"),Tt=m("No Albums Found."),St=m("Loading Album List..."),Rt=m("Select Album"),Ot=m("My Google Photos"),It=m("Choose Photo"),Lt=m("Sorry, we can't find the requested feed."),Kt=m("Sorry, we couldn't load the feed. Try again in a little while."),Ft=m("Select Album"),Gt=m("Cancel"),Pu=m("There was an error communicating with the server. Please try again.");m("group");
m("Please enter a subject.");m("Learn More");m("Change this setting");var Qu=m("Your public albums will not be included in community search."),Ru=m("Your public albums can be found in community search.");m("Name");var Su=m("Upload date"),Tu=m("Album date"),Uu=m("Sort by:");m("My Favorites");var Vu=m("Unlisted albums you have seen"),Wu=m("Shared gallery with you."),Xu=m("Shared a photo with you."),Yu=m("show link on my gallery"),Zu=m("remove album"),$u=m("remove user"),av=m(" (deleted)"),bv=m("Activity"),
cv=m("List"),dv=m("View:");m("Exit");var ev=m("pause"),fv=m("play"),gv=m("show captions"),hv=m("hide captions"),iv=m("seconds"),jv=m("Large"),kv=m("Medium"),lv=m("Small"),mv=m("Thumbnail"),nv=m("Select size"),ov=m("Show link to album"),pv=m("Link to this Photo");m("Album cover set.");m("Video deleted.");m("Photo deleted.");var qv=m("Are you sure you want to move these photos? They will no longer appear in your blog if you move them."),rv=m("Are you sure you want to delete the selected photos? They will no longer appear in your blog if you delete them."),
sv=m("Are you sure you want to move this photo? It will no longer appear on other sites if you move it."),tv=m("Are you sure you want to move this photo? It will no longer appear in your blog if you move it."),uv=m("Are you sure you want to delete this photo? It will no longer appear on other sites if you delete it."),vv=m("Are you sure you want to delete this photo? It will no longer appear in your blog if you delete it. It may take up to 24 hours for photos to be removed.");m("Are you sure you want to delete this photo? It will no longer appear in your blog if you delete it.");
var wv=m("Are you sure you want to delete this video?"),xv=m("Are you sure you want to delete this photo?"),yv=m("Delete this video"),zv=m("Delete this photo"),Av=m("From"),Bv=m("Hide album link");m("Copy and paste the HTML below to post a thumbnail of this image on your blog or MySpace page.");m("Embed in Blog/MySpace");var Cv=m("Move to another album"),Dv=m("Copy to another album"),Ev=m("Set as album cover"),Fv=m("Video Location"),Gv=m("Photo Location"),Hv=m("View album map"),Iv=m("Add location"),
Jv=m("Edit location"),Kv=m("The server may be a little bit broken temporarily. Please try again in a few moments while it sorts itself out."),Lv=m("Something unusual has happened. Hopefully, it is so unusual that you will never see this dialog again."),Mv=m("It seems the item you requested has been misplaced. Please check your internet connection, cross your fingers, and try again."),Nv=m("The server denied permission for this action. Please make sure you are still logged in to your account and try again.");
m("We're sorry, this photo is not currently available. It may have been deleted by its owner, or there may have been an error on the server. Click OK to continue.");var Ov=m("Order Prints"),Pv=m("Remove from order"),Qv=m("Download Flash");m("You need Adobe\u00ae Flash\u00ae Player 7 or higher to play slideshows.");var Rv=m("You need Adobe\u00ae Flash\u00ae Player 7 or higher to watch videos."),Sv=m("You need Flash 7 or higher to watch videos."),Tv=m("n/a"),Uv=m("No"),Vv=m("Yes"),Wv=m("Flash Used"),
Xv=m("Longitude"),Yv=m("Latitude"),Zv=m("Focal Length"),$v=m("Aperture"),aw=m("Exposure"),bw=m("ISO"),cw=m("Model"),dw=m("Camera"),ew=m("Filename"),fw=m("less info"),gw=m("more info"),hw=m("Slideshow");m("Download Video");var iw=m("Download Photo"),jw=m("delete"),kw=m("Add a comment"),lw=m("Post Comment"),mw=m("view all"),nw=m("Latest comment"),ow=m("Add a comment"),pw=m("Your comment seems to be a bit long. Please, do try to be more efficient with your words."),qw=m("Please enter a comment first."),
rw=m("Are you sure you want to delete this comment?"),sw=m("Commenting is closed for this photo."),tw=m("Are you sure you want to clear your order?"),uw=m("Please select a print provider from the list below:"),vw=m("We're sorry.  Picasa Web Albums does not currently work with any photo processing providers that ship to your country."),ww=m("Failed to retrieve list of Print Providers.  Please try again later."),xw=m("OK"),yw=m("Are you sure you want to delete this caption?"),zw=m("Delete caption"),
Aw=m("Save Caption"),Bw=m("Add a Caption"),Cw=m("edit"),Dw=m("Photos in the Drop Box will be removed from any other sites where they are used.");m("Last \u00bb");m("Next \u203a");m("\u2039 Previous");m("\u00ab First");var Ew=m("Please check your connection and try again.");m("Sorry, we encountered an error while loading this album.");m("View all");var Fw=m("Are you sure you want to delete this album?");m("Are you sure you want to delete the selected items?");var Gw=m("Next"),Hw=m("Previous"),Iw=m("Are you sure you want to flag this content for review?"),
Jw=m("You must select one of the provided categories.");m("Thank you for reporting this content. Your vigilance helps us improve Web Albums.");var Kw=m("Briefly tell us why you are reporting this content for review:"),Lw=m("This content is otherwise inappropriate."),Mw=m("This content contains nudity."),Nw=m("This content is offensive."),Ow=m("This content promotes violence or is hateful."),Pw=m("Why are you reporting this as inappropriate?"),Qw=m("What does this mean?");m("By submitting this form, you're alerting the Picasa team to inappropriate content on this page.");
var Rw=m("Are you sure you want to remove this from your favorites?"),Sw=m("Receive updates"),Tw=m("(you can always do this later)"),Uw=m("No, Skip this"),Vw=m("Yes"),Ww=m("Now that this user is one of your favorites, you can link to their public gallery from your public gallery."),Xw=m("There was an error communicating with the server. Please check your connection and try again."),Yw=m("Extra-extra large"),Zw=m("Extra large");m("December");m("November");m("October");m("September");m("August");m("July");
m("June");m("May");m("April");m("March");m("February");m("January");m("Blogger albums may not be made public.");var $w=m("Unlisted"),ax=m("Public"),bx=m("(default)"),cx=m("View in Google Earth");m("W");var dx=m("View map");m("Clear Map");var ex=m("Play slideshow"),fx=m("Add To Favorites");m("Download Photos");m("Upload Photos");var gx=m("View All"),hx=m("Submit"),ix=function(a){return"Error: "+a};
var jx=function(a,b,c){return a+"Album: "+b+c};
var kx=function(a){return a+"&deg; N"},
lx=function(a){return a+"&deg; S"},
mx=function(a){return a+"&deg; E"},
nx=function(a){return a+"&deg; W"};
var ox=function(a,b,c){return a+" - "+b+" of "+c};
var px=m("I'm afraid your caption is a bit too long. Please, try to be more brief!"),qx=function(a){return"Your caption currently exceeds "+a+" characters and will not be saved. Please shorten your caption."};
var rx=function(a,b){return"Photo "+a+" of "+b};
var sx=function(a){return a+"px"};
var tx=function(a){return"By "+a},
ux=function(a){return a+"'s Gallery"};
var vx=function(a){return"photos tagged "+a};
var wx=m("Cancel"),xx=m("All rights reserved"),yx=m("Some rights reserved"),zx=m("Do not allow reuse"),Ax=m("Allow reuse"),Bx=m("Allow remixing"),Cx=m("Allow commercial use"),Dx=m("Require share-alike"),Ex=m("An error has occured. Please try again later."),Fx=m("Reset photo to account default"),Gx=m("Set account default: "),Hx=m("Settings");var Ix=m("Google Photos"),Jx=m("Pause"),Kx=m("Resume"),Lx=function(a){var b=m("We are unable to find {$location}.",{location:a});return b},
Mx=function(a){var b=m("Please enter a location in the '{$location}' field.",{location:a});return b},
Nx=function(a){var b=m("{$username} 's Public Gallery",{username:a});return b},
Ox=function(a){var b=m("Sorry, Google does not serve more than 1000 items for any query. (You asked for items starting from {$offset})",{offset:a});return b},
Px=m("Did you mean:"),Qx=m("Find location"),Rx=m("Go"),Sx=m("Map your photos"),Tx=m("Click the map or drag the marker to adjust location."),Ux=m("Map Photos"),Vx=m("Loading&#8230;"),Wx=m("Next"),Xx=m("No"),Yx=m("Previous"),Zx=m("Done"),$x=m("Remove from map"),ay=m("View Album"),by=m("View Drop Box"),cy=m("Zoom in"),dy=m("Zoom out"),ey=m("Yes"),fy=m("All"),gy=m("None"),hy=m("Show"),iy=m("Hide"),jy=m("Select");m("Edit");var ky=m("Save"),ly=m("photo"),my=m("video"),ny=m("album"),oy=m("People in My Photos"),
py=m("People"),qy=m("Edit contact"),ry=m("Correct name tag mistakes"),sy=m("Delete all name tags");m("Google Photos");m("This photo will be available to view and share in Picasa Web Albums, Google's free photo hosting service. Go to: http://picasaweb.google.com to see it now and start sharing!");m("This photo will be available to view and share in Google Photos, Google's free photo hosting service. Go to: http://photos.google.com to see it now and start sharing!");var ty=m("My Photos"),uy=function(a){var b=
m("{$owner}'s Photos",{owner:a});return b},
vy=m("Report inappropriate content"),wy=m("Report issue to owner"),xy=m("Where in the World?"),yy=m("Slideshow"),zy=function(a){var b=m("Show all people in {$name} 's Gallery",{name:a});return b},
Ay=m("By submitting this form, you're alerting the Google Photos team to inappropriate content on this page."),By=m("Thank you for reporting this content. Your vigilance helps us improve Google Photos.");m("Thank you for reporting this content. Your vigilance helps us improve our community.");m("We rely on users like you to keep our community safe for all users and appreciate your taking the time to help the site.");m("Our team will review your report and will take the appropriate action based on our policies. Please note that we will not personally notify you of our decision. If you notice the content is still live on our site within a few days, it is likely we have determined the content does not violate our policies.");
m("Thanks again!");m("Thank you!");m("This content violates another policy. (describe next)");m("This content is violent or promotes illegal activities.");m("This content infringes on my copyright.");m("This content promotes hate against a protected group.");m("(required)");m("(optional)");m("Our policy on content containing nudity.");m("Our policy on content promoting hate against a protected group.");m("Our policy on content that is violent or promotes illegal activities.");m("Our policy on copyright.");
m("We will remove all images of frontal nudity for both men and women, as well as depictions of sex acts.  Images that are considered medical, scientific or educational in nature may be exempt from this policy.");m("We will remove all content that promotes hate against protected groups.  A protected group is distinguished by its race or ethnic origin, religion, disability, gender, age, veteran status, sexual orientation or gender identity. Please note that individuals are not considered a protected group.");
m("We will remove all graphic images of violence or direct threats of violence against any person or group. In addition, we will remove content which promotes or encourages dangerous and illegal activities which could result in harm to the person committing the act or those surrounding him or her.");m("Please select the option that best describes this case.");var Cy=m("Magnified view, click on image and drag to move."),Dy=function(a,b){var c=m("There were no items that matched {$query} found in {$corpusName}.",
{query:a,corpusName:b});return c},
Ey=(function(){var a=m("In order to find more items, consider the following tips:"),b=m("Don't worry about capitalization. Google searches are not case sensitive."),c=m("vacation Hawaii"),d=m("Hawaiian vacation"),e=m("Hawaii"),f=m('Choosing the right search terms is the key to finding the information you need.  Start with the obvious - if you\'re looking for items related to {$searchTerm} try typing "{$searchTerm}".',{searchTerm:"<b>"+e+"</b>"}),g=m("It's often advisable to use multiple search terms. For example, if you're looking for items of a {$searchPhrase}, type \"{$searchTerms}\".",
{searchPhrase:d,searchTerms:"<b>"+c+"</b>"}),h=m("the long and winding road"),i=m('Sometimes you\'ll only want results that include an exact phrase. In this case, simply put quotation marks around your search terms. For example, you could type "{$searchPhrase}".',{searchPhrase:h});return'<div class="lhcl_search_results_no_results">'+Dy('<span class="lhcl_search_results_query">{query}</span>',"{corpusName}")+'<div class="lhcl_search_results_help"><div>'+a+"</div><div><ul><li>"+f+"</li><li>"+g+"</li><li>"+
b+"</li><li>"+i+"</li></ul></div></div>"})(),
Fy=m("People in my photos");var Gy="dialogopen",Hy="dialogclose",Iy=function(a,b){if(za(b))b=new D(b,a);else b.target=b.target||a;return pg(j,b.type,false,b)};function Jy(a,b){var c=u(b);if(c)c.style.visibility=a?"":"hidden"}
function Ky(a,b){return function(c){if(!c)c=window.event;if(c&&!c.target)c.target=c.srcElement;b.call(a,c)}}
function Ly(a,b,c){if(a.addEventListener)a.addEventListener(b,c,false);else if(a.attachEvent)a.attachEvent("on"+b,c)}
function My(a,b,c){if(a.removeEventListener)a.removeEventListener(b,c,false);else if(a.detachEvent)a.detachEvent("on"+b,c)}
function Ny(a){if(window.event){a.cancelBubble=true;a.returnValue=false}else if(a){a.cancelBubble=true;if(a.preventDefault){a.preventDefault();a.stopPropagation()}}}
var Oy=[],Py=0;function Qy(a){pc(Oy,a);if(!Py)Py=window.setInterval(Ry,10)}
function Sy(a){tc(Oy,a);if(Oy.length==0)Ty()}
function Ty(){if(Py)window.clearInterval(Py);Py=0}
function Ry(){var a=Ka();for(var b=0;b<Oy.length;b++)Oy[b](a)}
function Uy(a,b){if(!(a&&b))return;We(a);x(a,b)}
function Vy(){return self.innerHeight||document.documentElement.clientHeight}
function Wy(a){a=a.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;");a=a.replace(/"/g,"&quot;").replace(/'/g,"&#39;");return a}
function Xy(a,b){var c=[],d=0,e=0,f=b||20,g=/[-.,?!\/\\#]/,h=-1;for(var i=0;i<a.length;i++){var k=a.charAt(i);if(k.search(/\s/)==0){d=0;continue}else if(k=="<"){while(k&&k!=">")k=a.charAt(++i);continue}if(d>=f){var n=a.substring(e,i);if(h>=e&&h<i){c.push(n.substring(0,h-e+1));d=i-h-1;e=h+1}else{c.push(a.substring(e,i));d=0;e=i}}if(k=="&")while(k&&k!=";")k=a.charAt(++i);else if(k.search(g)==0)h=i;d++}if(c.length){var q=se?"<wbr />":"&shy;";c.push(a.substring(e));return c.join(q)}else return a}
var Yy,Zy,$y;(function(){var a="(?:(?:(?:\\d{1,3}\\.){3}\\d{1,3})|(?:(?:[a-z0-9\\-]+\\.)+(?:aero|biz|cat|com|coop|edu|gov|info|int|jobs|mil|mobi|museum|name|net|org|pro|travel|[a-z]{2})\\b))",b="a-z0-9/~?&;=%@+$#_\\-",c="/(?:["+b+",:\\.]*["+b+"])?",d="\\b(?:https?://)?"+a+"(?::\\d+)?(?:"+c+")?",e="\\b[a-z0-9_.+\\-]+@"+a;Yy=new RegExp("("+a+")","gi");Zy=new RegExp("("+e+")","gi");$y=new RegExp("("+d+")","gi")})();
function az(a){var b=a.match(Zy);return!(!b)&&b==a}
function bz(a){var b=a.split(/\s/);for(var c=0;c<b.length;c++){var d=b[c];if(d.search(Zy)>=0){d=d.replace(Zy,'<a href="mailto:$1">$1</a>');b[c]=d}else if(d.search($y)>=0){d=d.replace($y,'<a href="http://$1" rel="nofollow">$1</a>');d=d.replace(/http:\/\/(https?:\/\/)/,"$1");b[c]=d}}return b.join(" ")}
function cz(){this.mda=0;this.BC={};this.xU=l(this.zR,this);Ly(window,"unload",this.xU)}
cz.prototype.Hd=function(a){a.__d=this.mda++;this.BC[a.__d]=a};
cz.prototype.ee=function(a){delete this.BC[a.__d]};
cz.prototype.zR=function(){for(var a in this.BC){var b=this.BC[a];if(b&&b.ha)b.ha()}My(window,"unload",this.xU)};
function dz(){Iy(j,"gphotounload");cg();if(_features.geotagging&&typeof GUnload!="undefined")GUnload()}
F(j,"unload",dz);function ez(a,b){var c=a.match(/(\{\w*\})/g);if(!c)return a;for(var d=0;d<c.length;d++){var e=c[d],f=new RegExp(e,"g"),g=e.substring(1,e.length-1);a=a.replace(f,b[g]!=undefined?b[g]:"")}return a}
if("a".replace(/(a)/g,function(){return"b"})=="b")ez=function(a,
b){return a.replace(/\{(\w*)\}/g,function(c,d){return b[d]!=undefined?b[d]:""})};
var fz=w("div");function gz(a,b){fz.innerHTML=ez(a,b);var c=fz.firstChild;if(c)fz.removeChild(fz.firstChild);return c}
var hz={},iz={};function X(a){a=a||"_";var b=hz[a]||0,c=a||"";if(b)c+="_"+b;hz[a]=b+1;return c}
function Y(a,b){iz[a]=b}
function Z(a){if(iz[a])delete iz[a];if(hz[a]==1)hz[a]=0}
function jz(a){var b=[];for(var c=1;c<arguments.length;c++)b.push(arguments[c]);b.push(a);if(iz[a])iz[a].apply(this,b)}
function kz(a){var b=a.split(/\,/);if(b.length!=3)return null;var c=parseInt(b[0],10),d=parseInt(b[1],10),e=parseInt(b[2],10);if(c<1)return null;if(d<1||d>12)return null;var f=new Date(e,d-1,c);return f}
function lz(a,b){window._loadDateTimeConstants();var c=pi(ii);return Gi(c.DATEFORMATS[b],a)}
function mz(a,b){if(b<0||b>3)return a;var c=kz(a);if(c==null)return a;return lz(c,b)}
function nz(a,b,c,d){if(d<0||d>3)return;if(typeof a!="number")return;if(typeof b!="number")return;if(typeof c!="number")return;if(a<1)return;if(b<1||b>12)return;var e=new Date(c,b-1,a);document.write(lz(e,d))}
function oz(){var a=Me("div","lhcl_wrap");r(a,function(b){qf(b,"lhcl_wrap");pz(b)})}
function pz(a){qz(a,0,"",0)}
function qz(a,b,c,d){if(!a)return;if(b<1&&s){a.style.wordWrap="break-word";return}var e=-1;if(b>0){a.innerHTML="g";for(var f=1;f<b;f++)a.innerHTML=a.innerHTML+"<br />g";e=a.offsetHeight;a.innerHTML=c;if(a.offsetWidth<=d&&a.offsetHeight<=e)return}if(!a.hasChildNodes())return;var g=a.firstChild;if(g.nodeValue.length<1)return;var h=0,i=0,k=g.nodeValue.length,n=5,q=a.style.width;if(q&&q.length>0){var t=parseInt(q,10);n=Math.max(5,Math.floor(t/15))}while(h<k){var y=Math.min(g.nodeValue.length,i+n),z=g.nodeValue.substring(i,
y).search(/\s/);if(z>-1||y<k){var O=z>-1?z+1:y,aa=z>-1?z+1:n,dc=g.splitText(O);if(e>0){a.removeChild(dc);if(a.offsetHeight>e){a.innerHTML=a.innerHTML+"&#8230;";h=k}else{var Re=w("wbr");a.appendChild(Re);a.appendChild(dc);g=dc;i=0;h+=aa}}else{var Re=w("wbr");a.insertBefore(Re,dc);g=dc;i=0;h+=aa}}else h=k}if(e>0)while(a.offsetHeight>e&&a.childNodes.length>2){a.removeChild(a.lastChild);a.removeChild(a.lastChild);a.innerHTML=a.innerHTML+"&#8230;"}}
var rz=function(a,b){return Math.max(a.top,b.top)<Math.min(a.bottom,b.bottom)&&Math.max(a.left,b.left)<Math.min(a.right,b.right)},
sz=function(a,b){if(!rz(a,b))return[new ke(a.top,a.left,a.width,a.height)];var c=[],d=a.top,e=a.bottom;if(b.top>a.top){c.push(new ce(a.top,a.right,b.top,a.left));d=b.top}if(b.bottom<a.bottom){c.push(new ce(b.bottom,a.right,a.bottom,a.left));e=b.bottom}if(b.left>a.left)c.push(new ce(d,b.left,e,a.left));if(b.right<a.right)c.push(new ce(d,a.right,e,b.right));return c};
de=function(a,b){return new ce(Math.min(a.top,b.top),Math.max(a.right,b.right),Math.max(a.bottom,b.bottom),Math.min(a.left,b.left))};
function tz(){Oi("_lh_backLink",location.pathname+location.search+location.hash)}
function uz(a,b){var c=b?"PUBLIC_ALBUM_INDEX_ON":"PUBLIC_ALBUM_INDEX_OFF";vz.qc(_setPrefUrl,Ia(wz,a,b),"pref="+c)}
function wz(a,b,c){if(c.status>=400||c.text.indexOf("success")<0){alert(Pu);return}a(b)}
function xz(a,b){var c=u(a);if(typeof c=="undefined")return;if(c.value.length>b)c.value=c.value.substring(0,b)}
function yz(){var a=u("lhid_flash_inter_frame");if(!a){a=gz('<iframe src="javascript:void 0" style="position:absolute; display: none; z-index:5;" id="lhid_flash_inter_frame" frameborder="0" scrolling="no"></iframe>',{});if(document.body.firstChild)document.body.insertBefore(a,document.body.firstChild);else document.body.F(a)}}
function zz(a){var b=u("lhid_flash_inter_frame");if(b){if(document.body.firstChild!=b){document.body.removeChild(b);document.body.insertBefore(b,document.body.firstChild)}sh(b,uh(a));zh(b,Ah(a));M(b,true)}}
function Az(){var a=u("lhid_flash_inter_frame");if(a)M(a,false)}
;var Cz=function(a,b){if(!Bz){gi(_LH.locale);if(!mi(ji,hi()))oi(qi,hi());Bz=true}return Ji(a,b)},
Bz=false;;function Dz(a){Ez.Hd(this);var b=a||2;this.ye=[];for(var c=0;c<b;c++)this.ye[c]=null;this.Dd=[];this.Do=1;if(typeof XMLHttpRequest!="undefined")this.gq=this.tS;else if(typeof window.ActiveXObject!="undefined")for(var c=0;c<Fz.length;c++){var d=Fz[c];try{var e=new ActiveXObject(d);if(!e)continue;this.gq=function(){return new ActiveXObject(d)};
break}catch(f){}}}
var Fz=["MSXML2.XMLHTTP.6.0","MSXML2.XMLHTTP.3.0","MSXML2.XMLHTTP","MICROSOFT.XMLHTTP"];Dz.prototype.ha=function(){Ez.ee(this);this.gq=null;while(this.ye.length){var a=this.ye.pop();if(a&&a.readyState<4)a.abort()}for(var b in this)this[b]=null};
Dz.prototype.Gs=function(){return!(!this.gq)};
Dz.prototype.qc=function(a,b,c,d){if(!this.gq){if(b)b(null);return 0}var e={id:this.Do++,url:a,cb:b,data:c};if(c||d)this.Dd.push(e);else this.Dd.unshift(e);this.zy();return e.id};
Dz.prototype.mF=function(a){for(var b=0;b<this.ye.length;b++)if(this.ye[b]&&this.ye[b].id==a){if(this.ye.xhr)this.ye.xhr.abort();this.ye.splice(b,1,null);this.zy();return}this.tR(a)};
Dz.prototype.tR=function(a){for(var b=0;b<this.Dd.length;b++)if(this.Dd[b].id==a){this.Dd.splice(b,1);return}};
Dz.prototype.HR=function(a){if(a.status>=400)return null;try{return eval("("+a.text+")")}catch(b){return null}};
Dz.prototype.tS=function(){return new XMLHttpRequest};
Dz.prototype.zy=function(){if(!(this.Dd.length&&this.gq))return;var a=cc(this.ye,null);if(a<0)return;var b=this.Dd.pop(),c=this.gq();c.onreadystatechange=l(this.VR,this,a,b.cb);this.ye[a]={id:b.id,xhr:c};c.open(b.data?"post":"get",b.url,true);if(b.data){c.setRequestHeader("Content-Type","application/x-www-form-urlencoded");c.setRequestHeader("Content-Length",b.data.length)}c.send(b.data)};
Dz.prototype.VR=function(a,b){if(this.ye.length<=a||!this.ye[a])return;var c=this.ye[a].xhr;if(!c||c.readyState!=4)return;if(b){var d={status:c.status,xml:c.responseXML,text:c.responseText};b(d)}delete this.ye[a];this.ye[a]=null;this.zy()};;var Jz=function(a,b,c){Gz(true,a);if(b)Hz=Rg(l(Iz,null,c),b)},
Iz=function(a){Kz();if(a)a()},
Kz=function(a){var b=a?"":null;Gz(false,b)},
Gz=function(a,b){Lz();var c=u("lhid_feedback"),d=u("lhid_notifyMsg");if(c&&d){M(c,a);if(b)B(d,b)}},
Lz=function(){if(Hz!=0){Sg(Hz);Hz=0}},
Hz=0;var Mz=function(a){this.Yd=false;this.Rca=a;this.yG=[]},
Nz;Mz.prototype.k3=function(a){if(typeof GMap2!="undefined"){if(a)a()}else{if(a)this.yG.push(a);if(!this.Yd){this.Yd=true;Rg(function(){var b=w("script");b.src="http://maps.google.com/maps?file=api&v=2&client=google-pweb&async=2&callback=pwa.mapLoaded&oe=utf-8&hl="+this.Rca;x(Ke().body,b)},
0,this)}}};
Mz.prototype.nT=function(){Iy(j,"mapload");while(this.yG.length>0){var a=this.yG.pop();a()}};
var Oz=function(a){if(Nz)Nz.k3(a)},
Pz=function(){if(Nz)Nz.nT()},
Qz=function(a){Nz=new Mz(a)};function Rz(a){this.ka=a;this.ud=l(this.bX,this)}
Rz.prototype.St=false;Rz.prototype.rw=1;Rz.prototype.FF=0;Rz.prototype.Cy=0;Rz.prototype.Gz=0;Rz.prototype.tE=0;Rz.prototype.Ox=0;Rz.prototype.ZH=0;Rz.prototype.stop=function(){if(this.St)Sy(this.ud);this.St=false};
Rz.prototype.Ro=function(a){this.rw=pe(a,0,1);Ch(this.ka,se?Math.min(this.rw,0.99):this.opacity);L(this.ka,"visibility",this.rw?"":"hidden")};
Rz.prototype.oi=function(a,b){a=pe(a,0,1);var c=this.rw;this.FF=c;this.Cy=a;this.Gz=a-c;var d=Ka();this.tE=d;this.Ox=d+Math.abs(this.Gz)*b;this.ZH=this.Ox-d;if(this.Ox<=this.tE){this.stop();this.Ro(this.Cy)}else{if(!this.St)Qy(this.ud);this.St=true}};
Rz.prototype.bX=function(a){var b=this.Cy;if(a<this.Ox){var c=(a-this.tE)/this.ZH;b=c*this.Gz+this.FF}else this.stop();this.Ro(b)};var Sz=function(a,b){this.ik=a;this.te=b?"tok="+b:null},
Tz=null,Uz=function(){if(!Tz)throw Error("Logger has not been initalized");return Tz};
Sz.prototype.log=function(a,b){var c=["page="+a];for(var d in b)c.push(d+"="+ab(b[d]));Gj(this.ik+"?"+c.join("&"),null,this.te?"POST":"GET",this.te)};
var Vz=function(a,b){Tz=new Sz(a,b)};var Wz=function(){if(re||ue)return false;if(ve)return Ee("412");if(s)return Ee("6");if(se)return Ee("1.8");return true};;;{var Xz=function(a,b){P.call(this);this.I=[null,null];this.wM=a||0;this.Pea=b||false;this.j=new K(this)};
o(Xz,P);Xz.prototype.xn=false;Xz.prototype.mA=false;Xz.prototype.D=function(){return"gphoto.ScaledImage"};
Xz.prototype.kb=function(){return false};
Xz.prototype.m=function(){var a=w("div");L(a,"position","relative");L(a,"overflow","hidden");r(this.I,function(b,c,d){var e=w("img");L(e,"position","absolute");M(e,false);x(a,e);d[c]=e},
this);this.xs=this.I.length;this.od(a)};
Xz.prototype.EV=function(){var a=this.na().videostatus;if(this.na().Gb()&&(!a||a=="ready"||a=="final")){var b=_LH.locale;if(b!="en_US")b+="&messagesUrl="+ab("http://video.google.com/FlashUiStrings.xlb?frame=flashstrings&hl="+_LH.locale);var c=bb(this.k.vsurl),d="http://video.google.com/googleplayer.swf?videoUrl="+ab(c)+"&hl="+b+"&autoplay=yes",e=w("embed");L(e,"position","absolute");var f=this.Pea?"mini":"normal";e.setAttribute("FlashVars","playerMode="+f);e.setAttribute("salign","TL");e.setAttribute("src",
d);e.setAttribute("scale","noScale");e.setAttribute("bgcolor","#eeeeee");e.setAttribute("quality","best");e.setAttribute("swLiveConnect","true");e.setAttribute("allowscriptaccess","always");e.setAttribute("width",this.k.w);e.setAttribute("height",this.k.h);e.setAttribute("type","application/x-shockwave-flash");M(e,false);yz();x(this.e,e);this.I[this.xs]=e;this.Ef(this.k);M(this.I[this.xs],true)}};
Xz.prototype.C6=function(){if(this.I[this.xs]){M(this.I[this.xs],false);A(this.I[this.xs]);delete this.I[this.xs]}};
Xz.prototype.b=function(){if(!this.W()){Xz.f.b.call(this);this.j.b()}};
Xz.prototype.n=function(){Xz.f.n.call(this);this.xn=false;this.ja={available:null,actualSize:null,active:false,inDrag:false,elt:null,minX:0,minY:0,curX:0,curY:0,xOff:0,yOff:0};if(this.Ia()&&this.Ia().Ia()){this.j.g(this.Ia().Ia().Ia(),"zoomin",this.zoomIn);this.j.g(this.Ia().Ia().Ia(),"zoomout",this.zoomOut)}this.aa()};
Xz.prototype.u=function(){Xz.f.u.call(this);this.j.ma();if(this.e)A(this.e);this.e=null;r(this.I,function(a,b,c){c[b]=null})};
Xz.prototype.U=function(a){if(this.na()!=a){Xz.f.U.call(this,a);if(this.Ha()){this.xn=false;this.aa()}}};
Xz.prototype.jP=function(a,b){b=a>0&&!(!b);if(a!=this.Bd||b!=this.mA){this.Bd=a;this.mA=!(!b);if(this.Ha())this.aa()}};
Xz.prototype.zJ=function(a){var b=this.cu(a);return gs(a,b)};
Xz.prototype.cu=function(a){var b=this.Ha()?this.e.offsetWidth:0;return new ne(Math.max(b,this.wM),this.Bd?this.Bd:a.height)};
Xz.prototype.Le=function(){return this.I&&this.I[0]};
Xz.prototype.aa=function(){if(this.k){var a=this.XM||this.k,b=new ne(a.w,a.h),c=this.cu(b),d=gs(b,c);if(this.XM!=this.k){b=new ne(this.k.w,this.k.h);var c=this.cu(b);d=gs(b,c)}var e=at(a.videostatus,""),f=false;if(!e||a.videostatus=="ready")e=this.k.Rf();else f=true;var g=ts(e,Math.max(d.width,d.height));if(this.XM!=this.k||g<0){if(g<0)g=144;if(f)g=0;rs(e,b,g,false,l(this.H0,this))}else{var h=oe(b,d)?0:Math.max(d.width,d.height);this.If(e,h)}}else this.L1();if(this.ja.active){this.XQ();this.ja.inDrag=
false}};
Xz.prototype.eE=function(a){this.C6();var b=!(!this.k.vsurl);if(b&&a)this.EV()};
Xz.prototype.L1=function(){r(this.I,function(a){if(a)M(a,false)})};
Xz.prototype.H0=function(a){rs(a.baseUrl,null,a.imgMax,a.crop,l(this.GK,this),null,this.I[1])};
Xz.prototype.GK=function(a){this.XM=this.k;M(this.I[0],false);M(this.I[1],true);A(this.I[0]);A(this.I[1]);this.Ef(this.k);x(this.e,this.I[0]);x(this.e,this.I[1]);rs(a.baseUrl,null,a.imgMax,a.crop,l(this.c0,this),null,this.I[0])};
Xz.prototype.c0=function(){M(this.I[0],false);M(this.I[1],true);var a=this.na().Gb();if(a)this.eE(true);else this.eE(false);this.dispatchEvent("load");if(!a)this.aa()};
Xz.prototype.If=function(a,b){var c=cs(a,b);M(this.I[0],true);if(this.I[0].src==c)M(this.I[1],false);else if(this.I[1].src!=c){if(this.I[1].src!=this.I[0].src)this.I[1].src=this.I[0].src;M(this.I[1],true);rs(a,null,b,false,l(this.GK,this),null,this.I[1])}};
Xz.prototype.Ef=function(a){var b=new ne(a.w,a.h),c=this.cu(b),d=gs(b,c);if(!this.mA)c.height=d.height;this.ja.available=c;var e=new ge((c.width-d.width)/2,(c.height-d.height)/2);if(this.Ha()){r(this.I,function(f){zh(f,d);sh(f,e)});
zh(this.A(),c.width<this.wM?this.wM:"",c.height)}return d};
Xz.prototype.XQ=function(){var a=this.ja.available.width,b=Math.min(this.ja.available.height,this.ja.actualSize.height),c=0;if(a>this.ja.actualSize.width){c=Math.floor((a-this.ja.actualSize.width)/2);a=this.ja.actualSize.width}zh(this.ja.elt,new ne(a,b));sh(this.ja.elt,new ge(c,0));this.ja.minX=a-this.ja.actualSize.width;this.ja.minY=b-this.ja.actualSize.height;this.ja.curX=0;this.ja.curY=0;L(this.ja.elt,"background-position","0 0")};
Xz.prototype.zoomOut=function(){if(this.ja.active){this.j.Sa(this.ja.elt,I,this.YP);this.j.Sa(this.ja.elt,fg,this.WH);this.j.Sa(this.ja.elt,eg,this.wn);this.j.Sa(this.ja.elt,Hf,this.wn);this.j.Sa(window,Hf,this.wn);this.e.removeChild(this.ja.elt);this.ja.active=false}};
Xz.prototype.zoomIn=function(){if(!this.$T())return;if(!this.ja.active){this.ja.active=true;var a=bc(fs),b=new ne(a,a);this.ja.actualSize=new ne(this.k.w,this.k.h);if(!this.ja.actualSize.Yp(b))this.ja.actualSize=this.ja.actualSize.uD(b);this.ja.elt=w("div");this.XQ();L(this.ja.elt,"background-image","url("+this.k.Rf()+")");L(this.ja.elt,"position","absolute");L(this.ja.elt,"z-index","1000");x(this.e,this.ja.elt);this.j.g(this.ja.elt,I,this.YP);this.j.g(this.ja.elt,fg,this.WH);this.j.g(this.ja.elt,
eg,this.wn);this.j.g(this.ja.elt,Hf,this.wn);this.j.g(window,Hf,this.wn)}};
Xz.prototype.YP=function(a){this.ja.xOff=a.offsetX;this.ja.yOff=a.offsetY;this.ja.inDrag=true;a.preventDefault()};
Xz.prototype.WH=function(a){if(this.ja.inDrag){var b=pe(this.ja.curX+a.offsetX-this.ja.xOff,this.ja.minX,0),c=pe(this.ja.curY+a.offsetY-this.ja.yOff,this.ja.minY,0);this.ja.xOff=a.offsetX;this.ja.yOff=a.offsetY;this.ja.curX=b;this.ja.curY=c;L(this.ja.elt,"background-position",b+"px "+c+"px")}};
Xz.prototype.wn=function(){this.ja.inDrag=false};
Xz.prototype.$T=function(){return true}};;var aA=function(a){Ez.Hd(this);this.P=a;var b={lhf_prev:X("Previous"),lhf_next:X("Next"),lhf_exit:X("Exit"),lhf_play:X("Play"),lhf_stop:X("Pause"),lhf_fast:X("Faster"),lhf_slow:X("Slower"),lhf_show:X("ToggleCaptions")};Y(b.lhf_prev,l(a.Yh,a));Y(b.lhf_next,l(a.Xh,a));Y(b.lhf_exit,l(a.hp,a));Y(b.lhf_play,l(a.uF,a));Y(b.lhf_stop,l(a.BF,a));Y(b.lhf_fast,l(a.jS,a));Y(b.lhf_slow,l(a.iS,a));Y(b.lhf_show,l(this.mS,this));this.zba=b;this.Su=new Ym(this.Es,0,this);var c=gz(Yz,{});M(c,false);x(document.body,
c);this.Iv=c;this.Qca=new Ym(this.fS,500,this);var d=new Zz(document.body,"lhcl_slideshow_feedback");this.xI=d;this.wI=new Ym(this.rF,2000,this);this.lA=new Rz(d.div);this.lA.Ro(0);var e=w("div");x(document.body,e);e.className="lhcl_slideshow_container";this.vba=e;var f=new Zz(e,"lhcl_slideshow_caption");this.dU=f;this.AG=new Rz(f.contents);this.AG.Ro(0);var g=new Zz(e,"lhcl_slideshow_controls");this.LU=g;this.mt=new Rz(g.contents);this.mt.Ro(0);if(!s){Ch(f.bg,0.7);Ch(g.bg,0.7);Ch(d.bg,0.7)}var h=
w("table");x(g.fg,h);var i=w("tbody");x(h,i);var k=w("tr");x(i,k);var n=w("td");x(k,n);this.Ffa=new $z(n,"img/slideshowprev.gif","img/slideshowprev_on.gif",'javascript:_d("'+b.lhf_prev+'")');var q=w("td");x(k,q);this.wea=new $z(q,"img/slideshowpause.gif","img/slideshowpause_on.gif",'javascript:_d("'+b.lhf_stop+'")');this.kk=new $z(q,"img/slideshowplay.gif","img/slideshowplay_on.gif",'javascript:_d("'+b.lhf_play+'")');this.kk.gj(false);var t=w("td");x(k,t);this.Afa=new $z(t,"img/slideshownext.gif",
"img/slideshownext_on.gif",'javascript:_d("'+b.lhf_next+'")');var y=w("td");x(k,y);this.pfa=new $z(y,"img/slideshowminus.gif","img/slideshowminus_on.gif",'javascript:_d("'+b.lhf_fast+'")');this.H$=w("td");x(k,this.H$);var z=w("td");x(k,z);this.Pfa=new $z(z,"img/slideshowplus.gif","img/slideshowplus_on.gif",'javascript:_d("'+b.lhf_slow+'")');var O=w("td");x(k,O);this.aE=w("a");this.aE.href='javascript:_d("'+b.lhf_show+'")';x(O,this.aE);B(this.aE,hv);var aa=w("td");x(k,aa);this.ofa=new $z(aa,"img/slideshowexit.gif",
"img/slideshowexit_on.gif",'javascript:_d("'+b.lhf_exit+'")');this.gda=F(document,fg,this.QR,false,this)};
aA.prototype.Fx=true;aA.prototype.Zd=true;aA.prototype.ha=function(){Ez.ee(this);H(this.gda);this.wI.b();this.Su.b();this.Qca.b();this.mt.stop();this.AG.stop();this.lA.stop();var a=this.zba;if(a){Z(a.lhf_prev);Z(a.lhf_next);Z(a.lhf_exit);Z(a.lhf_play);Z(a.lhf_stop);Z(a.lhf_fast);Z(a.lhf_slow);Z(a.lhf_show)}this.dU.ha();this.LU.ha();this.xI.ha();A(this.Iv);A(this.vba)};
aA.prototype.gj=function(){this.Su.stop();this.mt.oi(1,300);if(!this.n4)this.ty(3000)};
aA.prototype.ty=function(a){this.Su.stop();if(a)this.Su.start(a);else this.Es()};
aA.prototype.cS=function(a){if(!a&&!this.Zd){this.rF();this.wI.stop();return}if(a==this.Zd)return;this.wea.gj(a);this.kk.gj(!a);var b=ev;if(a){this.ty();b=fv}else this.gj();B(this.xI.fg,b);zz(this.xI.div);this.lA.oi(1,100);this.wI.start();this.Zd=a};
aA.prototype.rF=function(){Az();this.lA.oi(0,300)};
aA.prototype.yy=function(a){var b=a?1:0;this.AG.oi(b,300)};
aA.prototype.yF=function(a){this.pb=a;if(!this.Fx)return;if(a){a=Xy(a);this.dU.fg.innerHTML=a;this.yy(true)}else this.yy(false)};
aA.prototype.mS=function(){this.Fx=!this.Fx;var a=gv;if(this.Fx){this.yF(this.pb);a=hv}else if(this.pb)if(this.pb)this.yy(false);B(this.aE,a)};
aA.prototype.Hs=function(a){var b=a/1000;B(this.H$,iv+": "+b);this.gj()};
aA.prototype.fS=function(){M(this.Iv,true)};
aA.prototype.Es=function(){if(this.mt)this.mt.oi(0,300)};
aA.prototype.QR=function(a){if(a.clientX==this.hF&&a.clientY==this.kR)return;this.gj();this.hF=a.clientX;this.kR=a.clientY;this.n4=false;for(var b=a.target;b!=null;b=b.parentNode)if(b==this.LU.div){this.Su.stop();this.n4=true;break}};
var Zz=function(a,b){Ez.Hd(this);var c={fg:X("fg"),bg:X("bg"),outer:X("table"),inner:X("inner"),className:b};this.ka=gz('<table id="{outer}" class="{className}"><tr><td><div id="{inner}" style="position:relative;height:100%"><div id="{bg}" class="{className}_bg" style="position:absolute;z-index:2;width:100%;height:100%"></div><div id="{fg}" class="{className}_fg" style="position:relative;z-index:3;width:100%"></div></div></td></tr></table>',c);x(a,this.ka);this.contents=u(c.inner);this.div=u(c.outer);
this.fg=u(c.fg);this.bg=u(c.bg);if(s)this.fg.onresize=l(this.$R,this)};
Zz.prototype.ha=function(){Ez.ee(this);if(this.fg)this.fg.onresize=null;A(this.ka)};
Zz.prototype.$R=function(){L(this.bg,"height",this.fg.offsetHeight-1+"px")};
var $z=function(a,b,c,d){Ez.Hd(this);var e={anchor:X("anchor"),off_id:X("button"),off_src:b,on_id:X("buttonOn"),on_src:c,mouseover:X("mouseover"),mousedown:X("mousedown"),mouseout:X("mouseout"),mouseup:X("mouseup")};this.r=e;this.ka=gz('<table><tr><td><a id="{anchor}" onmouseover="_d(\'{mouseover}\')" onmouseout="_d(\'{mouseout}\')" onmousedown="_d(\'{mousedown}\')" onmouseup="_d(\'{mouseup}\')"><img id="{on_id}" src="{on_src}" style="position:absolute;" ondragstart="return false" /><img id="{off_id}" src="{off_src}" onmousedown="return false" /></a></td></tr></table>',
e);x(a,this.ka);this.Uaa=u(e.anchor);this.Uaa.href=d;this.pw=u(e.on_id);if(s)L(this.pw,"visibility","hidden");else{this.po=new Rz(this.pw);this.po.Ro(0)}Y(e.mouseover,l(this.SR,this));Y(e.mouseout,l(this.RR,this));Y(e.mousedown,l(this.PR,this));Y(e.mouseup,l(this.UR,this))};
$z.prototype.ha=function(){Ez.ee(this);var a=this.r;Z(a.mouseover);Z(a.mouseout);Z(a.mousedown);Z(a.mouseup);if(this.po)this.po.stop()};
$z.prototype.gj=function(a){M(this.ka,a)};
$z.prototype.SR=function(){if(!s)this.po.oi(0.5,200)};
$z.prototype.RR=function(){if(s)L(this.pw,"visibility","hidden");else this.po.oi(0,200)};
$z.prototype.PR=function(){if(s)L(this.pw,"visibility","");else this.po.oi(1,0)};
$z.prototype.UR=function(){if(s)L(this.pw,"visibility","hidden");else this.po.oi(0.5,0)};
var Yz='<div class="lhcl_slideshow_loading">'+Vx+"</div>";function bA(a,b,c){this.P=a;this.ad=_photo;this.x7=!(!c);this.p=new K(this);if(cA&&_features.fr){this.B=rr();this.B.client.CP("uname",_user.name);var d=new qr(_user.name,_album.id,_album.title,_album.name,-1);this.B.client.JF(d);this.B.client.UO(d);this.B.combined.co(_authuser.isOwner?null:_user.name)}this.r={albumlink:_album.link,albumlinkID:X("lhid_albumlink"),prevboxID:X("prevbox"),nextboxID:X("nextbox"),zoomID:X("zoom"),rotateID:X("rotate"),zoomTextID:X("zoomtext"),toolID:X("tools"),indextextID:X("indextext"),
emailID:X("email"),returnText:_album.isdefault?by:ay};var e=this.x7?dA:eA;this.ka=gz(e,this.r);b.appendChild(this.ka);if(!_authuser.user)M(u(this.r.emailID),false);if(_authuser.isOwner&&!this.x7){this.Uh={rotateFunc:X("func"),leftID:X("leftrotate"),rightID:X("rightrotate"),leftOn:"img/rotateleft_normal.gif",leftOff:"img/rotateleft_ghosted.gif",rightOn:"img/rotateright_normal.gif",rightOff:"img/rotateright_ghosted.gif"};u(this.r.toolID).innerHTML=ez(fA,this.Uh);Y(this.Uh.rotateFunc,l(this.WR,this))}Y(this.r.zoomID,
Ky(this.P,this.P.lp));this.Pm(false);this.zoom=false;this.VC=X("prev");this.yC=X("next");var f={src:"img/left_normal.gif",id:this.VC,alt:Yx},g={src:"img/right_normal.gif",id:this.yC,alt:Wx},h=gz(gA,f),i=gz(gA,g);h.style.visibility="hidden";i.style.visibility="hidden";u(this.r.prevboxID).appendChild(h);u(this.r.nextboxID).appendChild(i);Y(this.VC,l(this.sF,this,-1));Y(this.yC,l(this.sF,this,1));this.Om(true);this.Ifa=l(this.xy,this)}
var hA=m("Share Photo");bA.prototype.ha=function(){if(this.ka)A(this.ka);Z(this.r.zoomID);Z(this.VC);Z(this.yC);if(this.Uh)Z(this.Uh.rotateID);this.p.b();for(var a in this)this[a]=null};
bA.prototype.Om=function(a){var b=a||!this.P.ry();if(this.Uh){var c=u(this.Uh.leftID),d=u(this.Uh.rightID);if(b){c.src=this.Uh.leftOff;d.src=this.Uh.rightOff;qf(c,"lhcl_fakelink");qf(d,"lhcl_fakelink")}else{c.src=this.Uh.leftOn;d.src=this.Uh.rightOn;C(c,"lhcl_fakelink");C(d,"lhcl_fakelink")}this.Pba=b}};
bA.prototype.oS=function(a,b){var c=u(this.VC),d=u(this.yC);if(a){c.style.visibility="";c.href=location.href;c.hash=a}else{c.style.visibility="hidden";c.blur()}if(b){d.style.visibility="";d.href=location.href;d.hash=b}else{d.style.visibility="hidden";d.blur()}};
bA.prototype.Mk=function(a){this.hx(this.ad[a])};
bA.prototype.hx=function(a){this.Pm(false);this.ba=a;this.Yf=this.ba.id;var b=a.user?a.user.name:_user.name,c=["uname="+b,"iid="+this.Yf],d=(new N(document.location.href)).Ga("authkey");if(typeof d!="undefined")c.push("authkey="+d);var e="/lh/emailPhoto?"+c.join("&");u(this.r.emailID).href=e;var f=m("Share Video");u(this.r.emailID+"_t").innerHTML=this.ba.vsurl?f:hA;var g=u(this.r.albumlinkID);if(g)g.href=this.r.albumlink;this.Om(!(!this.ba.vsurl));if(cA&&_features.fr&&_features.froptin)this.ME()};
bA.prototype.Pm=function(a,b){var c=this.zoom!=a,d=u(this.r.zoomID),e=u(this.r.zoomTextID);if(b){a=false;d.src="img/zoom_ghosted.gif";d.title=""}else{d.src=a?"img/zoom_active.gif":"img/zoom_normal.gif";d.title=a?dy:cy}e.style.display=a?"":"none";d.className=b?"":"lhcl_fakelink";this.zoom=a;if(cA&&_features.fr&&_features.froptin&&c)this.ME()};
bA.prototype.WR=function(a){if(this.Pba||!this.P.vy())return;var b=this.Yf,c=_album.id,d=_user.name,e=this.ba.album;if(this.ba&&e&&this.ba.user){b=this.ba.id;c=e.id;d=this.ba.user.name}var f=["selectedphotos="+b,"optgt="+a,"uname="+d,"aid="+c,"noredir=true"];this.Pw=b;var g=_selectedPhotosPath+"&"+f.join("&");Gj(g,l(bA.prototype.xy,this),"POST")};
bA.prototype.xy=function(a){if(this.P.rd(a)||this.P.de(a)){this.P.Ay();return}this.P.Lk(this.Pw,true);if(cA&&_features.fr&&_features.froptin)Rg(function(){this.ME(true)},
5000,this)};
bA.prototype.sF=function(a,b){if(b==0)this.mouseups++;else if(b==1||b==2&&this.mouseups>=2){this.P.jp(a);this.mouseups=0}};
bA.prototype.ME=function(a){if(cA&&_features.fr&&_features.froptin&&(_authuser.isOwner||_album.isnametagsvisible)&&this.P instanceof iA)jA(this,this.P.Le(),this.Yf,_album.id,!this.zoom,_authuser.isOwner?"editor":kA,a)};
var eA='<div><table class="lhcl_photonav"><tr><td width="50%"><table width="100%"><tr><td width="100%"><span class="lhcl_albumlink" ><a id="{albumlinkID}" href="{albumlink}">&laquo; {returnText}</a></span></td><td><div class="lhcl_photoArrowBox" id="{prevboxID}" style="background-image:url(img/left_ghosted.gif)"></div></td></tr></table></td><td><table width="100%" class="lhcl_editbuttons"><tr><td><div class="lhcl_photoArrowBox" id="{nextboxID}" style="background-image:url(img/right_ghosted.gif)"></div></td><td width="100%"></td><td class="lhcl_toolspace"><span id="{toolID}"></span></td><td><span><a class="lhcl_nounderline" id="{emailID}"><div class="lhcl_cbbox lhcl_cbox_disabled"><p class="lhcl_cbdesc"></p><p class="lhcl_cblink"><em><img src="img/share_icon.gif"  /> <span id="{emailID}_t">'+
hA+'</span></em></p></div></a></span></td><td class="lhcl_toolspace"><span><img src="img/zoom_normal.gif" title="Zoom In" class="lhcl_fakelink"id="{zoomID}" onclick="_d(\'{zoomID}\')" /></span></td></tr></table></td></tr></table><div class="lhcl_zoomtext" id="{zoomTextID}" style="display:none">'+Cy+"</div></div>",gA='<a href="" id="{id}" onclick="return false"><img src="{src}" title="{alt}" onmouseup="_d(\'{id}\', 0); return false" onmousedown="_d(\'{id}\', 1); return false" ondblclick="_d(\'{id}\', 2); return false" /></a>',
fA=(function(){var a=m("Rotate Right"),b=m("Rotate Left");return'<img id="{leftID}" src="{leftOff}" title="'+b+'" class="lhcl_fakelink" onclick="_d(\'{rotateFunc}\', \'ROTATE270\')" /><img id="{rightID}" src="{rightOff}" title="'+a+'" class="lhcl_fakelink" onclick="_d(\'{rotateFunc}\', \'ROTATE90\')" />'})(),
dA='<div><table class="lhcl_photonav"><tr><td width="50%"><table width="100%"><tr><td width="100%"></td><td><div class="lhcl_photoArrowBox" id="{prevboxID}" style="background-image:url(img/left_ghosted.gif)"></div></td></tr></table></td><td><table width="100%" class="lhcl_editbuttons"><tr><td id="{lhui_indexbox}"><span><b id="{indextextID}">'+Vx+'</b></span></td><td><div class="lhcl_photoArrowBox" id="{nextboxID}" style="background-image:url(img/right_ghosted.gif)"></div></td><td width="100%"></td><td class="lhcl_toolspace"><span id="{toolID}"></span></td><td><span><a class="lhcl_nounderline" id="{emailID}"><div class="lhcl_cbbox lhcl_cbox_disabled"><p class="lhcl_cbdesc"></p><p class="lhcl_cblink"><em><img src="img/share_icon.gif"  /> <span id="{emailID}_t">'+
hA+'</span></em></p></div></a><span></td><td class="lhcl_toolspace"><span><img src="img/zoom_normal.gif" title="Zoom In" class="lhcl_fakelink"id="{zoomID}" onclick="_d(\'{zoomID}\')" /></span></td></tr></table></td></tr></table><div class="lhcl_zoomtext" id="{zoomTextID}" style="display:none">'+Cy+"</div></div>";function lA(a,b,c,d,e,f){this.P=a;this.ad=c?c:_photo;this.eea=e;var g={lhui_albumtitle:Xy(_album.title),lhui_indexbox:X("indexbox"),lhui_infoheader:X("infoheader"),lhui_infobox:X("infobox"),lhui_exifbox:X("exifbox"),lhui_downloadlink:X("download"),lhui_albumlink:X("lhid_viewalbum"),lhui_slideshowlink:X("slideshow"),lhui_togglelink:X("toggle"),lhui_links:X("links")};g.lhui_slideshow_link_markup=d?"":ez(mA,g);var h=_features.googlephotos?nA:(this.eea?oA:pA);g.flash_str=_features.newStrings?Rv:Sv;this.e=
gz(h,g);b.appendChild(this.e);this.g2=u(g.lhui_indexbox);this.k2=u(g.lhui_infoheader);this.j2=u(g.lhui_infobox);this.jl=u(g.lhui_exifbox);this.Tz=u(g.lhui_downloadlink);this.VF=u(g.lhui_albumlink);this.W9=d?null:u(g.lhui_slideshowlink);if(f)M(u(g.lhui_links),false);this.Hea=u(g.lhui_togglelink);if(_features.cart&&!_features.googlephotos&&pa(_updateCartPath)&&(e||_album.prints)){this.Qk=w("span");u(g.lhui_links).appendChild(this.Qk);this.cartMsgId=X("cartmsg");var i=gz('<div id="{cartMsgId}" class="lhcl_output"></div>',
this);u(g.lhui_links).appendChild(i);this.Rda=l(this.CF,this,false);this.Waa=l(this.CF,this,true);this.qba=l(this.vR,this);this.rba=l(this.wR,this);var k=Pi("_cart1","");if(k.length){var n="/lh/listCart";if(!e)n+="?uname="+_user.name;vz.qc(n,this.rba)}}this.sQ=g.lhui_togglelink;Y(this.sQ,l(this.nS,this));this.IP=false;var q=document.getElementById("hdf_profile");if(q)q.innerHTML+='<div id="debug_meta_rss"></div>';if(_features.newStrings)qA.push(["flash",Wv]);if(_features.geotagging)qA.push(["latitude",
Yv],["longitude",Xv])}
lA.prototype.ha=function(){if(this.e)A(this.e);if(cA&&this.icon)this.icon.close();if(this.sQ)Z(this.sQ);for(var a in this)this[a]=null};
lA.prototype.Mk=function(a){this.oa=a;var b=this.ad[a];if(!b)return;b.album=b.album||_album;if(!pa(b.oa))b.oa=a;this.hx(b)};
lA.prototype.LJ=function(){return(this.Ge?this.Ge.album:null)||_album};
lA.prototype.lZ=function(){return this.Ge?this.Ge.user:_user};
lA.prototype.hx=function(a){this.Ge=a;if(_photo&&_photo.length&&this.g2)this.g2.innerHTML=rx(a.oa+1,_photo.length);this.fe(a.r||null);if(a.vsurl){this.Tz.innerHTML="";u("lhid_flashMsg").style.display=rA?"none":""}else{this.Tz.href=a.s+"?imgdl=1";B(this.Tz,iw);u("lhid_flashMsg").style.display="none"}M(this.Tz,a.dl);var b=a.album;if(this.VF&&b){B(this.VF,ay);this.VF.href=b.link}if(this.W9)this.W9.href=location.href.replace(/#/,"#s");if(_features.cart&&!_features.googlephotos&&this.Qk)this.By(a,false)};
lA.prototype.fe=function(a){if(!a||!a.exif){if(!this.Iv)this.Iv=gz(sA,{});Uy(this.j2,this.Iv);this.DF(null);return}var b=u("debug_meta_rss");if(b)b.innerHTML=ez('<a href="/lh/rssPhoto?uname={username}&amp;iid={id}&js=1">RSS</a><br />',{username:_user.name,id:a.id});a.datestring=a.exif.time||a.date;var c=this.lZ();a.username=c?c.name:Vx;var d=this.LJ();a.albumname=d?d.name:Vx;a.sizestring=a.width+"&times;"+a.height+" pixels &#8211; "+(a.kbytes+"KB");this.j2.innerHTML=ez('<div class="lhcl_photoinfo">{datestring}<br />{sizestring}<br /></div>',
a);a.exif.title=a.title;if(this.k2)this.k2.innerHTML=a.video?tA:uA;if(a.lat&&a.lon){a.exif.latitude=a.lat;a.exif.longitude=a.lon}else{a.exif.latitude=null;a.exif.longitude=null}this.DF(a.exif)};
lA.prototype.DF=function(a){if(a){if(a.exposure){var b=parseFloat(a.exposure);if(b&&b<=0.5)a.exposure="1/"+Math.round(1/b)}if(a.focallength){var c=parseFloat(a.focallength);if(c)a.focallength=Math.round(c*10)/10}if(a.flash)a.flash=a.flash=="true"?Vv:Uv;if(_features.geotagging){var d=parseFloat(a.latitude),e=parseFloat(a.longitude);if(!isNaN(d*e)){d=d.toFixed(6);e=e.toFixed(6);a.latitude=d<0?lx(-d):kx(d);a.longitude=e<0?nx(-e):mx(e)}else{a.latitude=null;a.longitude=null}}}var f=[];for(var g=0;g<qA.length;g++){var h=
qA[g],i=h[0],k=h[1],n="";if(a){n=a[i];if(!n||n=="0"||n=="0.0")n=Tv;else{if(h.length>=3)n=ez(h[2],{s:n});n="<em>"+Xy(n)+"</em>"}}f.push(k+": "+n)}var q='<div class="lhcl_exifbox">'+f.join("<br />")+"</div>";We(this.jl);this.jl.innerHTML=q};
lA.prototype.nS=function(){var a=this.jl,b=this.Hea;if(!(a&&b))return;if(this.IP){a.className="lhcl_exifoff";b.innerHTML=gw;this.IP=false}else{a.className="lhcl_photoinfo";b.innerHTML=fw;this.IP=true}};
lA.prototype.CF=function(a){this.ZS=a;var b=this.Ge;this.Tda=(this.oa=b.oa);var c=["uname="+(b.user?b.user.name:_user.name),"iid="+b.id,"noredir=true"];if(!a)c.push("cartop=remove");var d=Pi("_rtok","");c.push("rtok="+d);vz.qc(_updateCartPath,this.qba,c.join("&"))};
lA.prototype.vR=function(a){if(this.P.rd(a))return;var b=false;if(a.text.indexOf("<id>1</id>")>=0)b=true;else if(this.P.de(a))return;this.Ge.inCart=this.ZS;if(this.Tda==this.oa)this.By(this.Ge,true);var c=u("lhid_cartDiv"),d=u("lhid_cartCount");if(c&&d){c.style.display="";var e=d.innerHTML?parseInt(d.innerHTML,10):0;if(!b)d.innerHTML=this.ZS?e+1:e-1}};
lA.prototype.wR=function(a){if(this.P.rd(a))return;this.fU=eval("("+a.text+")");var b=this.Ge?this.Ge:this.ad[this.oa];if(b)this.By(b,false)};
lA.prototype.K2=function(a){if(pa(a.inCart))return a.inCart;if(!this.fU)return false;var b=a.user?a.user.name:_user.name,c=this.fU[b];return c&&cc(c,a.id)>=0};
lA.prototype.By=function(a,b){var c=this.LJ();if(c&&c.prints&&!a.vsurl){M(this.Qk,true);if(this.K2(a)){if(b)u(this.cartMsgId).innerHTML='<span>This photo has been added. <a href="/lh/viewCart" onclick="javascript:_setBackLink()" >View your order</a>.</span>';else u(this.cartMsgId).innerHTML="";this.Qk.innerHTML=vA;this.Qk.onclick=this.Rda}else{this.Qk.innerHTML=wA;this.Qk.onclick=this.Waa;u(this.cartMsgId).innerHTML=""}}else{M(this.Qk,false);u(this.cartMsgId).innerHTML=""}};
var qA=[["title",ew],["make",dw],["model",cw],["iso",bw],["exposure",aw,"{s} sec"],["fstop",$v,"f/{s}"],["focallength",Zv,"{s}mm"]],sA='<div class="lhcl_photoinfo">'+Vx+"<br />&nbsp;</div>",uA=m("Photo information"),tA=m("Video information"),vA='<img src="img/clearcart.gif"> <span class="lhcl_fakelink">'+Pv+"</span><br />",wA='<img src="img/order.gif"> <span class="lhcl_fakelink">'+Ov+"</span>",mA='<a href="#" id="{lhui_slideshowlink}" onclick="_d(\'hideSlideshow\');">'+hw+"</a><br />",pA='<div class="lhcl_toolbox"><div class="lhcl_tools lhcl_hideoverflow"><div id="{lhui_indexbox}">&nbsp;</div><div class="lhcl_albumtitle">{lhui_albumtitle}</div><div id="{lhui_infobox}">'+
sA+'</div><div class="lhcl_exifoff" id="{lhui_exifbox}"></div><div style="text-align:right"><span class="lhcl_toollink lhcl_fakelink" id="{lhui_togglelink}" onclick="_d(\'{lhui_togglelink}\')">'+gw+'</span></div><div class="lhcl_pseudo_hr"></div><div id="{lhui_links}"><a href="#" id="{lhui_downloadlink}">'+iw+'</a><br />{lhui_slideshow_link_markup}</div><div id="lhid_flashMsg" class="lhcl_output" style="display:none;"><span>{flash_str}</span><br /><br /><a href="http://www.macromedia.com/go/getflashplayer">'+
Qv+"</a></div></div></div>",oA='<div class="lhcl_toolbox"><div class="lhcl_tools lhcl_hideoverflow"><a class="lhcl_albumtitle" id="lhid_main_albumtitle" href="#">'+jx("<b>"," </b>",'<span id="lhid_main_albumtitle_text">{lhui_albumtitle}</span>')+'</a><div id="{lhui_infobox}">'+sA+'</div><div class="lhcl_exifoff" id="{lhui_exifbox}"></div><div style="text-align:right"><span class="lhcl_toollink lhcl_fakelink" id="{lhui_togglelink}" onclick="_d(\'{lhui_togglelink}\')">'+gw+'</span></div><div class="lhcl_pseudo_hr"></div><div id="{lhui_links}"><a href="#" id="{lhui_downloadlink}" style="display:block;">'+
iw+'</a>{lhui_slideshow_link_markup}<a href="#" id="{lhui_albumlink}" style="display:block;">'+ay+'</a></div><div id="lhid_flashMsg" class="lhcl_output" style="display:none;"><span>'+Sv+'</span><br /><br /><a href="http://www.macromedia.com/go/getflashplayer">'+Qv+"</a></div></div></div>",nA='<div class="lhcl_toolbox"><div class="lhcl_tools lhcl_hideoverflow"><div><b id="{lhui_infoheader}">'+uA+'</b></div><div id="{lhui_infobox}">'+sA+'</div><div class="lhcl_exifoff" id="{lhui_exifbox}"></div><div style="text-align:right"><span class="lhcl_toollink lhcl_fakelink" id="{lhui_togglelink}" onclick="_d(\'{lhui_togglelink}\')">'+
gw+'</span></div><div id="{lhui_links}"><a href="#" id="{lhui_downloadlink}">'+iw+'</a><br />{lhui_slideshow_link_markup}</div><div id="lhid_flashMsg" class="lhcl_output" style="display:none;"><span>{flash_str}</span><br /><br /><a href="http://www.macromedia.com/go/getflashplayer">'+Qv+"</a></div></div></div>";var xA='<table style="width:auto;"><tr><td><a href="{l}"><img src="{s}" /></a></td></tr><tr><td style="font-family:arial,sans-serif; font-size:11px; text-align:right">'+Av+' <a href="{al}">{a}</a></td></tr></table>';'<ul class="lhcl_albumcontrols"><li style="padding-top:0.2em;width:216px"><div id="lhid_embed" class="lhcl_embed"><span class="lhcl_fakelink" onclick="_d(\'{lhui_snippetFunc}\')">'+pv+'</span> <img src="img/triangle.gif" class="hand" id="lhid_zippy" onclick="_d(\'{lhui_snippetFunc}\')"><div id="{lhui_snippetId}" class="lhcl_embedlinks" style="display:none;"><div id="lhid_snippetinfo" style="margin-top:0.2em;">Paste link in <b>email or IM</b><br/><input class="lhcl_embedtextbox" onclick="this.select()" id="lhid_link"></div><div class="lhcl_pseudo_hr_embed"></div><div style="margin-top: 0.4em;">HTML to <b>embed in website</b><input class="lhcl_embedtextbox" onclick="this.select()" type="text" id="{lhui_snippetTextBoxId}" /></div><div>'+
nv+'<select id="lhid_sizeSelect" onchange="_d(\'{lhui_snippetFunc}\', 1)"><option value="144">'+mv+" "+sx(144)+'</option><option value="288">'+lv+" "+sx(288)+'</option><option value="400">'+kv+" "+sx(400)+'</option><option value="800">'+jv+" "+sx(800)+'</option></select></div><div style="margin-top:0.3em">'+ov+' <input id="{lhui_snippetCheckBoxId}" type="checkbox" style="margin:0;" onclick="_d(\'{lhui_snippetFunc}\', 1)"/></div></div></div></li></ul>';function yA(a,b,c,d){this.P=a;this.ba=null;this.Nd=w("div");M(this.Nd,!d);b.appendChild(this.Nd);this.di=w("div");M(this.di,!d);b.appendChild(this.di);this.yU=X("comment");this.Iy=X("addcommentbox");this.$$=X("unauthcommentbox");this.AI=X("addcomment");this.ao=gz(zA,{lhui_commentbox:this.yU,lhui_addcommentbox:this.Iy,lhui_unauthcommentbox:this.$$,signInStr:AA(_authuser),lhui_focuscomment:this.AI});M(this.ao,false);c.appendChild(this.ao);Y(this.AI,l(this.KR,this));this.Hda=l(this.YR,this);this.Lba=
l(this.DR,this);this.bR=X("viewAll");Y(this.bR,l(this.sS,this));this.KH=X("deleteCommentFunc");Y(this.KH,l(this.CR,this));this.BI=X("focusFunc");Y(this.BI,l(this.LR,this));this.hz=_authuser.user}
yA.prototype.show=function(a){M(this.ao,a);M(this.Nd,a);M(this.di,a)};
yA.prototype.ha=function(){this.kp();if(this.Nd)A(this.Nd);if(this.di)A(this.di);if(this.ao)A(this.ao);Z(this.AI);Z(this.bR);Z(this.KH);Z(this.BI);for(var a in this)this[a]=null};
yA.prototype.kp=function(){this.Nd.innerHTML="&nbsp;";this.di.innerHTML="";if(this.Do){vz.mF(this.Do);this.Do=0}if(this.Yt)Z(this.Yt);if(this.Hx)Z(this.Hx)};
yA.prototype.EF=function(a){this.ba=a;this.fe(a.r)};
yA.prototype.fe=function(a){this.Yf=a.id;var b=a.comment;this.Nd.innerHTML="&nbsp;";if(b.length>0){var c=['<div style="width:99%"><table class="lhcl_comments">'],d=this.ba?this.ba.user:null,e=_authuser.name,f=d?d.name==e:_authuser.isOwner;for(var g=0;g<b.length;++g){var h=b[g];h.lhui_date=h.timestamp;h.lhcl_ownerclass=_authuser.name==h.username?"lhcl_ownercomment":"";var i=Ta(h.comment);i=i.replace(/\n\n+/g,"\n\n").replace(/\n/g,"<br />");h.lhui_comment=Xy(bz(i));if(h.link){h.lhui_linkstart='<a href="'+
h.link+'">';h.lhui_linkend="</a>"}if(f||e==h.username)h.lhui_deletelink=ez(BA,{lhui_commentId:h.id,lhui_func:this.KH});c.push(ez('<tr class="{lhcl_ownerclass}" ><td rowspan="2" class="lhcl_commentimg">{lhui_linkstart}<img src="{portrait}" />{lhui_linkend}</td><th width="100%">{lhui_linkstart}{nickname}{lhui_linkend}</th><th>{lhui_date}</th><th>{lhui_deletelink}</th></tr><tr><td colspan="3" class="lhcl_commenttext">{lhui_comment}</td></tr>',h))}c.push("</table></div>");this.Nd.innerHTML=c.join("")}var k=
u(this.yU);if(b.length){var h=b[b.length-1];h.lhui_date=h.timestamp;h.lhui_viewallbox=X("viewall");Uy(k,gz(CA,h));k.style.display="";if(b.length>1){var n=u(h.lhui_viewallbox);if(n)n.appendChild(gz(DA,{lhui_viewallfunc:this.bR}))}}else{k.innerHTML="";k.style.display="none"}this.ao.style.display="";u(this.Iy).style.display=this.hz?"":"none";u(this.$$).style.display=this.hz?"none":"";this.OR(b.length)};
yA.prototype.OR=function(a){We(this.di);if(!this.hz){this.signInBoxId=X("signInBox");var b=ez(EA,{lhui_signInBoxId:this.signInBoxId,signInStr:AA(_authuser)})}else{if(a>=100){u(this.Iy).style.display="none";this.di.innerHTML=sw;return}else u(this.Iy).style.display="";this.Yt=X("commentForm");this.ls=X("commentText");this.Hx=X("commentLength");this.xea=X("submitButton");var b=ez(FA,{lhui_formId:this.Yt,lhui_id:this.ls,lhui_sizeId:this.Hx,lhui_submitId:this.xea,lhui_focusFunc:this.BI})}this.di.innerHTML=
b;if(this.hz)F(u(this.ls),I,function(c){c.target.blur();c.target.focus()});
this.KW=false;Y(this.Yt,l(this.XR,this));Y(this.Hx,l(this.eS,this))};
yA.prototype.XR=function(){var a=u(this.Yt),b=u(this.ls);if(!(b&&a))return;var c=Va(b.value);if(!c){alert(qw);return}if(c.length>512){alert(pw);return}var d=["uname="+ab(this.ba?this.ba.user.name:_user.name),"aname="+ab(this.ba?this.ba.album.name:_album.name),"iid="+this.Yf,"AddComment="+ab(c),"noredir=true"];for(var e=0;e<a.elements.length;e++){var f=a.elements[e];f.disabled=true;f.blur()}this.Pw=this.Yf;this.tba=c;vz.qc(_addCommentPath,this.Hda,d.join("&"))};
yA.prototype.YR=function(a){if(this.P.rd(a)||this.P.de(a)){var b=u(this.ls);if(b)b.value=this.tba;return}this.P.Lk(this.Pw,true)};
yA.prototype.CR=function(a){if(!confirm(rw))return;var b=["uname="+ab(this.ba?this.ba.user.name:_user.name),"mode=delete","cid_"+a+"=on","iid="+this.Yf,"noredir=true"];this.Pw=this.Yf;vz.qc(_selectedCommentsPath,this.Lba,b.join("&"))};
yA.prototype.DR=function(){this.P.Lk(this.Pw,true)};
yA.prototype.eS=function(a){var b=u(this.Hx);if(!(b&&a))return;if(a.value.length>480){var c=a.value.length,d=w("span"),e=Ue("Your comment is "+c+" characters long. (512 maximum)");d.appendChild(e);if(c>512)d.style.color="red";Uy(b,d)}else b.innerHTML=""};
yA.prototype.sS=function(){window.scrollTo(0,vh(this.Nd)-50)};
yA.prototype.KR=function(){window.scrollTo(0,vh(this.di)-50);if(this.ls){var a=u(this.ls);if(a&&!a.disabled)a.focus()}};
yA.prototype.LR=function(a){if(this.KW)return;a.value="";a.style.color="#000";this.KW=true};
var AA=function(a){var b=new N(lb(_loginUrl));b.fa("continue",location.href);var c=new N(lb(_activateUrl)),d=new N(c.Ga("continue"));d.fa("continue",location.href);c.fa("continue",d.toString());if(typeof a.user=="undefined"&&typeof a.isLoggedIn!="undefined"){var e=m("{$signUpLink}Activate{$linkEnd} your free Google Photos account.",{signUpLink:'<a href="'+c.toString()+'">',linkEnd:"</a>"});return _features.googlephotos?e:'<a href="'+c.toString()+'">Activate</a> your free Picasa Web Albums account.'}else{var f=
m("{$signInLink}Sign in{$linkEnd} if you have a Google Photos account, or {$signUpLink}sign up{$linkEnd} for a free account.",{signInLink:'<a href="'+b.toString()+'">',signUpLink:'<a href="'+c.toString()+'">',linkEnd:"</a>"});return _features.googlephotos?f:'<a href="'+b.toString()+'">Sign in</a> if you have a Picasa Web Albums account, or '+('<a href="'+c.toString()+'">')+"sign up</a> for a free account."}},
zA='<div class="lhcl_toolbox"><div class="lhcl_tools"><div id="{lhui_commentbox}"></div><div id="{lhui_addcommentbox}"><a href="javascript:_d(\'{lhui_focuscomment}\')">'+ow+'</a></div><div id="{lhui_unauthcommentbox}" style="display:none;"><b>'+ow+"</b><div>{signInStr}</div></div></div></div>",CA='<div class="lhcl_lastcomment"><table><tr><td width="100%"><b>'+nw+'</b></td><td id="{lhui_viewallbox}"></td></tr></table><div class="lhcl_pseudo_hr"></div><div>&#8220;{lhui_comment}&#8221;</div><div class="lhcl_commentauthor">{nickname} &#8211; {lhui_date}</div><div class="lhcl_pseudo_hr"></div></div>',
DA='<a class="lhcl_toollink" href="javascript:_d(\'{lhui_viewallfunc}\')">'+mw+"</a>",FA='<form class="lhcl_commentform" id="{lhui_formId}" action="javascript:_d(\'{lhui_formId}\')"><div class="lhcl_title">'+kw+'</div><textarea id="{lhui_id}"onfocus="_d(\'{lhui_focusFunc}\', this)" onkeydown="_d(\'{lhui_sizeId}\', this)" onkeyup="_d(\'{lhui_sizeId}\', this)"></textarea><table><tr><td width="100%"><div id="{lhui_sizeId}">&nbsp;</div></td><td><input type="submit" id="{lhui_submitId}" value="'+lw+'"  /></td></tr></table></form>',
EA='<div id="{lhui_signInBoxId}" class="lhcl_toolbox lhcl_photoCommentSignIn"><div class="lhcl_tools"><b>'+ow+"</b><div>{signInStr}</div></div></div>",BA='<span class="lhcl_fakelink" tabindex="0" onclick="_d(\'{lhui_func}\', \'{lhui_commentId}\')">'+jw+"</span>";function GA(a,b,c){this.P=a;this.pb=w("div");this.pb.className="lhcl_caption";b.appendChild(this.pb);this.zp=w("div");c.appendChild(this.zp);this.$H=X("edit_caption");Y(this.$H,l(this.dS,this));this.WV=X("lhui_deleteCaption")}
GA.prototype.ha=function(){if(this.xt)H(this.xt);Z(this.$H);if(this.zp)A(this.zp);if(this.pb)A(this.pb);for(var a in this)this[a]=null};
GA.prototype.kp=function(){if(this.xt){H(this.xt);delete this.xt}this.pb.innerHTML="&nbsp;";this.zp.innerHTML="";if(this.Hc&&this.Hc.id){Z(this.Hc.id);Z(this.Hc.cancelID);Z(this.Hc.sizeID)}};
GA.prototype.W7=function(a){this.T=a};
GA.prototype.Mk=function(a){this.Yf=a.id;var b=a.c||"",c=Xy(bz(b));c=c.replace(/  /g,"&nbsp; ").replace(/\n/g,"<br />");var d={caption:c},e=false;if(!a.user)e=_authuser.isOwner;else if(cA&&_features.googlephotos&&this.T)e=a.isOwner(this.T.ri());if(e){d.id=this.$H;d.text=b?Cw:Bw;var f=[ez('&nbsp;<span id="{id}" class="lhcl_fakelink" onclick="_d(\'{id}\')">{text}</span>',d)];if(b)f.push(ez('&nbsp;<span id="{lhui_deleteId}" class="lhcl_fakelink" style="vertical-align: bottom;"><img src="img/trash.gif" alt="{lhui_title}" title="{lhui_title}"/></span>',
{lhui_deleteId:this.WV,lhui_title:zw}));d.addtext=f.join("");this.Hc={caption:b}}this.kp();this.pb.innerHTML=ez("<b>{caption}</b>{addtext}",d);this.pb.style.display="";if(e&&b)this.xt=F(u(this.WV),E,this.AR,false,this)};
GA.prototype.AR=function(){if(confirm(yw))this.vF("")};
GA.prototype.dS=function(){this.pb.style.display="none";this.Hc={caption:this.Hc.caption,id:X("lhid_capform"),saveID:X("lhid_capsave"),sizeID:X("lhid_capsize"),cancelID:X("lhid_capcancel")};this.zp.appendChild(gz(HA,this.Hc));this.zF();Y(this.Hc.id,l(this.pS,this));Y(this.Hc.cancelID,l(this.oF,this));Y(this.Hc.sizeID,l(this.zF,this));var a=u(this.Hc.id);a.focus();yg(a,a.value.length);F(a,I,function(b){b.target.blur();b.target.focus()})};
GA.prototype.oF=function(){this.pb.style.display="";this.zp.innerHTML=""};
GA.prototype.zF=function(){var a=u(this.Hc.sizeID),b=u(this.Hc.id),c=u(this.Hc.saveID);if(!(a&&b&&c))return;if(b.value.length>992){var d=b.value.length,e=1024,f=w("span");f.appendChild(Ue("Your caption is "+d+" characters long. ("+e+" maximum)"));if(d>e)f.style.color="red";Uy(a,f)}else a.innerHTML=""};
GA.prototype.pS=function(){var a=u(this.Hc.id);if(!a)return;var b=Va(a.value);if(b==this.Hc.caption){this.oF();return}if(b.length>1024){alert(qx(1024));return}u(this.Hc.saveID).disabled=true;u(this.Hc.cancelID).disabled=true;this.vF(b)};
GA.prototype.vF=function(a){var b=["description="+ab(a),"noredir=true"],c=this.Yf,d=l(this.uR,this,c,a),e=_updatePhotoPath+"&uname="+_user.name+"&aname="+_album.name+"&iid="+c;vz.qc(e,d,b.join("&"))};
GA.prototype.uR=function(a,b,c){if(!this.P)return;if(this.P.rd(c)||this.P.de(c)){u(this.Hc.saveID).disabled=false;u(this.Hc.cancelID).disabled=false;return}this.P.Nk(a,p(b))};
var HA='<div class="lhcl_captionform"><div id="{sizeID}" style="color:#999"></div><textarea name="description" id="{id}" rows="0" comments="0" onkeydown="_d(\'{sizeID}\')" onkeyup="_d(\'{sizeID}\')">{caption}</textarea><div><input type="button" value="'+Aw+'"class="lhcl_default" id="{saveID}" onclick="_d(\'{id}\')" /><input type="button" value="'+wx+'" id="{cancelID}" onclick="_d(\'{cancelID}\')"/></div></div>';function IA(){Ez.Hd(this);Y("ClearCart",l(this.yR,this))}
IA.prototype.ha=function(){Ez.ee(this);Z("ClearCart")};
IA.prototype.yR=function(){if(confirm(tw)){var a=u("lhid_clearform"),b=new N(_updateCartPath);b.fa("cartop","clear");b.fa("rtok",Pi("_rtok",""));a.action=b.toString();a.submit()}};
function JA(a,b,c){var d="uname="+a+"&iid="+c+"&cartop=remove&noredir=true",e=Pi("_rtok","");d+="&rtok="+e;vz.qc(_updateCartPath,null,d);var f=u("lhid_cart_"+c);if(f)A(f);cartPhotoCount--;var g=u("lhid_viewcartphotocount");B(g,cartPhotoCount);var h=u("lhid_cartCount");if(h)B(h,cartPhotoCount);cartAlbumCount[b]--;if(cartAlbumCount[b]<=0){var i=u("lhid_cartalbum_"+b);if(i)A(i)}if(cartPhotoCount<=0){u("lhid_emptycart").style.display="";u("lhid_instructions3_photos").style.display="";u("lhid_nonemptycart").style.display=
"none";u("lhid_instructions3_none").style.display="none";u("lhid_instructions3").style.display="none";u("lhid_instructions").className="lhcl_cartinstructions lhcl_instep1"}}
function KA(a,b){var c=Pi("printProvider");if(!c)return true;Oi(LA,"1");var d="/lh/sendToProvider?provider="+c;if(b)d=d+"&link="+b;if(a){window.open(d,"selectProviderWnd");var e='<a href="javascript:void(0)" onclick="return window._sendToProvider(false, \''+b+"');\">";u("lhid_notifyMsg").innerHTML="Your order has been opened in a new window.  If a new window didn't open, click "+e+"here</a>";u("lhid_feedback").style.display=""}else window.location=d;return false}
function MA(a,b){var c=u("lhid_orderform"),d=Pi("_rtok",""),e=new N(c.action);e.fa("rtok",d);e.fa("uname",a);e.fa("aid",b);c.action=e.toString();c.submit()}
function NA(){if(OA)return;OA=true;vz.qc("/lh/selectProvider",PA)}
function PA(a){var b=m("We're sorry.  Google Photos does not currently work with any photo processing providers that ship to your country.");if(a.status==200&&a.text.length==0){alert(_features.googlephotos?b:vw);OA=false;return}else if(a.status>=400||a.text.length==0){alert(ww);OA=false;return}jz("hideVideo");new QA(a.text)}
function QA(a){Ez.Hd(this);var b={lhui_ok_func:X("func"),lhui_cancel_func:X("func")};this.r=b;this.Xb=w("div");this.Xb.className="lhcl_cover";document.body.appendChild(this.Xb);this.Jf();this.nm=l(this.Jf,this);Ly(window,"resize",this.nm);window.scrollTo(0,0);RA();this.ka=gz(SA+a+TA,this.r);document.body.insertBefore(this.ka,document.body.firstChild);var c=Pi("printProvider");if(!c)document.getElementById("lhid_accept_provider").disabled=true;Y(this.r.lhui_ok_func,l(this.oR,this));Y(this.r.lhui_cancel_func,
l(this.nF,this));this.qw=Ky(this,this.tF);Ly(document,"keydown",this.qw)}
QA.prototype.tF=function(a){if(a.keyCode==27)this.nF()};
QA.prototype.ha=function(){Ez.ee(this);if(this.r){Z(this.r.lhui_ok_func);Z(this.r.lhui_cancel_func)}My(document,"keydown",this.qw);My(window,"resize",this.nm);OA=false;for(var a in this)this[a]=null};
QA.prototype.Fc=function(){A(this.ka);A(this.Xb);jz("restoreVideo");this.ha()};
QA.prototype.Jf=function(){var a=Math.max(document.body.offsetHeight,document.body.scrollHeight);a=Math.max(a,document.body.clientHeight);a=Math.max(a,document.documentElement.clientHeight);this.Xb.style.height=a+"px"};
QA.prototype.nF=function(){this.Fc()};
function UA(a){Oi("printProvider",a,2592000);var b=w("img");b.width=150;b.height=50;b.src="/lh/providerLogo/"+a;var c=u("lhid_currentprovider");Uy(c,b);u("lhid_hasprovider").style.display="";u("lhid_noprovider").style.display="none";c=u("lhid_instructions3");if(c){u("lhid_instructions3_none").style.display="none";if(cartPhotoCount>0){c.style.display="";u("lhid_instructions").className="lhcl_cartinstructions lhcl_instep3"}}}
QA.prototype.oR=function(){var a=u("lhid_chooseProviderForm").provider;if(typeof a.length=="undefined")UA(a.value);else for(var b=0;b<a.length;b++)if(a[b].checked){UA(a[b].value);break}this.Fc()};
function VA(){var a=0,b="";for(var c=1;c<=4;c++){var d=Pi("_cart"+c);if(d)b+=d}var e=b.split("@");if(e.length>=2){var f=e[1].match(/\:/g);if(f)a=f.length}if(a>0){u("lhid_cartDiv").style.display="";u("lhid_cartCount").innerHTML=a}}
var OA=false,LA="_lh_cartRedirect",SA='<div class="lhcl_dialog" style="width:400px; top:100px"><div class="lhcl_dialog_body"><form id="lhid_chooseProviderForm"><div class="lhcl_selectProviderTitle">'+uw+'</div><div style="padding:0 25px 15px 0;">',TA='</div><table style="width:100%"><tr><td style="width:50%; text-align:right;"><input type="button" value="'+wx+'" onclick="_d(\'{lhui_cancel_func}\')"/></td><td><input id="lhid_accept_provider" type="button" value="'+xw+'" onclick="_d(\'{lhui_ok_func}\')"/></td></tr></table></form></div></div>';var iA=function(){};
iA.prototype.ha=function(){Ez.ee(this);if(this.Fe)window.clearTimeout(this.Fe);Z("hideSlideshow");Z("hideVideo");Z("restoreVideo");if(this.am)this.am.ha();if(this.Yx)this.Yx.ha();if(this.pb)this.pb.ha();if(this.Nd)this.Nd.ha();if(this.gm)this.gm.ha();if(this.pd)this.pd.ha();if(this.Dc)this.Dc.ha();if(this.ka)A(this.ka);if(this.Nb)A(this.Nb);if(this.rh)this.rh.close();if(this.eca)A(this.eca);G(this.ob,"key",this.Wf,false,this);this.ob.b();if(this.zg)this.zg.b();if(this.ol)H(this.ol);if(this.iA)this.iA.b();
for(var a in this)this[a]=null};
iA.prototype.Fc=function(){RA();if(this.Do)vz.mF(this.Do);this.ha()};
iA.prototype.fe=function(a){var b=a[1],c=this.uN[b];if(c==undefined){location.replace(location.href.replace(/#.*/,"#unknown-"+b));return}this.oa=c;this.title=" - "+this.ba[c].t;var d=this.ba,e=c>0?d[c-1].id:null,f=c<d.length-1?d[c+1].id:null;this.gm.oS(e,f);if(this.rh)this.rh.Uo(_user.name,_album.id,b,!(!this.ba[c].vsurl));this.am.Mk(c);if(this.Dc)this.Dc.kp();this.pd.GoToIndex(c);if(this.Wca){var g=new N(_loginUrl);g.fa("continue",location.href);this.Wca.href=g.toString()}if(this.nea){var h=new N(_signUpUrl);
h.fa("continue",location.href);this.nea.href=h.toString()}};
iA.prototype.Le=function(){if(!this.pd||!this.pd.Le())return null;return this.pd.Le().Le()};
iA.prototype.jp=function(a,b){var c=this.oa+a,d=this.ba.length;if(b)c=qe(c,d);if(c<0||c>=d)return false;location=location.href.replace(/#.*/,"#"+this.ba[c].id);return true};
iA.prototype.Xh=function(){this.jp(1)};
iA.prototype.Yh=function(){this.jp(-1)};
iA.prototype.hp=function(){location=_album.link};
iA.prototype.Ds=function(){return Vy()-vh(this.Nb.firstChild)-50};
iA.prototype.Jf=function(){this.gp()};
iA.prototype.gp=function(){var a=this.pd.Le(),b=this.pd.GetIndex();if(!a||a.Gb()||a.getMinScale()>=1||!this.ba[b].dl){this.gm.Pm(false,true);if(a)a.SetZoom(false)}else this.gm.Pm(a.getZoom(),false)};
iA.prototype.Lk=function(a,b){var c=this.uN[a],d=this.ba[c],e=(new Date).getTime(),f=this.Oca[a];if(!b&&f&&e-f<60000)e=f;this.Oca[a]=e;var g="/lh/rssPhoto?uname="+_user.name+"&iid="+d.id+"&v="+e+"&js=1";this.Do=vz.qc(g,this.jfa)};
iA.prototype.rS=function(a,b){if(_features.showgeo){var c=this.ba[b];if(b==this.oa){if(c.lat&&c.lon){if(!this.o)this.o=new WA(this.Fi,2);this.bp(true);this.o.Lo(c)}else this.bp(false);this.qS(a,b)}}};
iA.prototype.bp=function(a){if(_features.geotagging){a=_features.showgeo&&a;var b=this.Yy.isOwner?this.Fi:this.GI;M(b,a)}};
iA.prototype.Vt=function(a){if(_features.geotagging){var b=Math.max(a,this.ba.length-a-1);for(var c=1;c<=b;c++){var d=a-c,e;if(d>=0){e=this.ba[d];if(e.lat&&e.lon)return this.Cs(e)}d=a+c;if(d<this.ba.length){e=this.ba[d];if(e.lat&&e.lon)return this.Cs(e)}}}return null};
iA.prototype.Cs=function(a){if(_features.geotagging)if(a.lat&&a.lon){var b={lat:a.lat,lon:a.lon,address:a.location||""};if(a.bb)b.bb={ll:{lat:a.bb.ll.lat,lon:a.bb.ll.lon},ur:{lat:a.bb.ur.lat,lon:a.bb.ur.lon},latSpan:a.bb.latSpan,lonSpan:a.bb.lonSpan};return b}return null};
iA.prototype.qS=function(a){if(_features.showgeo){var b=this.ba[this.oa],c=b.lat&&b.lon;if(this.Yy.isOwner){var d;d=c?Jv:Iv;B(this.pp,d);this.pp.onclick=l(this.qR,this,a)}M(this.Gaa,c)}};
iA.prototype.qR=function(a){if(this.Yy.isOwner){var b=this.ba[this.oa],c=this.vt,d;if(!(b.lat&&b.lon)&&!c){c=this.Vt(this.oa);if(!c&&this.Da.lat&&this.Da.lon){c=this.Cs(this.Da);d=this.Da.location}}if(this.Gb())this.MR();if(this.zg){this.zg.b();this.zg=null}if(this.ol){H(this.ol);this.ol=null}this.zg=new XA(b,l(this.pR,this,a),d,c?[c]:null);if(this.Gb())this.ol=F(this.zg,Zk,l(this.gS,this))}};
iA.prototype.pR=function(a){if(this.Yy.isOwner){var b=this.uN[a.id],c=this.ba[b];if(c.lat&&c.lon){this.vt=this.Cs(c);this.Da.hasGeotaggedPhotos=true}else this.vt=null;if(b==this.oa){a.lat=c.lat;a.lon=c.lon;this.am.fe(a);this.rS(a,b)}}};
iA.prototype.lp=function(){var a=this.pd.Le(),b=a.getMinScale(),c=a.getZoom();if(c){this.gm.Pm(false);a.SetZoom(false)}else if(!a.Gb()&&b<1){this.gm.Pm(true);a.SetZoom(true)}};
iA.prototype.Nk=function(a,b){var c=this.uN[a],d=this.ba[c];if(!d)return;d.c=b;this.pb.Mk(d)};
iA.prototype.vy=function(){if(this.Z$)return false;this.Z$=true;this.gm.Om(true);if(this.Yx)this.Yx.Om(true);return true};
iA.prototype.Ay=function(){this.Z$=false;this.gm.Om(false);if(this.Yx)this.Yx.Om(false)};
iA.prototype.rd=function(a){var b=a.status;if(b>=400){var c;c=b==403?Nv:(b==404?Mv:(b>=500?Kv+"\n"+ix(b):Lv+"\n"+ix(b)));alert(c);return true}return false};
iA.prototype.de=function(a){var b=a.text;if(a.target&&a.target.Sb)b=a.target.Sb();if(b.indexOf("success")<0){var c=b.replace(/\<[^>]*\>/g,"");alert(c);return true}return false};
iA.prototype.Wf=function(a){if(YA(a))return;if(this.zg&&this.zg.Ja())return;switch(a.keyCode){case 27:case 85:this.hp();a.preventDefault();break;case 74:case 37:this.Yh();a.preventDefault();break;case 75:case 39:this.Xh();a.preventDefault();break}};
iA.prototype.Gb=function(){if(!this.pd)return false;var a=this.pd.Le();if(!a)return false;return a.Gb()};
iA.prototype.ry=function(){if(this.Gb())return false;var a=this.pd.GetIndex();if(this.ba[a].br)return false;return true};
iA.prototype.Es=function(){if(this.pd.I)this.pd.I.ty()};
iA.prototype.MR=function(){if(this.Gb()&&this.pd.I&&this.pd.I.dI)this.pd.I.dI.style.visibility="hidden"};
iA.prototype.gS=function(){if(this.Gb()&&this.pd.I&&this.pd.I.dI)this.pd.I.dI.style.visibility=""};
var YA=function(a){if(a.altKey||a.ctrlKey||a.metaKey)return true;var b=a.target.nodeName.toLowerCase();if(b=="button"||b=="input"||b=="select"||b=="textarea")return true;return false};;function ZA(a){this.name=a}
ZA.prototype.toString=function(){return this.name};;;var $A=function(a,b){yl.call(this,"lhcl_menu");this.Nm=1000;this.up=a;this.Rv=b;this.zfa=false;this.JO(this.up,3);H(this.zn);this.zn=F(this.Rv,I,l(this.ro,this));F(this.Rv,"dblclick",this.NV,false,this)};
o($A,yl);$A.prototype.ro=function(a){yl.prototype.ro.call(this);zz(this.e);a.preventDefault()};
$A.prototype.qf=function(){Az();yl.prototype.qf.call(this)};
$A.prototype.NV=function(){if(this.vc)this.M(false)};
var aB=function(a){zl.call(this,lb(a));this.qg=false};
o(aB,zl);aB.prototype.Fd=function(a){this.qg=a;if(this.e)this.os()};
aB.prototype.os=function(){if(!this.e)return;if(this.qg)C(this.e,"lhcl_active");else qf(this.e,"lhcl_active")};
aB.prototype.$m=function(){if(!this.R)throw Error("MenuItem is not attached to a menu");this.e=v("div",{className:this.R.wJ()},this.pb,this.ng?v("span",null,"\u25b6"):null);this.e.onselectstart=function(){return false};
return this.e};
var bB=function(a){K.call(this);var b=a.query,c=null;this.ca=[];for(var d in a){if("primary"==d||"query"==d)continue;this.ca.push(a[d])}this.Ib=u("lhid_searchbox");this.Kfa=u("lhid_searchform");this.ZC=u("lhid_querytext");this.Km=!Qa(Va(b));this.Jfa=u("lhid_searchbox_context");this.cea=u("lhid_psc");this.dea=u("lhid_crowdfilter");this.Rv=u("lhid_btn");var e=u("lhid_searchanchor");if(!e)e=u("lhid_searchanchor_on");this.R=new $A(e,this.Rv);this.Ib.value=lb(b);this.ZC.value=this.Km?this.Ib.value:"";
var f=null;for(var g=0;g<this.ca.length;++g){var d=this.ca[g],h=d.name;d.name=h;var i=null;if(h=="-")i=new Al;else{i=new aB(h);i.qa(d)}this.R.add(i);if(d.is_primary){i.Fd(true);this.Kb=i;c=d}i.os();if("G"==d.search_corpus)f=i}this.g(this.R,wl,this.v9,true,this);this.g(this.Ib,I,this.ht,true,this);this.g(this.Ib,jg,this.ht,true,this);this.g(this.Ib,hg,this.EB,true,this);this.g(this.Ib,gg,this.EB,true,this);this.g(this.Ib,kg,this.EB,true,this);this.Ib.blur();if(f)this.qa(f);else{var k=c?c.search_corpus:
"G",n=c?c.name:Hu;this.Ph(k,n)}this.g(this.Ib,ig,this.HQ,true,this);this.g(document,"selectstart",this.C7,true,this)};
o(bB,K);bB.prototype.b=function(){if(this.W())return;K.prototype.b.call(this)};
bB.prototype.C7=function(a){if(this.R.vc)a.preventDefault()};
bB.prototype.v9=function(a){this.qa(a.item)};
bB.prototype.ht=function(){this.Ib.focus();if(!this.Km){this.Ib.value="";this.ZC.value="";qf(this.Ib,"lhcl_nosearch")}};
bB.prototype.EB=function(){var a=Va(this.Ib.value),b=Qa(a);this.ZC.value="";if(this.Km&&b)this.Km=false;else{this.Km=!b;if(this.Km)this.ZC.value=a}};
bB.prototype.HQ=function(){if(!this.Km){C(this.Ib,"lhcl_nosearch");var a=this.Kb.L().name;if(a){a=lb(a);this.Ib.value=a}}else qf(this.Ib,"lhcl_nosearch")};
bB.prototype.qa=function(a){if(this.Kb)this.Kb.Fd(false);a.Fd(true);this.Kb=a;var b=this.Kb.L();this.Ph(b.search_corpus,b.name)};
bB.prototype.Ph=function(a,b){this.Xea=a;var c="Now searching in "+b;this.Rv.title=c;if(!this.Km)this.Ib.value=lb(b);this.cea.value=a;this.dea.value="G"==a?1:0;this.HQ()};function cB(a,b,c){Ez.Hd(this);this.da=a;this.j=new K(this);var d=m("Untitled Album"),e={body_id:X("dlg"),toggle_id:X("toggle"),form_id:X("form"),submit_id:X("input"),cancel_id:X("input"),album_id:X("input"),title_id:X("input"),desc_id:X("input"),location_id:X("input"),dayselect:X("select"),monthselect:X("select"),yearselect:X("select"),albumlist:X("list"),choosefunc:X("func"),createfunc:X("func"),selectfunc:X("func"),onsubmit:X("func"),oncancel:X("func"),title:d,description:"",location:"",publicChecked:"checked",
privateChecked:"",dateChecked:"",uname:_user.name};if(_features.geotagging){e.lat_id=X("input");e.lon_id=X("input");e.latSpan_id=X("input");e.lonSpan_id=X("input");e.removeGeo_id=X("input");e.spin_id=X("img");e.results_id=X("div");e.map_id=X("map");e.lhui_geoTable=X("table");e.lhui_showLocation_id=X("input");e.lhui_locationChecked="checked";e.geocodefunc=X("func")}this.f6=a!=0&&a!=5;if(cA&&_features.froptin){e.nameTagsInherit=0;e.nameTagsShow=1;e.nameTagsHide=2;if(_album.nametagsvisibility==undefined)_album.nametagsvisibility=
0;e.nameTagsVisibilityInherit=_album.nametagsvisibility==0?"checked":"";e.nameTagsVisibilityShow=_album.nametagsvisibility==1?"checked":"";e.nameTagsVisibilityHide=_album.nametagsvisibility==2?"checked":"";e.defaultAlbumNameTagsVisibility=_user.defaultalbumnametagsvisibility}switch(a){case 5:var f=m("Edit album information");e.header=f;e.action=_editAlbumPath;e.dialog="updateAlbumDialog";e.redir="SAME_ALBUM";e.id=_album.id;e.title=_album.title;e.description=_album.description;e.location=_album.location;
e.publicChecked=_album.unlisted?"":"checked";e.privateChecked=_album.unlisted?"checked":"";e.dateChecked=_album.updatedate?"checked":"";if(_features.geotagging){if(_album.location&&(!_album.lat||!_album.lon))e.lhui_locationChecked="";e.lat=_album.lat;e.lon=_album.lon;if(_album.bb){e.latSpan=_album.bb.latSpan;e.lonSpan=_album.bb.lonSpan}}break;case 3:var g=m("Copying items to album: {$numPhotos}",{numPhotos:b.length});e.header=g;e.action=_copyOrMovePath;e.dialog="CopyOrMovePhotosDialog";e.selected=
b.join(",");e.photoop="copy";e.uname=_user.name;break;case 4:var h=m("Moving items to album: {$numPhotos}",{numPhotos:b.length});e.header=h;e.action=_copyOrMovePath;e.dialog="CopyOrMovePhotosDialog";e.selected=b.join(",");e.photoop="move";break;case 0:var i=m("Create a new album");e.header=i;e.action=_createAlbumPath;e.dialog="NewOrExistingDialog";e.dest="new";break;case 2:var k=m("Upload Photos: Create Album");e.header=k;e.action=_createAlbumPath;e.dialog="NewOrExistingDialog";e.redir="upload";e.dest=
"new";break;case 1:default:var n=m("Upload Photos: Create or Select Album");e.header=n;e.action=_createAlbumPath;e.dialog="NewOrExistingDialog";e.redir="upload";e.dest="new";e.uname=_authuser.name;break}if(a==5||a==0||a==2)this.WC=true;this.Da=_album;this.r=e;this.rfa=b;this.jm=c;this.Aa=-1;this.Xb=w("div");this.Xb.className="lhcl_cover";document.body.appendChild(this.Xb);this.nb();this.j.g(window,mg,this.nb);window.scrollTo(0,0);RA();jz("hideVideo");this.ka=gz(dB,this.r);document.body.insertBefore(this.ka,
document.body.firstChild);Y(this.r.oncancel,l(this.Fc,this));Y(this.r.onsubmit,l(this.Nl,this));Y(this.r.choosefunc,l(this.iM,this));Y(this.r.createfunc,l(this.Nv,this));Y(this.r.selectfunc,l(this.zD,this));if(_features.geotagging){Y(this.r.geocodefunc,l(this.HI,this));if(this.r.geotag){this.r.geotag.b();this.r.geotag=null}}this.j.g(document,gg,this.yh);if(a==1)this.iM();else this.Nv();zz(this.ka);Iy(this,Gy)}
var eB=m("Place Taken");cB.prototype.ha=function(){Ez.ee(this);if(this.r){Z(this.r.oncancel);Z(this.r.onsubmit);Z(this.r.choosefunc);Z(this.r.createfunc);Z(this.r.selectfunc);if(_features.geotagging){Z(this.r.geocodefunc);if(this.r.geotag)this.r.geotag.b()}}this.j.b();for(var a in this)this[a]=null};
cB.prototype.Fc=function(){Az();if(this.ka)A(this.ka);if(this.Xb)A(this.Xb);if(this.jm)this.jm();this.ha();Iy(this,Hy)};
cB.prototype.Nv=function(){this.sd=null;if(this.da==3||this.da==4)this.r.albumop="new";var a=u(this.r.submit_id);if(this.da==5){var b=m("Save changes");a.value=b}else if(this.da==0){var c=m("Create");a.value=c}else{var d=m("Continue");a.value=d}a.disabled=false;F(a,E,this.O5,false,this);this.AP(!_user.noalbums);var e=u(this.r.body_id),f=ez('<a href="{link}">{link}</a>',_user);this.r.publicSearchable="";if(_features.globalsearch){var g=m("This album will be visible to all users at {$galleryurl}. It will be included in Picasa Web Albums community search, based on preferences (Community search preferences may be modified on the Settings page).",
{galleryurl:f}),h=m("This album will be visible to all users at {$galleryurl}. It will be included in Google Photos community search, based on preferences (Community search preferences may be modified on the Settings page).",{galleryurl:f});this.r.public_desc=_features.googlephotos?h:g;if(_user.searchable)this.r.publicSearchable="_searchable"}else{var i=m("For albums you want to show publicly. Anyone who visits {$galleryurl} can see them",{galleryurl:f});this.r.public_desc=i(f)}Uy(e,gz(fB(),this.r));
var k=u(this.r.title_id);if(this.da==5){if(_album.blogger||_album.isdefault){k.disabled=true;k=u(this.r.desc_id)}if(!_features.autoupdatealbums&&_album.isdefault)u("lhid_date").disabled=true}k.focus();k.select();if(_features.geotagging){var n=u(this.r.lhui_showLocation_id);if(n.checked){var q=u(this.r.lhui_geoTable);M(q,true)}this.j.g(n,E,this.p0);var t;if(this.da==5){t={id:this.Da.id,lat:this.Da.lat,lon:this.Da.lon};if(this.Da.bb)t.bb={ll:{lat:this.Da.bb.ll.lat,lon:this.Da.bb.ll.lon},ur:{lat:this.Da.bb.ur.lat,
lon:this.Da.bb.ur.lon}}}else t={id:"newAlbumId"};var y=m("Would you like to move the map to the new location?");this.r.geotag=new gB(t,"album",u(this.r.map_id),u(this.r.results_id),u(this.r.location_id),u(this.r.lat_id),u(this.r.lon_id),u(this.r.latSpan_id),u(this.r.lonSpan_id),u(this.r.removeGeo_id),{noGeotagMessage:Mx(eB),geotaggedMessage:Tx,updateGeoPrompt:y},5,5,this.r.spin_id,false);if(!n.checked)this.r.geotag.fI(false)}if(cA&&_features.froptin){var z=u("lhid_publicaccess"),O=u("lhid_unlistedaccess");
this.j.g(z,E,this.oK);this.j.g(O,E,this.oK);this.LQ(z.checked)}this.m2()};
cB.prototype.iM=function(){this.Aa=-1;if(typeof vz=="undefined"||!vz.Gs()){if(this.WC)this.Fc();else{this.WC=true;this.Nv()}return}if(this.da==3||this.da==4)this.r.albumop="existing";var a=m("Select Album"),b=u(this.r.submit_id);b.value=a;b.disabled=true;this.AP(false);var c=u(this.r.body_id);Uy(c,gz(hB,this.r));if(this.Kd)this.VB();else{var d="/data/feed/back_compat/user/"+_authuser.name+"?alt=json&kind=album";vz.qc(d,l(this.Xj,this))}};
cB.prototype.AP=function(a){var b=u(this.r.toggle_id);if(this.WC){b.innerHTML="";b.style.display="none"}else{b.style.display="";Uy(b,gz(a?iB:jB,this.r))}};
cB.prototype.Xj=function(a){var b=vz.HR(a);if(!b)return;this.Kd=b.feed.entry;if(this.VB)this.VB()};
cB.prototype.VB=function(){if(!this.Kd){if(_features.PERFORMANCE_EXPERIMENTS){this.WC=true;this.Nv()}return}var a=u(this.r.albumlist);a.innerHTML="";var b=-1;for(var c=0;c<this.Kd.length;++c){var d=this.Kd[c];if(_album&&_album.id==d.gphoto$id.$t){if(this.da==4||this.da==3){this.Kd.splice(c,1);c--;continue}b=c}var e=false;for(var f=0;f<d.link.length;f++)e|=d.link[f].rel=="edit";if(!e)if(this.da==4||this.da==3||this.da==1){this.Kd.splice(c,1);c--;continue}d.selectfunc=this.r.selectfunc;d.onsubmit=this.r.onsubmit;
d.name=p(d.title.$t);d.icon=d.photo$thumbnail.$t;d.access=d.rights.$t!="public"?'<span class="lhcl_unlisted">'+$w+"</span>":'<span class="lhcl_public">'+ax+"</span>";d.num=c;a.appendChild(gz('<table class="lhcl_fakelink" onmousedown="_d(\'{selectfunc}\', {num}); return false" ondblclick="_d(\'{onsubmit}\')" onselectstart="return false"><tr><td><img src="{icon}" width="32" height="32" /></td><td width="100%">{name}<br />{access}</td></tr></table>',d))}if(!this.Kd)return;if(b>=0)this.zD(b)};
cB.prototype.O5=function(){this.f6=true};
cB.prototype.Nl=function(){if(!this.f6)return;var a=u(this.r.form_id);if(!a)return;var b=u(this.r.title_id);if(b&&Va(b.value)==""){var c=m("The title of your album must not be blank. Please enter a title.");alert(c);return}if(this.lk&&this.lk.style.display=="")return false;var d=u("lhid_datefield");if(d){var e=u(this.r.yearselect).value,f=parseInt(u(this.r.monthselect).value,10)+1,g=u(this.r.dayselect).value;d.value=f+"/"+g+"/"+e}var h=u(this.r.desc_id);if(h&&h.value.length>1000){var i=m("Album descriptions must be less than {$maxChars} characters long. You are currently using {$numChars} characters. Please make your description shorter and try again.",
{numChars:h.value.length,maxChars:1000}),k=m("Album descriptions must be {$maxChars} characters or less. You are currently using {$numChars} characters. Please make your description shorter and try again.",{numChars:h.value.length,maxChars:1000});alert(_features.newStrings?k:i);return}u(this.r.submit_id).disabled=true;u(this.r.cancel_id).disabled=true;if(this.da==1&&this.sd)location.href="/lh/webUpload?uname="+_authuser.name+"&aid="+this.sd;else a.submit()};
cB.prototype.yh=function(a){if(this.Aa<0)return;var b=a.keyCode;if(b==38||b==40){window.scrollTo(0,0);var c=b==38?-1:1;this.zD(pe(this.Aa+c,0,this.Kd.length-1));a.preventDefault();a.stopPropagation()}};
cB.prototype.zD=function(a){var b=u(this.r.albumlist);if(this.Aa>=0)b.childNodes[this.Aa].className="lhcl_fakelink";b.childNodes[a].className="lhcl_selected lhcl_fakelink";this.Aa=a;this.sd=this.Kd[a].gphoto$id.$t;u(this.r.album_id).value=this.sd;var c=u(this.r.submit_id);c.disabled=false;c.focus();c.parentNode.style.textAlign="right";var d=b.childNodes[this.Aa],e=d.offsetTop,f=e+d.offsetHeight,g=b.scrollTop,h=g+b.offsetHeight;if(e<g)b.scrollTop=Math.max(e-(f-e)*0.3,0);else if(f>h)b.scrollTop=e+(f-
e)*1.3-(h-g)};
cB.prototype.nb=function(){var a=Math.max(document.body.offsetHeight,document.body.scrollHeight);a=Math.max(a,document.body.clientHeight);a=Math.max(a,document.documentElement.clientHeight);this.Xb.style.height=a+"px"};
cB.prototype.m2=function(){this.aq=u("lhid_date");this.lk=u("lhid_dp");var a=this.da==5?kz(_album.datestring):new Date;if(a==null)a=new Date;this.Yk=a;var b=new Tk(this.Yk);b.v8(_LH.calDay==0?6:0);this.jx();this.Gx();this.j.g(this.aq,jg,this.p_);this.j.g(this.aq,gg,this.q_);this.j.g(u(this.r.title_id),jg,this.sK);this.j.g(u(this.r.desc_id),jg,this.sK);this.j.g(this.ka,E,this.s_);this.j.g(b,"select",this.r_);b.EP(5,"weekend");b.EP(6,"weekend");b.f9(false);b.r8(false);b.g9(false);b.create(this.lk);
if(!s){var c=this.lk.getElementsByTagName("button");for(var d=0,e=c.length;d<e;d++)c[d].type="button"}B(b.aI,wx);this.EH=b;M(this.lk,false)};
cB.prototype.p_=function(){M(this.lk,true)};
cB.prototype.sK=function(){this.UG()};
cB.prototype.q_=function(a){if(a.keyCode==9){this.OB=true;this.PB=true;this.OE()}};
cB.prototype.s_=function(a){if(this.lk.style.display=="none")return;if(a.target==this.aq){this.OB=true;this.PB=true;this.OE();return}var b=Bh(this.lk).qQ(),c=Se(),d=new ge(a.clientX+c.x,a.clientY+c.y);if(!b.contains(d)){this.UG();return}this.Yk=this.EH.getDate();this.Gx();this.jx();a.stopPropagation()};
cB.prototype.UG=function(){this.OB=true;this.PB=false;this.OE()};
cB.prototype.pQ=function(a){if(a.getFullYear()<1970){var b=m("Album dates must be set to 1970 or later. Please try again.");alert(b);return false}else if(a.getFullYear()>2037){var c=m("Album dates cannot be later than {$year}. Please try again.",{year:2037});alert(c);return false}else return true};
cB.prototype.C$=function(){var a=this.aq.value,b=new Date,c=pi(ii),d=Ii(c.DATEFORMATS[3],a,0,b);return d>0?this.pQ(b):false};
cB.prototype.r_=function(a){if(a.date)if(this.pQ(a.date)){this.Yk=a.date;this.Gx();this.jx()}else this.EH.setDate(this.Yk);if(!this.PB)M(this.lk,false);if(!this.OB)u(this.r.desc_id).focus();this.OB=false;this.PB=false};
cB.prototype.OE=function(){if(this.C$()){var a=this.aq.value,b=new Date,c=pi(ii);Ii(c.DATEFORMATS[3],a,0,b);this.Yk=b;this.EH.setDate(this.Yk);this.jx()}else this.Gx()};
cB.prototype.jx=function(){var a=this.Yk;u(this.r.dayselect).value=a.getDate();var b=a.getMonth()+1;u(this.r.monthselect).value=b-1;u(this.r.yearselect).value=a.getFullYear();u("lhid_datefield").value=b+"/"+a.getDate()+"/"+a.getFullYear()};
cB.prototype.Gx=function(){var a=this.Yk;this.aq.value=lz(a,3)};
cB.prototype.HI=function(){if(_features.geotagging){var a=u(this.r.location_id),b=Va(a.value);if(b)this.r.geotag.bq();else this.NG()}};
cB.prototype.NG=function(){if(_features.geotagging)this.r.geotag.MG()};
cB.prototype.p0=function(a){var b=a.target;if(b){var c=u(this.r.lhui_geoTable);M(c,b.checked);this.r.geotag.fI(b.checked);if(b.checked){this.r.geotag.Al().Kr();this.r.geotag.Al().reset();this.HI()}else this.NG()}};
cB.prototype.oK=function(a){this.LQ(a.target.value=="public")};
cB.prototype.LQ=function(a){var b=m("public albums"),c=m("unlisted albums"),d=u("lhid_inheritStatus");if(a&&this.r.defaultAlbumNameTagsVisibility==1||!a&&this.r.defaultAlbumNameTagsVisibility==2||this.r.defaultAlbumNameTagsVisibility==3){B(d,hy);qf(d,"hideState");C(d,"showState")}else{B(d,iy);qf(d,"showState");C(d,"hideState")}var e=u("lhid_albumAccess");if(a)B(e,b);else B(e,c)};
var dB='<div class="lhcl_dialog"><form id="{form_id}" method="post" action="{action}" onsubmit="_d(\'{onsubmit}\'); return false"><input type="hidden" name="uname" value="{uname}" /><input type="hidden" name="dialogName" value="{dialog}" /><input type="hidden" name="redir" value="{redir}" /><input type="hidden" name="dest" value="{dest}" /><input type="hidden" name="photoop" value="{photoop}" /><input type="hidden" name="selectedphotos" value="{selected}" /><div class="lhcl_dialog_body"><h1>{header}</h1><div id="{toggle_id}"></div><div id="{body_id}"></div></div><div class="lhcl_buttons"><input type="submit" class="lhcl_default" id="{submit_id}" /><input type="button" id="{cancel_id}" value="'+
wx+'" onclick="_d(\'{oncancel}\')" /></div></form></div>',kB=m("Use most recent upload date"),lB='<input type="checkbox" name="autoDate" value="true" id="lhid_autoUpdateDate" {dateChecked} />'+kB,fB=function(){var a=[];if(_features.geotagging)a.push('<div class="lhcl_geoalbumDialog">');else a.push("<div>");var b=m("Title"),c=m("Date"),d=m("Description"),e=m("(optional)");a.push("<h2>"+b+'</h2><input type="hidden" name="aid" value="{id}" /><input type="hidden" name="albumop" value="{albumop}" /><input type="text" class="lhcl_input" id="{title_id}" name="title" value="{title}" maxlength="100"/><table class="lhcl_table"><tbody><tr><td><h2>'+
c+'</h2></td><td style="text-align: right;">');if(_features.autoupdatealbums&&_album.isdefault)a.push(lB);a.push('</td></tr></tbody></table><div style="position:relative;"><div><input id="lhid_date" class="lhcl_input" type="text" autocomplete="off" /></div><div id="lhid_dp" style="display:none"></div><input type="hidden" id="lhid_datefield" name="date" /><input type="hidden" id="{dayselect}" name="dateDay" /><input type="hidden" id="{monthselect}" name="dateMonth" /><input type="hidden" id="{yearselect}" name="dateYear" /></div><h2>'+
d+" <em>"+e+'</em></h2><textarea class="lhcl_input" name="description" id="{desc_id}">{description}</textarea>');if(_features.geotagging){var f=m("Show location on map"),g=m("Update map");a.push('<table class="lhcl_table"><tbody><tr><td><h2>'+eB+" <em>"+e+'</em></h2></td><td style="text-align: right;"><input type="checkbox" id="{lhui_showLocation_id}" {lhui_locationChecked} /><label for="{lhui_showLocation_id}">'+f+'</label></td></tr></tbody></table><input type="text" class="lhcl_input" name="location" id="{location_id}" onkeydown="_d(\'{stoptimerfunc}\')" onkeyup="_d(\'{starttimerfunc}\')" value="{location}" maxlength="100" /><img src="img/spin.gif" style="visibility:hidden" id="{spin_id}" hspace=4><br /><input type="hidden" name="lat" id="{lat_id}" value="{lat}" /><input type="hidden" name="lon" id="{lon_id}" value="{lon}" /><input type="hidden" name="latspan" id="{latSpan_id}" value="{latSpan}" /><input type="hidden" name="lonspan" id="{lonSpan_id}" value="{lonSpan}" /><input type="hidden" name="removegeo" id="{removeGeo_id}" value="false" /><table id="{lhui_geoTable}" class="lhcl_geoResultTable" style="display: none;"><tbody><tr><td colspan="2"><a href="javascript:_d(\'{geocodefunc}\')">'+
g+'</a></td></tr><tr><td width="25%" valign="top"><div id="{results_id}"></div></td><td width="75%"><div id="{map_id}" class="lhcl_mapBorder lhcl_smallMap"></div></td></tr></tbody></table><br />')}else a.push("<h2>"+eB+" <em>"+e+'</em></h2><input type="text" class="lhcl_input" name="location" id="{location_id}" value="{location}" maxlength="100" />');var h=m("For albums you want to show publicly."),i=m("For albums that you only want to share with select people");a.push('<div id="lhid_access"><p><input type="radio" name="access" value="public" id="lhid_publicaccess" {publicChecked} /> <label for="lhid_publicaccess"><span class="lhcl_public{publicSearchable}_label">'+
ax+"</span></label> <em>"+bx+"</em> &#8211; "+h+'</p><p class="lhcl_mini_text">{public_desc}</p><p><input type="radio" name="access" value="private" id="lhid_unlistedaccess" {privateChecked} /> <label for="lhid_unlistedaccess"><span class="lhcl_unlisted_label">'+$w+"</span></label> &#8211; "+i+"</p></div>");if(cA&&_features.froptin){var k=m("Name tag visibility"),n=m("name tags"),q=m("in this album"),t=m("Use my current account setting"),y=m("in my");a.push('<div id="lhid_nameTagsVisibility" class="gphoto-nametagstatus"><p class="header">'+
k+':</p><p><input id="lhid_inheritNameTags" name="albumnametagsvisibility" type="radio" value="{nameTagsInherit}" {nameTagsVisibilityInherit}> <label for="lhid_inheritNameTags">'+t+': (<span id="lhid_inheritStatus"></span> '+n+" "+y+' <span id="lhid_albumAccess" class="bold"></span>)</label></p><p><input id="lhid_hideNameTags" name="albumnametagsvisibility" type="radio" value="{nameTagsHide}" {nameTagsVisibilityHide}> <label for="lhid_hideNameTags"><span class="hideState">'+iy+' </span><span class="bold">'+
n+" </span>"+q+'</label></p><p><input id="lhid_showNameTags" name="albumnametagsvisibility" type="radio" value="{nameTagsShow}" {nameTagsVisibilityShow}> <label for="lhid_showNameTags"><span class="showState">'+hy+' </span><span class="bold">'+n+" </span>"+q+"</label></p></div>")}a.push("</div>");return a.join("")},
hB='<div><input type="hidden" name="albumop" value="{albumop}" /><input id="{album_id}" type="hidden" name="aid" /><div class="lhcl_albumlist" id=\'{albumlist}\'>'+Vx+"</div></div>",mB=m("Enter details for a new album below or {$linkStart} choose an existing album {$linkEnd}.",{linkStart:"<a href=\"javascript:_d('{choosefunc}')\">",linkEnd:"</a>"}),iB="<p>"+mB+"</p>",nB=m("Choose an album below or {$linkStart} create a new album {$linkEnd}.",{linkStart:"<a href=\"javascript:_d('{createfunc}')\">",
linkEnd:"</a>"}),jB="<p>"+nB+"</p>";function oB(){var a=m("You need Adobe\u00ae Flash\u00ae Player 8 or higher to play slideshows.");Ez.Hd(this);this.Xb=w("div");this.Xb.className="lhcl_cover";document.body.appendChild(this.Xb);this.Jf();this.nm=l(this.Jf,this);Ly(window,"resize",this.nm);window.scrollTo(0,0);RA();jz("hideVideo");this.r={close:X("func"),change:X("func"),changeCode:X("func")};Y(this.r.close,l(this.Fc,this));Y(this.r.change,l(this.sy,this));Y(this.r.changeCode,l(this.sy,this));this.ka=gz(pB,this.r);document.body.insertBefore(this.ka,
document.body.firstChild);this.Zda=document.title;this.sy();this.af=_authuser;this.Da=_album;if(!rA)u("lhid_widget_preview").innerHTML='<div class="lhcl_output" style="height:192px"><span>'+a+'</span><br /><br /><a target="_blank" href="http://www.macromedia.com/go/getflashplayer">'+Qv+"</a></div>"}
oB.prototype.ha=function(){this.V();Ez.ee(this);My(window,"resize",this.nm);Z(this.r.close);Z(this.r.change);Z(this.r.changeCode);for(var a in this)this[a]=null};
oB.prototype.Fc=function(){if(this.ka)A(this.ka);if(this.Xb)A(this.Xb);if(this.jm)this.jm();document.title=this.Zda;this.ha()};
oB.prototype.Jf=function(){var a=Math.max(document.body.offsetHeight,document.body.scrollHeight);a=Math.max(a,document.body.clientHeight);a=Math.max(a,document.documentElement.clientHeight);this.Xb.style.height=a+"px"};
var sB=function(a,b,c,d){var e="host="+window.location.hostname;e+=window.location.port?"&port="+window.location.port:"";e+=a.captions.checked?"&captions=1":"";e+=a.autoplay.checked?"":"&noautoplay=1";var f=Zi("http",null,window.location.hostname,window.location.port,"/lh/getEmbed"),g={flash_src:_flashPath+"/slideshow.swf",width:d,height:Math.round(d*2/3),font:d<=144?10:13,url:b,vars:e,album_url:c,embed_url:f.toString()};return ez(a.html.checked?qB:rB,g)};
oB.prototype.sy=function(){var a=u("lhid_slideshow_options");this.pba=a.captions.checked;this.kG=a.autoplay.checked;this.qfa=a.html.checked;this.fp=a.size.value;var b=ab(_album.photosRss);u("lhid_html").value=sB(a,b,_album.link,a.size.value);u("lhid_widget_preview").innerHTML=sB(a,b,_album.link,288)};
oB.prototype.V=function(){var a={width:this.fp};if(this.pba)a.captions=1;Uz().log(116,a)};
var wB=function(){var a="";for(var b=0;b<_user.albums.length;b++)if(_user.albums[b].count>0)a+='<option value="'+b+'">'+_user.albums[b].title+" ("+_user.albums[b].count+")</option>";if(a.length==0)return;var c={change:X("func"),changeCode:X("func"),options:a};Y(c.change,tB);Y(c.changeCode,uB);document.getElementById("lhid_body").innerHTML=ez(vB,c);tB()},
tB=function(){uB();var a=Number(document.Embed.album.value);document.getElementById("lhid_embedPreview").innerHTML=sB(document.Embed,ab(_user.albums[a].link),_user.albums[a].url,288)},
uB=function(){var a=document.Embed.size.value,b=Number(document.Embed.album.value);document.Embed.code.value=sB(document.Embed,ab(_user.albums[b].link),_user.albums[b].url,a)},
xB=m("Slideshow options"),yB=m("Select slideshow size:"),zB=m("Show captions"),AB=m("Autoplay"),BB=m("HTML Links (for MySpace)"),CB=m("Embed slideshow"),DB=m("Copy and paste this into your website:"),EB=m("Need help putting this slideshow on your website?"),FB=m("Create a portable slideshow for your website, blog, Myspace, etc."),GB=m("Slideshow preview"),HB="<h2>"+xB+"</h2><div>"+yB+'</div><select name="size" onchange="_d(\'{changeCode}\')"><option value="144">'+lv+" "+sx(144)+'</option><option value="288" selected>'+
kv+" "+sx(288)+'</option><option value="400">'+jv+" "+sx(400)+'</option><option value="600">'+Zw+" "+sx(600)+'</option><option value="800">'+Yw+" "+sx(800)+'</option></select><br /><input type="checkbox" name="captions" onclick="_d(\'{change}\')"/>'+zB+'<input type="checkbox" name="autoplay" checked onclick="_d(\'{change}\')"/>'+AB+'<br /><input type="checkbox" name="html" onclick="_d(\'{change}\')"/>'+BB+"<hr><h2>"+CB+'</h2><div class="lhcl_copy">'+DB+'</div><textarea name="code" id="lhid_html" onclick="this.select()" rows="4"></textarea>',
IB='<a href="http://picasa.google.com/support/bin/answer.py?answer=66969" target="_blank">'+EB+"</a>",pB='<div id="lhid_slideshow" class="lhcl_dialog"><div class="lhcl_dialog_body"><h1>'+FB+'</h1><table><tr><td style="vertical-align:top"><form id="lhid_slideshow_options">'+HB+'</form></td><td><span class="lhcl_frameblock" style="display:block;"><div class="lhcl_small">'+GB+'</div><div id="lhid_widget_preview"></div></span><div class="lhcl_small">'+IB+'</div></td></tr></table></div><div class="lhcl_buttons"><input type="button" onclick="_d(\'{close}\')" value="'+
Zx+'" class="lhcl_default" id="{submit_id}" /></div></div>',vB='<p class="lhcl_h1">'+FB+'</p><table class="lhcl_slideshow"><tr><td><form name="Embed"><h2>Select an album</h2><select name="album" onchange="_d(\'{change}\')">{options}</select><br /><br />'+HB+'</form></td><td><span class="lhcl_frameblock" style="display:block;"><div class="lhcl_normal" style="text-align:center;">'+GB+'</div><div id="lhid_embedPreview"></div><br clear="all" /></span><div  style="text-align:center;">'+IB+"</div></td></tr></table>",
rB='<embed type="application/x-shockwave-flash" src="{flash_src}" width="{width}" height="{height}" flashvars="{vars}&RGB=0x000000&feed={url}" pluginspage="http://www.macromedia.com/go/getflashplayer"></embed>',qB='<div style="width:{width}px;font-family:arial,sans-serif;font-size:{font}px;"><div>'+rB+'</div><span style="float:left;"><a href="{album_url}" style="color:#3964c2">View Album</a></span><div style="text-align:right;"><a href="{embed_url}" style="color:#3964c2">Get your own</a></div></div>';;var KB=function(a){if(a)Oi(JB,"true");else Ri(JB);document.location.reload()};
La(Yr+"setForceExternalIp",KB);var LB=function(){var a=Pi(JB,"false");return a!="true"?false:true},
NB=function(a){var b="";if(LB()){var c={force:"false",view:"internal"};b=ez(MB,c)}else{var c={force:"true",view:"external"};b=ez(MB,c)}var d=document.getElementById(a);d.innerHTML=b};
La(Yr+"renderInternalExternalToggle",NB);var JB="GOOGLE_FORCE_EXTERNAL_IP",MB='<a href="javascript:pwa.setForceExternalIp({force});">View {view} version</a>';var OB=function(a,b){this.ue=a;this.zb=Math.max(b,1);this.jh=-1;this.gi=0;this.gn=0;this.Hm=Math.ceil(this.ue/this.zb);this.Tca=false;this.Sca=false;this.wH=1};
o(OB,J);var PB="totalpageschanged",QB="currentpagechanged",RB="currentitemchanged",SB="totalitemschanged";OB.prototype.fZ=function(a){if(a<0||a>=this.Hm)return null;var b;b=this.gi==a?this.gn:this.zb*a;return{index:a,startItemIndex:b,size:Math.min(this.zb,this.ue-b)}};
OB.prototype.Tg=function(a){if(this.jh==a||0==this.ue)return;if(a<0||a>=this.ue)if(this.ue==0)return;else if(a<0)a=0;else if(a>=this.ue)a=this.ue-1;if(isNaN(a))return;var b=false;if(this.Tca){var c=this.qu(a);if(isNaN(c))return;if(c!=this.gi){this.gi=c;b=true}}this.wH=this.jh<a?1:-1;this.jh=a;this.dispatchEvent(RB);if(b)this.dispatchEvent(QB)};
OB.prototype.xd=function(){return this.jh};
OB.prototype.WI=function(){return this.wH};
OB.prototype.Vf=function(a){if(a<0||a>this.Hm)return-1;return Math.floor(a*this.zb)};
OB.prototype.qu=function(a){if(a<0||a>=this.ue)return-1;return Math.floor(a/this.zb)};
OB.prototype.ON=function(){var a=Math.ceil(this.ue/this.zb),b=this.jh<0?-1:this.qu(this.jh);if(a!=this.Hm){this.Hm=a;if(this.gi>0&&this.gi>=this.Hm)this.GD(this.Hm-1);this.dispatchEvent(PB)}if(b>=0&&b!=this.$k){this.$k=b;this.dispatchEvent(QB)}if(this.jh>=this.ue){this.jh=this.ue-1;this.dispatchEvent(RB)}};
OB.prototype.ym=function(a,b){var c=this.ue!=a,d=pa(b)?!b:true;this.ue=a;if(a==0){this.$k=0;if(d)this.dispatchEvent(QB);return}this.ON();if(c&&d)this.dispatchEvent(SB)};
OB.prototype.Kj=function(){return this.ue};
OB.prototype.tq=function(){return this.Hm};
OB.prototype.A4=function(){this.GP(1)};
OB.prototype.R5=function(){this.GP(-1)};
OB.prototype.GP=function(a){var b=this.gn+a*this.zb;if(b<0)b=0;if(b>=this.ue)b=this.ue-this.zb;var c=this.qu(b);if(isNaN(c))return;this.gn=b;this.gi=c;this.dispatchEvent(QB)};
OB.prototype.hd=function(){return this.gi};
OB.prototype.GD=function(a,b,c){if(a<0||a>=this.Hm||isNaN(a))return;var d=!b&&a!=this.gi;this.gi=a;this.gn=this.gi*this.zb;var e=false;if(this.Sca)if(this.jh!=this.gn){this.wH=this.jh<this.gn?1:-1;this.jh=this.gn;e=!c}if(d)this.dispatchEvent(QB);if(e)this.dispatchEvent(RB)};
OB.prototype.Wi=function(a){this.zb=a;this.ON()};
OB.prototype.Yc=function(){return this.zb};var TB=function(a,b,c,d){this.k=a;this.AT=b;this.tb=b;this.ZP=d||0;this.HF=0;this.dk=c||b*2;this.aa(true);this.k.addEventListener(PB,l(this.aa,this,false));this.k.addEventListener(QB,l(this.aa,this,false))};
o(TB,J);var UB=function(a){D.call(this,"windowscroll",a);this.active=a.HF};
o(UB,D);TB.prototype.aa=function(a){var b=this.k.tq(),c=b-1,d=Math.min(c,this.ZP+(this.tb-1)),e=this.k.hd(),f=Math.min(this.dk,this.AT+e);f=Math.max(f,this.AT);var g=this.dk;if(f>b){f=b;g=b;d=c}var h=Math.ceil(g/2),i=-1,k=-1;if(f==this.dk)if(e>c-h){var n=c-e;k=this.dk-1-n;i=c-this.dk+1}else{k=h;i=e-h}else{i=0;k=e}this.tb=f;this.ZP=i;this.HF=k;if(!a)this.dispatchEvent(new UB(this))};
TB.prototype.BY=function(){return this.ZP};
TB.prototype.S=function(){return this.tb};
TB.prototype.RX=function(){return this.HF};
var VB=function(a,b,c){var d=c||b*2;return new TB(a,b,d,a.hd())};var WB=function(a){P.call(this);this.Sc=a||""};
o(WB,P);WB.prototype.m=function(){this.e=w("td");this.I=w("div");this.fj=w("div");x(this.e,this.I);x(this.e,this.fj)};
WB.prototype.n=function(){WB.f.n.call(this);this.fda=F(this.e,I,l(this.we,this))};
WB.prototype.u=function(){H(this.fda);WB.f.u.call(this)};
WB.prototype.we=function(){if(this.Ej)this.Ej()};
WB.prototype.t8=function(){this.show(true);of(this.I,this.Sc+"pagerfirst");B(this.fj,"");delete this.Ej};
WB.prototype.b9=function(a){this.show(true);of(this.I,this.Sc+"pagerprev");B(this.fj,Hw);of(this.e,this.Sc+"firstCell");of(this.fj,this.Sc+"firstCellTxt");this.Ej=l(a.R5,a)};
WB.prototype.S8=function(a){this.show(true);of(this.I,this.Sc+"pagernext");of(this.fj,this.Sc+"pagernextnum");B(this.fj,Gw);this.Ej=l(a.A4,a)};
WB.prototype.I8=function(){this.show(true);of(this.I,this.Sc+"pagerlast");B(this.fj,"");delete this.Ej};
WB.prototype.g8=function(){this.show(true);of(this.I,this.Sc+"pagercurrent");of(this.fj,this.Sc+"pagercurrentpagenum");delete this.Ej};
WB.prototype.Vi=function(a,b){this.show(true);of(this.I,this.Sc+"pagerpage");B(this.fj,b+1);of(this.fj,this.Sc+"pagerpagenum");this.Ej=l(a.GD,a,b)};
WB.prototype.show=function(a){M(this.e,a)};
var XB=function(a,b,c){P.call(this);this.U(a);this.Rea=b;this.Sc=c||"";this.Gd=VB(this.k,this.Rea)};
o(XB,P);XB.prototype.la=false;XB.prototype.m=function(){this.e=v("table",{id:"lhid_pager"});of(this.e,this.Sc+"pager");var a=w("tbody");this.DE=w("tr");x(this.e,a);x(a,this.DE)};
XB.prototype.n=function(){P.prototype.n.call(this);this.k.addEventListener(QB,this.aa,false,this);this.k.addEventListener(PB,this.aa,false,this)};
XB.prototype.u=function(){this.k.removeEventListener(QB,this.aa,false,this);this.k.removeEventListener(PB,this.aa,false,this);this.show(false);P.prototype.u.call(this)};
XB.prototype.aa=function(){var a=this.Gd.S(),b=0;b=a<=1?-1:1;if(b!=0&&this.e){this.la=b>0;M(this.e,this.la)}if(!b)return;var c=this.Gd.BY(),d=this.Gd.RX(),e=d-c;if(!this.yk)this.yk=[];var f=this.yk[0];if(!f){f=new WB(this.Sc);this.yk[0]=f;this.Z(f);f.C(this.DE)}if(c==0&&e==0)f.t8();else f.b9(this.k);for(var g=0;g<a;++g){var h=this.yk[g+1];if(!h){h=new WB(this.Sc);this.yk[g+1]=h;this.Z(h);h.C(this.DE)}h.Vi(this.k,c+g);if(d==g)h.g8()}var i=a+1,k=this.yk[i];if(!k){k=new WB(this.Sc);this.yk[i]=k;this.Z(k);
k.C(this.DE)}if(this.k.hd()+1==this.k.tq())k.I8();else k.S8(this.k);for(var g=i+1;g<this.yk.length;++g){var n=this.yk[g];if(n)n.show(false)}};
XB.prototype.gZ=function(){return this.Gd};
XB.prototype.show=function(a){if(this.la!=a){this.la=a;if(a)this.aa();if(this.e)M(this.e,a)}};var YB=function(a){this.mc=a;this.Q=this.mc.Wa();this.e=w("td");this.Kc=w("a");this.av=w("img");zh(this.av,"144px","96px");this.Kg=w("td");this.ba=null;this.NC=0;this.Sz=null;x(this.Kc,this.av);x(this.e,this.Kc);M(this.e,false)},
ZB=Ju,$B={PHOTO_DESCRIPTION:"",PHOTO_TAGS:ZB,ALBUM_TITLE:"",ALBUM_DESCRIPTION:"",ALBUM_LOCATION:""};YB.prototype.CT=function(a){this.av.src="img/spacer.gif";this.ba=null;if(!a)this.Kg.innerHTML=Vx};
YB.prototype.Fd=function(a){if(a){C(this.e,"lhcl_activecell");C(this.Kg,"lhcl_activecell")}else{qf(this.e,"lhcl_activecell");qf(this.Kg,"lhcl_activecell")}};
YB.prototype.yaa=function(a,b){x(a,this.e);x(b,this.Kg)};
YB.prototype.show=function(a){M(this.e,a);M(this.Kg,a)};
YB.prototype.select=function(a){if(this.mc&&this.ba&&a.Vl(0)){var b=this.NC;this.mc.Lo(b);this.mc.Hh.Tg(b);var c=aC();if(c.Wl())c.vm(b+"+1");location.href=this.Kc.href;a.stopPropagation()}};
YB.prototype.aa=function(a,b,c){if(!a||this.ba&&this.ba.id==a.id&&this.Q==c)return;this.ba=a;this.Q=c;this.NC=b;var d=this.ba.thumbnail(3);if(d){var e=location.href.replace(/#.*/,""),f=a.Mg?d.url:"img/spacer.png",g=e+"#"+b;this.Kc.href=g;this.Kc.title=a.t;if(this.Sz)G(this.Kc,I,this.Sz);this.Sz=l(this.select,this);F(this.Kc,I,this.Sz);this.av.src=f;zh(this.av,d.width+"px",d.height+"px")}var h=a.snippet||"",i=h&&a.snippetType?$B[a.snippetType]:"",k=a.user,n="/"+k.name,q=a.gd?a.gd():a.album,t=q.link,
y=q.title,z=a.DL?a.DL(this.mc.Wa()):a.truncated,O=z?ez('<div class="truncated_link"><a href="{albumLink}">[{moremessage}]</a></div>',{albumLink:t,moremessage:Iu}):"",aa=p(pb(k.nickname,24)),dc='<a href="'+n+'">'+aa+"</a>",Re=tx(dc),kG=ez('<div><div class="album_link"><span class="">{albumTitle}</span></div><div class="owner_gallery_link">{ownerLink}</div><div class="context_info">{snippetType} {snippet}</div>{truncatedLink}</div>',{albumLink:t,albumTitle:p(pb(y,24)),ownerLink:Re,snippet:pb(h,24),
snippetType:i,truncatedLink:O});this.Kg.innerHTML=kG};
var bC=function(a,b,c){this.P=a;this.zr=b;this.Hh=c;this.Qfa=0;this.gridMax=21;this.gridMin=18;this.Du=this.gridMin;this.maxColumns=7;this.minColumns=3;this.maxRows=6;this.cellSize=205;this.xB=[];this.AB=[];this.Rk=[];this.je=-1;this.ca=-1;this.Yd=false;this.Tx=0;this.Kz=0;this.Jz=0;this.Dh=true;var d=w("tr"),e=w("td");d.appendChild(e);x(this.P,d);e.innerHTML='<div class="lhcl_search_results_count_info" style="overflow: hidden;" />';this.Zm=e.firstChild;this.Zm.innerHTML=Vx;for(var f=0;f<this.maxRows;++f){var g=
this.B3();M(g,false);this.xB.push(g);x(this.P,g);g=this.A3();M(g,false);this.AB.push(g);x(this.P,g)}var h=this.maxRows*this.maxColumns;for(var f=0;f<h;++f)this.Rk.push(new YB(this));this.dw=w("td");var i=w("tr");i.appendChild(this.dw);x(this.P,i);this.Hh.addEventListener(QB,l(this.Ao,this))};
o(bC,J);bC.prototype.DT=function(){this.nG(false)};
bC.prototype.ET=function(){this.nG(true)};
bC.prototype.nG=function(a){for(var b=0;b<this.Rk.length;++b)this.Rk[b].CT(a)};
bC.prototype.cs=function(a){if(this.Tx){Sg(this.Tx);this.Tx=0;if(this.Jz>0){if(Ka()-this.Jz>45000)alert(Eu);else this.Tx=Rg(l(this.cs,this,false),1000);this.Jz=-1;this.Ao()}}if(!this.bo){this.bo=new R("lhcl_modal-dialog");var b=new Xk;this.bo.Rg(b);this.bo.Xi(Yt);this.bo.hc('<img src="img/spin.gif" />');this.bo.C()}if(a){this.Jz=Ka();this.Tx=Rg(l(this.cs,this,false),1000);this.DT();this.bo.Qa()}this.bo.M(a)};
bC.prototype.Wo=function(a){this.cs(false);if(this.Kz>0){Sg(this.Kz);this.Kz=0}if(!this.P)return;this.Yd=a;if(a)this.Kz=Rg(l(this.cs,this,true),1000);else this.cs(false)};
bC.prototype.Lo=function(a){var b=this.Hh.fZ(this.Hh.hd());if(!b)return;var c=b.startItemIndex;if(a>=c&&a<=c+this.Du){var d=a-c,e=this.Rk[d];if(this.Fy&&this.Fy!=e){this.Fy.Fd(false);this.Fy=null}if(e){e.Fd(true);this.Fy=e}}};
bC.prototype.lP=function(a){var b=a||Qe(),c=80,d=Math.max(b.width,c*this.je),e=Math.min(200,c*this.ca),f=u("lhid_listview");if(f)f.setAttribute("style","width: "+d+"px; height: "+e+"px;")};
bC.prototype.Kr=function(a){if(a.width+a.height<1)a={width:document.body.clientWidth,height:document.body.clientHeight};var b=Math.floor(a.width/this.cellSize);b=Math.min(this.maxColumns,b);b=Math.max(this.minColumns,b);var c=Math.floor(this.gridMax/b);c=Math.min(this.maxRows,c);var d=false;if(b!=this.je){this.je=b;d=true}if(c!=this.ca){this.ca=c;d=true}if(d){this.lP(a);var e=this.je*this.ca;this.Hh.Wi(e);this.Dh=true;if(this.Du!=e){this.Du=e;this.Ao();this.dispatchEvent({type:"gridsizechanged",gridSize:e})}}};
bC.prototype.ae=function(a){this.zr=a;if(!a.isLoaded())this.lP()};
bC.prototype.yb=function(){if(!this.Dh)return;this.Dh=false;var a=this.Du;this.Hh.Vf(this.Hh.hd());var b=0,c=this.xB[0],d=this.AB[0];this.Hh.xd();var e=0;for(;e<a;++e){var f=this.Rk[e],g=!(e%this.je);if(g){c=this.xB[b];d=this.AB[b];++b;We(c);We(d);M(c,true);M(d,true)}f.yaa(c,d);f.show(true)}for(var h=b;h<this.maxRows;++h){var i=this.xB[h],k=this.AB[h];if(i)M(i,false);if(k)M(k,false)}};
bC.prototype.Ao=function(){this.yb();var a=this.Du,b=this.Hh.hd(),c=this.Hh.Vf(b);this.Hh.xd();var d=0,e=0,f=a,g=Math.min(1000,this.zr.Yb());if(g<c+a)f=g-c;for(;e<a;++e){var h=this.Rk[e],i=c+e,k=this.zr.Me(i);h.show(true);h.aa(k,i,this.zr);if(!k)continue;++d}e=f;for(;e<this.Rk.length;++e){var h=this.Rk[e];if(h)h.show(false)}if(g<=0)M(this.Zm,false);else{B(this.Zm,ox(c+1,c+d,this.zr.ms()));M(this.Zm,true)}};
bC.prototype.B3=function(){return v("tr",{className:"lhcl_imgrow"})};
bC.prototype.A3=function(){return v("tr",{className:"lhcl_inforow"})};
bC.prototype.Wa=function(){return this.zr};var cC=function(a,b){this.P=a;this.T=b;this.e=w("td");of(this.e,"lhcl_searchtab");this.jQ=w("div");x(this.e,this.jQ);this.WB=w("span");B(this.WB,this.T.name);x(this.jQ,this.WB);F(this.WB,I,l(this.Is,this));this.Zm=w("span");x(this.jQ,this.Zm);this.es=w("td");of(this.es,"lhcl_nav_border");this.es.setAttribute("width","9");this.es.innerHTML='<img src="img/spacer.gif" width="9" height="1" />';M(this.es,false);x(this.P,this.e);x(this.P,this.es);this.jda=-1;this.Cda=-1;var c=b.Q;c.addEventListener(Es,
l(this.fy,this));if(c.isLoaded())this.fy();else{this.jda=c.addEventListener(Ds,l(this.fy,this));this.Cda=c.addEventListener(Cs,l(this.fy,this))}};
cC.prototype.fy=function(){var a=this.T.Q;if(a.isLoaded()){var b=a.ms();B(this.Zm," ("+b+")")}};
cC.prototype.os=function(){var a=aC(),b=a.Tk==this.T.search_corpus,c,d="";if(b)c="lhcl_searchtab_on";else{d="lhcl_fakelink";c="lhcl_searchtab"}M(this.es,b);of(this.WB,d);of(this.e,c);var e=a.Wl()||b;M(this.e,e)};
cC.prototype.Is=function(){var a=u("lhid_search_uname");a.value=this.T.search_corpus=="F"?_user.name:"";var b=dC,c=b.Tk==this.T.search_corpus;if(c)return;b.Ph(this.T.search_corpus)};;var hC=function(){Ez.Hd(this);this.wa=new mt;var a=aj(location.href);this.ze=a.Ga("q");if(!this.ze||this.ze==undefined){this.ze=a.Ga("tags");if(!this.ze||this.ze==undefined)this.ze=""}this.Wu=a.Ga("hl");this.Tk=a.Ga("psc")||"G";this.oa=-1;this.tz=a.Ga("cuname")||a.Ga("uname");this.af=_authuser;this.hX=!(!a.Ga("filter"));this.zb=24;this.ifa=a.Ga("id");this.Ka=null;this.ba=[];this.GL=new K(this);this.o2=false;dC=this;this.Hfa=1;this.j=new K(this);this.ob=new Dg(document);this.l2();this.ei();eC=this.Wl();
var b=u("lhid_query");if(b&&this.ze)B(b,ob(this.ze,40));if(!fC)fC=this.Tk;else this.Tk=fC;if(!gC)gC=this.tz;else this.tz=gC;this.Ka=this.Fn().Q;this.ba=this.Ka.ta;this.RE();this.iD()},
fC,dC,iC;o(hC,J);var jC=["S","C","F","G"],kC={},gC=null,eC=true;hC.prototype.Cd=-1;var aC=function(){return dC};
hC.prototype.vm=function(a){var b=location.toString();if(b.indexOf("#")>0)b=b.replace(/#.*/,"");var c=this.Wl(),d=b+"#"+a;if(eC!=c)location=d;else location.replace(d)};
hC.prototype.hy=function(a){this.vm(this.fC(a))};
hC.prototype.ei=function(){this.j.g(this.ob,"key",this.Wf);this.RE()};
hC.prototype.PH=function(){this.j.ma();this.GL.ma()};
hC.prototype.Wl=function(){return false};
hC.prototype.ha=function(){Ez.ee(this);this.PH();this.ob.b()};
hC.prototype.EE=function(){if(this.Ka){this.Ka.Qr(false);this.Ka.Sj(this.oa)}this.hX=false};
hC.prototype.lm=function(a){this.ba=a.currentTarget.ta;var b=u("lhid_searchload");M(b,false);if(!this.o2){this.o2=true;var c=u("lhid_searchmain");M(c,true);this.Wg(true)}};
hC.prototype.jp=function(a,b){var c=this.oa+a,d=this.Ka.Yb();if(b){while(c<0)c+=d;c%=d}if(c<0||c>=d)return false;this.hy(c);return true};
hC.prototype.Xh=iA.prototype.Xh;hC.prototype.Yh=iA.prototype.Yh;hC.prototype.Ds=iA.prototype.Ds;hC.prototype.Wf=iA.prototype.Wf;hC.prototype.gp=iA.prototype.gp;hC.prototype.lp=iA.prototype.lp;hC.prototype.Nk=iA.prototype.Nk;hC.prototype.rd=iA.prototype.rd;hC.prototype.de=iA.prototype.de;hC.prototype.Gb=function(){var a=this.oY();if(a&&a.vsurl)return true};
hC.prototype.vy=function(){return true};
hC.prototype.Ay=function(){};
hC.prototype.ry=function(){return false};
hC.prototype.Lk=function(){};
hC.prototype.Fc=function(){this.PH();this.Wg(false)};
hC.prototype.Wg=function(){};
hC.prototype.V=function(){Uz().log(this.Cd,this.wh())};
hC.prototype.wh=function(){var a={ts:Ka()},b=this.Fn();if(b){var c=b.Q;if(c&&c.bc()){var d=c.bc().lq();Uc(a,d)}}return a};
hC.prototype.fe=function(a){var b=false;if(!a){this.oa=0;b=true}else if(a.length<2){this.oa=0;b=true}else{this.oa=parseInt(a[1],10);if(isNaN(this.oa)){this.oa=0;b=true}if(this.oa<0){this.oa=0;b=true}}if(b)this.hy(this.oa);var c=this.Wl();if(eC!=c){eC=c;this.dv()}};
hC.prototype.fC=function(a){return a};
hC.prototype.dv=function(){dC=this;this.Ph(fC);this.iD();this.ei();this.Wg(true);this.V(this.wh())};
hC.prototype.oY=function(){if(!this.Ka)return null;return this.Ka.Me(this.oa)};
hC.prototype.kH=function(a){var b=a||this.Tk;if("G"==b)return Hu;else if("S"==b)return Gu;else if("C"==b)return Fu;else if("F"==b){var c=this.wa.Ne(this.tz,this.tz);if(c)return ux(c.nickname)}return Hu};
hC.prototype.iD=function(){var a=u("lhid_searchbar");for(var b=0;b<jC.length;++b){var c=kC[jC[b]];if(!c)continue;if(!c.tab)c.tab=new cC(a,c);c.tab.os()}};
hC.prototype.Fn=function(){var a=kC[fC];if(!a){fC="G";a=kC.G}return a};
hC.prototype.RE=function(){this.GL.ma();if(this.Ka)this.GL.g(this.Ka,Cs,l(this.lm,this))};
hC.prototype.Ph=function(a){var b=kC[a],c=this.Fn();if(c)c.savedIndex=this.oa;var d=false;if(b!=c){d=true;this.Ka=b.Q}this.Tk=b.search_corpus;this.Ka=b.Q;this.ba=this.Ka.ta;var e=b.savedIndex;fC=a;this.iD();this.RE();this.hy(e);if(d)this.V(this.wh());return e};
hC.prototype.iq=function(){var a=this.Fn();return a?a.lastError:null};
hC.prototype.OG=function(){var a=this.Fn();if(a)a.lastError=null};
hC.prototype.Tt=function(a,b){a.lastError=b};
hC.prototype.fw=function(){var a=u("lhid_searchload");M(a,false)};
hC.prototype.TZ=function(){return"LH_SEARCH_CONTEXT_INFO"};
hC.prototype.l2=function(){if(!iC)lC(this,this.TZ());var a=[];for(var b in kC){var c=kC[b],d=c.Q;d.addEventListener(Ds,l(this.fw,this));d.addEventListener("error",l(this.Tt,this,c));if(b==this.Tk){if(!d.isLoaded())d.Sj(0)}else a.push(d)}for(var e=0;e<a.length;++e)if(!a[e].isLoaded())a[e].Sj(0)};
var lC=function(a,b){if(iC)return;iC=window[b];for(var c in iC){var d=iC[c],e=false;d.savedIndex=0;if("S"==d.search_corpus&&(!a.af.name||!a.af.user))continue;if(pa(d)&&pa(d.search_corpus)){if(d.is_primary&&d.search_corpus=="G"&&a.hX==1)e=true;else if(d.search_corpus=="G")e=true;var f=a.wa.ZJ(this.ze);f.KD(function(i){var k=i.thumbnail();if(k)(new Image).src=k.url});
var g=f.bc();g.Qr(e);g.uP(a.ze,e);if(a.Wu)g.z8(a.Wu);g.SO(d.search_corpus);var h=a.af.name;if(d.search_corpus=="F")h=d.target_user||h;g.TO(h);f.Wi(a.zb);if(d.json)f.DN(0,d.json,d.is_primary);d.Q=f;kC[d.search_corpus]=d}}};var mC=function(a){hC.call(this);this.$k=-1;var b=u("lhid_rightbox");M(b,false);var c=w("tr"),d=w("td");this.Mv=w("table");this.Bv=w("tbody");this.X=null;this.gc=null;this.Bv.id="lhid_listview";Pe(this.Bv,{"class":"lhcl_search_results_content"});var e=u("lhid_searchmain");x(e,c);x(c,d);x(d,this.Mv);x(this.Mv,this.Bv);this.Ch=Qe(window);this.dw=null;this.fe(a)};
o(mC,hC);mC.prototype.Cd=118;mC.prototype.Wl=function(){return true};
mC.prototype.ei=function(){mC.f.ei.call(this);this.j.g(window,mg,l(this.nb,this))};
mC.prototype.nb=function(){if(this.gc){this.Ch=Qe(window);this.gc.Kr(this.Ch)}};
mC.prototype.rs=function(){var a=u("lhid_searchmain");M(a,true);if(this.gc)this.gc.Ao();if(this.km){this.km.gZ().aa();this.km.aa()}if(this.gc){this.ps(this.iq());this.gc.Lo(Math.max(this.oa,0))}};
mC.prototype.Wg=function(a){if(this.km)this.km.show(a);M(this.Mv,a);M(this.Bv,a);if(!a)this.LP(null)};
mC.prototype.wh=function(){var a=mC.f.wh.call(this);if(this.X){a.start=this.X.Vf(this.X.hd());a.num=this.X.Yc()}return a};
mC.prototype.fe=function(a){mC.f.fe.call(this,a);if(!this.Ka){var b=this.Fn();this.Ka=b.Q}if(!this.X){this.X=new OB(0,this.zb);this.X.ym(this.Ka.Yb());this.gc=new bC(this.Bv,this.Ka,this.X);var c=this.X,d=function(k){c.Wi(k.gridSize)};
this.gc.addEventListener("gridsizechanged",d);this.gc.Kr(Qe());var e=u("lhid_searchmain");this.dw=u("lhid_searchbody");this.PL=null;var f=w("tr"),g=w("td");g.align="center";var h=w("center");h.id="pager";x(g,h);x(f,g);x(e,f);this.km=new XB(this.X,10);this.km.C(h);this.X.addEventListener(QB,l(this.rj,this));this.Ka.Sj(this.oa);var i=u("lhid_searchload");M(i,false)}if(this.oa+1>=this.Ka.Yb())this.oa=Math.max(0,this.Ka.Yb()-1);this.X.GD(this.X.qu(this.oa));this.X.Tg(this.oa);this.rs()};
mC.prototype.fC=function(a){return a+"+1"};
mC.prototype.dv=function(){mC.f.dv.call(this);var a=Qe(window);if(this.Ch.width!=a.width||this.Ch.height!=a.height)this.nb()};
mC.prototype.rj=function(){var a=this.X.hd(),b=this.X.Vf(a),c=this.X.Yc();if(this.oa<b||this.oa>=b+c)this.hy(b);var d=this.Ka.ua(b,c,function(){}),
e=this.X.hd(),f=-1;f=this.$k<e?b+c:b-c;this.$k=e;if(f>=0&&f<this.Ka.Yb())this.Ka.ua(f,c,function(){});
this.gc.ET();this.gc.Wo(!d);this.ps(this.iq());this.V(this.wh())};
mC.prototype.LP=function(a,b,c){if(this.PL)A(this.PL);var d=u("lhid_searchload");if(a)if(c&&c.parentNode){x(c.parentNode,a);c.parentNode.insertBefore(a,c)}else{x(this.dw,a);if(b)if(d)this.dw.insertBefore(a,d)}this.PL=a};
mC.prototype.aY=function(){var a=[],b=["S","C","F","G"];for(var c=0;c<b.length;++c){var d=kC[b[c]];if(!d)continue;if(d.Q)if(d.Q.Yb()>0)a.push(d)}if(a.length==0)return null;var e=w("div"),f=w("div");f.className="lhcl_search_results_help";var g=w("div");g.className="lhcl_search_results_no_results";var h=p(this.ze);g.innerHTML=Dy('<span class="lhcl_search_results_query">'+Xy(h)+"</span>",this.kH());var i=m("However, photos that matched {$query} were found in the following locations:",{query:'<span class="lhcl_search_results_query">'+
h+"</span>"});f.innerHTML=i;var k=w("div");k.className="lhcl_search_results_help_related";x(e,g);x(e,f);x(f,k);for(var c=0;c<a.length;++c){var d=a[c],n=w("span");n.className="lhcl_fakelink";B(n,d.name);x(k,n);F(n,I,l(this.Ph,this,d.search_corpus));var q=w("span");B(q," ("+d.Q.ms()+") ");x(k,q)}return e};
mC.prototype.ps=function(a){var b=null;a=a||this.iq();var c=false;if(a){c=true;var d="";if(Fs==a.status)d=Ox(a.value);else if(Gs==a.status){var e=m("You entered a query string that was too long.  Please enter one that is less than 128 characters.");d=e}else if("empty_query_string"==a.status||this.ze==""){var f=m('You didn\'t enter any query in the search box. Please enter some terms in the search box and click the "Search Photos" button again.');d=f}else{var g=m("Unknown search error: {$code}",{code:a.value});
d=g}if(d){b=w("div");of(b,"lhcl_error_msg");B(b,d)}}M(this.Mv,true);var h;if(!b&&this.X&&this.Ka.isLoaded()){var i=this.X.hd(),k=this.X.tq(),n=i+1>=k,q=this.Ka.bc().Lq(),t=this.Ka.Yb(),y=0==t,z=false;if(n&&q&&t>0&&t<this.Ka.ms()){var O=m("In order to show you the most relevant results, we have omitted some entries very similar to those already displayed.<br>If you like, you can {$beginLink}repeat the search with the omitted results included.{$endLink}",{beginLink:'<span class="lhcl_fakelink" id="lhid_uncrowded_link">',
endLink:"</span>"}),aa=O,dc=u("lhid_uncrowded_link");if(dc)A(dc);b=v("div",{className:"lhcl_search_crowded_results"});x(b,Ve(aa));z=true}if(!b&&n&&t>1000){b=w("div");of(b,"lhcl_error_msg");B(b,Ox(this.X.Vf(i)+1));c=true}if(y&&!b){M(this.Mv,false);b=this.aY();if(!b)b=gz(Ey,{query:Xy(p(this.ze)),corpusName:this.kH()})}}this.OG();this.LP(b,c,z?u("pager"):null);if(z)h=u("lhid_uncrowded_link");if(h)F(h,I,l(this.EE,this))};
mC.prototype.EE=function(){if(this.Ka){var a=this.X.Vf(this.X.hd());this.Ka.bc().Qr(false);this.Ka.reset();this.Ka.ua(a,this.X.Yc(),function(){});
if(this.gc)this.gc.Wo(true)}};
mC.prototype.lm=function(a){mC.f.lm.call(this,a);if(this.X)this.X.ym(Math.min(1000,this.Ka.Yb()));this.rs();if(this.gc)this.gc.Wo(false)};
mC.prototype.Wf=function(a){if(YA(a))return;if(a.keyCode==13){window.location.replace(window.location.href.replace(/#.*/,"#"+this.oa));a.stopPropagation();return}mC.f.Wf.call(this,a)};
mC.prototype.Ph=function(a){if(this.Ka&&this.Ka.Tk==a)return;if(this.gc)this.gc.Dh=true;var b=mC.f.Ph.call(this,a);if(this.X){var c=Math.min(1000,this.Ka.Yb());b=Math.min(b,c-1);b=Math.max(b,0);this.X.ym(c);this.X.Tg(b);this.gc.ae(this.Ka);var d=true;if(this.Ka.isLoaded()&&this.Ka.Yb()>0)d=this.Ka.Sj(b);if(d){if(ve)Rg(l(this.gc.Ao,this.gc));else this.gc.Ao();this.ps(this.iq());this.gc.Wo(false)}else this.gc.Wo(true);if(this.km)this.km.aa()}};
mC.prototype.fw=function(){mC.f.fw.call(this);if(this.gc)this.gc.Wo(false);this.ps(this.iq())};
mC.prototype.Tt=function(a,b){mC.f.Tt.call(this,a,b);if(fC==a.search_corpus){this.ps();this.OG()}};;;;function nC(a,b){this.Gf=a;this.e=b;this.xp=w("div");this.xp.className="lhcl_bandselect";Ch(this.xp,0.2);this.lo=l(this.mw,this);this.cm=l(this.hm,this);this.dm=l(this.Li,this);this.Yv=l(this.im,this);this.Rx=[];this.Dd={};this.X9=s||ve;this.hM=false;F(this.e,"mousedown",this.lo);this.e.onselectstart=function(){return false}}
nC.prototype.mw=function(a){G(this.e,"mousedown",this.lo);F(document,"mousemove",this.cm);F(document,"mouseup",this.dm);F(document,"mouseout",this.Yv);var b=th(),c=a.clientX+b.scrollLeft,d=a.clientY+b.scrollTop;this.zs=c;this.Bs=d;var e=this.xp.style;e.left=c+"px";e.top=d+"px";e.width="0";e.height="0";this.KL=new ce(d,c,d,c);this.Rx.length=0;if(!(a.shiftKey||a.ctrlKey))this.Gf.Pd();document.body.appendChild(this.xp);a.stopPropagation();a.preventDefault()};
nC.prototype.hm=function(a){var b=th(),c=a.clientX+b.scrollLeft,d=a.clientY+b.scrollTop,e=new ce(Math.min(d,this.Bs),Math.max(c,this.zs),Math.max(d,this.Bs),Math.min(c,this.zs)),f=this.xp.style;f.left=e.left+"px";f.top=e.top+"px";f.width=e.right-e.left+"px";f.height=e.bottom-e.top+"px";var g=de(e,this.KL),h=sz(g,this.KL),i=sz(g,e),k=a.ctrlKey?2:1;for(var n=0;n<h.length;n++)this.zO(e,h[n],k);for(var n=0;n<i.length;n++)this.zO(e,i[n],0);this.KL=e};
nC.prototype.zO=function(a,b,c){var d=this.Gf.Wt(b.right,b.top),e=this.Gf.Wt(b.left,b.bottom),f=this.Gf.slide,g=false;for(var h=d.row;h<=e.row;h++){var i=h*d.rowLength+e.col,k=Math.min(h*d.rowLength+d.col,(h+1)*d.rowLength-1);k=Math.min(k,f.length-1);for(var n=i;n<=k;n++){var q=f[n],t=q.mu(),y=uh(t),z=new ce(y.y,y.x+t.offsetWidth,y.y+t.offsetHeight,y.x);if(c==0){if(!rz(a,z)){tc(this.Rx,q);g=true;this.Dd[q.iid]=[q,q.selected]}}else if(rz(b,z)){pc(this.Rx,q);g=true;this.Dd[q.iid]=[q,c==2?!q.selected:
true]}}}if(g)if(this.X9&&!this.hM)Rg(this.$K,0,this);else this.$K()};
nC.prototype.$K=function(){for(var a in this.Dd){var b=this.Dd[a];b[0].XK(b[1]);delete this.Dd[a]}if(this.X9){this.hM=true;Rg(function(){this.hM=false},
0,this)}};
nC.prototype.Li=function(a){F(this.e,"mousedown",this.lo);G(document,"mousemove",this.cm);G(document,"mouseup",this.dm);G(document,"mouseout",this.Yv);for(var b=0;b<this.Rx.length;b++){var c=this.Rx[b];if(a.ctrlKey)this.Gf.rk(c,!c.selected);else this.Gf.rk(c,true)}document.body.removeChild(this.xp)};
nC.prototype.im=function(a){if(!a.relatedTarget)this.Li(a)};function oC(a,b,c,d,e,f,g){this.Gf=a;this.ka=gz('<div class="lhcl_slide" style="left:0"><div class="lhcl_slideImage"style="background-image:url({url})"><div class="lhcl_selectBox" style="visibility:hidden"><div class="lhcl_innerSelectBox"></div></div></div></div>',{url:e});this.I=this.ka.firstChild;this.lo=l(this.mw,this);this.cm=l(this.hm,this);this.dm=l(this.Li,this);F(this.I,"mousedown",this.lo);this.Og=f;this.N=new Date(g);this.iid=d;this.old_layout=c;this.layout_num=b;this.src=e;this.selected=
false}
oC.prototype.remove=function(){A(this.ka)};
oC.prototype.A=function(){return this.ka};
oC.prototype.mu=function(){return this.I};
oC.prototype.mw=function(a){if(!this.selected||a.shiftKey||a.ctrlKey)this.Gf.X0(this,a.shiftKey,a.ctrlKey);if(this.selected){F(document,"mousemove",this.cm);F(document,"mouseup",this.dm);G(this.I,"mousedown",this.lo);this.zs=a.clientX;this.Bs=a.clientY}a.stopPropagation();a.preventDefault()};
oC.prototype.hm=function(a){var b=a.clientX-this.zs,c=a.clientY-this.Bs;if(b*b+c*c>=16){this.Li(a);b+=this.I.offsetTop;c+=this.I.offsetLeft;this.Gf.G_(this,a.clientX,a.clientY,b,c)}a.stopPropagation();a.preventDefault()};
oC.prototype.Li=function(){G(document,"mousemove",this.cm);G(document,"mouseup",this.dm);F(this.I,"mousedown",this.lo)};
oC.prototype.select=function(a){this.selected=!(!a);this.XK(a)};
oC.prototype.XK=function(a){this.I.firstChild.style.visibility=a?"":"hidden"};
var pC=function(a,b){return a.layout_num<b.layout_num?-1:(a.layout_num>b.layout_num?1:0)},
qC=function(a,b){return Za(a.Og,b.Og)},
rC=function(a,b){return a.N<b.N?-1:(a.N==b.N?0:1)};function sC(a){this.Gf=a;this.Ty=false;this.iT=l(this.jT,this);this.ca=[]}
sC.prototype.qx=function(a){var b=a.first,c=a.last,d=a.mid;a.t0=Ka();a.x0.length=(a.x1.length=c-b);var e=this.Gf.slide;for(var f=b;f<c;f++){var g=f-b,h=parseInt(e[f].A().style.left,10);a.x0[g]=h;a.x1[g]=d>=0?(f<d?-10:10):0}};
sC.prototype.f$=function(a,b,c){var d=a*c,e=d+b;if(this.ca.length){var f=this.ca[0];if(f.first==d){if(f.mid!=e){f.mid=e;this.qx(f);this.kE()}return}else{if(f.mid>=0){f.mid=-1;this.qx(f)}for(var g=1;g<this.ca.length;g++){var h=this.ca[g];if(h.first==d){this.ca.splice(g,1);this.ca.unshift(h);this.qx(h);this.kE();return}}}}var i=Math.min(d+c,this.Gf.slide.length),a={first:d,last:i,mid:e,x0:[],x1:[],t0:-1};this.qx(a);this.ca.unshift(a);this.kE()};
sC.prototype.kE=function(){if(!this.Ty){this.Ty=true;Qy(this.iT)}};
sC.prototype.bQ=function(){if(this.Ty){this.Ty=false;Sy(this.iT)}};
sC.prototype.reset=function(){var a=this.Gf.slide;while(this.ca.length){var b=this.ca.pop(),c=b.first+b.x0.length;for(var d=b.first;d<c;d++)a[d].A().style.left="0"}this.bQ()};
sC.prototype.jT=function(a){var b=this.Gf.slide,c=false;for(var d=this.ca.length-1;d>=0;d--){var e=this.ca[d];if(e.t0<0)continue;c=true;var f=a-e.t0;if(f<100){var g=f/100;for(var h=0;h<e.x0.length;h++)b[e.first+h].A().style.left=e.x0[h]+(e.x1[h]-e.x0[h])*g+"px"}else{for(var h=0;h<e.x1.length;h++)b[e.first+h].A().style.left=e.x1[h]+"px";e.t0=-1;if(e.mid==-1)this.ca.splice(d,1)}}if(!c)this.bQ()};function tC(a,b,c){this.ka=w("div");this.rX=w("div");this.rX.className="lhcl_tableEnd";a.appendChild(this.ka);a.appendChild(this.rX);this.afa=new nC(this,a);this.h7=new sC(this);this.Aa=[];this.Pf=null;this.slide=[];this.cm=l(this.hm,this);this.dm=l(this.Li,this);this.Yv=l(this.im,this);if(s)this.Cfa=F(document,"selectstart",this.Ml,false,this);Y(X("reorder"),l(this.sD,this));Y(X("copyToAlbum"),l(this.MU,this));Y(X("moveToAlbum"),l(this.r4,this));Y(X("deleteSelected"),l(this.vj,this));Y(X("sortSlides"),
l(this.R4,this));this.Qba=b;this.IW=c;this.Lz(true)}
tC.prototype.Ml=function(a){if(this.Pf&&df(this.Pf,a.target)||df(this.ka,a.target))a.preventDefault()};
tC.prototype.VS=function(a,b,c,d,e,f){c+="?crop=1&imgmax=64";var g=at(f,"_lt");if(g)c=g;var h=new oC(this,this.slide.length,a,b,c,d,e);this.slide.push(h);this.ka.appendChild(h.A())};
tC.prototype.w2=function(a,b){b.sort(pC);if(a>=this.slide.length)for(var c=0;c<b.length;c++){this.slide.push(b[c]);this.ka.appendChild(b[c].A())}else{var d=this.ka.childNodes[a];for(var c=0;c<b.length;c++){this.ka.insertBefore(b[c].A(),d);this.slide.splice(a,0,b[c]);a++}}for(var c=0;c<this.slide.length;c++)this.slide[c].layout_num=c};
tC.prototype.aK=function(){return gc(this.Aa,function(a){return a.iid})};
tC.prototype.jD=function(a){var b=th(),c=a.clientX+b.scrollLeft,d=a.clientY+b.scrollTop,e=this.Wt(c,d,true),f=e.index;for(var g=this.slide.length-1;g>=0;g--){var h=this.slide[g];if(h.selected){this.slide.splice(g,1);h.remove();if(g<f)f--}}this.w2(f,this.Aa)};
tC.prototype.Pd=function(){while(this.Aa.length)this.rk(this.Aa.pop(),false)};
tC.prototype.rk=function(a,b,c){tc(this.Aa,a);if(b)if(c)this.Aa.unshift(a);else this.Aa.push(a);a.select(b);if(this.Aa.length<2)this.Lz(this.Aa.length==0)};
tC.prototype.X0=function(a,b,c){if(ib(ye,"Mac")){c=b;b=false}if(b)if(this.Aa.length){var d=this.Aa[0].layout_num,e=a.layout_num;if(!c)this.Pd();if(d<e)for(var f=d;f<=e;f++)this.rk(this.slide[f],true);else for(var f=d;f>=e;f--)this.rk(this.slide[f],true)}else this.rk(a,true);else if(c)this.rk(a,!a.selected,true);else{this.Pd();this.rk(a,true)}};
tC.prototype.G_=function(a,b,c,d,e){if(this.Pf)A(this.Pf);var f=uh(a.A());this.zs=b;this.Bs=c;this.YH=f.x+d;this.Uz=f.y+e;var g=gz('<div style="position:absolute;left:{x}px;top:{y}px"><div class="lhcl_slideImage" style="background-image:url({src})"></div></div>',{x:this.YH,y:this.Uz,src:a.src});Ch(g.firstChild,0.5);document.body.appendChild(g);if(this.Aa.length>1){var h=w("div");h.className="lhcl_draginfo";g.appendChild(h);h.innerHTML="+ "+(this.Aa.length-1)+" more"}F(document,"mousemove",this.cm);
F(document,"mouseup",this.dm);F(document,"mouseout",this.Yv);this.R3=document.documentElement.scrollHeight-Vy();this.Pf=g};
tC.prototype.hm=function(a){var b=a.clientX,c=a.clientY,d=b-this.zs,e=c-this.Bs;this.BM(d,e);var f=th(),g=b+f.scrollLeft,h=c+f.scrollTop,i=this.Wt(g,h,true);this.h7.f$(i.row,i.col,i.rowLength);var k=Vy(),n=0;if(c<40)n=c-40;else if(k-c<40)n=40-(k-c);if(n){n=2000*(n/40);n=pe(n,-2000,2000)}this.Zy(n)};
tC.prototype.im=function(a){if(!a.relatedTarget)this.Zy(0)};
tC.prototype.BM=function(a,b){this.Pf.style.left=this.YH+a+"px";this.Pf.style.top=this.Uz+b+"px"};
tC.prototype.Li=function(a){G(document,"mousemove",this.cm);G(document,"mouseup",this.dm);G(document,"mouseout",this.Yv);this.h7.reset();this.jD(a);this.Zy(0);if(this.Pf)A(this.Pf);this.Pf=null};
tC.prototype.Zy=function(a){if(a==0){if(this.$w){Sy(this.$w);this.$w=null}}else{this.bea=a;if(!this.$w&&this.R3>0){this.W2=Ka();this.$w=l(this.u7,this);Qy(this.$w)}}};
tC.prototype.u7=function(a){var b=(a-this.W2)*0.0010;this.W2=a;var c=this.bea*b,d=th(),e=Number(d.scrollTop);if(c!=0){d.scrollTop=pe(Number(e+c),0,this.R3);if(this.Pf){var f=this.Pf.offsetLeft-this.YH,g=this.Pf.offsetTop-this.Uz;this.Uz+=d.scrollTop-e;this.BM(f,g)}}};
tC.prototype.g7=function(){if(this.slide.length==0)return 0;var a=this.ka.offsetWidth,b=Math.floor(a/this.slide[0].A().offsetWidth);return Math.max(b,1)};
tC.prototype.Wt=function(a,b,c){var d=0,e=0,f=this.g7();if(f>0){var g=this.slide[0].A(),h=g.offsetWidth,i=g.offsetHeight,k=uh(g),n=Math.max(a-k.x,0),q=Math.max(b-k.y,0);if(c)n+=h/2;var t=Math.floor((this.slide.length-1)/f);d=Math.min(Math.floor(q/i),t);var y=d!=t?f:this.slide.length%f||f;e=Math.min(Math.floor(n/h),y)}var z=d*f+e;return{index:z,row:d,col:e,rowLength:f}};
tC.prototype.FP=function(a){this.sD();var b=this.aK();if(b.length>100){alert("We're sorry, you cannot copy or move more than 100 photos at once. You currently have "+b.length+" photos selected.");return}this.Np(true);this.QP(false);new cB(a,b,l(this.cU,this))};
tC.prototype.cU=function(){this.Np(false);this.QP(true)};
tC.prototype.QP=function(a){var b=Me("select");for(var c=0;c<b.length;c++)b[c].style.visibility=a?"":"hidden"};
tC.prototype.MU=function(){this.FP(3)};
tC.prototype.r4=function(){if(!_album.blogger||confirm(qv))this.FP(4)};
tC.prototype.vj=function(){this.sD();this.Np(true);if(!confirm(_album.blogger?rv:"Are you sure you want to delete the selected photos?\n\nImportant: deleting photos cannot be undone.")){this.Np(false);return}var a=["noredir=true","optgt=DELETE","aid="+_album.id,"selectedphotos="+this.aK()];document.body.style.cursor="wait";vz.qc(_selectedPhotosPath,l(this.VV,this,this.Aa.length),a.join("&"))};
tC.prototype.VV=function(a,b){if(b.status>=400||b.text.indexOf("success")<0){alert(Pu);document.body.style.cursor="";return}if(a>=this.slide.length)location.href=_album.link;else location.reload()};
tC.prototype.sD=function(a){if(a)this.Np(true);var b=Infinity,c=-1;for(var d=0;d<this.slide.length;d++)if(this.slide[d].old_layout!=d){b=d;break}for(var d=this.slide.length-1;d>b&&d>0;d--)if(this.slide[d].old_layout!=d){c=d+1;break}var e=[],f=Infinity,g=-1;for(var d=b;d<c;d++){var h=this.slide[d];e.push(h.iid);f=Math.min(f,h.old_layout);g=Math.max(g,h.old_layout)}if(e.length){var i=["aid="+_album.id,"uname="+ab(_authuser.name),"start="+f,"stop="+g,"iids="+e.join(",")];vz.qc(_reorderPath,l(this.K6,
this,a),i.join("&"))}else if(a)document.location=a};
tC.prototype.K6=function(a,b){if(b.status>=400||b.text.indexOf("success")<0){alert(Pu);return}if(a)document.location=a};
tC.prototype.Np=function(a){this.Qba.disabled=a;this.Lz(a)};
tC.prototype.Lz=function(a){if(this.Aa.length==0)a=true;for(var b=0;b<this.IW.length;b++)this.IW[b].disabled=a};
tC.prototype.ds=function(a){this.slide.sort(a);for(var b=0;b<this.slide.length;b++){var c=this.slide[b];c.remove();this.ka.appendChild(c.A());c.layout_num=b}};
tC.prototype.R4=function(a){if(a.value=="name")this.ds(qC);else if(a.value=="date")this.ds(rC);a.value="default"};function uC(a,b,c,d,e){this.Ld=a;this.sp=null;this.$ea=null;this.qea=b;this.Zaa=c;this.Cea=d;this.Bea=e;Y("sortAlbums",l(this.AF,this));this.AF(vC.param)}
uC.prototype.AF=function(a){var b=[];if(a==vC.param){this.pF(this.Ld);b.push(vC.label);b.push(ez(wC,xC))}else if(a==xC.param){if(this.sp==null){this.sp=[];this.sp.length=this.Ld.length;for(var c=0;c<this.Ld.length;c++)this.sp[c]=this.Ld[c];this.sp.sort(yC)}this.pF(this.sp);b.push(ez(wC,vC));b.push(xC.label)}var d=zC+" "+b.join(" | ");this.qea.innerHTML=d};
uC.prototype.pF=function(a){var b=new fn;for(var c=0;c<a.length;c++){var d=a[c];d.width=this.Cea;d.height=this.Bea;var e=_features.googlephotos?24:32,f=_features.googlephotos?13:17,g=ob(d.title,e,true);d.shorttitle=Ng(g,f);d.popuptitle=g.length<d.title.length?' title="'+d.title+'"':"";d.photo_count=d.count;if(d.unlisted)d.listing='<span class="lhcl_status">'+$w+"</span>";d.i=c;var h=d.blogger?'<div class="lhcl_blogger lhcl_fakelink" style="background: url({src}) no-repeat left; width: {width}px; height: {height}px"><img src="img/blogger.gif" /></div>':
'<img src="{src}" width="{width}" height="{height}"/>';if(d.isdefault){d.addclass=" lhcl_dropbox";d.icon='<a href="'+d.url+'"><img src="img/dropbox.gif"></a>&nbsp;'}d.image_html=ez(h,d);if(_features.geotagging&&d.lat&&d.lon)d.icon_html='<img src="img/transparent.gif" class="SPRITE_mapped" />';b.F(ez('<div class="lhcl_album"><div class="lhcl_imagebox{addclass}"><div class="lhcl_frame"><div class="lhcl_padding"><a href="{url}">{image_html}</a></div></div></div><div class="lhcl_desc{addclass}"><table class="lhcl_title" style="height:4em;"><tr><td><div class="lhcl_titlebox">{icon}<a href="{url}"{popuptitle}>{shorttitle}</a> <span class="lhcl_info">({photo_count})</span>{icon_html}</div><div class="lhcl_info">{date}{listing}</div></td></tr></table></div></div>',
d))}this.Zaa.innerHTML=b.toString()};
var AC=function(a){Oi("_lhmessageid",a?"INDEX_SET_ON":"INDEX_SET_OFF",-1,"/");window.location.reload()},
yC=function(a,b){return b.upload_secs-a.upload_secs},
zC='<span class="lhcl_notbold">'+Uu+"</span>",wC="<a href=\"javascript:_d('sortAlbums', '{param}');\">{label}</a>",vC={param:"none",label:Tu},xC={param:"upload",label:Su};var GC=function(){u("lhid_emailtofield").focus();Y("LTA",xz);Y("CancelMail",BC);Y("CheckFacesAll",CC);Y("CheckFacesNone",DC);Y("AddRemoveToEmail",EC);if(_features.contactpicker){Wp="/lh/ui/";F(u("lhid_pick"),E,FC);_initEmailAutocomplete(u("lhid_emailtofield"),"/lh/data/contacts",true)}else _initEmailAutocomplete(u("lhid_emailtofield"),"/lh/contacts?out=js",true)};
function FC(a){var b="toolbar=no,location=no,menubar=no,scrollbars=no,resizable=yes,status=no,width=315,height=610,top="+a.screenY+",left="+a.screenX+",screenY="+a.screenY+",screenX="+a.screenX,c=mq("lhid_emailtofield",false).toString();window.open(c,"_picker",b)}
function HC(){var a=u("lhid_emailtofield"),b=u("lhid_emailto");if(!(a&&b))return false;var c=ia(a.value),d=[];for(var e=0;e<c.length;e++){var f=c[e];if(!f)continue;var g=fa(c[e])[1];if(!g||!az(g)){var h=m('The address "{$address}" does not appear to be a valid email address. Please check the address and try again.',{address:f});alert(h);return false}d.push(f)}if(!d.length){var i=m("Please specify at least one recipient.");alert(i);return false}b.value=d.join(",");return true}
function BC(a){var b=u("lhid_emailform");for(var c=0;c<b.elements.length;c++){var d=b.elements[c];if(d.type!="textarea")continue;if(Va(d.value)!=""){var e=m('Your email has not been sent. Press "OK" to discard your message, or "Cancel" to continue writing.');if(confirm(e))break;else return}}window.location=a}
function CC(){return IC("sids","lhid_emailform",true)}
function DC(){return IC("sids","lhid_emailform",false)}
function IC(a,b,c){var d="lhid_checkbox_",e=u(b);if(typeof e!="undefined")for(var f=0;f<e.elements.length;f++){var g=e.elements[f];if(g.name==a){g.checked=c;var h=g.id.substring(d.length);EC(h)}}}
function EC(a){var b=u("lhid_emailtofield"),c=u("lhid_email_"+a),d=u("lhid_checkbox_"+a),e=Va(c.innerHTML),f=", ";if(e.length==0)return;if(d.checked){var g=b.value.split(",");for(var h=0;h<g.length;h++){var i=Va(g[h]);if(Xa(i,e)==0)break}if(h==g.length)b.value+=e+f}else{var k="",g=b.value.split(",");for(var h=0;h<g.length;h++){var i=Va(g[h]);if(i.length!=0&&0!=Xa(i,e))k+=i+f}b.value=k}}
;var JC="ALL_RIGHTS_RESERVED",KC="ATTRIBUTION",LC="ATTRIBUTION_NO_DERIVATIVES",MC="ATTRIBUTION_NON_COMMERCIAL_NO_DERIVATIVES",NC="ATTRIBUTION_NON_COMMERCIAL",OC="ATTRIBUTION_NON_COMMERCIAL_SHARE_ALIKE",PC="ATTRIBUTION_SHARE_ALIKE",QC=function(a){var b=a.chk_remix.checked,c=a.chk_com.checked,d=a.chk_sa.checked;if(!b&&!c)return MC;if(!b&&c)return LC;if(b&&!c&&!d)return NC;if(b&&!c&&d)return OC;if(b&&c&&!d)return KC;if(b&&c&&d)return PC},
RC=function(a){return a.rb_c.checked?JC:QC(a)},
WC=function(a,b){var c=a!=JC;b.rb_c.checked=!c;b.rb_cc.checked=c;b.chk_remix.checked=c?SC(a):false;b.chk_com.checked=c?TC(a):false;b.chk_sa.checked=c?UC(a):false;VC(a,b)},
SC=function(a){switch(a){case JC:case MC:case LC:return false}return true},
TC=function(a){switch(a){case JC:case NC:case OC:case MC:return false}return true},
UC=function(a){switch(a){case OC:case PC:return true}return false},
VC=function(a,b){var c=a!=JC;M(b.div_cc,c);M(b.img_c,!c);M(b.img_by,c);M(b.img_nc,c?!TC(a):false);M(b.img_nd,c?!SC(a):false);M(b.img_sa,c?UC(a):false);M(b.lab_c,!c);M(b.lab_cc,c);if(!b.chk_remix.checked)b.chk_sa.checked=false;b.chk_sa.disabled=!b.chk_remix.checked;b.lab_sa.disabled=!b.chk_remix.checked;b.lab_cc.href=XC(a)},
XC=function(a){switch(a){case JC:return"";case KC:return"http://creativecommons.org/licenses/by/3.0/";case PC:return"http://creativecommons.org/licenses/by-sa/3.0/";case LC:return"http://creativecommons.org/licenses/by-nd/3.0/";case NC:return"http://creativecommons.org/licenses/by-nc/3.0/";case OC:return"http://creativecommons.org/licenses/by-nc-sa/3.0/";case MC:return"http://creativecommons.org/licenses/by-nc-nd/3.0/";default:return""}},
YC=function(){};var aD=function(a){if(_features.creativecommons)ZC();$C(a)},
$C=function(a){var b=u("lhid_settingsform"),c=u("cancel_top"),d=u("save_top"),e=u("cancel_bottom"),f=u("save_bottom");if(b&&(c&&d||e&&f)){var g=new hu;g.eD(b);var h=u("lhcl_portrait_id");if(h)g.eD(h);g.UD(" ");if(c)F(c,E,function(){g.CI();document.location=a});
if(e)F(e,E,function(){g.CI();document.location=a});
F(b,"submit",function(){if(c)c.disabled=true;if(d)d.disabled=true;if(e)e.disabled=true;if(f)f.disabled=true;var i=bD?!bD.n7(b):true;if(i)g.A$();return i})}},
cD=function(){var a=u("lhid_lic_formValue");return a?a.value:null},
dD=function(a){var b=u("lhid_lic_formValue");b.value=a},
eD=function(){var a=new YC;a.rb_c=u("lhid_lic_category_c");a.rb_cc=u("lhid_lic_category_cc");a.chk_remix=u("lhid_lic_cc_remix");a.chk_com=u("lhid_lic_cc_com");a.chk_sa=u("lhid_lic_cc_sa");a.img_c=u("lhid_lic_cc_preview_c");a.img_by=u("lhid_lic_cc_preview_by");a.img_nc=u("lhid_lic_cc_preview_nc");a.img_nd=u("lhid_lic_cc_preview_nd");a.img_sa=u("lhid_lic_cc_preview_sa");a.lab_c=u("lhid_lic_text_c");a.lab_cc=u("lhid_lic_text_cc");a.lab_sa=u("lhid_lic_cc_sa_lab");a.div_cc=u("lhid_lic_cc_settings");return a},
ZC=function(){var a=eD(),b=cD();if(b!=null)WC(b,a);F(a.rb_c,E,fD);F(a.rb_cc,E,fD);F(a.chk_remix,E,fD);F(a.chk_com,E,fD);F(a.chk_sa,E,fD)},
fD=function(){var a=eD(),b=RC(a);VC(b,a);dD(b)};function gD(a,b,c){vz.qc(a,l(hD,null,b,c))}
function hD(a,b,c){if(c.status>=400||c.text.indexOf("success")<0){alert(Xw);return}var d=u("lhid_addFav");if(d)M(d,false);d=u("lhid_inFav");if(d)M(d,true);d=u("lhid_nolink");if(d)M(d,true);jz("hideVideo");new iD(a,b)}
function jD(a){if(a.status>=400||a.text.indexOf("success")<0){alert(Xw);return}var b=u("lhid_nolink");if(b)M(b,false);b=u("lhid_link");if(b)M(b,true)}
function iD(a,b){Ez.Hd(this);this.Pda=a;var c={lhui_username:b,lhui_yes_func:X("func"),lhui_no_func:X("func")};this.r=c;this.Xb=w("div");this.Xb.className="lhcl_cover";document.body.appendChild(this.Xb);this.Jf();this.nm=l(this.Jf,this);Ly(window,"resize",this.nm);window.scrollTo(0,0);RA();this.ka=gz(kD,this.r);document.body.insertBefore(this.ka,document.body.firstChild);Y(this.r.lhui_yes_func,l(this.GR,this));Y(this.r.lhui_no_func,l(this.hS,this));this.qw=Ky(this,this.tF);Ly(document,"keydown",this.qw);
this.kS()}
iD.prototype.c$="";iD.prototype.ha=function(){Ez.ee(this);if(this.r){Z(this.r.lhui_yes_func);Z(this.r.lhui_no_func)}My(document,"keydown",this.qw);My(window,"resize",this.nm);for(var a in this)this[a]=null};
iD.prototype.Fc=function(){if(this.ka)A(this.ka);if(this.Xb)A(this.Xb);jz("restoreVideo");this.ha()};
iD.prototype.kS=function(){if(typeof vz=="undefined"||!vz.Gs()){window.location=this.c$+"&redir="+encodeURIComponent(window.location);this.Fc()}};
iD.prototype.Jf=function(){var a=Math.max(document.body.offsetHeight,document.body.scrollHeight);a=Math.max(a,document.body.clientHeight);a=Math.max(a,document.documentElement.clientHeight);this.Xb.style.height=a+"px"};
iD.prototype.hS=function(){this.Fc()};
iD.prototype.GR=function(){vz.qc(this.Pda,jD);this.Fc()};
var kD='<div class="lhcl_dialog"><div class="lhcl_dialog_body lhcl_addfavorite"><p><img src="img/favorites_checked.gif" />{lhui_username} has been added as a favorite.</p><br /><hr /><p>'+Ww+'</p><p>Would you like to link to {lhui_username}\'s public gallery?</p><br /><table><tr><td class="lhcl_halfwidth lhcl_align_right""><input type="button" value="'+Vw+'" onclick="_d(\'{lhui_yes_func}\')" /></td><td class="lhcl_halfwidth lhcl_dialog_button"><input type="button" value="'+Uw+'" onclick="_d(\'{lhui_no_func}\')" /></td></tr><tr><td></td><td class="lhcl_dialog_button"><span class="lhcl_mini_text">'+
Tw+"</span></td></tr></table></div></div>";function lD(a,b,c,d){if(_features.googlephotos){this.s8(d);if(d)return;this.Nda=u("lhid_userHomeFavorites")}this.lb=a;this.ed=b;this.Kc=c;this.UK=u(_features.googlephotos?"lhid_favlink":"lhid_header");this.IF=u("lhid_activity");this.pN=u("lhid_favorites");this.zea=u("lhid_teaser");this.vfa=u("lhid_lastshown");this.lb.sort(this.AU);if(this.IF)this.u5();if(this.pN)this.zN(false);var e=window.location.hash;if(e=="")this.Dx("");else this.Dx(e);this.ud=l(this.rs,this);Qy(this.ud);Y("call",mD);Y("toggleNotify",
l(this.O$,this));Y("toggleLink",l(this.M$,this))}
lD.prototype.s8=function(a){this.tg=new nD;var b=[];if(_authuser.isOwner)b.push([ty,"/"]);else{var c=lb(uy(_user.nickname));b.push([c,"/"+_user.name])}this.qW=new oD(a?2:0,_user.name,b);this.tg.J(this.qW);this.tg.C(u("lhid_context"))};
var mD=function(a){vz.qc(a,pD)},
pD=function(a){if(a.status>=400||a.text.indexOf("success")<0){alert(Xw);window.location.reload()}};
lD.prototype.rs=function(){var a=window.location.hash;if(this.Gba==a)return;this.Dx(a)};
lD.prototype.u5=function(){var a=[];a.push('<table class="lhcl_fullwidth"><tr>');for(var b=0;b<this.ed.length;++b){if(null==this.ed[b]||this.ed[b].deleted)continue;a.push('<td class="lhcl_halfcol">',"<table><tr>",'<td><a href="',this.ed[b].link,'">','<img class="lhcl_activity" src="',this.ed[b].src,'"></a></td>','<td><a href="',this.ed[b].link,'" class="lhcl_plain">');var c=this.ed[b].type;switch(this.ed[b].reason){case "starred":a.push('<div class="lhcl_title">',this.ed[b].album,"</div>",'<div class="lhcl_info">By ',
this.ed[b].person,".</div>");break;case "photos_added":a.push('<div class="lhcl_title">',this.ed[b].album,"</div>",'<div class="lhcl_info">',"Photos added by "+this.ed[b].person+": "+this.ed[b].photo_count,"</div>");break;case "email_notification":switch(c){case "album":a.push('<div class="lhcl_title">',this.ed[b].album,"</div>",'<div class="lhcl_info">',this.ed[b].person+" shared an album with you.","</div>");break;case "photo":a.push('<div class="lhcl_title">',this.ed[b].person,"</div>",'<div class="lhcl_info">',
Xu,"</div>");break;case "account":a.push('<div class="lhcl_title">',this.ed[b].person,"</div>",'<div class="lhcl_info">',Wu,"</div>");break}break}a.push('<div class="lhcl_info">',this.ed[b].date,"</div>","</a></td></tr></table></td>");if((b+1)%2==0)a.push("</tr><tr>")}a.push("</tr></table>");this.IF.innerHTML=a.join("")};
lD.prototype.zN=function(a){Ri("AddFavoriteId");if(this.dH==a)return;this.dH=a;var b=[];if(_features.contacts){b.push("<p>");if(a)b.push("You are viewing all favorite users.  To view only those whose activity updates you are receiving, click "+('<a href="'+this.Kc+'#list">')+"here</a>");else b.push("You are receiving activity updates from these users. For all your favorite users, click "+('<a href="'+this.Kc+'#all">')+"here</a>");b.push("</p>")}b.push('<table class="lhcl_fullwidth">');var c=a?"#all":
"#list",d=encodeURIComponent(this.Kc+c),e=false;for(var f=0;f<this.lb.length;++f)if(!_features.contacts||a||this.lb[f].notify){if(e)b.push('<tr><td colspan="2" class="lhcl_listEnd"></td></tr>');else e=true;b.push('<tr class="lhcl_favoritePersonListed">','<td class="lhcl_host"><a href="',this.lb[f].link,'"><img class="lhcl_portrait" src="',this.lb[f].src,'" ></a></td>','<td class="lhcl_data"><table><tr>','<td><table style="width:100%;"><tr><td><a href="',this.lb[f].link,'">',this.lb[f].name,"</a>");
b.push("</td>");if(_features.contacts){b.push('<td class="lhcl_starProperty"><div><input type="checkbox" ');if(this.lb[f].notify)b.push('checked="true"');b.push("onclick=\"_d('toggleNotify', '",f,"');\" />",Sw,"</div></td>")}b.push('<td class="lhcl_starProperty">');b.push('<div><input type="checkbox" ');b.push("onclick=\"_d('toggleLink', '",f,"');\" ");if(this.lb[f].linked)b.push('checked="true" ');b.push("/>",Yu);b.push("</div></td></tr></table></td>");if(this.lb[f].unstar){b.push('<td class="lhcl_starProperty">',
"<a");b.push(" onclick=\"return confirm('",Rw,"');\"");b.push(' href="',this.lb[f].unstar,"&redir=",d,'">',$u,"</a></td>")}else b.push("<td>&nbsp;</td>");b.push("</tr>");if(this.lb[f].albums.length>0){b.push('<tr><td colspan="2"><div class="lhcl_unlistedListing">',Vu,":</div></td></tr>");for(var g=0;g<this.lb[f].albums.length;++g){var h="";if((g+1)%2)h=" lhcl_colorbar";b.push('<tr><td class="lhcl_starred',h,'">');if(this.lb[f].albums[g].deleted)b.push(this.lb[f].albums[g].name,av);else b.push('<a href="',
this.lb[f].albums[g].link,'">',this.lb[f].albums[g].name,"</a>");b.push('</td><td class="lhcl_starProperty',h,'">',"<a");b.push(" onclick=\"return confirm('",Rw,"');\"");b.push(' href="',this.lb[f].albums[g].unstar,"&redir=",d,'">',Zu,"</a></td></tr>")}}b.push("</table>")}b.push("</table>");this.pN.innerHTML=b.join("")};
lD.prototype.Dx=function(a){this.Gba=a;var b=a=="#activity",c=this.IF.style,d=this.pN.style,e=this.zea.style;if(_features.googlephotos)var f=this.Nda.style;c.display="none";d.display="none";e.display="none";if(_features.googlephotos){f.display="none";this.UK.style.display="none";this.qW.ce(b?1:0)}if(b){(this.ed.length==0?e:c).display="";this.UK.innerHTML='<span class="lhcl_notbold">'+dv+'</span> <a href="'+this.Kc+'#list">'+cv+"</a> | "+bv}else{(this.lb.length==0?e:d).display="";this.UK.innerHTML=
'<span class="lhcl_notbold">'+dv+"</span> "+cv+' | <a href="'+this.Kc+'#activity">'+bv+"</a>";if(_features.contacts)this.zN(a=="#all");if(_features.googlephotos)f.display=""}};
lD.prototype.O$=function(a){mD(this.lb[a].notify?this.lb[a].notifylink.replace("notify=true","notify=false"):this.lb[a].notifylink.replace("notify=false","notify=true"));this.lb[a].notify=!this.lb[a].notify;this.dH=!this.dH;this.Dx(window.location.hash)};
lD.prototype.M$=function(a){mD(this.lb[a].linked?this.lb[a].removelink:this.lb[a].addlink);this.lb[a].linked=!this.lb[a].linked};
lD.prototype.AU=function(a,b){if(a.name<b.name)return-1;else if(a.name>b.name)return 1;return 0};function qD(a,b,c){var d=u(b);if(d)d.innerHTML='<object id="UploadListView" classid="CLSID:474F00F5-3853-492C-AC3A-476512BBC336" codebase="'+a+"uploader2.cab#version="+c+'" width="564" height="375"></OBJECT>'}
;function rD(a,b,c){var d=m("Click to add a caption");Ez.Hd(this);this.Ra=a;this.wc={};this.uj=d;this.Kea=b;this.e=c;this.tD="";this.Cb={lhui_nextfunc:X("func"),lhui_promptfunc:X("func"),lhui_savefunc:X("func"),lhui_undofunc:X("func"),lhui_checkfunc:X("func")};Y(this.Cb.lhui_nextfunc,l(this.qF,this));Y(this.Cb.lhui_promptfunc,l(this.JR,this));Y(this.Cb.lhui_savefunc,l(this.eU,this));Y(this.Cb.lhui_undofunc,l(this.aS,this));Y(this.Cb.lhui_checkfunc,l(this.lS,this));Y("ReturnToAlbum",l(this.wF,this));
this.qF(0,50)}
rD.prototype.ha=function(){Ez.ee(this);if(this.Fe)window.clearTimeout(this.Fe);if(this.Cb){Z(this.Cb.lhui_nextfunc);Z(this.Cb.lhui_promptfunc);Z(this.Cb.lhui_savefunc);Z(this.Cb.lhui_undofunc);Z(this.Cb.lhui_checkfunc);Z("ReturnToAlbum")}for(var a in this)this[a]=null};
rD.prototype.qF=function(a,b){var c=Ka(),d=["off="+a,"max="+b,"nocache="+c],e="http://"+document.domain+":"+document.location.port+"/data/feed/back_compat/user/"+_authuser.name+"/albumid/"+_album.id+"?kind=photo&alt=rss&"+d.join("&"),f=l(this.rR,this,a,b);vz.qc(e,f)};
rD.prototype.ip=function(a,b){var c=b,d=b.split(":");if(d.length>1)if(!s)c=d[1];var e=a.getElementsByTagName(c);for(var f=0;f<e.length;f++){if(e[f].nodeName!=b)continue;var g=e[f].firstChild;if(g!=null){var h=g.nodeValue;if(h!=null)return h}}return null};
rD.prototype.sR=function(a){var b={};b.id=this.ip(a,"gphoto:id");b.l=this.ip(a,"link");b.c=this.ip(a,"description")||"";b.a=this.ip(a,"title");b.s=this.ip(a,"photo:thumbnail");return b};
rD.prototype.rR=function(a,b,c){var d=m("Sorry, we encountered an error while loading this album.");if(!c||c.status>=400){if(!c)alert(d+"\n"+Ew);else alert(d+"\n\n"+ix(c.status));return}if(c.text.substring(0,5)!="<?xml"){alert(d);return}var e;if(s)try{e=new ActiveXObject("Microsoft.XMLDOM");e.async=false;var f=e.loadXML(c.text);if(!f)alert(d+"\n\n"+ix(c.status))}catch(g){}else e=c.xml;var h=e.getElementsByTagName("item");if(_album.photoCount>50)this.FR(a,h.length);this.e.innerHTML="";window.scrollTo(0,
0);for(var i=0;i<h.length;i++){var k=this.sR(h[i]);this.wc[k.id]={};k.lhui_checkfunc=this.Cb.lhui_checkfunc;k.lhui_promptfunc=this.Cb.lhui_promptfunc;k.lhui_savefunc=this.Cb.lhui_savefunc;k.z=_album.photoCount-a-i;k.index=a+i;k.tabindex=i+1;k.selected=false;k.cid="lhid_caption_"+k.id;if(k.c){k.active="lhcl_active";k.caption=k.c}else{k.active="";this.wc[k.id].empty=true;k.caption=this.uj}this.Ra[k.index]=k;var n=gz('<div><table class="lhcl_onebatchcaption"><tr><td class="lhcl_photo"><img src="{s}" alt="{a}"/></td><td class="lhcl_textbox"><textarea id="lhid_caption_{id}" name="caption_{id}" onkeydown="_d(\'{lhui_checkfunc}\', \'{index}\')" onfocus="_d(\'{lhui_promptfunc}\', \'{index}\')" onblur="_d(\'{lhui_savefunc}\', \'{index}\')" onkeyup="_d(\'{lhui_checkfunc}\', \'{index}\')" class="{active}" tabindex="{tabindex}">{caption}</textarea><div id="status_{id}" class="lhcl_statusbox"></div></td></tr></table></div>',
k);this.e.appendChild(n);if(i<b-1){var q=w("div");q.className="lhcl_captionspacer";this.e.appendChild(q)}}};
rD.prototype.FR=function(a,b){var c={};c.lhui_nextfunc=this.Cb.lhui_nextfunc;var d="",e="";if(a>0){c.offset=a-50;c.interval=50;d=ez(sD,c)}if(_album.photoCount>a+50){c.offset=a+50;c.interval=Math.min(50,_album.photoCount-(a+50));c.tabindex=b+1;e=ez(tD,c)}var f={};f.msg=_features.newStrings?ox(a+1,a+b,_album.photoCount):a+1+" - "+(a+b)+" ( "+_album.photoCount+")";var g=ez("<span>&nbsp; {msg} &nbsp;</span>",f),h=[d,g,e].join("");u("lhid_batchnav_top").innerHTML=h;u("lhid_batchnav_bottom").innerHTML=
h};
rD.prototype.wF=function(){if(this.tD){if(this.Fe){window.clearTimeout(this.Fe);this.Fe=0}this.Fe=Rg(this.wF,200,this)}else window.location=_album.link};
rD.prototype.JR=function(a){var b=this.Ra[a].id;this.Ra[a].input=u("lhid_caption_"+b);if(this.wc[b].empty){this.Ra[a].input.className="lhcl_active";this.Ra[a].input.value=""}};
rD.prototype.ER=function(a){var b=this.Ra[a].id,c=this.Ra[a].input,d=c.value.replace(/[\s\xa0]+/g," ").replace(/^\s+|\s+$/g,""),e=this.Ra[a].c;c.value=d;if(e==Wy(d)){if(Qa(d)){c.value=this.uj;c.className="";this.wc[b].state="default";this.Zh(a)}return false}this.wc[b].empty=Qa(d)?true:false;if(d.length>1024){this.wc[b].state="error";this.Zh(a);return false}return true};
rD.prototype.xF=function(a){var b=this.Ra[a].id,c=this.Ra[a].input,d=Va(c.value),e=u("aid").value,f=this.Kea+"&aid="+e,g=l(this.bS,this,a),h=["optgt=SAVE_PHOTO_CAPTIONS","noredir=true","caption_"+b+"="+ab(d)];vz.qc(f,g,h.join("&"));this.tD=true;this.wc[b].state=this.wc[b].state=="reverting"?"reverting":"saving";this.Zh(a)};
rD.prototype.bS=function(a,b){if(this.rd(b)||this.de(b)){var c=this.Ra[a].id,d=this.Ra[a].input;d.className="";d.value=this.uj;this.wc[c].state="";this.Zh(a);this.tD=false;return}var c=this.Ra[a].id,d=this.Ra[a].input,e=d.value;this.wc[c].undo=this.Ra[a].c;this.Ra[a].c=Wy(e);if(this.wc[c].empty){d.className="";d.value=this.uj}else d.className="lhcl_active";if(this.wc[c].state=="reverting"){this.wc[c].state="reverted";this.Zh(a)}else{this.wc[c].state="saved";this.Zh(a)}this.tD=false};
rD.prototype.aS=function(a){var b=this.Ra[a].id,c=this.Ra[a].input,d=this.wc[b].undo;c.value=lb(d);this.wc[b].empty=Qa(d)?true:false;this.wc[b].state="reverting";this.xF(a)};
rD.prototype.lS=function(a){var b=this.Ra[a].id,c=this.Ra[a].input,d=c.value,e=this.Ra[a].c;if(d!=e&&this.wc[b].state!="prompt"&&!Qa(d)){this.wc[b].state="prompt";this.Zh(a)}this.xR(a)};
rD.prototype.xR=function(a){var b=this.Ra[a].id,c=this.Ra[a].input,d=u("status_"+b);if(!(c&&d))return;if(c.value.length>992){this.wc[b].state="warning";this.Zh(a)}else if(this.wc[b].state=="error"){this.wc[b].state="prompt";this.Zh(a)}};
rD.prototype.Zh=function(a){var b=this.Ra[a].id,c=this.Ra[a].input,d=this.wc[b].state,e=u("status_"+b),f,g={};g.lhui_undofunc=this.Cb.lhui_undofunc;g.index=a;e.style.color="#666";switch(d){case "prompt":f=uD;break;case "saving":f=vD;break;case "saved":f=wD;break;case "reverting":f=xD;break;case "reverted":f=yD;break;case "warning":g.num=c.value.length;g.max=1024;if(g.num>g.max)e.style.color="#f00";var h=m("Your caption is {$num} characters long. ({$max} maximum)",{num:g.num,max:g.max});g.msg=_features.newStrings?
h:"";f="<span>{msg}</span>";break;case "error":f=zD;break;default:f="<span> </span>";break}var i=gz(f,g);Uy(e,i)};
rD.prototype.eU=function(a){if(this.ER(a))this.xF(a)};
var sD='<span><a id="lhid_previous" class="lhcl_navlink" href="javascript: _d(\'{lhui_nextfunc}\', {offset}, {interval})">&lt; '+Hw+"</a></span>";var tD='<span><a class="lhcl_navlink" href="javascript:_d(\'{lhui_nextfunc}\', {offset}, {interval})" tabindex="{tabindex}">'+Gw+"&gt;</a></span>",AD=m("Changes to captions are automatically saved."),BD=m("Saving caption&#8230;"),CD=m("Reverting caption&#8230;"),DD=m("This caption has been reverted."),ED=m("This caption has been saved."),FD=m("Undo"),uD=
'<span class="lhcl_statusdefault">'+AD+"</span>",vD='<table><tr><td class="lhcl_status"><img src="img/progressanim.gif" class="lhcl_statusimg" /></td><td class="lhcl_status">'+BD+"</td></tr></table>",xD='<table><tr><td class="lhcl_status"><img src="img/progressanim.gif" class="lhcl_statusimg" /></td><td class="lhcl_status">'+CD+"</td></tr></table>",yD='<table><tr><td class="lhcl_status"><img src="img/check.gif" class="lhcl_statusimg" /></td><td class="lhcl_status">'+DD+"</td></tr></table>",wD='<table><tr><td class="lhcl_status"><img src="img/check.gif" class="lhcl_statusimg" /></td><td class="lhcl_status">'+
ED+"</td><td class=\"lhcl_undo\"><a href=\"javascript:_d('{lhui_undofunc}', '{index}')\">"+FD+"</a></td></tr></table>",zD=_features.newStrings?'<div class="lhcl_warning" style="width:400px;">'+qx(1024)+"</div>":'<span class="lhcl_warning">'+px+"</span>";rD.prototype.rd=function(a){var b=a.status;if(b>=400){var c;c=b==403?"403 "+Nv:(b==404?"404 "+Mv:(b>=500?"500 "+Kv:"unknown "+Lv));alert(ez(c,{status:b}));return true}return false};
rD.prototype.de=function(a){if(a.text.indexOf("success")<0){var b=a.text.replace(/\<[^>]*\>/g,"");alert(b);return true}return false};;;var GD=function(){var a=false;function b(c){var d=u(c);if(d){M(d,true);a=true}}
if(s)b("lhid_activexControlInfo");if(ze)b("lhid_macInfo");else if(Ae)a=true;if(a)b("lhid_uploadInfo")},
HD=function(a){var b=Va(a.value);return Qa(b)||(_features.asyncUploads?/.+\.(?:3gp|asf|avi|bmp|gif|jpe|jpeg|jpg|m2t|m2ts|mmv|mov|mp4|mpg|png|wmv)$/i.test(b):/.+\.(?:bmp|gif|jpe|jpeg|jpg|png)$/i.test(b))};
var ID=function(a){var b=u("lhid_photosToAdd");if(b)B(b,a)},
JD=function(a,b){var c=u("lhid_greenbar");if(c){var d=Math.round(180*(a/b));L(c,"width",Math.min(d,180)+"px")}};var KD=function(a,b,c,d,e){this.dba=e?e-d:5;this.qE=a;this.at=b;this.LG=c;this.$f=[];this.RW=[];this.j=new K(this)};
o(KD,P);KD.prototype.b=function(){if(!this.W()){KD.f.b.call(this);this.j.b()}};
KD.prototype.$=function(a){KD.f.$.call(this,a);var b=Math.min(this.dba,5),c=u("lhid_files");if(b<1){var d=u("lhid_maxed");if(d)M(d,true);var e=u("lhid_selectheader");if(e)M(e,false);if(c)M(c,false)}if(c)for(var f=0;f<b;f++){var g=w("div");x(c,g);var h=v("input",{type:"file",name:"file"+f,size:32}),i=w("span");of(i,"lhcl_error");M(i,false);x(g,h);x(g,i);B(i,Bu);this.$f[f]=h;this.RW[f]=i;this.j.g(h,kg,this.NE)}this.j.g(this.e,"submit",this.Nl);this.j.g(this.LG,E,this.Q6);this.NE()};
KD.prototype.NE=function(){var a=this.II(),b=a.length;ID(b);this.LG.disabled=!b;this.qE.disabled=!b;ID(a.length);for(var c=0;c<this.$f.length;c++){var d=this.$f[c],e=this.RW[c];if(HD(d)){if(Qa(d.value))d.value="";of(d,"");M(e,false)}else{of(d,"lhcl_error");M(e,true);this.qE.disabled=true}}};
KD.prototype.Q6=function(){this.e.reset();for(var a=0;a<this.$f.length;a++)if(this.$f[a].value){window.location.reload(true);return}this.NE()};
KD.prototype.Nl=function(a){var b=this.II(),c=b.length,d=u("lhid_num");if(!(d&&c)){a.preventDefault();return}if(jc(b,HD)){d.value=c;this.qE.value=zu;this.qE.disabled=true;this.at.disabled=true;this.LG.disabled=true}else{alert(Au);a.preventDefault()}};
KD.prototype.II=function(){return fc(this.$f,function(a){return!Qa(a.value)})};
var LD=function(a,b,c,d){var e=u("lhid_uploadFiles"),f=u("startbutton"),g=u("cancel"),h=u("clearall");if(e&&f&&g&&h){var i=new KD(f,g,h,c,d);i.ia(e)}GD();JD(a,b);var k=u("lhid_uploadTable");L(k,"display","block")};var MD=function(a){var b=a.toSpan();return{ll:{lat:String(a.getSouthWest().lat()),lon:String(a.getSouthWest().lng())},ur:{lat:String(a.getNorthEast().lat()),lon:String(a.getNorthEast().lng())},latSpan:b.lat()/2,lonSpan:b.lng()/2}},
ND=function(a){if(a&&a.lat&&a.lon){var b=Number(a.lat),c=Number(a.lon);if(!isNaN(b+c))return{lat:b,lon:c}}return null},
OD=function(a,b,c){c.extend(a);if(b){var d=ND(b.ll);c.extend(new GLatLng(d.lat,d.lon));d=ND(b.ur);c.extend(new GLatLng(d.lat,d.lon))}},
PD=function(a){var b=ND(a.ll),c=ND(a.ur);return new GLatLngBounds(new GLatLng(b.lat,b.lon),new GLatLng(c.lat,c.lon))};var QD=function(a,b,c,d){ot.call(this,b,c,d)};
o(QD,ot);QD.prototype.nc=null;QD.prototype.Gi=null;QD.prototype.D=function(){return"gphoto.map.Icon"};
QD.prototype.n=function(){QD.f.n.call(this);var a=document.getElementsByTagName("base")[0].href;Dh(this.e,a+"img/shadow64.png");this.nc=w("div");of(this.nc,this.db+"-image");x(this.e,this.nc);this.aa();this.Ab(this.Eq);F(this.e,I,this.xj,false,this);if(this.k)this.e.title=lb(this.k.c||this.k.t)};
QD.prototype.aa=function(){if(this.k){L(this.nc,"background-image","url("+this.k.s+"?imgmax=64&crop=1)");if(this.k.lat&&this.k.lon){if(!this.Gi){this.Gi=w("div");of(this.Gi,this.db+"-mapped");L(this.Gi,"backgroundImage","url(img/mapped.gif)");x(this.nc,this.Gi)}}else if(this.Gi){A(this.Gi);this.Gi=null}}Hh(this.e,true)};
QD.prototype.u=function(){ot.f.u.call(this);G(this.nc,I,this.xj,false,this);this.nc=null;this.Gi=null};
QD.prototype.Ab=function(a){if(this.Eb&&a!=this.Eq){QD.f.Ab.call(this,a);var b=this.db+"-selected";if(a)C(this.e,b);else qf(this.e,b)}};
QD.prototype.xj=function(a){this.dispatchEvent(new pt(this,a))};
QD.prototype.eJ=function(){var a=v("div",{"class":this.db+"-drag"});L(a,"background-image","url("+this.k.s+"?imgmax=32&crop=1)");return a};
QD.prototype.fJ=function(){return RD};
var RD=s?new ge(16,33):new ge(13,31);var UD=function(a,b,c,d){var e=new GIcon(SD(),b);TD.icon=e;this.Hi=new GMarker(c,TD);this.parent=a;this.data=d;this.handlers=[GEvent.bind(this.Hi,"mouseover",this,this.Kl),GEvent.bind(this.Hi,"mouseout",this,this.Jl),GEvent.bind(this.Hi,"remove",this,this.b)];this.editHandlers=[];this.viewHandlers=[];this.disableDragging()};
UD.prototype.b=function(){r(this.handlers,GEvent.removeListener);r(this.editHandlers,GEvent.removeListener);r(this.viewHandlers,GEvent.removeListener)};
UD.prototype.Ud=function(){this.parent.Oe(this)};
UD.prototype.Kl=function(){this.parent.$O(this)};
UD.prototype.Jl=function(){if(this.parent.sJ()==this)this.parent.$O(null)};
UD.prototype.J_=function(){this.parent.closeInfoWindow()};
UD.prototype.I_=function(){this.parent.raa(this)};
UD.prototype.getData=function(){return this.data};
UD.prototype.Ln=function(){return this.Hi};
UD.prototype.getPoint=function(){return this.Hi.getPoint()};
UD.prototype.enableDragging=function(){if(!this.editHandlers.length)this.editHandlers.push(GEvent.bind(this.Hi,"dragstart",this,this.J_),GEvent.bind(this.Hi,"dragend",this,this.I_));r(this.viewHandlers,GEvent.removeListener);this.Hi.enableDragging()};
UD.prototype.disableDragging=function(){if(!this.viewHandlers.length)this.viewHandlers.push(GEvent.bind(this.Hi,"mousedown",this,this.Ud));r(this.editHandlers,GEvent.removeListener);this.Hi.disableDragging()};
var SD=function(){if(!VD){VD=new GIcon;VD.iconSize=new GSize(32,32);VD.iconAnchor=new GPoint(15,31);VD.shadow="img/marker-shadow.png";VD.shadowSize=new GSize(59,32);VD.transparent="img/marker-overlay.png";VD.infoWindowAnchor=new GPoint(15,0)}return VD},
VD=null,WD="?imgmax=64&crop=1",XD="?imgmax=32&crop=1",TD={draggable:true};var YD=function(a){this.e=a;this.fb=[];this.o=new GMap2(a);this.mz();this.o.addControl(new GLargeMapControl);this.o.addControl(new GScaleControl);this.o.addControl(new GHierarchicalMapTypeControl);this.o.addControl(new GOverviewMapControl);this.o.enableContinuousZoom();this.o.enableScrollWheelZoom();this.o.enableDoubleClickZoom();this.o.addMapType(G_PHYSICAL_MAP);this.o.setMapType(G_PHYSICAL_MAP);this.ufa=new GKeyboardHandler(this.o);this.cC=[GEvent.addListener(this.o,"click",l(this.xh,this))];this.Ve=
new Ig(this.e);F(this.Ve,Jg,this.Wj,true,this)};
o(YD,J);YD.prototype.sc=false;YD.prototype.hC={};YD.prototype.b=function(){if(!this.W()){while(this.fb.length)this.fb.pop().b();G(this.Ve,Jg,this.Wj,true,this);r(this.cC,GEvent.removeListener);this.Ve.b();YD.f.b.call(this)}};
YD.prototype.Al=function(){return this.o};
YD.prototype.CJ=function(){return this.fb};
YD.prototype.mz=function(){var a,b=false;if(this.fb.length>0){var c=this.fb[0].getPoint();a=new GLatLngBounds(c,c)}else a=new GLatLngBounds;for(var d=0;d<this.fb.length;d++){var e=this.fb[d],f=e.getData().bb;OD(e.getPoint(),f,a);if(f)b=true}var g=this.o.getBoundsZoomLevel(a),h=a.getCenter();if(!b){var i=this.o.getBoundsZoomLevel(new GLatLngBounds(h,h));g=Math.min(g,i-2)}this.o.setCenter(h,g);this.o.savePosition()};
YD.prototype.gU=function(a,b){var c;c=b?this.o.getBoundsZoomLevel(b):this.o.getBoundsZoomLevel(new GLatLngBounds(a,a))-2;this.o.setCenter(a,c)};
YD.prototype.Mt=function(a){if(this.sc!=a){this.sc=a;if(a){this.o.disableDoubleClickZoom();for(var b=0;b<this.fb.length;b++)this.fb[b].enableDragging()}else{this.o.enableDoubleClickZoom();for(var b=0;b<this.fb.length;b++)this.fb[b].disableDragging()}}};
YD.prototype.addMarker=function(a,b,c){var d=a+WD,e=new UD(this,d,c,b),f=e.Ln();this.o.addOverlay(f);var g=yc(this.fb,e,ZD);if(g<0)rc(this.fb,e,-(g+1));else{var h=this.fb[g];this.fb[g]=e;this.gD(h)}if(this.sc)e.enableDragging();f.setImage(a+XD);var i=f.getIcon().image;this.hC[i]=e;return e};
YD.prototype.gD=function(a){var b=a.Ln();tc(this.fb,a);this.o.removeOverlay(b);var c=b.getIcon().image;delete this.hC[c]};
YD.prototype.AJ=function(a){var b=a||this.o.getBounds().getCenter(),c=this.o.fromLatLngToDivPixel(b),d=Ah(this.e);d.width/=2;d.height/=2;var e=this.o.fromDivPixelToLatLng(new GPoint(c.x-d.width,c.y+d.height)),f=this.o.fromDivPixelToLatLng(new GPoint(c.x+d.width,c.y-d.height));return new GLatLngBounds(e,f)};
YD.prototype.FI=function(a){var b=ie(uh(this.e),Se()),c=new GPoint(a.x-b.x,a.y-b.y);return this.o.fromContainerPixelToLatLng(c)};
YD.prototype.sJ=function(){return this.nca};
YD.prototype.$O=function(a){this.nca=a};
YD.prototype.closeInfoWindow=function(){this.o.closeInfoWindow()};
YD.prototype.raa=function(a){this.dispatchEvent(new D("markermove",a))};
YD.prototype.Oe=function(a){this.dispatchEvent(new D($D,a))};
YD.prototype.xh=function(a,b){var c=null;if(a&&a.getIcon){var d=a.getIcon().image;c=this.hC[d]}if(c)this.dispatchEvent(new D($D,c));else if(b)this.dispatchEvent(new aE("mapclick",this.o,b))};
YD.prototype.Wj=function(a){a.preventDefault()};
var ZD=function(a,b){return a.getData().index-b.getData().index},
aE=function(a,b,c){D.call(this,a,b);this.latlng=c};
o(aE,D);var $D="markerclick";var bE=function(a){J.call(this);this.o=a.Al();this.kd=this.o.getInfoWindow();this.fb=a.CJ();this.cC=[GEvent.addListener(this.o,"infowindowopen",l(this.J0,this)),GEvent.addListener(this.o,"infowindowclose",l(this.l_,this)),GEvent.addListener(this.o,"movestart",l(this.s0,this)),GEvent.addListener(this.o,"moveend",l(this.r0,this))];this.j=new K(this);this.Df=new Ym(this.kG,4000,this)};
o(bE,J);bE.prototype.UC=null;bE.prototype.fk=null;bE.prototype.Dw=null;bE.prototype.fD=null;bE.prototype.Zk=null;bE.prototype.en=0;bE.prototype.sw=false;bE.prototype.Zd=false;bE.prototype.Pv=false;bE.prototype.sc=false;bE.prototype.b=function(){if(!this.W()){while(this.cC.length)GEvent.removeListener(this.cC.pop());this.j.b();bE.f.b.call(this)}};
bE.prototype.close=function(){this.o.closeInfoWindow()};
bE.prototype.open=function(a){if(!this.Zk||!this.sw||a.getData().index!=this.Zk.getData().index){this.en=yc(this.fb,a,ZD);this.aN()}};
bE.prototype.XI=function(){return this.Zk};
bE.prototype.Mt=function(a){this.sc=a};
bE.prototype.aN=function(){this.Zk=this.fb[this.en];if(this.kd.isHidden())this.So(false);this.Zk.Ln().openInfoWindow(this.jV(this.Zk.getData()));this.sP()};
bE.prototype.jV=function(a){var b=a.s+"?imgmax=400",c=a.l,d=a.w,e=a.h;this.UC=X("lhui_prev");this.fk=X("lhui_next");this.Dw=X("lhui_play");this.fD=this.sc?X("lhui_remove"):"";if(d>400||e>400){var f=d/e;if(d>e){d=400;e=d/f}else{e=400;d=e*f}}var g=cE(Number(a.lat),Number(a.lon)),h=this.fb.length;return ez(dE(h,a.c,this.sc),{lhui_prev:this.UC,lhui_next:this.fk,lhui_play:this.Dw,lhui_remove:this.fD,lhui_link:c,lhui_src:b,lhui_width:d,lhui_height:e,lhui_photoIndex:rx(this.en+1,h),lhui_caption:Xy(a.c||
""),lhui_title:Xy(a.t||"",5),lhui_lat:g.lat,lhui_lon:g.lng})};
bE.prototype.J0=function(){if(this.fb.length>1){var a=u(this.UC),b=u(this.fk),c=u(this.Dw);this.j.g(a,I,this.R0);this.j.g(b,I,this.G0);this.j.g(c,I,this.Q0)}if(this.sc){var d=u(this.fD);this.j.g(d,I,this.Ku)}if(this.Zd&&!this.Pv)this.Df.start();this.sw=true;this.dispatchEvent("open");Uz().log(128)};
bE.prototype.l_=function(){if(this.Df.zd())this.So(false);this.j.ma();this.dispatchEvent("close");Rg(this.tU,0,this)};
bE.prototype.R0=function(){this.So(false);this.we(-1)};
bE.prototype.G0=function(){this.So(false);this.we(1)};
bE.prototype.Q0=function(){this.So(!this.Zd)};
bE.prototype.Ku=function(){this.dispatchEvent(new D("removemarker",this.Zk))};
bE.prototype.s0=function(){if(this.Zd&&!this.Df.zd())this.Pv=true};
bE.prototype.r0=function(){if(this.Pv){this.Pv=false;this.Df.start()}};
bE.prototype.So=function(a){this.Zd=a;this.sP();if(a)this.Df.start(2000);else this.Df.stop()};
bE.prototype.sP=function(){var a=u(this.Dw);if(a)B(a,this.Zd?ev:fv)};
bE.prototype.kG=function(){if(!this.kd.isHidden())this.we(1);else this.So(false)};
bE.prototype.we=function(a){this.en=qe(this.en+a,this.fb.length);var b=this.fb[this.en];if(b)this.aN()};
bE.prototype.tU=function(){this.sw=false};
bE.prototype.isOpen=function(){return this.sw};
var cE=function(a,b){var c={};a=a.toFixed(6);b=b.toFixed(6);c.lat=a<0?lx(-a):kx(a);c.lng=b<0?nx(-b):mx(b);return c},
dE=function(a,b,c){var d=['<div class="lhcl_infowindow">'];if(a>1)d.push('<table class="lhcl_photoinfobar"><tbody><tr><td width="25%" style="valign:center"><span class="lhcl_fakelink" id="{lhui_play}"></span></td><td width="25%" style="text-align:right"><img src="img/left_normal.gif" id="{lhui_prev}" /></td><td width="25%"><img src="img/right_normal.gif" id="{lhui_next}" /></td><td width="25%" style="text-align:right"><div class="lhcl_photoinfo">{lhui_photoIndex}</div></td></tr></tbody></table>');
d.push('<div class="lhcl_infophoto"><a href="{lhui_link}"><img src="{lhui_src}" width="{lhui_width}" height="{lhui_height}" /></a></div>');if(b)d.push('<div class="lhcl_caption" style="width: 400px">{lhui_caption}</div>');d.push('<table class="lhcl_photoinfobar"><tbody><tr><td width="25%"><div class="lhcl_photoinfo">{lhui_title}</div></td><td width="25%" style="text-align:right"><div class="lhcl_photoinfo">{lhui_lat}</div></td><td width="25%"><div class="lhcl_photoinfo">{lhui_lon}</div></td><td width="25%" style="text-align:right">');
if(c)d.push('<span class="lhcl_fakelink" id="{lhui_remove}">'+$x+"</span>");else{var e=m("View full size");d.push('<a href="{lhui_link}">'+e+"</a>")}d.push("</td></tr></tbody></table></div>");return d.join("")};var eE=function(a,b,c){P.call(this);this.tk(a);this.U(b);this.fc=_user;this.Yy=_authuser;this.$l=c};
o(eE,P);eE.prototype.db="";eE.prototype.vz=null;eE.prototype.sc=false;eE.prototype.b=function(){if(!this.W())eE.f.b.call(this)};
eE.prototype.m=function(){this.e=v("div",{"class":this.db})};
eE.prototype.u=function(){if(this.Eb)A(this.e);eE.f.u.call(this)};
eE.prototype.n=function(){eE.f.n.call(this);this.Lc()};
eE.prototype.D=function(){return"gphoto.map.MapHeader"};
eE.prototype.kb=function(){return false};
eE.prototype.tk=function(a){this.db=a;if(this.Eb)of(this.e,this.db)};
eE.prototype.gaa=function(a){if(this.vz){var b;if(this.$l==0)b="Albums: "+a;else if(this.$l==1)b="Photos: "+a;B(this.vz,b)}};
eE.prototype.m8=function(a){this.sc=a;this.Lc()};
eE.prototype.Lc=function(){if(!this.Eb)return;var a;if(this.$l==0)a=this.FY();else if(this.$l==1)a=this.XX();a.lhui_countDiv=X("lhui_countDiv");var b=fE(this.$l,this.sc);Uy(this.e,gz(b,a));this.vz=u(a.lhui_countDiv)};
eE.prototype.FY=function(){return{lhui_src:this.k.portrait,lhui_imgLink:this.k.link,lhui_owner:this.fc.nickname}};
eE.prototype.XX=function(){var a="lhcl_public",b=Qu;if(!_authuser.isOwner){a+="_foreign";b=""}else if(this.fc.searchable){a+="_searchable";b=Ru}a+="_label";var c=ob(this.k.title,14,true),d=c!=this.k.title,e=ob(this.k.location,18,true),f=e!=this.k.location,g=as(this.k.largepreview);g=cs(g,64,true);return{lhui_src:g,lhui_imgLink:this.k.link,lhui_title:c,lhui_titleFull:d?this.k.title:"",lhui_location:e,lhui_locationFull:f?this.k.location:"",lhui_date:this.k.dateonly,lhui_visibilityClass:this.k.unlisted?
"lhcl_unlisted_label":a,lhui_visibility:this.k.unlisted?$w:ax,lhui_visibilityTip:this.k.unlisted?"":b,lhui_owner:tx(this.fc.nickname)}};
var fE=function(a,b){var c=['<div class="lhcl_prettybox lhcl_recordbox" style="overflow: hidden;"><div class="lhcl_body">'];if(b)c.push(Cu);else{c.push('<table class="lhcl_host"><tr><td><a href="{lhui_imgLink}"><img src="{lhui_src}"/></a></td>');if(a==0)c.push('<td width="100%"><div class="lhcl_host"><div class="lhcl_name">{lhui_owner}</div><div id="{lhui_countDiv}"></div>');else if(a==1)c.push('<td style="padding-left: 6px" width="100%"><div class="lhcl_info"><div class="lhcl_title" title="{lhui_titleFull}">{lhui_title}</div><div class="lhcl_mapHeaderMeta"><div>{lhui_owner}</div><div title="{lhui_locationFull}">{lhui_location}</div><div>{lhui_date} &#8211; <span class="{lhui_visibilityClass}" title="{lhui_visibilityTip}">{lhui_visibility}</span></div><div id="{lhui_countDiv}"></div></div>');
c.push("</div></td></tr></table>")}c.push("</div></div>");return c.join("")};var gE=function(){Xl.call(this);var a=this.Dg();a.g(document,I,this.Oe,true);a.g(document,eg,this.Vj,true)};
o(gE,Xl);var hE=m("Center map here");gE.prototype.dz=false;gE.prototype.lQ=null;gE.prototype.nQ=null;gE.prototype.oQ=null;gE.prototype.mQ=null;gE.prototype.zd=function(){return this.dz||this.Ja()||this.eR()};
gE.prototype.nq=function(){return this.Pi};
gE.prototype.LZ=function(){return this.lQ};
gE.prototype.XO=function(a){this.Wz=a};
gE.prototype.iP=function(a){this.nQ=a;if(a&&this.Wz){this.Kx();this.ub(new Ol($x,"remove"))}};
gE.prototype.Ln=function(){return this.nQ};
gE.prototype.cP=function(a){this.oQ=a;if(a&&a.length&&ic(a,function(b){return b.na().marker})){this.Kx();
this.ub(new Ol(hE,"center"));if(this.Wz){this.Kx();this.ub(new Ol($x,"remove"))}}};
gE.prototype.IY=function(){return this.oQ};
gE.prototype.gP=function(a){this.mQ=a;if(a){this.Kx();this.ub(new Ol(cy,"zoomin"));this.ub(new Ol(dy,"zoomout"));this.ub(new Ol(hE,"center"))}};
gE.prototype.UY=function(){return this.mQ};
gE.prototype.Kx=function(){if(this.Ag())this.ub(new Tl)};
gE.prototype.Oe=function(a){this.dz=a.Vl(2)};
gE.prototype.Vj=function(a){if(a.Vl(2))this.dz=false};
gE.prototype.Xo=function(a,b,c){this.oU();this.Pi=new ge(b,c);this.lQ=a.e;gE.f.Xo.call(this,a,b,c)};
gE.prototype.oU=function(){this.XO(false);this.iP(null);this.cP(null);this.gP(null);while(this.Ag())this.Kh(0)};
gE.prototype.rc=function(a){gE.f.rc.call(this,a,undefined,undefined,true)};
gE.prototype.Us=function(a){this.j.g(a.e,a.hA,this.ow,true)};
gE.prototype.zt=function(a){this.j.Sa(a.e,a.hA,this.ow,true)};var iE=function(a,b,c,d){this.j=new K(this);var e=Ah(a),f={lhui_georesults:X("lhui_georesults"),width:e.width-10};if(b.length>0)f.lhui_header=Px;this.BN=gz('<div id="lhid_georesults_popup" class="lhcl_geoResultPopup" style="width: {width}px;"><h2>{lhui_header}</h2><div id="{lhui_georesults}"></div></div>',f);a.appendChild(this.BN);il.call(this,this.BN);var g=u(f.lhui_georesults);if(b.length==0){this.LO(true);var h=p(d);g.innerHTML=Lx('<span style="font-weight: bold;">'+Xy(h)+"</span>")}else{this.LO(false);
this.tV(g,b,c)}this.a9(1);this.setPosition(new pl(a,3));this.M(true)};
o(iE,il);iE.prototype.tV=function(a,b,c){c=Math.min(b.length,c);for(var d=0;d<c;d++){var e=p(b[d].address),f=gz('<div class="lhcl_geoResult"><span class="lhcl_fakelink">{address}</span></div>',{address:e});a.appendChild(f);this.j.g(f,E,l(this.b1,this,b[d]))}};
iE.prototype.b1=function(a){if(a.bb)this.dispatchEvent(new jE(kE,a));else this.Ut(a)};
iE.prototype.Ut=function(a){var b=[_geocodePath,"location="+ab(a.address),"lat="+a.lat,"lon="+a.lon,"mresults=false","alt=json"];Gj(b.join("&"),l(this.Xs,this,a))};
iE.prototype.Xs=function(a,b){var c=b.target;try{if(c.Re()){var d=c.Uf();if(xa(d)&&d.length)a=d[0]}}catch(e){}this.dispatchEvent(new jE(kE,a))};
iE.prototype.b=function(){this.M(false);this.j.b();il.prototype.b.call(this);A(this.BN)};
var kE="selectresult",jE=function(a,b){D.call(this,a);this.geoResult=b};
o(jE,D);var mE=function(){R.call(this,null,true);this.Fp();var a=m("Enter a location");this.ck=new lE(this.Oa(),a,Rx,"lhcl_locationdialog");this.M(true);var b=this.ck.vZ();b.focus();b.select();this.w7=F(this.ck,"geocode",this.W0,false,this)};
o(mE,R);mE.prototype.Fp=function(){Uy(this.Au(),v("h1",null,Sx));var a=new Xk;a.J(dl,wx,false,true);this.Rg(a)};
mE.prototype.b=function(){if(!this.W()){R.prototype.b.call(this);if(this.w7)H(this.w7)}};
mE.prototype.W0=function(a){this.dispatchEvent(a);this.M(false)};var lE=function(a,b,c,d){var e={lhui_searchLabel:b,lhui_searchButtonLabel:c,lhui_divClass:d||"",lhui_popupAnchor:X("lhui_popupAnchor")};x(a,gz('<div class="{lhui_divClass}"><div id="{lhui_popupAnchor}"><form action="javascript:void 0"><b>{lhui_searchLabel}</b> <input type="text" class="lhcl_input" maxlength="100"/> <button type="submit" class="lhcl_locationsearchbutton">{lhui_searchButtonLabel}</button></form></div></div>',e));var f=a.getElementsByTagName("form")[0];u(e.lhui_doneButton);this.Gda=
u(e.lhui_popupAnchor);this.Ib=f.getElementsByTagName("input")[0];this.j=new K(this);this.j.g(f,"submit",this.Nl);this.j.g(window,mg,this.nb);J.call(this)};
o(lE,J);lE.prototype.PC=null;lE.prototype.om=null;lE.prototype.b=function(){if(!this.W()){this.Pz();this.j.b();J.prototype.b.call(this)}};
lE.prototype.Pz=function(){if(this.om){H(this.PC);this.PC=null;this.om.b();this.om=null}};
lE.prototype.vZ=function(){return this.Ib};
lE.prototype.Nl=function(){this.Pz();this.search(this.Ib.value)};
lE.prototype.search=function(a){a=Va(a);if(a){var b=[_geocodePath,"location="+ab(a),"alt=json"];Gj(b.join("&"),l(this.v7,this,a))}};
lE.prototype.v7=function(a,b){var c=b.target,d;try{d=c.Uf()}catch(e){}if(d)if(d.length!=1||d[0].spellCorrection){this.om=new iE(this.Gda,d,5,a);this.PC=F(this.om,kE,this.a1,false,this)}else this.dispatchEvent(new nE(d[0]))};
lE.prototype.a1=function(a){this.Ib.value=a.geoResult.address;this.dispatchEvent(new nE(a.geoResult));this.Pz()};
lE.prototype.nb=function(){if(this.om)this.om.Qa()};
var nE=function(a){D.call(this,"geocode");this.geoResult=a};
o(nE,D);var pE=function(){R.call(this,null,true);Uy(this.Au(),v("h1",null,Sx));this.hc(oE);this.Rg(al);this.Mp=F(this,Zk,this.of,false,this);this.M(true)};
o(pE,R);pE.prototype.mk=false;pE.prototype.b=function(){if(!this.W()){pE.f.b.call(this);if(this.Mp)H(this.Mp)}};
pE.prototype.M=function(a){if(!this.mk)pE.f.M.call(this,a)};
pE.prototype.of=function(a){if(a.key=="yes"){this.mk=true;this.jy("EXIF_DIALOG",false);this.jy("EXIF_GEO",true,l(this.O9,this))}else if(a.key=="no"){this.jy("EXIF_GEO",false);this.jy("EXIF_DIALOG",false,l(this.J1,this))}};
pE.prototype.jy=function(a,b,c){var d=["pref="+a,"value="+b].join("&");Gj(_setPrefUrl,c,"POST",d)};
pE.prototype.J1=function(a){if(!a.target.Re())alert(Kv)};
pE.prototype.O9=function(a){this.mk=false;if(!a.target.Re()){alert(Kv);return}window.location.reload()};
var qE=m("We've noticed that some photos in this album have location information. You currently do not display this EXIF location data. Would you like to change your default setting to display this location information? (Note: you can change this setting in the future by going to the Settings page)"),oE='<div style="width:50em">'+qE+"</div>";var sE=function(){Ez.Hd(this);this.Fb=!(!_authuser.isOwner);this.Da=_album;this.j=new K(this);this.ya=new W(QD,"lhcl_iconList","lhcl_mapIcon");this.ya.gI(true);this.mM=new eE("lhcl_albumMapHeader",this.Da,1);var a=u("lhid_leftbox");while(a.className!="lhcl_body"&&a.parentNode)a=a.parentNode;this.h5=a;this.OP(false);this.e=gz(rE,{lhui_albumName:_album.title});x(this.h5,this.e);this.$aa=u("lhid_iconHeader");this.mM.C(this.$aa);this.nc=u("lhid_iconListContainer");this.ya.C(this.nc);this.lM=u("lhid_mapToolHeader");
this.Xq=u("lhid_mapElement");this.j.g(window,mg,this.nb);this.Wx=[];this.R=new gE(this);Oz(l(this.Hv,this))};
sE.prototype.ha=function(){Ez.ee(this);this.R.b();this.j.b();this.ya.b();this.Er.b();if(this.It)H(this.It);if(this.xD)H(this.xD);if(this.ck)this.ck.b();if(this.fM)this.fM.b();if(this.Wb)this.Wb.b();if(this.kd)this.kd.b()};
sE.prototype.Fc=function(){this.ha();A(this.e);this.OP(true)};
sE.prototype.Hv=function(){if(typeof GBrowserIsCompatible!="undefined"&&GBrowserIsCompatible()){this.GW=new di(this.Xq);this.ya.HS(this.GW);this.j.g(this.GW,"drop",this.H_);this.Wb=new YD(this.Xq);this.R.C(document.body);this.R.rc(this.Xq);this.R.rc(this.nc);this.kd=new bE(this.Wb);this.$z(!this.Da.hasGeotaggedPhotos);this.nb();if(this.Da.hasGeotaggedPhotos)this.w5();else if(this.Da.lat&&this.Da.lon)this.PO(this.Da);else{this.Wb.mz();this.fM=new mE;this.j.g(this.fM,"geocode",this.zK)}this.j.g(this.Wb,
$D,this.u0);this.j.g(this.Wb,"markermove",this.v0);this.j.g(this.Wb,"mapclick",this.q0);this.j.g(this.R,"show",this.e_);this.j.g(this.R,xk,this.w0);this.j.g(this.kd,"open",this.g0);this.j.g(this.kd,"close",this.f0);this.j.g(this.kd,"removemarker",this.h0);this.j.g(this.ya,lg,this.Ml);this.V()}};
sE.prototype.OP=function(a){var b=$e(this.h5);while(b){M(b,a);b=af(b)}};
sE.prototype.$z=function(a){a=a&&this.Fb;if(this.sc!=a){var b=u("lhid_navbar_nz");if(b)M(b,!a);of(this.e,a?"lhcl_mapEditMode":"lhcl_mapViewMode");this.sc=a;this.ya.Ug(a?2:1);this.A8();var c="lhcl_iconList";if(a){c+="-edit";this.CW()}else this.FW();this.ya.tk(c);if(this.Wb){this.kd.close();this.kd.Mt(a);this.Wb.Mt(a);this.ya.enableDragging(a);this.mM.m8(a)}this.nb()}};
sE.prototype.QG=function(){if(this.ck){this.ck.b();this.ck=null}We(this.lM);if(this.It)H(this.It);if(this.xD)H(this.xD)};
sE.prototype.A8=function(){var a;a=this.sc?_photo:fc(_photo,function(c){return c.lat&&c.lon});
if(!this.Er||this.Er.S()!=a.length){this.Er=new wt(a);this.ya.U(this.Er);this.mM.gaa(this.Er.S())}for(var b=0;b<a.length;b++)a[b].index=b};
sE.prototype.w5=function(){var a=this.ya.Ag();for(var b=0;b<a;b++){var c=this.ya.wd(b),d=c.na();if(d.lat&&d.lon)this.PF(d)}this.Wb.mz()};
sE.prototype.FW=function(){this.QG();var a={lhui_albumLink:this.Da.link,lhui_editButtonContainer:X("lhui_editButtonContainer"),lhui_linkContainer:X("lhui_linkContainer")};x(this.lM,gz(tE,a));if(this.Fb){var b=m("Edit Map"),c=u(a.lhui_editButtonContainer),d=gz('<div class="lhcl_uibutton lhcl_ur"><p class="lhcl_ul"></p><p class="lhcl_bl"><em class="lhcl_br"><img src="{lhui_img}">{lhui_text}</em></p></div>',{lhui_img:"img/mapped_sm.gif",lhui_text:b});x(c,d);this.It=F(d,E,l(this.$z,this,true))}c=u(a.lhui_linkContainer);
if(c&&this.Er.S()>0){var e=gz('<div><a href="{lhui_href}"><img src="{lhui_img}" /></a> <a href="{lhui_href}">{lhui_text}</a></div>',{lhui_href:this.Da.kmlLink,lhui_img:"img/kml.jpg",lhui_text:cx});x(c,e)}};
sE.prototype.CW=function(){this.QG();var a={lhui_albumLink:this.Da.link,lhui_doneButton:X("lhui_doneButton"),lhui_search:X("lhui_search")};x(this.lM,gz(uE,a));var b=u(a.lhui_search);this.ck=new lE(b,Qx,Rx,"lhcl_mapsearch");this.xD=F(this.ck,"geocode",this.zK,false,this);var c=u(a.lhui_doneButton);this.It=F(c,E,l(this.$z,this,false))};
sE.prototype.nb=function(){var a=vh(this.Xq),b=Math.max(Qe().height-u("lhid_footer").offsetHeight-a-7,300);L(this.Xq,"height",b+"px");var c=b-(vh(this.nc)-a);L(this.nc,"height",c+"px");if(this.Wb)this.Wb.Al().checkResize()};
sE.prototype.zK=function(a){this.PO(a.geoResult)};
sE.prototype.PO=function(a){if(this.Wb&&a){var b,c=ND(a),d=new GLatLng(c.lat,c.lon);if(a.bb)b=PD(a.bb);this.Wb.gU(d,b)}};
sE.prototype.v0=function(a){var b=a.target,c=b.getPoint(),d=b.getData();this.QD(d,c);this.WE()};
sE.prototype.QD=function(a,b){var c=a.lat&&a.lon?new GLatLng(Number(a.lat),Number(a.lon)):null;if(b==c||b&&c&&b.equals(c))return;if(b){var d=this.Wb.AJ(b);a.lat=String(b.lat());a.lon=String(b.lng());a.bb=MD(d)}else{delete a.lat;delete a.lon;delete a.bb}this.Wx.push(a)};
sE.prototype.WE=function(){if(this.Wx.length>0){var a=gc(this.Wx,this.$W,this),b=["iids="+a.join(",")],c=this.Wx[0];if(c.lat&&c.lon)b.push("lat="+c.lat,"lon="+c.lon,"latspan="+c.bb.latSpan,"lonspan="+c.bb.lonSpan);else b.push("removegeo=true");Gj(_batchUpdateGeoPath,null,"POST",b.join("&"));oc(this.Wx)}};
sE.prototype.$W=function(a){return a.id};
sE.prototype.u0=function(a){if(this.R.zd())return;var b=a.target;this.kd.open(b)};
sE.prototype.g0=function(){var a=this.kd.XI();this.ya.wO(a.getData().index)};
sE.prototype.f0=function(){if(!this.sc)this.ya.Pd()};
sE.prototype.h0=function(a){if(this.sc)this.VN([a.target])};
sE.prototype.Ml=function(){if(!this.sc){var a=this.ya.oe();if(a.length==1){var b=a[0],c=b.na().marker;if(c)this.kd.open(c)}}};
sE.prototype.H_=function(a){if(this.Wb){var b=this.Wb.FI(new ge(a.clientX,a.clientY));this.QF(b)}};
sE.prototype.QF=function(a){this.kd.close();var b=this.ya.oe();this.Wb.AJ(a);if(b.length>1){var c=fc(b,function(h){return h.na().marker}),
d=m("Some photos in your selection are already mapped. Are you sure you want to overwrite the current locations?\n(Mapped photos in selection: {$count})",{count:c.length}),e=m("Some photos in your selection are already mapped. Are you sure you want to overwrite the current locations?{$lineBreak}(Mapped photos in selection: {$count})",{lineBreak:"\n",count:c.length});if(c.length&&!confirm(_features.newStrings?e:d))return}for(var f=0;f<b.length;f++){var g=b[f].na();this.QD(g,a);this.PF(g)}this.WE()};
sE.prototype.PF=function(a){if(a.marker)this.Wb.gD(a.marker);this.Da.hasGeotaggedPhotos=true;var b=new GLatLng(Number(a.lat),Number(a.lon)),c=this.Wb.addMarker(a.s,a,b),d=this.ya.wd(a.index);d.aa();a.marker=c};
sE.prototype.VN=function(a){var b=this.kd.isOpen(),c=this.kd.XI(),d=m("Are you sure you want to remove the selected photos from the map?\n(Mapped photos in selection: {$count})",{count:a.length}),e=m("Are you sure you want to remove the selected photos from the map?{$lineBreak}(Mapped photos in selection: {$count})",{lineBreak:"\n",count:a.length});if(a.length>1&&!confirm(_features.newStrings?e:d))return;for(var f=0;f<a.length;f++){var g=a[f],h=g.getData();if(b&&g==c)this.kd.close();this.QD(h,null);
this.Wb.gD(g);delete h.marker;this.ya.wd(h.index).aa()}this.Da.hasGeotaggedPhotos=this.Wb.CJ().length>0;this.WE()};
sE.prototype.q0=function(a){if(this.R.Ja()){this.R.M(false);return}var b=this.ya.oe();if(!this.sc||this.R.zd()||this.kd.isOpen()||!b.length||ic(b,function(h){return h.na().marker}))return;
var c=a.latlng,d=c.lat();if(d>=-85&&d<=85)this.QF(c);var b=this.ya.oe();if(b.length){var e=bc(b).Eg()+1,f=this.ya.Ag();for(var g=e;g<f;g++)if(!this.ya.wd(g).na().marker){this.ya.wO(g);break}}};
sE.prototype.e_=function(){this.R.XO(this.sc);if(this.R.LZ()==this.Xq){this.R.gP(this.Wb.FI(this.R.nq()));this.R.iP(this.Wb.sJ());this.kd.close()}else if(this.sc)this.R.cP(this.ya.oe())};
sE.prototype.w0=function(a){var b=a.target,c=b.na(),d=this.Wb.Al(),e=this.R.UY(),f=this.R.IY(),g=this.R.Ln(),h;if(f){h=gc(f,function(aa){return aa.na().marker});
h=fc(h,function(aa){return aa})}else if(g)h=[g];
if(c=="center"){if(e)d.panTo(e);else if(h){var i=gc(h,function(aa){return aa.getPoint()}),
k;if(h.length==1){var n=h[0].getData().bb;k=n?PD(n):null}else k=new GLatLngBounds(i[0],i[0]);for(var q=1;q<i.length;q++)k.extend(i[q]);var t=d.getZoom(),y=k?d.getBoundsZoomLevel(k):t,z;if(h.length==1){var O=y-t;if(Math.abs(O)<2)y=t;z=h[0].getPoint()}else z=k.getCenter();if(y==t)d.panTo(z);else d.setCenter(z,y)}}else if(c=="remove")this.VN(h);else if(c=="zoomin")d.setCenter(e,d.getZoom()+1);else if(c=="zoomout")d.setCenter(e,d.getZoom()-1)};
sE.prototype.V=function(){Uz().log(127)};
var rE='<div class="lhcl_mapViewMode"><div class="lhcl_editcontrols"><div class="lhcl_title">'+Ux+': <span class="lhcl_albumname">{lhui_albumName}</span></div></div><table class="lhcl_albumMapContainer"><tr><td><div id="lhid_iconHeader"></div><div id="lhid_iconListContainer"></div></td><td width="100%"><div id="lhid_mapToolHeader"></div><div id="lhid_mapElement" class="lhcl_mapViewBorder"></div></td></tr></table></div>',uE='<div class="lhcl_editMapHeader"><div class="lhcl_toolbar"><table><tbody><tr><td><button id="{lhui_doneButton}"><b>'+
Zx+'</b></button></td><td width="100%"></td><td><div id="{lhui_search}"></div></td></tr></tbody></table></div></div>',tE='<div class="lhcl_viewMapHeader"><div class="lhcl_albumbox"><div class="lhcl_header"><table class="lhcl_tableheader lhcl_editbuttons"><tbody><tr><td id="{lhui_editButtonContainer}"></td><td width="100%"><a href="{lhui_albumLink}">'+Du+'</a></td><td id="{lhui_linkContainer}"></td></tr></tbody></table></div></div></div>';var gB=function(a,b,c,d,e,f,g,h,i,k,n,q,t,y,z){this.cba=z||false;this.r=a;this.MV=b;this.$l=q;this.o=new WA(c,this.$l,this.pH());if(this.r.lat&&this.r.lon){this.o.BD(this.r.id);this.o.savePosition()}this.Tw=d;this.lda=t;this.gM=e;this.Y2=f;this.Xca=g;this.Z2=h;this.v3=i;this.ht=k;this.dr=n;this.pl=new Ym(this.S4,600,this);this.j=new K(this);this.j.g(e,gg,this.rr);this.j.g(e,hg,l(this.pl.start,this.pl,600));this.j.g(this.o,vE,this.AW);this.i2=y||"";this.lO=X("lhui_func");Y(this.lO,l(this.B7,this));
this.ky()};
o(gB,Vd);gB.prototype.OL=null;gB.prototype.uA=true;gB.prototype.b=function(){if(this.W())return;Vd.prototype.b.call(this);this.o.b();this.j.b();if(this.pl)this.pl.b();Z(this.lO)};
gB.prototype.bq=function(){if(!this.uA)return;var a=Va(this.gM.value);if(a){Jy(true,this.i2);var b=["location="+ab(a),"alt=json"],c=_geocodePath+"&"+b.join("&");Gj(c,l(this.PX,this));this.OL=a}};
gB.prototype.MG=function(){this.PE("","",null);this.KE(null,null,null);this.ky();this.o.reset();this.o.savePosition()};
gB.prototype.cY=function(){return this.r.bb};
gB.prototype.AK=function(a,b){var c=[];if(a.length==1&&!a[0].spellCorrection){a[0].hasBoundingBox=true;this.ND(a[0]);c=null}else if(a.length>0){var d=Math.min(a.length,this.lda);for(var e=0;e<d;e++){c[e]=a[e];c[e].index=e}this.tA=c;this.bx=-1;this.o.rL(4,{geoResults:c})}this.ky(c,b)};
gB.prototype.Al=function(){return this.o};
gB.prototype.fI=function(a){this.uA=a};
gB.prototype.pH=function(){var a={};a[this.MV+"s"]=[this.r];return a};
gB.prototype.S4=function(){if(!this.uA)return;var a=Va(this.gM.value);if(a&&this.OL!=a)if(!this.cba&&this.r.lat&&this.r.lon)this.Y5();else this.bq()};
gB.prototype.rr=function(a){if(this.pl)this.pl.stop();if(a.keyCode==13){this.bq();Ny(a)}};
gB.prototype.PX=function(a){if(this.pl)this.pl.stop();Jy(false,this.i2);var b=a.target,c;try{c=b.Uf()}catch(d){this.P9();return}if(c)this.AK(c)};
gB.prototype.PE=function(a,b,c){this.Y2.value=a;this.Xca.value=b;this.ht.value=a&&b?"false":"true";if(c){this.Z2.value=c.latSpan;this.v3.value=c.lonSpan}else{this.Z2.value="";this.v3.value=""}};
gB.prototype.KE=function(a,b,c){this.r.lat=a;this.r.lon=b;if(c){if(!this.r.bb)this.r.bb={ll:{},ur:{}};this.r.bb.latSpan=c.latSpan;this.r.bb.lonSpan=c.lonSpan;this.r.bb.ll.lat=c.ll.lat;this.r.bb.ll.lon=c.ll.lon;this.r.bb.ur.lat=c.ur.lat;this.r.bb.ur.lon=c.ur.lon}else this.r.bb=null};
gB.prototype.Ut=function(a){var b=["location="+ab(a.address),"lat="+a.lat,"lon="+a.lon,"mresults=false","alt=json"],c=_geocodePath+"&"+b.join("&");Gj(c,l(this.Xs,this,a))};
gB.prototype.Xs=function(a,b){var c=b.target;try{if(c.Re()){var d=c.Uf();if(d&&d.length>0){var e=d[0];a.lat=e.lat;a.lon=e.lon;a.bb=e.bb}}}catch(f){}a.hasBoundingBox=true;if(this.bx==a.index)this.ND(a)};
gB.prototype.ND=function(a){var b=a.bb;if(!b&&!a.hasBoundingBox){this.Ut(a);return}this.KE(a.lat,a.lon,b);this.PE(a.lat,a.lon,b);this.o.rL(this.$l,this.pH());this.o.BD(this.r.id);this.o.savePosition()};
gB.prototype.P9=function(){var a=m("There was an error while trying to determine the location."),b=gz(wE,{text:a});Uy(this.Tw,b)};
gB.prototype.ky=function(a,b){var c;if(b)c=b;else if(!a)c=this.Y2.value?this.dr.geotaggedMessage:this.dr.noGeotagMessage;else if(a.length==0){var d=p(this.gM.value);c=Lx('<span style="font-weight: bold;">'+Xy(d)+"</span>")}else{this.oW(a);return}var e=gz(wE,{text:c});Uy(this.Tw,e)};
gB.prototype.oW=function(a){Uy(this.Tw,v("span",{"class":"lhcl_didyou"},Px));for(var b=0;b<a.length;b++){var c=String.fromCharCode("A".charCodeAt(0)+b),d={index:b,id:"result"+b,resultfunc:this.lO,letter:c,address:Xy(p(a[b].address))},e=gz('<div class="lhcl_geoResult"><span id="{id}" class="lhcl_fakelink" onclick="_d(\'{resultfunc}\', {index})">{letter}. {address}</span></div>',d);x(this.Tw,e)}};
gB.prototype.Y5=function(){var a={msg:this.dr.updateGeoPrompt,yesId:X("lhui_func"),noId:X("lhui_func")};Uy(this.Tw,gz(xE,a));var b=u(a.yesId),c=u(a.noId);this.j.g(b,I,this.bq);this.j.g(c,I,l(this.ky,this,null,null))};
gB.prototype.B7=function(a){if(a==this.bx)return;if(this.bx>=0){var b=u("result"+this.bx);b.className="lhcl_fakelink"}this.bx=a;var c=u("result"+a);c.className="lhcl_selectedResult";this.ND(this.tA[a])};
gB.prototype.AW=function(a){if(a.type==vE){var b=a.marker.getPoint(),c=String(b.lat()),d=String(b.lng()),e=this.o.savePosition({lat:c,lon:d});this.KE(c,d,e);this.PE(c,d,e)}};
var wE="<div>{text}</div>",xE='<div>{msg}<br /><span class="lhcl_fakelink" id="{yesId}">'+ey+'</span>&#32;<span class="lhcl_fakelink" id="{noId}">'+Xx+"</span></div>";var XA=function(a,b,c,d){R.call(this,null,true);this.r={location_id:X("input"),lat_id:X("input"),lon_id:X("input"),latSpan_id:X("input"),lonSpan_id:X("input"),removeGeo_id:X("input"),spin_id:X("img"),results_id:X("div"),map_id:X("map"),geocodefunc:X("func"),cleargeofunc:X("func"),lat:a.lat,lon:a.lon,location:c};if(a.bb){this.r.latSpan=a.bb.latSpan;this.r.lonSpan=a.bb.lonSpan}this.ba=a;this.tA=d?d:[];if(b)this.ud=b;this.T4()};
o(XA,R);XA.prototype.Wd=false;XA.prototype.QZ=function(){var a;if(this.ba)a=this.ba.user;if(!a)a=_user;return a};
XA.prototype.ZX=function(){var a;if(this.ba)a=this.ba.album;if(!a)a=_album;return a};
XA.prototype.b=function(){if(!this.W()){R.prototype.b.call(this);if(this.Wd){Z(this.r.geocodefunc);Z(this.r.cleargeofunc);this.ql.b();this.ql=null;H(this.Mp)}this.r=null}};
XA.prototype.Ja=function(){return this.W()?false:R.prototype.Ja.call(this)};
XA.prototype.T4=function(){Uy(this.Au(),gz("<h1>{title}</h1>",{title:Sx}));Uy(this.Oa(),gz(yE,this.r));var a=m("Save location"),b=new Xk;b.J("save",a,false);b.J(dl,wx,false,true);this.Rg(b);this.M(true);var c=u(this.r.location_id);c.focus();c.select();var d={id:this.ba.id,lat:this.ba.lat,lon:this.ba.lon};if(this.ba.bb)d.bb={ll:{lat:this.ba.bb.ll.lat,lon:this.ba.bb.ll.lon},ur:{lat:this.ba.bb.ur.lat,lon:this.ba.bb.ur.lon}};this.ql=new gB(d,"photo",u(this.r.map_id),u(this.r.results_id),u(this.r.location_id),
u(this.r.lat_id),u(this.r.lon_id),u(this.r.latSpan_id),u(this.r.lonSpan_id),u(this.r.removeGeo_id),{noGeotagMessage:Mx(Qx),geotaggedMessage:Tx},6,10,this.r.spin_id,true);Y(this.r.geocodefunc,l(this.ql.bq,this.ql));Y(this.r.cleargeofunc,l(this.ql.MG,this.ql));if(!(d.lat&&d.lon)&&this.tA.length>0)this.ql.AK(this.tA);this.Wd=true;this.Qa();this.Mp=F(this,Zk,l(this.of,this));var e=document.getElementsByName("save");if(e)L(e[0],"font-weight","bold");Iy(this,Gy)};
XA.prototype.of=function(a){if(a.key=="save"){var b=u(this.r.lat_id).value,c=u(this.r.lon_id).value,d=u(this.r.latSpan_id).value,e=u(this.r.lonSpan_id).value,f=u(this.r.removeGeo_id).value;if(b&&c||f=="true"&&this.ba.lat&&this.ba.lon){var g=this.QZ(),h=this.ZX(),i=["uname="+g?g.name:"","aname="+h?h.name:"","iid="+this.ba.id,"lat="+b,"lon="+c,"latspan="+d,"lonspan="+e,"removegeo="+f,"noredir=true"].join("&"),k=_updatePhotoPath,n;Gj(k,l(this.uaa,this,this.ba,b,c,this.ql.cY(),this.ud),"POST",i,n)}}Iy(this,
Hy)};
XA.prototype.uaa=function(a,b,c,d,e,f){var g=f.target;if(g.Re()){if(b&&c){a.lat=b;a.lon=c;a.bb=d}else{a.lat=null;a.lon=null;a.bb=null}if(e)e()}};
var yE='<div class="lhcl_geophotoDialog"><b>'+Qx+'</b> <input type="text" class="lhcl_input" name="location" id="{location_id}" value="{location}" maxlength="100" /><button class="lhcl_locationsearchbutton" onclick="_d(\'{geocodefunc}\')">'+Rx+'</button><img src="img/spin.gif" style="visibility:hidden; margin: 0 4px" id="{spin_id}" /><br /><div class="lhcl_align_right"><a href="javascript:_d(\'{cleargeofunc}\')">'+$x+'</a><br /></div><input type="hidden" name="lat" id="{lat_id}" value="{lat}" /><input type="hidden" name="lon" id="{lon_id}" value="{lon}" /><input type="hidden" name="latspan" id="{latSpan_id}" value="{latSpan}" /><input type="hidden" name="lonspan" id="{lonSpan_id}" value="{lonSpan}" /><input type="hidden" name="removegeo" id="{removeGeo_id}" value="false" /><table width="720px" class="lhcl_geoResultTable"><tr><td width="20%" valign="top"><div id="{results_id}"></div></td><td width="80%"><div id="{map_id}" class="lhcl_mapBorder lhcl_smallMap"></div></td></tr></table><br /></div>';var WA=function(a,b,c){Ez.Hd(this);J.call(this);a.photoMap=this;this.eL=a;this.da=b;this.yQ=c;this.fb=[];this.iL={};this.Vfa=vz;Oz(l(this.E3,this))};
o(WA,J);var vE="markermove",zE="http://www.google.com/mapfiles/";WA.prototype.zH=0;WA.prototype.CO=null;WA.prototype.b=function(){if(!this.W()){WA.f.b.call(this);Ez.ee(this);this.it();if(this.o)GEvent.clearInstanceListeners(this.o)}};
WA.prototype.ha=function(){this.b()};
WA.prototype.rL=function(a,b){this.da=a;this.yQ=b;this.it();this.BD(null);this.uL()};
WA.prototype.it=function(){while(this.fb.length){var a=this.fb.pop();this.o.removeOverlay(a);GEvent.clearInstanceListeners(a)}this.iL={};this.cq=null};
WA.prototype.reset=function(){this.it();this.Qw()};
WA.prototype.A=function(){return this.eL};
WA.prototype.Lo=function(a){this.Ge=a;if(this.o)if(a)this.tL();else{this.Qw();this.Qaa=false}};
WA.prototype.RD=function(a){if(a==this.zH)return;this.zH=a;if(this.o){if(this.fo){this.o.removeControl(this.fo);this.fo=null}if(a==1)this.fo=new GLargeMapControl;else if(a==2)this.fo=new GSmallMapControl;else if(a==3)this.fo=new GSmallZoomControl;if(this.fo)this.o.addControl(this.fo)}};
WA.prototype.SD=function(a){if(a==this.Eba)return;this.Eba=a;if(this.o){if(this.Qv){this.o.removeControl(this.Qv);this.o.removeMapType(G_PHYSICAL_MAP);this.Qv=null}if(a==1){this.Qv=new GHierarchicalMapTypeControl;this.o.addMapType(G_PHYSICAL_MAP)}if(this.Qv)this.o.addControl(this.Qv)}};
WA.prototype.cA=function(a){if(this.o)if(a&&!this.ww){this.ww=new GOverviewMapControl;this.o.addControl(this.ww)}else if(!a&&this.ww){this.o.removeControl(this.ww);this.ww=null}};
WA.prototype.BD=function(a){if(!a)this.cq=null;else if(this.fb.length>0)this.cq=this.fb[this.iL[a]];else this.CO=a};
WA.prototype.savePosition=function(a){if(this.o){var b=this.o.getBounds();if(a){var c=this.o.getCenter(),d=ND(a);this.o.setCenter(new GLatLng(d.lat,d.lon));this.o.savePosition();b=this.o.getBounds();this.o.setCenter(c)}else this.o.savePosition();return MD(b)}return null};
WA.prototype.Kr=function(){if(this.o)this.o.checkResize()};
WA.prototype.E3=function(){this.o=new GMap2(this.eL);this.o.enableContinuousZoom();this.o.enableScrollWheelZoom();this.o.checkResize();this.Maa=new GLatLngBounds;this.Sea=this.o.getBoundsZoomLevel(this.Maa);this.Qw();this.OS();this.uL()};
WA.prototype.pt=function(a,b,c){var d=new GMarker(a,c);d.data=b;d.dataId=b.id;GEvent.addListener(d,"dragend",l(this.oM,this,vE,"dragend",d));return d};
WA.prototype.wV=function(a,b){return this.pt(a,b,{clickable:false})};
WA.prototype.rH=function(a,b){var c=new GIcon;c.image=zE+"marker_mini.png";c.shadow=zE+"marker_mini_shadow.png";c.iconSize=new GSize(12,20);c.shadowSize=new GSize(22,20);c.iconAnchor=new GPoint(6,20);return this.pt(a,b,{clickable:false,icon:c})};
WA.prototype.gV=function(a,b){var c=String.fromCharCode("A".charCodeAt(0)+b.index),d=new GIcon(G_DEFAULT_ICON);d.image=zE+"marker"+c+".png";return this.pt(a,b,{clickable:false,icon:d})};
WA.prototype.$U=function(a,b){return this.pt(a,b,{draggable:true})};
WA.prototype.uL=function(){if(!this.o)return;this.p2();this.o.enableDoubleClickZoom();this.o.setMapType(G_PHYSICAL_MAP);if(this.da==1||this.da==0)this.t2();else if(this.da==3)this.u2();else if(this.da==2)this.tL();else if(this.da==4)this.r2();else if(this.da==5||this.da==6){this.o.disableDoubleClickZoom();this.s2()}this.o.savePosition()};
WA.prototype.s2=function(){this.ev(["photo","album"],l(this.$U,this))};
WA.prototype.r2=function(){this.ev(["geoResult"],l(this.gV,this))};
WA.prototype.t2=function(){this.ev(["album"],l(this.rH,this))};
WA.prototype.xY=function(a){var b=[];for(var c=0;c<a.length;c++){var d=this.yQ[a[c]+"s"];if(d)for(var e=0;e<d.length;e++){var f=d[e];f.storeType=a[c];b.push(f)}}return b};
WA.prototype.ev=function(a,b){var c=this.xY(a),d,e=false,f;for(var g=0;g<c.length;g++){var h=c[g],i=ND(h);if(i){var k=new GLatLng(i.lat,i.lon),n=b(k,h);this.fb.push(n);this.o.addOverlay(n);this.iL[n.dataId]=this.fb.length-1;if(this.CO)this.cq=n;if(!f||f.getPoint().lat()<i.lat)f=n;if(!d)d=new GLatLngBounds(k,k);OD(k,h.bb,d);if(h.bb)e=true}}var q,t=this.yQ.defaultView;if(this.fb.length==1)q=this.fb[0].getPoint();else if(this.fb.length==0)if(t){var k=ND(t);q=new GLatLng(k.lat,k.lon);d=new GLatLngBounds(q,
q);OD(q,t.bb,d);if(t.bb)e=true}else{this.Qw();return}if(!q)q=d.getCenter();var y=this.o.getBoundsZoomLevel(d);if(!e)y=Math.min(y,this.DJ(q)-2);this.o.setCenter(q,y);this.oX(f)};
WA.prototype.oX=function(a){if(a){var b=this.o.fromLatLngToDivPixel(a.getPoint()).y-1.5*a.getIcon().iconSize.height,c=this.o.fromLatLngToDivPixel(this.o.getBounds().getNorthEast());if(c.y>b)this.o.zoomOut()}};
WA.prototype.u2=function(){this.ev(["photo"],l(this.rH,this))};
WA.prototype.tL=function(){this.it();if(!this.Ge)return;var a=ND(this.Ge);if(a){this.o.checkResize();var b=new GLatLng(a.lat,a.lon),c=this.wV(b,this.Ge);this.fb.push(c);this.o.addOverlay(c);var d=this.o.getZoom(),e=this.DJ(b);if(this.Ge.bb)d=this.o.getBoundsZoomLevel(PD(this.Ge.bb));else if(!this.Qaa)d=e-2;this.Qaa=true;this.o.setZoom(d);this.o.panTo(b)}};
WA.prototype.DJ=function(a){return this.o.getBoundsZoomLevel(new GLatLngBounds(a,a))};
WA.prototype.Qw=function(){if(this.o)this.o.setCenter(this.Maa.getCenter(),this.Sea)};
WA.prototype.OS=function(){GEvent.bind(this.o,"click",this,this.Ov);GEvent.addDomListener(this.eL,se?"DOMMouseScroll":"mousewheel",l(this.o4,this))};
WA.prototype.p2=function(){switch(this.da){case 0:case 6:this.cA(true);this.RD(1);this.SD(1);break;case 1:case 2:case 3:this.cA(false);this.RD(3);this.SD(0);break;case 5:this.cA(false);this.RD(2);this.SD(1);break}};
WA.prototype.Q_=function(a,b){if(this.cq&&!a){this.cq.setPoint(b);this.oM(vE,"click",this.cq)}};
WA.prototype.Ov=function(a,b){if(this.da==5||this.da==6)this.Q_(a,b)};
WA.prototype.o4=function(a){if(!a)a=window.event;if(!a.preventDefault)a.returnValue=false;else a.preventDefault()};
WA.prototype.oM=function(a,b,c){this.dispatchEvent(new AE(a,b,this,c))};
function AE(a,b,c,d){D.call(this,a);this.mapEventType=b;this.marker=d;this.map=c}
o(AE,D);if(s)try{document.execCommand("BackgroundImageCache",false,true)}catch(BE){}var CE=function(){if(_features.geotagging){var a=u("lhid_map");if(a)new WA(a,1,{albums:_user.albums})}r(_user.albums,function(b){var c=u("title_"+b.id),d=_features.googlephotos?24:32,e=ob(b.title,d,true);c.innerHTML=Ng(e);if(e.length<b.title.length)c.title=b.title});
VA();Y("index",Ia(uz,AC))},
DE=function(a,b){if(ib(ye,"Win"))if(_picasaVersion>=3){var c=u("lhid_launchGplFooter");if(c)M(c,true)}else if(_picasaVersion>0)M(u("lhid_launchpicasafooter"),true);else M(u("lhid_downloadpicasafooter"),true);else if(ib(ye,"Mac")){var d=u("lhid_downloadpicasafooter");if(d){var e=Me("a",null,d);if(e.length)e[0].href=b}}else if(ib(ye,"Linux"))M(u("lhid_downloadpicasafooter"),false)},
EE=function(a,b,c){var d=u(a),e=u(b),f=u(c);if(d)M(d,_picasaVersion==0);if(e)M(e,_picasaVersion>0&&_picasaVersion<3);if(f)M(f,_picasaVersion>=3)};
var FE=function(){if(_album.hasGeotaggedPhotos||_authuser.isOwner)new sE(window.location.hash)},
GE=[],HE=function(){GE.push(Ia.apply(null,arguments))},
IE=function(){while(GE.length)GE.pop()()};
(function(){var a=[{compatible:Wz,setup:HE,start:IE,albumList:CE,footer:DE,mail:GC,uploadForm:LD,mapLoaded:Pz,mapSetup:Qz,cart:VA,loggerSetup:Vz,setupSettingsPage:aD,client:EE}];a.push({albummap:FE});r(a,function(b){Gc(b,function(c,d){La(Yr+d,c)})});
ws=_authuser||ws;xs=ux})();
var Ez=new cz,vz=new Dz(1),bD=null,rA=zb(hn,"7")>=0;function RA(a){if(bD)bD.hide();if(a)bD=null}
(function(){window._d=jz;function a(){var e=u("lhid_settingsform"),f=u("lhid_personaurl");if(!e||!f)return;var g=e.urlnick;if(!g)return;if(g.length){for(var h=0;h<g.length;h++)if(g[h].checked){B(f,g[h].value);break}}else B(f,g.value)}
function b(){var e=u("lhid_email_upload_enabled"),f=u("lhid_email_upload_additional_settings"),g=u("lhid_email_upload_secret_word"),h=u("lhid_email_upload_address_box"),i=u("lhid_personaurl");if(!e||!f)return;M(f,e.checked);if(!g||!h||!i)return;if(g.value==""){var k=m("Enter a secret word above to create your address");B(h,k);L(h,"color","red")}else if(g.value!=lf(h)){h.innerHTML=lf(i)+".<b>"+p(g.value)+"</b>@picasaweb.com";L(h,"color","black")}}
window._loadDateTimeConstants=function(){gi(_LH.locale);if(!mi(ii,hi()))ni(ri,hi());window._fd=function(e){return mz(e,2)};
window._pd=function(e,f,g){nz(e,f,g,2)};
window._loadDateTimeConstants=function(){}};
window._fd=function(e){window._loadDateTimeConstants();return window._fd(e)};
window._pd=function(e,f,g){window._loadDateTimeConstants();window._pd(e,f,g)};
window._ps=function(e){bB.prototype.ha=bB.prototype.b;var f=e.foreign;if(pa(f))f.name=ux(f.name);window.LH_SEARCH_CONTEXT_INFO=e;Ez.Hd(new bB(e))};
window._st=function(){Y("updateGalleryUrl",a);Y("updateEmailUploadSettings",b);a();b();var e=function(f){M(u("lhid_publicalbumindexing_set"),true);M(u("lhid_publicalbumindexing_unset"),false);u("lhid_publicalbumindexing").checked=f};
Y("index",Ia(uz,e))};
window._sl=function(){Y("index",Ia(uz,Ia(null,document.referrer)))};
window._h=function(e,f){if(_features.geotagging){var g=u("lhid_map");if(g)new WA(g,1,{albums:_user.albums})}if(_user.albums.length>0)new uC(_user.albums,u("lhid_albumsort"),u("lhid_albums"),e,f);VA();Y("index",Ia(uz,AC))};
window._b=function(){new rD(_thumbnail,_updateCaptionsPath,u("lhid_captions"))};
window._f=function(e,f,g,h){var e=new lD(e,f,g,h)};
window._vc=function(){window._selectProvider=NA;window._sendToProvider=KA;window._removeCart=JA;new IA};
window._sp=function(e,f){if(Qi(LA)){Ri(LA);e.submit()}else window.location=f};
window._onload=function(){oz()};
Y("closePromo",function(){u("lhid_promoWidget").style.display="none";vz.qc(_setPrefUrl,null,"pref=HIDE_PROMO")});
Y("closeBloggerWelcome",function(){u("lhid_bloggerWidget").style.display="none";vz.qc(_setPrefUrl,null,"pref=HIDE_BLOGGER_WIDGET")});
Y("ProfilePicker",function(){if(bD==null)bD=new Et(_selectedPhotosPath,_user.name,_album.id,"PORTRAIT");bD.rx("PORTRAIT",false);jz("hideVideo");bD.show(function(){jz("restoreVideo")})});
Y("SettingsPicker",function(){if(bD==null)bD=new Et(_selectedPhotosPath,_user.name,_album.id,"PORTRAIT");bD.rx("PORTRAIT",true);bD.show()});
Y("CoverPicker",function(){if(bD==null)bD=new Et(_selectedPhotosPath,_user.name,_album.id,"BANNER");bD.rx("BANNER",false);bD.show()});
window._reorganize=function(e){var f=ze?"lhid_helpSelectMac":"lhid_helpSelect";M(u(f),true);var g=u("lhid_edit_frame"),h=w("div");h.className="lhcl_lighttable";g.appendChild(h);var i=[u("lhid_copy"),u("lhid_move"),u("lhid_delete")],k=new tC(h,u("lhid_done"),i);for(var n=0;n<e.length;n++){var q=e[n];k.VS(q.l,q.id,q.s,q.t,q.d,q.v)}if(_album.pageInterval>0){var t=[];if(_album.offset>0)t.push('<a href="',_album.pageLink,'&off=0">&laquo; 1</a>&nbsp;&nbsp;');if(_album.offset>_album.pageInterval){var y=
_album.offset-_album.pageInterval,z=y+1;t.push('<a href="',_album.pageLink,"&off=",y,'">&lsaquo; ',z,"</a>&nbsp;&nbsp;")}t.push(ox(_album.offset+1,_album.offset+e.length,_album.photocount),"&nbsp;&nbsp;");if(_album.offset<_album.photocount-2*_album.pageInterval){var O=_album.offset+_album.pageInterval,z=O+1;t.push('<a href="',_album.pageLink,"&off=",O,'">',z," &rsaquo</a>&nbsp;&nbsp;")}if(_album.offset<_album.photocount-_album.pageInterval){var aa=(Math.ceil(_album.photocount/_album.pageInterval)-
1)*_album.pageInterval,z=aa+1;t.push('<a href="',_album.pageLink,"&off=",aa,'">',z," &raquo;</a>&nbsp;&nbsp;")}u("lhid_pager").innerHTML=t.join("")}yz()};
window._hideWidget=function(e,f){M(u(e),false);if(vz&&vz.Gs())vz.qc(f,null,"POST")};
window._ef=wB;window._setVisibility=Jy;window._addFavorite=gD;window._v=HC;window._setUpdateCartAction=MA;window._setBackLink=tz;var c=["BR_IsMac",function(){return ib(ye,"Mac")},
"BR_IsWin",function(){return ib(ye,"Win")},
"SetCookie",Oi,"get",u,"LH_Dialog",cB,"DIALOG_CREATE",0,"DIALOG_UPLOAD_FIRST",2,"DIALOG_UPLOAD",1,"DIALOG_EDIT",5,"DIALOG_COPY",3,"DIALOG_MOVE",4,"LH_CreateActiveXUploader",qD,"FlashRequest",Ia(jz,"f")];if(typeof wn=="function"){c.push("CL_callInsertNotes");c.push(wn)}for(var d=0;d<c.length;d+=2)La(c[d],c[d+1]);La("_xmlhttp",vz);La("xmlhttp",vz);Ma(vz,"Supported",vz.Gs);Ma(vz,"Request",vz.qc)})();var JE=function(a){P.call(this);this.Sc=a||""};
JE.prototype.O=function(a){return this.Sc+(a?"-"+a:"")};
JE.prototype.qb=function(a,b){var c=b||this.A(),d=this.H()+"-"+a,e=u(d);if(!e){var f=Me(null,this.O(a),c);if(f.length){e=f[0];e.id=d}else e=null}return e};var LE=function(a,b){pm.call(this,"",new KE(b||"lhcl_Checkbox"))};
o(LE,pm);var ME,NE;LE.prototype.ML=0;LE.prototype.Vb=function(a,b){if(this.wf(a)&&b!=this.Xf(a)){this.Bb=b?this.Bb|a:this.Bb&~a;this.pa.Vb(this,a,b)}};
var KE=function(a){Nk.call(this);if(!OE){OE=true;Uc(KE.prototype,JE.prototype)}JE.call(this,a);this.Sc=a},
OE;o(KE,Nk);KE.prototype.kb=function(){return false};
KE.prototype.m=function(){return v("div",{"class":this.Sc})};
KE.prototype.Bg=function(a,b){switch(a||0){case 4096:return b&&this.O("checkedActive_")||null;case ME:case NE:if(!b)return null;default:return KE.f.Bg.call(this,a)}};
KE.prototype.Vc=function(){return null};
KE.prototype.hc=function(){};
KE.prototype.Vb=function(a,b,c){var d=a.A();if(b==4||b==16){qf(d,this.Bg(this.ML,true));var b=null;if(a.Xf(4))b=a.Xf(16)?4096:4;else if(a.Xf(16))b=16;this.ML=b;C(d,this.Bg(b,true))}else KE.f.Vb.call(this,a,b,c)};var QE=function(a){P.call(this);if(!PE){PE=true;Uc(QE.prototype,JE.prototype)}JE.call(this,a);this.p=new K(this);this.Lf={};this.Va=[]},
PE;o(QE,P);var RE=new RegExp("\\{\\$prefix\\}","g");QE.prototype.yg=function(a){return Ve(a.replace(RE,this.O()))};
QE.prototype.b=function(){if(this.W())return;QE.f.b.call(this);this.p.b()};
QE.prototype.Jh=function(a){while(this.Va.length)this.removeChild(this.Va[0],a)};var SE=function(a,b,c,d,e){QE.call(this,e||"lhcl_ScaledImage");this.kL=a;this.Bd=d||-1;this.yf=c||-1;this.Yw=b||1};
o(SE,QE);SE.prototype.Fj=null;SE.prototype.sa=null;SE.prototype.rf=false;SE.prototype.Jg=null;SE.prototype.Ue="standard";SE.prototype.m=function(){SE.f.m.call(this);C(this.e,this.O());this.lx(this.kL)};
SE.prototype.mu=function(){return this.sa};
SE.prototype.isLoaded=function(){return this.rf};
SE.prototype.lx=function(a){this.kL=a;this.sa=w("img");this.rf=false;$f(this.sa,"load",this.e0,false,this);this.sa.src=this.kL};
SE.prototype.ce=function(a){this.Ue=a};
SE.prototype.wP=function(a,b,c){this.Yw=a||this.Yw;this.yf=b||this.yf;this.Bd=c||this.Bd;this.If()};
SE.prototype.Hp=function(a){var b=new ce(a.top,a.width+a.left,a.height+a.top,a.left),c="rect("+Math.round(b.top)+"px "+Math.round(b.right)+"px "+Math.round(b.bottom)+"px "+Math.round(b.left)+"px)";L(this.sa,"clip",c);var d=Math.round(-b.top)+"px 0 0 "+Math.round(-b.left)+"px";L(this.sa,"margin",d);L(this.sa,"position","absolute")};
SE.prototype.e0=function(){this.rf=true;this.Jg=new ne(this.sa.width,this.sa.height);this.wP();var a=this.A();We(a);x(a,this.sa);this.dispatchEvent("load")};
SE.prototype.If=function(){if(!this.rf)return;if(this.Ue=="standard"){var a=this.Yw;if(this.yf>=0&&this.Jg.width*a>this.yf)a=this.yf/this.Jg.width;if(this.Bd>=0&&this.Jg.height*a>this.Bd)a=this.Bd/this.Jg.height;zh(this.sa,this.Jg.width*a,this.Jg.height*a)}else if(this.Ue=="zoomandcrop"){var a;a=this.Fj?Math.min(this.Yw,Math.min(this.yf/this.Fj.width,this.Bd/this.Fj.height)):Math.min(this.Yw,Math.max(this.yf/this.Jg.width,this.Bd/this.Jg.height));var b=this.Jg.width*a,c=this.Jg.height*a;zh(this.sa,
b,c);var d=new ke(0,0,b,c);if(this.Fj){d.left=this.Fj.left*a-(this.yf-this.Fj.width*a)/2;d.width=this.yf;d.top=this.Fj.top*a-(this.Bd-this.Fj.height*a)/2;d.height=this.Bd}else{if(b>this.yf){var e=b-this.yf;d.left=e/2;d.width-=e}if(c>this.Bd){var e=c-this.Bd;d.top=e/2;d.height-=e}}this.Hp(d)}};var TE=function(a){QE.call(this,a||"lhcl_PhotoOverlay");this.JG=new Ym(this.yb,250,this);this.M1=new Ym(this.a0,3000,this);this.Tb={}};
o(TE,QE);var UE="lowerThird_";TE.prototype.az=0;TE.prototype.xE=false;var VE=[UE,"middleThird_","upperThird_"];TE.prototype.m=function(){this.od(this.yg('<div class="{$prefix}"><div class="lhcl_RelativeContainer lhcl_FillVertical"><div class="{$prefix}-background_"></div><div class="{$prefix}-above_"></div><div class="{$prefix}-photo_"></div><div class="{$prefix}-below_"></div><div class="{$prefix}-upperThird_"></div><div class="{$prefix}-middleThird_"></div><div class="{$prefix}-lowerThird_"></div></div></div>'))};
TE.prototype.b=function(){if(this.W())return;TE.f.b.call(this);this.JG.b();this.M1.b()};
TE.prototype.n=function(){TE.f.n.call(this);if(this.s5)this.rP(this.s5);Gc(this.Tb,function(a,b){this.oP(b,a)},
this);this.p.g(this.A(),fg,this.Uj);if(s&&!Ee("7"))this.p.g(window,"scroll",this.C_)};
TE.prototype.C_=function(){if(th().scrollTop>0){var a=th().scrollTop+"px",b=this.A();L(b,"top",a)}};
TE.prototype.NJ=function(){return this.sa.mu()};
TE.prototype.a8=function(a){this.az=a};
TE.prototype.oP=function(a,b){this.Tb[a]=b;if(!this.Ha())return;var c=this.qb(a);We(c);if(!b)return;x(c,b);if(!this.xE&&(a==UE||a=="middleThird_"||a=="upperThird_"))M(b,false);else if(a=="above_"||a=="below_")this.Ch=null};
TE.prototype.rP=function(a){this.s5=a;if(!this.Ha())return;if(this.sa){this.removeChild(this.sa,true);this.sa=null}this.sa=new SE(a);this.Z(this.sa);var b=this.qb("photo_");this.sa.C(b);this.JG.start()};
TE.prototype.CZ=function(){var a=this.A();if(!a)return new ne(0,0);return new ne(a.offsetWidth,a.offsetHeight)};
TE.prototype.a0=function(){this.zP(false)};
TE.prototype.Uj=function(){this.zP(true)};
TE.prototype.yb=function(){var a=this.CZ();if(!oe(this.Ch,a)){this.Ch=a;if(s&&!Ee("7")){var b=document.documentElement.clientHeight+"px",c=this.A();L(c,"height",b);var d=this.qb("background_");L(d,"height",b)}var e=this.Tb.Wea,f=e&&e.offsetHeight||0,g=this.Tb.bfa,h=g&&g.offsetHeight||0;this.sa.wP(1,a.width-this.az*2,a.height-f-h-this.az*2);var i=this.sa.A(),k=i.offsetHeight;L(i,"visibility",k!=0?"visible":"hidden");if(k==0){this.Ch=null;Rg(this.yb,0,this)}var n=k+f+h,q=(a.height-n)/2;if(e){var t=
this.qb("above_");sh(t,0,q)}var y=this.qb("photo_");sh(y,0,q+f);if(g){var z=this.qb("below_");sh(z,0,q+f+k)}}this.JG.start()};
TE.prototype.zP=function(a){if(this.xE!=a)r(VE,function(b){var c=this.Tb[b];if(c)M(c,a)},
this);this.xE=a;if(a)this.M1.start()};var WE=function(a,b){ut.call(this);this.Q=a;this.Q.Y8(true);if(b)this.nP(b)};
o(WE,ut);WE.prototype.Ba=Fd("gphoto.FeedModel");WE.prototype.nP=function(a){if(this.X)G(this.X,QB,this.rj,false,this);this.X=a;F(this.X,QB,this.rj,false,this)};
WE.prototype.S=function(){if(!this.Q.isLoaded())return-1;return this.Q.Yb()};
WE.prototype.wb=function(a){return this.Q.Me(a)};
WE.prototype.remove=function(a){var b=this.wb(a);this.Q.remove(a);return b};
WE.prototype.Eo=function(a,b,c){this.Q.ua(a,b-a,function(d,e,f){c(e,d.vA().slice(e,e+f+1))})};
WE.prototype.ae=function(a){this.Q=a};
WE.prototype.Wa=function(){return this.Q};
WE.prototype.isLoaded=function(){return this.Q.isLoaded()};
WE.prototype.rj=function(){this.dispatchEvent("update")};var YE=function(a,b,c,d){st.call(this,a,b,c,d);this.lea=XE;this.Q=a.na();this.LM=false};
o(YE,st);var ZE={marginHorizontalPx:0,marginVerticalPx:0,paddingHorizontalPx:10,paddingVerticalPx:8,widthEm:1,heightEm:1.2},$E=Fd("gphoto.BasicEntityIcon");YE.prototype.zk=false;YE.prototype.columns=4;var XE=false,aF=false,bF=3;YE.prototype.Vp=-1;YE.prototype.V=function(a){$E.Zb(a)};
YE.prototype.b=function(){if(!this.W()){YE.f.b.call(this);if(cA&&_features.fr&&this.Tc)this.Tc.b()}};
YE.prototype.Rh=function(a){var b=bF==a;bF=a;return b};
YE.prototype.m=function(){this.od(v("div",{"class":this.db+" "+this.db+"-loading"}));Eh(this.e);var a=v("div",{"class":this.db+"-content"});this.$u=v("div",{"class":this.db+"-photo"});this.I=v("img");this.ek=v("div",{"class":this.db+"-meta"});x(this.e,a);x(this.e,this.ek);x(a,this.$u);x(this.$u,this.I);if(this.lea){this.Kg=v("div",{"class":this.db+"-photo-info"});x(a,this.Kg)}this.cv()};
YE.prototype.n=function(){this.Nh(true);if(!this.k)return;else this.aa();if(cA&&_features.fr&&this.Tc)this.Tc.n()};
YE.prototype.u=function(){if(this.e)cg(this.e);if(cA&&_features.fr&&this.Tc)this.Tc.u()};
YE.prototype.JD=function(a){this.Vp=a;if(this.Kg&&aF){var b="("+this.Vp+") ";if(this.k)try{b=b+this.k.Td()}catch(c){}B(this.Kg,b)}};
YE.prototype.hJ=function(){return this.Vp};
YE.prototype.Nh=function(a){if(!this.e)return;var b=this.db+"-loading";tf(this.e,b,a);if(a){M(this.e,true);if(!this.cw){this.cw=v("div",{"class":this.db+"-msg"});x(this.e,this.cw)}this.cw.innerHTML=Vx;M(this.I,false)}else{if(this.cw)M(this.cw,false);M(this.I,true)}};
YE.prototype.mP=function(a){var b=this.LM!=a;this.LM=a;if(b)this.aa()};
YE.prototype.clear=function(){if(this.I)this.I.src="img/transparent.gif"};
YE.prototype.U=function(a,b){if(a==Rs)a=null;if(this.k!=a)this.k=a;if(this.k)this.Nh(false);this.aa();if(b)this.Ye()};
YE.prototype.aa=function(){if(!this.k||this.k==Rs)return;this.Dea=bF;var a=this.k.thumbnail(this.Dea);if(a){var b=this.k.isLoaded()?a.url:"";if(!b)this.V("no thumbnail for entity: "+this.k.id+" at index "+bF);if(this.I&&b!=this.I.src){var c=1,d=1,e=this.k.Jn()||1;if(e>=1)d=1/e;else c=e;A(this.I);zh(this.I,c+"em",d+"em");this.I.src=this.LM?"img/transparent.gif":b;x(this.$u,this.I);this.Nh(false)}this.qs();if(this.Kg){var f=this.k.Td();if(aF)f="("+this.Vp+") "+f;B(this.Kg,f)}}};
YE.prototype.qs=function(){};
YE.prototype.Ye=function(){if(cA&&_features.fr&&this.Tc)this.Tc.Ye()};
YE.prototype.show=function(a){if(this.e)M(this.e,a)};var cF=function(a,b,c,d){YE.call(this,a,b,c,d)};
o(cF,YE);cF.prototype.m=function(){cF.f.m.call(this);this.pb=v("p");x(this.ek,this.pb)};
cF.prototype.qs=function(){var a=this.k?this.k.c:"";if(this.pb&&mf(this.pb)!=a){if(this.W1)H(this.W1);if(this.V1)H(this.V1);if(!a||Qa(a))B(this.pb,"");else{this.pb.innerHTML=Ng(ob(a,90));this.W1=F(this.e,Gf,this.rI,false,this);this.V1=F(this.e,Hf,this.rI,false,this)}}};
cF.prototype.rI=function(){if(this.e&&this.ek){var a=ph(this.e,"z-index")==1000?1:1000;L(this.e,"z-index",a);uf(this.ek,"goog-icon-list-icon-meta-hover")}};var dF=function(a,b,c,d){YE.call(this,a,b,c,d)};
o(dF,YE);var eF={marginHorizontalPx:36,marginVerticalPx:6,paddingHorizontalPx:22,paddingVerticalPx:8,widthEm:1,heightEm:1.6};dF.prototype.m=function(){dF.f.m.call(this);this.fT=v("div");this.KT=v("div",{"class":this.db+"-meta-gallerylink"});this.Y9=v("div",{"class":this.db+"-meta-snippet"});this.X$=v("div",{"class":this.db+"-meta-albumlink"});x(this.ek,this.fT);x(this.ek,this.KT);x(this.ek,this.Y9);x(this.ek,this.X$)};
dF.prototype.cv=function(a,b){F(a||this.$u,I,this.xj,false,this);F(b||this.$u,I,this.RH,false,this)};
dF.prototype.qs=function(){if(this.k&&this.ek){var a=pb(this.k.gd().title,24);B(this.fT,a);var b=this.k.user,c=pb(b.nickname,24),d="/"+b.name;this.KT.innerHTML=ez('<span>{bylinetext} </span> <a href="{gallerylink}">{gallerynickname}</a>',{bylinetext:fF,gallerylink:d,gallerynickname:p(c)});this.Y9.innerHTML=this.k.snippet;var e=this.k.gd().link;this.X$.innerHTML=this.k.truncated?ez('<a href="{albumLink}"> [{moremessage}]</a>',{albumLink:e,moremessage:Iu}):""}};
var fF=m("By");var gF=function(a,b,c){this.Se=a;this.pM=b;this.Fq=c;this.dP(zs);this.p=new K(this)};
gF.prototype.je=-1;gF.prototype.ca=-1;gF.prototype.Ys=true;gF.prototype.lz=144;gF.prototype.bt=144;gF.prototype.ny=-1;gF.prototype.Cv=-1;gF.prototype.oA=0;gF.prototype.bF=0;gF.prototype.Ba=Fd("gphoto.ListLayout");gF.prototype.V=function(a){this.Ba.Zb(a)};
gF.prototype.n=function(){if(this.p)this.p.ma();this.p.g(this.Se.Ia().Ia(),mg,this.o1);this.p.g(window,"scroll",this.JK)};
gF.prototype.u=function(){this.p.ma()};
gF.prototype.b8=function(a,b){this.lz=a;this.bt=b;this.V("new cellWidth_: "+this.lz+", new cellHeight_: "+this.bt);this.Ys=true};
gF.prototype.dP=function(a){this.V("SetIconSize: "+a);var b=this.Fq.widthEm*a+this.Fq.marginHorizontalPx+this.Fq.paddingHorizontalPx,c=this.Fq.heightEm*a+this.Fq.marginVerticalPx+this.Fq.paddingVerticalPx;this.b8(b,c)};
gF.prototype.JK=function(){this.Zs()};
gF.prototype.o1=function(){this.Zs()};
gF.prototype.Zs=function(){var a=Qe();if(!this.ZB)this.ZB=this.Se.A();var b=wh(this.ZB),c=Bh(this.ZB),d=Ah(this.ZB);if(d.height==0){this.ny=a.height-(b.y>0?b.y:0);this.Cv=a.width-(b.x>0?2*b.x:0)}else{this.ny=b.y>0?a.height-b.y:(d.height+b.y<a.height?b.y+d.height:a.height);this.Cv=d.width}this.Ys=true;this.RT(c,b,a);this.Wd=true;this.V("visibleListHeight: "+this.ny+", listWidth: "+this.Cv)};
gF.prototype.RT=function(a,b){var c=a.top-(a.top+b.y);c=c<0?0:c;var d=Math.floor(c/this.bt),e=this.ju();this.oA=d*e.columns;this.bF=this.cF*this.Iaa;this.V("firstVisibleItem: "+this.oA+", visibleItemCount: "+this.bF)};
gF.prototype.Yc=function(){var a=this.ju();return a.columns*a.rows};
gF.prototype.Wd=false;gF.prototype.NN=function(){this.Zs();return this.ju()};
gF.prototype.ju=function(){if(!this.Wd){this.Wd=true;this.Zs()}if(this.Ys){var a=this.Cv/this.lz;a=a>1?a:1;this.je=Math.floor(a);this.Iaa=this.je;var b=this.ny/this.bt;b=b>1?b:1;this.ca=Math.ceil(b);this.cF=this.ca;if(this.pM==-1){var c=this.Se.na();if(c){var d=c.S();if(d>0)this.ca=Math.ceil(d/this.je)}}else this.ca=Math.ceil(this.pM/this.je);this.je=pe(this.je,1,this.je);this.ca=pe(this.ca,1,this.ca);this.cF=Math.min(this.ca,this.cF);this.Ys=false}this.V("columns: "+this.je+", rows: "+this.ca+", visibleColumns: "+
this.Iaa+", visibleRows: "+this.cF);return{columns:this.je,rows:this.ca}};
gF.prototype.iK=function(){return this.bF};
gF.prototype.oJ=function(){return Math.max(0,this.oA)};
gF.prototype.pv=function(){return this.pM!=-1};var hF=function(a,b){b.iconClass=b.iconClass||YE;if(b.debug){b.iconClass.showInfo=true;b.iconClass.debug=true}W.call(this,b.iconClass,b.listCssClass,b.iconCssClass);if(!this.Pe||!this.Pe.prototype.Rh){this.V("replacing iconClass");this.aP(YE)}this.eb=b;this.X=a;this.p=new K(this);this.Ug(1);this.gI(b.keyboardShortcuts);this.Vda=new Ym(this.KQ,100,this);this.KV=new Ym(this.LE,100,this);this.yb=new gF(this,b.maxPageSize,b.iconCssAttributes);this.Dh=true;this.nX=true};
o(hF,W);hF.prototype.ml=-1;hF.prototype.Ba=Fd("gphoto.BasicFeedList");hF.prototype.Yu=128;hF.prototype.la=true;hF.prototype.V=function(a){this.Ba.Zb(a)};
hF.prototype.show=function(a){this.V("show: "+a);if(this.la!=a){this.la=a;if(this.e)M(this.e,a);this.ei(a);if(a)this.nb()}};
hF.prototype.Ja=function(){return this.la};
hF.prototype.ei=function(a){var b=a?this.p.g:this.p.Sa;b.call(this.p,this.Ia().Ia(),mg,this.nb);b.call(this.p,window,"scroll",this.jW);if(this.yb.pv())b.call(this.p,this.X,QB,this.K0);a?this.yb.n():this.yb.u()};
hF.prototype.n=function(){W.prototype.n.call(this);this.yb.n();this.ei(this.Ja())};
hF.prototype.m=function(){hF.f.m.call(this);L(this.e,"fontSize",this.Yu+"px");this.oH();this.j.g(this,rt,this.gB)};
hF.prototype.u=function(){this.p.ma();this.yb.u();hF.f.u.call(this)};
hF.prototype.jW=function(){this.tO=true;this.KV.start()};
hF.prototype.nb=function(){this.Dh=true;this.tO=true;if(this.yb.pv())this.yb.NN();this.Vda.start()};
hF.prototype.Ur=function(a){this.Dh=true;this.Pe.prototype.Rh(a);this.Yu=ys[a];if(this.e)this.e.style.fontSize=this.Yu+"px";this.yb.dP(this.Yu);this.nb()};
hF.prototype.c5=function(a){if(a!=this.k.Wa()){this.V("mismatch feed load");return}var b=a.Yb(),c=this.X.Kj();if(c!=b)this.Dh=true;if(this.nX){this.yb.NN();this.KQ();this.nX=false}};
hF.prototype.U=function(a){W.prototype.U.call(this,a);this.p.g(this.k.Wa(),Cs,this.LE);if(this.la)this.Nu()};
hF.prototype.Nu=function(){this.V("handleUpdate_");this.yb.ju();this.X.Wi(this.yb.Yc());var a=this.X.hd(),b=this.X.Vf(a),c=this.X.Yc();this.ml=b;this.k.Wa().ua(b,c,l(this.c5,this))};
hF.prototype.KQ=function(){this.V("updateLayout_: "+this.Dh+", scrollDirty: "+this.tO);if(!this.la)return;if(this.tO)this.yb.JK();if(!this.Dh)return;var a=this.yb.oJ(),b=this.yb.iK(),c=this.yb.pv(),d,e,f=-1,g=this.k?this.k.Wa().Yb():0;if(c){d=Math.min(this.yb.Yc(),g);e=this.Ag();if(e>d)f=d;this.startIndex=0;this.visibleItemCount=d}else d=g;this.X.Wi(this.yb.Yc());var h=this.X.hd(),i=this.X.Vf(h);this.ml=i;if(c&&d+this.ml>g)f=g-this.ml;for(var k=a;k<d;++k){var n=this.wd(k),q=i+k;if(!n){this.Op(k,this.nc,
k>=b);n=this.wd(k);if(!n)continue}if(n.hJ()!=q)n.JD(q);if(c)n.show(q<g);else M(n.e,true)}if(c)for(var t=f;t>=0&&t<e;++t){var y=this.wd(t);y.show(false)}this.Dh=false;this.KV.start()};
hF.prototype.K0=function(){if(this.k){var a=this.X.hd();this.ml=this.X.Vf(a);this.nU();this.LE()}};
hF.prototype.nU=function(){var a=this.yb.Yc();for(var b=0;b<a;b++){var c=this.wd(b);if(c)c.clear()}};
hF.prototype.LE=function(a){if(!this.k)return;var b=this.yb.oJ(),c=-1,d=this.k.Wa().Yb(),e=this.yb.pv(),f=this.yb.iK();if(e){var g=this.X.hd();c=this.ml>=0?this.ml:this.X.Vf(g);b=0;f=Math.min(this.yb.Yc(),d)}var h=this.k.Wa(),i=b+f;for(var k=b;k<i;++k){var n=this.wd(k);if(!n)continue;var q;if(c>=0){q=c+k;if(q<d){n.JD(q);n.show(true)}else{n.JD(-1);n.show(false)}}else q=n?n.hJ():-1;if(q>=0){var t=this.k.wb(q);if(!t&&t!=Rs)h.Sj(q);n.U(t,a);n.mP(false)}}};
hF.prototype.Op=function(a,b,c){var d;if(this.k)d=this.k.wb(a);var e=new this.Pe(this,this.Xu,a,d);e.mP(!(!c));this.Z(e);e.C(b);if(this.le)this.le.ub(e.A(),e)};var jF=function(a){D.call(this,iF,a);this.entity=a};
o(jF,D);var iF="entitychanged";var kF=function(a,b){this.element=a;this.headerElement=b};
kF.prototype.u=function(){if(this.element)A(this.element)};
var lF=function(a){P.call(this);this.$e=a;this.p=new K(this);this.uO={}};
o(lF,P);lF.prototype.me=function(){return this.$e.me()};
lF.prototype.n=function(){lF.f.n.call(this);this.p.g(this.$e,iF,this.qj)};
lF.prototype.u=function(){this.p.ma();Gc(this.uO,function(a){a.u()});
lF.f.u.call(this)};
lF.prototype.b=function(){if(!this.W())this.p.b()};
lF.prototype.qj=function(a){var b=a.entity;this.U(b)};
lF.prototype.m=function(){this.e=v("div",{"class":"lhcl_toolbox",style:"display: none;"})};
lF.prototype.pq=function(a){var b=this.uO[a];if(!b){var c=v("div",{"class":"lhcl_tools lhcl_hideoverflow",style:"position: relative"}),d=w("table"),e=w("tbody"),f=w("tr");x(this.e,c);x(c,d);x(d,e);x(e,f);b=new kF(c,f);this.uO[a]=b}return b};
lF.prototype.Xi=function(a,b,c){var d=this.pq(a),e;if(c){e=w("a");e.href=c;e.title=b;B(e,b)}if(!d.titleElement){d.titleElement=v("td",{"class":"lhcl_detailtitle"});x(d.headerElement,d.titleElement)}if(e){We(d.titleElement);x(d.titleElement,e)}else B(d.titleElement,b)};
lF.prototype.show=function(a){if(this.e)M(this.e,a)};{var mF=function(a,b,c,d,e,f,g,h,i,k,n,q,t,y,z){P.call(this);this.WF=a;this.Cr=b;this.rp=c;this.sea=d;this.Dca=e;this.Yca=f;this.Gca=g;this.FT=h;this.Sba=i;this.Dda=lb(k);this.jca=n;this.Yaa=y;this.t3=q;this.N=t;this.ud=z;this.p=new K(this)};
o(mF,P);mF.prototype.u=function(){mF.f.u.call(this);if(this.p)this.p.ma()};
mF.prototype.show=function(a){if(this.e&&this.rp)M(this.e,a)};
mF.prototype.N9=function(){new oB};
mF.prototype.lx=function(a){this.WF=a;if(this.Ha())this.sa.src=this.WF};
mF.prototype.m=function(){mF.f.m.call(this);var a={"class":"lhcl_toolbox"};if(!_album||!this.rp)a.style="display: none;";else a.id="lhid_albumprop";this.e=v("div",a);this.ot();if(this.Cr>0)this.QU();this.UU();this.fV();if(this.Cr>0)this.uV()};
mF.prototype.n=function(){mF.f.n.call(this);if(this.cI)this.p.g(this.cI,I,this.N9);if(this.ud)this.p.g(this.sa,E,this.ud)};
mF.prototype.ot=function(){var a=v("table",{"class":"gphoto-sidebar-subitem"}),b=w("tbody"),c=w("tr"),d=w("td");this.sa=v("img",{src:this.WF,width:"56",height:"56"});var e=v("td",{width:"100%",valign:"top",align:"left"}),f=this.Dca?$w:ax,g=this.t3?Xy(this.t3)+"<br />":"",h=m("{$numPhotos} photos",{numPhotos:this.Cr});e.innerHTML=this.N+"<br />"+g+h+" - "+this.sea+" - "+f;x(d,this.sa);x(c,d);x(c,e);x(b,c);x(a,b);x(this.e,a);var i=v("div",{"class":"gphoto-sidebar-subitem"});i.innerHTML=this.Yaa;x(this.e,
i)};
mF.prototype.QU=function(){if(_authuser&&_authuser.isOwner){var a=m("Link to this album"),b=v("div",{"class":"gphoto-sidebar-subitem"}),c=v("div",{style:"margin-bottom=3px; -moz-user-select: none"}),d=v("img",{src:"img/transparent.gif","class":"SPRITE_link lhcl_spriting_alignMiddle lhcl_spriting_marginRight5"}),e=v("span",{"class":"lhcl_spriting_alignMiddle"},a),f=v("img",{src:"img/transparent.gif"});x(c,d);x(c,e);x(c,f);var g=m("Paste Link in email or IM"),h=v("div",null,g),i=v("input",{type:"text",
value:window.location.href.toString(),onclick:"this.select()","class":"gphoto-sidebar-subitem"});x(b,h);x(b,i);if(_features.newStrings){var k=m("Embed Slideshow");this.cI=v("div",null,v("img",{src:"img/transparent.gif","class":"SPRITE_embed"})," "+k);x(b,this.cI)}x(this.e,c);x(this.e,b);new Tp(c,b)}};
mF.prototype.uV=function(){var a=m("RSS");if(this.Sba){var b=v("div"),c=v("a",{href:this.Dda},a);x(b,c);x(this.e,b)}};
mF.prototype.fV=function(){if(this.jca){var a=v("div",{"class":"gphoto-sidebar-subitem"}),b=v("a",{href:this.Yca},v("img",{src:"img/transparent.gif","class":"SPRITE_map-sm lhcl_spriting_alignMiddle lhcl_spriting_marginRight5"}),dx);x(a,b);x(this.e,a);var c=v("div",{"class":"gphoto-sidebar-subitem"}),d=v("img",{"class":"SPRITE_earth-sm lhcl_spriting_marginRight5 lhcl_spriting_alignMiddle ",src:"img/transparent.gif"}),e=v("a",{href:this.Gca},cx);x(c,d);x(c,e);x(this.e,c)}};
mF.prototype.UU=function(){if(this.FT){var a=m("View Blog"),b=v("div",{"class":"gphoto-sidebar-subitem"}),c=v("img",{src:"img/blogo16.gif",width:"16px",height:"16px"}),d=v("a",{href:this.FT},a);x(b,c);x(b,d);x(this.e,b)}}};var nF=function(){P.call(this)};
o(nF,P);nF.prototype.D=function(){return"gphoto.AlbumLinkSidebar"};
nF.prototype.show=function(a){M(this.e,a)};
nF.prototype.U7=function(a,b){if(this.Ha())this.e.innerHTML=jx("","",'<a href="'+b+'">'+Xy(p(a))+"</a>")};
nF.prototype.m=function(){this.e=v("div",{"class":"lhcl_toolbox"})};var oF=function(a){P.call(this);this.eb=a};
o(oF,P);oF.prototype.show=function(a){M(this.e,a)};
oF.prototype.ae=function(a){this.Q=a};
oF.prototype.aa=function(){if(this.k)this.dispatchEvent(new jF(this.k))};
oF.prototype.isOwner=function(a){return this.P&&this.P.isOwner(a)};
oF.prototype.me=function(){return this.eb.appContext};
oF.prototype.zA=function(){return this.eb};{var pF=function(a){lF.call(this,a);this.Os=new Sk;this.Os.Jd("lhcl_addPersonButton");this.Z(this.Os);this.Qs=new Sk;this.Qs.Jd("lhcl_addTagButton");this.Z(this.Qs);this.Dc=[];this.js={}};
o(pF,lF);pF.prototype.n=function(){pF.f.n.call(this);this.p.g(this.Qs,[xk,I],this.J9,true);this.p.g(this.Os,[xk,I],this.ZZ,true)};
pF.prototype.U=function(a){pF.f.U.call(this,a);this.oB();this.Qs.M(this.$e.isOwner(a),true);this.Os.M(this.$e.isOwner(a),true);ec(this.Dc,function(b){this.removeChild(b,true)},
this);this.Dc=[];Oc(this.js);this.Um="";this.k.Ei(l(function(){r(this.k.On(),this.SF,this)},
this));if(this.$r)this.$r.ZE(this.k.id,this.k.user.name,this.$e.isOwner(this.k))};
pF.prototype.m=function(){pF.f.m.call(this);if(_features.fr&&_features.froptin){var a=m("People");this.Xi("people",a);this.MC=this.pq("people");this.$r=new qF(this.MC.element);this.Z(this.$r,false);this.$r.C(this.MC.element)}this.Xi("tags",Ku);this.kQ=this.pq("tags");this.uE=v("ul",{"class":"lhcl_taglist"});x(this.kQ.element,this.uE)};
pF.prototype.C=function(a){pF.f.C.call(this,a);var b=w("td");x(this.kQ.headerElement,b);this.Qs.C(b);if(this.MC){var c=w("td");x(this.MC.headerElement,c);this.Os.C(c)}};
pF.prototype.RF=function(a,b){var c=a.value,d=false;if(c){var e=Ws(c);if(this.Faa(e)){d=true;r(e,function(f){var g=this.AV(f);if(g){var h=this.SF(g);d=d&&!(!h)}else d=false},
this)}if(d)this.k.SC()}if(d)a.value="";if(!b&&d)this.oB()};
pF.prototype.oB=function(a){if(a)a.preventDefault();if(this.eh)M(this.eh,false)};
pF.prototype.ZZ=function(){if(this.$r)this.$r.dh()};
pF.prototype.J9=function(){if(!this.eh){this.eh=v("div",{"class":"lhcl_addtagform",style:"display: none;"});var a=v("table",{"class":"lhcl_addtag"}),b=w("tbody"),c=w("tr"),d=v("td",{colspan:"2",width:"100%"}),e=v("input",{id:"lhid_newTagsText","class":"lhcl_taginputfield",type:"text",maxlength:"4096"}),f=v("td",{"class":"lhcl_addtagbuttoncell"}),g=v("td",{"class":"lhcl_addtagbuttoncell"}),h=w("tr"),i=new Sk(Mu),k=new Sk(Lu);this.p.g(i,[I,xk],l(function(t){t.preventDefault();this.RF(e)},
this));this.p.g(k,[I,xk],this.oB);this.p.g(e,If,l(function(t){if(t.keyCode==13){this.RF(e,true);t.preventDefault()}},
this));this.Z(i);this.Z(k);i.C(f);k.C(g);this.y$=e;x(d,e);x(c,d);x(h,f);x(h,g);x(a,b);x(b,c);x(b,h);var n=v("div",{"class":"lhcl_pseudo_hr"});x(this.eh,n);x(this.eh,a);x(this.kQ.element,this.eh);Xe(this.eh,this.uE);var q=v("div",{"class":"lhcl_addtaghelp"});q.innerHTML=Ou+"<br/>"+Nu;x(this.eh,q)}if(this.eh.style.display!="none")M(this.eh,false);else{this.y$.value="";M(this.eh,true);this.y$.focus()}};
pF.prototype.SF=function(a){var b=String(a.t).toLowerCase();if(!this.js[b]){var c=new rF(a,this.$e.isOwner(this.k)),d=yc(this.Dc,c,sF);if(d<0){d=-(d+1);rc(this.Dc,c,d)}this.Z(c);c.C(this.uE);this.js[b]=c;if(d>=0){var e=this.uE.childNodes[d];if(e)Xe(c.A(),e)}return true}return false};
pF.prototype.$V=function(a){var b=a.tag;tc(this.Dc,a);this.removeChild(a,true);Nc(this.js,b.title);this.k.B6(b,true)};
pF.prototype.AV=function(a,b){var c=this.js[a];if(c)return null;return this.k.PS(a,b)};
pF.prototype.Faa=function(a){if(a.length>100){alert("Too many tags (max is 100, but "+a.length+" entered)");return false}if(a.length==this.Dc.length){var b=0;for(var c=0;c<a.length;++c)if(this.js[a[c]])++b;else break;if(b==this.Dc.length)return false}var d=0;for(var c=0;c<a.length;++c){var e=a[c];if(e.length>128){alert("Tags can only be 128 letters long. You entered one that was "+e.length+" letters long.");return false}d+=e.length}if(d>4096){alert("The maximum length for all tags has been exceeded. Each photo may use up to 4096 characters for all tags, but "+
d+" have been entered.");return false}return true};
pF.prototype.JZ=function(a,b){if(!this.Um){this.Um="/lh/view?uname="+this.k.user.name;if(b)this.Um+="&isOwner=true";this.Um+="&tags="}return this.Um+ab(a.t)};
var rF=function(a,b){P.call(this);this.tag=a;this.Fb=b};
o(rF,P);var sF=function(a,b){return Za(a.tag.t,b.tag.t)};
rF.prototype.m=function(){this.e=w("li");var a=v("a"),b=this.tag.t,c=b;c=ob(c,30);a.title=b;B(a,c);a.href=this.P.JZ(this.tag,this.Fb);x(this.e,a);if(this.Fb){var d=v("a",{"class":"lhcl_tagdelete"});L(d,"position","absolute");x(this.e,d);$f(d,I,this.vj,false,this)}};
rF.prototype.vj=function(){if(this.P)this.P.$V(this)}};{var tF=function(a){lF.call(this,a)};
o(tF,lF);tF.prototype.m=function(){};
tF.prototype.C=function(a){this.e=a;this.GI=a;this.Fi=u("lhid_map");if(this.GI&&this.Fi){var b=u("lhid_geolinks");We(b);this.pp=v("span",{tabindex:0,"class":"lhcl_fakelink",style:"display: none;"});b.appendChild(this.pp);b.appendChild(w("br"));this.p.g(this.pp,I,this.JS);this.Gaa=v("a",{href:"#",style:"display: none;"},Hv);b.appendChild(this.Gaa)}this.vt=null;this.n()};
tF.prototype.U=function(a){tF.f.U.call(this,a);M(this.pp,this.$e.isOwner(this.k));if(a){a.Ei(l(function(){this.NQ(a)},
this));if(a.Gb())u("lhid_mapheader").innerHTML=Fv;else u("lhid_mapheader").innerHTML=Gv}};
tF.prototype.NQ=function(a){if(_features.showgeo){if(a.lat&&a.lon){if(!this.o)this.o=new WA(this.Fi,2);this.bp(true);this.o.Lo(a)}else this.bp(false);this.oaa()}};
tF.prototype.bp=function(a){if(_features.geotagging){a=_features.showgeo&&a;var b=this.$e.isOwner(this.k)?this.Fi:this.GI;M(b,a)}};
tF.prototype.Vt=function(){return null};
tF.prototype.qY=function(a){if(_features.geotagging)if(a.lat&&a.lon){var b={lat:a.lat,lon:a.lon,address:a.location||""};if(a.bb)b.bb={ll:{lat:a.bb.ll.lat,lon:a.bb.ll.lon},ur:{lat:a.bb.ur.lat,lon:a.bb.ur.lon},latSpan:a.bb.latSpan,lonSpan:a.bb.lonSpan};return b}return null};
tF.prototype.oaa=function(){if(_features.showgeo){var a=this.k,b=a.lat&&a.lon;if(this.$e.isOwner(a)){var c;c=b?Jv:Iv;B(this.pp,c)}}};
tF.prototype.JS=function(){if(this.$e.isOwner(this.k)){var a=this.k,b=this.vt,c;if(!(a.lat&&a.lon)&&!b)b=this.Vt(this.oa);if(this.zg){this.zg.b();this.zg=null}if(this.ol){H(this.ol);this.ol=null}this.zg=new XA(a,l(this.IS,this,a),c,b?[b]:null)}};
tF.prototype.IS=function(a){if(this.$e.isOwner(this.k)&&a==this.k){this.vt=a.lat&&a.lon?this.qY(a):null;this.NQ(a)}}};{var vF=function(a){lF.call(this,a);if(_features.creativecommons){this.UL=new uF(this.$e);this.Z(this.UL)}};
o(vF,lF);vF.prototype.m=function(){var a=u("lhid_tools");this.am=new lA(null,a,null,null,null,true);this.e=this.am.e;if(_features.creativecommons){this.Lca=this.pq("license");this.UL.C(this.Lca.element)}this.YB=this.pq("links");M(this.YB.element,false);this.Vq(Ev,this.Xb);this.Vq(Dv,this.NU);this.Vq(Cv,this.jr);this.Mba=this.Vq(zv,this.vj);this.Nba=this.Vq(yv,this.vj);var b=v("div");b.innerHTML=wF;L(b,"padding-top","10px");x(this.YB.element,b);this.p.g(u("lhid_snippet_size"),kg,this.gy);this.p.g(u("lhid_snippet_link"),
kg,this.gy);this.gy();this.show(false);A(this.e)};
vF.prototype.Vq=function(a,b){var c=v("div",{"class":"lhcl_fakelink"});B(c,a);this.p.g(c,I,b);x(this.YB.element,c);return c};
vF.prototype.u=function(){vF.f.u.call(this);this.am.ha()};
vF.prototype.U=function(a){vF.f.U.call(this,a);if(a)a.Ei(l(function(){this.am.fe(a.r);M(this.YB.element,this.$e.isOwner(a));if(this.$e.isOwner(a))this.gy();if(_features.creativecommons)this.UL.U(a)},
this));else this.am.fe(null)};
vF.prototype.NU=function(){if(this.k)LH_copyDialog([this.k.id])};
vF.prototype.jr=function(){var a=true;if(!this.k)return;if(_album&&_album.isdefault&&_features.newStrings)a=confirm(sv);else if(_album&&_album.blogger)a=confirm(tv);if(a)LH_moveDialog([this.k.id])};
vF.prototype.vj=function(){if(this.k)this.k.yt()};
vF.prototype.haa=function(a,b){if(!this.P.rd(b)&&!this.P.de(b))this.dispatchEvent(new Jt(this.k.album.id,a))};
vF.prototype.Xb=function(){var a=["selectedphotos="+this.k.id,"noredir=true","optgt=BANNER","uname="+_user.name,"aid="+this.k.album.id],b=this.k.ph.media.thumbnail[0].url;vz.qc(_selectedPhotosPath,l(this.haa,this,b),a.join("&"))};
vF.prototype.gy=function(){u("lhid_link").value=window.location.href.toString();var a=u("lhid_snippet_size").value,b=u("lhid_snippet_link");if(this.k){var c=this.k.s,d=c.lastIndexOf("/"),e=c.substr(0,d),f=c.substr(d),g={l:this.k.lu(),s:e+"/s"+a+f,al:this.k.album.link,a:this.k.album.name};u("lhid_snippet").value=ez(b.checked?'<a href="{l}"><img src="{s}" /></a>':xA,g);M(this.Mba,!this.k.Gb());M(this.Nba,this.k.Gb())}};
var xF=m("Embed image"),wF='<img src="img/transparent.gif" class="SPRITE_link lhcl_spriting_alignMiddle lhcl_spriting_marginRight5" /><span style="vertical-align:middle">'+vu+'</span><br><input id="lhid_link" type="text"  onclick="this.select()"></input><br><img src="img/transparent.gif" class="SPRITE_embed"> '+xF+'<div id="lhid_embed_form"><div style="margin-top: 0.4em;"><input onclick="this.select()" type="text" id="lhid_snippet" /></div><div>'+nv+'<select id="lhid_snippet_size"><option value="144">'+
mv+" "+sx(144)+'</option><option value="288">'+lv+" "+sx(288)+'</option><option value="400">'+kv+" "+sx(400)+'</option><option value="800">'+jv+" "+sx(800)+'</option></select></div><input type="checkbox" id="lhid_snippet_link"/>'+Bv+"<br /></div>"};{var uF=function(a){lF.call(this,a)};
o(uF,lF);uF.prototype.m=function(){var a=v("div",{"class":"lhcl_tools lhcl_license",id:"lhid_license"});this.$(a)};
uF.prototype.$=function(a){uF.f.$.call(this,a);var b=new fn;b.F('<img id="lhid_lic_img_c" src="img/transparent.gif" ');b.F('    class="SPRITE_license_no lhcl_spriting_alignMiddle" />');b.F('<img id="lhid_lic_img_by" src="img/transparent.gif" ');b.F('    class="SPRITE_license_by lhcl_spriting_alignMiddle" />');b.F('<img id="lhid_lic_img_nc" src="img/transparent.gif" ');b.F('    class="SPRITE_license_nc lhcl_spriting_alignMiddle" />');b.F('<img id="lhid_lic_img_nd" src="img/transparent.gif" ');b.F('    class="SPRITE_license_nd lhcl_spriting_alignMiddle" />');
b.F('<img id="lhid_lic_img_sa" src="img/transparent.gif" ');b.F('    class="SPRITE_license_sa lhcl_spriting_alignMiddle" />');b.F('<span id="lhid_lic_text_c" class="lhcl_license ');b.F('    lhcl_license_greyTextColor lhcl_spriting_marginRight5">');b.F(p(xx));b.F("</span>");b.F('<a id="lhid_lic_text_cc" ');b.F('   class="lhcl_license lhcl_license_greyTextColor ');b.F('          lhcl_spriting_marginRight5" ');b.F('   target="_blank">');b.F(p(yx));b.F("</a>");b.F('<span id="lhid_lic_editButton" style="margin-left:1em;" ');
b.F('    class="lhcl_toollink lhcl_fakelink lhid_lic_editBtn">');b.F(Cw);b.F("</span>");b.F('<div id="lhid_lic_editDiv" style="display:none;">');b.F('  <ul style="margin:0.5em 0pt;padding-left:0px;"');b.F('      class="lhcl_license">');b.F('    <li class="lhcl_license_checkboxLabel">');b.F('      <label class="lhcl_license">');b.F('        <input type="radio" id="lhid_lic_category_c" ');b.F('         class="lhid_lic_category_c lhcl_license ');b.F('           lhcl_license_inputElement" ');b.F('         name="lhid_lic_category" />');
b.F(p(zx));b.F("      </label>\n");b.F("    </li>");b.F('    <li class="lhcl_license_checkboxLabel">');b.F('      <label class="lhcl_license">');b.F('        <input type="radio" id="lhid_lic_category_cc" ');b.F('          class="lhid_lic_category_cc lhcl_license ');b.F('          lhcl_license_inputElement" name="lhid_lic_category" />');b.F(p(Ax));b.F('</label> (<a href="http://creativecommons.org/licenses/" ');b.F('          class="lhcl_license_textColor" ');b.F('          target="_blank">Creative Commons</a>)');
b.F('      <div id="lhid_lic_cc_popup" style="display:none;">\n');b.F('        <ul style="list-style-type:none;">');b.F('          <li class="lhcl_license_checkboxLabel">');b.F("            <label>");b.F('              <input type="checkbox" id="lhid_lic_cc_remix" ');b.F('                name="lhid_lic_cc_remix" ');b.F('                class="lhid_lic_cc_remix lhcl_license ');b.F('                  lhcl_license_inputElement"');b.F('                value="lhid_license_allowRemix" />');b.F(p(Bx));b.F("            </label>");
b.F("            <br />");b.F("          </li>");b.F('          <li class="lhcl_license_checkboxLabel">');b.F("            <label>");b.F('              <input type="checkbox" id="lhid_lic_cc_com" ');b.F('                  name="lhid_lic_cc_com" class="lhid_lic_cc_com ');b.F('                  lhcl_license lhcl_license_inputElement" />');b.F(p(Cx));b.F("            <label>");b.F("            <br />");b.F("          </li>");b.F('          <li class="lhcl_license_checkboxLabel">');b.F('            <label id="lhid_lic_cc_sa_lab">');
b.F('              <input type="checkbox" id="lhid_lic_cc_sa" ');b.F('                name="lhid_lic_cc_sa" class="lhid_lic_cc_sa ');b.F('                  lhcl_license lhcl_license_inputElement" />');b.F(p(Dx));b.F("            </label>");b.F("          </li>");b.F("        </ul>");b.F("      </div>");b.F('      <div class="lhid_lic_saveCancelDiv" ');b.F('          style="margin-top:1em;margin-bottom:1em;">');b.F("      </div>");b.F('      <div id="lhid_license_resetToDefaultButton" ');b.F('          class="lhid_license_resetToDefaultButton lhcl_toollink ');
b.F('          lhcl_fakelink" style="margin-top:1em;margin-bottom:1em;');b.F("          cursor:pointer;text-align:left;color:#112abb;");b.F('font-size:0.86em;">');b.F(p(Fx)+"</div>");b.F(p(Gx));b.F('        <a href="/lh/settings#licensing" style="color:#112abb;');b.F('            text-decoration:underline;font-size:0.86em;" ');b.F('            class="lhcl_license_linkButton ');b.F('            lhid_license_setDefaultButton lhcl_toollink">');b.F(p(Hx)+"</a>");b.F('      <div style="margin-bottom:1em;" />');
b.F("    </li>");b.F("  </ul>");b.F("</div>");b.F('<div class="lhcl_pseudo_hr" />');a.innerHTML=b.toString();var c=Me("div","lhid_lic_saveCancelDiv",a)[0],d=new Sk(ky);this.Z(d);d.C(c);this.p.g(d,[I,xk],this.k7);var e=new v("span");L(e,"marginLeft","20px");x(c,e);var f=new Sk(wx);this.Z(f);f.C(c);this.p.g(f,xk,this.aU);g=Me("div","lhid_license_resetToDefaultButton",a);this.p.g(g[0],E,this.T6);if(_authuser.isOwner){var g=Me("span","lhid_lic_editBtn",a);this.p.g(g[0],I,this.HW)}g=Me("input","lhid_lic_category_c",
a);this.p.g(g[0],E,this.ss);g=Me("input","lhid_lic_category_cc",a);this.p.g(g[0],E,this.ss);g=Me("input","lhid_lic_cc_remix",a);this.p.g(g[0],E,this.ss);g=Me("input","lhid_lic_cc_com",a);this.p.g(g[0],E,this.ss);g=Me("input","lhid_lic_cc_sa",a);this.p.g(g[0],E,this.ss)};
uF.prototype.HW=function(){this.bs(true)};
uF.prototype.bs=function(a){var b=u("lhid_lic_editDiv");M(b,a);var c=u("lhid_lic_editButton");M(c,!a)};
uF.prototype.U=function(a){uF.f.U.call(this,a);if(a&&a.r){var b=this.kq();this.BP(this.k.NA(),b);var c=a.isOwner(this.$e.zA().authUser),d=u("lhid_lic_editButton");M(d,c)}};
uF.prototype.kq=function(){var a=new YC;a.rb_c=u("lhid_lic_category_c");a.rb_cc=u("lhid_lic_category_cc");a.lab_c=u("lhid_lic_text_c");a.lab_cc=u("lhid_lic_text_cc");a.div_cc=u("lhid_lic_cc_popup");a.img_c=u("lhid_lic_img_c");a.img_by=u("lhid_lic_img_by");a.img_nc=u("lhid_lic_img_nc");a.img_nd=u("lhid_lic_img_nd");a.img_sa=u("lhid_lic_img_sa");a.chk_remix=u("lhid_lic_cc_remix");a.chk_sa=u("lhid_lic_cc_sa");a.lab_sa=u("lhid_lic_cc_sa_lab");a.chk_com=u("lhid_lic_cc_com");return a};
uF.prototype.ss=function(){var a=this.kq(),b=RC(a);this.TQ(b,a)};
uF.prototype.TQ=function(a,b){VC(a,b);var c=u("lhid_license_resetToDefaultButton");M(c,this.k.zL())};
uF.prototype.BP=function(a,b){WC(a,b);var c=u("lhid_license_resetToDefaultButton"),d=this.k.zL()!=0;M(c,d)};
uF.prototype.k7=function(){var a=this.kq(),b=RC(a),c=["iid="+this.k.id,"opt=SET","lic="+b],d=_setLicensePath+"&"+c.join("&"),e=l(this.l7,this,a);Gj(d,function(){var f=this.Sb();e(f,b)})};
uF.prototype.l7=function(a,b,c){if(b.indexOf("<result>success</result>")==-1)alert(Ex);this.k.eP(c);this.k.fP(true);this.TQ(c,a);uF.prototype.bs(false)};
uF.prototype.aU=function(){var a=this.kq();WC(this.k.NA(),a);this.bs(false)};
var yF=/<result>(.*)<\/result>/,zF=/<accountlic>(.*)<\/accountlic>/;uF.prototype.T6=function(){var a=["iid="+this.k.id,"opt=DELETE"],b=_setLicensePath+"&"+a.join("&"),c=l(this.U6,this);Gj(b,function(){var d=this.Sb(),e=d.match(yF)[1],f=d.match(zF),g=f?f[1]:"";c(e,g)})};
uF.prototype.U6=function(a,b){if(a!="success")alert(Ex);if(b!="")this.k.eP(b);this.k.fP(false);var c=this.kq();this.BP(this.k.NA(),c);this.bs(false)}};var AF="iconclass",BF="itemwidth",CF="itemheight",DF="orientation",EF="horizontal",FF="preloadfunction",GF="maxloadsize",HF="originalsizefn";{var IF=function(a,b,c,d,e){ot.call(this,b,c,e);this.eb=d;this.Iz=d.debug;this.Lea=d.updateimages;this.ica=d[FF];if(e)this.U(e)};
o(IF,ot);IF.prototype.Yd=true;IF.prototype.uI=false;IF.prototype.PM=false;IF.prototype.m=function(){this.$(w("div"))};
IF.prototype.$=function(a){this.e=a;this.e._fsIndex=this.oa;this.e.className="filmstrip-icon gphoto-icon-loading";this.sa=w("img");this.e.appendChild(this.sa);if(this.Iz){this.cg=w("div");this.e.appendChild(this.cg)}this.OD(this.oa,false);this.If()};
IF.prototype.U=function(a,b){IF.f.U.call(this,a);if(typeof b!="undefined")this.OD(b);this.If()};
IF.prototype.Nh=function(a){this.Yd=a;this.If()};
IF.prototype.U8=function(a){this.PM=a;this.If()};
IF.prototype.LD=function(a){this.uI=a;this.If()};
IF.prototype.aa=function(){this.If()};
IF.prototype.If=function(){if(this.e){if(this.Iz&&this.cg)B(this.cg,this.oa);if(this.Lea&&this.k)if(this.Yd){this.e.style.backgroundImage="";C(this.e,"gphoto-icon-loading")}else{qf(this.e,"gphoto-icon-loading");var a=this.ica(this.k,this.eb);this.sa.src=a}if(this.PM)C(this.e,"gphoto-icon-offscreen");else qf(this.e,"gphoto-icon-offscreen");if(this.uI)C(this.e,"gphoto-icon-fail");else qf(this.e,"gphoto-icon-fail")}};
IF.prototype.Vi=function(a){this.OD(a,true)};
IF.prototype.OD=function(a,b){IF.f.Vi.call(this,a);if(this.e){this.e._fsIndex=a;this.sa._fsIndex=a}if(b)this.If()}};{var KF=function(a,b,c,d,e){var f=d||{};f[FF]=f[FF]||JF;IF.call(this,a,b,c,f,e)};
o(KF,IF);var JF=function(a,b){if(!a||a==Rs)return"";return cs(as(a.thumbnail().url),b[GF],b.crop)},
LF=function(a){if(a&&Ba(a.w)&&Ba(a.h))return new ne(a.w,a.h);return null}};{var PF=function(a){if(a>=1)return 1;if(a<=0)return 0;if(MF==1)NF();return OF(a)},
MF=1,OF=function(a){var b=0;a=a*8;if(a<1)b=a-(1-Math.exp(-a));else{var c=Math.exp(-1);a-=1;var d=1-Math.exp(-a);b=c+d*(1-c)}return b*MF},
NF=function(){MF=1/OF(1)}};{var QF=function(){Fk.call(this)},
RF;o(QF,Fk);QF.prototype.O=function(){return"gphoto-filmstrip"};
QF.prototype.m=function(a){var b=QF.f.m.call(this,a);this.ia(a,b);return b};
QF.prototype.ia=function(a,b){var c=QF.f.ia.call(this,a,b),d=v("div",{"class":"gphoto-filmstrip-images"}),e=v("div",{"class":"imagestrip"});x(d,e);x(b,d);a.E8(e,d);return c}};{var UF=function(a,b,c){Q.call(this,"",b||RF||(RF=new QF),c);this.eb=Sc(SF);Uc(this.eb,a);this.Pe=this.eb[AF];this.tda=this.eb[HF]||TF;this.M5=this.eb[FF];var d;if(this.eb[DF]==EF){d=this.eb[BF];this.Zv="left"}else{d=this.eb[CF];this.Zv="top"}this.Aj=this.eb.itemgap+d;var e=this.eb.swapthreshold*this.Aj,f=Math.floor(e);this.fba=Math.ceil(e);this.u$=Math.floor(this.Aj+f);this.Pp=this.eb.animationtime;this.nda=this.eb.offscreencount;this.ki={};this.Mg={};this.Uq=[];this.Yg=[];this.$ca=this.eb[GF];this.Hp=
this.eb.crop;this.tca=l(this.rf,this);this.sca=l(this.a2,this)};
o(UF,Q);var VF="currentitemchanged",SF={};SF[BF]=80;SF[CF]=96;SF.itemgap=5;SF.animationtime=750;SF[AF]=null;SF[DF]=EF;SF.debug=false;SF.updateimages=true;SF.swapthreshold=0.3333333333333333;SF.offscreencount=2;SF[FF]=null;SF[GF]=0;SF.crop=false;SF[HF]=null;var TF=function(){return null};
UF.prototype.ah=-1;UF.prototype.Nf=0;UF.prototype.kr=0;UF.prototype.cH=0;UF.prototype.E8=function(a,b){this.Hq=a;this.Bi=b};
UF.prototype.n=function(){UF.f.n.call(this);this.VQ(this.uJ());if(this.k){this.uG();this.Ir(1)}};
UF.prototype.Jc=function(a){if(this.sb())switch(a.keyCode){case 37:this.wp();a.preventDefault();return true;case 39:this.forward();a.preventDefault();return true;case 34:this.b5();a.preventDefault();return true;case 33:this.a5();a.preventDefault();return true}return UF.f.Jc.call(this,a)};
UF.prototype.u=function(){if(this.jc)this.jc.dW();UF.f.u.call(this);A(this.Hq)};
UF.prototype.qZ=function(){return Math.min(this.ah>0?this.ah*2:0,this.k.S())};
UF.prototype.rf=function(a){this.GQ(a,1)};
UF.prototype.a2=function(a){this.GQ(a,2)};
UF.prototype.GQ=function(a,b){var c=a.EZ();if(!this.Mg[c])return null;else this.Mg[c][1]=b;var d=this.Mg[c][0];this.Uq[d]=b;var e=this.ki[d];if(e){e.Nh(false);if(2==b)e.LD(true)}return e};
UF.prototype.Ir=function(a,b){if(!(this.k&&this.M5))return;var c=this.qZ(),d=Ba(b)?b:this.Nf,e=Math.min(Math.max(0,d+a*c),this.$n),f=Math.min(d,e),g=Math.max(d,e);this.k.Eo(f,g,l(this.vW,this,f,g-f+1))};
UF.prototype.U=function(a){UF.f.U.call(this,a);this.uG();this.Ir(1);var b=this.Nf,c=Math.min(b+this.ah,this.$n);this.k.Eo(b,c,l(this.k1,this));this.dispatchEvent({type:VF,index:this.Nf,model:this.k})};
UF.prototype.uJ=function(){return Ah(this.e)};
UF.prototype.VQ=function(a){var b;b=this.eb[DF]==EF?Math.floor(a.width/this.Aj):Math.floor(a.height/this.Aj);b=Math.max(0,b);if(this.k){var c=this.k.S();b=Math.min(b,c);this.$n=Math.max(c-1,0);this.Jca=b>=c}var d=b*this.Aj;if(this.Bi)if(this.eb[DF]==EF)this.Bi.style.width=d+"px";else this.Bi.style.height=d+"px";this.ah=b};
UF.prototype.RN=function(){if(!this.Ha())return;this.VQ(this.uJ());var a;a=this.k&&this.ah>=this.k.S()?this.ah:this.ah+this.nda;var b=this.Yg.length-a;if(b>0)for(var c=this.Yg.length-1;c>=0&&c<=b;c--)this.v6(c);else if(a>this.Yg.length&&this.k&&this.k.S()){var d=Math.max(a+b,0),e=a-d;for(var c=d;c<e;++c){var f=this.k?this.k.wb(c):null,g=new this.Pe(this,"icon",c,this.eb,f);this.Ms(g,c);this.JQ(g,c);this.ki[c]=g}}};
UF.prototype.uG=function(){if(!this.Ha())return;delete this.ki;this.ki={};delete this.Mg;this.Mg={};this.Uq.length=0;this.Yg.length=0;this.dca=false;this.Jca=false;this.kr=0;this.cH=0;this.ah=-1;this.Nf=0;if(this.Hq){We(this.Hq);this.Hq.style[this.Zv]=0}if(this.k&&this.k.S())this.RN();this.dca=true};
UF.prototype.Ms=function(a,b){if(a){this.Yg[b]=a;this.Z(a);a.C(this.Hq);a.sC=b*this.Aj;a.A().style[this.Zv]=a.sC+"px"}};
UF.prototype.v6=function(a){var b=this.Yg[a];if(b){this.ki[b.Eg()]=null;this.removeChild(b);this.Yg.splice(a,1)}};
UF.prototype.forward=function(){this.Rr(Math.min(this.$n,this.Nf+1))};
UF.prototype.wp=function(){this.Rr(Math.max(0,this.Nf+-1))};
UF.prototype.b5=function(){this.Rr(Math.min(this.$n,this.Nf+this.ah-1))};
UF.prototype.a5=function(){this.Rr(Math.max(0,this.Nf-this.ah+1))};
UF.prototype.k1=function(a,b){var c=b.length;this.laa(a,c,0)};
UF.prototype.laa=function(a,b,c){for(var d=0;d<Math.min(this.ah,b);++d){var e=c+d,f=this.Yg[e],g=a+d,h=this.k.wb(g);if(h==f.na())continue;f.U(h,g);var i=this.Uq[g];f.Nh(!i);f.LD(i==2);this.ki[g]=f}};
UF.prototype.vW=function(a,b){for(var c=0;c<b;++c){var d=a+c,e=!(!this.Uq[d]);if(e)continue;var f=this.k.wb(d);if(!f)continue;var g=this.M5(f,this.eb);if(!g){var h=this.ki[d];if(h)h.Nh(false);this.Uq[d]=true;continue}var i=this.Mg[g];if(!i){this.Mg[g]=[d,-1];rs(as(g),this.tda(f,this.eb),this.$ca,this.Hp,this.tca,this.sca)}}};
UF.prototype.Rr=function(a){var b=this.y4(a);if(!b)return;this.dispatchEvent({type:VF,index:this.Nf,model:this.k})};
UF.prototype.y4=function(a,b){a=pe(a,0,this.$n);if(this.Nf==a)return false;if(this.jc){this.jc.stop();this.jc=null}var c=this.RU(a);if(!c)return false;var d=[c.startPos,0],e=[c.stopPos,0];c.endAnimCallback=b;if(this.Pp==0)this.iI(c,null);else{this.jc=new Ug(d,e,this.Pp,PF);this.jc.addEventListener(Wg,l(this.uW,this,c));this.jc.addEventListener("end",l(this.iI,this,c));this.jc.play()}this.Nf=a;this.Ir(c.direction);return true};
UF.prototype.RU=function(a){var b=this.Nf-a;if(a<0||b==0)return null;var c=b<0?1:-1;return{direction:c,startPos:this.kr,stopPos:-1*a*this.Aj-this.cH}};
var WF=function(a,b){var c;if(b==1){c=a.shift();a.push(c)}else{c=a.pop();a.unshift(c)}return c};
UF.prototype.JQ=function(a,b){var c=this.Uq[b],d=!c;a.Nh(d);a.LD(c==2)};
UF.prototype.v$=function(a){var b=WF(this.Yg,a),c=b.Eg(),d=b.sC+a*this.Yg.length*this.Aj;b.sC=d;var e=c+a*this.Yg.length;b.A().style[this.Zv]=d+"px";var f=this.k?this.k.wb(e):null;this.ki[c]=null;if(e<=this.$n)if(e<0||!f)b.Vi(e);else{this.JQ(b,e);b.U(f,e);this.ki[e]=b}else{b.Vi(e);b.Nh(false)}b.U8(e>this.$n)};
UF.prototype.iI=function(a){this.jr(a.stopPos,a.direction);if(a.endAnimCallback)a.endAnimCallback();this.jc=null};
UF.prototype.uW=function(a,b){var c=a.direction==1?Math.floor(b.x):Math.ceil(b.x);if(c==a.stopPos&&this.jc){this.jc.stop();this.jc=null}else this.jr(c,a.direction)};
UF.prototype.KG=function(a,b){var c;do{var d=this.Yg[0].sC;c=b==1?Math.abs(d+this.kr):Math.abs(d+this.Aj+this.kr)+this.fba;if(c>=this.u$)this.v$(b)}while(c>=this.u$)};
UF.prototype.jr=function(a,b){this.KG(a,b);this.kr=a;this.Hq.style[this.Zv]=a+"px";this.KG(a,b)};
UF.prototype.Ud=function(a){UF.f.Ud.call(this,a);var b=a.target._fsIndex;if(typeof b=="number")this.Rr(b)};
UF.prototype.Hk=function(a){var b=this.ki[a];if(b)b.aa()};
UF.prototype.D=function(){return"gphoto.FilmStrip"}};{var YF=function(a){lF.call(this,a);this.rh=new XF};
o(YF,lF);YF.prototype.D=function(){return"gphoto.FlaggerBox"};
YF.prototype.m=function(){YF.f.m.call(this);this.rh.ia(this.A());this.Pt=v("span",{id:"lhid_flagger"});x(this.A(),this.Pt);this.OM=v("div",{"class":"lhcl_fakelink issueLink"});B(this.OM,wy);x(this.e,this.OM)};
YF.prototype.n=function(){YF.f.n.call(this);this.p.g(this.rh,"gphoto_flag_sent",this.H5);this.p.g(this.OM,[I,E],this.O6)};
YF.prototype.O6=function(a){a.stopPropagation();a.preventDefault();if(this.k.type=="Album")this.dispatchEvent(new su(null,this.na().H()));else this.dispatchEvent(new su(this.na()))};
YF.prototype.b=function(){if(!this.W()){YF.f.b.call(this);this.rh.b()}};
YF.prototype.U=function(a){YF.f.U.call(this,a);if(a)if(a.type=="Album")this.yP(a);else if(a.type=="Photo")a.Ei(l(this.yP,this,a))};
YF.prototype.yP=function(a){if(a.type=="Album")this.rh.Uo(a.username,a.id,null,false);else if(a.type=="Photo")this.rh.Uo(a.user.name,a.album.id,a.id,a.Gb());We(this.Pt);x(this.Pt,v("a",{href:"javascript: void 0;"},vy));this.rh.hI(true)};
YF.prototype.H5=function(){We(this.Pt);x(this.Pt,v("b",null,ZF));this.rh.hI(false)}};var XF=function(a){P.call(this,a);this.j=new K(this);this.lD=new Dj};
o(XF,P);XF.prototype.j=null;XF.prototype.lD=null;XF.prototype.kv=false;XF.prototype.D=function(){return"gphoto.FlaggerWidget"};
XF.prototype.m=function(){XF.f.m.call(this)};
XF.prototype.n=function(){XF.f.n.call(this)};
XF.prototype.u=function(){XF.f.u.call(this);this.j.ma()};
XF.prototype.b=function(){if(!this.W()){XF.f.b.call(this);this.j.b();this.lD.b()}};
XF.prototype.Uo=function(a,b,c,d){if(!this.kv){this.Wh=a;this.sd=b;this.Yf=c;this.tfa=d}};
XF.prototype.Fp=function(){if(!this.kv){var a=new R("lhcl_flagger");a.Xi(vy);a.hc($F);var b=new Xk;b.J("ok",hx,true);b.J(dl,wx,false,true);a.Rg(b);this.j.g(a,Zk,l(this.tK,this));a.M(true);this.kv=true;Iy(this,Gy)}};
XF.prototype.VG=function(a){var b=a.target;this.j.Sa(b,Zk,this.tK);b.b();this.kv=false;Iy(this,Hy)};
XF.prototype.tK=function(a){if(a.type==Zk){var b=a.key;if(b=="ok"){var c=this.L7(a);if(c==Li){this.dispatchEvent("gphoto_flag_sent");return this.VG(a)}else if(c==Mi)return false}}this.dispatchEvent("gphoto_flag_canceled");this.VG(a)};
XF.prototype.L7=function(a){var b=xf(a.target.Oa().firstChild),c=gd(b,"reason"),d=gd(b,"msg"),e=Va(String(d)),f;if(!c)f=Jw;else if(e.length>200)f="We're sorry, the maximum length for flagging messages is 200 characters. You are currently using "+e.length+" characters. Please modify your message and try again.";if(f){alert(f);return Mi}if(!confirm(Iw))return"abort";var g=["uname="+this.Wh];if(this.sd)g.push("aid="+this.sd);if(this.Yf)g.push("iid="+this.Yf);var h=["reason="+c,"msg="+ab(e)],i="/lh/flagInappropriate?"+
g.join("&");this.lD.send(i,"POST",h.join("&"));return Li};
var $F='<form id="lhui_form"><p>'+Ay+' <a href="http://picasa.google.com/support/bin/answer.py?answer=44321" target="_blank">'+Qw+"</a></p><h2>"+Pw+'</h2><p><input type="radio" name="reason" value="1" /> '+Ow+'<br /><input type="radio" name="reason" value="2" /> '+Nw+'<br /><input type="radio" name="reason" value="3" /> '+Mw+'<br /><input type="radio" name="reason" value="4" /> '+Lw+'</p><p>If you own the copyright for this work and would like it removed, please see our <a href="http://www.google.com/picasa_web_dmca.html" target="_blank">instructions for notification of copyright infringement</a>.</p><h2>'+
Kw+'</h2><p><textarea name="msg"></textarea></p></form>',ZF=By;XF.prototype.hI=function(a){if(this.Nca==a)return;this.Nca=a;if(a)this.j.g(this.e,E,this.Fp,undefined,this);else this.j.Sa(this.e,E,this.Fp,undefined,this)};var aG=function(a,b){oF.call(this,b);this.af=b.authUser;this.X=a;this.p=new K(this)};
o(aG,oF);aG.prototype.Ba=Fd("gphoto.PhotoEntityView");aG.prototype.Tu=0;aG.prototype.n=function(){aG.f.n.call(this);if(this.Tc)this.Tc.n();this.p.g(this.Ia().Ia(),mg,this.nb);this.p.g(j,Gy,l(this.SP,this,false));this.p.g(j,Hy,l(this.SP,this,true))};
aG.prototype.u=function(){aG.f.u.call(this);this.p.ma();if(this.Tc)this.Tc.u()};
aG.prototype.b=function(){if(!this.W()){aG.f.b.call(this);this.p.b();if(this.Tc)this.Tc.b()}};
aG.prototype.show=function(a){oF.prototype.show.call(this,a);if(a){if(!this.Mc){this.Mc=new Xz;this.Z(this.Mc,true)}this.p.g(this.X,RB,this.qj)}else{this.p.Sa(this.X,RB,this.qj);if(this.Mc){this.removeChild(this.Mc,true);delete this.Mc}}};
aG.prototype.C=function(a){aG.f.C.call(this,a)};
aG.prototype.SP=function(a){if(this.k){if(!a)this.Tu++;else if(this.Tu>0)this.Tu--;this.Mc.eE(this.Tu==0)}};
aG.prototype.qj=function(){var a=this.X.xd();this.Ba.Zb("showing item: "+a);if(a<0)this.U(null);else this.Q.ua(a,1,l(this.Wp,this,false))};
aG.prototype.Wp=function(a,b,c){var d=this.X.xd();this.Ba.Zb("entityLoaded: "+c+", myIndex: "+d);var e=b.Me(c);if(a&&e){this.Ba.Zb("Prefetching: "+c);var f=new ne(e.w,e.h);if(this.Qi)ss(this.Qi);this.Qi=rs(e.Rf(),f,this.Mc.zJ(f),false,null)}else if(d==c){if(!e){this.me().xL();return}this.U(e)}};
aG.prototype.U=function(a,b){aG.f.U.call(this,a);if(!b)this.aa();if(!this.k||!this.Mc)return;if(cA&&_features.fr&&_features.froptin)jA(this,this.Mc.Le(),this.k.id,null,true,this.isOwner(this.k)?"editor":kA,false)};
aG.prototype.aa=function(){aG.f.aa.call(this);this.nb();this.Mc.U(this.k);var a=this.X.xd()+this.X.WI();if(a>=0&&a<this.Q.Yb())this.Q.ua(a,1,l(this.Wp,this,true))};
aG.prototype.nb=function(){if(this.Mc){var a=Qe(),b=vh(this.A());this.Mc.jP(a.height-b);this.Mc.aa()}};var bG=function(a){P.call(this);this.X=a.appContext.yd();this.p=new K(this);this.eb=a;this.Eh=new this.eb.feedListClass(this.X,this.eb);this.mn=new this.eb.entityDetailClass(this.X,this.eb);this.qca=new Ym(this.B8,1,this);this.Z(this.mn);this.Z(this.Eh);if(this.eb.maxPageSize!=-1){this.mr=new XB(this.X,10);this.Z(this.mr)}},
cG;o(bG,P);bG.prototype.Ba=Fd("gphoto.BasicFeedView");bG.prototype.X1=1;bG.prototype.xC=cG;bG.prototype.ae=function(a){var b=this.Q!=a;this.Q=a;if(b)this.eX()};
bG.prototype.Wa=function(){return this.Q};
bG.prototype.eX=function(){this.p.g(this.Q,Es,function(){this.X.ym(this.Q.Yb())});
this.X.ym(this.Q.Yb());if(!this.$B)this.$B=new WE(this.Q,this.X);else this.$B.ae(this.Q);this.Eh.U(this.$B);this.mn.ae(this.Q);if(cA&&_features.fr)this.B=rr()};
bG.prototype.n=function(){P.prototype.n.call(this);this.p.g(this.Eh,kg,this.rv);this.p.g(this.Eh,lg,this.G7);this.p.g(this.X,PB,this.fN)};
bG.prototype.u=function(){this.p.ma();P.prototype.u.call(this)};
bG.prototype.fN=function(){if(this.mr)this.mr.show(this.Eh.Ja()&&this.X.tq()>1)};
bG.prototype.b=function(){this.p.b();P.prototype.b.call(this)};
bG.prototype.G7=function(){var a=this.Eh.oe()[0];if(a){var b=a.Vp;this.X.Tg(b)}};
bG.prototype.Fl=function(){var a=this.X.xd();return this.Q.Me(a)};
bG.prototype.rv=function(){var a=this.Eh.oe()[0];this.ID(a.na(),true);this.dispatchEvent("itemactivated")};
bG.prototype.Ur=function(a){this.X1=a-this.xC;this.xC=a;this.qca.start()};
bG.prototype.B8=function(a){var b=a||this.xC;b=pe(b,0,As);this.Eh.Ur(b)};
bG.prototype.C=function(a){P.prototype.C.call(this,a);this.mn.C(a);this.Eh.C(a);if(this.mr){this.mr.C(a);this.mr.aa()}};
bG.prototype.aa=function(){this.mn.aa()};
bG.prototype.cE=function(a){if(!this.Eb)return;if(a){this.mn.show(false);this.Eh.show(true)}else{this.Eh.show(false);this.mn.show(true);if(!this.Gd)this.Gd=Te();this.Gd.scrollTo(0,0)}this.fN()};
bG.prototype.ID=function(a,b){this.Lfa=a;this.mn.U(a,b)};
bG.prototype.isOwner=function(a){if(a&&this.eb&&this.eb.authUser)return a.isOwner(this.eb.authUser);return false};
bG.prototype.u_=function(){this.$B.dispatchEvent("remove")};
bG.prototype.zA=function(){return this.eb};{var dG="owner_id",eG={value:"albums_list"},fG={value:"photos_list"},gG={value:"tags_list",label:Ku},hG={value:"person_filter"},iG={value:"tag_filter",label:vx},jG={value:"search_filter"},lG="uname",mG={name:"isOwner",value:"true"},nG="aid",oG="S",pG="F",qG={name:"psc",value:"C"},rG={name:"psc",value:"G"},sG="tags",tG="sids",uG="optgt",vG="TAGS",wG="q",xG="pagesize",yG="debug",zG="feat",AG={name:"filter",value:"1"},BG=function(a,b,c){J.call(this);this.bba=a;this.wa=b;this.EN=c;this.jN(window.location)};
o(BG,J);BG.prototype.Wh=null;BG.prototype.isOwner=false;BG.prototype.sd=null;BG.prototype.Q=null;BG.prototype.vH=false;BG.prototype.yH=null;BG.prototype.ey=function(){var a=window.location.hash;if(this.yH==a)return;this.yH=a;this.jN(window.location);this.dispatchEvent({type:"changed",context:a})};
BG.prototype.jN=function(a){var b=new N(a);this.Cd=null;this.uo="";this.Wh=b.Ga(lG)||this.bba.ri();if(b.Ga(wG)!=undefined){this.Cd="s";this.uo=CG}this.vH=b.Ga("filter")=="1";this.og=b.Ga(sG);this.Lh=b.Ga(wG);this.zb=Number(b.Ga(xG));this.Iz=!(!b.Ga(yG));if(this.Lh){this.searchContext=b.Ga("psc");if(this.searchContext==undefined)this.searchContext=_authuser.isOwner?oG:(_user.name?pG:rG.value)}if(this.vE(b,qG)){if(this.og){this.Cd="ft";this.uo=DG}}else if(this.vE(b,rG)){if(this.og){this.Cd="ct";this.uo=
DG}}else{this.isOwner=this.vE(b,mG)&&_authuser.isOwner;this.sd=b.Ga(nG);this.Yo=b.Ga(tG);if(this.og){this.Cd=this.sd?"at":(this.isOwner?"gt":"pgt");this.uo=DG}else if(this.Yo){this.Cd=this.sd?"ap":(this.isOwner?"gp":"pgp");this.uo=EG}else if(this.Lh){}else if(_album.name&&ib(b.Nj(),_album.name)){this.sd=_album.id;this.Wh=_user.name;this.isOwner=_authuser.isOwner;this.Cd="a";this.uo=_user.nickname+" - "+_album.title}}var c=b.JA();if(c&&c.length>0)this.HU=null;else{var d=b.Ga(uG),e=fG;if(d==vG)e=gG;
else if(this.sd==null&&this.og==null)e=eG;this.HU=e}this.Zba=b.Ga(zG)};
BG.prototype.vE=function(a,b){return a.Ga(b.name)==b.value};
BG.prototype.BA=function(){return this.HU};
BG.prototype.kY=function(){if(this.og)return iG.label(p(this.og));else if(this.Yo)return"";else if(this.Lh)return"search results for "+p(this.Lh)};
BG.prototype.mf=function(){return this.Cd};
BG.prototype.ae=function(a){this.Q=a};
BG.prototype.Wa=function(){if(!this.Q)if(this.Cd=="a")this.Q=this.wa.VX(this.EN,this.Wh,_album);else if(this.Lh){this.Q=this.wa.ZJ(this.Lh);var a=this.Q.bc();a.SO(this.searchContext);a.TO(this.Wh);a.Qr(this.vH)}else if(this.og){var a=this.wa.VA(undefined,undefined,undefined,undefined,this.EN);this.Q=new lt(this.wa,a)}else this.Q=this.wa.OJ(this.EN);return this.Q};
BG.prototype.Cg=function(){var a=this.BA(),b=null;if(!a){var c=window.location.hash.match(/#(\d+)/);if(c&&c.length==2){var d,e;if(this.Cd!="s"){d=this.Wh;e=this.sd}b=this.wa.MJ(c[1],d,e)}}return b};
BG.prototype.dB=function(){return this.Wh};
BG.prototype.lf=function(){return this.wa.Ne(this.Wh,null)};
BG.prototype.NY=function(){return!(!this.isOwner)};
BG.prototype.q1=function(){return this.sd!=null};
BG.prototype.dq=function(){return this.sd};
BG.prototype.ti=function(){return this.Lh||this.og};
BG.prototype.HZ=function(){return this.Yo||""};
BG.prototype.wZ=function(){return this.Yo?0:(this.og?1:2)};
BG.prototype.uZ=function(){if(this.searchContext){if(this.searchContext==qG.value)return 2;else if(this.searchContext==pG)return 3;else if(this.searchContext==oG)return 4}else if(this.Cd=="at")return 0;else if(this.Cd=="ft")return 2;else if(this.Cd=="gt")return 4;else if(this.Cd=="pgt")return 3;return 1};
BG.prototype.Yc=function(){return this.zb};
BG.prototype.H2=function(){return this.Iz};
BG.prototype.Td=function(){return this.uo};
var CG=m("Search results"),DG=m("Tag results"),EG=m("People results");BG.prototype.yY=function(){return this.Zba}};{var FG=function(a){P.call(this);this.Qb=a;F(this.Qb,"changed",this.Lc,true,this);Y("slideshow",function(){new oB})};
o(FG,P);FG.prototype.Lc=function(){this.Qg()};
FG.prototype.Qg=function(){var a=m("My Public Gallery"),b=[],c={gallery:true,favorites:true,community:true,faces:false};this.pC=null;this.oC=[];var d=null,e=null,f=this.Qb.mf();if(f=="pgt"){b.push({content:Nx(_user.nickname),link:_user.link});c.gallery=false}else if(f=="gt"){var g=new N(window.location),h="http://"+g.ul();h+=g.Cq()?":"+g.Dl():"";h+="/home";b.push({content:Gu,link:h});c.gallery=false}else if(f=="at"){b.push({content:Xy(_album.title),link:_album.link});d=_album.largepreview}else if(f==
"ft"){var g=new N(window.location),h="http://"+g.ul();h+=g.Cq()?":"+g.Dl():"";h+="/lh/favorites";b.push({content:Fu,link:h});c.favorites=false}else if(f=="ct"){b.push({content:Hu});c.community=false}else if(f=="pgp"){b.push({content:Nx(_user.nickname),link:_user.link});b.push({content:py,link:"/lh/people?uname="+_user.name});d=_subject.thumb;e=" lhcl_sp_iconicShape";c={}}else if(f=="gp"){b.push({content:oy,link:"/lh/people?uname="+_user.name+"&isOwner=true"});d=_subject.thumb;e=" lhcl_sp_iconicShape";
c={}}else if(f=="ap"){b.push({content:_album.title,link:_album.link});d=_subject.thumb;e=" lhcl_sp_iconicShape";c={faces:true}}else if(f=="a"){b.push({content:_album.title,link:_album.link});d=_album.largepreview;c={}}else if(f=="s"){if(this.Qb.searchContext==qG.value){c.favorites=false;var g=new N(window.location),h="http://"+g.ul();h+=g.Cq()?":"+g.Dl():"";h+="/lh/favorites";b.push({content:Fu,link:h})}else if(this.Qb.searchContext==rG.value){c.community=false;b.push({content:Hu})}else if(this.Qb.searchContext==
oG){c.gallery=false;b.push({content:Gu,link:_user.link})}else b.push({content:Nx(_user.nickname),link:_user.link});if(!_authuser.name)c.gallery=false;if(this.Qb.Lh.length>0){var i=p(this.Qb.Lh),k=m("More photos about {$tag} in: ",{tag:i});this.pC=k}else c={}}var n=this.Qb.kY();if(n)if(!this.Qb.BA()){var q=window.location.href.toString();q=q.replace(/#.*$/,"#");b.push({content:n,link:q});c={}}else{b.push({content:n});if(this.Qb.og){var i=p(this.Qb.og),t=m("More photos tagged {$tag} in: ",{tag:i});
this.pC=t}}GG(b,d,this.Qb.isOwner);if(e){var y=u("lhid_cover_id");y.className+=e}if(this.Qb.isOwner&&this.Qb.Yo)this.ZR();var z={type:0};if(this.Qb.og)z={type:iG,value:this.Qb.og};else if(this.Qb.Yo)z={type:hG,value:this.Qb.Yo};else if(this.Qb.Lh)z={type:jG,value:this.Qb.Lh};if(c.gallery){var O,aa;if(this.Qb.mf()=="s"){O=Gu;aa=_authuser.name}else O=this.Qb.Wh==_authuser.name?a:Nx(_user.nickname),aa=_authuser.name?_authuser.name:_user.nickname;this.Ns(O,HG([{type:dG,value:aa},z]))}if(c.favorites&&
_authuser.user)this.Ns(uu,HG([{type:dG,value:_authuser.name},{type:qG},z]));if(c.community)this.Ns(tu,HG([{type:dG},{type:rG},z]));if(c.faces){var dc=this.Qb.isOwner?"View photos of "+n+" in your Gallery":"View photos of "+n+" in "+_user.nickname+"'s Gallery",Re=[{type:dG},z];if(this.Qb.isOwner)Re.push({type:mG});this.Ns(dc,HG(Re))}this.E6()};
FG.prototype.Ns=function(a,b){this.oC.push({text:a,link:b})};
FG.prototype.E6=function(){var a=u("lhid_more");if(!this.oC.length){a.innerHTML="";return}var b=[];for(var c=0;c<this.oC.length;c++){var d=this.oC[c];b.push('<a href="'+d.link+'">'+d.text+"</a>")}a.innerHTML=this.pC?this.pC+b.join("&nbsp;|&nbsp;"):b.join("&nbsp;|&nbsp;")};
var HG=function(a){var b=new N(window.location),c="http://"+b.ul();c+=b.Cq()?":"+b.Dl():"";c+=b.Nj();var d=[];for(var e=0;e<a.length;e++){var f=a[e].type;switch(f){case dG:var g=a[e].value?a[e].value:b.Ga(lG);if(g!=undefined)d.push(lG+"="+g);break;case "album_id":var h=a[e].value?a[e].value:b.Ga(nG);d.push(nG+"="+h);break;case gG:d.push(uG+"="+vG);break;case iG:d.push(sG+"="+a[e].value);break;case hG:d.push(tG+"="+a[e].value);break;case jG:d.push(wG+"="+a[e].value);break;case mG:case qG:case rG:case AG:d.push(f.name+
"="+f.value);break}}if(d.length>0)c+="?"+d.join("&");return c},
GG=function(a,b){var c=u("lhid_context");of(c,"lhcl_newmeta");var d=[];for(var e=0;e<a.length;e++)if(a[e].link)d.push('<a href="'+a[e].link+'" style="text-decoration:none;">'+a[e].content+"</a>");else d.push(a[e].content);var f='<span class="lhcl_title">'+d.join("&nbsp;&rsaquo;&nbsp;")+"</span>",g='<div id="lhid_subcontext" class="lhcl_meta_description"></div>',h=["<table cellspacing=0 cellpadding=0><tr><td>"];if(b)h.push('<img id="lhid_cover_id" class="lhcl_metathumbnail" src="',b,'"></td><td>',
f);else h.push("<div>",f,"</div>");h.push(g);h.push("</td></tr></table>");c.innerHTML=h.join("")};
FG.prototype.ZR=function(){if(_subject==null)return;var a=u("lhid_subcontext"),b={},c=[];c.push(_subject.name);if(_subject.email&&_subject.email.length>0)c.push(_subject.email);if(_subject.phone&&_subject.phone.length>0)c.push(_subject.phone);b.info=c.join("&nbsp;&nbsp;");b.favorite=_subject.starlink?"&nbsp;"+IG(_subject.starlink,fx,"img/favorite_gray.gif",true):"";var d=gz("<div><table><tr><td>{info}</td><td>{favorite}</td></tr></table></div>",b);a.appendChild(d)};
var JG=function(a){if(a)GG([{content:a}])},
IG=function(a,b,c,d,e){var f="<td><a "+(e?'id="'+e+'" ':"")+'href="'+a+'" style="text-decoration:none">',g="</a></td>",h=c?'<img src="'+c+'" style="height:11px">':"";return d?f+'<div class="lhcl_cbbox lhcl_cbox_disabled"><p class="lhcl_cbdesc"></p><p class="lhcl_cblink"><em>'+h+"<span>"+b+"</span></em></p></div>"+g:f+h+'<span style="padding:4px; font-size:0.8em;">'+b+"</span>"+g}};{var nD=function(){P.call(this)};
o(nD,P);nD.prototype.D=function(){return"gphoto.context.Display"};
nD.prototype.m=function(){nD.f.m.call(this);of(this.A(),"gphoto-context-display")};
nD.prototype.J=function(a){this.Jh(true);this.Cba=a;if(a)this.Z(a,true);if(this.Ha())M(this.A(),!(!a))};
nD.prototype.mY=function(){return this.Cba}};{var KG=function(a){P.call(this);this.Cz=a||[]};
o(KG,P);KG.prototype.D=function(){return"gphoto.context.BreadCrumb"};
KG.prototype.n=function(){KG.f.n.call(this);var a=this.Cz.length-1;r(this.Cz,function(b,c){if(c>0)x(this.A(),v("span",{"class":"gphoto-context-separator"},">"));var d;d=b[1]?v("a",{href:b[1]}):w("span");x(this.A(),d);if(b[2])d.innerHTML=b[0];else B(d,b[0]);if(c==a)of(d,"gphoto-context-current")},
this)};
KG.prototype.Jy=function(a,b){this.Cz.push([a,b])};
KG.prototype.Ks=function(a){r(a,function(b){var c=b.length>1&&b[1]?b[1]:null;this.Jy(b[0],c)},
this)};
KG.prototype.Rm=function(a,b){this.Cz.push([a,b,true])}};{var LG=function(a,b){P.call(this);this.cg=a||"";this.zM=[];if(b)this.Xk=b};
o(LG,P);LG.prototype.Xk="gphoto-context-toggle";LG.prototype.D=function(){return"gphoto.context.Toggle"};
LG.prototype.m=function(){this.od(w("span"));of(this.A(),this.Xk)};
LG.prototype.n=function(){LG.f.n.call(this);if(this.cg)B(this.A(),this.cg+"\u00a0");r(this.zM,function(a,b){if(b>0)this.A().appendChild(v("span",{"class":"gphoto-context-separator"},"|"));var c;c=a[0]==this.Db?w("b"):v("a",{href:a[1]});B(c,a[2]);this.A().appendChild(c)},
this)};
LG.prototype.rg=function(a,b,c){this.zM.push([a,b,c])};
LG.prototype.ce=function(a){this.Db=a};
LG.prototype.m4=function(){return this.zM.length}};{var MG=function(a,b,c,d){P.call(this);this.Fk=a;this.nba=b;this.mba=c;this.Bca=d};
o(MG,P);MG.prototype.D=function(){return"gphoto.context.Album"};
MG.prototype.u=function(){MG.f.u.call(this);if(this.Aba)H(this.Aba)};
MG.prototype.n=function(){MG.f.n.call(this);var a=new KG;a.Jy(this.nba,this.mba);var b=p(this.Fk),c=this.Bca?"<div />"+b:b;a.Rm(c);this.Z(a,true)}};{var NG=function(a,b,c,d,e,f,g){P.call(this);this.Mb=a;this.rG=b;this.Ada=c;this.hQ=d||"";this.w$=e||"";this.WG=f||"";this.vU=g||""};
o(NG,P);NG.prototype.Zu=null;NG.prototype.D=function(){return"gphoto.context.PeopleSearch"};
NG.prototype.m=function(){var a;if(this.Mb.length==1){a=w("table");var b=w("tbody"),c=w("tr");this.Zu=w("td");this.Sx=v("td",{width:"100%"});x(a,b);x(b,c);x(c,this.Zu);x(c,this.Sx)}else{a=w("div");this.Sx=a}this.od(a)};
NG.prototype.n=function(){NG.f.n.call(this);if(this.Zu){this.sa=v("img",{src:this.Mb[0].thumb,"class":"gphoto-person-icon"});this.Zu.appendChild(this.sa)}var a=new KG;a.Ks(this.rG);a.Jy(py,this.Ada);a.Rm(Ng(this.GZ()));this.Z(a);a.C(this.Sx);if(this.hQ){var b=v("span",{"class":"gphoto-context-search-toggle",style:"line-height:1em"});if(this.w$){var c=v("a",{href:this.w$});c.innerHTML=Ng(p(this.hQ));x(b,c)}else B(b,this.hQ);x(this.Sx,b)}if(this.WG){b=v("div",{"class":"gphoto-context-search-toggle",
style:"line-height:2em"});if(this.vU){c=v("a",{href:this.vU});c.innerHTML=Ng(p(this.WG));x(b,c)}else B(b,this.WG);x(this.Sx,b)}};
NG.prototype.GZ=function(){var a=gc(this.Mb,function(b){return b.dispname});
return p(a.join(", "))}};{var QG=function(a,b,c,d,e,f){P.call(this);this.ze=a;this.sO=b;this.da=c;this.Zw=[];this.Eca=d;if(b==0)this.Zw.push(f,e[0]);else if(b==2)this.Zw.push([OG,e[0]]);else if(b==3)this.Zw.push([f,e[0]]);else if(b==4)this.Zw.push([PG,e[0]])};
o(QG,P);var RG=m("Community photos"),OG=m("Favorites"),PG=m("My Photos");m("People");QG.prototype.D=function(){return"gphoto.context.Search"};
QG.prototype.m=function(){this.od(v("table"));var a=w("tbody"),b=w("tr");this.Vx=w("td");this.uz=w("td");x(this.A(),a);x(a,b);x(b,this.Vx);x(b,this.uz);L(this.Vx,"width","100%");of(this.uz,"gphoto-context-search-count")};
QG.prototype.n=function(){QG.f.n.call(this);var a=Ng(p(this.ze)),b=w("span"),c=new KG;c.Ks(this.Zw);if(this.da==1){var d=m("photos tagged {$formatStart}{$query}{$formatEnd}",{query:a,formatStart:"<b>",formatEnd:"</b>"}),e=m('More photos tagged "{$formatStart}{$query}{$formatEnd}" in:',{query:a,formatStart:"<b>",formatEnd:"</b>"});c.Rm(d);b.innerHTML=e}else{var f=m("Search results for {$formatStart}{$query}{$formatEnd}",{query:a,formatStart:"<b>",formatEnd:"</b>"}),g=m('More results for "{$formatStart}{$query}{$formatEnd}" in:',
{query:a,formatStart:"<b>",formatEnd:"</b>"});c.Rm(f);b.innerHTML=g}var h=new LG(" "),i={type:this.da==2?jG:iG,value:ab(this.ze)};if(this.Eca){if(this.sO!=4){var k={type:dG,value:_authuser.name},n={type:mG,value:"true"};h.rg(4,HG([k,i,n]),PG)}if(this.sO!=2){var k={type:qG};h.rg(2,HG([k,i]),OG)}}if(this.sO!=1){var k={type:rG},q={type:AG};h.rg(1,HG([k,q,i]),RG)}this.Z(c);c.C(this.Vx);if(h.m4()){x(this.Vx,b);this.Z(h);h.C(this.Vx);of(h.A(),"gphoto-context-search-toggle")}this.cO()};
QG.prototype.xP=function(a,b,c){this.oa=a;this.zb=b;this.vQ=c;if(this.Ha())this.cO()};
QG.prototype.cO=function(){if(this.zb&&this.vQ&&Ba(this.oa)){var a=Cz(1,this.oa+1),b=Cz(1,this.vQ),c=Cz(1,Math.min(this.oa+this.zb,this.vQ));m("{$start} {$dash} {$end} of {$total}",{start:a,dash:"&#8211;",end:c,total:b});this.uz.innerHTML=ox(a,c,b)}else We(this.uz)}};{var SG=function(a,b,c,d,e){P.call(this);this.Ue=a;this.a6=b;this.Fb=c;this.fc=d;this.Da=e};
o(SG,P);SG.prototype.D=function(){return"gphoto.context.People"};
SG.prototype.ce=function(a,b){this.Ue=a;this.a6=b;this.aa()};
SG.prototype.n=function(){SG.f.n.call(this);this.aa()};
SG.prototype.WU=function(){var a=new KG,b=[];if(this.Ue==1&&this.Da){if(this.Fb)b.push([ty,"/home"]);else{var c=lb(ux(this.fc.nickname));b.push([c,this.fc.link])}var d=lb(this.Da.title);b.push([d,this.Da.link])}else if(!this.Fb){var c=lb(ux(this.fc.nickname));b.push([c,this.fc.link])}a.Ks(b);if(!this.Fb||this.Ue==1){var e="<div />"+py;a.Rm(e)}return a};
SG.prototype.aa=function(){this.Jh();We(this.A());this.Z(this.WU(),true);var a=new N("/lh/people");a.fa("uname",this.fc.name);if(this.Ue==1){if(this.Fb)a.fa("isOwner","true");x(this.A(),v("a",{href:a.toString()},zy(this.fc.nickname)))}else if(this.Fb){var b=m("All"),c=m("Public"),d=m("View:"),e=w("table"),f=w("tbody"),g=w("tr"),h=v("td",{width:"50%",align:"left"}),i=v("td",{width:"50%",align:"right"}),k=a.toString();a.fa("isOwner","true");var n=a.toString(),q=new N("/lh/favorites"),t=new LG("","gphoto-context-people-favorites-toggle");
t.rg(0,n,Fy);t.rg(2,q.toString()+"#",uu);t.ce(0);this.Z(t,false);t.C(h);t=new LG(d+" ");t.rg(false,n,b);t.rg(true,k,c);t.ce(this.a6);this.Z(t,false);t.C(i);x(this.A(),e);x(e,f);x(f,g);x(g,h);x(g,i)}}};{var oD=function(a,b,c){P.call(this);this.Db=a;this.yw=b;this.rG=c||[]};
o(oD,P);oD.prototype.D=function(){return"gphoto.context.Favorites"};
oD.prototype.n=function(){oD.f.n.call(this);this.Ze()};
oD.prototype.ce=function(a){if(this.Db!=a){this.Db=a;this.Ze()}};
oD.prototype.EW=function(){var a=m("Linked Galleries"),b=new KG;b.Ks(this.rG);var c="<div />"+a;b.Rm(c);this.Z(b,true)};
oD.prototype.DW=function(){var a=w("table");x(this.A(),a);var b=w("tbody");x(a,b);var c=w("tr");x(b,c);var d=v("td",{width:"50%",align:"left"});x(c,d);var e=v("td",{width:"50%",align:"right"});x(c,e);var f=new LG("","gphoto-context-people-favorites-toggle");if(_features.froptin){var g=new N("/lh/people");g.fa("uname",this.yw);g.fa("isOwner","true");f.rg(0,g.toString(),Fy)}var h=new N(window.location.href);f.rg(1,h.toString(),uu);f.ce(1);this.Z(f,false);f.C(d);var i=m("View:");h.Oo("list");var k=h.toString();
h.Oo("activity");var n=h.toString();f=new LG(i+" ");f.rg(0,k,cv);f.rg(1,n,bv);f.ce(this.Db);this.Z(f,false);f.C(e)};
oD.prototype.Ze=function(){this.Jh();We(this.A());if(this.Db==2)this.EW();else this.DW()}};{var TG=function(a){P.call(this);this.Xp=a;this.gb=a.AY();this.X=a.Mn();this.p=new K(this);this.ob=new Dg(document);this.lg=new nm;this.V9=u("lhid_thumbsizes_container");this.iH={};this.kc=[];this.zB=u("lhid_indexbox");this.mG=u("lhid_backlink");this.lG=u("lhid_backlink_msg");this.Gw=this.np("previous",true);this.Ki=this.np("next",true);this.oD=this.np("rotateleft",true);this.pD=this.np("rotateright",true);this.qy=this.np("zoom",true);this.Iq=false;this.Z(this.lg);this.gW()};
o(TG,P);TG.prototype.At=0;TG.prototype.np=function(a){var b=new Sk;b.Jd("lhcl_"+a);this.Z(b);return b};
TG.prototype.Ok=function(a,b){var c=this.iH[a.H()];if(!c){c=[];this.iH[a.H()]=c}c.push(b)};
TG.prototype.$=function(a){P.prototype.$.call(a);var b=u("lhid_thumbsizes");if(b)this.lg.ia(b);else this.lg.C(a);u("lhid_leftnav");var c=u("lhid_midnav"),d=u("lhid_rightnav");this.Gw.C(c);this.Ki.C(c);this.qy.C(d);this.pD.C(d);this.oD.C(d)};
TG.prototype.n=function(){P.prototype.n.call(this);this.lg.Vr(0);this.lg.Qo(As);this.lg.qa(3);this.lg.Rh(1);this.lg.Z7(1);this.lg.p9(1);this.lg.Q8(false);var a=this.lg;this.p.g(this.lg,yk,function(){this.gb.Ur(a.L())});
this.p.g(this.Gw,xk,this.FM);this.p.g(this.Ki,xk,this.EM);this.p.g(this.oD,xk,this.d7);this.p.g(this.pD,xk,this.e7);this.p.g(this.qy,xk,this.Xx);this.p.g(this.Xp,UG,this.yM);this.p.g(this.X,SB,this.CE);this.p.g(this.X,RB,this.CE);this.p.g(this.ob,"key",this.k0);this.p.g(j,Gy,l(this.bA,this,false));this.p.g(j,Hy,l(this.bA,this,true));this.yM()};
TG.prototype.k0=function(a){if(!VG(a)&&this.At==0)switch(a.keyCode){case 37:this.FM();a.preventDefault();break;case 39:this.EM();a.preventDefault();break}};
var VG=function(a){if(a.altKey||a.ctrlKey||a.metaKey)return true;var b=a.target.nodeName.toLowerCase();if(b=="button"||b=="input"||b=="select"||b=="textarea")return true;return false};
TG.prototype.u=function(){this.p.ma();var a=u("lhid_thumbsizes");if(a)this.lg.b();P.prototype.u.call(this)};
TG.prototype.P8=function(a,b){var c=this.iH[a.H()];if(c){var d=0;for(var e=0;e<c.length;++e){var f=c[e];if(f&b)++d}var g=d==c.length;M(a.e,g)}};
TG.prototype.yM=function(){var a=this.Xp.Fg();if(a==0)return;var b=!(!(a&4));this.bA(b);this.CE();this.sh(l(function(g){this.P8(g,a)},
this));if(this.zB)M(this.zB,b);if(this.mG&&this.lG){if(b){var c=window.location.toString();this.mG.href=c.replace(/#.*$/,"#");var d=this.Xp.me().Rb();if(d.mf()!="a"){var e=m("Back to all results");B(this.lG,e)}else{var f=m("View album");B(this.lG,f)}}M(this.mG,b)}if(this.Iq)this.Xx();if(this.V9)M(this.V9,!b)};
TG.prototype.we=function(a){if(this.Iq)this.Xx();var b=this.X.xd(),c=this.X.Kj(),d=pe(b+a,0,c-1);if(b!=d)this.X.Tg(d)};
TG.prototype.CE=function(){var a=this.X.xd();this.NW(a);if(this.zB)B(this.zB,rx(a+1,this.X.Kj()))};
TG.prototype.gW=function(){this.Gw.Ca(false);this.Ki.Ca(false)};
TG.prototype.bA=function(a){if(a&&this.At>0)this.At--;else if(!a)this.At++};
TG.prototype.NW=function(a){this.Gw.Ca(a>0);this.Ki.Ca(a+1<this.X.Kj())};
TG.prototype.FM=function(){this.we(-1)};
TG.prototype.EM=function(){this.we(1)};
TG.prototype.d7=function(){this.pO("ROTATE270")};
TG.prototype.e7=function(){this.pO("ROTATE90")};
TG.prototype.pO=function(a){if(this.Iq)this.Xx();if(this.gb.Fl().Gb())return;var b=this.gb.Fl(),c=["selectedphotos="+b.id,"optgt="+a,"uname="+b.user.name,"aid="+b.album.id,"noredir=true"],d=_selectedPhotosPath+"&"+c.join("&");Gj(d,l(TG.prototype.IK,this),"POST")};
TG.prototype.Xx=function(){if(this.Iq){M(u("lhid_zoom"),false);this.qy.r6("lhcl_zoom_on");this.Xp.dispatchEvent("zoomout")}else if(!this.gb.Fl().Gb()){this.qy.Jd("lhcl_zoom_on");var a=u("lhid_zoom");B(a,Cy);M(a,true);this.Xp.dispatchEvent("zoomin")}this.Iq=!this.Iq};
TG.prototype.IK=function(){var a=this.gb.Fl();a.reload(l(TG.prototype.HK,this))};
TG.prototype.HK=function(){this.Xp.dispatchEvent(mg)}};var WG=function(a,b,c,d,e,f){this.tT=a;this.GU=d;this.p=new K(this);this.wa=new mt;this.af=this.wa.Ne(a,b);this.af.email=c;var g;if(e){this.r=e.feedUrl||"";this.Mb=e.subjects||[];this.Ld=e.albums||[];this.Zea=e.albumStats||[];if(e.owner){var h=e.owner;g=h.name;this.wa.Ne(g,h.nickname)}}if(g&&f){this.tt=this.wa.gd(g,f.id,f.title,f.name);this.tt.J8(f.link);this.tt.G8(f.kmlLink);this.tt.M8(f.mapLink);this.tt.q9(f.uploadlink);this.tt.H8(f.largepreview)}this.Qb=new BG(this,this.wa,this.r);var i=null;if(s&&
(window.location.hash==""||window.location.hash=="#")){var k="lhid_historyiframe";document.write('<iframe id="'+k+'" style="display:none"></iframe>');i=u(k);i.id="";var n=ef(i);n.open("text/html");n.write("<title>"+p(window.document.title)+"</title><body></body>");n.close()}this.dL=new $m(false,null,null,i);this.Ba=Fd("gphoto.AppContext");this.iW=[];this.X=new OB(0,24);this.X.Tg(0);Jd().ox(Bd);no();this.p.g(this.dL,"navigate",this.uC);this.kda=e.objectionEmail};
o(WG,J);WG.prototype.JH=null;WG.prototype.p6=function(a,b){this.iW.push(new XG(a,b))};
WG.prototype.j8=function(a){this.JH=new XG(a,null)};
WG.prototype.start=function(){this.Ba.Zb("Starting app");this.dL.Ca(true)};
WG.prototype.Th=function(a){this.dL.Th(a)};
WG.prototype.ri=function(){return this.tT};
WG.prototype.KI=function(){if(!this.af)this.af=this.wa.Ne(this.tT,"");return this.af};
WG.prototype.qe=function(){return this.Mb};
WG.prototype.Gn=function(){return this.Ld};
WG.prototype.bY=function(){return Ix};
WG.prototype.Oa=function(){return this.GU};
WG.prototype.Rb=function(){return this.Qb};
WG.prototype.CA=function(){return this.wa};
WG.prototype.yd=function(){return this.X};
WG.prototype.cZ=function(){return this.kda};
WG.prototype.xL=function(){var a=m("The requested photo is invalid or unavailable. You will be redirected momentarily."),b=window.location.toString().replace(/#.*$/,"");Jz(a,1000,function(){window.location.replace(b)})};
WG.prototype.uC=function(a){this.Ba.Zb("nav to #"+a.token);var b=lc(this.iW,function(c){return c.test(a.token)});
if(!b)b=this.JH;if(this.f5&&this.hn&&this.f5==b.vpage)this.hn.aa(a.token);else{if(this.hn)this.hn.b();this.hn=new b.vpage(this);this.hn.C(this.GU);this.hn.aa(a.token)}document.title=lb(this.hn.Td());this.f5=b.vpage};
var XG=function(a,b){this.vpage=a;this.re=b};
XG.prototype.test=function(a){return this.re&&this.re.test(a)};var YG=function(a){P.call(this);this.V("Created a thingy of type "+this.D());this.T=a;this.j=new K(this)};
o(YG,P);YG.prototype.b=function(){if(!this.W()){YG.f.b.call(this);this.j.b()}};
YG.prototype.D=function(){return"gphoto.BasePage"};
YG.prototype.me=function(){return this.T};
YG.prototype.Wa=function(){return this.T.Rb().Wa()};
YG.prototype.Td=function(){return""};
YG.prototype.n=function(){YG.f.n.call(this);var a=this.Wa();if(a){this.rK();this.j.g(a,Es,this.rK)}};
YG.prototype.u=function(){YG.f.u.call(this);this.j.ma()};
YG.prototype.aa=function(){};
YG.prototype.rK=function(){var a=this.Wa();if(a){var b=a.Yb();this.me().yd().ym(b)}};
YG.prototype.V=function(a){if(!this.Ba)this.Ba=Fd(this.D());this.Ba.Zb(a)};var ZG=function(a){YG.call(this,a);this.Zj=[];this.Mc=new Xz;this.Z(this.Mc);this.Df=new Ym(this.$S,0,this)};
o(ZG,YG);ZG.prototype.GM=1;ZG.prototype.xe=3000;ZG.prototype.D=function(){return"gphoto.SlidePage"};
ZG.prototype.Td=function(){var a=m("Slideshow");return a};
ZG.prototype.m=function(){ZG.f.m.call(this);var a=document.body;this.$da=rh(a,"backgroundColor");L(a,"background","#000");var b=this.hK();this.aea=b.style.overflow;L(b,"overflow","hidden");var c=$e(document.body);while(c){if(c.style.display!="none"){M(c,false);this.Zj.push(c)}c=af(c)}this.Mc.C();this.Cb=new aA(this);this.Cb.Hs(this.xe)};
ZG.prototype.n=function(){ZG.f.n.call(this);this.j.g(this.me().yd(),RB,this.o_);this.j.g(this.Mc,"load",this.Il);this.j.g(window,mg,this.nb);this.j.g(document,gg,this.Wf);Y("f",l(this.IR,this));this.vn(true)};
ZG.prototype.u=function(){ZG.f.u.call(this);this.vn(false);this.Cb.ha();while(this.Zj.length)M(this.Zj.pop(),true);L(document.body,"background",this.$da);L(this.hK(),"overflow",this.aea);Z("f")};
ZG.prototype.b=function(){if(!this.W()){ZG.f.b.call(this);oc(this.Zj)}};
ZG.prototype.U=function(a){ZG.f.U.call(this,a);this.nb();this.Mc.U(a)};
ZG.prototype.aa=function(a){this.T.Rb().ey();var b=a.match(/^slideshow(\/\d*)?$/)[1],c=0;if(za(b)&&b.length>1){b=b.substr(1);var d=this.Wa().vA();for(var e=0;e<d.length;e++)if(d[e].id==b){c=e;break}}this.aM(c)};
ZG.prototype.hK=function(){return s?document.documentElement:document.body};
ZG.prototype.Wp=function(a,b){var c=this.me().yd(),d=c.xd();c.Tg(b);this.V("entityLoaded: "+b+", oldIndex: "+d);if(d==b){var e=a.Me(b);this.U(e)}};
ZG.prototype.o_=function(){var a=this.me().yd().xd();this.V("showing photo at: "+a);if(a<0)this.U(null);else this.aM(a)};
ZG.prototype.aM=function(a){var b=this.Wa();if(b)b.ua(a,1,l(this.Wp,this))};
ZG.prototype.Il=function(){var a=this.Wa(),b=this.me().yd(),c=b.xd(),d=this.na();if(d)this.Cb.yF(d.c);var e=this.GM==1?1:-1,f=qe(c+e,b.Kj());if(a)a.ua(f,1,l(this.L5,this))};
ZG.prototype.L5=function(a,b){if(this.FN){ss(this.FN);this.FN=0}var c=a.Me(b);if(c){var d=c.Rf();if(d){var e=new ne(c.w,c.h),f=this.Mc.zJ(e);this.FN=rs(d,e,f,false,null)}}};
ZG.prototype.we=function(a){var b=this.me().yd(),c=b.xd(),d=qe(c+a,b.Kj());this.GM=d>c?1:0;b.Tg(d)};
ZG.prototype.$S=function(){this.we(1);if(this.k&&!this.k.Gb())this.Df.start(this.xe)};
ZG.prototype.IR=function(a,b){if(a=="stateChanged"){var c=b.split(",");if(c[1].indexOf("completed")>-1)this.Df.start(this.xe)}};
ZG.prototype.vn=function(a){if(a==this.Zd)return;this.Zd=a;if(a)this.Df.start(this.xe);else this.Df.stop();this.Cb.cS(a)};
ZG.prototype.nb=function(){var a=Qe();this.Mc.jP(a.height,true);this.Mc.aa()};
ZG.prototype.Wf=function(a){var b=parseInt(String.fromCharCode(a.keyCode),10);if(!isNaN(b)&&b>=1&&b<=20){this.xe=b*1000;this.Cb.Hs(this.xe);if(this.Zd)this.Df.start(this.xe);return}switch(a.keyCode){case 85:case 27:this.hp();a.preventDefault();case 74:case 37:this.Yh();a.preventDefault();break;case 75:case 39:this.Xh();a.preventDefault();break;case 32:if(this.Zd)this.BF();else this.uF();a.preventDefault();break}};
ZG.prototype.Yh=function(){this.vn(false);this.we(-1)};
ZG.prototype.Xh=function(){this.vn(false);this.we(1)};
ZG.prototype.hp=function(){this.me().Th("")};
ZG.prototype.uF=function(){this.vn(true)};
ZG.prototype.BF=function(){this.vn(false)};
ZG.prototype.jS=function(){this.xe=Math.max(this.xe-1000,1000);this.Cb.Hs(this.xe);if(this.Zd)this.Df.start(this.xe)};
ZG.prototype.iS=function(){this.xe=Math.min(this.xe+1000,20000);this.Cb.Hs(this.xe);if(this.Zd)this.Df.start(this.xe)};var aH=function(a){R.call(this,"gphoto-issueDialog",true);this.T=a;var b=new Xk,c=m("Send"),d=m("Discard");b.J("ok",c,true,false);b.J(dl,d,false,true);this.Rg(b);this.j=new K(this);var e=this.T.KI(),f=e.nickname||e.name,g=m("To"),h=m("Subject"),i=m("Please don't share this with anyone"),k=m("Please don't make this public"),n=m("Please don't display name tags"),q=m("Please contact me if you would like any additional information. Thank you for your consideration."),t=m("Note: This request will be sent from your email address to allow additional correspondence"),
y=m("{$authuser} sent you a message from Google Photos",{authuser:f});this.Uc={form:$G(),thumbnail:$G(),greeting:$G(),message:$G(),ownerNickname:$G(),sender:$G(),signature:$G(),noShare:$G(),noPublic:$G(),noName:$G()};this.hc('<table class="gphoto-mailheader"><tbody><tr><td class="label">'+g+':</td><td class="ownernickname" id="'+this.Uc.ownerNickname+'"></td></tr><tr><td class="label">'+Av+':</td><td class="authuser" id="'+this.Uc.sender+'"></td></tr><tr><td class="label">'+h+':</td><td class="subject">'+
p(y)+'</td></tr></tbody></table><div class="message-area"><div class="preview-area"><div class="thumbnail" id="'+this.Uc.thumbnail+'"></div><div class="message-content"><p class="greeting" id="'+this.Uc.greeting+'"></p><p class="message" id="'+this.Uc.message+'"></p><form id="'+this.Uc.form+'"><ul><li><input name="noshare" id="'+this.Uc.noShare+'" type="checkbox" checked>'+i+'</input></li><li><input name="nopublic" id="'+this.Uc.noPublic+'" type="checkbox">'+k+'</input></li><li><input name="nonametags" id="'+
this.Uc.noName+'" type="checkbox">'+n+'</input></li></ul></form><p class="closing">'+q+'</p><p class="signature" id="'+this.Uc.signature+'"></p></div></div><p class="footer">'+t+"</p></div>")};
o(aH,R);var bH=0,$G=function(a){return"notifyDlg_"+(a?a:"")+bH++};
aH.prototype.n=function(){aH.f.n.call(this);if(!this.DI){var a=this.T.KI(),b=a.nickname||a.name,c=lb(a.email);this.DI=u(this.Uc.form);var d=u(this.Uc.sender);B(d,b+(c?" ("+c+")":""));var e=u(this.Uc.signature);B(e,b);this.et=[];this.et.push(u(this.Uc.noShare));this.et.push(u(this.Uc.noPublic));this.et.push(u(this.Uc.noName));this.H7=this.rl().eY("ok")}this.j.g(this,"dialogselect",this.of);r(this.et,l(function(f){this.j.g(f,[kg,"propertychange"],this.Aaa)},
this))};
aH.prototype.Aaa=function(){var a=hc(this.et,function(b,c){return b+(c.checked?1:0)},
0);if(a>0)this.H7.removeAttribute("disabled");else this.H7.setAttribute("disabled",true)};
aH.prototype.open=function(a){if(!a){var b=m("Error. Invalid item.");alert(b);return}if(!this.Ha())this.C();this.U(a);this.M(true);Iy(this,Gy)};
aH.prototype.U=function(a){this.k=a;var b=this.k.lf(),c=b.nickname||"",d=u(this.Uc.ownerNickname);B(d,c);var e=u(this.Uc.thumbnail),f=u(this.Uc.greeting),g=u(this.Uc.message),h=m("Hi {$owner},",{owner:c});B(f,h);var i,k;if(a.type=="Album"){i=ny;k="aid"}else{i=ly;k="iid";if(a.type=="Photo"&&a.Gb())i=my}if(this.nB)We(this.nB);else{this.nB=v("div",{style:"display: none;"});x(this.DI,this.nB)}x(this.nB,v("input",{type:"hidden",name:k,value:a.id}));var n=m("I would like to make the following request regarding this {$type}.",
{type:i});B(g,n);We(e);var q=a.thumbnail(3),t=v("img",{src:q.url,width:q.width,height:q.height});x(e,t);var y=a.Td(),z=v("div",{"class":"title",title:y});B(z,lb(pb(y,16)));x(e,z)};
aH.prototype.of=function(a){if(a.key=="ok"){a.stopPropagation();var b=zf(this.DI),c=this.T.cZ();Gj(c,null,"POST",b)}this.k=null;Iy(this,Hy)};{var $=function(a){YG.call(this,a);var b=this.T.Rb(),c=Sc(cH),d=b.mf();if(d=="a")c.iconClass=cF;else if(dH(d)){c.maxPageSize=24;c.iconClass=dF;c.iconCssClass="goog-icon-list-searchicon";c.iconCssAttributes=eF}var e=b.Yc();if(!isNaN(e))c.maxPageSize=e;c.debug=b.H2();c.authUser=this.T.ri();c.appContext=a;this.gb=new bG(c);this.tg=new nD;this.cd=new eH(d,this,b);this.jl=new vF(this.gb);this.j.g(this.jl,fu,this.BQ);this.Zp=new YF(this.gb);this.w3=u("lhid_feedview");var f=u("lhid_tools");this.Nd=new yA(this,
this.w3,f,true);this.kz=u("lhid_caption");this.pb=new GA(this,this.kz,this.kz);this.pb.W7(this.T);this.kM=new tF(this.gb);this.Px=new pF(this.gb);this.DM=this.qV();this.Pg=this.ot();this.Z(this.jl);this.Z(this.Zp);this.Z(this.kM);this.Z(this.Px);this.Z(this.tg);this.Z(this.DM);this.Z(this.gb);if(this.hh=="a")this.Z(this.Pg);this.jF=u("lhid_trayhandle");this.kF=u("lhid_tray");if(this.jF&&this.kF){this.py=new Tp(this.jF,this.kF);this.py.expand();this.j.g(this.py,"toggle",this.nb)}this.j.g(window,mg,
this.nb);this.j.g(this.gb,iF,this.maa);if(d=="a"){this.Br=u("lhid_peoplewidget");if(this.Br){this.Br.innerHTML="";var g=m("People in this album");this.qN=new fH(g,gH,hH,iH,b.isOwner,_album);this.Z(this.qN)}if(_album.hasHiddenGeo&&b.isOwner)this.iA=new pE}},
gH,hH,iH;o($,YG);var cH={maxPageSize:-1,feedListClass:hF,iconClass:YE,listCssClass:null,iconCssClass:null,iconCssAttributes:ZE,entityDetailClass:aG,keyboardShortcuts:false,appContext:null},UG="modechanged";$.prototype.sE=false;$.prototype.Sn=null;$.prototype.Fr=null;$.prototype.rd=function(a){var b=a.status;if(!(!(!b))&&a.target&&a.target.nf)b=a.target.nf();if(b<0||b>=400){var c;c=b==403?Nv:(b==404?Mv:(b>=500?Kv+"\n"+ix(b):Lv+"\n"+ix(b)));alert(c);return true}return false};
$.prototype.de=function(a){var b=a.text;if(a.target&&a.target.Sb)b=a.target.Sb();if(b.indexOf("success")<0){var c=b.replace(/\<[^>]*\>/g,"");alert(c);return true}return false};
$.prototype.Lk=function(){var a=this.gb.Fl();a.r=null;a.ph=null;a.Ei(l(function(){this.Nd.EF(a)},
this))};
$.prototype.Ba=Fd("gphoto.FeedViewPage");$.prototype.Db=0;$.prototype.Cf=null;$.prototype.V=function(a){this.Ba.Zb(a)};
$.prototype.ce=function(a){if(a!=this.Db){var b=this.Db;this.Db=a;this.f8();if(this.Ha()){var c=!(!(this.Db&4));this.WQ(!c);this.Nd.show(c);this.kM.show(c);this.Px.show(c);this.jl.show(c);this.JE();var d=null,e=this.Db&2;if(e){var f=this.T.Rb(),g=f.Wa();d=g.bc().Cg()}else d=this.T.Rb().Cg();var h=this.Fb(d);if(d&&!h){if(e)this.Zp.U(d);this.Zp.show(true)}else this.Zp.show(!h&&c);var i=u("lhcl_tagbox");if(i)M(i,!c);var k=u("lhid_frquick");if(k)M(k,!c);if(this.Br)M(this.Br,!c);if(this.Pg)this.Pg.show(!c);
if(this.kz)M(this.kz,c);if(b!=0&&!(!(this.Db&4)))this.Or()}this.dispatchEvent(UG)}};
$.prototype.f8=function(){var a=null,b=this.T.Rb(),c=b.mf();this.hh=c;if(c=="a"){if(this.Cf)this.Cf.hide();var d=lb(_album.title);lb(_album.dateonly);lb(_album.location);var e=!(!(this.Db&2));_album.largepreview.replace(/\/s\d{2,3}(-c)?\//,"/s64-c/");var f=this.T.Rb().lf(),g,h;if(f.isOwner(this.T.ri())){g="/home";h=ty}else{g="/"+f.name;h=f.nickname}a=new MG(d,h,g,e)}else if(b.HZ())a=this.yV();else{var i=b.uZ(),k,n=[];if(i==0){n.push(_album.link);k=lb(_album.name)}else if(i==2){n.push("/lh/favorites");
this.TE(OG)}else if(i==3){n.push(_user.link);k=lb(_user.nickname)}else if(i==4){n.push("/home");this.TE(PG)}else if(i==1)this.TE(RG);a=new QG(b.ti()||"",i,b.wZ(),!(!_authuser.name),n,k)}this.tg.J(a)};
$.prototype.Fg=function(){return this.Db};
$.prototype.WQ=function(a){if(this.kF&&this.jF&&this.py&&typeof this.hh!="undefined"){var b=this.py.J2(),c=!a||this.hh=="pgt"||this.hh=="gt"||this.hh=="at"||this.hh=="a";M(this.kF,c);M(this.jF,c);if(c&&!b)this.py.collapse()}};
$.prototype.yV=function(){var a=this.T.qe();if(!a.length)return null;var b,c,d,e,f=gc(a,function(aa){return lb(aa.dispname)}),
g=f.join(", "),h=m("View all photos of {$subject}",{subject:g}),i=m("Find more photos of {$subject}",{subject:g}),k=this.T.Rb(),n=k.lf(),q=this.T.ri(),t=n.name==q,y=[];if(this.hh=="ap"){if(t)y.push([ty,"/"]);else y.push([uy(n.nickname),"/"+n.name]);y.push([lb(_album.title),_album.link]);b=h;c=hH;if(t){d=i;e=iH}}else if(this.hh=="gp"){var z=m("View public photos of {$subject}",{subject:g});b=a.length==1?z:"";var O=new N(window.location);O.fa("isOwner","false");c=a.length==1?O.toString():"";d=i;e=iH}else if(this.hh==
"pgp"){y.push([uy(n.nickname),"/"+n.name]);b=t?h:"";c=t?hH:""}return new NG(a,y,gH,b,c,d,e)};
$.prototype.qV=function(){var a=new TG(this);a.Ok(a.Gw,4);a.Ok(a.Ki,4);a.Ok(a.oD,1);a.Ok(a.oD,4);a.Ok(a.pD,1);a.Ok(a.pD,4);a.Ok(a.qy,4);return a};
$.prototype.ot=function(){if(this.T.Rb().mf()!="a")return new nF;var a=_album.largepreview.replace(/\/s\d{2,3}(-c)?\//,"/s64-c/"),b=this.T.Rb().Wa().Yb()>0?l(this.Oy,this):null;return new mF(a,_album.photoCount,_album.id,_album.used,_album.unlisted,_album.mapLink,_album.kmlLink,_album.bloglink,_album.enableRss,_album.photosFeed,_album.hasGeotaggedPhotos,_album.location,_album.dateonly,Xy(_album.description),b)};
$.prototype.AY=function(){return this.gb};
$.prototype.Mn=function(){return this.T.yd()};
$.prototype.Td=function(){return this.T.bY()+" - "+this.T.Rb().Td()};
$.prototype.D=function(){return"gphoto.FeedViewPage"};
$.prototype.m=function(){$.f.m.call(this);var a=u("lhid_navheader");if(a)this.DM.ia(a);else this.DM.C(this.e);var b=u("lhid_context");if(b)this.tg.C(b);else this.tg.C(this.e);this.gb.C(this.e);this.gb.Ur(3);var c=u("lhid_tools"),d=u("lhid_geodiv"),e=u("lhid_albumproperties");if(e&&this.Pg){e.innerHTML="";this.Pg.C(e)}if(c)this.Px.C(c);Xe(this.Nd.ao,this.Px.A());if(d){Ye(d,this.Px.A());this.kM.C(d)}if(c){this.jl.C(c);var f=u("lhid_flagger");if(f)c.removeChild(f.parentNode);this.Zp.C(c)}};
$.prototype.n=function(){P.prototype.n.call(this);this.sE=false;if(this.cd){this.cd.C(u("lhid_feedToolbar"));this.Z(this.cd,false)}if(this.Br&&this.qN)this.qN.ia(this.Br);var a=this.T.yd(),b=this.T.CA();this.j.g(this.T.Rb(),"changed",this.JU);this.j.g(a,RB,this.qj);this.j.g(a,QB,this.rj);this.j.g(a,PB,this.JE);this.j.g(a,[QB,SB,PB],this.qaa);this.j.g(this.gb,"itemactivated",this.rv);this.j.g(b,"deleterequested",this.CU);this.j.g(b,"deleteposted",this.BT);this.j.g(b,Bs,this.a7);this.j.g(this.Pg,ru,
this.FK);this.j.g(this.Zp,ru,this.FK)};
$.prototype.u=function(){$.f.u.call(this);this.j.ma();this.Nd.ha();this.pb.ha();this.s6()};
$.prototype.CU=function(a){var b=a.target,c=xv;if(_album.isdefault)c=uv;else if(_album.blogger)c=vv;return b&&confirm(typeof b.Gb!="undefined"&&b.Gb()?wv:c)};
$.prototype.rE=function(a){var b=this.gb.Wa(),c=b.Yb(),d=!Ba(a)?this.T.yd().xd():a;this.V("syncWithFeed_, index: "+d+" count: "+c);this.T.yd().ym(c);if(0==c){window.location.hash="";return}var e=pe(d,0,c-1);if(d!=e)this.T.yd().Tg(e);else{var f=b.Me(d);if(f==Rs)f=null;this.gb.ID(f);this.ns(f)}};
$.prototype.BT=function(){this.rE();this.gb.u_()};
$.prototype.a7=function(){this.rE()};
$.prototype.FK=function(a){var b=this.T.ri();if(!b){var c=m("You must be logged in to report an issue to another user.");alert(c);return}var d=a.entity;if(!d&&a.albumId){var e=this.T.Rb(),f=e.dB();d=this.T.CA().UX(f,a.albumId)}if(!this.NM){this.NM=new aH(this.T);var g=m("Report an issue");this.NM.Xi(g)}this.NM.open(d)};
$.prototype.b=function(){this.j.b();if(this.Sn)H(this.Sn);if(this.Fr)H(this.Fr);if(this.Cf)this.Cf.b();if(this.iA)this.iA.b();P.prototype.b.call(this)};
$.prototype.uC=function(){this.V("NEW context");this.T.Rb().ey()};
$.prototype.aa=function(a){var b=!a;this.WQ(b);this.gb.cE(b);this.T.Rb().ey()};
$.prototype.nb=function(){this.dispatchEvent(mg)};
$.prototype.Or=function(){if(this.Ha())Uz().log(this.VY(),this.wh())};
$.prototype.wh=function(){var a=this.T.Rb();a.mf();var b=!(!(this.Db&2)),c=a.Wa(),d={ts:Ka()},e=a.yY();if(e)d.feat=e;if(c){var f=c.bc();if(f){var g=f.lq();Uc(d,g)}}if(b&&this.gb.X){d.start=this.gb.X.Vf(this.gb.X.hd());d.num=Math.min(this.gb.X.Yc(),c.Yb())}if(a.q1())d.aid=a.dq();var h=a.NY();if(h)d.isOwner=true;else{var i=a.Cg(),k=this.T.ri();if(i&&i.isOwner(k))d.isOwner=true}return d};
$.prototype.VY=function(){var a=-1,b=this.T.Rb(),c=!(!(this.Db&2)),d=b.mf();if(d=="pgt"||d=="gt"||d=="at"||d=="ft"||d=="ct")a=c?178:179;else if(dH(d))a=c?118:119;else if(d=="pgp"||d=="gp"||d=="ap")a=c?176:177;else if(d=="a")a=c?2:3;return a};
$.prototype.JU=function(){var a=this.T.Rb(),b=a.Wa(),c=a.BA();if(b!=this.gb.Wa())this.gb.ae(b);if(this.Db!=0)Kz();var d=this.T.Rb().Cg();this.V("contextChanged, entity: "+d);var e=this.Db,f=c!=null;if(f){if(d){this.V("loading entity details, "+d.H()+", "+d.Td());d.ua(l(function(){this.gb.ID(d);this.dispatchEvent(new jF(d));this.ns(d)},
this))}e|=2;e&=-5;var g=b.bc().Cg();if(g)e=this.iy(g,e)}else{if(!d){this.T.xL();return}e|=4;e&=-3;e=this.iy(d,e)}this.ce(e);var h=u("lhid_more");M(h,f);this.gb.cE(f);if(!f&&d){var i=this.Mn();i.xd();var k=!this.sE||!b.isLoaded();if(!this.fX)this.fX=new Qs("id",b);this.fX.nA(d.id,l(this.n8,this,k))}else this.FQ(_user.portrait,_user.link,_user.nickname,e&1);var n=a.mf()=="a";if(this.Sn){H(this.Sn);this.Sn=null}if(!n)if(!b.isLoaded())this.j.g(b,Ds,this.NP);else if(b.Yb()==0)this.NP();var q=u("lhid_cover_id");
if(q)if(f&&n&&e&1)this.Sn=F(q,E,this.Oy,false,this);if(f)if(!b.isLoaded())$f(b,Es,l(this.Or,this));else this.Or()};
$.prototype.n8=function(a,b){var c=b[0];if(a){this.sE=true;this.rE(c)}var d=this.Mn();d.Tg(c)};
$.prototype.Fb=function(a){return this.T.ri()&&a&&typeof a.isOwner!="undefined"&&a.isOwner(this.T.ri())};
$.prototype.iy=function(a,b){if(this.Fb(a))b|=1;else b&=-2;return b};
$.prototype.rv=function(){this.V("itemActivated_");var a=this.Db;a|=4;a&=-3;var b=this.gb.Fl();a=this.iy(b,a);this.T.Th(b.id);this.dispatchEvent(new jF(b));this.ce(a);this.ns(b);var c=u("lhid_more");M(c,false);this.gb.cE(false);this.gb.aa()};
$.prototype.ns=function(a){this.V("updateDetails_, detail mode? :"+(this.Db&4));if(this.Db&4)if(a&&a!=Rs){this.pb.Mk(a);a.Ei(l(function(){this.Nd.EF(a);var b=u("lhid_share");if(b)b.href="/lh/emailPhoto?uname="+a.user.name+"&iid="+a.id;this.AQ(a.album.title,a.album.link)},
this))}};
$.prototype.qj=function(){var a=this.gb.Fl();if(a){this.ns(a);var b=a.id;this.T.Th(b);var c=this.iy(a,this.Db);if(this.Db==c&&!(!(this.Db&4)))this.Or();this.ce(c);this.T.Rb().ey()}};
$.prototype.rj=function(){this.JE();if(!(!(this.Db&2)))this.Or()};
$.prototype.qaa=function(){if(this.tg){var a=this.tg.mY();if(a&&typeof a.xP!="undefined"){var b=this.Mn(),c=b.Vf(b.hd());a.xP(c,b.Yc(),b.Kj())}}};
$.prototype.JE=function(){if(this.Ha()){var a=!(!(this.Db&2));if(a){if(this.hh=="s"){var b=this.Mn().hd(),c=this.Mn().tq();if(b==c-1||c==1){var d=new N(window.location),e=d.Ga("filter");if(e=="1"){d.fa("filter",0);if(!this.Wk){this.Wk=v("div",{"class":"lhcl_search_crowded_results"});var f=d.toString()+"#",g=m("In order to show you the most relevant results, we have omitted some entries very similar to those already displayed. {$breakTag}If you like, you can {$linkStart}repeat the search with the omitted results included{$linkEnd}.",
{breakTag:"<br>",linkStart:'<a href="'+f+'">',linkEnd:"</a>"});this.Wk.innerHTML=g;x(this.w3,this.Wk)}else M(this.Wk,true)}}else if(this.Wk)M(this.Wk,false)}}else if(this.Wk)M(this.Wk,false)}};
$.prototype.PU=function(a,b){var c=new Et(_selectedPhotosPath,a,b,"BANNER");return c};
$.prototype.Oy=function(){if(!this.Cf){var a=this.T.Rb();this.Cf=this.PU(a.dB(),_album.id);this.Fr=F(this.Cf,fu,this.BQ,false,this)}this.Cf.rx("BANNER",false);this.Cf.show()};
$.prototype.s6=function(){if(this.Cf){this.Cf.hide();this.Cf.b();this.Cf=null}if(this.Fr)H(this.Fr)};
$.prototype.BQ=function(a){var b=a.thumbUrl||"";if(b)this.daa(b)};
$.prototype.daa=function(a){if(this.hh=="a"){a=a.replace(/\/s\d{2,3}(-c)?\//,"/s64-c/");this.Pg.lx(a)}};
$.prototype.AQ=function(a,b){var c=this.T.Rb();if(a&&b&&c.mf()!="a"&&this.Pg){this.Pg.show(true);this.Pg.U7(a,b)}else if(this.Pg)this.Pg.show(false)};
$.prototype.Nk=function(a,b){var c=this.T.CA().MJ(a);if(c){c.Sg(b);this.ns(c)}};
$.prototype.maa=function(a){if(a.entity&&a.entity.$a&&a.entity.$a.author){var b=a.entity.$a.author[0];this.FQ(b.thumbnail,b.uri,b.nickname);this.AQ(a.entity.album.title,a.entity.album.link)}};
$.prototype.FQ=function(a,b,c,d){var e=this.T.Rb(),f=e.mf();if(f=="a")return;var g={portraitUrl:a,gallerylink:b,displayname:p(c)};g.portrait=ez(d?'<div class="lhcl_portraitcontainer"onmouseover="_setVisibility(true, \'lhid_pickerbutton\')"onmouseout="_setVisibility(false, \'lhid_pickerbutton\')"style="height:48"><div id="lhid_pickerbutton" style="visibility:hidden"><a href="javascript:_d(\'ProfilePicker\')"><img src="img/pickerbutton_small.gif" /></a></div><a href="javascript:_d(\'ProfilePicker\')"><img class="lhcl_portrait" id="lhcl_portrait_id" src="{portraitUrl}" /></a></div>':
'<div><a id="lhid_portraitlink" href="{gallerylink}"><img class="lhcl_portrait" id="lhcl_portrait_id" src="{portraitUrl}" /></a></div>',g);var h=ez(jH,g);u("lhid_hostbox").innerHTML=h};
$.prototype.NP=function(){var a=u("lhid_noresults");if(a)M(a,true)};
$.prototype.TE=function(a){var b=u("lhid_noresults");if(b)b.innerHTML=ez(Ey,{query:this.T.Rb().ti(),corpusName:a})};
var kH=function(a,b,c,d,e,f,g,h,i){var k=u(a);if(k){var n=lb(c||"");gH=f||"";hH=g||"";iH=h||"";var q=new WG(b,n,d||"",k,e,i);q.j8($);q.p6(ZG,/^slideshow(\/\d*)?$/);q.start()}},
lH=m("Photos by"),jH='<div class="lhcl_sidebox"><table class="lhcl_host"><tr><td class="lhcl_column_left">{portrait}</td><td class="lhcl_column_right">'+lH+'<br><div class="lhcl_name" id="lhid_user_nickname"><a href="{gallerylink}">{displayname}</a></div></td></tr></table></div></div>'};{var mH=function(a){this.gl=this.A9();this.HN=this.B9();this.qn=this.z9();this.kc=this.C9(a)};
mH.prototype.C9=function(a){var b=m("Share"),c=m("Download"),d=m("Prints"),e=m("Edit");return{SLIDESHOW:new nH("lhid_slideshow",yy,"lhcl_toolbar_text","SPRITE_slideshow lhcl_spriting_alignBottom",["a","s","gt","pgt","ct","ft","pgp","gp","ap"],l(this.AC,this,a),a),SHARE:new nH("lhid_share",b,"lhcl_toolbar_text","SPRITE_share lhcl_spriting_alignBottom",["a","s","gt","pgt","ct","ft","pgp","gp","pg","ap"],l(this.G9,this),a),DOWNLOAD:new nH("lhid_dowload",c,"lhcl_toolbar_text",null,["a"],l(this.kC,this),
a,this.qn),PRINTS:new nH("lhid_prints",d,"lhcl_toolbar_text",null,["a"],l(this.kC,this),a,this.HN),EDIT:new nH("lhid_editMenu",e,"lhcl_toolbar_text",null,["a"],l(this.kC,this),a,this.gl)}};
mH.prototype.A9=function(){var a=m("Album properties"),b=m("Organize & Reorder"),c=m("Captions"),d=m("Delete album"),e=m("Album cover"),f=m("Album map"),g=m("Edit in Google Photo Lab");return{ALBUM_PROPERTIES:{id:"lhid_editAlbumPropertiesButton",msg:a,enabled:l(this.bC,this)},ALBUM_COVER:{id:"lhid_albumCoverButton",msg:e,enabled:l(this.xw,this)},ALBUM_MAP:{id:"lhid_albumMapButton",msg:f,enabled:l(this.xw,this)},CAPTIONS:{id:"lhid_editCaptionsButton",msg:c,enabled:l(this.xw,this)},DELETE_ALBUM:{id:"lhid_deleteAlbumButton",
msg:d,enabled:l(this.UT,this)},DELETE_PHOTO:{id:"lhid_deletePhotoButton",msg:zv,enabled:l(this.Ry(false),this)},DELETE_VIDEO:{id:"lhid_deleteVideoButton",msg:yv,enabled:l(this.Ry(false),this)},ORGANIZE:{id:"lhid_organizeButton",msg:b,enabled:l(this.xw,this)},EDIT_IN_PICASA:{id:"lhid_editInPicasaButton",msg:g,enabled:l(this.YT,this)}}};
mH.prototype.B9=function(){var a=m("Order prints"),b=m("View order");return{VIEW_ORDER:{id:"lhid_viewOrder",msg:b,enabled:l(this.Ry(_features.cart))},ORDER_PRINTS:{id:"lhid_orderPrints",msg:a,enabled:l(this.ZT,this)}}};
mH.prototype.z9=function(){var a=m("Download to Picasa"),b=m("Download to Google Photo Lab"),c=m("Download photo"),d=m("Print with Google Photo Lab"),e=m("Make collage"),f=m("Make movie");return{TO_PICASA:{id:"lhid_downloadToPicasa",msg:a,enabled:l(this.WT,this)},TO_GPL:{id:"lhid_downloadToPhotoLab",msg:b,enabled:l(this.VT,this)},PRINT_PICASA:{id:"lhid_printWithPicasa",msg:d,enabled:l(this.iz,this)},COLLAGE_PICASA:{id:"lhid_collageWithPicasa",msg:e,enabled:l(this.iz,this)},MOVIE_PICASA:{id:"lhid_movieWithPicasa",
msg:f,enabled:l(this.iz,this)},DOWNLOAD_PHOTO:{id:"lhid_downloadPhoto",msg:c,enabled:l(this.XT,this)}}};
mH.prototype.fY=function(){return this.kc};
mH.prototype.Ry=function(a){return function(){return a}};
mH.prototype.Z4=function(a){return a.dR()};
mH.prototype.kC=function(a,b){for(var c in a)if(a[c].enabled(b))return true;return false};
mH.prototype.bC=function(a){return a.feedViewPage.Fg()==3};
mH.prototype.xw=function(a){return this.bC(a)&&_album.photoCount};
mH.prototype.AC=function(a){if(_album.id)return _album.photoCount>0;else if(a.feedViewPage){var b=a.feedViewPage.Wa();return b&&b.isLoaded()&&b.ms()>0}else return false};
mH.prototype.iz=function(a){return _album.picasa.length>0&&_picasaVersion>=3&&!(a.feedViewPage.Fg()&4)};
mH.prototype.WT=function(a){return _album.picasa.length>0&&_picasaVersion>0&&_picasaVersion<3&&!(a.feedViewPage.Fg()&4)};
mH.prototype.VT=function(a){return _album.picasa.length>0&&_picasaVersion>=3&&!(a.feedViewPage.Fg()&4)};
mH.prototype.YT=function(a){if(_features.editinpicasa&&a.feedViewPage.Fg()==5&&_picasaVersion>=3){var b=a.contextManager.Cg();return b&&!b.Gb()}return false};
mH.prototype.UT=function(a){return this.bC(a)&&(!_album.blogger||_album.blogger&&!_album.photoCount)};
mH.prototype.XT=function(a){if(_album.picasa&&a.feedViewPage.Fg()&4){var b=a.contextManager.Cg();return b&&!b.Gb()}return false};
mH.prototype.ZT=function(a){var b=a.contextManager.Cg(),c=a.feedViewPage.Fg()&2;return _features.cart&&pa(_updateCartPath)&&this.AC()&&(c||b&&!b.Gb())&&(a.dR()||_album.prints)};
mH.prototype.G9=function(a){var b=this.Z4(a);return(b||!_album.unlisted)&&(this.AC(a)||!a.feedViewPage)};
mH.prototype.XZ=function(a,b,c,d,e){switch(a){case "lhid_slideshow":var f="slideshow";if(c)f+="/"+c.id;b.feedViewPage.me().Th(f);break;case "lhid_share":this.E9(b,c,d,e);break;case this.HN.VIEW_ORDER.id:window.location="/lh/viewCart";break;case this.HN.ORDER_PRINTS.id:this.W4(b,c,d,e);break;case this.qn.TO_PICASA.id:case this.qn.TO_GPL.id:window.location=_album.picasa;break;case this.gl.EDIT_IN_PICASA.id:var g=new N(c.mi);g.fa("alt","atom");g.fa("imgdl","1");window.location="picasa://editimagefeed?url="+
ab(g.toString());break;case this.qn.PRINT_PICASA.id:var g=new N(_album.photosRss);g.fa("imgdl","1");window.location="picasa://printalbumfeed?url="+ab(g.toString());break;case this.qn.COLLAGE_PICASA.id:window.location="picasa://collagefeed?url="+ab(_album.photosRss);break;case this.qn.MOVIE_PICASA.id:window.location="picasa://makemoviefeed?url="+ab(_album.photosRss);break;case this.qn.DOWNLOAD_PHOTO.id:window.location=bs(c.s);break;case this.gl.ALBUM_PROPERTIES.id:new cB(5);break;case this.gl.ALBUM_MAP.id:window.location=
_album.mapLink;break;case this.gl.ORGANIZE.id:window.location=_reorderPath;break;case this.gl.CAPTIONS.id:window.location=_editCaptionsPath;break;case this.gl.ALBUM_COVER.id:b.feedViewPage.Oy();break;case this.gl.DELETE_ALBUM.id:this.UV();break}};
mH.prototype.E9=function(a,b,c,d){if(b&&c&4){var e="/lh/emailPhoto?uname="+b.user.name+"&iid="+b.id;window.location=e}else if(a.pageType=="a"&&c&2){var f="/lh/emailAlbum?uname="+d+"&aid="+_album.id;window.location=f}else if(c&2){var g=a.contextManager.Wa().Me(0),h=[],i=new N(g.Rf()),k=i.Ga("imgmax");if(k==null||k==0||k>400)i.fa("imgmax",400);h.push("source="+ab(i.toString()));h.push("uname="+ab(g.user.name));h.push("continue="+ab(window.location.toString()));var n="/lh/emailFeedForm?"+h.join("&");
window.location=n}else if(a.pageType=="pg"){var q="/lh/emailGalleryForm?uname="+a.ownerName;window.location=q}};
mH.prototype.UV=function(){var a=Fw;if(_album.isdefault)a+=" "+Dw;if(!confirm(a))return;var b=v("form",{method:"post",action:_deleteAlbumPath},v("input",{type:"hidden",name:"aid",value:_album.id}));x(Ke().body,b);b.submit()};
var nH=function(a,b,c,d,e,f,g,h){this.id=a;this.msg=b;this.txtClass=c;this.iconClass=d;this.menuItems=h;this.JT=f;this.d5=e;this.T=g};
nH.prototype.enabled=function(){var a=false,b=this.d5.length;for(var c=0;c<b;c++)if(this.d5[c]==this.T.pageType){a=true;break}if(!a)return false;if(this.menuItems)return this.JT(this.menuItems,this.T);return this.JT(this.T)};
nH.prototype.oj=function(){return this.menuItems?this.CV():this.BV()};
nH.prototype.BV=function(){return new Mk(v("div",{id:this.id,"class":"goog-toolbar-button"},v("div",{"class":"goog-inline-block "+this.iconClass},"\u00a0"),v("div",{"class":"goog-inline-block "+this.txtClass},this.msg)),Lm())};
nH.prototype.CV=function(){var a=new Wl;a.kx(this.id);Gc(this.menuItems,function(b){if(b.enabled(this.T)){var c=new Ol(b.msg);c.kx(b.id);c.HD(255,true);a.ub(c)}},
this);return new em(v("div",{"class":"goog-toolbar-menu-button"},v("div",{"class":"goog-inline-block "+this.txtClass},this.msg)),a,Fm())};
mH.prototype.W4=function(a,b,c,d){var e=a.pageType=="a"&&c&2,f=l(e?this.BS:this.SS,a.feedViewPage);Kz();var g=Pi("_rtok",""),h=["uname="+d,"noredir=true","rtok="+g];if(e)h.push("aid="+a.contextManager.sd);else if(b)h.push("iid="+b.H());h=h.join("&");Gj(_updateCartPath,f,"POST",h)};
mH.prototype.SS=function(a){if(!this.rd(a)){var b=m("This photo has been added.");Jz(b,3000)}};
mH.prototype.BS=function(a){if(!this.rd(a)&&!this.de(a))window.location="/lh/viewCart"}};{var eH=function(a,b,c,d,e){P.call(this);this.T=new oH(a,b,c,d,e);this.cd=null;this.j=new K(this);this.AE=new K(this);this.kc=new mH(this.T)};
o(eH,P);eH.prototype.n=function(){P.prototype.n.call(this);if(this.T.contextManager)this.j.g(this.T.contextManager,"changed",this.ly);if(this.T.feedViewPage){this.j.g(this.T.feedViewPage,UG,this.ly);this.j.g(this.T.feedViewPage,iF,this.iaa)}this.IT=u("lhid_feedToolbar");this.ly()};
eH.prototype.ly=function(){var a=this.T&&this.T.feedViewPage?this.T.feedViewPage.Wa():null;if(a&&!a.isLoaded()){if(!this.kA)this.kA=F(a,[Es,Ds],l(this.ly,this));return}else if(this.kA){H(this.kA);this.kA=0}M(this.IT,false);if(!this.T.feedViewPage){this.Ue=-1;this.dO();if(this.cd)this.AE.g(this.cd,xk,this.MK)}else{var b=this.T.feedViewPage.Fg(),c=this.T.contextManager.mf();this.T.pageType=c;this.Ue=b;if(this.cd){this.cd.b();this.cd=null;this.AE.ma()}this.dO();if(this.cd)this.AE.g(this.cd,xk,this.MK)}if(this.cd)M(this.IT,
true)};
eH.prototype.MK=function(a){var b=a.target.Vc().id;if(!b)b=a.target.H();this.kc.XZ(b,this.T,this.Dba,this.T.feedViewPage?this.T.feedViewPage.Fg():null,this.T.contextManager?this.T.contextManager.dB():null)};
eH.prototype.dO=function(){if(!this.T.pageType)return;this.cd=new Tm;var a=this.kc.fY();Gc(a,function(b){if(!b.enabled())return;this.cd.Z(b.oj(),true);this.cd.Z(new Fl(Pm()),true)},
this);this.cd.MD(false);this.cd.C(this.e)};
var pH=function(a,b,c){var d=u("lhid_feedToolbar");if(d){var e=new eH(a,null,null,b,c);e.ia(d)}};
eH.prototype.u=function(){if(this.cd){this.cd.b();this.cd=null}this.j.ma();this.AE.ma();this.T.pageType=null;this.Ue=null;P.prototype.u.call(this)};
eH.prototype.iaa=function(a){this.Dba=a.target};
var oH=function(a,b,c,d,e){this.pageType=a;if(!this.pageType&&c)this.pageType=c.mf();this.feedViewPage=b;this.contextManager=c;this.authUserName=d||null;this.ownerName=e||null};
oH.prototype.dR=function(){return this.feedViewPage?!(!(this.feedViewPage.Fg()&1)):(this.authUserName&&this.ownerName?this.authUserName==this.ownerName:false)}};var dH=function(a){return a=="s"||a=="ct"||a=="ft"||a=="pgt"||a=="gt"||a=="at"||a=="f"};{var qH=function(a,b,c){P.call(this);this.j=new K(this);this.Q=a;this.Q.Wi(50);this.$ba=F(this.Q,Cs,this.lm,false,this);this.Zj=[];this.ih=0;this.qM=-1;this.rt=1;this.Dr=1;this.Uu=-1;this.X4=b;this.af=c;this.E4=0;this.iw=0;this.Q.Sj(0);this.rca=l(this.rf,this)};
o(qH,P);qH.prototype.mv=false;qH.prototype.Dz=-1;qH.prototype.D=function(){return"gphoto.explore.LocationGame"};
qH.prototype.V=function(){var a={};a.hiscore=this.Uu;a.numguess=this.E4;a.numphotos=this.iw;Uz().log(168,a)};
qH.prototype.u9=function(){if(this.af.name&&this.Uu>this.X4){var a=["pref=WHERE_IN_WORLD_HIGH_SCORE","value="+this.Uu];Gj(_setPrefUrl,null,"POST",a.join("&"))}};
qH.prototype.b=function(){if(!this.W()){qH.f.b.call(this);this.V();this.u9();H(this.$ba);if(this.Ve)this.Ve.b();if(this.$p)this.$p.b();oc(this.Zj);if(this.o)GEvent.clearInstanceListeners(this.o)}};
qH.prototype.m=function(){var a="img/experimental/logo/b-gp-"+_LH.locale+".gif";this.e=gz(rH,{logoSrc:a,logoAlt:Ix})};
qH.prototype.n=function(){qH.f.n.call(this);this.I=u("lhid_targetPhoto");this.Fi=u("lhid_gameMap");this.nD=u("lhid_gameStatus");this.Xda=u("lhid_roundDisplay");this.Yda=u("lhid_roundScore");this.lca=u("lhid_highScore");this.cR=u("lhid_viewPhoto");this.NC=u("lhid_photoIndex");this.Ki=u("lhid_next");this.Vba=u("lhid_exit");this.PQ(1,1);this.VE(0);this.EQ(this.X4);if(!this.o)Oz(l(this.Hv,this));this.j.g(this.Ki,E,this.B4);this.Ve=new Ig(this.Fi);this.j.g(this.Ve,Jg,this.Wj);this.j.g(this.Vba,E,this.M_);
this.j.g(window,mg,this.Ef);this.$p=new sg;this.j.g(this.$p,"fontsizechange",this.Ef)};
qH.prototype.u=function(){qH.f.u.call(this);this.j.ma();if(this.$p){this.$p.b();this.$p=null}if(this.Ve){this.Ve.b();this.Ve=null}};
qH.prototype.show=function(a){if(this.la==a)return;this.la=a;if(a){Iy(this,Gy);this.G1();var b=th();b.scrollTop=0;M(this.e,true);this.Qa();this.Ef()}else{Iy(this,Hy);this.aaa();M(this.e,false)}};
qH.prototype.Qa=function(){if(this.la){var a=Se();sh(this.e,a)}};
qH.prototype.Ef=function(){if(this.la){var a=Qe(),b=Math.max(580,a.height);L(this.e,"height",b+"px");var c=vh(this.Fi),b=b-c-10;L(this.Fi,"height",b+"px");if(this.o)this.xG()}};
qH.prototype.xG=function(){this.o.checkResize();var a=new GLatLngBounds;this.o.setCenter(new GLatLng(20,0),this.o.getBoundsZoomLevel(a));this.o.savePosition()};
qH.prototype.Hv=function(){this.o=new GMap2(this.Fi);this.xG();this.o.addControl(new GLargeMapControl);this.o.addControl(new GScaleControl);this.o.addControl(new GHierarchicalMapTypeControl);this.o.enableContinuousZoom();this.o.enableScrollWheelZoom();this.o.addMapType(G_PHYSICAL_MAP);this.o.setMapType(G_PHYSICAL_MAP);this.jn=new GIcon;this.jn.iconSize=new GSize(32,32);this.jn.iconAnchor=new GPoint(15,31);this.jn.shadow="img/marker-shadow.png";this.jn.shadowSize=new GSize(59,32);this.jn.transparent=
"img/marker-overlay.png";this.jn.infoWindowAnchor=new GPoint(15,0);this.VZ=new GMarker(new GLatLng(0,0),{clickable:false});GEvent.bind(this.o,"click",this,this.Ov);if(this.ih<=this.qM)this.dE()};
qH.prototype.Ov=function(a,b){if(this.mv&&b&&Math.abs(b.lat())<=85){var c=(new Date).getTime();this.E4++;this.mv=false;this.VZ.setLatLng(b);this.o.addOverlay(this.VZ);var d=this.Q.Me(this.ih),e=new GLatLng(d.lat,d.lon),f=d.Rf("s32-c");Rg(l(this.BW,this,b,e,f,this.ih),500);var g=(e.distanceFrom(b)/1000).toFixed(1),h=((c-this.mg)/1000).toFixed(1);this.xaa(d,g,h)}};
qH.prototype.BW=function(a,b,c,d){if(this.ih!=d)return;var e=new GLatLngBounds(a);e.extend(b);var f=this.o.getBoundsZoomLevel(e);f=Math.max(f-1,0);this.o.setCenter(e.getCenter(),f);var g=new GIcon(this.jn,c),h=new GMarker(b,{icon:g,clickable:false});this.o.addOverlay(h);this.o.addOverlay(new GPolyline([a,b],"#EE0000"))};
qH.prototype.Wj=function(a){a.preventDefault()};
qH.prototype.M_=function(){this.show(false)};
qH.prototype.Fo=function(){this.o.clearOverlays();this.mv=false;this.o.returnToSavedPosition()};
qH.prototype.lm=function(a){var b=this.ih>this.qM;this.qM+=a.count;if(this.o&&b)this.dE()};
qH.prototype.dE=function(){if(this.Q.SK(this.ih)){M(this.I,false);this.Fo();var a=this.Q.Me(this.ih);rs(a.Rf(),new ne(a.w,a.h),400,false,this.rca,null,this.I)}};
qH.prototype.rf=function(){this.mv=true;this.mg=(new Date).getTime();M(this.I,true);this.saa();M(this.Ki,true);this.iw++;var a=this.Q.Me(this.ih+1);if(this.Qi){ss(this.Qi);this.Qi=0}if(a)this.Qi=rs(a.Rf(),new ne(a.w,a.h),400,false,null)};
qH.prototype.B4=function(){M(this.Ki,false);M(this.nD,false);M(this.cR,false);this.ih++;this.Q.Sj(this.ih);this.PQ();if(this.Q.SK(this.ih))this.dE()};
qH.prototype.PQ=function(a,b){this.Dr=b||this.Dr+1;var c=this.Dr>10;this.rt=a?a:(c?this.rt+1:this.rt);if(c){this.Dr=1;this.VE(0)}B(this.NC,rx(this.Dr,10));var d=m("ROUND: {$roundNum}",{roundNum:this.rt});B(this.Xda,d)};
qH.prototype.VE=function(a){if(this.Dz!=a){this.Dz=a;this.Yda.innerHTML=Cz(1,a)}};
qH.prototype.EQ=function(a){if(this.Uu<a){this.Uu=a;this.lca.innerHTML=Cz(1,a)}};
qH.prototype.saa=function(){var a;if(this.Dr<10){var b=m("Next &raquo;");a=b}else{var c=m("Start round {$round}",{round:this.rt+1});a=c}this.Ki.innerHTML=a};
qH.prototype.xaa=function(a,b,c){var d=this.QT(b,c),e=this.Dz+d;this.VE(e);this.EQ(e);var f=Cz(1,b),g=m("Your guess was {$distance} km away. {$points} points!",{distance:f,points:d});this.nD.innerHTML=g;of(this.nD,"gphoto-explore-results");M(this.nD,true);this.cR.href=a.lu()+"?feat=wiw";M(this.cR,true)};
qH.prototype.G1=function(){var a=$e(document.body);while(a){if(a.style.display!="none"){M(a,false);this.Zj.push(a)}a=af(a)}};
qH.prototype.aaa=function(){while(this.Zj.length)M(this.Zj.pop(),true)};
qH.prototype.QT=function(a){var b=0;if(a<=6000){b=1408/(Math.sqrt(a/1000)+1)-408;b=b<0?0:Math.round(b)}return b};
var sH=m("SCORE:"),tH=m("HIGH SCORE:"),uH=m("Click the map to enter your guess."),vH=m("View photo"),wH=m("&laquo; Exit"),rH='<div class="gphoto-explore-game"><a href="/"><img class="gphoto-explore-logo" src="{logoSrc}" alt="{logoAlt}" /></a><span class="gphoto-explore-gameTitle">'+xy+'</span><table width="99%" cellspacing="0" cellpadding="0"><tr><td><table class="gphoto-explore-scoreBox"><tr><td width="30%" class="gphoto-explore-roundScore">'+sH+'<span id="lhid_roundScore"></span></td><td width="40%" class="gphoto-explore-highScore">'+
tH+'<span id="lhid_highScore"></span></td><td width="30%" class="gphoto-explore-round"><div id="lhid_roundDisplay"></div></td></tr></table></td><td class="gphoto-explore-instructions" id="lhid_gameStatus">'+uH+'</td></tr><tr><td><div class="gphoto-explore-photoContainer"><img id="lhid_targetPhoto" style="display: none;" /></div><table width="100%"><tr class="gphoto-explore-photoInfo"><td class="lhcl_align_left"><a style="display: none;" target="_blank" id="lhid_viewPhoto">'+vH+'</a></td><td class="lhcl_align_right"><span id="lhid_photoIndex"></span></td></tr><tr class="gphoto-explore-controls"><td class="lhcl_align_left"><span class="lhcl_fakelink" id="lhid_exit">'+
wH+'</span></td><td class="lhcl_align_right"><span class="lhcl_fakelink" id="lhid_next"></span></td></tr></table></td><td width="100%"><div class="gphoto-explore-map" id="lhid_gameMap"></div></td></tr></table></div>'};{var xH=function(a,b,c,d){P.call(this);this.j=new K(this);this.wa=a;this.Q=this.wa.OJ(b);this.hca=c;this.af=d;var e=m("Start game");this.lE=new Sk(e);this.lE.Jd("gphoto-button");this.Z(this.lE)};
o(xH,P);xH.prototype.D=function(){return"gphoto.explore.LocationGameWidget"};
xH.prototype.b=function(){if(!this.W()){xH.f.b.call(this);this.j.b();this.Q.b()}};
xH.prototype.$=function(a){qH.f.$.call(this,a);var b=m("Check out photos from around the world and guess where they were taken!");xH.f.$.call(this,a);var c=v("div",{"class":"gphoto-explore-widget-title"},xy);x(a,c);var d=w("div");x(a,d);this.lE.C(d);var e=v("table",{width:"100%","class":"gphoto-wiw-teaser"});x(a,e);var f=w("tbody");x(e,f);var g=w("tr");x(f,g);var h=v("td");x(g,h);this.q5=v("img",{src:"img/wiw_photo.jpg","class":"gphoto-wiw-photo"});x(h,this.q5);h=v("td");x(g,h);this.G3=v("img",{src:"img/wiw_map.jpg"});
x(h,this.G3);var i=v("div",{},b);x(a,i)};
xH.prototype.n=function(){xH.f.n.call(this);this.j.g(this.lE,xk,this.mE);this.j.g(this.G3,E,this.mE);this.j.g(this.q5,E,this.mE)};
xH.prototype.u=function(){this.j.ma()};
xH.prototype.mE=function(){if(!this.rA){this.rA=new qH(this.Q,this.hca,this.af);this.Z(this.rA);this.rA.C(document.body)}this.rA.show(true)}};{var yH=function(a,b,c){WE.call(this,a,b);this.Yq=Ba(c)?c:1000};
o(yH,WE);yH.prototype.S=function(){var a=yH.f.S.call(this);return Math.min(a,this.Yq)};
yH.prototype.wb=function(a){return a<this.Yq?yH.f.wb.call(this,a):null};
yH.prototype.nY=function(){var a=this.X.xd();return this.Q.Me(a)};
yH.prototype.yd=function(){return this.X}};{var zH=function(a,b,c){S.call(this,Il,b,c);var d=a||{};d[AF]=d[AF]||KF;d[FF]=d[FF]||JF;d[BF]=d[BF]||48;d[CF]=d[CF]||48;d.itemgap=d.itemgap||5;d[GF]=48;d.crop=true;d[HF]=d[HF]||LF;this.Qd=new UF(d);this.Av=new Sk;this.Av.Jd("gphoto-filmstrip-left");this.Uw=new Sk;this.Uw.Jd("gphoto-filmstrip-right");this.Z(this.Av);this.Z(this.Qd);this.Z(this.Uw)};
o(zH,S);zH.prototype.U=function(a){zH.f.U.call(this,a);this.Qd.U(a);this.MQ()};
zH.prototype.forward=function(){this.Qd.forward()};
zH.prototype.wp=function(){this.Qd.wp()};
zH.prototype.b=function(){if(!this.W())zH.f.b.call(this)};
zH.prototype.m=function(){zH.f.m.call(this);this.bG()};
zH.prototype.$=function(a){zH.f.$.call(this,a);this.bG()};
zH.prototype.bG=function(){var a=v("div",{"class":"filmstrip-holder"});x(this.e,a);this.Av.C(this.e);this.Qd.C(a);this.Uw.C(this.e);this.Vaa=v("div",{"class":"gphoto-active-cluster"})};
zH.prototype.n=function(){zH.f.n.call(this);this.j.g(this.Qd,VF,this.O_);this.j.g(this.Av,[xk,I],this.wp);this.j.g(this.Uw,[xk,I],this.forward);this.j.g(window,mg,this.Qd.RN,false,this.Qd);x(this.e,this.Vaa)};
zH.prototype.u=function(){zH.f.u.call(this)};
zH.prototype.O_=function(a){this.k.yd().Tg(a.index);this.MQ()};
zH.prototype.MQ=function(){if(!this.Ha())return;var a=this.k?this.k.yd().xd():0,b=this.k?this.k.S():0;this.Av.Vb(1,a==0);this.Uw.Vb(1,a+1>=b)};
zH.prototype.D=function(){return"gphoto.explore.RecentFilmStrip"}};{var AH=function(a,b,c){P.call(this);this.j=new K(this);this.wa=a;this.aca=b;this.Ir=c;this.Qd=new zH;this.Z(this.Qd);var d=m("Full screen"),e=m("Refresh");if(_features.recentphotos){this.kk=new Sk(d);this.kk.Jd("gphoto-slideshowbutton");this.Z(this.kk)}this.Bo=new Sk(e);this.Bo.Jd("gphoto-button");this.Z(this.Bo);this.ab=new Ym(this.Qd.forward,10000,this.Qd);this.Jq=true;this.lN=0;this.uca=l(this.d0,this);this.Ba=Uz()};
o(AH,P);AH.prototype.D=function(){return"gphoto.explore.RecentWidget"};
AH.prototype.b=function(){if(!this.W()){AH.f.b.call(this);this.j.b();if(this.nC)H(this.nC)}};
AH.prototype.$=function(a){AH.f.$.call(this,a);C(a,"gphoto-explore-recent");var b=m("Recent Photos"),c=w("table");x(a,c);var d=w("tbody");x(c,d);var e=w("tr");x(d,e);var f=w("td");x(e,f);var g=v("div",{"class":"gphoto-explore-widget-title"},b);x(f,g);var f=v("td",{width:"100%"});x(e,f);if(this.kk)this.kk.C(f);this.Bo.C(f);this.Bo.Vb(1,true);var h=v("div",{"class":"gphoto-explore-recent-filmContainer"});x(a,h);this.Qd.C(h);var i=w("table");x(h,i);d=w("tbody");x(i,d);e=w("tr");x(d,e);f=v("td",{"class":"gphoto-explore-recent-photo"});
x(e,f);this.sN=Ah(f);this.sN.height-=8;this.Kc=v("a",{style:"display:none;"});x(f,this.Kc);this.I=w("img");x(this.Kc,this.I);e=w("tr");x(d,e);f=w("td");x(e,f);this.Cb=v("span",{"class":"gphoto-explore-filmstripControls"});x(f,this.Cb)};
AH.prototype.n=function(){AH.f.n.call(this);if(this.kk)this.j.g(this.kk,xk,function(){window.location.href="/lh/recentPhotos"});
this.j.g(this.Bo,xk,this.T0);this.j.g(this.Cb,E,this.L$);this.j.g(j,[Gy,Hy],this.of);this.nH(this.Ir);this.CQ()};
AH.prototype.u=function(){AH.f.u.call(this);this.j.ma()};
AH.prototype.nH=function(a){var b=this.aca,c=["nocache="+(new Date).getTime()];if(this.Kca)c.push("published-min="+this.Kca.K$(true));b=b+"&"+c.join("&");var d=this.wa.VA(null,"","G",true,b),e=new lt(this.wa,d);e.Wi(100);if(a)e.DN(0,a,true);var f=new OB(100,100);if(this.nC)H(this.nC);this.nC=F(f,RB,this.j0,false,this);if(this.kl){this.kl.nP(f);this.kl.ae(e)}else this.kl=new yH(e,f,f.Kj());e.ua(0,100,l(this.Qd.U,this.Qd,this.kl))};
AH.prototype.j0=function(){this.ab.stop();if(this.bM){ss(this.bM);this.bM=0}var a=this.kl.nY();if(a){var b=new ne(a.w,a.h),c=gs(b,this.sN);this.bM=rs(as(a.Rf()),b,c,false,this.uca,l(this.Qd.forward,this.Qd),this.I);this.Kc.href=a.lu()+"?feat=recent"}};
AH.prototype.d0=function(){M(this.Kc,true);var a=this.kl.yd(),b=a.xd(),c=a.WI(),d=this.kl.S();if(b+1>=d)this.Bo.Vb(1,false);else if(this.Jq)this.ab.start();if(this.Qi){ss(this.Qi);this.Qi=0}var e=b+c;if(e>=0&&e<d){var f=this.kl.wb(e);if(f){var g=new ne(f.w,f.h),h=gs(g,this.sN);this.Qi=rs(as(f.Rf()),g,h,false,null)}}};
AH.prototype.L$=function(){this.Jq=!this.Jq;if(this.Jq)this.ab.start();else this.ab.stop();this.CQ()};
AH.prototype.CQ=function(){this.Cb.innerHTML=this.Jq?Jx:Kx};
AH.prototype.of=function(a){if(a.type==Gy)this.lN++;else this.lN--;if(this.Jq)if(this.lN>0)this.ab.stop();else this.ab.start()};
AH.prototype.T0=function(){M(this.Kc,false);this.nH();this.Bo.Vb(1,true);this.Ba.log(169)}};{var BH=function(a,b,c,d,e,f,g){this.wa=new mt;this.Oda=a;this.l6=new AH(this.wa,b,c);this.gca=d;this.NX=new xH(this.wa,e,f,g);this.Z(this.l6);this.Z(this.NX);var h=m("Explore");window.document.title=Ix+" - "+h};
o(BH,P);BH.prototype.$=function(a){BH.f.$.call(this,a);this.l6.ia(u(this.Oda));this.NX.ia(u(this.gca))};
BH.prototype.n=function(){this.Eb=true};
var CH=function(a,b,c,d,e,f,g){var h=new BH(b,c,d,e,f,g,_authuser);h.ia(a);$f(j,"gphotounload",h.b,false,h)};
m("Get your photos featured!")};var DH={ERAS:["BC","AD"],ERANAMES:["Before Christ","Anno Domini"],NARROWMONTHS:["J","F","M","A","M","J","J","A","S","O","N","D"],MONTHS:["January","February","March","April","May","June","July","August","September","October","November","December"],SHORTMONTHS:["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"],WEEKDAYS:["Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"],SHORTWEEKDAYS:["Sun","Mon","Tue","Wed","Thu","Fri","Sat"],NARROWWEEKDAYS:["S","M","T",
"W","T","F","S"],SHORTQUARTERS:["Q1","Q2","Q3","Q4"],QUARTERS:["1st quarter","2nd quarter","3rd quarter","4th quarter"],AMPMS:["AM","PM"],DATEFORMATS:["EEEE, MMMM d, yyyy","MMMM d, yyyy","MMM d, yyyy","M/d/yy"],TIMEFORMATS:["h:mm:ss a v","h:mm:ss a z","h:mm:ss a","h:mm a"],ZONESTRINGS:null};DH.STANDALONENARROWMONTHS=DH.NARROWMONTHS;DH.STANDALONEMONTHS=DH.MONTHS;DH.STANDALONESHORTMONTHS=DH.SHORTMONTHS;DH.STANDALONEWEEKDAYS=DH.WEEKDAYS;DH.STANDALONESHORTWEEKDAYS=DH.SHORTWEEKDAYS;DH.STANDALONENARROWWEEKDAYS=
DH.NARROWWEEKDAYS;si(DH,"en_US");{var FH=function(a){QE.call(this,"lhcl_DrawingPane");this.jU=new Ym(this.i_,250,this);this.B=rr();this.vk={};this.da=a;this.fm=new EH};
o(FH,QE);var GH="smallviewonly",kA="viewonly";FH.prototype.lL=false;FH.prototype.cf=function(){if(this.mh){this.mx(false);this.Ye()}};
FH.prototype.m=function(){FH.f.m.call(this);C(this.A(),this.O());this.fm.C(this.A())};
FH.prototype.n=function(){FH.f.n.call(this);this.p.g(this.B.client,Zq,this.D0);this.p.g(this.B.client,Yq,this.B0);this.p.g(this.B.client,Tq,this.$Z);this.p.g(this.B.client,$q,this.A0);this.p.g(this.A(),fg,this.Uj);this.p.g(this.A(),Hf,this.pf);this.p.g(this.A(),Gf,this.se);this.p.g(this,"cancel",this.cf);this.p.g(this,Vq,this.wq);this.p.g(this.B.client,ar,this.Iu);this.Ye()};
FH.prototype.u=function(){FH.f.u.call(this);this.p.ma()};
FH.prototype.GA=function(){return new ke(this.sa.offsetLeft,this.sa.offsetTop,this.sa.offsetWidth,this.sa.offsetHeight)};
FH.prototype.Le=function(){return this.sa};
FH.prototype.Xa=function(){return this.da};
FH.prototype.w8=function(a){this.P1=a;this.Ye()};
FH.prototype.qP=function(a,b,c,d){this.ig=a;this.rp=b;this.sa=c;this.mx(false);if(d)this.qU();this.Ye();this.jU.start()};
var jA=function(a,b,c,d,e,f,g){if(a&&c&&b){if(!a.Tc)a.Tc=new FH(f||kA);if(a.Tc.Ha()){a.Tc.u();A(a.Tc.A())}if(e){a.Tc.qP(c,d,b,g);a.Tc.C(b.parentNode)}}};
FH.prototype.Ye=function(){if(!this.Ha())return;var a=this.B.client.Cl()[this.ig];this.Jh(true);Oc(this.vk);if(this.da==GH){var b=a&&Hr(Ic(a.ac()).sort(HH))||[],c=[],d=0;r(b,function(h){if(h.H()==tr)d++;else c.push(h.Lb())},
this);var e,f=m("{$numUnnamed} unnamed",{numUnnamed:d}),g=f;if(c.length>3||c.length==3&&d>0){c[3]="...";c.length=4;e=c.join(", ")}else if(c.length==3){c.length=3;e=c.join(", ")}else if(c.length>0)e=c.join(", ")+(d>0?" + "+g:"");else if(d>0)e=g;this.fm.U(e)}else if(a){Gc(a.ac(),function(h){var i=new IH(h);this.vk[h.H()]=i;this.Z(i);i.C(this.A());if(this.P1&&h.H()==this.P1.H())i.Ab(true)},
this);this.Dq()}this.ds()};
FH.prototype.qU=function(){var a=this.B.client.Cl()[this.ig];if(a){this.B.client.WN(a);if(this.Ha())this.Ye()}};
FH.prototype.hY=function(a){var b=new ge(a.offsetX,a.offsetY),c=new ce(this.sa.offsetTop,this.sa.offsetLeft+this.sa.offsetWidth,this.sa.offsetTop+this.sa.offsetHeight,this.sa.offsetLeft);if(!c.contains(b))return null;var d=null,e=-1;Gc(this.vk,function(f){var g=f.A(),h=new ce(g.offsetTop,g.offsetLeft+g.offsetWidth,g.offsetTop+g.offsetHeight,g.offsetLeft),i=he(h,b);if(e<0||i<e){e=i;d=f}});
return d};
FH.prototype.$Z=function(){if(this.da!="editor"||this.mh)return;var a=this.sa.offsetWidth,b=this.sa.offsetHeight,c=new ge(a/2,b/2);if(!(new ce(0,a,b,0)).contains(c))return;var d=Math.min(this.sa.offsetWidth,this.sa.offsetHeight)/6,e=d*0.8,f=d;if(c.x-e<0)c.x=e;else if(c.x+e>=a)c.x=a-e-1;if(c.y-f<0)c.y=f;else if(c.y+f>=b)c.y=b-f-1;var g=this.B.client.Cl()[this.ig];if(!g){g=new Er({id:this.ig,aid:this.rp,url:this.sa.src,shapes:[]});this.B.client.TS(g)}var h=new Fr({id:Rr(),subject:this.B.client.qe()[tr],
type:"Face",photo:g,x1:c.x-e,y1:c.y-f,x2:c.x+e,y2:c.y+f,w:a,h:b});this.mh=new IH(h);this.mx(true);this.Z(this.mh);this.mh.C(this.A());this.mh.MO(true);this.mh.M(true)};
FH.prototype.Yi=function(a,b,c){if(!this.mo){if(!b)return;this.mo=new JH(null,this);this.mo.C(document.body)}if(b&&a&&c){this.mo.U(a);this.mo.Ax(a);var d=Ah(this.mo.A()),e=c.A(),f=Ah(e),g=uh(e),h=(f.width-d.width)/2+g.x,i=f.height+g.y;sh(this.mo.A(),h,i)}this.mo.M(b)};
FH.prototype.wq=function(a){this.Yi(null,false,null);if(!a.shape)return;if(!a.shape.Yj()){this.mh.MO(false);this.B.combined.dh(a.shape,l(this.wq,this,a));this.Ye();this.mh=null;this.mx(false);return}if(a.contactInformation){var b=a.contactInformation;this.B.combined.Ps(b.name,b.displayName,b.email,[a.shape],"",l(this.Ye,this),b.notUsedDefaultDisplayName,"TEXT_BOX","ONE_UP_VIEW",null)}else if(a.subject)this.B.combined.ve(a.subject,[a.shape],l(this.Ye,this),a.recommendation?"RECOMMENDATION":"TEXT_BOX",
"ONE_UP_VIEW",a.confidenceScore)};
FH.prototype.i_=function(){var a=this.GA();if(!le(a,this.Ica)){this.Ica=a;this.Ye()}if(this.da!=GH)this.jU.start()};
FH.prototype.A0=function(a){var b=this.vk[a.shape.H()];if(b)b.Yi(true)};
FH.prototype.Uj=function(a){if(this.lL||!sf(a.target,this.O())||this.da==GH)return;var b=this.hY(a);if(!b){this.Dq();return}this.Dq(b);b.M(true)};
FH.prototype.pf=function(a){if(this.da==GH){if(this.fm)this.fm.M(false)}else if(a.target==this.A()&&(!a.relatedTarget||!df(this.A(),a.relatedTarget)))this.Dq()};
FH.prototype.B0=function(a){var b=this.vk[a.shape.H()];if(b)b.M(false)};
FH.prototype.se=function(){if(this.da==GH&&this.fm){var a=Ah(this.fm.A()),b=Ah(this.A()),c=(b.width-a.width)/2,d=b.height-a.height-4;sh(this.fm.A(),c,d);this.fm.M(true)}};
FH.prototype.D0=function(a){var b=this.vk[a.shape.H()];if(b)b.M(true)};
FH.prototype.Iu=function(a){if(a.subType=="modify"&&mc(a.photoIds,this.ig))this.Ye()};
FH.prototype.Dq=function(a){Gc(this.vk,function(b){if(b!=a)b.M(false,true)},
this)};
var HH=function(a,b){return xc(a.getBounds().left,b.getBounds().left)};
FH.prototype.mx=function(a){this.lL=a;if(a)this.Dq();else{if(this.mh){var b=this.mh.na();b.Sf().Lw(b)}this.mh=null}};
FH.prototype.ds=function(){Ic(this.vk).sort(KH);var a=this.A();ec(this.vk,function(b){var c=b.A();A(c);x(a,c)})}};{var LH=function(){QE.call(this,"lhcl_MessagePane");this.I1=new Ym(this.qf,5000,this);var a=m("Undo"),b=$l();this.by=new Mk(a,b);this.Z(this.by)};
o(LH,QE);var MH={error:"error_",info:"info_",undo:"undo_",warning:"warning_"};LH.prototype.cy=null;LH.prototype.m=function(){this.od(this.yg('<div class="{$prefix}"><table class="lhcl_NoSpace"><tbody><tr><td><span class="{$prefix}-messageText_"></span>&nbsp;<span class="{$prefix}-undoButton_"></span></td></tr></tbody></table></div>'));var a=this.qb("undoButton_");this.by.C(a);M(this.by.A(),false)};
LH.prototype.n=function(){LH.f.n.call(this);this.p.g(this.by,xk,this.j1);this.qf()};
LH.prototype.u=function(){LH.f.u.call(this);this.p.ma()};
LH.prototype.aa=function(a,b,c){if(this.X2=="error")return;this.X2=a;this.cy=c;if(a=="clearmessage"||!b){this.qf();return}this.I1.yI();var d=this.qb("messageText_"),e=this.A();C(e,this.O(MH[a]));M(e,true);B(d,b);M(this.by.A(),a=="undo"&&c);if(a!="info")window.scroll(0,0);if(a=="warning"||a=="info")this.I1.start()};
LH.prototype.j1=function(){if(this.cy){this.cy();this.cy=null;this.dispatchEvent("undo");this.dispatchEvent({type:"updatemessagepanerequested",subType:"clearmessage"})}};
LH.prototype.qf=function(){if(this.X2=="error")return;var a=this.A();M(a,false);Gc(MH,function(b){qf(a,this.O(b))},
this)}};{var OH=function(a){P.call(this);Ez.Hd(this);this.U(a);this.p=new K(this);this.B=rr();this.ef=new NH;this.p.g(this.ef,"edit",this.wK)};
o(OH,P);OH.prototype.m=function(){this.$(this.K.createElement("div"))};
OH.prototype.$=function(a){OH.f.$.call(this,a);var b=v("div",{"class":"gphoto-sharepeoplesection"});this.y7=v("div",{"class":"shareboxheader"});x(b,this.y7);this.kea=new PH(this.na());this.kea.C(b);var c=false;if(c){this.fea=v("div");x(b,this.fea)}x(a,b)};
OH.prototype.n=function(){OH.f.n.call(this);$f(this.B.combined,dr,this.Mu,false,this);this.B.combined.co()};
OH.prototype.u=function(){OH.f.u.call(this);this.p.ma()};
OH.prototype.Mu=function(){var a=this.B.client.qq(null,Nr);this.zaa(a.length)};
OH.prototype.zaa=function(a){var b=m("People in these photos"),c=m("({$peopleCount})",{peopleCount:a});this.y7.innerHTML="<b>"+b+"</b> "+c};
var QH=function(a){var b=u("lhid_ShareFaces");if(b){var c=new OH(a);c.ia(b)}}};{var PH=function(a){QE.call(this);this.U(a);this.p=new K(this);this.B=rr();this.ef=new NH;this.f3=[]};
o(PH,QE);var RH="gphoto-sharepeoplelistbox";PH.prototype.m=function(){this.$(this.K.m("div",{"class":RH}))};
PH.prototype.$=function(a){PH.f.$.call(this,a);var b=v("table",{"class":"gphoto-sharepeoplelisttop"}),c=w("tbody"),d=w("tr"),e=w("td"),f=v("td",{"class":RH+"-allnonelink"});x(b,c);x(c,d);x(d,e);x(d,f);var g=m("Send to:");B(e,g);this.vO=v("a",{href:"javascript:void(0);"});B(this.vO,fy);this.yO=v("a",{href:"javascript:void(0);"});B(this.yO,gy);var h=this.K.createTextNode(" | ");x(f,this.vO);x(f,h);x(f,this.yO);x(a,b)};
PH.prototype.n=function(){PH.f.n.call(this);this.p.g(this.vO,I,this.Y0);this.p.g(this.yO,I,this.$0);$f(this.B.combined,dr,this.Mu,false,this);this.B.combined.j9(this.na());this.B.combined.co()};
PH.prototype.u=function(){PH.f.u.call(this);this.p.ma()};
PH.prototype.Y0=function(){this.GG(true)};
PH.prototype.$0=function(){this.GG(false)};
PH.prototype.GG=function(a){r(this.f3,function(b){b.rm(a)},
this)};
PH.prototype.K_=function(a){this.ef.open("edit",[a.subject])};
PH.prototype.Mu=function(){var a=this.A(),b=this.B.client.qq(Kr,Nr);Gc(b,function(c){var d=new SH(c);if(!d.sb())this.p.g(d,Wq,this.K_);d.C(a);this.f3.push(d)},
this)}};{var SH=function(a){P.call(this);this.p=new K(this);this.B=rr();this.U(a)};
o(SH,QE);var TH="gphoto-sharepersonlistitem";SH.prototype.m=function(){this.$(this.K.m("div",{"class":TH}));this.RQ()};
SH.prototype.$=function(a){SH.f.$.call(this,a);var b=this.na(),c=v("table",{"class":TH+" listitem"}),d=w("tbody"),e=w("tr"),f=v("td",{"class":TH+" checkboxcell"}),g=v("td",{"class":TH+" iconcell"}),h=v("td",{"class":TH+" infocell"});x(c,d);x(d,e);x(e,f);x(e,g);x(e,h);this.Ap=new LE;this.Ap.C(f);this.uB=w("div");var i=b.xl();if(i){var k=new SE(i,1,48,60);k.C(this.uB);C(this.uB,"itemdisabled")}x(g,this.uB);this.Nz=v("div",{"class":TH+" displayname itemdisabled"});B(this.Nz,b.au());this.Rp=v("div",{"class":TH+
" email"});var n=m("Add email");this.LF=v("a",{href:"javascript:void(0);"});B(this.LF,n);if(b.ne())B(this.Rp,b.ne());else{this.YO(false);x(this.Rp,this.LF)}x(h,this.Nz);x(h,this.Rp);x(a,c)};
SH.prototype.n=function(){SH.f.n.call(this);this.p.g(this.B.client,fr,this.KK);this.KK();this.p.g(this.LF,E,this.YZ);this.p.g(this.Ap,xk,this.qK)};
SH.prototype.u=function(){SH.f.u.call(this);this.p.ma()};
SH.prototype.rm=function(a){if(this.sb()){this.Ap.rm(a);this.qK()}};
SH.prototype.ak=function(){return this.Ap.ak()};
SH.prototype.sb=function(){return this.Ea};
SH.prototype.YO=function(a){this.Ea=a;tf(this.uB,"itemdisabled",!a);tf(this.Nz,"itemdisabled",!a);this.Ap.Ca(a)};
SH.prototype.YZ=function(){this.dispatchEvent({type:Wq,subject:this.na()})};
SH.prototype.qK=function(){var a=u("lhid_emailtofield"),b=a.value,c=this.na().ne(),d=u("lhid_InvitationCount"),e=d.value;if(!c)return;var f=new fn;if(this.Ap.ak()){f.F(b);var g=b.split(",");for(var h=0;h<g.length;h++){var i=Va(g[h]);if(Xa(i,c)==0)break}if(h==g.length){f.F(c);f.F(", ");e++}}else{var g=b.split(",");for(var h=0;h<g.length;h++){var i=Va(g[h]);if(i.length!=0&&Xa(i,c)!=0){f.F(i);f.F(", ")}else if(i.length!=0&&e!=0)e--}}a.value=f.toString();d.value=e};
SH.prototype.KK=function(a){if(!a||a.subType=="modify"&&mc(a.subjectIds,this.na().H()))this.RQ()};
SH.prototype.RQ=function(){var a=this.na();B(this.Nz,a.au());if(!this.Ea&&a.ne()){We(this.Rp);B(this.Rp,a.ne());this.YO(true)}else if(this.Ea)B(this.Rp,a.ne())}};{var UH=function(a,b,c){QE.call(this,"lhcl_ShapeIcon");this.B=rr();this.zj=c||0;this.U(a)};
o(UH,QE);var VH=m("Click anywhere to hide photo");UH.prototype.Ep=null;UH.prototype.Wm=null;UH.prototype.Aa=false;UH.prototype.qm=null;UH.prototype.Ra=null;UH.prototype.m=function(){var a='<div class="{$prefix}"><div class="{$prefix}-thumbnailArea_ lhcl_NoSpace"><img class="{$prefix}-thumbnail_" width="80" height="96"></div>';a=this.zj!=2?a+'<div class="{$prefix}-controlsArea_"><div class="lhcl_RelativeContainer"><div class="{$prefix}-selection_"></div><div class="{$prefix}-contextPhoto_"></div></div></div>':
a+"</div>";this.od(this.yg(a));this.Ra=this.qb("thumbnail_");this.Ra.src=this.na().Gl();switch(this.zj){case 0:var b=this.qb("selection_");this.qm=new LE;this.Z(this.qm);this.qm.C(b);break;case 1:this.F$=this.yg('<img class="{$prefix}-thumbnailProhibitionBadge_" src="img/ui/shapeIcon/noSymbol.gif" style="display: none"/>');this.Ra.parentNode.appendChild(this.F$);break}if(this.zj!=2){var c=this.qb("contextPhoto_");this.Ep=v("img",{src:"img/ui/shapeIcon/contextPhotoIcon.gif"});var d=m("Displays this photo in its original context.");
this.Wm=new Um(this.Ep);this.Wm.y8('<div class="lhcl_Tooltip"><nobr>'+d+"</nobr></div>");x(c,this.Ep)}};
UH.prototype.b=function(){if(this.Wm)this.Wm.b()};
UH.prototype.n=function(){UH.f.n.call(this);this.Oc(this.Yn());this.p.g(this.Ra,E,this.h1);if(this.Ep)this.p.g(this.Ep,E,this.m_);if(this.zj==0)this.p.g(this.qm,xk,this.d1);this.p.g(this.A(),Gf,this.se);this.p.g(this.A(),Hf,this.pf)};
UH.prototype.u=function(){UH.f.u.call(this);if(this.Wm)this.Wm.M(false);this.p.ma()};
UH.prototype.Yn=function(){return this.Aa};
UH.prototype.Oc=function(a,b){this.Aa=a;if(this.Ha()){switch(this.zj){case 0:if(this.qm.ak()!=a)this.qm.rm(a);tf(this.A(),this.O("selected_"),a);break;case 1:this.pW(a);break}if(b)this.kW()}};
UH.prototype.kW=function(){this.dispatchEvent({type:"selectionchanged",subType:this.Yn()?"selected":"deselected",shapes:[this.na()]})};
UH.prototype.m_=function(){$f(this.B.combined,cr,this.P0,false,this);this.B.combined.su(this.na().Sf())};
UH.prototype.pf=function(){tf(this.A(),this.O("hover_"),false)};
UH.prototype.se=function(){tf(this.A(),this.O("hover_"),true)};
UH.prototype.M0=function(a,b){if(b.Ha())b.b();a.b()};
UH.prototype.N0=function(a,b){b.C(a.NJ().parentNode)};
UH.prototype.P0=function(){var a=this.na(),b=a.Sf(),c=new FH(kA);c.w8(a);var d=new TE;d.a8(20);d.rP(b.uq());d.oP(UE,this.yg(WH));d.C(document.body);C(d.A(),this.O("photoOverlay_"));$f(d.A(),E,l(this.M0,this,d,c),false,this);var e=d.NJ();c.qP(b.H(),b.dq(),e,false);$f(d,"load",l(this.N0,this,d,c),false,this)};
UH.prototype.d1=function(){this.Oc(this.qm.ak(),true)};
UH.prototype.h1=function(a){if(this.zj==2){this.dispatchEvent({type:"picked",shape:this.na()});return}this.Oc(!this.Yn(),true);this.dispatchEvent({type:E,originalEvent:a})};
UH.prototype.pW=function(a){M(this.F$,a)};
var WH='<div class="{$prefix}-PhotoOverlayInstructions"><div class="lhcl_RelativeContainer"><div class="{$prefix}-PhotoOverlayInstructions-backgroundTopLeft_"></div><div class="{$prefix}-PhotoOverlayInstructions-backgroundTopRight_"></div><div class="{$prefix}-PhotoOverlayInstructions-backgroundBottomLeft_"></div><div class="{$prefix}-PhotoOverlayInstructions-backgroundBottomRight_"></div><div class="{$prefix}-PhotoOverlayInstructions-backgroundTop_"></div><div class="{$prefix}-PhotoOverlayInstructions-backgroundRight_"></div><div class="{$prefix}-PhotoOverlayInstructions-backgroundBottom_"></div><div class="{$prefix}-PhotoOverlayInstructions-backgroundLeft_"></div><div class="{$prefix}-PhotoOverlayInstructions-contents_">'+
VH+"</div></div></div>"};{var XH=function(a,b){R.call(this,b||"lhcl_FacesMgrDlg",true);this.V=Ed(this.D());this.zj=a;this.B=rr();this.RV=false;this.fE=null;this.Vo=null;this.Ex=null;var c=new Xk,d=m("Save changes");c.J("save",d,true);c.J(dl,wx,false,true);this.Rg(c)};
o(XH,R);XH.prototype.n=function(){XH.f.n.call(this);this.V.jf("enterDocument");this.p.g(this,Zk,this.Z0)};
XH.prototype.u=function(){XH.f.u.call(this);this.V.jf("exitDocument");this.p.ma()};
XH.prototype.wk=function(a,b,c,d,e){this.RV=c;this.fE=d;this.m7=e;this.Jh();this.AX(a,b);this.p3(d);this.M(true)};
XH.prototype.AX=function(a,b){var c=this.Au();We(c);x(c,a);var d=this.zj==2?"":'<div class="'+this.Gc+'-selectAllNone_">'+p(jy)+':&nbsp;&nbsp;<span id="lhid_FacesMgrDlg-selectAll" class="lhcl_fakelink">'+p(fy)+'</span>&nbsp;&nbsp;<span id="lhid_FacesMgrDlg-selectNone" class="lhcl_fakelink">'+p(gy)+"</span></div>";this.hc('<div id="lhid_FacesMgrDlg-instructions_" class="'+this.Gc+'-instructions_"><div id="lhid_FacesMgrDlg-instructionsText_" class="'+this.Gc+'-instructionsText_"></div>'+d+'</div><div id="lhid_FacesMgrDlg-shapeIcons_" class="'+
this.Gc+'-shapeIcons_"></div>');this.Oa();var e=u("lhid_FacesMgrDlg-instructionsText_");We(e);x(e,b);var f=u("lhid_FacesMgrDlg-selectAll");if(f)this.p.g(f,I,l(this.HO,this,true));var g=u("lhid_FacesMgrDlg-selectNone");if(g)this.p.g(g,I,l(this.HO,this,false));this.Ex=u("lhid_FacesMgrDlg-shapeIcons_")};
XH.prototype.yZ=function(a){var b=[];Gc(this.Vo,function(c){if(c.Yn()==a)b.push(c.na())},
this);return b};
XH.prototype.p3=function(){this.FE();this.Tq()};
XH.prototype.FE=function(){this.V.jf("unloadShapes");if(this.Ex!=null)We(this.Ex);this.Vo=null};
XH.prototype.Tq=function(){this.V.jf("load_: "+this.fE.join(","));$f(this.B.combined,er,this.f1,false,this);var a=this.B.client.qe(),b=gc(this.fE,function(c){this.V.jf("sid="+c+"; subject="+a[c]);return a[c]},
this);this.B.combined.o3(b)};
XH.prototype.f1=function(){this.V.jf("handleSubjectShapesLoaded_");this.Vo=[];var a=this.B.client.qe();Gc(this.fE,function(c){var d=a[c];this.XS(d)},
this);if(!this.Vo||nc(this.Vo)){var b=m("There are no faces available.");x(this.Ex,v("div",{"class":"lhcl_FacesMgrDlg-noFacesAvailable"},b))}};
XH.prototype.XS=function(a){var b=a.vf();Gc(a.ac(),function(c){var d=new UH(c,b,this.zj);this.Vo.push(d);this.Z(d);d.Oc(this.RV);d.C(this.Ex)},
this)};
XH.prototype.Z0=function(a){switch(a.key){case "save":this.U0(a);break;case dl:this.h_(a);break;default:throw new Error("unexpected event: key="+a.key);}this.FE()};
XH.prototype.U0=function(){var a=this.yZ(true);this.V.jf("handleSave: selected shapes="+a.length);this.m7(a)};
XH.prototype.h_=function(){this.V.jf("CANCEL button clicked")};
XH.prototype.HO=function(a){Gc(this.Vo,function(b){b.Oc(a,false)},
this)};
XH.prototype.D=function(){return"gphoto.sapphire.ui.FacesManagerDialog"}};{var YH=function(){this.V=Ed(this.D());XH.call(this,1)};
o(YH,XH);YH.prototype.wk=function(a,b){var c=a.Lb(),d=[a.H()],e=m("Correct name tag mistakes:"),f=m('Click on faces that are not "{$name}".',{name:c}),g=Ve('<span><span class="lhcl_FacesMgrDlg-title">'+p(e)+'</span><span class="lhcl_FacesMgrDlg-title-subject-name"> '+p(c)+"</span></span>"),h=Ve("<span>"+p(f)+"</span>");YH.f.wk.call(this,g,h,false,d,b)};
YH.prototype.D=function(){return"gphoto.sapphire.ui.TaggedFacesManagerDialog"}};{var ZH=function(){XH.call(this,2,"gphoto-portraitChooser");var a=new Xk;a.J(dl,xu,false,true);this.Rg(a);this.j=new K(this);this.j.g(this,"picked",this.D9)};
o(ZH,XH);ZH.prototype.wk=function(a,b){var c=a.Lb(),d=[a.H()],e=m("Select a Portrait Photo for "),f=m('Click on the portrait to use for "{$name}".',{name:c}),g=Ve('<span><span class="lhcl_FacesMgrDlg-title">'+p(e)+'</span><span class="lhcl_FacesMgrDlg-title-subject-name"> '+p(c)+"</span></span>"),h=Ve("<span>"+p(f)+"</span>");ZH.f.wk.call(this,g,h,false,d,b)};
ZH.prototype.D9=function(a){a.stopPropagation();a.preventDefault();this.m7(a.shape);this.FE();this.M(false)};
ZH.prototype.D=function(){return"gphoto.sapphire.ui.PortraitPicker"}};{var NH=function(){R.call(this,"lhcl_peopleManagerDialog",true);this.V=Ed(this.D());this.j=new K(this);this.Mp=F(this,Zk,l(this.of,this));this.B=rr();this.wC=null};
o(NH,R);NH.prototype.da=null;NH.prototype.wL=[];NH.prototype.mk=false;NH.prototype.b=function(){if(!this.W()){NH.f.b.call(this);H(this.Mp);this.PG();this.j.b()}};
NH.prototype.M=function(a){if(!this.mk)pE.f.M.call(this,a)};
NH.prototype.RJ=function(){var a=this.Mb[0];if("_subject_"in a)return a;return a.na().subject};
NH.prototype.open=function(a,b){this.PG();this.wC=null;this.da=a;this.Mb=b;var c;We(this.Vc());if(a=="edit"){var d=m("Edit Contact Information");this.Xi(d);this.hc(this.dV());c=ky}else{var e=m("Which contact information do you want to save?");this.Xi(e);this.hc(this.oV());c=Zx}var f=new Xk;f.J("save",c,true);f.J(dl,wx,false,true);this.Rg(f);this.M(true);var g=u("lhid_photoselect"),h=this.RJ();M(g,h&&h.H());this.j.g(g,[I],this.BG);this.qO=document.getElementsByName("save")[0];L(this.qO,"font-weight",
"bold");var i=document.getElementsByName("subjectNickname")[0];if(i&&i.focus)try{i.focus()}catch(k){}};
NH.prototype.BG=function(){var a=this.RJ();if(!this.y5)this.y5=new ZH;this.y5.wk(a,l(this.z5,this))};
NH.prototype.z5=function(a){var b=a.Gl(),c=a.dc().xl();if(c!=b){this.wC=a;var d=u("lhid_portrait_img");d.src=b}};
NH.prototype.PG=function(){while(this.wL.length>0){var a=this.wL.pop();a.b()}this.j.ma()};
NH.prototype.yu=function(a){if(a instanceof U)return a;return a.na().subject};
NH.prototype.Eaa=function(){var a=document.getElementsByName("subjectNickname")[0].value,b=document.getElementsByName("subjectName")[0].value,c=document.getElementsByName("subjectEmail")[0].value,d=u("lhid_nicknameWarning"),e=u("lhid_nameWarning"),f=u("lhid_emailWarning"),g=!Sa(a);M(d,!g);var h=!Sa(b);M(e,!h);var i=Sa(c)||az(c);M(f,!i);return g&&h&&i};
NH.prototype.of=function(a){var b=u("lhid_photoselect");if(b)this.j.Sa(b,[I],this.BG);if(a.key==dl){a.stopPropagation();a.preventDefault();var c=new $H("cancelpeoplemgr",null,null);c.cancelled=true;this.Xw(c);return}if(a.key=="save"){a.stopPropagation();var d=Me("form","",this.Oa())[0],e=xf(d),f=this.B.combined,g;if(this.da=="edit"){if(!this.Eaa()){a.preventDefault();return}g=this.Mb[0];if(!g.H()){var h={name:String(gd(e,"subjectName","")),displayName:String(gd(e,"subjectNickname","")),email:String(gd(e,
"subjectEmail",""))};c=new $H(this.da,g,null);c.contactinfo=h;this.Xw(c);return}else{c=new $H(this.da,g,null);var i=this.yu(g),k=l(function(){c.subject=i;this.Xw(c)},
this),n;n=this.wC?l(function(){f.wm(i,this.wC,k)},
this):k;f.ok(i,String(gd(e,"subjectName","")),String(gd(e,"subjectNickname","")),String(gd(e,"subjectEmail","")),n)}}else{var q=gd(e,"subject"),t,y,z=[];for(var O=0;O<this.Mb.length;O++){var aa=this.yu(this.Mb[O]);if(aa.H()==q){g=this.Mb[O];t=O;y=aa}else z.push(aa)}sc(this.Mb,t);var c=new $H(this.da,g,this.Mb);f.ar(y,z,l(this.Xw,this,c))}this.qO.disabled=true;this.mk=true}};
NH.prototype.Xw=function(a){this.mk=false;this.M(false);this.qO.disabled=false;this.dispatchEvent(a)};
NH.prototype.dV=function(){var a=this.yu(this.Mb[0]),b=aI(a),c=m("Change Portrait");b.lhui_photoselect=c;return ez(bI(),b)};
NH.prototype.oV=function(){var a=["<form><table><tbody>"];for(var b=0;b<this.Mb.length;b++){var c=this.yu(this.Mb[b]),d=aI(c);d.lhui_sid=c.H();d.lhui_numShapes=c.Mj();if(b!=0){a.push('<tr><td colspan="3"><div class="lhcl_pseudo_hr" />');a.push("</td></tr>")}else d.lhui_checked="checked";a.push(ez('<tr><td><div><input type="radio" name="subject" {lhui_checked} value="{lhui_sid}" id="radio{lhui_sid}" /></div></td><td><label for="radio{lhui_sid}"><img src="{lhui_src}" class="lhcl_sp_iconicShape" /></label></td><td width="100%"><div class="lhcl_sp_subjectName">{lhui_nickname} <span class="lhcl_notbold" style="font-size: 84%;">({lhui_numShapes})</span></div><div class="lhcl_sp_subjectInfo">{lhui_name}<br />{lhui_email}</div></td></tr>',
d))}a.push("</tbody></table></form>");return a.join("")};
var aI=function(a){return{lhui_src:a.xl(),lhui_nickname:a.au(true,-1),lhui_name:a.getName(true,-1),lhui_email:a.ne(true,-1)}};
NH.prototype.D=function(){return"gphoto.sapphire.ui.PeopleManagerDialog"};
var $H=function(a,b,c){D.call(this,a,b);this.remainingSubjects=c;this.cancelled=false};
o($H,D);var bI=function(){var a=m("Email"),b=m("Name"),c=m("Nickname"),d=m("Name must not be empty."),e=m("Nickname must not be empty."),f=m("Invalid e-mail address.");return'<div><form><div style="float: left"><img src="{lhui_src}" id="lhid_portrait_img" width="80" height="96"><div id="lhid_photoselect" style="display: none;"class="lhcl_fakelink" title="{lhui_photoselect}">{lhui_photoselect}</div></td></div><div style="float: left"><div id="lhid_nicknameWarning" class="lhcl_warning" style="display:none" > '+
e+'</div><div><label for="lhid_subjectNickname">'+c+'</label><input type="text" id=lhid_subjectNickname" class="lhcl_input" tabindex="1"name="subjectNickname" value="{lhui_nickname}" maxlength="15"></div><div id="lhid_nameWarning" class="lhcl_warning" style="display:none" > '+d+'</div><div><label for="lhid_subjectName">'+b+'</label><input type="text" id="lhid_subjectName" class="lhcl_input" tabindex="2"name="subjectName" value="{lhui_name}" maxlength="128"></div><div id="lhid_emailWarning" class="lhcl_warning" style="display:none" > '+
f+'</div><div><label for="lhid_subjectEmail">'+a+'</label><input type="text" id="lhid_subjectEmail" class="lhcl_input" tabindex="3"name="subjectEmail" value="{lhui_email}"  maxlength="128"></div></div><div style="clear: both"/></form></div>'}};{var cI=function(){U.call(this,{name:"",id:""});this.Bj="";this.mfa=""};
o(cI,U);cI.prototype.R8=function(a){this.Og=a;var b=m("Create new contact:");this.wg=b;this.lfa="";this.nfa=""};
cI.prototype.Lb=function(a){return cI.f.Lb.call(this,a,-1)};
cI.prototype.toString=function(){return cI.f.getName.call(this)};
cI.prototype.Yj=function(){return false};
var dI=function(a){En.call(this,[],false);this.To(a||[]);this.t5=new cI};
o(dI,En);dI.prototype.To=function(a){this.ca=a};
dI.prototype.pk=function(a,b,c){var d=vb(a),e=new RegExp(d,"i"),f=[];r(this.ca,function(g){if(e.test(g.getName())||e.test(g.Lb())||e.test(g.ne()))f.push(g)});
if(!f.length){this.t5.R8(a);f=[this.t5]}c(a,wc(f,0,b))}};{var eI=function(){Dn.call(this)};
o(eI,Dn);eI.prototype.C=Bn;var fI=function(a,b){a.KO(true);this.sda(b);a.KO(false)};
eI.prototype.Mw=function(a,b,c){var d=a.data,e=d.ne(true),f=d.Lb(true),g=d.getName(true),h;h=g&&e?g+", "+e:g+e;var i="iconholder";if(!d.Yj()){h='"'+h+'"';i=i+" gphoto-newsubject"}c.innerHTML='<div class="'+i+'"><img src="'+d.xl()+'"></div><div><span>'+f+"</span></div><div>"+h+"</div>";C(c,"gphoto-subjectinfo-row")};
var gI=function(a,b){if(a.nodeType==3){var c,d=null;if(xa(b)){c=b.length>0?b[0]:"";if(b.length>1)d=wc(b,1)}else c=b;if(c.length==0)return;var e=a.nodeValue;c=vb(c);var f=new RegExp("(.*?)("+c+")(.*)","gi"),g=[],h=0,i=f.exec(e);if(i){g.push(i[1]);g.push(i[2]);g.push(i[3]);h=f.lastIndex;a.nodeValue=g[0];var k=this.K.m("span",{"class":"gphoto-subject-match"});if(g[1])this.K.appendChild(k,this.K.createTextNode(g[1]));this.K.appendChild(a.parentNode,k);if(g[2])this.K.appendChild(a.parentNode,this.K.createTextNode(g[2]))}else if(d)this.Rn(a,
d)}else{var n=a.firstChild;while(n){var q=n.nextSibling;this.Rn(n,b);n=q}}},
hI=function(a,b,c){P.call(this);this.nK=b;this.uU=c;this.j=new K(this);this.ag=new el(wu);this.Z(this.ag);this.Bc=new zn(null,null,false);this.r$=new dI;var d=new eI,e=new An(null,d);e.Rn=l(gI,e);e.sda=e.xh;e.xh=l(fI,e,this);this.jb=new yn(this.r$,e,this.Bc);this.Bc.Uy(this.jb);this.jb.X7(true);this.jb.V7(false);this.oT=new Sk(yu);this.To(a)};
o(hI,P);hI.prototype.jG=false;hI.prototype.m=function(){hI.f.m.call(this);this.$(this.e)};
hI.prototype.$=function(a){C(a,"gphoto-subjectinput");var b=v("input",{type:"text"}),c=w("table"),d=w("tbody"),e=w("tr"),f=v("td",{width:"100%"});x(c,d);x(d,e);x(e,f);x(f,b);f=w("td");x(e,f);this.oT.C(f);x(a,c);this.od(a);this.ag.ia(b)};
hI.prototype.n=function(){hI.f.n.call(this);this.Bc.dG(this.ag.A());if(this.jb)this.j.g(this.jb,"update",this.vT);this.j.g(this.oT,xk,function(){this.pG.stop();this.GF=null;this.jb.Io();if(!this.GF)this.Vz();this.GF=null},
false,this);this.j.g(this.ag.A(),ig,function(a){if(!this.pG)this.pG=new Ym(l(this.dispatchEvent,this,a),100);this.pG.start()})};
hI.prototype.u=function(){this.j.ma();this.Bc.NH(this.ag.A());this.ag.qa("");hI.f.u.call(this)};
hI.prototype.To=function(a){if(!a)return;this.r$.To(a)};
hI.prototype.Ax=function(a){this.jea=a};
hI.prototype.C8=function(a){this.nK=a};
hI.prototype.KO=function(a){this.jG=a};
hI.prototype.clear=function(){if(this.ag.A())this.ag.qa("")};
hI.prototype.vT=function(a){a.stopPropagation();a.preventDefault();var b=a.row;if(b&&!b.Yj()){b=null;if(this.jG){this.Vz();return}}if(b&&b.getName()){if(!b.au()){var c=this.cJ(b.getName());b.l8(c)}this.GF=b;this.dispatchEvent({type:Vq,subject:b,clusterBox:this.uU})}else{var d=this.ag.A();$f(d,hg,l(this.Vz,this,b))}};
hI.prototype.lK=function(){Rg(this.g6,0,this)};
hI.prototype.g6=function(){try{this.ag.pX()}catch(a){}};
hI.prototype.cJ=function(a){if(!a)return"";var b=a.indexOf(" "),c=b>=0?a.substr(0,Math.min(15,b)):a;return c};
hI.prototype.Vz=function(a){if(!this.oN)this.oN=new NH;var b,c,d,e=a;if(!e){b=this.ag.L();var f=yo(b);c=f.email;b=f.name;var g=this.cJ(b);e=new U({dispname:g,name:b,email:c})}else d=e.xl();if(this.nK){var h=this.nK();if(h&&h!=d)e.D8(h)}this.dispatchEvent({type:"editopened",subject:e});$f(this.oN,"edit",this.p$,false,this);this.oN.open("edit",[e])};
hI.prototype.p$=function(a){a.stopPropagation();a.preventDefault();var b={type:Vq,contactInformation:a.contactinfo,clusterBox:this.uU,shape:this.jea};if(a.subject)b.subject=a.subject;this.dispatchEvent(b)};
hI.prototype.M=function(a){if(!a)this.jb.ji();M(this.A(),a)}};{var iI=function(a){P.call(this);Ez.Hd(this);this.j=new K(this);this.qda=a;this.B=rr();var b=m("Try It!");this.ez=new Sk(b);this.ez.Jd("gphoto-froptbutton");this.Z(this.ez)};
o(iI,P);iI.prototype.m=function(){this.$(this.K.createElement("div"))};
iI.prototype.$=function(a){iI.f.$.call(this,a);var b=v("div",{"class":"lhcl_FrPromoBox"});v("div",{"class":"lhcl_FrPromoBox-closeBox"});x(a,b);this.ez.C(a)};
iI.prototype.n=function(){iI.f.n.call(this);this.j.g(this.ez,xk,this.Y$)};
iI.prototype.u=function(){iI.prototype.f.u.call(this);this.j.ma()};
iI.prototype.Y$=function(){window.location=this.qda};
var jI=function(a){var b=u("lhid_frBox");if(b){M(b,true);var c=new iI(a);c.ia(b)}}};{var kI=function(a,b,c){QE.call(this,"lhcl_FrStatusBox");this.gg=a;this.XG=b;this.uQ=Ba(c)?c:0;this.B=rr();this.oz=new Ym(this.eaa,5000,this)};
o(kI,QE);var lI=m("Name tags"),mI="InProgress",nI=null;kI.prototype.VP=true;kI.prototype.m=function(){this.od(this.yg(oI));var a=this.qb("contents_");M(a,false);this.Ida=v("img",{id:"lhid_greenbar",width:"1",height:"13",src:"img/greenbar.gif"});var b=this.qb("progressbar_");L(b,"width","150px");x(b,this.Ida);this.pV()};
kI.prototype.n=function(){kI.f.n.call(this);if(this.gg=="OptedIn")this.YE();else{this.p.g(this.B.combined,"albumstatsloaded",this.IQ);this.p.g(this.B.combined,Uq,this.b_);this.B.combined.h3()}this.p.g(this.x4,xk,this.F0)};
kI.prototype.u=function(){kI.f.u.call(this);this.p.ma()};
var pI=function(a,b,c){if(!nI)nI=new kI(a,b,c);return nI};
kI.prototype.pV=function(){this.x4=qI();var a=this.qb("button_");this.x4.C(a);M(a,false)};
kI.prototype.F0=function(){window.location=this.XG};
kI.prototype.b_=function(){this.cT=Ic(this.B.client.Gn()).length;this.YE()};
kI.prototype.eaa=function(){this.B.combined.i3(null,true)};
kI.prototype.Wg=function(){if(this.VP){this.VP=false;var a=this.qb("loading_");M(a,false);var b=this.qb("contents_");M(b,true)}};
kI.prototype.YE=function(){if(this.gg==mI&&!this.oz.zd())this.oz.yI();this.paa();this.IQ();this.Wg()};
kI.prototype.paa=function(){var a=this.qb("headline_");if(this.gg==mI&&this.cT){var b=m("Looking For People...");B(a,b)}else{var c=m("Scan complete!");B(a,c)}};
kI.prototype.IQ=function(){var a=this.qb("thumbnail_");if(!this.Ra){this.Ra=new SE("img/experimental/face.gif",1,60,72);this.Z(this.Ra);this.Ra.C(a)}var b=this.qb("instructions_");if(this.gg==mI&&this.cT>0){var c=0,d=0;Gc(this.B.client.Gn(),function(k){c+=Number(k.numPhotos);d+=k.stats.fullyProcessedPhotos},
this);var e=0;if(c&&c>0)e=d/c;this.vaa(e);var f=m("{$percent}% Complete",{percent:Math.round(e*100)});B(b,f);if(e==1){this.oz.stop();this.gg="OptedIn";this.YE()}else this.oz.start()}else{var g=this.qb("progressbar_");M(g,false);if(!this.uQ)Gc(this.B.client.Gn(),function(k){this.uQ+=Number(k.stats.unnamedShapes)},
this);var h=m("{$faceCount} faces found.",{faceCount:this.uQ});B(b,h);var i=this.qb("button_");M(i,true)}};
kI.prototype.vaa=function(a){var b=u("lhid_greenbar");if(b){var c=Math.round(150*a);L(b,"width",Math.min(c,150)+"px")}};
var rI=function(a,b,c,d){var e=u("lhid_frBox"),f=rr();f.client.CP("uname",a.name);if(e){M(e,true);var g=pI(b,c,d);g.C(e)}},
oI='<div class="{$prefix}"><div class="lhcl_title {$prefix}-title_">'+lI+'</div><div class="{$prefix}-loading_">'+Vx+'</div><table class="{$prefix}-contents_"><tr valign="top"><td><div class="{$prefix}-thumbnail_"></div></td><td><div class="{$prefix}-headline_"></div><div class="{$prefix}-progressbar_"></div><div class="{$prefix}-instructions_"></div><div class="{$prefix}-button_"></div></td></tr></table></div>'};{var EH=function(a,b){QE.call(this,"lhcl_NamePopup");this.Wz=!(!b);this.sz=null;this.wg=null;if(a)this.U(a)};
o(EH,QE);EH.prototype.m=function(){this.od(this.yg('<div class="{$prefix}"><div class="lhcl_RelativeContainer"><div class="{$prefix}-background_"><div class="{$prefix}-contents_"></div></div></div></div>'));this.sz=this.qb("contents_");B(this.sz,this.wg)};
EH.prototype.U=function(a){EH.f.U.call(this,a);var b="";if(a!=null){b=za(a)?a:a.Lb();if(this.Wz&&!a.vf()){var c=m("Click to name");b=c}b=ob(b,55)}this.wg=b;if(this.sz)B(this.sz,this.wg)};
EH.prototype.M=function(a){if(Sa(this.wg))a=false;L(this.A(),"visibility",a?"visible":"hidden")}};{var JH=function(a,b){QE.call(this,"lhcl_NamingPopup");this.V=Ed(this.D());this.Uba=b||this;this.dg=new hI;var c=rr();$f(c.combined,dr,l(function(){this.dg.To(c.client.OZ())},
this));c.combined.co();this.Z(this.dg);this.U(a)};
o(JH,QE);JH.prototype.U=function(a){JH.f.U.call(this,a);this.dg.clear();if(this.k)this.dg.C8(l(this.k.Gl,this.k))};
JH.prototype.Ax=function(a){this.dg.Ax(a)};
JH.prototype.cf=function(){this.dispatchEvent("cancel");this.M(false)};
JH.prototype.m=function(){this.od(this.yg('<div class="{$prefix}"><table class="lhcl_NoSpace"><tr><td><div class="{$prefix}-nameText_"></div></td><td class="lhcl_NamingPopup-quickCancelCell_"><img class="lhcl_NamingPopup-quickCancel_" src="img/tag_delete_normal.png"></td></tr></table></div>'));this.dg.C(this.qb("nameText_"))};
JH.prototype.n=function(){JH.f.n.call(this);this.p.g(this.dg,Vq,this.wq);this.p.g(this.dg,"editopened",l(this.M,this,false));this.p.g(this.qb("quickCancel_"),E,this.cf);this.p.g(this.dg,ig,this.cf);this.dg.lK()};
JH.prototype.u=function(){JH.f.u.call(this);this.p.ma()};
JH.prototype.hasFocus=function(){return this.dg.hasFocus()};
JH.prototype.M=function(a){if(this.e){L(this.e,"visibility",a?"visible":"hidden");this.dg.M(a);if(a)this.dg.lK()}};
JH.prototype.wq=function(a){a.stopPropagation();var b={type:Vq,shape:this.na()};if(a.contactInformation)b.contactInformation=a.contactInformation;else b.subject=a.subject;this.Uba.dispatchEvent(b)}};{var sI=function(a){QE.call(this,"lhcl_SelectionIndicator");this.e5=a};
o(sI,QE);sI.prototype.m=function(){sI.f.m.call(this);C(this.A(),this.O())};
sI.prototype.n=function(){sI.f.n.call(this);this.p.g(this.e5,"selectionchanged",this.c1);this.UQ()};
sI.prototype.u=function(){sI.f.u.call(this);this.p.ma()};
sI.prototype.c1=function(){this.UQ()};
sI.prototype.UQ=function(){var a=this.e5.getSelectedShapes().length,b=m("Faces selected: {$count}",{count:a});B(this.A(),b)}};{var IH=function(a){QE.call(this,"lhcl_ShapeBox");this.V=Ed(this.D());this.x5=new K(this);this.$d=null;this.U(a)};
o(IH,QE);IH.prototype.mj=false;IH.prototype.nv=false;IH.prototype.vc=false;IH.prototype.hr=null;IH.prototype.gr=[0,0];IH.prototype.cf=function(a){this.Yi(false);if(this.mj&&a.target!=this)this.dispatchEvent("cancel")};
IH.prototype.m=function(){this.od(this.yg('<div class="{$prefix}"><div class="lhcl_RelativeContainer lhcl_FillVertical"><div class="{$prefix}-backgroundTopLeft_"></div><div class="{$prefix}-backgroundTopRight_"></div><div class="{$prefix}-backgroundBottomLeft_"></div><div class="{$prefix}-backgroundBottomRight_"></div><div class="{$prefix}-backgroundTop_"></div><div class="{$prefix}-backgroundRight_"></div><div class="{$prefix}-backgroundBottom_"></div><div class="{$prefix}-backgroundLeft_"></div></div></div>'));
this.UE();this.M(false)};
IH.prototype.n=function(){IH.f.n.call(this);this.p.g(this.A(),E,this.hB);this.p.g(this,"cancel",this.cf);this.GE()};
IH.prototype.u=function(){IH.f.u.call(this);this.p.ma();this.qA();this.DQ()};
IH.prototype.MO=function(a){this.mj=a;tf(this.A(),this.O("beingEdited_"),a);this.DQ();this.UE();Rg(function(){this.hB()},
0,this)};
IH.prototype.Ab=function(a){this.nv=a;tf(this.A(),this.O("highlighted_"),a);this.M(true);this.MP(true);this.UE()};
IH.prototype.M=function(a,b){a=this.nv||a;if(this.vc==a&&!b)return;this.vc=a;L(this.A(),"visibility",a?"visible":"hidden");this.MP(a);if(a&&!this.$d instanceof JH)this.Yi(false)};
var KH=function(a,b){var c=a.na().getBounds(),d=b.na().getBounds();if(a.mj)return-1;else if(b.mj)return 1;return xc(c.width*c.height,d.width*d.height)};
IH.prototype.Yi=function(a){a=this.EL()&&a;this.XN();this.Ia().Yi(this.na(),a,this)};
IH.prototype.D=function(){return"gphoto.sapphire.ui.ShapeBox"};
IH.prototype.qA=function(){if(this.$d){this.x5.ma();this.$d.M(false)}};
IH.prototype.yA=function(){var a=this.na(),b=this.Ia().GA(),c=a.getBounds(),d=a.PJ(),e=b.width/d.width;return new ke(c.left*e,c.top*e,c.width*e,c.height*e)};
IH.prototype.hB=function(a){var b=this.Ia();if(!b||b.Xa()!="editor")return;if(a)a.stopPropagation();if(this.EL())this.Yi(true)};
IH.prototype.U_=function(a){a.stopPropagation()};
IH.prototype.V_=function(a){this.qA();this.hr=xh(a,this.Ia().Le());this.IC=this.yA();var b=this.A(),c=b.offsetWidth,d=b.offsetHeight,e=xh(a,b),f=e.x,g=e.y,h=10;this.gr=[f<h?-1:(f>=c-h?1:0),g<h?-1:(g>=d-h?1:0)]};
IH.prototype.W_=function(a){if(!this.hr)return;var b=this.Ia().Le(),c=b.offsetWidth,d=b.offsetHeight,e=xh(a,b);if(e.x<0||e.y<0||e.x>=c||e.y>=d)return;if(this.gr[0]!=0&&this.gr[1]!=0){var f=this.yA().qQ();if(this.gr[0]==-1)f.left=Math.min(f.right-20,e.x);else f.right=Math.max(f.left+20,e.x);if(this.gr[1]==-1)f.top=Math.min(f.bottom-20,e.y);else f.bottom=Math.max(f.top+20,e.y);this.na().OO(new ke(f.left,f.top,f.right-f.left,f.bottom-f.top),new ne(c,d));this.GE()}else{var g=ie(e,this.hr),h=new ke(this.IC.left+
g.x,this.IC.top+g.y,this.IC.width,this.IC.height);if(h.left<0)h.left=0;else if(h.left+h.width>=c)h.left=c-h.width-1;if(h.top<0)h.top=0;else if(h.top+h.height>=d)h.top=d-h.height-1;this.na().OO(h,new ne(c,d));this.GE()}};
IH.prototype.X_=function(){this.hr=null};
IH.prototype.Y_=function(){this.Yi(true);this.hr=null};
IH.prototype.EL=function(){return this.na().dc().H()==tr};
IH.prototype.XN=function(){if(this.$d){this.$d.b();this.$d=null}};
IH.prototype.fO=function(){if(this.$d){var a=this.$d.A(),b=Ah(a),c=Ah(this.e),d=(c.width-b.width)/2,e=c.height;if(this.$d instanceof JH){d+=this.e.offsetLeft;e+=this.e.offsetTop}sh(this.$d.A(),d,e)}};
IH.prototype.MP=function(a){a=this.nv||a;if(a){if(!(this.$d instanceof EH)){if(this.$d)this.XN();this.$d=new EH(this.na().dc(),this.Ia().Xa()=="editor");this.Z(this.$d);this.$d.C(this.A())}this.fO();this.$d.M(true);this.x5.g(this.$d.A(),E,this.hB)}else this.qA()};
IH.prototype.GE=function(){var a=this.yA(),b=this.Ia().GA();a.left+=b.left;a.top+=b.top;var c=this.A();sh(c,a.left,a.top);zh(c,a.width,a.height);this.fO()};
IH.prototype.DQ=function(){if(this.mj&&this.Ha()){if(!this.ui)this.ui=v("div",{"class":this.O("glassPane_")});x(this.A().parentNode,this.ui)}else if(this.ui&&(!this.mj||!this.Ha()))A(this.ui);if(this.ui){var a=this.mj?this.p.g:this.p.Sa;a.call(this.p,this.ui,I,this.V_);a.call(this.p,this.ui,eg,this.Y_);a.call(this.p,this.ui,fg,this.W_);a.call(this.p,this.ui,Hf,this.X_);a.call(this.p,this.ui,E,this.U_)}};
IH.prototype.UE=function(){var a=this.na();tf(this.A(),this.O("noTemplate_"),!a.C1()&&!a.dc().vf()&&!this.mj&&!this.nv)}};{var qF=function(a){QE.call(this,"lhcl_ShapeTagsSection");this.B=rr();this.is=new K(this);this.i5=a};
o(qF,QE);qF.prototype.Fb=false;qF.prototype.m=function(){this.od(this.yg('<div class="{$prefix}"><div class="{$prefix}-peopleTags_"></div></div>'))};
qF.prototype.b=function(){qF.f.b.call(this);this.is.b()};
qF.prototype.n=function(){qF.f.n.call(this);this.p.g(this.B.client,ar,this.Iu);this.p.g(this.B.combined,br,this.O0)};
qF.prototype.u=function(){qF.f.u.call(this);this.p.ma()};
qF.prototype.ZE=function(a,b,c){this.ig=a;this.yw=b;this.Fb=c;this.qb("peopleTags_");if(!this.Fb)M(this.i5,false);this.B.client.Cl()[this.ig];this.B.combined.n3([this.ig],false,!this.Fb&&this.yw)};
qF.prototype.dh=function(){if(this.Fb)this.B.client.dispatchEvent(Tq)};
qF.prototype.v_=function(a){if(this.Fb){var b=l(this.ZE,this,this.ig,this.yw,this.Fb);if(a.dc().H()==tr){var c=m("Do you want to delete this name tag? If the name tag is deleted, the outline box will no longer appear around this face.");if(confirm(c))if(a.UI()=="Auto")this.B.combined.ve(this.B.client.qe()["100004"],[a],b);else this.B.combined.Lw(a,b)}else this.B.combined.ve(this.B.client.qe()[tr],[a],b)}};
qF.prototype.o0=function(a){if(this.Fb){var b="";if(a.vf())b="&sid="+a.dc().H();this.B.client.dispatchEvent({type:$q,shape:a})}else if(a.vf())window.location=a.dc().IZ()};
qF.prototype.C0=function(a){this.B.client.dispatchEvent({type:Yq,shape:a})};
qF.prototype.E0=function(a){this.B.client.dispatchEvent({type:Zq,shape:a})};
qF.prototype.Iu=function(a){if(!a||a.subType=="modify"&&mc(a.photoIds,this.ig))Rg(l(this.ZE,this,this.ig,this.yw,this.Fb),0)};
qF.prototype.O0=function(a){if(a&&a.photoIds[0]!=this.ig)return;this.is.ma();var b=this.qb("peopleTags_");We(b);var c=this.B.client.Cl()[this.ig],d=c&&Ic(c.ac()).sort(tI);if(this.Fb||d&&d.length)M(this.i5,true);if(d&&d.length){r(d,function(f){var g=f.dc(),h=g.Xa();if(f.vf()||h==0){var i=this.yg('<div class="{$prefix}-PersonTag"><img src="img/experimental/person.gif"> <span class="{$prefix}-PersonTag-name_"></span><a class="{$prefix}-PersonTag-delete_ lhcl_tagdelete" href="javascript: void(0);"><img src="img/spacer.gif"></a></div>'),
k=Me(null,this.O("PersonTag-name_"),i)[0],n=m("Find more pictures of {$subject}",{subject:g.Lb()}),q,t=false;if(this.Fb&&f.vf())q=v("a",{href:hH+"&sids="+g.H(),title:n},g.Lb());else{t=true;q=v("span",{"class":"lhcl_fakelink"},g.Lb())}x(k,q);var y=Me(null,this.O("PersonTag-delete_"),i)[0],z;if(h==0){var O=m("Delete this name tag from the photo");z=O}else{var aa=m('Delete the name tag "{$subject}" from the photo',{subject:g.Lb()});z=aa}y.setAttribute("title",z);this.is.g(y,E,l(this.v_,this,f));if(t)this.is.g(q,
I,l(this.o0,this,f));this.is.g(i,Gf,l(this.E0,this,f));this.is.g(i,Hf,l(this.C0,this,f));x(b,i)}},
this);qf(b,this.O("empty_"))}else if(this.Fb&&_features.froptin){var e=m("No people");x(b,v("div",null,e));C(b,this.O("empty_"))}};
var tI=function(a,b){var c=a.dc(),d=b.dc(),e=c.Xa()==0,f=d.Xa()==0;if(e||f)return e&&f?0:(e?1:-1);return Xa(c.Lb(),d.Lb())}};{var uI=function(a,b,c,d){ot.call(this,b,c,d);this.V=Ed(this.D())};
o(uI,ot);var vI=m("name tags"),wI=m("Change"),xI=function(a){return a.na().subject.H()};
uI.prototype.Bp=null;uI.prototype.j=null;uI.prototype.An="";uI.prototype.D=function(){return"gphoto.sapphire.ui.PeopleManagerIcon"};
uI.prototype.n=function(){QD.f.n.call(this);this.j=new K(this);var a=v("table");x(this.e,a);var b=v("tbody");x(a,b);var c=v("tr");x(b,c);var d=v("td");x(c,d);this.Bp=v("input",{type:"checkbox","class":this.db+"-checkbox"});x(d,this.Bp);d=v("td");x(c,d);var e=v("div",{"class":this.db+"-iconicShape"});x(d,e);var f=v("img",{src:this.k.subject.xl(),"class":"lhcl_sp_iconicShape"});x(e,f);this.Pb=v("td",{width:"100%","class":this.db+"-content"});x(c,this.Pb);this.j.g(this.Bp,E,this.j_);this.j.g(f,I,this.xj);
this.j.g(this.e,Gf,l(this.BK,this,true));this.j.g(this.e,Hf,l(this.BK,this,false));this.aa()};
uI.prototype.u=function(){uI.f.u.call(this);if(this.An)H(this.An);this.j.b();delete this.Bp;delete this.Pb};
uI.prototype.aa=function(){var a=!this.k.removed&&!this.k.hidden;if(a!=this.sb()){this.Ca(a);return}var b=this.k.subject,c=this.k.isFavorite,d=this.db+"-favorites",e=b.Lb(),f=ob("View photos of "+e,32),g=ob("Find more photos of "+e,32),h=ob("View "+e+"'s gallery page",32),i={lhui_displayName:p(e),lhui_name:b.getName(true),lhui_email:b.ne(true),lhcl_favoritesClass:d,lhui_peopleUrl:this.k.peopleLink,lhui_numShapes:b.Mj(),lhui_peopleSlideshow:this.k.peopleSlideshow,lhui_seeMore:p(f),lhui_clusterUrl:this.k.clusterLink,
lhui_findMatches:p(g),lhui_gallery:this.k.gallery,lhui_galleryMsg:p(h),lhui_personID:b.H(),lhcl_controlsClass:this.db+"-controls"},k=yI(this.k.ownerView,this.k.isLhUser,c,c?e+" is a favorite.":fx);Uy(this.Pb,gz(k,i));var n=u("lhid_edit_"+b.H());if(n)F(n,E,l(this.dispatchEvent,this,new D("edit",this)));var q=u("lhid_delete_"+b.H());if(q)F(q,E,l(this.dispatchEvent,this,new D("delete",this)));var t=u("lhid_correct_"+b.H());if(t)F(t,E,l(this.dispatchEvent,this,new D("correct",this)));var y=u("lhid_visibility_link_"+
b.H());if(y){F(y,I,l(this.S0,this));this.SQ(b.BL())}var z=Me("div",d,this.Pb)[0];if(this.An){H(this.An);this.An=""}if(z&&!c)this.An=F(z,E,l(this.dispatchEvent,this,new D("addfavorite",this)))};
uI.prototype.BK=function(a){tf(this.e,this.db+"-hover",a)};
uI.prototype.Ab=function(a){uI.f.Ab.call(this,a);if(this.Eb&&a!=this.Eq){var b=this.db+"-selected";tf(this.e,b,a)}};
uI.prototype.Oc=function(a){uI.f.Oc.call(this,a);var b=this.oe();this.Bp.checked=b;this.k.selected=b};
uI.prototype.j_=function(a){this.dispatchEvent(new pt(this,a))};
uI.prototype.S0=function(){var a=rr(),b=!this.k.subject.BL();a.combined.Cm(this.k.subject,b);this.SQ(b)};
uI.prototype.SQ=function(a){var b=u("lhid_visibility_status_"+this.k.subject.H());if(a){qf(b,"hideState");B(b,hy)}else{C(b,"hideState");B(b,iy)}};
uI.prototype.Ca=function(a){uI.f.Ca.call(this,a);M(this.e,a)};
var yI=function(a,b,c,d){var e=["<table><tbody>","<tr>","<td>",'<div class="lhcl_sp_subjectName">',"{lhui_displayName} ({lhui_numShapes})</div>"];if(a){e.push('<div class="lhcl_sp_subjectInfo">{lhui_name}<br/>{lhui_email}</div>');e.push('<div class="lhcl_fakelink" id="lhid_edit_{lhui_personID}"> '+qy+"</div>");if(b){e.push('<div class="{lhcl_favoritesClass}"><img src="img/transparent.gif" class="SPRITE_favorite_white" />');if(c)e.push('<span class="lhcl_peopleManagerIcon-inFavorites">'+d+"</span>");
else e.push('<span class="lhcl_fakelink">'+d+"</span>");e.push("</div>")}}e.push('</td><td width="33%" class="{lhcl_controlsClass}">');e.push('<div><a href="{lhui_peopleUrl}">{lhui_seeMore}</a></div>');if(a){if(b)e.push('<div><a href="{lhui_gallery}">{lhui_galleryMsg}</a></div>');e.push('<div><a href="{lhui_clusterUrl}" class="lhcl_align_right">{lhui_findMatches}</a></div>')}e.push('<div><a href="{lhui_peopleSlideshow}">'+ex+"</a>&nbsp;");e.push('<a href="{lhui_peopleSlideshow}"><img src="img/transparent.gif" class="SPRITE_slideshow_icon" /></a></div>');
e.push('</td><td width="25%" class="{lhcl_controlsClass}">');if(a)e.push('<div class="lhcl_peopleManagerIcon-visibility"><span id="lhid_visibility_status_{lhui_personID}"></span> <span class="nameTags">'+vI+'</span> <span class="lhcl_fakelink" id="lhid_visibility_link_{lhui_personID}">'+wI+'</span></div><div class="lhcl_fakelink" id="lhid_correct_{lhui_personID}"> ',ry,"</div>",'<div class="lhcl_fakelink" id="lhid_delete_{lhui_personID}">',sy,"</div>");e.push("</td></tr></tbody></table>");return e.join("")}};{var BI=function(a,b,c,d,e,f,g){zI=zy(_user.nickname);this.V=Ed(this.D());this.j=new K(this);this.yD=new Ym(this.V0,400,this);var h=gz(AI(),{});x(a,h);this.uda=u("lhid_peopleManager");this.Da=_album.id?_album:null;this.Yd=u("lhid_loading");this.XL=u("lhid_listBody");this.ya=new W(uI,"lhcl_peopleManagerList","lhcl_peopleManagerIcon");this.ya.C(this.XL);this.ya.Ug(3);this.j.g(this.ya,lg,this.Ml);this.j.g(this.ya,"addfavorite",this.N_);this.Bw=-1;this.rN=c;this.wU=d||[];this.lb=e||[];this.uea=f||"";
this.Mca=g||"";this.ef=new NH;this.j.g(this.ef,"edit",this.wK);this.j.g(this.ef,"merge",this.y0);this.j.g(this.ya,"edit",this.JW);this.j.g(this.ya,"delete",this.ZV);this.j.g(this.ya,"correct",this.n_);this.B=rr();var i=this.B.combined;this.j.g(i,dr,this.LV);this.j.g(this.B.server,Xq,this.wi);this.vV(u("lhid_searchControls"));this.XU(u("lhid_buttons"));this.j.g(u("lhid_selectNone"),E,l(this.ya.Pd,this.ya));this.$h=null;this.tg=new nD;this.tg.C(u("lhid_context"));this.Lc(b,this.Da)};
m("Show only people in this album");m("Show only people in my public photos");m("Show all people in my photos");var zI;BI.prototype.kc=[];BI.prototype.Ib=null;BI.prototype.GN="";BI.prototype.Bc=null;BI.prototype.hk=false;BI.prototype.ha=function(){Ez.ee(this);this.j.b();this.ef.b();if(this.yba)H(this.yba);if(this.Bc)this.Bc.b();if(this.yD)this.yD.b()};
BI.prototype.m6=function(){var a=!(!this.Da)?1:0,b=new SG(a,!this.hk,_authuser.isOwner,_user,this.Da);this.tg.J(b)};
BI.prototype.Lc=function(a,b){this.hk=a;this.m6();this.Bw=(this.Bw+1)%this.rN.length;this.Ib.qa("");M(this.XL,false);M(this.Yd,true);of(this.uda,this.hk?"lhcl_ownerView":"lhcl_nonOwner");this.B.client.clearData();var c=this.B.combined;c.co(!a?_user.name:null,null,null,b?[b.id]:null)};
BI.prototype.vV=function(a){var b=m("Find person"),c=v("table",{"class":"lhcl_editbuttons"});x(a,c);var d=v("tbody");x(c,d);var e=v("tr");x(d,e);var f=v("td",{width:"100%"});x(e,f);f=v("td");x(e,f);this.Ib=new el(b);this.Ib.C(f);var g=this.Ib.A();of(g,"lhcl_peopleManagerSearch");L(g,"background-image","url(img/search.gif)");this.Bc=new Ag(g);this.j.g(this.Bc,"input",l(this.yD.start,this.yD,400))};
BI.prototype.XU=function(a){var b=m("Find photos of the selected person(s)."),c=m("View photos");m("Delete the selected persons");var d=m("Merge two selected persons"),e=m("Merge");m("Edit a person's information.");var f=v("div",{"class":"lhcl_editbuttons"}),g=new Sk(yy);g.Jd("gphoto-slideshowbutton");g.Dm(ex);this.j.g(g,xk,l(this.yK,this,true));g.C(f);g.Ca(false);var h={min:1,max:100};this.kc.push({button:g,enable:h});var i=new Sk(c);i.Jd("gphoto-searchbutton");i.Dm(b);this.j.g(i,xk,l(this.yK,this,
false));i.C(f);i.Ca(false);var h={min:1,max:100};this.kc.push({button:i,enable:h});if(_authuser.isOwner){var k=v("span",{"class":"lhcl_ownerOnly"});x(f,k);var n=new Sk(e);n.Jd("gphoto-mergebutton");n.Dm(d);this.j.g(n,xk,l(this.B_,this,"merge"));n.C(k);n.Ca(false);h={min:2,max:2};this.kc.push({button:n,enable:h});k=v("span",{"class":"lhcl_ownerOnly"});x(f,k);this.$h=qI();this.$h.C(k);this.j.g(this.$h,xk,this.Eu)}x(a,f)};
BI.prototype.LV=function(){var a=m('You have no name tags. To start naming the people in your photos, click the "Add name tags" button above.'),b=m("You have no public name tags. Please make sure your albums allow visitors to view the people information."),c=m("There are no public name tags."),d=this.B.client.qq(null,Nr);this.k=new wt(gc(d,l(this.nt,this)));this.ya.U(this.k);M(this.Yd,false);M(this.XL,true);if(!d.length&&!this.Lt){var e;e=this.hk?a:(_authuser.isOwner?b:c);this.Lt=v("div",{},e);x(u("lhid_body"),
this.Lt)}else if(d.length&&this.Lt){A(this.Lt);this.Lt=null}};
BI.prototype.nt=function(a,b){var c=a.H(),d={subject:a,peopleLink:this.rN[this.Bw]+"&sids="+c,hidden:false,removed:false,selected:false};d.peopleSlideshow=d.peopleLink+"#slideshow/";if(this.hk){d.userName=a.mZ();d.isLhUser=a.R2();d.ownerView=true;d.clusterLink=this.wU[this.Bw]+"&sid="+c;if(d.isLhUser){d.gallery="/"+d.userName;var b=yc(this.lb,{name:d.userName},CI);d.isFavorite=b>=0}}return d};
BI.prototype.UN=function(a){var b=[],c=-1,d;for(var e=0;e<a.length;e++){var f=a[e].Eg(),g=this.k.wb(f);g.removed=true;if(c==-1)c=f;else if(d+1!=f){this.k.Hk(c,d);c=f}d=f;b.push(g.subject)}this.k.Hk(c,d);return b};
var CI=function(a,b){return Xa(a.name,b.name)};
BI.prototype.Ml=function(){var a=this.ya.dZ();for(var b=0;b<this.kc.length;b++){var c=this.kc[b];c.button.Ca(a>=c.enable.min&&a<=c.enable.max)}};
BI.prototype.N_=function(a){var b=a.target.na(),c=a.target.Eg();Gj(this.uea+"&starree="+b.userName,l(this.dX,this,c))};
BI.prototype.dX=function(a,b){if(!b.target.Re()){alert(Xw);return}var c=this.k.wb(a);c.isFavorite=true;c.gallery="/"+c.userName;this.k.Hk(a);new iD(this.Mca+"&starree="+c.userName,c.userName);var d={name:c.userName,email:c.email,link:c.gallery};zc(this.lb,d,CI)};
BI.prototype.V0=function(){var a=Va(this.Ib.L()).toLowerCase(),b=-1;if(a!=this.GN){this.GN=a;var c=this.k.S();for(var d=0;d<c;d++){var e=this.k.wb(d),f=[e.subject.Lb()];if(this.hk){f.push(e.subject.getName());f.push(e.subject.ne())}var g=f.join(" ").toLowerCase(),h=false;if(!e.selected&&a&&!ib(g,a))h=true;if(h!=e.hidden){if(b==-1)b=d;e.hidden=h}else if(b!=-1){this.k.Hk(b,d-1);b=-1}}if(b!=-1)this.k.Hk(0,c-1)}};
BI.prototype.Eu=function(){window.location=this.wU[0]};
BI.prototype.yK=function(a){var b=gc(this.ya.oe(),xI),c=this.rN[this.Bw]+"&sids="+b.join(",");if(a)c+="#slideshow";window.location=c};
BI.prototype.JW=function(a){this.ef.open("edit",[a.target])};
BI.prototype.B_=function(a){this.ef.open(a,uc(this.ya.oe()))};
BI.prototype.wK=function(){this.Lc(this.hk);this.ya.Pd()};
BI.prototype.y0=function(a){this.UN(a.remainingSubjects);this.k.Hk(a.target.Eg());this.ya.Pd()};
BI.prototype.ZV=function(a){var b=a.target.na().subject,c=b.Lb()||b.getName()||b.ne(),d=m("Are you sure you want to delete all name tags for {$name}?",{name:c});if(confirm(d)){var e=this.UN([a.target]);this.B.combined.Ri(e)}};
BI.prototype.n_=function(a){var b=a.target.na().subject;if(!this.z$)this.z$=new YH;var c=l(this.caa,this,a.target.Eg());this.z$.wk(b,c)};
BI.prototype.caa=function(a,b){if(!b||b.length==0)return;var c=l(function(){this.Lc(this.hk)},
this);this.B.combined.ve(this.B.client.qe()[tr],b,c)};
BI.prototype.wi=function(){alert(Kv)};
var AI=function(){var a=m("Loading people..."),b=m("Clear selection");return'<div><table width="100%"><tbody><tr><td id="lhid_buttons"></td><td id="lhid_searchControls" width="50%"></td></tr></tbody></table><span class="lhcl_fakelink" id="lhid_selectNone">'+b+'</span><div id="lhid_body" class="lhcl_body"><div id="lhid_loading" style="display: none; text-align: center;"><img src="img/spin.gif" /><span class="lhcl_peopleManagerLoading">'+a+'</span></div><div id="lhid_listBody"></div></div></div>'},
DI=function(a,b,c,d,e,f){var g=u("lhid_leftbox");new BI(g,a&&_authuser.isOwner,b,c,d,e,f)};
BI.prototype.D=function(){return"gphoto.sapphire.ui.PeopleManager"}};{var EI=function(a,b,c,d){ot.call(this,b,c,d)};
o(EI,ot);EI.prototype.zk=true;EI.prototype.columns=3;EI.prototype.u=P.prototype.u;EI.prototype.D=function(){return"gphoto.sapphire.ui.PeopleWidgetIcon"};
EI.prototype.m=function(){this.e=v("td")};
EI.prototype.n=function(){EI.f.n.call(this);var a=v("div",{"class":this.db});x(this.e,a);var b=gz('<div><a href="{lhui_href}"><img src="{lhui_src}" class="lhcl_sp_iconicShape" /></a></div>',{lhui_href:this.k.link,lhui_src:this.k.src});b.setAttribute("title",this.k.name);x(a,b);var c=ob(this.k.name,9,false),d=v("a",{"class":this.db+"-name",href:this.k.link},c);if(c!=this.k.name)d.setAttribute("title",this.k.name);x(a,d);this.dF=true}};{var fH=function(a,b,c,d,e,f){P.call(this);this.j=new K(this);this.ya=new W(EI,"lhcl_peopleWidgetList","lhcl_peopleWidgetIcon");this.Z(this.ya);this.$h=null;if(d!=null&&e){this.$h=qI();this.Z(this.$h)}this.Kc=b;this.Bda=c;this.XG=d;this.kca=a;this.B=rr();var g=this.B.combined,h=e&&_authuser.isOwner;this.Da=f;this.j.g(g,dr,l(function(){this.reload()},
this));g.co(!h?_user.name:null,6,null,f?[f.id]:null);this.j.g(this.B.client,fr,this.e1)};
o(fH,P);fH.prototype.b=function(){if(!this.W()){fH.f.b.call(this);this.j.b();this.ya.b()}};
fH.prototype.D=function(){return"gphoto.sapphire.ui.PeopleWidget"};
fH.prototype.m=function(){fH.f.m.call(this);this.$(this.e)};
fH.prototype.$=function(a){fH.f.$.call(this,a);C(a,"lhcl_sidebox");var b=gz(FI,{lhui_header:this.kca,lhui_href:this.Kc}),c=Me("div","lhcl_peopleWidgetList",b)[0];this.ya.ia(c);if(this.$h!=null)this.$h.C(b);x(a,b)};
fH.prototype.n=function(){fH.f.n.call(this);if(this.$h!=null)this.j.g(this.$h,xk,this.Eu)};
fH.prototype.Eu=function(){window.location=this.XG};
fH.prototype.e1=function(a){if(a&&a.subType=="modify")this.reload()};
fH.prototype.reload=function(){var a=this.B.client.qq(Or,Nr);if(a.length>0){a=wc(a,0,6);var b=new wt(gc(a,this.nt,this));this.ya.U(b)}};
fH.prototype.nt=function(a){return{src:a.xl(),name:a.Lb(false),link:this.Bda+"&sids="+a.H()}};
var FI='<div><div class="lhcl_title" style="position: relative">{lhui_header} <a class="gphoto-viewall" href="{lhui_href}">'+gx+'</a></div><div class="lhcl_peopleWidgetList"/></div>',GI=function(a,b,c,d,e){var f=u("lhid_peopleWidget"),g=_album&&_album.id?_album:null,h=new fH(a,b,c,d,e,g);h.ia(f)}};var qI=function(){var a=m("Add name tags"),b=m("Name people in your photos"),c=new Sk(a);c.Dm(b);c.Jd("gphoto-addnametagsbutton");return c};var HI=function(a){var b=w("img");this.I=b;this.P=a;b.src="img/lab.gif";b.style.position="absolute";b.style.right="0.1em";b.style.top="0.1em";Ch(b,0.5)};
HI.prototype.rc=function(){this.P.style.position="relative";this.P.appendChild(this.I)};
HI.prototype.close=function(){if(this.I)A(this.I)};var cA={};;if(cA){La(Yr+"contextWidget",JG);La(Yr+"view",kH);La(Yr+"toolbar",pH);La(Yr+"peopleWidget",GI);La(Yr+"peopleManager",DI);La(Yr+"frPromoBox",jI);La(Yr+"frStatusBox",rI);La(Yr+"FrOptSettingsHandler",pr);La(Yr+"frOpt",hr);La(Yr+"optOut",ir);La(Yr+"sharePeopleSection",QH);La(Yr+"explore",CH)};var II={ERAS:["BC","AD"],ERANAMES:["Before Christ","Anno Domini"],NARROWMONTHS:["J","F","M","A","M","J","J","A","S","O","N","D"],MONTHS:["January","February","March","April","May","June","July","August","September","October","November","December"],SHORTMONTHS:["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"],WEEKDAYS:["Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"],SHORTWEEKDAYS:["Sun","Mon","Tue","Wed","Thu","Fri","Sat"],NARROWWEEKDAYS:["S","M","T",
"W","T","F","S"],SHORTQUARTERS:["Q1","Q2","Q3","Q4"],QUARTERS:["1st quarter","2nd quarter","3rd quarter","4th quarter"],AMPMS:["AM","PM"],DATEFORMATS:["EEEE, MMMM d, yyyy","MMMM d, yyyy","MMM d, yyyy","M/d/yy"],TIMEFORMATS:["h:mm:ss a v","h:mm:ss a z","h:mm:ss a","h:mm a"],ZONESTRINGS:null};II.STANDALONENARROWMONTHS=II.NARROWMONTHS;II.STANDALONEMONTHS=II.MONTHS;II.STANDALONESHORTMONTHS=II.SHORTMONTHS;II.STANDALONEWEEKDAYS=II.WEEKDAYS;II.STANDALONESHORTWEEKDAYS=II.SHORTWEEKDAYS;II.STANDALONENARROWWEEKDAYS=
II.NARROWWEEKDAYS;si(II,"en");var JI={DECIMAL_SEP:".",GROUP_SEP:",",PERCENT:"%",ZERO_DIGIT:"0",PLUS_SIGN:"+",MINUS_SIGN:"-",EXP_SYMBOL:"E",PERMILL:"\u2030",INFINITY:"\u221e",NAN:"NaN",DECIMAL_PATTERN:"#,##0.###",SCIENTIFIC_PATTERN:"#E0",PERCENT_PATTERN:"#,##0%",CURRENCY_PATTERN:"\u00a4#,##0.00;(\u00a4#,##0.00)",DEF_CURRENCY_CODE:"USD"};JI.MONETARY_SEP=JI.DECIMAL_SEP;JI.MONETARY_GROUP_SEP=JI.GROUP_SEP;ti(JI,"en");
})();
