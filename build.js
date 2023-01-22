var fs = require('fs');
var html = fs.readFileSync('index.html','utf8');

// converting the HTML file in WEBPAGE.h file that the arduino could work with.
var escaped = html.replace(/\\/g,'\\\\').replace(/\"/g,'\\"').replace(/[\n\t]/g,'');

var hFile = `/*
This file has been generated by build.js javascript file.
You need NODEJS to run the JS file. 
Go to the Project folder in CMD. Use command: node build.js
*/
const char* WEBPAGE="${escaped}";`

fs.writeFileSync('WEBPAGE.h',hFile);