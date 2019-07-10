const char RebootHtml[] PROGMEM =
" \
<html><head><meta http-equiv=\"refresh\" content=\"15;URL=/\"></head> \
<body> \
%RESULT% \
<p>Rebooting... Wait about <span id='s'>15</span> sec.</p> \
</body> \
<script>function t(){if(s>0)document.getElementById('s').innerHTML = (--s); \
else clearInterval(iID);} \
var s = document.getElementById('s').innerHTML; \
window.iID = setInterval(t, 1000); \
</script></html> \
";

bool do_update = false;
String uprogress;

bool reset_fl = false;
uint32_t maxSketchSpace = 0;

String getUpdateProgress() {return uprogress; }
bool getUpdateStatus() { return do_update; }

String processorUpdate(const String& var)
{
   do
   {
      if(reset_fl) break;
      if(var == "RESULT")
      {
         if(Update.hasError()) return F("<p>Update <b><font color='red'>FAIL!</font></b></p>");
         else return F("<p>Update <b><font color='#00AA00'>Success!</font></b></p>");
      }
   } while(0);
   return String();
}

void handleReboot(AsyncWebServerRequest *request)
{
   reset_fl = true;
   handleReset(request);
}

void handleReset(AsyncWebServerRequest *request)
{
   if(isauth()) { deferredReset(1000); }
   AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", RebootHtml, processorUpdate);
   request->send(response);
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool last)
{
   //Upload handler chunks in data
   if(isauth())
   {
      HardwareSerial *dbg = getDebugPort();
      if(!index)
      { // if index == 0 then this is the first frame of data
         dbg->printf("UploadStart: %s\n", filename.c_str());
         dbg->setDebugOutput(true);
         do_update = true;
         uprogress="0%";

         // calculate sketch space required for the update
         maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
         if(!Update.begin(maxSketchSpace))
         {//start with max available size
            Update.printError(*dbg);
         }
         Update.runAsync(true); // tell the updaterClass to run in async mode
      }

      //Write chunked data to the free sketch space
      if(Update.write(data, len) != len) { Update.printError(*dbg); }
      else
      {
         dbg->printf("Update progress: %uB\n", index);
         uint32_t p = 100*index/maxSketchSpace;
         uprogress = String(p)+"%";
      }

      if(last)
      { // if the final flag is set then this is the last frame of data
         if(Update.end(true))
         { //true to set the size to the current progress
            dbg->printf("Update Success: %u B\nRebooting...\n", index+len);
            uprogress = "100%";
         }
         else
         {
            Update.printError(*dbg);
         }
         dbg->setDebugOutput(false);
      }
   }
}
