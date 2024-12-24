/*
 * Slim email implementation
 *
 * in this file:    GenericEmailSend; sendSlimEmail; ServerResponseAnlysis; sendCommand; base64Encode; 
 *                  ParseReplaceBR; getVersion; EmailBegin;
 *
 * V0 20-XII-2024   []
 * 
 */

#include    "ESP8266WiFi.h"
#include    "WiFiClientSecure.h"
#include    "Arduino.h"
#include    "Clock.h"
#include    "Utilities.h"
#include    "SlimEmail.h"

TimePack  _SysEClock;                // clock data
Clock     _RunEClock(_SysEClock);     // clock instance
Utilities _RunEUtil(_SysEClock);      // Utilities instance
WiFiClientSecure    SlimClient;

// ****************************************************************************************/
SlimEmail::SlimEmail(uint8_t CeaseEmail, uint8_t PrintSession, uint8_t PrintConnection){
    _CeaseEmailSend = CeaseEmail;
    _PrintEmailSession = PrintSession;
    _PrintConnectionTime = PrintConnection;
}   // end of SlimEmail

// ****************************************************************************************/
EmailControl SlimEmail::GenericEmailSend(EmailControl _EmailContIn, TimePack _SysEClock){
    /*
     * method to send an email using retries mechanism
     */
    static const char Mname[] PROGMEM = "GenericEmailSend:";
    static const char ErrorFailTooManyRetries[] PROGMEM = "997 Failed after too many retry attampts";
    static const char SimulatedSend[] PROGMEM = "Email sent (simulation)";
    char responseBuffer[ServerResponseBufLen];              // buffer for serrver's response
    EmailControl _E = _EmailContIn;
    _E.EmailError = false;

    if ( _CeaseEmailSend ) {
         _E.RC = email_sent;
        strcpy_P(_E.ErrorBuffer,SimulatedSend);
        return _E;
    } // end of skip email send
                                                            // initiate session
    if ( _E.ConnectionSessionOn==0 ) {                      // session start
        _E.retries = 0;    
        _E.ConnectionSessionOn = true;
        _E.EmailMillis = millis();
    }   // end of session start check
                                                            // send email
    _E.RC = sendSlimEmail(RECIPIENT_EMAIL, _E.Subject, _E.Body, responseBuffer, _PrintEmailSession);
    #if _DEBUGON==100
        _RunEUtil.InfoStamp(_SysEClock,Mname,nullptr,0,0); Serial.print(F("Slim RC=")); Serial.print(_E.RC); Serial.print(F(" -END\n"));
    #endif  //_DEBUGON==100

                                                            // check results
    if ( _E.RC==email_sent ) {                              // successful send
        _E.ConnectionSessionOn = false;                     // terminate session
        #if _LOGGME==1
            _RunEUtil.InfoStamp(_SysEClock,Mname,nullptr,1,0); Serial.print(F("Email sent successfully! (attampt ")); 
            Serial.print(_E.retries+1); Serial.print(F(" to connect to server). Email send time ")); 
            Serial.print(_RunEClock.ElapseStopwatch(_E.EmailMillis)); Serial.print(F("mS -END\n"));
        #endif  //_LOGGME==1
    } else {                                                // failure to send
        _E.EmailError = true;
        #if _LOGGME==1
            _RunEUtil.InfoStamp(_SysEClock,Mname,nullptr,1,0); Serial.print(F("Attampt ")); 
            Serial.print(_E.retries+1); Serial.print(F(" to send email failed. RC=")); Serial.print(_E.RC); 
            Serial.print(F(" Reason: ")); Serial.print(_E.ErrorBuffer); Serial.print(F(" -END\n"));
        #endif  //_LOGGME==1
                                                            // retries mechanism
        _E.retries++;                                       // retries counter
        if (_E.retries<Max_Retries) {                       // keep trying
            _E.FlagEmailEvent=true;                         // signal to set <SendEmail> by calling method
        } else {                                            // termination (failure)
            _E.ConnectionSessionOn = false;                 // terminate session
            _E.RC = TooManyRetries_Error;
            strcpy_P(_E.ErrorBuffer,ErrorFailTooManyRetries);
            #if _LOGGME==1
                _RunEUtil.InfoStamp(_SysEClock,Mname,nullptr,1,0); 
                Serial.print(F("Error! Connection to SMTP server failed after many attampts. Abort sending email. -END\n "));
            #endif  //_LOGGME==1
        }   // end of retry mechanism

    }   // end of connection test

    return  _E;
}   // end of GenericEmailSend

//****************************************************************************************/
uint8_t SlimEmail::sendSlimEmail(const char* recipient, char* Msg_Subject, char* Msg_Body, char* responseBuffer, bool PrintEcho) {
    /*
     * method to send a slim and simple email
     * returns  1 - OK (email_sent)
     *          0 - failed to connect (Could_not_connect)
     *          2 - authentification error (Authentication_Error)
     *          3 - content error (Email_Content_Error)
     *          4 - (Send_Error)
     */
    SlimClient.setBufferSizes(512, 512);
                                                                        // Connect to SMTP server
    SlimClient.setInsecure();                                           // Bypass SSL verification for testing
    if (SlimClient.connect(SMTP_HOST, SMTP_PORT)!=0) {                  // successful connection
        if(PrintConnectionTime==1) Serial.print(F("\tsendSlimEmail: Connected to SMTP server -END\n"));
    } else {                                                            // failed to connect
        if(PrintConnectionTime==1){
            Serial.print(F("\tsendSlimEmail: Failed to connected to SMTP server. ")); 
            Serial.printf(" LastSSLError error: %d", SlimClient.getLastSSLError()); Serial.print(F(" -END\n"));
        }
        static const char FailToConnect[] PROGMEM = "998 Failed to connected to SMTP server.";
        strcpy_P(responseBuffer,FailToConnect);                         // error indication
        return  Could_not_connect;                                      // indicate failure
    }   // end off connection
    
                                                                        // Send EHLO
    responseBuffer = sendCommand("EHLO IoT.Server",1,responseBuffer,ServerResponseBufLen,PrintEcho);
    if ( ServerResponseAnlysis(responseBuffer) != 1 ) return Could_not_connect;
                                                                        // Start TLS
    responseBuffer = sendCommand("STARTTLS",1,responseBuffer,ServerResponseBufLen,PrintEcho);
    if ( ServerResponseAnlysis(responseBuffer) != 1 ) return Could_not_connect;
                                                                        // Authentication (Base64 Encode)
    responseBuffer = sendCommand("AUTH LOGIN",1,responseBuffer,ServerResponseBufLen,PrintEcho);
    if ( ServerResponseAnlysis(responseBuffer) != 1 ) return Could_not_connect;
    char* base64buffer = new char[128];                                 // buffer to encode
        base64Encode(AUTHOR_EMAIL, base64buffer, 128);
        responseBuffer =                                                // Send encoded email
        sendCommand(base64buffer,1,responseBuffer,ServerResponseBufLen,PrintEcho);
        if ( ServerResponseAnlysis(responseBuffer) != 1 ) {
                                                    delete [] base64buffer;
                                                    return Authentication_Error;}
        base64Encode(AUTHOR_PASSWORD, base64buffer, 128);
        responseBuffer =                                                // Send encoded password
        sendCommand(base64buffer,1,responseBuffer,ServerResponseBufLen,PrintEcho);
        if ( ServerResponseAnlysis(responseBuffer) != 1 ) {
                                                    delete [] base64buffer;
                                                    return Authentication_Error;}
    delete [] base64buffer;
                                                                        // Mail From and TO
    char* mailPayloadBuffer = new char[EmailPayloadBufferSize];         // buffer to manage addresses etc
        snprintf(mailPayloadBuffer, EmailPayloadBufferSize, "MAIL FROM:<%s>", AUTHOR_EMAIL);
        responseBuffer = sendCommand(mailPayloadBuffer,1,responseBuffer,ServerResponseBufLen,PrintEcho);
        if ( ServerResponseAnlysis(responseBuffer) != 1 ) {
                                                    delete [] mailPayloadBuffer;
                                                    return Authentication_Error; }
        snprintf(mailPayloadBuffer, EmailPayloadBufferSize, "RCPT TO:<%s>", recipient);
        responseBuffer = sendCommand(mailPayloadBuffer,1,responseBuffer,ServerResponseBufLen,PrintEcho);    
        if ( ServerResponseAnlysis(responseBuffer) != 1 ) {
                                                    delete [] mailPayloadBuffer;
                                                    return Authentication_Error; }
                                                                        // Data Section
        responseBuffer = sendCommand("DATA",1,responseBuffer,ServerResponseBufLen,PrintEcho);
        if ( ServerResponseAnlysis(responseBuffer) != 1 ) {
                                                    delete [] mailPayloadBuffer;
                                                    return Email_Content_Error; }
        snprintf(mailPayloadBuffer, EmailPayloadBufferSize, "From: \"%s\" <%s>", AUTHOR_NAME,AUTHOR_EMAIL);
        responseBuffer = sendCommand(mailPayloadBuffer,0,responseBuffer,ServerResponseBufLen,PrintEcho);
        snprintf(mailPayloadBuffer, EmailPayloadBufferSize, "To: \"%s\" <%s>", RECIPIENT_NAME,RECIPIENT_EMAIL);
        responseBuffer = sendCommand(mailPayloadBuffer,0,responseBuffer,ServerResponseBufLen,PrintEcho);
        snprintf(mailPayloadBuffer, EmailPayloadBufferSize, "Subject: %s", Msg_Subject);
        responseBuffer = sendCommand(mailPayloadBuffer,0,responseBuffer,ServerResponseBufLen,PrintEcho);
    delete  [] mailPayloadBuffer;
    responseBuffer = sendCommand("\r\n",0,responseBuffer,ServerResponseBufLen,PrintEcho);// terminate header
    Msg_Body = ParseReplaceBR(Msg_Body);                                // replace <br> with CRLF
    responseBuffer = sendCommand(Msg_Body,0,responseBuffer,ServerResponseBufLen,PrintEcho);
    responseBuffer = sendCommand(".",1,responseBuffer,ServerResponseBufLen,PrintEcho);   //  to indicate the end of the message
    if ( ServerResponseAnlysis(responseBuffer) != 1 ) {
                                                    //delete [] responseBuffer;
                                                    return Email_Content_Error;}

    // End the email
    responseBuffer = sendCommand("QUIT",1,responseBuffer,ServerResponseBufLen,PrintEcho);
    if ( ServerResponseAnlysis(responseBuffer) != 1 ) {
                                                    //delete [] responseBuffer;
                                                    return Send_Error;}
    return  email_sent;
}   // end of sendSlimEmail

#if BACKUPVERSION==1
//****************************************************************************************/
uint8_t SlimEmail::sendSlimEmailOriginal(const char* recipient, char* subject, char* body,TimePack _SysEClock, bool PrintEcho) {
    /*
     * method to send a slim and simple email
     * returns  1 - OK
     *          0 - failed to connect
     */
    static const char Mname[] PROGMEM = "sendSlimEmailOriginal:";
    char    base64buffer[128];                                          // temp buffer to encode
    char    mailPayloadBuffer[256];                                     // temp buffer to manage addresses etc
    #define Retry_Delay 2000
     
    SlimClient.setBufferSizes(512, 512);

    #ifdef  ConnectUningGmailIP                                         // how to get Gmail sever IP 74.125.206.108
        IPAddress smtpIP;
        if (WiFi.hostByName("smtp.gmail.com", smtpIP)) {
            _RunEUtil.InfoStamp(_SysEClock,Mname,nullptr,1,0); Serial.print(F("Gmail SMTP server IP is "));
            Serial.println(smtpIP);
    }
    #endif  //ConnectUningGmailIP

    unsigned long CurrentMillis_t;
    CurrentMillis_t = _RunEClock.StartStopwatch();

    #define ChatGPTexample  0
    #if ChatGPTexample==1
        // Connect to SMTP server
        SlimClient.setInsecure();  // Skip certificate verification
        if (!SlimClient.connect("smtp.gmail.com", 465)) {
            Serial.println("Connection to SMTP server failed.");
            Serial.printf("Error: %d\n", SlimClient.getLastSSLError());
            return 0;
        }
        Serial.println("Connected to SMTP server.");

        // Send EHLO
        sendCommand("EHLO esp8266.local",1,PrintEcho);

        // Start TLS
        sendCommand("STARTTLS",1),PrintEcho;

        // Authentication (Base64 Encode)
        sendCommand("AUTH LOGIN",1,PrintEcho);
        base64Encode(AUTHOR_EMAIL, base64Email, sizeof(base64Email));
        base64Encode(AUTHOR_PASSWORD, base64Password, sizeof(base64Password));

        sendCommand(base64Email,1,PrintEcho);     // Send encoded email
        sendCommand(base64Password,1,PrintEcho);  // Send encoded password

        // Mail From
        sendCommand("MAIL FROM:<your-email@gmail.com>",1,PrintEcho);
        
        // Recipient
        char mailToCommand[256];
        snprintf(mailToCommand, sizeof(mailToCommand), "RCPT TO:<%s>", recipient);
        sendCommand(mailToCommand,1,PrintEcho);
        
        // Data Section
        sendCommand("DATA",1,PrintEcho);
        sendCommand("Subject: Test Email from ESP8266\r\n\r\nHello from ESP8266!\r\n.",1,PrintEcho);
    #endif  //ChatGPTexample

    // Connect to SMTP server
    bool    connected = false;
    SlimClient.setInsecure();                                           // Bypass SSL verification for testing
    for (uint8_t ii=0; ii<Max_Retries; ii++) {
        if (SlimClient.connect(SMTP_HOST, SMTP_PORT)!=0) {
            connected = true;
            #if PrintConnectionTime==1
                _RunEUtil.InfoStamp(_SysEClock,Mname,nullptr,1,0); Serial.print(F("Connected to SMTP server (try=")); Serial.print(ii); 
                Serial.print(F("). ")); Serial.print(_RunEClock.ElapseStopwatch(CurrentMillis_t)); Serial.print(F("mS -END\n"));
            #endif  //PrintConnectionTime
            break;
        } else {
            #if PrintConnectionTime==1
                _RunEUtil.InfoStamp(_SysEClock,Mname,nullptr,1,0); Serial.print(F("Failed to connected to SMTP server. Try no. ")); 
                Serial.print(ii); Serial.printf(" Error: %d", SlimClient.getLastSSLError());
                Serial.print(_RunEClock.ElapseStopwatch(CurrentMillis_t)); Serial.print(F("mS -END\n"));
            #endif  //PrintConnectionTime
            delay(Retry_Delay);
        }       // end connect check
    }   // end of retry loop
    if ( !connected ) {
        _RunEUtil.InfoStamp(_SysEClock,Mname,nullptr,1,0); Serial.print(F("Connection to SMTP server failed after many tries. Abort sending. "));
        Serial.printf("Error: %d\n", SlimClient.getLastSSLError());
        return  0;
    }                                                                   // Send EHLO
    sendCommandOriginal("EHLO IoT.Server",1,PrintEcho);
                                                                        // Start TLS
    sendCommandOriginal("STARTTLS",1,PrintEcho);
                                                                        // Authentication (Base64 Encode)
    sendCommandOriginal("AUTH LOGIN",1,PrintEcho);
    base64Encode(AUTHOR_EMAIL, base64buffer, sizeof(base64buffer));
    sendCommandOriginal(base64buffer,1,PrintEcho);                              // Send encoded email
    base64Encode(AUTHOR_PASSWORD, base64buffer, sizeof(base64buffer));
    sendCommandOriginal(base64buffer,1,PrintEcho);                              // Send encoded password

                                                                        // Mail From and TO
    snprintf(mailPayloadBuffer, 256/*sizeof(mailPayloadBuffer)*/, "MAIL FROM:<%s>", AUTHOR_EMAIL);
    sendCommandOriginal(mailPayloadBuffer,1,PrintEcho);
    snprintf(mailPayloadBuffer, 256/*sizeof(mailPayloadBuffer)*/, "RCPT TO:<%s>", recipient);
    sendCommandOriginal(mailPayloadBuffer,1,PrintEcho);

    #if PrintConnectionTime==1
        _RunEUtil.InfoStamp(_SysEClock,Mname,nullptr,1,0); Serial.print(F("Post \"Mail\" command ")); 
        Serial.print(_RunEClock.ElapseStopwatch(CurrentMillis_t)); Serial.print(F("mS -END\n"));
    #endif  //PrintConnectionTime
    
                                                                        // Data Section
    sendCommandOriginal("DATA",1,PrintEcho);
    snprintf(mailPayloadBuffer, sizeof(mailPayloadBuffer), "From: \"%s\" <%s>", AUTHOR_NAME,AUTHOR_EMAIL);
    sendCommandOriginal(mailPayloadBuffer,0,PrintEcho);
    snprintf(mailPayloadBuffer, sizeof(mailPayloadBuffer), "To: \"%s\" <%s>", RECIPIENT_NAME,RECIPIENT_EMAIL);
    sendCommandOriginal(mailPayloadBuffer,0,PrintEcho);
    snprintf(mailPayloadBuffer, sizeof(mailPayloadBuffer), "Subject: %s", subject);
    sendCommandOriginal(mailPayloadBuffer,0,PrintEcho);
    sendCommandOriginal("\r\n",0,PrintEcho);                              // terminate header
    body = ParseReplaceBR(body);                                  // replace <br> with CRLF
    sendCommandOriginal(body,0,PrintEcho);
    sendCommandOriginal(".",1,PrintEcho);                                 //  to indicate the end of the message
    #if PrintConnectionTime==1
        _RunEUtil.InfoStamp(_SysEClock,Mname,nullptr,1,0); Serial.print(_RunEClock.ElapseStopwatch(CurrentMillis_t)); Serial.println("mS -END");
    #endif  //PrintConnectionTime

    // End the email
    sendCommandOriginal("QUIT",1,PrintEcho);
    _RunEUtil.InfoStamp(_SysEClock,Mname,nullptr,1,0); Serial.print(F("Email sent time ")); Serial.print(_RunEClock.ElapseStopwatch(CurrentMillis_t)); Serial.print(F("mS -END\n"));
    return  1;
}   // end of sendSlimEmailOriginal

//****************************************************************************************/
void SlimEmail::sendCommandOriginal(char* cmd, bool Wait4Response, bool PrintEcho) {
    /*
     * method to send command to Wifi client and read the response
     * SMTP and similar line-based protocols require lines to be terminated with CRLF
     * 
     * cmd  - the command string
     * Wait4Response    - if set, whait for the server's response
     *                  - else, just send CRLF (for partial command sending)
     * PrintEcho        - if set, print the dialogue (both sides)
     */
    SlimClient.print(cmd);                                          // Send command
    if ( Wait4Response )    {                                       // full command
        SlimClient.print("\r\n");                                   // send command termination
        if ( PrintEcho ) {                                          // echo on terminal
            Serial.print("\t>");
            Serial.print(cmd);
            Serial.print("\n");
        }   // end of echo
                                                                    // get server's response
        while (!SlimClient.available()) { delay(50); }              // wait
        while (SlimClient.available()) {
            String response = SlimClient.readStringUntil('\n');     // read response
            if ( PrintEcho ) {                                      // echo response
                Serial.print("\t");
                Serial.print(response);
                Serial.print("\n");
            }
        }
    } else {                                                        // partial command send line termination
        SlimClient.print("\r\n");
    }   // end of Wait4Response
}       // end of sendCommandOriginal

#endif  //BACKUPVERSION==1
//****************************************************************************************/
uint8_t SlimEmail::ServerResponseAnlysis(char* buffer){
    /*
     * method to analysis Gamail Server resonse via observation of the response <buffer>
     * retuns   1 - OK
     *          0 - timeout before the server responded
     *          2 - authentification failure
     * 
     * eg. of error message: 535-5.7.8 Username and Password not accepted. For more information, go to
     * eg. of accet message: 235 2.7.0 Accepted
     */
    uint8_t RC=1;
    if ( buffer[0]==0x00) RC=0; // 999 timeout
    if ( buffer[0]=='2' && buffer[1]=='2' && buffer[2]=='1' ) RC=1; // 221 2.0.0 closing connection 
    if ( buffer[0]=='2' && buffer[1]=='3' && buffer[2]=='5' ) RC=1; // 235 2.7.0 Accepted
    if ( buffer[0]=='2' && buffer[1]=='5' && buffer[2]=='0' ) RC=1; // 250-smtp.gmail.com at your service, [62.56.147.2]
    if ( buffer[0]=='3' && buffer[1]=='3' && buffer[2]=='4' ) RC=1; // 334 VXNlcm5hbWU6
    if ( buffer[0]=='3' && buffer[1]=='5' && buffer[2]=='4' ) RC=1; // 354 Go ahead
    if ( buffer[0]=='5' && buffer[1]=='3' && buffer[2]=='0' ) RC=1; // 530-5.7.0 Authentication Required. For more information, go to
    if ( buffer[0]=='5' && buffer[1]=='3' && buffer[2]=='5' ) RC=2; // 535-5.7.8 Username and Password not accepted. For more information, go to
    if ( buffer[0]=='9' && buffer[1]=='9' && buffer[2]=='9' ) RC=0; // 999 timeout
    return  RC;                                                          // OK
}   // end of ServerResponseAnlysis

//****************************************************************************************/
char* SlimEmail::sendCommand(char* cmd, bool Wait4Response, char* buffer, uint16_t bufferSize, bool PrintEcho) {
    /*
     * method to send command to Wifi client and read the response
     * SMTP and similar line-based protocols require lines to be terminated with CRLF
     * 
     * cmd              - the command string
     * Wait4Response    - if set, whait for the server's response
     *                  - else, just send CRLF (for partial command sending)
     * buffer           - to store the server's response
     *                    for timeout, buffer is cleared
     * bufferSize       - size of <buffer>
     * PrintEcho        - if set, print the dialogue (both sides)
     * returns          - <buffer> usually with the server's response.
     *                  - for servers' timeout situation, buffer="999 timeout"
     */
    static const char ErrorTimeOut[] PROGMEM = "999 Timeout Error";
    int16_t index;
    SlimClient.print(cmd);                                          // Send command
    if ( PrintEcho ) {                                              // echo on terminal
        Serial.print("\t>");
        index = strlen(cmd);
        for (int16_t ii=0;ii<index;ii++){                           // print cmd, replace LF with LF+TAB
            if ( cmd[ii] == '\r')   ;                               // skip CR
            else if ( cmd[ii]=='\n' ) Serial.print("\n\t");         // replace LF with LF+TAB
            else Serial.print(cmd[ii]);
        }
    }   // end of echo
    if ( Wait4Response )    {                                       // full command
        SlimClient.print("\r\n");                                   // send command termination
        if ( PrintEcho ) Serial.print("\n\t");                      // echo on terminal
                                                                    // get server's response
        memset(buffer, 0, bufferSize);                              // Clear the buffer
        index = 0;
        unsigned long BeginWait = millis();
        while (!SlimClient.available()) {                           // wait for server
            delay(50);
            if ( millis()-BeginWait>EmailServerConnectionTimeout) { // timeout triggered
                if ( PrintEcho ) {                                  // echo on terminal
                    Serial.print("\tError: Wait for server time out!!! ");  
                    Serial.print(millis() - BeginWait); Serial.print("mS -END\n");
                }   // end of echo
                strcpy_P(buffer,ErrorTimeOut);                      // <buffer> with timeout error indication
                return buffer; 
            }   // end of timeout
        }   // end of wait
        while (SlimClient.available()) {                            // collect server's response  
            char c = SlimClient.read();
                if (index < bufferSize - 1) {                       // Prevent overflow
                    buffer[index++] = c;
                    if ( PrintEcho ) {                              // echo response
                        if ( c == '\r')   ;                         // skip CR
                        else if (c=='\n') Serial.print("\n\t");     // replace LF with LF+TAB
                        else Serial.print(c);
                    }   // end of echo
                }   // end of char read
        }   // end of char available
        
        buffer[index] = '\0';                                       // Null-terminate the buffer
        if ( PrintEcho ) Serial.print("\n");    
        
    } else {                                                        // partial command send line termination
        SlimClient.print("\r\n");
        if ( PrintEcho ) Serial.print("\n");   
    }   // end of Wait4Response
    return buffer;
}       // end of sendCommand

//****************************************************************************************/
char*    SlimEmail::ParseReplaceBR(char* buff) {
    /*
     * method to parse a buffer for <br> or <BR> and replace (override) with CRLF
     */
    uint16_t    BufLen = strlen(buff);
    char*   ptrIn = buff;
    char*   ptrOut = buff;
    uint16_t ii=0;
    while ( ii<BufLen ) {                                       // for <br> replace
        if (  *ptrIn=='<' && ( *(ptrIn+1)=='b' || *(ptrIn+1)=='B' ) && 
        ( *(ptrIn+2)=='r' || *(ptrIn+2)=='R') && *(ptrIn+3)=='>' ) {
            *ptrOut++ = '\r';
            *ptrOut++ = '\n';
            ptrIn = ptrIn+4;
            ii = ii+4;
        }   else {                                              // regular char
            *ptrOut++ = *ptrIn++;                               // overrride
            ii++;    
        }
    }   // end of buff loop
    *ptrOut=0x00;                                               // new buffer termination
    return  buff;
}   // end of ParseReplaceBR

//****************************************************************************************/
void SlimEmail::base64Encode(const char* input, char* output, size_t outputSize) {
    /* 
     * method to encode (base64) without use of String class nor "base64.h"
     */
    const char base64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t inputLen = strlen(input);
    size_t outputLen = 4 * ((inputLen + 2) / 3);    // Calculate required output size
    
    if (outputSize < outputLen + 1) {               // Ensure output buffer is large enough
        Serial.println("Output buffer is too small for Base64 encoding.");
        return;
    }
    
    size_t i = 0, j = 0;
    for (; i < inputLen - 2; i += 3) {
        output[j++] = base64Table[(input[i] >> 2) & 0x3F];
        output[j++] = base64Table[((input[i] & 0x03) << 4) | ((input[i+1] >> 4) & 0x0F)];
        output[j++] = base64Table[((input[i+1] & 0x0F) << 2) | ((input[i+2] >> 6) & 0x03)];
        output[j++] = base64Table[input[i+2] & 0x3F];
    }

    if (i < inputLen) {
        output[j++] = base64Table[(input[i] >> 2) & 0x3F];
        if (i == inputLen - 1) {
            output[j++] = base64Table[((input[i] & 0x03) << 4)];
            output[j++] = '=';
        } else {
            output[j++] = base64Table[((input[i] & 0x03) << 4) | ((input[i+1] >> 4) & 0x0F)];
            output[j++] = base64Table[((input[i+1] & 0x0F) << 2)];
        }
        output[j++] = '=';
    }
    output[j] = '\0';                                   // Null-terminate
}       // end of base64Encode

// ****************************************************************************************/
const   char* SlimEmail::getVersion() {
    /*
     * method to return the lib's version
     */
    return  SimpleEmailVersion;
}   // end of getVersion

// ****************************************************************************************/
EmailControl SlimEmail::EmailBegin(EmailControl InControl, char* buffer, uint8_t DefaultEmailType){
    /*
     * method to initiate email control structure
     */
    static const char DefaultSpecialMessage[] PROGMEM = "Default special message";
    EmailControl _E=InControl;
    _E.EmailType = DefaultEmailType;                           // default type
    _E.Payload = 0;
    _E.retries = 0;
    _E.ConnectionSessionOn = 0;
    _E.FlagEmailEvent = false;
    _E.EmailError = false;
    _E.Body = nullptr;
    _E.Subject  = nullptr;
    _E.ErrorBuffer = nullptr;
    _E.SpecialMessageBuffer = buffer;
    strcpy_P(_E.SpecialMessageBuffer,DefaultSpecialMessage);
    _E.TimeSinceStarted = millis();
    return  _E;
}   // end of EmailBegin

//****************************************************************************************/