# Walter SPL sensor

## Short description

This project contains an example of an LTE connected noise level sensor. The 
used sensor module is from PCB Artists and all info can be found on the website:
https://pcbartists.com/product-documentation/decibel-meter-module-interfacing-guide/

The sketch uploads the A-weighted (dbA) sound pressure level and the minimum
and maximum values every minute. The minimum and maximum is reset after every
transmission. The data is uploaded to https://walterdemo.quickspot.io