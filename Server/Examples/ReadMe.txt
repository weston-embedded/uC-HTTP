Common Folder:
**************
This folder contain the web resources (HTML, css, images, etc) used for most of the example applications.


Example Folders:
****************

Basic:
------ 
This example is a basic web server application. An introduction page can be fetch from a web browser. The hook
functions in this example are used to do dynamic content with token replacement, do redirecting, parse form data
and construct custom response body in JSON. 
No add-ons are used in this example.

NoFS:
-----
This simple example shows how to configure and start an HTTP server instance with no File System. When the default
page is fetch from a web browser, the server stack will answer with a simple "hello word".

REST:
-----
This example is a RESTful application to view, add, modify and delete resources (simply a users list in this
example). It uses the REST add-on module. All the information passed between the HTTP client and server are 
constructed in JSON.

CtrlLayer:
----------
This example combine a login barrier (using the Auth module) with the Basic and REST applications all in one 
example through the Control Layer module.

SSL-TLS:
-------- 
This folder is not an example application but only an example of HTTP server instance configuration and hook
functions for a secure HTTP application. 


Notes:
******
1) The REST example make use of the JQuery libraries. Therefore the host used as the HTTP client most also have
   access to the Internet to fetch those library files.

2) To further test your HTTP server application or to simply upload files to your server, we recommend the POSTMAN
   add-on of Chrome. It allows to construct your custom HTTP requests.

3) Those examples have been tested with Firefox and Chrome. There is no guarantee that they will work with other 
   web browsers since some JavaScript codes included have not been check to be all platform supporting. 